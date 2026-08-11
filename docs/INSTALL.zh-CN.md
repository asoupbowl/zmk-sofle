# Eyelash Sofle Codex 安装说明

## 兼容硬件

发行包只能直接用于以下组合：

- Eyelash Sofle PCB，且矩阵与本仓库定义一致
- 左右两边均为 nice!nano v2
- 左右两边均为 nice!view 160×68
- 左边是 ZMK central，右边是 peripheral
- macOS 13 或更高版本
- 已安装并登录 Codex 桌面端

其他 ZMK 键盘可以参考源码，但不要直接刷入 UF2。

## 安装

1. 从 GitHub Releases 下载完整 ZIP，并完整解压。
2. 给右键盘进入 UF2 引导盘，只复制 `RIGHT-CODEX-DISPLAY.uf2`。
3. 给左键盘进入 UF2 引导盘，只复制 `LEFT-CODEX-KEYMAP.uf2`。
4. 在 macOS“系统设置 → 蓝牙”中连接 `Eyelash Sofle`。
5. 打开 `macOS` 文件夹，右键 `Install.command`，选择“打开”。
6. macOS 询问蓝牙权限时选择“允许”。

日常同步通过蓝牙完成，不需要 USB。USB 仅用于刷固件和维护。

> `SETTINGS-RESET-EMERGENCY-ONLY.uf2` 会清除配对信息。除非故障排查明确要求，否则不要刷。

## 键位

基础层与照片键帽一致：`Control / Option / Command / MO1 / Space`，Caps 键发送 macOS 的 `Control + Space` 来切换中英文。

按住 `MO1`：

| 组合 | 功能 |
| --- | --- |
| `MO1 + 1…0` | F1…F10 |
| `MO1 + - / =` | F11 / F12 |
| `MO1 + Delete` | Forward Delete |
| `MO1 + A` | 全屏截图 |
| `MO1 + S` | 选区截图 |
| `MO1 + I/J/K/L` | 上/左/下/右 |
| `MO1 + U/O` | Home / End |
| `MO1 + Y/H` | Page Up / Page Down |
| `MO1 + Z/X` | RGB 关闭/开启 |
| `MO1 + C/V` | 下一个/上一个 RGB 效果 |
| `MO1 + N/M` | RGB 变亮/变暗 |
| `MO1 + ,/.` | RGB 减速/加速 |

轻点右拇指区 `=` 会输入等号；按住它进入维护层：

- `1…5`：选择蓝牙槽位 1…5
- `Q`：清除当前槽位
- `W`：清除全部槽位（谨慎）
- `A/S`：选择 USB / BLE 输出

## Codex 右屏

右屏显示 Codex 桌面端主配额：

- 剩余百分比和进度条
- 配额窗口，例如 5H 或 7D
- 本地重置日期和时间
- nice!view 原生电池、充电和分体连接状态

电脑端每 60 秒读取一次本机 Codex app-server，并通过加密的已配对 BLE 连接发送 8 字节快照。账号凭据、会话内容和提示词不会写入键盘，也不会上传到本仓库。

Codex 桌面窗口不需要保持打开。Mac 从睡眠中唤醒或蓝牙短暂关闭后，同步助手会自动重新连接、立即同步一次，然后继续每 60 秒刷新。Mac 必须处于唤醒状态，并保持 Codex 账号登录。

## 故障排查

- **Finder 复制 UF2 报错 -36**：打开终端，使用 `cp 固件路径 /Volumes/NICENANO/`。
- **键盘能输入但右屏没有 Codex 用量**：忘记该蓝牙设备并重新配对一次，然后重新打开同步应用。
- **Mac 唤醒后暂时没有用量**：V1.1 会自动重新连接；保持键盘开启，通常数秒内恢复。可查看日志确认出现 `Wireless Codex channel is ready`。
- **没有蓝牙授权提示**：在“系统设置 → 隐私与安全性 → 蓝牙”中允许 Eyelash Sofle Codex Usage。
- **应用没有窗口**：正常。它是登录后台程序。
- **检查日志**：`~/Library/Logs/Eyelash Sofle Codex Usage/wireless.log`。
- **找不到 Python 3**：安装 Python 3，或安装 Xcode Command Line Tools 后再次运行安装程序。
