#!/bin/zsh
set -euo pipefail

script_dir="${0:A:h}"
source_app="$script_dir/Eyelash Sofle Codex Usage.app"
install_dir="$HOME/Applications"
installed_app="$install_dir/Eyelash Sofle Codex Usage.app"

fail() {
  echo
  echo "安装失败：$1"
  echo "Installation failed: $1"
  echo
  read -r "?按回车关闭 / Press Return to close: "
  exit 1
}

[[ -d "$source_app" ]] || fail "安装程序旁边没有找到应用。请完整解压发行包后再运行。"

python_path=""
for candidate in /usr/bin/python3 /opt/homebrew/bin/python3 /usr/local/bin/python3; do
  if [[ -x "$candidate" ]]; then
    python_path="$candidate"
    break
  fi
done
[[ -n "$python_path" ]] || fail "没有找到 Python 3。请先安装 Python 3。"

if [[ ! -x /Applications/Codex.app/Contents/Resources/codex \
   && ! -x /Applications/ChatGPT.app/Contents/Resources/codex \
   && ! -x "$HOME/Applications/Codex.app/Contents/Resources/codex" \
   && ! -x "$HOME/Applications/ChatGPT.app/Contents/Resources/codex" \
   && -z "${CODEX_BIN:-}" ]]; then
  fail "没有找到 Codex 桌面端。请先安装并登录 Codex。"
fi

mkdir -p "$install_dir"
pkill -x EyelashSofleCodex 2>/dev/null || true
ditto "$source_app" "$installed_app"
xattr -dr com.apple.quarantine "$installed_app" 2>/dev/null || true
xattr -cr "$installed_app"
codesign --force --deep --sign - "$installed_app"
codesign --verify --deep --strict "$installed_app" || fail "应用签名校验未通过。"
open "$installed_app"

echo
echo "安装完成。首次出现蓝牙访问提示时，请点击“允许”。"
echo "Installed. Click Allow when macOS asks for Bluetooth access."
echo
echo "日志 / Log: ~/Library/Logs/Eyelash Sofle Codex Usage/wireless.log"
echo
read -r "?按回车关闭 / Press Return to close: "
