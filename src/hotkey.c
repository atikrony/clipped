#include "hotkey.h"
#include "ui.h"

static Display  *s_dpy              = NULL;
static Window    s_root             = 0;
static guint     s_keycode          = 0;
static guint     s_mods             = 0;
static char     *s_hotkey           = NULL;
static GIOChannel *s_chan           = NULL;
static guint     s_watch_id         = 0;
static guint32   s_last_toggle_time = 0;

/* Exposed so ui_show_window can pass it to gtk_window_present_with_time. */
guint32 g_last_hotkey_time = GDK_CURRENT_TIME;

static guint parse_mods(const char *s) {
    guint m = 0;
    if (strstr(s, "super") || strstr(s, "win"))    m |= Mod4Mask;
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
        /* Try capitalised */
        char *up = g_ascii_strup(k, -1);
        sym = XStringToKeysym(up);
        g_free(up);
    }
    if (sym == NoSymbol) return 0;
    return XKeysymToKeycode(dpy, sym);
}

static void ungrab(void) {
    if (!s_dpy || !s_root || !s_keycode) return;
    static const guint extra[] = { 0, LockMask, Mod2Mask, LockMask|Mod2Mask };
    for (int i = 0; i < 4; i++)
        XUngrabKey(s_dpy, s_keycode, s_mods | extra[i], s_root);
}

static void grab(void) {
    if (!s_dpy || !s_root || !s_keycode) return;
    static const guint extra[] = { 0, LockMask, Mod2Mask, LockMask|Mod2Mask };
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
            /* Debounce: X11 auto-repeat fires multiple KeyPress events for a
             * single physical press. Drop any repeat within 300 ms. */
            if ((ev.xkey.time - s_last_toggle_time) > 300u) {
                s_last_toggle_time  = ev.xkey.time;
                g_last_hotkey_time  = ev.xkey.time;   /* pass time to presenter */
                ui_toggle_window();
            }
        }
    }
    return TRUE;
}

void hotkey_set(const char *keystr) {
    ungrab();
    g_free(s_hotkey);
    s_hotkey  = g_ascii_strdown(keystr, -1);
    s_mods    = parse_mods(s_hotkey);
    s_keycode = parse_keycode(s_hotkey, s_dpy);
    grab();
}

const char *hotkey_get(void) {
    return s_hotkey ? s_hotkey : "super+v";
}

void hotkey_init(void) {
    s_dpy = XOpenDisplay(NULL);
    if (!s_dpy) { g_warning("hotkey: cannot open X display"); return; }
    s_root = DefaultRootWindow(s_dpy);

    s_chan     = g_io_channel_unix_new(ConnectionNumber(s_dpy));
    s_watch_id = g_io_add_watch(s_chan, G_IO_IN, x_dispatch, NULL);
}

void hotkey_cleanup(void) {
    if (s_watch_id) { g_source_remove(s_watch_id); s_watch_id = 0; }
    if (s_chan)     { g_io_channel_unref(s_chan);   s_chan      = NULL; }
    ungrab();
    if (s_dpy) { XCloseDisplay(s_dpy); s_dpy = NULL; }
    g_free(s_hotkey); s_hotkey = NULL;
}
