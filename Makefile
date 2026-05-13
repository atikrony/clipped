CC      = gcc
PKGS    = gtk+-3.0 x11 xtst
export PKG_CONFIG_PATH ?= /usr/lib/x86_64-linux-gnu/pkgconfig
CFLAGS  = -O2 -Wall -Wextra -std=gnu11 -Wno-deprecated-declarations \
          $(shell pkg-config --cflags $(PKGS))
LDFLAGS = $(shell pkg-config --libs $(PKGS)) -lm

PREFIX  = /usr
BINDIR  = $(DESTDIR)$(PREFIX)/bin
APPDIR  = $(DESTDIR)$(PREFIX)/share/applications
AUTDIR  = $(DESTDIR)/etc/xdg/autostart
PIXDIR  = $(DESTDIR)$(PREFIX)/share/pixmaps

TARGET  = clipman
SRCDIR  = src
SRCS    = $(wildcard $(SRCDIR)/*.c)
OBJS    = $(SRCS:.c=.o)

# ── Packages needed to compile ────────────────────────────────────────────
BUILD_DEPS = gcc make pkg-config libgtk-3-dev libx11-dev libxtst-dev

# ── Packages needed to run the installed binary ───────────────────────────
RUNTIME_DEPS = libgtk-3-0 libx11-6 libglib2.0-0

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(SRCDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

# ── Install build + runtime dependencies via apt ─────────────────────────
deps:
	@echo "Installing build and runtime dependencies..."
	sudo apt-get update -qq
	sudo apt-get install -y $(BUILD_DEPS) $(RUNTIME_DEPS)
	@echo "All dependencies installed."

# ── Build + install in one step (run 'make deps' first if needed) ─────────
install: $(TARGET)
	install -Dm755 $(TARGET)                        $(BINDIR)/$(TARGET)
	install -Dm644 clipman.desktop                  $(APPDIR)/$(TARGET).desktop
	install -Dm644 clipman.autostart.desktop        $(AUTDIR)/$(TARGET).desktop
	install -Dm644 assets/clipman-cat.png           $(PIXDIR)/clipman-cat.png

uninstall:
	rm -f $(BINDIR)/$(TARGET)
	rm -f $(APPDIR)/$(TARGET).desktop
	rm -f $(AUTDIR)/$(TARGET).desktop

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all deps install uninstall clean
