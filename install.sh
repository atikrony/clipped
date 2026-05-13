#!/usr/bin/env bash
# install.sh — install dependencies, build, and install clipman
set -e

echo "==> Installing dependencies..."
sudo apt-get update -qq
sudo apt-get install -y \
    gcc make pkg-config \
    libgtk-3-dev libx11-dev libxtst-dev \
    libgtk-3-0 libx11-6 libxtst6 libglib2.0-0 libcairo2 libgdk-pixbuf-2.0-0 libpango-1.0-0

echo "==> Building..."
make clean
make

echo "==> Installing to /usr..."
sudo make install

echo ""
echo "Done. Launch with:  clipman &"
echo "Or log out and back in — the autostart entry will run it automatically."
