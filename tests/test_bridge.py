import importlib.util
import errno
import sys
import termios
import unittest
from pathlib import Path
from unittest import mock


MODULE_PATH = Path(__file__).parents[1] / "host" / "codex_usage_bridge.py"
SPEC = importlib.util.spec_from_file_location("codex_usage_bridge", MODULE_PATH)
bridge = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = bridge
SPEC.loader.exec_module(bridge)


class BridgeTests(unittest.TestCase):
    @mock.patch.object(bridge.termios, "tcflush")
    @mock.patch.object(bridge.tty, "setraw")
    @mock.patch.object(bridge.os, "open", return_value=7)
    def test_raw_mode_permission_error_uses_cdc_defaults(
        self, open_mock, setraw_mock, _flush_mock
    ):
        setraw_mock.side_effect = termios.error(errno.EPERM, "Operation not permitted")
        self.assertEqual(bridge.SerialPort._open_raw("/dev/cu.test"), 7)
        open_mock.assert_called_once()

    def test_crc_reference(self):
        self.assertEqual(bridge.crc8(b"123456789"), 0xF4)

    def test_packs_two_windows(self):
        snapshot = {
            "primary": {"usedPercent": 23, "windowDurationMins": 300, "resetsAt": 4600},
            "secondary": {"usedPercent": 41, "windowDurationMins": 10080, "resetsAt": 87400},
        }
        packed = bridge.pack_snapshot(snapshot, now=1000)
        self.assertEqual(packed.param1, 23 | (41 << 8) | (5 << 16) | (168 << 24))
        self.assertEqual(packed.param2, 60 | (1440 << 16))
        self.assertRegex(packed.wire_line().decode(), r"^CX1,[0-9A-F]{8},[0-9A-F]{8}\*[0-9A-F]{2}\n$")

    def test_missing_secondary_is_encoded_as_unknown(self):
        packed = bridge.pack_snapshot(
            {"primary": {"usedPercent": 0, "windowDurationMins": 10080, "resetsAt": 1000}},
            now=1000,
        )
        self.assertEqual(packed.secondary_used, 0xFF)
        self.assertEqual((packed.param2 >> 16) & 0xFFFF, 0xFFFF)

    def test_percent_is_clamped(self):
        packed = bridge.pack_snapshot(
            {"primary": {"usedPercent": 145, "windowDurationMins": 300}}, now=1000
        )
        self.assertEqual(packed.primary_used, 100)


if __name__ == "__main__":
    unittest.main()
