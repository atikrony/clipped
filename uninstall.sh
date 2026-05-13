#!/usr/bin/env bash
# uninstall.sh — remove clipman from the system
set -e

echo "==> Stopping clipman..."
pkill -x clipman 2>/dev/null && echo "    Process killed." || echo "    Not running."
rm -f "/tmp/clipman-$(id -u).lock"

echo "==> Removing installed files..."
sudo rm -f /usr/bin/clipman
sudo rm -f /usr/share/applications/clipman.desktop
sudo rm -f /etc/xdg/autostart/clipman.desktop
sudo rm -f /usr/share/pixmaps/clipman-cat.png

echo "==> Removing user data and history..."
rm -rf "$HOME/.local/share/clipman"

echo "==> Done. Clipman has been removed."
