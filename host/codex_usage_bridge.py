#!/usr/bin/env python3
"""Send the current Codex rate-limit snapshot to an Eyelash Sofle keyboard."""

from __future__ import annotations

import argparse
import errno
import glob
import json
import os
import selectors
import shutil
import signal
import subprocess
import sys
import termios
import time
import tty
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path
from typing import Any


UNKNOWN_PERCENT = 0xFF
PROBE = b"CX1?\n"


def crc8(data: bytes) -> int:
    value = 0
    for byte in data:
        value ^= byte
        for _ in range(8):
            value = ((value << 1) ^ 0x07) & 0xFF if value & 0x80 else (value << 1) & 0xFF
    return value


def _remaining_percent(window: dict[str, Any] | None) -> int:
    if not window or window.get("usedPercent") is None:
        return UNKNOWN_PERCENT
    used = max(0, min(100, int(window["usedPercent"])))
    return 100 - used


def _duration_hours(window: dict[str, Any] | None) -> int:
    if not window or window.get("windowDurationMins") is None:
        return 0
    return max(0, min(255, round(int(window["windowDurationMins"]) / 60)))


def _reset_parts(window: dict[str, Any] | None) -> tuple[int, int, int, int]:
    if not window or window.get("resetsAt") is None:
        return 0, 0, 0, 0
    reset = datetime.fromtimestamp(int(window["resetsAt"]))
    return reset.month, reset.day, reset.hour, reset.minute


@dataclass(frozen=True)
class PackedSnapshot:
    param1: int
    param2: int
    remaining_percent: int

    def wire_line(self) -> bytes:
        payload = f"CX1,{self.param1:08X},{self.param2:08X}".encode("ascii")
        return payload + f"*{crc8(payload):02X}\n".encode("ascii")


def pack_snapshot(snapshot: dict[str, Any], now: float | None = None) -> PackedSnapshot:
    primary = snapshot.get("primary")
    remaining = _remaining_percent(primary)
    month, day, hour, minute = _reset_parts(primary)
    param1 = remaining | (_duration_hours(primary) << 8)
    param2 = month | (day << 8) | (hour << 16) | (minute << 24)
    return PackedSnapshot(param1, param2, remaining)


def find_codex() -> str:
    configured = os.environ.get("CODEX_BIN")
    home = Path.home()
    candidates = [
        configured,
        shutil.which("codex"),
        "/Applications/Codex.app/Contents/Resources/codex",
        "/Applications/ChatGPT.app/Contents/Resources/codex",
        str(home / "Applications/Codex.app/Contents/Resources/codex"),
        str(home / "Applications/ChatGPT.app/Contents/Resources/codex"),
    ]
    for candidate in candidates:
        if candidate and Path(candidate).is_file():
            return candidate
    raise FileNotFoundError("找不到 Codex CLI；请设置 CODEX_BIN=/path/to/codex")


class CodexAppServer:
    def __init__(self, timeout: float = 30.0) -> None:
        self.timeout = timeout
        self.process: subprocess.Popen[str] | None = None
        self.next_id = 1

    def close(self) -> None:
        if self.process and self.process.poll() is None:
            self.process.terminate()
            try:
                self.process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                self.process.kill()
        self.process = None

    def start(self) -> None:
        self.close()
        self.process = subprocess.Popen(
            [find_codex(), "app-server", "--stdio"],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True,
            bufsize=1,
        )
        self._request(
            "initialize",
            {
                "clientInfo": {"name": "eyelash-sofle-codex-usage", "version": "1.0.0"},
                "capabilities": {"experimentalApi": True},
            },
        )

    def _request(self, method: str, params: Any = None) -> dict[str, Any]:
        if not self.process or self.process.poll() is not None:
            raise RuntimeError("Codex app-server 未运行")
        request_id = self.next_id
        self.next_id += 1
        message: dict[str, Any] = {"id": request_id, "method": method}
        if params is not None:
            message["params"] = params
        assert self.process.stdin and self.process.stdout
        self.process.stdin.write(json.dumps(message, separators=(",", ":")) + "\n")
        self.process.stdin.flush()

        selector = selectors.DefaultSelector()
        selector.register(self.process.stdout, selectors.EVENT_READ)
        deadline = time.monotonic() + self.timeout
        try:
            while time.monotonic() < deadline:
                ready = selector.select(max(0, deadline - time.monotonic()))
                if not ready:
                    break
                line = self.process.stdout.readline()
                if not line:
                    break
                try:
                    response = json.loads(line)
                except json.JSONDecodeError:
                    continue
                if response.get("id") != request_id:
                    continue
                if "error" in response:
                    raise RuntimeError(response["error"].get("message", str(response["error"])))
                return response["result"]
        finally:
            selector.close()
        raise TimeoutError(f"等待 Codex app-server 的 {method} 响应超时")

    def rate_limits(self) -> dict[str, Any]:
        if not self.process or self.process.poll() is not None:
            self.start()
        result = self._request("account/rateLimits/read")
        buckets = result.get("rateLimitsByLimitId") or {}
        return buckets.get("codex") or result["rateLimits"]


class SerialPort:
    def __init__(self, port: str, probe_timeout: float = 1.0) -> None:
        self.requested_port = port
        self.probe_timeout = probe_timeout
        self.fd: int | None = None
        self.path: str | None = None

    def close(self) -> None:
        if self.fd is not None:
            os.close(self.fd)
        self.fd = None
        self.path = None

    @staticmethod
    def candidates() -> list[str]:
        patterns = (
            "/dev/cu.usbmodem*",
            "/dev/cu.usbserial*",
            "/dev/ttyACM*",
            "/dev/ttyUSB*",
        )
        return sorted({path for pattern in patterns for path in glob.glob(pattern)})

    @staticmethod
    def _open_raw(path: str) -> int:
        fd = os.open(path, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
        try:
            tty.setraw(fd, termios.TCSANOW)
            termios.tcflush(fd, termios.TCIOFLUSH)
        except termios.error as exc:
            if exc.args[0] != errno.EPERM:
                os.close(fd)
                raise
        return fd

    @staticmethod
    def _read_reply(fd: int, timeout: float) -> bytes:
        selector = selectors.DefaultSelector()
        selector.register(fd, selectors.EVENT_READ)
        deadline = time.monotonic() + timeout
        data = bytearray()
        try:
            while time.monotonic() < deadline:
                ready = selector.select(max(0, deadline - time.monotonic()))
                if not ready:
                    break
                chunk = os.read(fd, 128)
                if chunk:
                    data.extend(chunk)
                    if b"\n" in data:
                        break
        finally:
            selector.close()
        return bytes(data)

    def _connect(self) -> None:
        candidates = [self.requested_port] if self.requested_port != "auto" else self.candidates()
        if not candidates:
            raise FileNotFoundError("没有发现 USB 串口；请用数据线连接左半键盘")

        failures: list[str] = []
        for path in candidates:
            fd: int | None = None
            try:
                fd = self._open_raw(path)
                os.write(fd, PROBE)
                reply = self._read_reply(fd, self.probe_timeout)
                if b"CX1!" not in reply:
                    failures.append(path)
                    os.close(fd)
                    continue
                self.fd = fd
                self.path = path
                return
            except OSError:
                failures.append(path)
                if fd is not None:
                    os.close(fd)
        raise RuntimeError("没有找到 Eyelash Sofle 的 Codex 通道；已检查：" + ", ".join(failures))

    def write(self, data: bytes) -> tuple[str, str]:
        if self.fd is None:
            self._connect()
        assert self.fd is not None and self.path is not None
        os.write(self.fd, data)
        reply = self._read_reply(self.fd, self.probe_timeout).decode("ascii", "replace").strip()
        if not reply.startswith("CX1,"):
            raise TimeoutError(f"键盘未确认数据帧：{reply or '无响应'}")
        return self.path, reply.removeprefix("CX1,")


def run(args: argparse.Namespace) -> int:
    app_server = CodexAppServer(args.timeout)
    serial = SerialPort(args.port, args.probe_timeout)
    stopping = False

    def stop(_signum: int, _frame: Any) -> None:
        nonlocal stopping
        stopping = True

    signal.signal(signal.SIGINT, stop)
    signal.signal(signal.SIGTERM, stop)
    try:
        while not stopping:
            try:
                limits = app_server.rate_limits()
                packed = pack_snapshot(limits)
                line = packed.wire_line()
                if args.dry_run:
                    destination = "dry-run"
                    reply = "DRY"
                    print(line.decode("ascii").rstrip())
                else:
                    destination, reply = serial.write(line)
                remaining = (
                    "-"
                    if packed.remaining_percent == UNKNOWN_PERCENT
                    else f"{packed.remaining_percent}%"
                )
                print(
                    f"[{time.strftime('%F %T')}] sent to {destination}: "
                    f"remaining={remaining} keyboard={reply}",
                    flush=True,
                )
            except Exception as exc:
                print(f"[{time.strftime('%F %T')}] {type(exc).__name__}: {exc}", file=sys.stderr, flush=True)
                app_server.close()
                serial.close()
                if args.once:
                    return 1
            if args.once:
                break
            deadline = time.monotonic() + args.interval
            while not stopping and time.monotonic() < deadline:
                time.sleep(min(1, deadline - time.monotonic()))
    finally:
        app_server.close()
        serial.close()
    return 0


def max_interval(value: str) -> int:
    parsed = int(value)
    if parsed < 30:
        raise argparse.ArgumentTypeError("刷新间隔不能小于 30 秒")
    return parsed


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="把 Codex 用量同步到 Eyelash Sofle 右屏")
    parser.add_argument("--port", default="auto", help="Codex CDC 串口；默认自动探测")
    parser.add_argument("--interval", type=max_interval, default=60, help="刷新秒数，最小 30")
    parser.add_argument("--timeout", type=float, default=30, help="Codex 查询超时秒数")
    parser.add_argument("--probe-timeout", type=float, default=1.0, help="键盘串口探测超时秒数")
    parser.add_argument("--once", action="store_true", help="仅同步一次")
    parser.add_argument("--dry-run", action="store_true", help="只输出帧，不访问串口")
    return parser.parse_args()


if __name__ == "__main__":
    raise SystemExit(run(parse_args()))
