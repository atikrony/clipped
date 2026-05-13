#!/usr/bin/env bash
# Build a self-contained .deb package for clipman.
# Usage:  ./build-deb.sh
set -e

cd "$(dirname "$0")"

VER=1.0.0
PKG=clipman
ARCH=$(dpkg --print-architecture)
BUILDROOT="build/${PKG}_${VER}_${ARCH}"

echo "==> Installing build dependencies..."
sudo apt-get install -y \
    gcc make pkg-config \
    libgtk-3-dev libx11-dev libxtst-dev

echo "==> Building binary..."
make clean
make

echo "==> Creating package layout..."
rm -rf "$BUILDROOT"
mkdir -p "$BUILDROOT/DEBIAN"
mkdir -p "$BUILDROOT/usr/bin"
mkdir -p "$BUILDROOT/usr/share/applications"
mkdir -p "$BUILDROOT/usr/share/pixmaps"
mkdir -p "$BUILDROOT/etc/xdg/autostart"

# Binary
install -m 755 clipman                   "$BUILDROOT/usr/bin/clipman"

# Tray icon
install -m 644 assets/clipman-cat.png   "$BUILDROOT/usr/share/pixmaps/clipman-cat.png"

# Desktop entry (app menu)
install -m 644 clipman.desktop          "$BUILDROOT/usr/share/applications/clipman.desktop"

# Autostart entry (runs at login)
install -m 644 clipman.autostart.desktop "$BUILDROOT/etc/xdg/autostart/clipman.desktop"

# DEBIAN/control
cat > "$BUILDROOT/DEBIAN/control" << EOF
Package: $PKG
Version: $VER
Architecture: $ARCH
Maintainer: Your Name <you@example.com>
Depends: libgtk-3-0, libglib2.0-0, libx11-6, libxtst6, libcairo2, libgdk-pixbuf-2.0-0, libpango-1.0-0
Section: utils
Priority: optional
Description: Lightweight clipboard history manager
 Press Super+V to open clipboard history, or click the cat icon in the
 system tray.  Click any item to paste it into the previously focused
 window.  Starts automatically at login.
EOF

# DEBIAN/postinst — refresh the app menu after install
cat > "$BUILDROOT/DEBIAN/postinst" << 'POSTINST'
#!/bin/sh
update-desktop-database -q /usr/share/applications 2>/dev/null || true
exit 0
POSTINST
chmod 755 "$BUILDROOT/DEBIAN/postinst"

echo "==> Building .deb..."
mkdir -p build
dpkg-deb --build --root-owner-group "$BUILDROOT"

DEB="build/${PKG}_${VER}_${ARCH}.deb"
echo ""
echo "  Package : $DEB"
echo "  Install : sudo dpkg -i $DEB"
echo "  Remove  : sudo dpkg -r $PKG"
