// Native Clipboard Manager Renderer
// This handles the UI and communicates with the main process for native clipboard functionality

class NativeClipboardApp {
  constructor() {
    this.currentPage = "clipboardPage";
    this.clipboardHistory = [];
    this.recentEmojis = [];
    this.currentEmojiCategory = "recent";
    this.searchQuery = "";
    this.dropdownTimeout = null;

    this.init();
  }

  async init() {
    await this.loadClipboardHistory();
    await this.loadRecentEmojis();
    this.setupEventListeners();
    this.setupIPCListeners();
    this.renderClipboardHistory();
    // Clear emoji content on init to ensure no emoji content on other pages
    const emojiList = document.getElementById("emojiList");
    const emojiCategories = document.querySelector(".emoji-categories");
    if (emojiList) {
      emojiList.innerHTML = "";
    }
    if (emojiCategories) {
      emojiCategories.style.display = "none";
    }
    // Don't render emoji picker on init - only when emoji page is shown
  }

  async loadClipboardHistory() {
    try {
      this.clipboardHistory = await window.clipboardAPI.getHistory();
    } catch (error) {
      console.error("Failed to load clipboard history:", error);
      this.clipboardHistory = [];
    }
  }

  async loadRecentEmojis() {
    try {
      this.recentEmojis = await window.clipboardAPI.getRecentEmojis();
    } catch (error) {
      console.error("Failed to load recent emojis:", error);
      this.recentEmojis = [];
    }
  }

  setupEventListeners() {
    // Navigation buttons
    document.getElementById("clipboardBtn").addEventListener("click", () => {
      this.showPage("clipboardPage");
    });

    document.getElementById("emojiBtn").addEventListener("click", () => {
      this.showPage("emojiPage");
    });

    document.getElementById("settingsBtn").addEventListener("click", () => {
      this.showPage("settingsPage");
    });

    // Window controls
    document.getElementById("clearBtn").addEventListener("click", () => {
      window.clipboardAPI.hideWindow();
    });

    // Search functionality
    const searchInput = document.getElementById("searchInput");
    if (searchInput) {
      searchInput.addEventListener("input", (e) => {
        this.searchQuery = e.target.value;
        this.renderClipboardHistory();
      });
    }

    // Clear clipboard button in settings (updated for new design)
    const clearClipboardBtn = document.getElementById("clearClipboardBtn");
    if (clearClipboardBtn) {
      clearClipboardBtn.addEventListener("click", () => {
        this.clearAllHistory();
      });
    }

    // Edit hotkey button (new button for the beautiful design)
    const editHotkeyBtn = document.getElementById("editHotkeyBtn");
    if (editHotkeyBtn) {
      editHotkeyBtn.addEventListener("click", () => {
        this.toggleHotkeyRecording();
      });
    }

    // Hotkey recording
    this.setupHotkeyRecording();

    // Global click handler for dropdowns
    document.addEventListener("click", (e) => {
      if (!e.target.closest(".clipboard-actions")) {
        this.closeAllDropdowns();
      }
    });

    // Keyboard shortcuts
    document.addEventListener("keydown", (e) => {
      if (e.key === "Escape") {
        window.clipboardAPI.hideWindow();
      }
    });

    // Emoji category buttons
    document.addEventListener("click", (e) => {
      if (e.target.classList.contains("emoji-category")) {
        const category = e.target.dataset.category;
        this.switchEmojiCategory(category);
      }
    });
  }

  setupIPCListeners() {
    // Listen for clipboard updates from main process
    window.clipboardAPI.onClipboardUpdated((history) => {
      this.clipboardHistory = history;
      this.renderClipboardHistory();
    });
  }

  showPage(pageId) {
    // Clear emoji list when leaving emoji page
    if (this.currentPage === "emojiPage" && pageId !== "emojiPage") {
      const emojiList = document.getElementById("emojiList");
      if (emojiList) {
        emojiList.innerHTML = "";
      }
    }

    // Hide all pages
    document.querySelectorAll(".page").forEach((page) => {
      page.classList.remove("active");
    });

    // Show selected page
    const targetPage = document.getElementById(pageId);
    if (targetPage) {
      targetPage.classList.add("active");
      this.currentPage = pageId;
    }

    // Update button states
    document.querySelectorAll(".icon-btn").forEach((btn) => {
      btn.classList.remove("active");
    });

    if (pageId === "clipboardPage") {
      document.getElementById("clipboardBtn").classList.add("active");
      // Ensure emoji content is cleared when going to clipboard
      const emojiList = document.getElementById("emojiList");
      const emojiCategories = document.querySelector(".emoji-categories");
      if (emojiList) {
        emojiList.innerHTML = "";
        emojiList.style.display = "none";
      }
      if (emojiCategories) {
        emojiCategories.style.display = "none";
      }
    } else if (pageId === "emojiPage") {
      document.getElementById("emojiBtn").classList.add("active");
      // Ensure emoji elements are restored when going to emoji page
      const emojiList = document.getElementById("emojiList");
      const emojiCategories = document.querySelector(".emoji-categories");
      if (emojiList) {
        emojiList.style.display = "grid";
      }
      if (emojiCategories) {
        emojiCategories.style.display = "flex";
      }
      this.renderEmojiPicker();
    } else if (pageId === "settingsPage") {
      document.getElementById("settingsBtn").classList.add("active");
      // Ensure all emoji content is completely hidden when going to settings
      const emojiList = document.getElementById("emojiList");
      const emojiCategories = document.querySelector(".emoji-categories");
      if (emojiList) {
        emojiList.innerHTML = "";
        emojiList.style.display = "none";
      }
      if (emojiCategories) {
        emojiCategories.style.display = "none";
      }
    }
  }

  async renderClipboardHistory() {
    const clipboardList = document.getElementById("clipboardList");
    const emptyState = document.getElementById("emptyState");

    if (!clipboardList) return;

    // Filter by search query if present
    let filteredHistory = this.clipboardHistory;
    if (this.searchQuery) {
      try {
        filteredHistory = await window.clipboardAPI.searchHistory(
          this.searchQuery
        );
      } catch (error) {
        console.error("Search failed:", error);
      }
    }

    // Clear existing items (except empty state)
    const existingItems = clipboardList.querySelectorAll(".clipboard-item");
    existingItems.forEach((item) => item.remove());

    if (filteredHistory.length === 0) {
      if (emptyState) {
        emptyState.style.display = "flex";
      }
      return;
    }

    if (emptyState) {
      emptyState.style.display = "none";
    }

    // Sort: pinned items first
    filteredHistory.sort((a, b) => {
      if (a.pinned && !b.pinned) return -1;
      if (!a.pinned && b.pinned) return 1;
      return 0;
    });

    // Render items
    filteredHistory.forEach((item, displayIndex) => {
      const itemElement = this.createClipboardItem(item, item.id);
      clipboardList.appendChild(itemElement);
    });
  }

  createClipboardItem(item, itemId) {
    const itemDiv = document.createElement("div");
    itemDiv.className = `clipboard-item ${item.pinned ? "pinned" : ""} ${
      item.type || "text"
    }`;

    // Format content based on type
    let displayContent;
    let titleContent;

    if (item.type === "image") {
      console.log("Processing image item");
      console.log("Image content starts with:", item.content.substring(0, 50));
      console.log("Image content length:", item.content.length);

      itemDiv.innerHTML = `
        <div class="image-container">
          <img src="${
            item.content
          }" alt="Clipboard Image" class="clipboard-image" 
               onerror="console.log('Image failed to load:', this.src)" 
               onload="console.log('Image loaded successfully')" />
        </div>
        <div class="clipboard-actions">
          <button class="menu-btn" title="Options">⋮</button>
          <div class="dropdown-menu">
            <div class="dropdown-option pin-option">
              ${item.pinned ? "Unpin" : "Pin"}
            </div>
            <div class="dropdown-option delete-option">Delete</div>
          </div>
        </div>
      `;

      titleContent = "Image";
    } else {
      // Text content
      displayContent = item.content;
      if (displayContent.length > 200) {
        displayContent = displayContent.substring(0, 200) + "...";
      }
      titleContent = item.content.replace(/"/g, "&quot;");
    }

    // Format timestamp
    const date = new Date(item.timestamp);
    const timeString = date.toLocaleTimeString([], {
      hour: "2-digit",
      minute: "2-digit",
    });

    if (item.type === "image") {
      itemDiv.innerHTML = `
        <div class="image-container">
          <img src="${
            item.content
          }" alt="Clipboard Image" class="clipboard-image" 
               onerror="console.log('Image failed to load. Source:', this.src.substring(0, 100))" 
               onload="console.log('Image loaded successfully')" />
        </div>
        <div class="clipboard-actions">
          <button class="menu-btn" title="Options">⋮</button>
          <div class="dropdown-menu">
            <div class="dropdown-option pin-option">
              ${item.pinned ? "Unpin" : "Pin"}
            </div>
            <div class="dropdown-option delete-option">Delete</div>
          </div>
        </div>
      `;
    } else {
      itemDiv.innerHTML = `
        <div class="clipboard-text" title="${titleContent}">${displayContent}</div>
        <div class="clipboard-actions">
          <button class="menu-btn" title="Options">⋮</button>
          <div class="dropdown-menu">
            <div class="dropdown-option pin-option">
              ${item.pinned ? "Unpin" : "Pin"}
            </div>
            <div class="dropdown-option delete-option">Delete</div>
          </div>
        </div>
      `;
    }

    // MAIN CLICK EVENT - NATIVE PASTE FUNCTIONALITY
    itemDiv.addEventListener("click", (e) => {
      if (!e.target.closest(".clipboard-actions")) {
        this.pasteItem(item);
      }
    });

    // Menu button
    const menuBtn = itemDiv.querySelector(".menu-btn");
    menuBtn.addEventListener("click", (e) => {
      e.stopPropagation();
      this.toggleDropdown(menuBtn);
    });

    // Dropdown options
    const pinOption = itemDiv.querySelector(".pin-option");
    pinOption.addEventListener("click", (e) => {
      e.stopPropagation();
      this.togglePin(itemId);
      this.closeAllDropdowns();
    });

    const deleteOption = itemDiv.querySelector(".delete-option");
    deleteOption.addEventListener("click", (e) => {
      e.stopPropagation();
      this.deleteItem(itemId);
      this.closeAllDropdowns();
    });

    return itemDiv;
  }

  // NATIVE PASTE FUNCTIONALITY - The key feature!
  async pasteItem(item) {
    try {
      await window.clipboardAPI.pasteItem(item);
      // Main process handles hiding based on preferences
    } catch (error) {
      // Fallback to copy for text items
      if (item.type !== "image") {
        this.copyText(item.content);
      }
    }
  }

  // Legacy function for text-only pasting
  async pasteTextNatively(text) {
    await this.pasteItem({ type: "text", content: text });
  }

  async copyItem(item) {
    try {
      if (item.type === "image") {
        await window.clipboardAPI.copyImage(item.content);
      } else {
        await window.clipboardAPI.copyText(item.content);
      }
    } catch (error) {
      console.error("Failed to copy item:", error);
    }
  }

  async copyText(text) {
    try {
      await window.clipboardAPI.copyText(text);
    } catch (error) {
      console.error("Failed to copy text:", error);
    }
  }

  async togglePin(itemId) {
    try {
      await window.clipboardAPI.togglePin(itemId);
      await this.loadClipboardHistory();
      this.renderClipboardHistory();
    } catch (error) {
      console.error("Failed to toggle pin:", error);
    }
  }

  async deleteItem(itemId) {
    try {
      await window.clipboardAPI.deleteItem(itemId);
      await this.loadClipboardHistory();
      this.renderClipboardHistory();
    } catch (error) {
      console.error("Failed to delete item:", error);
    }
  }

  async clearAllHistory() {
    try {
      await window.clipboardAPI.clearHistory();
      await this.loadClipboardHistory();
      this.renderClipboardHistory();
    } catch (error) {
      console.error("Failed to clear history:", error);
    }
  }

  async renderEmojiPicker(category = "recent") {
    const emojiList = document.getElementById("emojiList");
    if (!emojiList) return;

    // Only render if emoji page is currently active
    const emojiPage = document.getElementById("emojiPage");
    if (!emojiPage || !emojiPage.classList.contains("active")) return;

    // Clear existing emojis
    emojiList.innerHTML = "";
    this.currentEmojiCategory = category;

    // Update category button states
    document.querySelectorAll(".emoji-category").forEach((btn) => {
      btn.classList.remove("active");
      if (btn.dataset.category === category) {
        btn.classList.add("active");
      }
    });

    let emojisToShow = [];

    if (category === "recent") {
      emojisToShow = this.recentEmojis;
      if (emojisToShow.length === 0) {
        emojiList.innerHTML =
          '<div class="emoji-empty"><div class="emoji-empty-text">No recent emojis — Click any emoji to add it to recents</div></div>';
        return;
      }
    } else {
      emojisToShow = this.getEmojisByCategory(category);
    }

    // Render emojis
    emojisToShow.forEach((emoji) => {
      const emojiSpan = this.createEmojiElement(emoji);
      emojiList.appendChild(emojiSpan);
    });
  }

  getEmojisByCategory(category) {
    const emojiCategories = {
      smileys: [
        "😀",
        "😃",
        "😄",
        "😁",
        "😆",
        "😅",
        "🤣",
        "😂",
        "🙂",
        "🙃",
        "😉",
        "😊",
        "😇",
        "🥰",
        "😍",
        "🤩",
        "😘",
        "😗",
        "😚",
        "😙",
        "😋",
        "😛",
        "😜",
        "🤪",
        "😝",
        "🤑",
        "🤗",
        "🤭",
        "🤫",
        "🤔",
        "🤐",
        "🤨",
        "😐",
        "😑",
        "😶",
        "😏",
        "😒",
        "🙄",
        "😬",
        "🤥",
        "😔",
        "😪",
        "🤤",
        "😴",
        "😷",
        "🤒",
        "🤕",
        "🤢",
        "🤮",
        "🤧",
        "🥵",
        "🥶",
        "🥴",
        "😵",
        "🤯",
        "🤠",
        "🥳",
        "😎",
        "🤓",
        "🧐",
        "😕",
        "😟",
        "🙁",
        "😮",
        "😯",
        "😲",
        "😳",
        "🥺",
        "😦",
        "😧",
        "😨",
        "😰",
        "😥",
        "😢",
        "😭",
        "😱",
        "😖",
        "😣",
        "😞",
        "😓",
        "😩",
        "😫",
        "🥱",
        "😤",
        "😡",
        "😠",
        "🤬",
        "😈",
        "👿",
        "💀",
        "☠️",
        "💩",
        "🤡",
        "👹",
        "👺",
        "👻",
        "👽",
        "👾",
        "🤖",
        "😺",
        "😸",
        "😹",
        "😻",
        "😼",
        "😽",
        "🙀",
        "😿",
        "😾",
      ],
      people: [
        "👋",
        "🤚",
        "🖐️",
        "✋",
        "🖖",
        "👌",
        "🤌",
        "🤏",
        "✌️",
        "🤞",
        "🤟",
        "🤘",
        "🤙",
        "👈",
        "👉",
        "👆",
        "🖕",
        "👇",
        "☝️",
        "👍",
        "👎",
        "👊",
        "✊",
        "🤛",
        "🤜",
        "👏",
        "🙌",
        "👐",
        "🤲",
        "🤝",
        "🙏",
        "✍️",
        "💅",
        "🤳",
        "💪",
        "🦾",
        "🦿",
        "🦵",
        "🦶",
        "👂",
        "🦻",
        "👃",
        "🧠",
        "🫀",
        "🫁",
        "🦷",
        "🦴",
        "👀",
        "👁️",
        "👅",
        "👄",
        "💋",
        "👶",
        "🧒",
        "👦",
        "👧",
        "🧑",
        "👱",
        "👨",
        "🧔",
        "👩",
        "🧓",
        "👴",
        "👵",
        "🙍",
        "🙎",
        "🙅",
        "🙆",
        "💁",
        "🙋",
        "🧏",
        "🙇",
        "🤦",
        "🤷",
        "👮",
        "🕵️",
        "💂",
        "🥷",
        "👷",
        "🤴",
        "👸",
        "👳",
        "👲",
        "🧕",
        "🤵",
        "👰",
        "🤰",
        "🤱",
        "👼",
        "🎅",
        "🤶",
        "🦸",
        "🦹",
        "🧙",
        "🧚",
        "🧛",
        "🧜",
        "🧝",
        "🧞",
        "🧟",
        "💆",
        "💇",
        "🚶",
        "🧍",
        "🧎",
        "🏃",
        "💃",
        "🕺",
        "🕴️",
        "👯",
        "🧖",
        "🧗",
        "🤺",
        "🏇",
        "⛷️",
        "🏂",
        "🏌️",
        "🏄",
        "🚣",
        "🏊",
        "⛹️",
        "🏋️",
        "🚴",
        "🚵",
        "🤸",
        "🤼",
        "🤽",
        "🤾",
        "🤹",
        "🧘",
        "🛀",
        "🛌",
        "👭",
        "👫",
        "👬",
        "💏",
        "💑",
        "👪",
      ],
      animals: [
        "🐶",
        "🐱",
        "🐭",
        "🐹",
        "🐰",
        "🦊",
        "🐻",
        "🐼",
        "🐨",
        "🐯",
        "🦁",
        "🐮",
        "🐷",
        "🐽",
        "🐸",
        "🐵",
        "🙈",
        "🙉",
        "🙊",
        "🐒",
        "🐔",
        "🐧",
        "🐦",
        "🐤",
        "🐣",
        "🐥",
        "🦆",
        "🦅",
        "🦉",
        "🦇",
        "🐺",
        "🐗",
        "🐴",
        "🦄",
        "🐝",
        "🐛",
        "🦋",
        "🐌",
        "🐞",
        "🐜",
        "🦟",
        "🦗",
        "🕷️",
        "🕸️",
        "🦂",
        "🐢",
        "🐍",
        "🦎",
        "🦖",
        "🦕",
        "🐙",
        "🦑",
        "🦐",
        "🦞",
        "🦀",
        "🐡",
        "🐠",
        "🐟",
        "🐬",
        "🐳",
        "🐋",
        "🦈",
        "🐊",
        "🐅",
        "🐆",
        "🦓",
        "🦍",
        "🦧",
        "🐘",
        "🦛",
        "🦏",
        "🐪",
        "🐫",
        "🦒",
        "🦘",
        "🐃",
        "🐂",
        "🐄",
        "🐎",
        "🐖",
        "🐏",
        "🐑",
        "🦙",
        "🐐",
        "🦌",
        "🐕",
        "🐩",
        "🦮",
        "🐕‍🦺",
        "🐈",
        "🐈‍⬛",
        "🐓",
        "🦃",
        "🦚",
        "🦜",
        "🦢",
        "🦩",
        "🕊️",
        "🐇",
        "🦝",
        "🦨",
        "🦡",
        "🦦",
        "🦥",
        "🐁",
        "🐀",
        "🐿️",
        "🦔",
        "🌲",
        "🌳",
        "🌴",
        "🌱",
        "🌿",
        "☘️",
        "🍀",
        "🎋",
        "🎍",
        "🌾",
        "🌵",
        "🌶️",
        "🍄",
        "🌰",
        "🌼",
        "🌻",
        "🌺",
        "🌸",
        "🌷",
        "🌹",
        "🥀",
        "💐",
        "🌕",
        "🌖",
        "🌗",
        "🌘",
        "🌑",
        "🌒",
        "🌓",
        "🌔",
        "🌙",
        "🌛",
        "🌜",
        "🌚",
        "🌝",
        "🌞",
        "⭐",
        "🌟",
        "💫",
        "✨",
        "☄️",
        "☀️",
        "🌤️",
        "⛅",
        "🌥️",
        "☁️",
        "🌦️",
        "🌧️",
        "⛈️",
        "🌩️",
        "🌨️",
        "❄️",
        "☃️",
        "⛄",
        "🌬️",
        "💨",
        "💧",
        "💦",
        "☔",
        "☂️",
        "🌊",
        "🌍",
        "🌎",
        "🌏",
      ],
      food: [
        "🍎",
        "🍏",
        "🍐",
        "🍊",
        "🍋",
        "🍌",
        "🍉",
        "🍇",
        "🍓",
        "🫐",
        "🍈",
        "🍒",
        "🍑",
        "🥭",
        "🍍",
        "🥥",
        "🥝",
        "🍅",
        "🍆",
        "🥑",
        "🥦",
        "🥬",
        "🥒",
        "🌶️",
        "🫑",
        "🌽",
        "🥕",
        "🫒",
        "🧄",
        "🧅",
        "🥔",
        "🍠",
        "🥐",
        "🥖",
        "🍞",
        "🥨",
        "🥯",
        "🥞",
        "🧇",
        "🧀",
        "🍖",
        "🍗",
        "🥩",
        "🥓",
        "🍔",
        "🍟",
        "🍕",
        "🌭",
        "🥪",
        "🌮",
        "🌯",
        "🫔",
        "🥙",
        "🧆",
        "🥚",
        "🍳",
        "🥘",
        "🍲",
        "🫕",
        "🥣",
        "🥗",
        "🍿",
        "🧈",
        "🧂",
        "🥫",
        "🍱",
        "🍘",
        "🍙",
        "🍚",
        "🍛",
        "🍜",
        "🍝",
        "🍠",
        "🍢",
        "🍣",
        "🍤",
        "🍥",
        "🥮",
        "🍡",
        "🥟",
        "🥠",
        "🥡",
        "🦀",
        "🦞",
        "🦐",
        "🦑",
        "🦪",
        "🍦",
        "🍧",
        "🍨",
        "🍩",
        "🍪",
        "🎂",
        "🍰",
        "🧁",
        "🥧",
        "🍫",
        "🍬",
        "🍭",
        "🍮",
        "🍯",
        "🍼",
        "🥛",
        "☕",
        "🫖",
        "🍵",
        "🍶",
        "🍾",
        "🍷",
        "🍸",
        "🍹",
        "🍺",
        "🍻",
        "🥂",
        "🥃",
        "🥤",
        "🧋",
        "🧃",
        "🧉",
        "🧊",
      ],
      objects: [
        "⌚",
        "📱",
        "📲",
        "💻",
        "⌨️",
        "🖥️",
        "🖨️",
        "🖱️",
        "🖲️",
        "🕹️",
        "🗜️",
        "💽",
        "💾",
        "💿",
        "📀",
        "📼",
        "📷",
        "📸",
        "📹",
        "🎥",
        "📽️",
        "🎞️",
        "📞",
        "☎️",
        "📟",
        "📠",
        "📺",
        "📻",
        "🎙️",
        "🎚️",
        "🎛️",
        "🧭",
        "⏱️",
        "⏲️",
        "⏰",
        "🕰️",
        "⏳",
        "⌛",
        "📡",
        "🔋",
        "🔌",
        "💡",
        "🔦",
        "🕯️",
        "🪔",
        "🧯",
        "🛢️",
        "💸",
        "💵",
        "💴",
        "💶",
        "💷",
        "🪙",
        "💰",
        "💳",
        "💎",
        "⚖️",
        "🪜",
        "🧰",
        "🔧",
        "🔨",
        "⚒️",
        "🛠️",
        "⛏️",
        "🪓",
        "🪚",
        "🔩",
        "⚙️",
        "🪤",
        "🧱",
        "⛓️",
        "🧲",
        "🔫",
        "💣",
        "🧨",
        "🔪",
        "🗡️",
        "⚔️",
        "🛡️",
        "🚬",
        "⚰️",
        "🪦",
        "⚱️",
        "🏺",
        "🔮",
        "📿",
        "🧿",
        "💈",
        "⚗️",
        "🔭",
        "🔬",
        "🕳️",
        "🩹",
        "🩺",
        "💊",
        "💉",
        "🧬",
        "🦠",
        "🧫",
        "🧪",
        "🌡️",
        "🧹",
        "🧺",
        "🧻",
        "🚽",
        "🚰",
        "🚿",
        "🛁",
        "🧴",
        "🧷",
        "🧸",
        "🧵",
        "🪡",
        "🧶",
        "🪢",
        "👓",
        "🕶️",
        "🥽",
        "🥼",
        "🦺",
        "👔",
        "👕",
        "👖",
        "🧣",
        "🧤",
        "🧥",
        "🧦",
        "👗",
        "👘",
        "🥻",
        "🩱",
        "🩲",
        "🩳",
        "👙",
        "👚",
        "👛",
        "👜",
        "👝",
        "🛍️",
        "🎒",
        "🩴",
        "👞",
        "👟",
        "🥾",
        "🥿",
        "👠",
        "👡",
        "🩰",
        "👢",
        "👑",
        "👒",
        "🎩",
        "🎓",
        "🧢",
        "🪖",
        "⛑️",
        "💄",
        "💍",
      ],
      symbols: [
        "❤️",
        "🧡",
        "💛",
        "💚",
        "💙",
        "💜",
        "🖤",
        "🤍",
        "🤎",
        "💔",
        "❣️",
        "💕",
        "💞",
        "💓",
        "💗",
        "💖",
        "💘",
        "💝",
        "💟",
        "☮️",
        "✝️",
        "☪️",
        "🕉️",
        "☸️",
        "✡️",
        "🔯",
        "🕎",
        "☯️",
        "☦️",
        "🛐",
        "⛎",
        "♈",
        "♉",
        "♊",
        "♋",
        "♌",
        "♍",
        "♎",
        "♏",
        "♐",
        "♑",
        "♒",
        "♓",
        "🆔",
        "⚛️",
        "🉑",
        "☢️",
        "☣️",
        "📴",
        "📳",
        "🈶",
        "🈚",
        "🈸",
        "🈺",
        "🈷️",
        "✴️",
        "🆚",
        "💮",
        "🉐",
        "㊙️",
        "㊗️",
        "🈴",
        "🈵",
        "🈹",
        "🈲",
        "🅰️",
        "🅱️",
        "🆎",
        "🆑",
        "🅾️",
        "🆘",
        "❌",
        "⭕",
        "🛑",
        "⛔",
        "📛",
        "🚫",
        "💯",
        "💢",
        "♨️",
        "🚷",
        "🚯",
        "🚳",
        "🚱",
        "🔞",
        "📵",
        "🚭",
        "❗",
        "❕",
        "❓",
        "❔",
        "‼️",
        "⁉️",
        "🔅",
        "🔆",
        "〽️",
        "⚠️",
        "🚸",
        "🔱",
        "⚜️",
        "🔰",
        "♻️",
        "✅",
        "🈯",
        "💹",
        "❇️",
        "✳️",
        "❎",
        "🌐",
        "💠",
        "Ⓜ️",
        "🌀",
        "💤",
        "🏧",
        "🚾",
        "♿",
        "🅿️",
      ],
      activities: [
        "⚽",
        "🏀",
        "🏈",
        "⚾",
        "🥎",
        "🎾",
        "🏐",
        "🏉",
        "🥏",
        "🎱",
        "🪀",
        "🏓",
        "🏸",
        "🏒",
        "🏑",
        "🥍",
        "🏏",
        "🪃",
        "🥅",
        "⛳",
        "🪁",
        "🏹",
        "🎣",
        "🤿",
        "🥊",
        "🥋",
        "🎽",
        "🛹",
        "🛷",
        "⛸️",
        "🥌",
        "🎿",
        "⛷️",
        "🏂",
        "🪂",
        "🤺",
        "🏇",
        "🧘",
        "🏄",
        "🏊",
        "🤽",
        "🚣",
        "🧗",
        "🚵",
        "🚴",
        "🏆",
        "🥇",
        "🥈",
        "🥉",
        "🏅",
        "🎖️",
        "🏵️",
        "🎗️",
        "🎫",
        "🎟️",
        "🎪",
        "🤹",
        "🎭",
        "🩰",
        "🎨",
        "🎬",
        "🎤",
        "🎧",
        "🎼",
        "🎵",
        "🎶",
        "🎯",
        "🎳",
        "🎮",
        "🎰",
        "🧩",
      ],
    };

    return emojiCategories[category] || [];
  }

  switchEmojiCategory(category) {
    this.renderEmojiPicker(category);
  }

  createEmojiElement(emoji) {
    const emojiSpan = document.createElement("span");
    emojiSpan.className = "emoji";
    emojiSpan.textContent = emoji;
    emojiSpan.addEventListener("click", async () => {
      // Add to recent emojis
      await this.addRecentEmoji(emoji);

      // Paste the emoji using the regular paste method
      try {
        await window.clipboardAPI.pasteItem({
          type: "text",
          content: emoji,
        });
      } catch (error) {
        console.error("Failed to paste emoji:", error);
      }
    });
    return emojiSpan;
  }

  async addRecentEmoji(emoji) {
    try {
      this.recentEmojis = await window.clipboardAPI.addRecentEmoji(emoji);
      // If currently viewing recent emojis and on emoji page, refresh the view
      if (
        this.currentEmojiCategory === "recent" &&
        this.currentPage === "emojiPage"
      ) {
        this.renderEmojiPicker("recent");
      }
    } catch (error) {
      console.error("Failed to add recent emoji:", error);
    }
  }
  toggleDropdown(menuBtn) {
    const dropdown = menuBtn.nextElementSibling;
    const isOpen = dropdown.classList.contains("show");
    const clipboardItem = menuBtn.closest(".clipboard-item");

    // Close all dropdowns first
    this.closeAllDropdowns();

    // Toggle current dropdown
    if (!isOpen) {
      dropdown.classList.add("show");
      clipboardItem.classList.add("dropdown-open");

      // Auto-close after delay
      if (this.dropdownTimeout) {
        clearTimeout(this.dropdownTimeout);
      }
      this.dropdownTimeout = setTimeout(() => {
        this.closeAllDropdowns();
      }, 5000);
    }
  }

  closeAllDropdowns() {
    document.querySelectorAll(".dropdown-menu").forEach((dropdown) => {
      dropdown.classList.remove("show");
    });

    document.querySelectorAll(".clipboard-item").forEach((item) => {
      item.classList.remove("dropdown-open");
    });

    if (this.dropdownTimeout) {
      clearTimeout(this.dropdownTimeout);
      this.dropdownTimeout = null;
    }
  }

  setupHotkeyRecording() {
    const hotkeyInput = document.getElementById("hotkeyInput");
    const editBtn = document.getElementById("editHotkeyBtn");
    const settingsInfo = document.querySelector(".settings-info");

    this.isRecording = false;
    this.recordedKeys = [];

    // Load current hotkey on init
    this.loadCurrentHotkey();

    // Hotkey recording event listener
    this.hotkeyRecordingListener = (e) => {
      if (!this.isRecording) return;

      e.preventDefault();
      e.stopPropagation();

      const keys = [];
      if (e.ctrlKey) keys.push("Ctrl");
      if (e.altKey) keys.push("Alt");
      if (e.shiftKey) keys.push("Shift");
      if (e.metaKey) keys.push("Super");

      // Add the main key if it's not a modifier
      if (!["Control", "Alt", "Shift", "Meta"].includes(e.key)) {
        keys.push(e.key.toUpperCase());
      }

      if (keys.length > 0) {
        const hotkeyString = keys.join(" + ");
        hotkeyInput.value = hotkeyString;
        this.recordedKeys = keys;
      }
    };

    this.toggleHotkeyRecording = () => {
      if (!this.isRecording) {
        this.startHotkeyRecording();
      } else {
        this.stopHotkeyRecording();
      }
    };

    this.startHotkeyRecording = () => {
      this.isRecording = true;
      editBtn.textContent = "Stop";
      editBtn.style.background = "#e74c3c";
      hotkeyInput.value = "";
      hotkeyInput.placeholder = "Press key combination...";
      hotkeyInput.style.background = "#fff3cd";
      document.addEventListener("keydown", this.hotkeyRecordingListener, true);
    };

    this.stopHotkeyRecording = () => {
      this.isRecording = false;
      editBtn.textContent = "Edit";
      editBtn.style.background = "#5faee3";
      document.removeEventListener(
        "keydown",
        this.hotkeyRecordingListener,
        true
      );

      if (this.recordedKeys.length > 0) {
        this.saveHotkey(this.recordedKeys.join(" + "));
      } else {
        // Restore original value if no keys recorded
        this.loadCurrentHotkey();
      }

      hotkeyInput.style.background = "#eaf6fd";
      hotkeyInput.placeholder = "";
    };
  }

  async loadCurrentHotkey() {
    try {
      const currentHotkey = await window.clipboardAPI.getCurrentHotkey();
      const settingsInfo = document.querySelector(".settings-info b");
      if (settingsInfo) {
        settingsInfo.textContent = currentHotkey || "Super + V";
      }
      document.getElementById("hotkeyInput").value =
        currentHotkey || "Super + V";
    } catch (error) {
      console.error("Failed to load current hotkey:", error);
    }
  }

  async saveHotkey(hotkeyString) {
    try {
      await window.clipboardAPI.setHotkey(hotkeyString);
      const settingsInfo = document.querySelector(".settings-info b");
      if (settingsInfo) {
        settingsInfo.textContent = hotkeyString;
      }
      document.getElementById("hotkeyInput").value = hotkeyString;
      console.log("Hotkey saved:", hotkeyString);
    } catch (error) {
      console.error("Failed to save hotkey:", error);
      alert("Failed to save hotkey. Make sure it's a valid combination.");
    }
  }
}

// Initialize the app when DOM is loaded
document.addEventListener("DOMContentLoaded", () => {
  window.nativeClipboardApp = new NativeClipboardApp();

  // Set initial active button
  document.getElementById("clipboardBtn").classList.add("active");
});
