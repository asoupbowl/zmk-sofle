#!/bin/zsh
set -euo pipefail

repo_root="${0:A:h:h}"
output_dir="${1:-$repo_root/dist}"
version="${APP_VERSION:-1.0.0}"
build_number="${APP_BUILD_NUMBER:-1}"
build_temp="$(mktemp -d)"
app_name="Eyelash Sofle Codex Usage.app"
app_path="$output_dir/$app_name"

cleanup() {
  /bin/rm -R -- "$build_temp"
}
trap cleanup EXIT

mkdir -p "$output_dir"
/bin/rm -R -- "$app_path" 2>/dev/null || true
mkdir -p "$app_path/Contents/MacOS" "$app_path/Contents/Resources"

frameworks=(-framework AppKit -framework CoreBluetooth -framework ServiceManagement)

xcrun swiftc -swift-version 5 -O \
  -module-cache-path "$build_temp/module-cache-arm64" \
  -target arm64-apple-macos13.0 "${frameworks[@]}" \
  "$repo_root/macos/wireless_bridge.swift" -o "$build_temp/EyelashSofleCodex-arm64"

xcrun swiftc -swift-version 5 -O \
  -module-cache-path "$build_temp/module-cache-x86_64" \
  -target x86_64-apple-macos13.0 "${frameworks[@]}" \
  "$repo_root/macos/wireless_bridge.swift" -o "$build_temp/EyelashSofleCodex-x86_64"

lipo -create \
  "$build_temp/EyelashSofleCodex-arm64" \
  "$build_temp/EyelashSofleCodex-x86_64" \
  -output "$app_path/Contents/MacOS/EyelashSofleCodex"

cp "$repo_root/macos/wireless-app-Info.plist" "$app_path/Contents/Info.plist"
cp "$repo_root/host/codex_usage_bridge.py" "$app_path/Contents/Resources/codex_usage_bridge.py"
chmod 755 "$app_path/Contents/MacOS/EyelashSofleCodex"
chmod 644 "$app_path/Contents/Resources/codex_usage_bridge.py"

/usr/libexec/PlistBuddy -c "Set :CFBundleShortVersionString $version" "$app_path/Contents/Info.plist"
/usr/libexec/PlistBuddy -c "Set :CFBundleVersion $build_number" "$app_path/Contents/Info.plist"

xattr -cr "$app_path"
codesign --force --deep --sign - "$app_path"
codesign --verify --deep --strict "$app_path"

echo "$app_path"
