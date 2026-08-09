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

    def test_packs_desktop_quota_dimensions(self):
        reset = bridge.datetime(2026, 8, 16, 9, 51).timestamp()
        snapshot = {
            "primary": {"usedPercent": 23, "windowDurationMins": 10080, "resetsAt": reset},
        }
        packed = bridge.pack_snapshot(snapshot)
        self.assertEqual(packed.remaining_percent, 77)
        self.assertEqual(packed.param1, 77 | (168 << 8))
        self.assertEqual(packed.param2, 8 | (16 << 8) | (9 << 16) | (51 << 24))
        self.assertRegex(packed.wire_line().decode(), r"^CX1,[0-9A-F]{8},[0-9A-F]{8}\*[0-9A-F]{2}\n$")

    def test_missing_reset_is_encoded_as_zero(self):
        packed = bridge.pack_snapshot(
            {"primary": {"usedPercent": 0, "windowDurationMins": 10080}},
        )
        self.assertEqual(packed.remaining_percent, 100)
        self.assertEqual(packed.param2, 0)

    def test_percent_is_clamped(self):
        packed = bridge.pack_snapshot(
            {"primary": {"usedPercent": 145, "windowDurationMins": 300}}
        )
        self.assertEqual(packed.remaining_percent, 0)


if __name__ == "__main__":
    unittest.main()
