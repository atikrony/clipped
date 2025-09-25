// Button functionality for clipboard UI

document.getElementById("noteBtn").onclick = function () {
  showClipboardPage("clipboardPage");
};
document.getElementById("fingerBtn").onclick = function () {
  showClipboardPage("emojiPage");
};
document.getElementById("settingsBtn").onclick = function () {
  showClipboardPage("settingsPage");
};

document.getElementById("deleteBtn").onclick = function () {
  const container = document.querySelector(".container");
  container.classList.add("fade-out");
  setTimeout(() => {
    container.style.display = "none";
  }, 400);
};

// Show the requested clipboard page and hide others
function showClipboardPage(pageId) {
  const pages = document.querySelectorAll(".clipboard-page");
  pages.forEach((page) => {
    if (page.id === pageId) {
      // Settings page needs flex display, others use block
      page.style.display = pageId === "settingsPage" ? "flex" : "block";
    } else {
      page.style.display = "none";
    }
  });
  // Always show the clipboard area if hidden
  const container = document.querySelector(".container");
  container.style.display = "";
  container.classList.remove("fade-out");
}

// Hotkey editing logic
const hotkeyInput = document.getElementById("hotkeyInput");
const editHotkeyBtn = document.getElementById("editHotkeyBtn");
if (editHotkeyBtn) {
  let editing = false;
  editHotkeyBtn.onclick = function () {
    if (!editing) {
      hotkeyInput.removeAttribute("readonly");
      hotkeyInput.focus();
      editHotkeyBtn.textContent = "Save";
      editing = true;
    } else {
      hotkeyInput.setAttribute("readonly", "readonly");
      editHotkeyBtn.textContent = "Edit";
      editing = false;
    }
  };
}

// Clear clipboard functionality
const clearClipboardBtn = document.getElementById("clearClipboardBtn");
if (clearClipboardBtn) {
  clearClipboardBtn.onclick = function () {
    if (confirm("Are you sure you want to clear all clipboard items?")) {
      const clipboardItems = document.querySelectorAll(".clipboard-item");
      clipboardItems.forEach((item, index) => {
        setTimeout(() => {
          item.style.animation = "fadeOut 0.3s forwards";
          setTimeout(() => {
            item.remove();
          }, 300);
        }, index * 50); // Stagger the animations
      });
    }
  };
}

// Add event listeners for menu and dropdown functionality
document.addEventListener("click", function (e) {
  // Handle menu button clicks
  if (e.target.classList.contains("menu-btn")) {
    e.stopPropagation(); // Prevent clipboard item click
    toggleDropdown(e.target);
  }

  // Handle dropdown option clicks
  if (e.target.classList.contains("pin-option")) {
    e.stopPropagation();
    togglePin(e.target);
    closeAllDropdowns();
  }

  if (e.target.classList.contains("delete-option")) {
    e.stopPropagation();
    deleteClipboardItem(e.target);
    closeAllDropdowns();
  }

  // Close dropdowns when clicking outside
  if (!e.target.closest(".clipboard-actions")) {
    closeAllDropdowns();
  }
});

// Toggle dropdown menu visibility
function toggleDropdown(menuBtn) {
  const dropdown = menuBtn.nextElementSibling;
  const isOpen = dropdown.classList.contains("show");

  // Close all other dropdowns first
  closeAllDropdowns();

  // Toggle current dropdown
  if (!isOpen) {
    dropdown.classList.add("show");
  }
}

// Close all dropdown menus
function closeAllDropdowns() {
  const dropdowns = document.querySelectorAll(".dropdown-menu");
  dropdowns.forEach((dropdown) => {
    dropdown.classList.remove("show");
  });
}

// Toggle pin state of clipboard item
function togglePin(pinOption) {
  const clipboardItem = pinOption.closest(".clipboard-item");
  const isCurrentlyPinned = clipboardItem.classList.contains("pinned");

  if (isCurrentlyPinned) {
    clipboardItem.classList.remove("pinned");
    pinOption.innerHTML = "📌 Pin";
  } else {
    clipboardItem.classList.add("pinned");
    pinOption.innerHTML = "📌 Unpin";

    // Move pinned items to the top
    const clipboardList = clipboardItem.parentNode;
    clipboardList.insertBefore(clipboardItem, clipboardList.firstChild);
  }
}

// Delete clipboard item with animation
function deleteClipboardItem(deleteOption) {
  const clipboardItem = deleteOption.closest(".clipboard-item");
  clipboardItem.style.animation = "fadeOut 0.3s forwards";

  setTimeout(() => {
    clipboardItem.remove();
  }, 300);
}
