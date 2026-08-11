# Eyelash Sofle Codex setup

## Compatible hardware

The prebuilt firmware is only for this exact combination:

- Eyelash Sofle PCB with the same matrix and pins as this repository
- nice!nano v2 on both halves
- nice!view 160×68 on both halves
- left half as ZMK central and right half as peripheral
- macOS 13 or newer
- Codex desktop installed and signed in

Other ZMK keyboards can reuse the source as a reference, but must not flash these UF2 files directly.

## Install

1. Download and fully extract the ZIP from GitHub Releases.
2. Put the right half in UF2 bootloader mode and copy only `RIGHT-CODEX-DISPLAY.uf2`.
3. Put the left half in UF2 bootloader mode and copy only `LEFT-CODEX-KEYMAP.uf2`.
4. Connect `Eyelash Sofle` in macOS System Settings → Bluetooth.
5. Open the `macOS` folder, right-click `Install.command`, and choose Open.
6. Click Allow when macOS requests Bluetooth access.

Daily usage sync is wireless. USB is only needed for firmware updates and maintenance.

> `SETTINGS-RESET-EMERGENCY-ONLY.uf2` erases Bluetooth bonds. Do not flash it unless troubleshooting explicitly requires it.

## Keymap

The base layer follows the installed keycaps: `Control / Option / Command / MO1 / Space`. Caps sends macOS `Control + Space` to switch input sources.

Hold `MO1`:

| Chord | Action |
| --- | --- |
| `MO1 + 1…0` | F1…F10 |
| `MO1 + - / =` | F11 / F12 |
| `MO1 + Delete` | Forward Delete |
| `MO1 + A` | Full-screen screenshot |
| `MO1 + S` | Selection screenshot |
| `MO1 + I/J/K/L` | Up/Left/Down/Right |
| `MO1 + U/O` | Home / End |
| `MO1 + Y/H` | Page Up / Page Down |
| `MO1 + Z/X` | RGB off/on |
| `MO1 + C/V` | Next/previous RGB effect |
| `MO1 + N/M` | RGB brighter/dimmer |
| `MO1 + ,/.` | RGB slower/faster |

Tap the right-thumb `=` key to type equals; hold it for the maintenance layer:

- `1…5`: select Bluetooth profile 1…5
- `Q`: clear the current profile
- `W`: clear all profiles (use with care)
- `A/S`: select USB / BLE output

## Codex screen

The right display mirrors the main Codex desktop quota:

- remaining percentage and progress bar
- quota window such as 5H or 7D
- local reset date and time
- native nice!view battery, charging, and split-connection status

Every 60 seconds the Mac helper reads the local Codex app-server and sends an 8-byte snapshot over the encrypted, bonded BLE connection. Account credentials, conversation content, and prompts are never stored on the keyboard or committed to this repository.

The Codex desktop window does not need to remain open. After the Mac wakes or Bluetooth temporarily becomes unavailable, the helper reconnects automatically, syncs immediately, and then resumes the 60-second refresh cycle. The Mac must be awake and the Codex account must remain signed in.

## Troubleshooting

- **Finder reports error -36 while copying UF2**: use `cp firmware-path /Volumes/NICENANO/` in Terminal.
- **Typing works but Codex usage is missing**: forget the Bluetooth device, pair it once more, then reopen the helper app.
- **Usage is briefly missing after wake**: V1.1 reconnects automatically; keep the keyboard on and it should recover within a few seconds. The log should show `Wireless Codex channel is ready`.
- **No Bluetooth permission prompt**: allow Eyelash Sofle Codex Usage in System Settings → Privacy & Security → Bluetooth.
- **The app has no window**: this is expected; it is a login background app.
- **Log file**: `~/Library/Logs/Eyelash Sofle Codex Usage/wireless.log`.
- **Python 3 is missing**: install Python 3 or Xcode Command Line Tools, then run the installer again.
