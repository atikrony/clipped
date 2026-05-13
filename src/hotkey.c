#include "hotkey.h"
#include "ui.h"

static char   *s_hotkey         = NULL;
guint32        g_last_hotkey_time = 0;

const char *hotkey_get(void) {
    return s_hotkey ? s_hotkey : "super+v";
}

/* ═══════════════════════════════════════════════════════════════
 * WINDOWS — RegisterHotKey via a hidden message-only window
 * ═══════════════════════════════════════════════════════════════ */
#if defined(_WIN32)

static HWND   s_hwnd    = NULL;
static HANDLE s_thread  = NULL;
static UINT   s_vk      = 0;
static UINT   s_mods    = 0;

static UINT parse_mods_win(const char *s) {
    UINT m = MOD_NOREPEAT;
    if (strstr(s, "ctrl")  || strstr(s, "control")) m |= MOD_CONTROL;
    if (strstr(s, "shift"))                          m |= MOD_SHIFT;
    if (strstr(s, "alt"))                            m |= MOD_ALT;
    if (strstr(s, "super") || strstr(s, "win"))     m |= MOD_WIN;
    return m;
}

static UINT parse_vk(const char *s) {
    const char *k = strrchr(s, '+');
    k = k ? k + 1 : s;
    if (g_utf8_strlen(k, -1) == 1) return VkKeyScanA(g_ascii_tolower(k[0])) & 0xFF;
    if (g_ascii_strcasecmp(k, "space")  == 0) return VK_SPACE;
    if (g_ascii_strcasecmp(k, "return") == 0) return VK_RETURN;
    if (g_ascii_strcasecmp(k, "tab")    == 0) return VK_TAB;
    return 0;
}

static gboolean idle_toggle(gpointer ud) {
    (void)ud;
    g_last_hotkey_time = (guint32)GetTickCount();
    ui_toggle_window();
    return FALSE;
}

static LRESULT CALLBACK hk_wnd_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_HOTKEY) g_idle_add(idle_toggle, NULL);
    return DefWindowProcA(hwnd, msg, wp, lp);
}

static DWORD WINAPI hk_thread(LPVOID arg) {
    (void)arg;
    WNDCLASSA wc = {0};
    wc.lpfnWndProc   = hk_wnd_proc;
    wc.hInstance     = GetModuleHandleA(NULL);
    wc.lpszClassName = "ClippedHK";
    RegisterClassA(&wc);
    s_hwnd = CreateWindowExA(0, "ClippedHK", NULL, 0,
                              0, 0, 0, 0, HWND_MESSAGE, NULL, wc.hInstance, NULL);
    if (s_hwnd && s_vk)
        RegisterHotKey(s_hwnd, 1, s_mods, s_vk);
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) DispatchMessageA(&msg);
    return 0;
}

void hotkey_init(void) { /* thread starts on first hotkey_set() call */ }

void hotkey_set(const char *keystr) {
    g_free(s_hotkey);
    s_hotkey = g_ascii_strdown(keystr, -1);
    s_mods   = parse_mods_win(s_hotkey);
    s_vk     = parse_vk(s_hotkey);
    if (s_hwnd) {
        UnregisterHotKey(s_hwnd, 1);
        if (s_vk) RegisterHotKey(s_hwnd, 1, s_mods, s_vk);
    } else {
        s_thread = CreateThread(NULL, 0, hk_thread, NULL, 0, NULL);
    }
}

void hotkey_cleanup(void) {
    if (s_hwnd) {
        UnregisterHotKey(s_hwnd, 1);
        PostMessageA(s_hwnd, WM_QUIT, 0, 0);
        if (s_thread) { WaitForSingleObject(s_thread, 1000); CloseHandle(s_thread); }
    }
    g_free(s_hotkey); s_hotkey = NULL;
}

/* ═══════════════════════════════════════════════════════════════
 * macOS — Carbon RegisterEventHotKey
 * ═══════════════════════════════════════════════════════════════ */
#elif defined(__APPLE__)

static EventHotKeyRef  s_hk_ref  = NULL;
static EventHandlerRef s_handler = NULL;

static UInt32 parse_mods_mac(const char *s) {
    UInt32 m = 0;
    if (strstr(s, "ctrl")  || strstr(s, "control")) m |= controlKey;
    if (strstr(s, "shift"))                          m |= shiftKey;
    if (strstr(s, "alt"))                            m |= optionKey;
    if (strstr(s, "super") || strstr(s, "cmd"))     m |= cmdKey;
    if (!m) m = cmdKey;   /* default: Cmd modifier */
    return m;
}

static UInt32 parse_keycode_mac(const char *s) {
    const char *k = strrchr(s, '+');
    k = k ? k + 1 : s;
    /* macOS ANSI virtual keycodes for the alphabet */
    static const char keys[] = "asdfhgzxcvbqwertyuiop"  /* 0-20 */
                                "\0\0\0\0\0"               /* 21-25 gaps */
                                "jkl\0;'\0,/nm.\0\0\0";   /* rough */
    if (g_utf8_strlen(k, -1) == 1) {
        char c = g_ascii_tolower(k[0]);
        /* Simplified but correct map for common keys */
        switch (c) {
            case 'a': return 0;  case 's': return 1;  case 'd': return 2;
            case 'f': return 3;  case 'h': return 4;  case 'g': return 5;
            case 'z': return 6;  case 'x': return 7;  case 'c': return 8;
            case 'v': return 9;  case 'b': return 11; case 'q': return 12;
            case 'w': return 13; case 'e': return 14; case 'r': return 15;
            case 'y': return 16; case 't': return 17; case 'o': return 31;
            case 'u': return 32; case 'i': return 34; case 'p': return 35;
            case 'l': return 37; case 'j': return 38; case 'k': return 40;
            case 'n': return 45; case 'm': return 46;
            default:  return (UInt32)(c & 0xFF);
        }
    }
    if (g_ascii_strcasecmp(k, "space")  == 0) return 49;
    if (g_ascii_strcasecmp(k, "return") == 0) return 36;
    if (g_ascii_strcasecmp(k, "tab")    == 0) return 48;
    return 9; /* fallback: V */
}

static gboolean idle_toggle(gpointer ud) {
    (void)ud;
    g_last_hotkey_time = (guint32)(CFAbsoluteTimeGetCurrent() * 1000.0);
    ui_toggle_window();
    return FALSE;
}

static OSStatus hk_event_handler(EventHandlerCallRef next, EventRef ev, void *ud) {
    (void)next; (void)ud;
    g_idle_add(idle_toggle, NULL);
    return noErr;
}

void hotkey_init(void) {
    EventTypeSpec type = {kEventClassKeyboard, kEventHotKeyPressed};
    InstallApplicationEventHandler(hk_event_handler, 1, &type, NULL, &s_handler);
}

void hotkey_set(const char *keystr) {
    g_free(s_hotkey);
    s_hotkey = g_ascii_strdown(keystr, -1);
    if (s_hk_ref) { UnregisterEventHotKey(s_hk_ref); s_hk_ref = NULL; }
    EventHotKeyID hkid = {'clpd', 1};
    RegisterEventHotKey(parse_keycode_mac(s_hotkey), parse_mods_mac(s_hotkey),
                        hkid, GetApplicationEventTarget(), 0, &s_hk_ref);
}

void hotkey_cleanup(void) {
    if (s_hk_ref)  { UnregisterEventHotKey(s_hk_ref); s_hk_ref = NULL; }
    if (s_handler) { RemoveEventHandler(s_handler);    s_handler = NULL; }
    g_free(s_hotkey); s_hotkey = NULL;
}

/* ═══════════════════════════════════════════════════════════════
 * Linux / X11 — XGrabKey
 * ═══════════════════════════════════════════════════════════════ */
#else

static Display   *s_dpy             = NULL;
static Window     s_root            = 0;
static guint      s_keycode         = 0;
static guint      s_mods            = 0;
static GIOChannel *s_chan           = NULL;
static guint      s_watch_id        = 0;
static guint32    s_last_toggle_time = 0;

static guint parse_mods(const char *s) {
    guint m = 0;
    if (strstr(s, "super") || strstr(s, "win"))     m |= Mod4Mask;
    if (strstr(s, "ctrl")  || strstr(s, "control")) m |= ControlMask;
    if (strstr(s, "shift"))                          m |= ShiftMask;
    if (strstr(s, "alt"))                            m |= Mod1Mask;
    return m;
}

static guint parse_keycode(const char *s, Display *dpy) {
    const char *k = strrchr(s, '+');
    k = k ? k + 1 : s;
    KeySym sym = XStringToKeysym(k);
    if (sym == NoSymbol) {
        char *up = g_ascii_strup(k, -1);
        sym = XStringToKeysym(up);
        g_free(up);
    }
    if (sym == NoSymbol) return 0;
    return XKeysymToKeycode(dpy, sym);
}

static void ungrab(void) {
    if (!s_dpy || !s_root || !s_keycode) return;
    static const guint extra[] = {0, LockMask, Mod2Mask, LockMask|Mod2Mask};
    for (int i = 0; i < 4; i++)
        XUngrabKey(s_dpy, s_keycode, s_mods | extra[i], s_root);
}

static void grab(void) {
    if (!s_dpy || !s_root || !s_keycode) return;
    static const guint extra[] = {0, LockMask, Mod2Mask, LockMask|Mod2Mask};
    for (int i = 0; i < 4; i++)
        XGrabKey(s_dpy, s_keycode, s_mods | extra[i], s_root,
                 True, GrabModeAsync, GrabModeAsync);
    XFlush(s_dpy);
}

static gboolean x_dispatch(GIOChannel *src, GIOCondition cond, gpointer ud) {
    (void)src; (void)cond; (void)ud;
    while (XPending(s_dpy)) {
        XEvent ev;
        XNextEvent(s_dpy, &ev);
        if (ev.type == KeyPress) {
            if ((ev.xkey.time - s_last_toggle_time) > 300u) {
                s_last_toggle_time = ev.xkey.time;
                g_last_hotkey_time = ev.xkey.time;
                ui_toggle_window();
            }
        }
    }
    return TRUE;
}

void hotkey_init(void) {
    s_dpy = XOpenDisplay(NULL);
    if (!s_dpy) { g_warning("hotkey: cannot open X display"); return; }
    s_root     = DefaultRootWindow(s_dpy);
    s_chan     = g_io_channel_unix_new(ConnectionNumber(s_dpy));
    s_watch_id = g_io_add_watch(s_chan, G_IO_IN, x_dispatch, NULL);
}

void hotkey_set(const char *keystr) {
    ungrab();
    g_free(s_hotkey);
    s_hotkey  = g_ascii_strdown(keystr, -1);
    s_mods    = parse_mods(s_hotkey);
    s_keycode = parse_keycode(s_hotkey, s_dpy);
    grab();
}

void hotkey_cleanup(void) {
    if (s_watch_id) { g_source_remove(s_watch_id); s_watch_id = 0; }
    if (s_chan)     { g_io_channel_unref(s_chan);   s_chan      = NULL; }
    ungrab();
    if (s_dpy) { XCloseDisplay(s_dpy); s_dpy = NULL; }
    g_free(s_hotkey); s_hotkey = NULL;
}

#endif  /* platform switch */
