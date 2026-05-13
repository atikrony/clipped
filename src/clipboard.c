#include "clipboard.h"
#include "storage.h"
#include "ui.h"

static GtkClipboard *s_cb     = NULL;
static gulong        s_sig    = 0;
static gboolean      s_ignore = FALSE;

/* Returns TRUE if clipboard has the gnome file-manager atom */
static gboolean has_gnome_files(GtkClipboard *cb) {
    GdkAtom atom = gdk_atom_intern("x-special/gnome-copied-files", FALSE);
    GtkSelectionData *sel = gtk_clipboard_wait_for_contents(cb, atom);
    if (!sel) return FALSE;
    gtk_selection_data_free(sel);
    return TRUE;
}

/* Returns every URI on the clipboard, or NULL if none.
 * Caller must g_strfreev() the result. */
static gchar **get_uris(GtkClipboard *cb) {
    GdkAtom atom = gdk_atom_intern("text/uri-list", FALSE);
    GtkSelectionData *sel = gtk_clipboard_wait_for_contents(cb, atom);
    if (!sel) return NULL;
    gchar **uris = gtk_selection_data_get_uris(sel);
    gtk_selection_data_free(sel);
    return uris;   /* may be NULL */
}

/* If the clipboard holds exactly one local image file, load and return
 * a GdkPixbuf for it. Returns NULL if the URI is missing, is multiple
 * files, is not local, or is not a supported image format.
 * Caller owns the returned pixbuf. */
static GdkPixbuf *load_single_image_uri(GtkClipboard *cb) {
    gchar **uris = get_uris(cb);
    if (!uris) return NULL;

    /* Reject if more than one URI */
    if (uris[0] == NULL || uris[1] != NULL) { g_strfreev(uris); return NULL; }

    GError *err  = NULL;
    gchar  *path = g_filename_from_uri(uris[0], NULL, &err);
    g_strfreev(uris);
    if (!path) { if (err) g_error_free(err); return NULL; }

    /* Ask gdk-pixbuf whether it knows this format — cheap check, no full load */
    gint w, h;
    GdkPixbufFormat *fmt = gdk_pixbuf_get_file_info(path, &w, &h);
    if (!fmt) { g_free(path); return NULL; }   /* not a recognised image */

    GdkPixbuf *pb = gdk_pixbuf_new_from_file(path, NULL);
    g_free(path);
    return pb;   /* NULL on load error */
}

static void on_owner_change(GtkClipboard *cb, GdkEvent *ev, gpointer ud) {
    (void)ev; (void)ud;

    if (s_ignore) { s_ignore = FALSE; return; }

    /* ── Determine what is on the clipboard ───────────────────── */
    gboolean gnome = has_gnome_files(cb);
    gchar  **uris  = get_uris(cb);
    int      n     = 0;
    if (uris) { while (uris[n]) n++; }

    /* More than one file selected → ignore completely */
    if (n > 1) { g_strfreev(uris); return; }

    g_strfreev(uris);   /* done counting */

    /* ── Case 1: inline image data (screenshot, browser, viewer) ── */
    if (gtk_clipboard_wait_is_image_available(cb)) {
        GdkPixbuf *pb = gtk_clipboard_wait_for_image(cb);
        if (pb) {
            storage_add_image(pb);
            g_object_unref(pb);
            ui_refresh_list();
        }
        return;
    }

    /* ── Case 2: single file URI — try to load as image from disk ── */
    if (n == 1 || gnome) {
        GdkPixbuf *pb = load_single_image_uri(cb);
        if (pb) {
            storage_add_image(pb);
            g_object_unref(pb);
            ui_refresh_list();
        }
        /* Whether or not it was an image, we are done with file URIs */
        return;
    }

    /* ── Case 3: plain text ────────────────────────────────────── */
    if (gtk_clipboard_wait_is_text_available(cb)) {
        char *text = gtk_clipboard_wait_for_text(cb);
        if (text) {
            storage_add_text(text);
            g_free(text);
            ui_refresh_list();
        }
    }
}

void clipboard_init(void) {
    s_cb  = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    s_sig = g_signal_connect(s_cb, "owner-change",
                             G_CALLBACK(on_owner_change), NULL);
}

void clipboard_paste_item(ClipItem *item) {
    if (!item || !s_cb) return;
    s_ignore = TRUE;
    if (item->type == CLIP_TEXT  && item->text)
        gtk_clipboard_set_text(s_cb, item->text, -1);
    else if (item->type == CLIP_IMAGE && item->image)
        gtk_clipboard_set_image(s_cb, item->image);
}

void clipboard_cleanup(void) {
    if (s_cb && s_sig) {
        g_signal_handler_disconnect(s_cb, s_sig);
        s_sig = 0;
    }
}
