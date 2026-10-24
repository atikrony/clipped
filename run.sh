#!/usr/bin/env bash
# Portable launcher — build if needed, then run from this directory.
# No installation required.
set -e

DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$DIR"

# Rebuild if binary is missing or any source file is newer than the binary
if [ ! -f clipped ] || find src -name "*.c" -newer clipped 2>/dev/null | grep -q .; then
    echo "[clipped] Building..."
    make
fi

echo "[clipped] Starting (hotkey: Super+V, tray icon in panel)..."
exec ./clipped "$@"
