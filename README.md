# Eyelash Sofle：Codex 用量屏幕与 macOS 键位

[English](README_EN.md) · [下载最新发行包](https://github.com/asoupbowl/zmk-sofle/releases/latest) · [中文安装说明](docs/INSTALL.zh-CN.md)

这是 Eyelash Sofle ZMK 固件的公开参考实现：右侧 nice!view 显示 Codex 桌面端主配额，电脑通过已配对的加密蓝牙连接每 60 秒无线同步；基础键位和功能层按 macOS 使用习惯整理。

## 功能

- 右屏显示 Codex 剩余百分比、配额窗口、进度条和本地重置时间。
- 保留 nice!view 原生电池、充电和分体连接状态栏。
- 日常同步完全无线，不需要 USB。
- Caps 切换中英文，修饰键为 Control / Option / Command。
- `MO1` 功能层包含 F1–F12、导航、截图和 RGB 控制。
- 5 个蓝牙设备槽位；Codex BLE 特征要求已绑定并加密。
- 不把 Codex 凭据、会话或提示词写入键盘或 GitHub。

## 能否直接使用？

只有以下硬件组合可以直接刷发行包：

- Eyelash Sofle PCB，矩阵与本仓库一致
- 左右 nice!nano v2
- 左右 nice!view 160×68
- 左侧 central、右侧 peripheral
- macOS 13+ 与已登录的 Codex 桌面端

不同 PCB、控制器、屏幕或 Windows/Linux 用户请仅参考源码，不要直接刷 UF2。

## 快速安装

1. 从 [Releases](https://github.com/asoupbowl/zmk-sofle/releases/latest) 下载完整 ZIP 并解压。
2. 右侧刷 `RIGHT-CODEX-DISPLAY.uf2`，左侧刷 `LEFT-CODEX-KEYMAP.uf2`。
3. 在 macOS 蓝牙设置连接 `Eyelash Sofle`。
4. 右键打开 `macOS/Install.command`，并允许蓝牙访问。

完整步骤、键位表和故障排查见 [中文安装说明](docs/INSTALL.zh-CN.md)。

> `SETTINGS-RESET-EMERGENCY-ONLY.uf2` 会清除蓝牙配对，平时不要刷。

## 开发与验证

固件由 GitHub Actions 使用 `build.yaml` 构建。Mac 应用可在 macOS 上复现：

```sh
chmod +x macos/build_app.sh
./macos/build_app.sh dist
python3 -m unittest discover -s tests -v
```

核心目录：

- `config/eyelash_sofle.keymap`：键位和功能层
- `src/codex_usage_widget.c`：右屏界面
- `src/host_ble.c`：键盘端加密 BLE 接收
- `host/codex_usage_bridge.py`：Codex 主配额读取与编码
- `macos/wireless_bridge.swift`：macOS 蓝牙同步应用

## 上游与致谢

本仓库 fork 自 Eyelash Sofle 配置，并基于 ZMK、nice!view 与 cormoran 的 ZMK 分支。原硬件/外壳与售后信息请参考上游仓库；本 fork 的 Codex 集成是社区参考项目，不是 OpenAI 官方键盘产品。
