const {
  app,
  BrowserWindow,
  Tray,
  Menu,
  globalShortcut,
  ipcMain,
  clipboard,
  nativeImage,
} = require("electron");
const { exec } = require("child_process");
const path = require("path");
const Store = require("electron-store");

// --- Helpers ---------------------------------------------------------------
// Detect session type (Wayland vs X11)
const isWaylandSession = () =>
  process.env.XDG_SESSION_TYPE === "wayland" || !!process.env.WAYLAND_DISPLAY;

// Very small shell-escape for passing text safely to commands
function shellEscape(str) {
  if (str == null) return "''";
  return `'${String(str).replace(/'/g, "'\\''")}'`;
}

// Try a list of shell commands in order until one succeeds (exit code 0)
function tryCommandsSequentially(commands, cb) {
  if (!commands || commands.length === 0) return cb(new Error("no-commands"));
  const { exec } = require("child_process");
  const [cmd, ...rest] = commands;
  exec(cmd, (err) => {
    if (!err) return cb(null, true);
    if (rest.length === 0) return cb(err || new Error("all-failed"));
    tryCommandsSequentially(rest, cb);
  });
}

// Storage for clipboard history
const store = new Store({
  defaults: {
    clipboardHistory: [],
    maxItems: 50,
    recentEmojis: [],
    hotkey: "Super+V",
  },
});

class NativeClipboardManager {
  constructor() {
    this.mainWindow = null;
    this.tray = null;
    this.isQuitting = false;
    this.clipboardWatcher = null;
    this.lastClipboardContent = "";
    this.clipboardHistory = store.get("clipboardHistory", []);
    // Track the active window info for cursor positioning
    this.lastActiveWindowInfo = null;
    this.activeWindowTracker = null;
    // Track the previously focused X11 window id so we can restore focus before pasting
    this.lastActiveWindowId = null;
    // Debounce toggle to prevent double-trigger
    this.toggleInProgress = false;
  }

  async initialize() {
    await app.whenReady();

    this.createWindow();
    this.createTray();
    this.setupGlobalShortcuts();
    this.setupIPCHandlers();
    this.startClipboardWatcher();
    this.setupAppEvents();

    console.log("Native Clipboard Manager initialized successfully!");
  }

  createWindow() {
    this.mainWindow = new BrowserWindow({
      width: 450,
      height: 600,
      show: false,
      frame: false, // Custom window without system decorations
      resizable: false,
      skipTaskbar: true,
      icon: path.join(__dirname, "assets/note.png"),
      webPreferences: {
        nodeIntegration: false,
        contextIsolation: true,
        preload: path.join(__dirname, "preload.js"),
      },
    });

    this.mainWindow.loadFile("index.html");

    // Hide window when clicking outside or pressing Escape
    this.mainWindow.on("blur", () => {
      if (!this.isQuitting) {
        this.hideWindow();
      }
    });

    this.mainWindow.on("close", (event) => {
      if (!this.isQuitting) {
        event.preventDefault();
        this.hideWindow();
      }
    });
  }

  createTray() {
    const trayIcon = nativeImage.createFromPath(
      path.join(__dirname, "assets/note.png")
    );
    this.tray = new Tray(trayIcon.resize({ width: 16, height: 16 }));

    const contextMenu = Menu.buildFromTemplate([
      {
        label: "Show Clipped",
        click: () => this.showWindow(),
      },
      {
        label: "Clear History",
        click: () => this.clearHistory(),
      },
      { type: "separator" },
      {
        label: "Quit",
        click: () => this.quitApp(),
      },
    ]);

    this.tray.setContextMenu(contextMenu);
    this.tray.setToolTip("Clipped - Native Clipboard Manager");

    this.tray.on("click", () => {
      this.toggleWindow();
    });
  }

  setupGlobalShortcuts() {
    // Get the saved hotkey from settings (default to Super+V)
    const primaryHotkey = store.get("hotkey", "Super+V");
    console.log(`Attempting to register primary hotkey: ${primaryHotkey}`);

    try {
      globalShortcut.register(primaryHotkey, () => {
        this.toggleWindow();
      });
      console.log(`Global shortcut ${primaryHotkey} registered successfully`);
    } catch (error) {
      console.error(`Failed to register ${primaryHotkey}:`, error);

      // If custom hotkey fails, try fallback options
      const fallbacks = ["Super+V", "Alt+V", "CommandOrControl+Alt+V"];
      let registered = false;

      for (const fallback of fallbacks) {
        if (fallback !== primaryHotkey) {
          // Don't try the same key again
          try {
            console.log(`Trying fallback: ${fallback}`);
            globalShortcut.register(fallback, () => {
              this.toggleWindow();
            });
            console.log(`Global shortcut ${fallback} registered as fallback`);
            // Update stored hotkey to the working fallback
            store.set("hotkey", fallback);
            registered = true;
            break;
          } catch (fallbackError) {
            console.error(
              `Failed to register fallback ${fallback}:`,
              fallbackError
            );
          }
        }
      }

      if (!registered) {
        console.error("Failed to register any global shortcut!");
      }
    }
  }

  setupIPCHandlers() {
    // Get clipboard history
    ipcMain.handle("get-clipboard-history", () => {
      return this.clipboardHistory;
    });

    // Paste text directly to active application (legacy)
    ipcMain.handle("paste-text", (event, text) => {
      return this.pasteTextNatively(text);
    });

    // Paste item (text or image) directly to active application
    ipcMain.handle("paste-item", (event, item) => {
      return this.pasteItem(item);
    });

    // Copy text to clipboard and hide window
    ipcMain.handle("copy-text", (event, text) => {
      clipboard.writeText(text);
      this.hideWindow();
      return true;
    });

    // Copy image to clipboard and hide window
    ipcMain.handle("copy-image", (event, dataURL) => {
      const nativeImage = require("electron").nativeImage;
      const image = nativeImage.createFromDataURL(dataURL);
      clipboard.writeImage(image);
      this.hideWindow();
      return true;
    });

    // Pin/unpin item
    ipcMain.handle("toggle-pin", (event, index) => {
      if (this.clipboardHistory[index]) {
        this.clipboardHistory[index].pinned =
          !this.clipboardHistory[index].pinned;
        this.saveHistory();
        return true;
      }
      return false;
    });

    // Delete item
    ipcMain.handle("delete-item", (event, index) => {
      if (this.clipboardHistory[index]) {
        this.clipboardHistory.splice(index, 1);
        this.saveHistory();
        return true;
      }
      return false;
    });

    // Clear all history
    ipcMain.handle("clear-history", () => {
      this.clearHistory();
      return true;
    });

    // Window controls
    ipcMain.handle("hide-window", () => {
      this.hideWindow();
      return true;
    });

    // Search history
    ipcMain.handle("search-history", (event, query) => {
      if (!query) return this.clipboardHistory;
      return this.clipboardHistory.filter((item) =>
        item.content.toLowerCase().includes(query.toLowerCase())
      );
    });

    // Recent emojis management
    ipcMain.handle("add-recent-emoji", (event, emoji) => {
      let recentEmojis = store.get("recentEmojis", []);
      // Remove if already exists
      recentEmojis = recentEmojis.filter((e) => e !== emoji);
      // Add to beginning
      recentEmojis.unshift(emoji);
      // Keep only last 20
      recentEmojis = recentEmojis.slice(0, 20);
      store.set("recentEmojis", recentEmojis);
      return recentEmojis;
    });

    ipcMain.handle("get-recent-emojis", () => {
      return store.get("recentEmojis", []);
    });

    // Hotkey management
    ipcMain.handle("get-current-hotkey", () => {
      return store.get("hotkey", "Super+V");
    });

    ipcMain.handle("set-hotkey", (event, hotkeyString) => {
      try {
        // Unregister all existing shortcuts first
        globalShortcut.unregisterAll();

        // Save new hotkey to store
        store.set("hotkey", hotkeyString);

        // Register the new hotkey directly
        globalShortcut.register(hotkeyString, () => {
          this.toggleWindow();
        });

        console.log(`Custom hotkey ${hotkeyString} registered successfully`);
        return true;
      } catch (error) {
        console.error("Failed to set custom hotkey:", error);

        // If custom hotkey fails, fallback to setupGlobalShortcuts
        try {
          this.setupGlobalShortcuts();
          return true;
        } catch (fallbackError) {
          console.error("Failed to register any hotkey:", fallbackError);
          return false;
        }
      }
    });
  }

  startClipboardWatcher() {
    this.lastClipboardContent = clipboard.readText();
    this.lastClipboardImage = clipboard.readImage();
    this.lastImageDataURL = this.lastClipboardImage.isEmpty()
      ? ""
      : this.lastClipboardImage.toDataURL();

    this.clipboardWatcher = setInterval(() => {
      // Check for text content
      const currentContent = clipboard.readText();
      if (
        currentContent &&
        currentContent !== this.lastClipboardContent &&
        currentContent.trim().length > 0
      ) {
        this.addToHistory({
          type: "text",
          content: currentContent,
          timestamp: new Date().toISOString(),
          pinned: false,
          id: Date.now(),
        });
        this.lastClipboardContent = currentContent;
      }

      // Check for image content
      const currentImage = clipboard.readImage();
      if (!currentImage.isEmpty()) {
        const imageDataURL = currentImage.toDataURL();

        // Check if this is a new image (compare data URLs)
        if (
          imageDataURL !== this.lastImageDataURL &&
          imageDataURL.length > 100
        ) {
          this.addToHistory({
            type: "image",
            content: imageDataURL,
            timestamp: new Date().toISOString(),
            pinned: false,
            id: Date.now(),
          });
          this.lastClipboardImage = currentImage;
          this.lastImageDataURL = imageDataURL;
        }
      }
    }, 500);
  }

  startActiveWindowTracker() {
    if (!isWaylandSession()) {
      // Track active window info every 1 second (less frequent than clipboard)
      this.activeWindowTracker = setInterval(() => {
        // Only track if our clipboard window is not active
        if (!this.mainWindow || !this.mainWindow.isFocused()) {
          exec(
            "xdotool getactivewindow getwindowgeometry --shell",
            (err, stdout) => {
              if (!err && stdout) {
                const lines = stdout.trim().split("\n");
                const windowInfo = {};

                for (const line of lines) {
                  if (line.startsWith("X="))
                    windowInfo.x = parseInt(line.split("=")[1]);
                  else if (line.startsWith("Y="))
                    windowInfo.y = parseInt(line.split("=")[1]);
                  else if (line.startsWith("WIDTH="))
                    windowInfo.width = parseInt(line.split("=")[1]);
                  else if (line.startsWith("HEIGHT="))
                    windowInfo.height = parseInt(line.split("=")[1]);
                }

                // Only update if we got valid data and it's not the full screen
                if (
                  windowInfo.x !== undefined &&
                  windowInfo.y !== undefined &&
                  !(
                    windowInfo.x === 0 &&
                    windowInfo.y === 0 &&
                    windowInfo.width >= 1920
                  )
                ) {
                  this.lastActiveWindowInfo = windowInfo;
                }
              }
            }
          );
        }
      }, 1000);
    }
  }

  startActiveWindowTracker() {
    if (!isWaylandSession()) {
      // Track active window info every 1 second (less frequent than clipboard)
      this.activeWindowTracker = setInterval(() => {
        // Only track if our clipboard window is not active
        if (!this.mainWindow || !this.mainWindow.isFocused()) {
          exec(
            "xdotool getactivewindow getwindowgeometry --shell",
            (err, stdout) => {
              if (!err && stdout) {
                const lines = stdout.trim().split("\n");
                const windowInfo = {};

                for (const line of lines) {
                  if (line.startsWith("X="))
                    windowInfo.x = parseInt(line.split("=")[1]);
                  else if (line.startsWith("Y="))
                    windowInfo.y = parseInt(line.split("=")[1]);
                  else if (line.startsWith("WIDTH="))
                    windowInfo.width = parseInt(line.split("=")[1]);
                  else if (line.startsWith("HEIGHT="))
                    windowInfo.height = parseInt(line.split("=")[1]);
                }

                // Only update if we got valid data
                if (windowInfo.x !== undefined && windowInfo.y !== undefined) {
                  this.lastActiveWindowInfo = windowInfo;
                }
              }
            }
          );
        }
      }, 1000);
    }
  }

  addToHistory(item) {
    // Remove if already exists (compare content)
    this.clipboardHistory = this.clipboardHistory.filter(
      (existingItem) => existingItem.content !== item.content
    );

    // Add to beginning
    this.clipboardHistory.unshift(item);

    // Limit history size
    const maxItems = store.get("maxItems", 50);
    this.clipboardHistory = this.clipboardHistory.slice(0, maxItems);

    this.saveHistory();

    // Notify renderer
    if (this.mainWindow && !this.mainWindow.isDestroyed()) {
      this.mainWindow.webContents.send(
        "clipboard-updated",
        this.clipboardHistory
      );
    }
  }

  // NATIVE PASTE FUNCTIONALITY - This is the key feature!
  pasteItem(item) {
    return new Promise((resolve) => {
      // Put data on the system clipboard first
      if (item.type === "image") {
        const nativeImage = require("electron").nativeImage;
        const image = nativeImage.createFromDataURL(item.content);
        clipboard.writeImage(image);
      } else {
        clipboard.writeText(item.content);
      }

      // Hide our window so focus returns to previous app
      this.hideWindow();

      const delayMs = 10; // faster paste response

      const runPasteSequence = () => {
        const wayland = isWaylandSession();
        const commands = [];

        if (wayland) {
          commands.push("wtype -M ctrl v -m ctrl");
          if (item.type !== "image") {
            const textArg = shellEscape(item.content);
            commands.push(`wtype --delay 1 ${textArg}`);
          }
        } else {
          commands.push("xdotool key --clearmodifiers ctrl+v");
          if (item.type !== "image") {
            const textArg = shellEscape(item.content);
            commands.push(`xdotool type --clearmodifiers --delay 1 ${textArg}`);
            commands.push(
              "xclip -selection clipboard -o | xdotool type --clearmodifiers --file -"
            );
          }
        }

        tryCommandsSequentially(commands, () => {
          this.lastActiveWindowId = null;
          resolve(true);
        });
      };

      setTimeout(() => {
        if (!isWaylandSession() && this.lastActiveWindowId) {
          exec(
            `xdotool windowactivate --sync ${this.lastActiveWindowId}`,
            () => {
              runPasteSequence();
            }
          );
        } else {
          runPasteSequence();
        }
      }, delayMs);
    });
  }

  // Legacy function for text-only pasting (for backward compatibility)
  pasteTextNatively(text) {
    return this.pasteItem({ type: "text", content: text });
  }

  saveHistory() {
    store.set("clipboardHistory", this.clipboardHistory);
  }

  clearHistory() {
    this.clipboardHistory = [];
    this.saveHistory();

    if (this.mainWindow && !this.mainWindow.isDestroyed()) {
      this.mainWindow.webContents.send(
        "clipboard-updated",
        this.clipboardHistory
      );
    }
  }

  showWindow() {
    if (this.mainWindow) {
      console.log("showWindow called - positioning at cursor...");
      // Position at cursor BEFORE showing to avoid visual jump
      this.positionAtCursor(() => {
        console.log("positionAtCursor callback - showing window");
        this.mainWindow.show();
        this.mainWindow.focus();
      });
    }
  }

  hideWindow() {
    if (this.mainWindow) {
      this.mainWindow.hide();
    }
  }

  toggleWindow() {
    if (!this.mainWindow) return;

    // Prevent double-trigger with debouncing
    if (this.toggleInProgress) {
      console.log("toggleWindow called but already in progress - ignoring");
      return;
    }

    this.toggleInProgress = true;
    console.log(
      `toggleWindow called - window visible: ${this.mainWindow.isVisible()}`
    );

    if (this.mainWindow.isVisible()) {
      this.hideWindow();
      // Reset the flag immediately for hide
      this.toggleInProgress = false;
    } else {
      this.showWindow();
      // Reset the flag after a longer delay to allow show to complete
      setTimeout(() => {
        this.toggleInProgress = false;
        console.log("toggleWindow debounce reset");
      }, 500);
    }
  }

  centerWindow() {
    if (this.mainWindow) {
      const { screen } = require("electron");
      const primaryDisplay = screen.getPrimaryDisplay();
      const { width, height } = primaryDisplay.workAreaSize;
      const windowBounds = this.mainWindow.getBounds();

      const x = Math.round((width - windowBounds.width) / 2);
      const y = Math.round((height - windowBounds.height) / 2);

      this.mainWindow.setPosition(x, y);
    }
  }

  positionAtCursor(callback) {
    if (this.mainWindow) {
      const { screen } = require("electron");
      const primaryDisplay = screen.getPrimaryDisplay();
      const { width, height } = primaryDisplay.workAreaSize;
      const windowBounds = this.mainWindow.getBounds();

      // Get the window with keyboard focus and use mouse position within that window
      if (!isWaylandSession()) {
        exec("xdotool getwindowfocus", (err, stdout) => {
          if (!err && stdout) {
            const windowId = stdout.trim();
            // Get window name to verify it's VS Code
            exec(`xdotool getwindowname ${windowId}`, (err2, windowName) => {
              const name = windowName ? windowName.trim() : "Unknown";

              // If it's VS Code, use mouse position (which should be near text cursor)
              if (
                name.includes("Visual Studio Code") ||
                name.includes("Code")
              ) {
                const cursorPoint = screen.getCursorScreenPoint();

                // Position popup near mouse cursor within VS Code
                let x = cursorPoint.x + 15;
                let y = cursorPoint.y + 15;

                // Keep window within screen bounds
                if (x + windowBounds.width > width) {
                  x = cursorPoint.x - windowBounds.width - 15;
                }
                if (y + windowBounds.height > height) {
                  y = cursorPoint.y - windowBounds.height - 15;
                }

                // Ensure minimum distance from screen edges
                x = Math.max(10, Math.min(x, width - windowBounds.width - 10));
                y = Math.max(
                  10,
                  Math.min(y, height - windowBounds.height - 10)
                );

                console.log(`Focused on VS Code: "${name}"`);
                console.log(
                  `Using mouse position: ${cursorPoint.x}, ${cursorPoint.y}`
                );
                console.log(`Setting popup position to: ${x}, ${y}`);
                this.mainWindow.setPosition(x, y);
                if (callback) callback();
              } else {
                // If not VS Code, try to get window geometry
                exec(
                  `xdotool getwindowgeometry --shell ${windowId}`,
                  (err3, stdout3) => {
                    if (!err3 && stdout3) {
                      const lines = stdout3.trim().split("\n");
                      let activeX = 0,
                        activeY = 0,
                        activeWidth = 800,
                        activeHeight = 600;

                      for (const line of lines) {
                        if (line.startsWith("X="))
                          activeX = parseInt(line.split("=")[1]);
                        else if (line.startsWith("Y="))
                          activeY = parseInt(line.split("=")[1]);
                        else if (line.startsWith("WIDTH="))
                          activeWidth = parseInt(line.split("=")[1]);
                        else if (line.startsWith("HEIGHT="))
                          activeHeight = parseInt(line.split("=")[1]);
                      }

                      let x = activeX + Math.round(activeWidth * 0.3);
                      let y = activeY + Math.round(activeHeight * 0.4);

                      if (x + windowBounds.width > width) {
                        x = activeX + activeWidth - windowBounds.width - 20;
                      }
                      if (y + windowBounds.height > height) {
                        y = activeY + activeHeight - windowBounds.height - 20;
                      }

                      x = Math.max(
                        10,
                        Math.min(x, width - windowBounds.width - 10)
                      );
                      y = Math.max(
                        10,
                        Math.min(y, height - windowBounds.height - 10)
                      );

                      console.log(`Focused on non-VS Code: "${name}"`);
                      console.log(
                        `Using window geometry: ${activeX},${activeY} ${activeWidth}x${activeHeight}`
                      );
                      console.log(`Setting popup position to: ${x}, ${y}`);
                      this.mainWindow.setPosition(x, y);
                      if (callback) callback();
                    } else {
                      console.log(
                        "Could not get window geometry, using fallback"
                      );
                      this.positionAtPointer();
                      if (callback) callback();
                    }
                  }
                );
              }
            });
          } else {
            console.log("Could not get focused window, using fallback");
            this.positionAtPointer();
            if (callback) callback();
          }
        });
      } else {
        // On Wayland, fallback to pointer position
        this.positionAtPointer();
        if (callback) callback();
      }
    } else if (callback) {
      callback();
    }
  }

  positionAtPointer() {
    if (this.mainWindow) {
      const { screen } = require("electron");
      const cursorPoint = screen.getCursorScreenPoint();
      const primaryDisplay = screen.getPrimaryDisplay();
      const { width, height } = primaryDisplay.workAreaSize;
      const windowBounds = this.mainWindow.getBounds();

      let x = cursorPoint.x + 10;
      let y = cursorPoint.y + 10;

      if (x + windowBounds.width > width) {
        x = cursorPoint.x - windowBounds.width - 10;
      }
      if (y + windowBounds.height > height) {
        y = cursorPoint.y - windowBounds.height - 10;
      }

      x = Math.max(10, Math.min(x, width - windowBounds.width - 10));
      y = Math.max(10, Math.min(y, height - windowBounds.height - 10));

      this.mainWindow.setPosition(x, y);
    }
  }

  positionAtPointer() {
    if (this.mainWindow) {
      const { screen } = require("electron");
      const cursorPoint = screen.getCursorScreenPoint();
      const primaryDisplay = screen.getPrimaryDisplay();
      const { width, height } = primaryDisplay.workAreaSize;
      const windowBounds = this.mainWindow.getBounds();

      let x = cursorPoint.x + 10;
      let y = cursorPoint.y + 10;

      if (x + windowBounds.width > width) {
        x = cursorPoint.x - windowBounds.width - 10;
      }
      if (y + windowBounds.height > height) {
        y = cursorPoint.y - windowBounds.height - 10;
      }

      x = Math.max(10, Math.min(x, width - windowBounds.width - 10));
      y = Math.max(10, Math.min(y, height - windowBounds.height - 10));

      this.mainWindow.setPosition(x, y);
    }
  }
  setupAppEvents() {
    app.on("window-all-closed", () => {
      // keep running in tray
    });
    app.on("before-quit", () => {
      this.isQuitting = true;
      if (this.clipboardWatcher) clearInterval(this.clipboardWatcher);
      if (this.activeWindowTracker) clearInterval(this.activeWindowTracker);
    });
    app.on("will-quit", () => {
      globalShortcut.unregisterAll();
    });
  }

  quitApp() {
    this.isQuitting = true;
    if (this.tray) this.tray.destroy();
    app.quit();
  }
}

// Initialize
const clipboardManager = new NativeClipboardManager();
app.whenReady().then(() => clipboardManager.initialize());

// Single instance
const gotTheLock = app.requestSingleInstanceLock();
if (!gotTheLock) {
  app.quit();
} else {
  app.on("second-instance", () => {
    if (clipboardManager.mainWindow) {
      clipboardManager.showWindow();
    }
  });
}
