const { contextBridge, ipcRenderer } = require("electron");

contextBridge.exposeInMainWorld("clipboardAPI", {
  // Get clipboard history
  getHistory: () => ipcRenderer.invoke("get-clipboard-history"),

  // NATIVE PASTE - This is the key functionality!
  pasteText: (text) => ipcRenderer.invoke("paste-text", text),
  pasteItem: (item) => ipcRenderer.invoke("paste-item", item),

  // Copy text to clipboard
  copyText: (text) => ipcRenderer.invoke("copy-text", text),
  copyImage: (dataURL) => ipcRenderer.invoke("copy-image", dataURL),

  // Item operations
  togglePin: (index) => ipcRenderer.invoke("toggle-pin", index),
  deleteItem: (index) => ipcRenderer.invoke("delete-item", index),
  clearHistory: () => ipcRenderer.invoke("clear-history"),

  // Search
  searchHistory: (query) => ipcRenderer.invoke("search-history", query),

  // Recent emojis
  addRecentEmoji: (emoji) => ipcRenderer.invoke("add-recent-emoji", emoji),
  getRecentEmojis: () => ipcRenderer.invoke("get-recent-emojis"),

  // Window control
  hideWindow: () => ipcRenderer.invoke("hide-window"),

  // Hotkey management
  getCurrentHotkey: () => ipcRenderer.invoke("get-current-hotkey"),
  setHotkey: (hotkeyString) => ipcRenderer.invoke("set-hotkey", hotkeyString),

  // Event listeners
  onClipboardUpdated: (callback) => {
    ipcRenderer.on("clipboard-updated", (event, history) => callback(history));
  },

  removeAllListeners: () => {
    ipcRenderer.removeAllListeners("clipboard-updated");
  },
});
