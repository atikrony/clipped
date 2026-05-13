#!/usr/bin/env bash
# build-deb.sh — build a distributable .deb for Clipped
# Works on Ubuntu 20.04 / 22.04 / 24.04, Debian 11/12, Linux Mint, and more.
set -e

cd "$(dirname "$0")"

VER=1.0.0
PKG=clipman
ARCH=$(dpkg --print-architecture)
# Build in /tmp — avoids permission issues on NTFS/exFAT mounts where
# all dirs are forced to 777 and dpkg-deb refuses to build from them.
BUILDROOT="/tmp/${PKG}_${VER}_${ARCH}"
OUTDIR="$(pwd)/build"

# ── 1. Install build dependencies only if missing ─────────────────
MISSING=""
for pkg in libgtk-3-dev libx11-dev libxtst-dev pkg-config gcc make; do
    dpkg -s "$pkg" &>/dev/null || MISSING="$MISSING $pkg"
done
if [ -n "$MISSING" ]; then
    echo "==> Installing missing build dependencies:$MISSING"
    sudo apt-get install -y $MISSING
else
    echo "==> Build dependencies already installed."
fi

# ── 2. Build (strip binary for smaller size) ──────────────────────
echo "==> Building..."
make clean
make
strip clipman
echo "    Binary size: $(du -sh clipman | cut -f1)"

# ── 3. Package layout ─────────────────────────────────────────────
echo "==> Creating package layout..."
rm -rf "$BUILDROOT"
mkdir -p "$BUILDROOT/DEBIAN"
mkdir -p "$BUILDROOT/usr/bin"
mkdir -p "$BUILDROOT/usr/share/applications"
mkdir -p "$BUILDROOT/usr/share/pixmaps"
mkdir -p "$BUILDROOT/etc/xdg/autostart"

install -m 755 clipman                      "$BUILDROOT/usr/bin/clipman"
install -m 644 assets/clipman-cat.png       "$BUILDROOT/usr/share/pixmaps/clipman-cat.png"
install -m 644 clipman.desktop              "$BUILDROOT/usr/share/applications/clipman.desktop"
install -m 644 clipman.autostart.desktop    "$BUILDROOT/etc/xdg/autostart/clipman.desktop"

# ── 4. DEBIAN/control ─────────────────────────────────────────────
cat > "$BUILDROOT/DEBIAN/control" << EOF
Package: $PKG
Version: $VER
Architecture: $ARCH
Maintainer: irony <atiqrony.aiub@gmail.com>
Depends: libgtk-3-0, libglib2.0-0, libx11-6, libxtst6, libcairo2, libgdk-pixbuf-2.0-0, libpango-1.0-0, curl
Section: utils
Priority: optional
Homepage: https://github.com/atikrony/clipped
Description: Clipped — lightweight clipboard history manager
 Press Super+V to open clipboard history. Click items to paste.
 Includes emoji browser, symbol browser, and settings page.
 Works on Ubuntu 20.04/22.04/24.04, Debian, Mint and more.
 Runs via XWayland (GDK_BACKEND=x11) on Wayland sessions.
EOF

# ── 5. postinst — refresh app menu and notify user ────────────────
cat > "$BUILDROOT/DEBIAN/postinst" << 'POSTINST'
#!/bin/sh
update-desktop-database -q /usr/share/applications 2>/dev/null || true
echo ""
echo "  Clipped installed successfully!"
echo "  Log out and back in for autostart to take effect."
echo "  Or launch now:  clipman &"
echo "  Toggle panel:   Super+V"
echo ""
exit 0
POSTINST
chmod 755 "$BUILDROOT/DEBIAN/postinst"

# ── 6. prerm — stop running instance before uninstall ─────────────
cat > "$BUILDROOT/DEBIAN/prerm" << 'PRERM'
#!/bin/sh
pkill -x clipman 2>/dev/null || true
exit 0
PRERM
chmod 755 "$BUILDROOT/DEBIAN/prerm"

# ── 7. postrm — clean user data on purge ──────────────────────────
cat > "$BUILDROOT/DEBIAN/postrm" << 'POSTRM'
#!/bin/sh
if [ "$1" = "purge" ]; then
    rm -rf "$HOME/.local/share/clipman" 2>/dev/null || true
fi
update-desktop-database -q /usr/share/applications 2>/dev/null || true
exit 0
POSTRM
chmod 755 "$BUILDROOT/DEBIAN/postrm"

# ── 8. Build .deb ─────────────────────────────────────────────────
echo "==> Building .deb..."
mkdir -p "$OUTDIR"
dpkg-deb --build --root-owner-group "$BUILDROOT"
mv "${BUILDROOT}.deb" "$OUTDIR/"

DEB="build/${PKG}_${VER}_${ARCH}.deb"
echo ""
echo "  ✓  Package: $DEB  ($(du -sh "$DEB" | cut -f1))"
echo ""
echo "  Install:   sudo dpkg -i $DEB"
echo "  Or:        sudo apt install ./$DEB   (auto-installs missing runtime deps)"
echo "  Uninstall: sudo dpkg -r $PKG"
