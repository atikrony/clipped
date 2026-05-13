/* GtkStatusIcon is deprecated in GTK3 but fully functional on Cinnamon,
 * Windows, XFCE, MATE, and LXQt.  GNOME users need the "AppIndicator"
 * shell extension.  Suppress the deprecation warning.
 * macOS does not support GtkStatusIcon — tray is skipped there. */
#ifdef __APPLE__
/* ── macOS stub ─ no tray support via GTK ─────────────────────── */
#include "tray.h"
void tray_init    (void) { /* tray not available on macOS via GTK */ }
void tray_cleanup (void) {}
#else   /* Linux + Windows */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include "tray.h"
#include "ui.h"
#include "hotkey.h"
#include <math.h>
#include <unistd.h>
#include <libgen.h>

static GtkStatusIcon *s_icon = NULL;

/* ── Fallback: draw a white sitting cat with Cairo (no file needed) ── */
static GdkPixbuf *make_cat_icon(int size) {
    cairo_surface_t *surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size, size);
    cairo_t *cr = cairo_create(surf);
    cairo_scale(cr, size / 64.0, size / 64.0);
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);

    cairo_set_source_rgba(cr, 1, 1, 1, 1);
    cairo_set_line_width(cr, 5.5);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_move_to(cr, 50, 62);
    cairo_curve_to(cr, 62, 54, 64, 38, 54, 26);
    cairo_stroke(cr);

    cairo_move_to(cr, 10, 26); cairo_line_to(cr, 16, 6);  cairo_line_to(cr, 26, 22); cairo_close_path(cr); cairo_fill(cr);
    cairo_move_to(cr, 54, 26); cairo_line_to(cr, 48, 6);  cairo_line_to(cr, 38, 22); cairo_close_path(cr); cairo_fill(cr);

    cairo_save(cr); cairo_translate(cr, 32, 50); cairo_scale(cr, 17, 13);
    cairo_arc(cr, 0, 0, 1, 0, 2*G_PI); cairo_restore(cr); cairo_fill(cr);

    cairo_arc(cr, 32, 26, 15, 0, 2*G_PI); cairo_fill(cr);

    cairo_set_source_rgba(cr, 1.0, 0.68, 0.68, 1.0);
    cairo_move_to(cr, 13, 24); cairo_line_to(cr, 17, 10); cairo_line_to(cr, 24, 22); cairo_close_path(cr); cairo_fill(cr);
    cairo_move_to(cr, 51, 24); cairo_line_to(cr, 47, 10); cairo_line_to(cr, 40, 22); cairo_close_path(cr); cairo_fill(cr);

    cairo_set_source_rgba(cr, 1, 1, 1, 1);
    cairo_save(cr); cairo_translate(cr, 22, 62); cairo_scale(cr, 7, 3); cairo_arc(cr, 0, 0, 1, 0, 2*G_PI); cairo_restore(cr); cairo_fill(cr);
    cairo_save(cr); cairo_translate(cr, 42, 62); cairo_scale(cr, 7, 3); cairo_arc(cr, 0, 0, 1, 0, 2*G_PI); cairo_restore(cr); cairo_fill(cr);

    cairo_set_source_rgba(cr, 0.10, 0.15, 0.25, 1.0);
    cairo_arc(cr, 26, 25, 3.0, 0, 2*G_PI); cairo_fill(cr);
    cairo_arc(cr, 38, 25, 3.0, 0, 2*G_PI); cairo_fill(cr);
    cairo_set_source_rgba(cr, 1, 1, 1, 1);
    cairo_arc(cr, 27.2, 23.8, 1.2, 0, 2*G_PI); cairo_fill(cr);
    cairo_arc(cr, 39.2, 23.8, 1.2, 0, 2*G_PI); cairo_fill(cr);

    cairo_set_source_rgba(cr, 1.0, 0.55, 0.55, 1.0);
    cairo_move_to(cr, 32, 31); cairo_line_to(cr, 30, 33); cairo_line_to(cr, 34, 33);
    cairo_close_path(cr); cairo_fill(cr);

    GdkPixbuf *pb = gdk_pixbuf_get_from_surface(surf, 0, 0, size, size);
    cairo_destroy(cr);
    cairo_surface_destroy(surf);
    return pb;
}

/* ── Load the cat PNG; fall back to Cairo drawing if not found ── */
static GdkPixbuf *load_icon(int size) {
    /* Resolve the directory the running binary lives in. */
    char exe[512] = {0};
    char rel[560] = {0};
    if (readlink("/proc/self/exe", exe, sizeof(exe) - 1) > 0) {
        char tmp[512];
        g_strlcpy(tmp, exe, sizeof(tmp));
        snprintf(rel, sizeof(rel), "%s/assets/clipman-cat.png", dirname(tmp));
    }

    const char *candidates[] = {
        "/usr/share/pixmaps/clipman-cat.png",
        "/usr/local/share/pixmaps/clipman-cat.png",
        rel,
        NULL
    };

    for (int i = 0; candidates[i] && candidates[i][0]; i++) {
        GError    *err = NULL;
        GdkPixbuf *pb  = gdk_pixbuf_new_from_file_at_scale(
                            candidates[i], size, size, TRUE, &err);
        if (pb)  return pb;
        if (err) g_error_free(err);
    }

    return make_cat_icon(size);   /* Cairo fallback */
}

/* ── Tray callbacks ─────────────────────────────────────────────── */
static void on_activate(GtkStatusIcon *icon, gpointer ud) {
    (void)icon; (void)ud;
    g_last_hotkey_time = gtk_get_current_event_time();
    ui_toggle_window();
}

static void on_popup_menu(GtkStatusIcon *icon, guint button,
                          guint activate_time, gpointer ud) {
    (void)icon; (void)ud;
    GtkWidget *menu     = gtk_menu_new();
    GtkWidget *mi_open  = gtk_menu_item_new_with_label("Open Clipman");
    GtkWidget *sep      = gtk_separator_menu_item_new();
    GtkWidget *mi_quit  = gtk_menu_item_new_with_label("Quit");

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi_open);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), sep);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi_quit);

    g_signal_connect_swapped(mi_open, "activate", G_CALLBACK(ui_show_window), NULL);
    g_signal_connect        (mi_quit, "activate", G_CALLBACK(gtk_main_quit),  NULL);

    gtk_widget_show_all(menu);
    gtk_menu_popup(GTK_MENU(menu), NULL, NULL,
                   gtk_status_icon_position_menu, icon,
                   button, activate_time);
}

/* ── Public API ─────────────────────────────────────────────────── */
void tray_init(void) {
    GdkPixbuf *pb = load_icon(64);
    s_icon = gtk_status_icon_new_from_pixbuf(pb);
    g_object_unref(pb);
    gtk_status_icon_set_tooltip_text(s_icon, "Clipman — click to open clipboard");
    gtk_status_icon_set_visible(s_icon, TRUE);
    g_signal_connect(s_icon, "activate",   G_CALLBACK(on_activate),   NULL);
    g_signal_connect(s_icon, "popup-menu", G_CALLBACK(on_popup_menu), NULL);
}

void tray_cleanup(void) {
    if (s_icon) { g_object_unref(s_icon); s_icon = NULL; }
}

#pragma GCC diagnostic pop
#endif  /* !__APPLE__ */
