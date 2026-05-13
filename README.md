# Clipped 📋

A lightweight clipboard history manager with an emoji browser, symbol browser, and system tray icon.  
Built with C + GTK3. Works on **Linux**, **Windows**, and **macOS**.

---

## Features

- Clipboard history with search and one-click paste
- Emoji browser — search by name (cat, fire, heart…)
- Unicode symbol browser — math, arrows, currency, Greek…
- Settings page — configurable hotkey, max history
- System tray cat icon — click to open, right-click to quit
- Autostart at login
- Single-instance lock

---

## Download (pre-built)

All release files are in the [`releases/`](releases/) folder.

| Platform | File | How to install |
|---|---|---|
| Linux (Ubuntu / Debian / Mint) | `clipman_1.0.0_amd64.deb` | `sudo apt install ./releases/clipman_1.0.0_amd64.deb` |
| Windows | `clipped.exe` | Run the `.exe` directly |
| macOS | `clipped_1.0.0_macos.dmg` | Open the `.dmg` and drag to Applications |

---

## Linux

### Install from .deb

```bash
sudo apt install ./releases/clipman_1.0.0_amd64.deb
```

This installs the binary, desktop entry, and autostart entry.  
The app will start automatically on next login.

### Uninstall

```bash
sudo dpkg -r clipman
```

Or use the included script (also removes history and config):

```bash
./uninstall.sh
```

### Build from source

```bash
./install.sh        # installs build deps + compiles + installs to /usr
```

Or step by step:

```bash
make deps           # install build dependencies via apt
make                # compile
sudo make install   # install to /usr
```

### Run without installing (portable)

```bash
./run.sh
```

---

## Windows

### Install

1. Download `releases/clipped.exe`
2. Double-click to run — no installation needed
3. The app will appear in the system tray

> **Requires:** [MSYS2 GTK3 runtime](https://www.msys2.org) DLLs in the same folder as the `.exe`.  
> See [build instructions](#build-on-windows) to get the DLL bundle.

### Uninstall

Close the app from the tray → right-click → **Quit**, then delete the folder.

### Build on Windows

1. Install [MSYS2](https://www.msys2.org)
2. Open **MSYS2 MinGW 64-bit** terminal
3. Run:

```bash
pacman -S mingw-w64-x86_64-gtk3 mingw-w64-x86_64-gcc pkg-config make
make -f Makefile.windows
```

Output goes to `releases/clipped.exe`.  
Bundle the required DLLs alongside it:

```bash
ldd releases/clipped.exe   # shows which DLLs to copy
```

---

## macOS

### Install

1. Download `releases/clipped_1.0.0_macos.dmg`
2. Open the `.dmg` and drag **Clipped** to your Applications folder
3. Launch from Applications

> **First launch:** macOS will ask for **Accessibility permission** (required for the global hotkey).  
> Go to **System Settings → Privacy & Security → Accessibility** and enable Clipped.

### Uninstall

Drag `Clipped.app` from Applications to Trash.  
To also remove history and config:

```bash
rm -rf ~/.config/clipman ~/.local/share/clipman
```

### Build on macOS

```bash
# Install Homebrew: https://brew.sh
brew install gtk+3 pkg-config

make -f Makefile.macos        # builds clipped.app
make -f Makefile.macos dmg    # packages as releases/clipped_1.0.0_macos.dmg
```

---

## Default Hotkey

`Super+V` on all platforms:

| Platform | Key |
|---|---|
| Linux | Super (⊞ Win key) + V |
| Windows | Win (⊞) + V |
| macOS | Command (⌘) + V |

Change it any time in the **⚙ Settings** page inside the app.

---

## Compatibility

| Distro / OS | Works? | Notes |
|---|---|---|
| Ubuntu 20.04 / 22.04 | ✅ | |
| Ubuntu 24.04 | ✅ | Runs via XWayland (`GDK_BACKEND=x11`) |
| Linux Mint | ✅ | Tray works natively on Cinnamon |
| Debian 11 / 12 | ✅ | |
| Windows 10 / 11 | ✅ | Needs GTK3 DLLs |
| macOS 12+ (Monterey+) | ✅ | Needs Homebrew GTK3 + Accessibility permission |

---

## Built by

**irony** — vibe coded 🐱
