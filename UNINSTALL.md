# Clipped Uninstaller

This directory contains uninstall scripts to completely remove Clipped from your system.

## 📋 Available Uninstall Options

### 1. For .deb Package Installation (Recommended)

```bash
./uninstall-deb.sh
```

**Use this if you installed Clipped using:**

- `sudo apt install ./clipped-native_1.0.0_amd64.deb`
- `sudo dpkg -i clipped-native_1.0.0_amd64.deb`
- Double-clicking the .deb file

### 2. For Manual/Source Installation

```bash
./uninstall.sh
```

**Use this if you:**

- Installed from source code
- Copied files manually
- Used AppImage
- Need comprehensive cleanup

## 🧹 What Gets Removed

Both scripts will remove:

- ✅ Application executable files
- ✅ Desktop shortcuts and menu entries
- ✅ Autostart configuration
- ✅ Application data and cache files
- ✅ User configuration files
- ✅ System integration files

## 🔧 Usage Instructions

### For .deb Package Users:

1. Open terminal in this directory
2. Run: `./uninstall-deb.sh`
3. Enter your password when prompted (for sudo)
4. Confirm uninstallation

### For Manual Installation Users:

1. Open terminal in this directory
2. Run: `./uninstall.sh`
3. Confirm uninstallation
4. For system-wide removal, run with sudo: `sudo ./uninstall.sh`

## ⚠️ Important Notes

- **Backup Data**: These scripts will remove ALL Clipped data including clipboard history
- **Running Processes**: Scripts will automatically stop any running Clipped processes
- **System Restart**: You may need to restart your desktop session for complete cleanup
- **Source Code**: If you're in the source code directory, it won't be removed

## 🆘 Troubleshooting

**Script won't run?**

```bash
chmod +x uninstall.sh
chmod +x uninstall-deb.sh
```

**Still seeing Clipped after uninstall?**

1. Restart your desktop session
2. Clear application cache: `sudo update-desktop-database`
3. Run the comprehensive uninstaller: `sudo ./uninstall.sh`

**Package removal fails?**

```bash
sudo apt-get remove --purge clipped-native
sudo apt-get autoremove
```

## 📧 Support

If you encounter issues with uninstallation, please:

1. Check the terminal output for error messages
2. Try running with sudo for system-wide cleanup
3. Manually remove remaining files if needed

---

Thank you for using Clipped! 👋
