#!/bin/bash

# Simple Clipped Uninstaller for .deb package installations
# Usage: ./uninstall-deb.sh

echo "Clipped Uninstaller for .deb package"
echo "===================================="
echo

# Check if package is installed
if ! dpkg -l | grep -q "clipped-native"; then
    echo "❌ Clipped is not installed via .deb package"
    echo "   Use ./uninstall.sh for manual installations"
    exit 1
fi

echo "📦 Found Clipped installed via .deb package"
echo

# Confirm uninstallation
read -p "Do you want to uninstall Clipped? [y/N]: " response
if [[ ! "$response" =~ ^[Yy]$ ]]; then
    echo "Uninstallation cancelled."
    exit 0
fi

echo
echo "🔄 Uninstalling Clipped..."

# Stop any running processes
echo "• Stopping Clipped processes..."
pkill -f "clipped" 2>/dev/null || true
pkill -f "Clipped" 2>/dev/null || true
sleep 1

# Remove the package
echo "• Removing package..."
sudo apt-get remove --purge -y clipped-native

# Clean up user data
echo "• Cleaning up user data..."
rm -rf ~/.config/clipped-native 2>/dev/null || true
rm -rf ~/.cache/clipped-native 2>/dev/null || true
rm -f ~/.config/autostart/clipped.desktop 2>/dev/null || true

# Update desktop database
echo "• Updating desktop database..."
update-desktop-database ~/.local/share/applications/ 2>/dev/null || true

echo
echo "✅ Clipped has been successfully uninstalled!"
echo "   All files and configurations have been removed."
echo "   You may need to restart your session for complete cleanup."
echo