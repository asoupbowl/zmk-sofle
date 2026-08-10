# Eyelash Sofle Codex usage display and macOS keymap

[中文](README.md) · [Latest release](https://github.com/asoupbowl/zmk-sofle/releases/latest) · [English setup guide](docs/INSTALL.en.md)

This is a public reference implementation for the Eyelash Sofle ZMK keyboard. The right nice!view mirrors the main Codex desktop quota, synchronized every 60 seconds over the encrypted connection of the paired Bluetooth keyboard. The base and function layers are organized for macOS.

## Features

- Remaining Codex percentage, quota window, progress bar, and local reset time.
- Native nice!view battery, charging, and split-connection header.
- Fully wireless daily sync; USB is not required after setup.
- Caps switches input sources; modifiers are Control / Option / Command.
- `MO1` layer for F1–F12, navigation, screenshots, and RGB controls.
- Five Bluetooth profiles and an encrypted, bonded custom BLE characteristic.
- Codex credentials, conversations, and prompts are never stored on the keyboard or GitHub.

## Can I flash it directly?

Only this exact hardware combination can use the release UF2 files directly:

- Eyelash Sofle PCB with the same matrix as this repository
- nice!nano v2 on both halves
- nice!view 160×68 on both halves
- left central and right peripheral
- macOS 13+ with Codex desktop signed in

For another PCB, controller, display, Windows, or Linux, use the source as a reference and do not flash the prebuilt UF2 files.

## Quick install

1. Download and extract the full ZIP from [Releases](https://github.com/asoupbowl/zmk-sofle/releases/latest).
2. Flash `RIGHT-CODEX-DISPLAY.uf2` to the right half and `LEFT-CODEX-KEYMAP.uf2` to the left half.
3. Connect `Eyelash Sofle` in macOS Bluetooth settings.
4. Right-click `macOS/Install.command`, choose Open, and allow Bluetooth access.

See the [English setup guide](docs/INSTALL.en.md) for the complete keymap and troubleshooting steps.

> `SETTINGS-RESET-EMERGENCY-ONLY.uf2` erases Bluetooth bonds. Do not flash it during normal installation.

## Build and test

GitHub Actions builds the firmware from `build.yaml`. Rebuild the Mac app on macOS with:

```sh
chmod +x macos/build_app.sh
./macos/build_app.sh dist
python3 -m unittest discover -s tests -v
```

Important paths:

- `config/eyelash_sofle.keymap`: base and function layers
- `src/codex_usage_widget.c`: right-display UI
- `src/host_ble.c`: encrypted keyboard-side BLE receiver
- `host/codex_usage_bridge.py`: Codex quota reader and encoder
- `macos/wireless_bridge.swift`: macOS Bluetooth helper

## Upstream and credits

This repository is forked from the Eyelash Sofle configuration and builds on ZMK, nice!view, and cormoran's ZMK branch. Refer to the upstream repository for the original hardware and case information. The Codex integration is a community reference project, not an official OpenAI keyboard product.
