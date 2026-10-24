#!/usr/bin/env bash
# uninstall.sh — remove clipped from the system
set -e

echo "==> Stopping clipped..."
pkill -x clipped 2>/dev/null && echo "    Process killed." || echo "    Not running."
rm -f "/tmp/clipped-$(id -u).lock"

echo "==> Removing installed files..."
sudo rm -f /usr/bin/clipped
sudo rm -f /usr/share/applications/clipped.desktop
sudo rm -f /etc/xdg/autostart/clipped.desktop
sudo rm -f /usr/share/pixmaps/clipped-cat.png

echo "==> Removing user data and history..."
rm -rf "$HOME/.local/share/clipped" "$HOME/.config/clipped"

echo "==> Done. Clipped has been removed."
