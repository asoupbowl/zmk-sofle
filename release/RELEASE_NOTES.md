# Eyelash Sofle Codex v1.1.0

This maintenance release updates the macOS helper. Keyboard firmware is unchanged from v1.0.0.

Fixes:

- reconnect automatically after the Mac sleeps or Bluetooth temporarily becomes unavailable
- discard stale BLE characteristics before reconnecting
- sync immediately after reconnection, then continue refreshing every 60 seconds
- ignore an in-flight usage result if it belongs to an old Bluetooth connection

The Codex desktop window does not need to remain open. The Mac must be awake and signed in.

## Included configuration

This release packages the tested configuration for the exact Eyelash Sofle + nice!nano v2 + dual nice!view hardware combination.

Highlights:

- wireless Codex desktop main-quota display on the right half
- native nice!view battery and connection header
- macOS Control / Option / Command layout and Caps input-source switching
- MO1 layer for function keys, navigation, screenshots, and RGB controls
- universal macOS helper for Apple Silicon and Intel
- Chinese and English setup guides

Download the full ZIP for a first-time installation. The individual UF2 files are also attached for upgrades.

Do not flash `SETTINGS-RESET-EMERGENCY-ONLY.uf2` unless you intentionally need to erase Bluetooth pairings.
