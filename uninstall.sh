#!/bin/bash

# Clipped Uninstaller Script
# This script removes Clipped clipboard manager and all its associated files

echo "==================================================="
echo "        Clipped Clipboard Manager Uninstaller     "
echo "==================================================="
echo

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_question() {
    echo -e "${BLUE}[QUESTION]${NC} $1"
}

# Check if running as root for system-wide removal
if [[ $EUID -eq 0 ]]; then
    print_status "Running as root - will remove system-wide installation"
    SYSTEM_WIDE=true
else
    print_status "Running as user - will remove user-specific files"
    SYSTEM_WIDE=false
fi

echo
print_question "This will remove Clipped and all its data. Continue? [y/N]"
read -r response
if [[ ! "$response" =~ ^[Yy]$ ]]; then
    echo "Uninstallation cancelled."
    exit 0
fi

echo
print_status "Starting uninstallation process..."

# Step 1: Kill any running Clipped processes
print_status "Stopping Clipped processes..."
pkill -f "clipped" 2>/dev/null || true
pkill -f "Clipped" 2>/dev/null || true
pkill -f "electron.*clipped" 2>/dev/null || true
sleep 2

# Step 2: Remove autostart entries
print_status "Removing autostart entries..."
if [ -f "$HOME/.config/autostart/clipped.desktop" ]; then
    rm -f "$HOME/.config/autostart/clipped.desktop"
    print_status "Removed autostart desktop file"
fi

# Step 3: Remove desktop files
print_status "Removing desktop files..."
if [ -f "$HOME/.local/share/applications/clipped.desktop" ]; then
    rm -f "$HOME/.local/share/applications/clipped.desktop"
    print_status "Removed user desktop file"
fi

if [ "$SYSTEM_WIDE" = true ] && [ -f "/usr/share/applications/clipped.desktop" ]; then
    rm -f "/usr/share/applications/clipped.desktop"
    print_status "Removed system desktop file"
fi

# Step 4: Remove application data and config
print_status "Removing application data..."

# Electron-store data (usually in ~/.config/clipped-native or similar)
CONFIG_DIRS=(
    "$HOME/.config/clipped-native"
    "$HOME/.config/Clipped"
    "$HOME/.config/clipped"
)

for dir in "${CONFIG_DIRS[@]}"; do
    if [ -d "$dir" ]; then
        rm -rf "$dir"
        print_status "Removed config directory: $dir"
    fi
done

# Application cache
CACHE_DIRS=(
    "$HOME/.cache/clipped-native"
    "$HOME/.cache/Clipped"
    "$HOME/.cache/clipped"
)

for dir in "${CACHE_DIRS[@]}"; do
    if [ -d "$dir" ]; then
        rm -rf "$dir"
        print_status "Removed cache directory: $dir"
    fi
done

# Step 5: Remove system-wide installation (if installed via .deb)
if [ "$SYSTEM_WIDE" = true ]; then
    print_status "Removing system-wide installation..."
    
    # Try to remove via package manager first
    if dpkg -l | grep -q "clipped-native"; then
        print_status "Removing via apt/dpkg..."
        apt-get remove --purge -y clipped-native 2>/dev/null || dpkg --remove clipped-native 2>/dev/null
    fi
    
    # Remove installation directory
    if [ -d "/opt/Clipped" ]; then
        rm -rf "/opt/Clipped"
        print_status "Removed /opt/Clipped directory"
    fi
    
    # Remove symlinks
    if [ -L "/usr/local/bin/clipped" ]; then
        rm -f "/usr/local/bin/clipped"
        print_status "Removed /usr/local/bin/clipped symlink"
    fi
    
    if [ -L "/usr/bin/clipped" ]; then
        rm -f "/usr/bin/clipped"
        print_status "Removed /usr/bin/clipped symlink"
    fi
fi

# Step 6: Remove local installation (if installed manually)
LOCAL_INSTALL_DIRS=(
    "$HOME/.local/share/clipped"
    "$HOME/Applications/Clipped"
    "$HOME/opt/Clipped"
)

for dir in "${LOCAL_INSTALL_DIRS[@]}"; do
    if [ -d "$dir" ]; then
        rm -rf "$dir"
        print_status "Removed local installation: $dir"
    fi
done

# Step 7: Remove any remaining files
print_status "Cleaning up remaining files..."

# Remove any clipped-related files in home directory
find "$HOME" -name "*clipped*" -type f 2>/dev/null | grep -E "\.(desktop|log|config)$" | while read -r file; do
    if [[ "$file" != *"/clipped/"* ]]; then  # Don't remove source code directory
        rm -f "$file"
        print_status "Removed file: $file"
    fi
done

# Step 8: Update desktop database
print_status "Updating desktop database..."
if command -v update-desktop-database >/dev/null 2>&1; then
    if [ "$SYSTEM_WIDE" = true ]; then
        update-desktop-database /usr/share/applications/ 2>/dev/null || true
    fi
    update-desktop-database "$HOME/.local/share/applications/" 2>/dev/null || true
fi

# Step 9: Clear any cached icons
print_status "Clearing icon cache..."
if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    gtk-update-icon-cache -f -t "$HOME/.local/share/icons/hicolor" 2>/dev/null || true
    if [ "$SYSTEM_WIDE" = true ]; then
        gtk-update-icon-cache -f -t /usr/share/icons/hicolor 2>/dev/null || true
    fi
fi

echo
echo "==================================================="
print_status "Clipped has been successfully uninstalled!"
echo "==================================================="
echo
print_status "What was removed:"
echo "  • Application files and executables"
echo "  • Desktop shortcuts and menu entries"
echo "  • Autostart configuration"
echo "  • Application data and cache"
echo "  • Configuration files"
echo "  • System integration files"
echo
print_warning "Note: If you installed from source code, the source directory was not removed."
print_warning "You may need to restart your desktop session for all changes to take effect."
echo
echo "Thank you for using Clipped! 👋"