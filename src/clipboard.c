#include "clipboard.h"
#include "storage.h"
#include "ui.h"

static GtkClipboard *s_cb       = NULL;
static gulong        s_sig      = 0;
static gboolean      s_ignore   = FALSE;

/* Returns TRUE if clipboard contains file-manager-copied files */
static gboolean has_gnome_files(GtkClipboard *cb) {
    GdkAtom atom = gdk_atom_intern("x-special/gnome-copied-files", FALSE);
    GtkSelectionData *sel = gtk_clipboard_wait_for_contents(cb, atom);
    if (!sel) return FALSE;
    gtk_selection_data_free(sel);
    return TRUE;
}

/* Count how many file URIs are on the clipboard.
 * Returns 0 if there is no URI list at all. */
static int count_uris(GtkClipboard *cb) {
    GdkAtom atom = gdk_atom_intern("text/uri-list", FALSE);
    GtkSelectionData *sel = gtk_clipboard_wait_for_contents(cb, atom);
    if (!sel) return 0;
    gchar **uris = gtk_selection_data_get_uris(sel);
    gtk_selection_data_free(sel);
    if (!uris) return 0;
    int n = 0;
    while (uris[n]) n++;
    g_strfreev(uris);
    return n;
}

static void on_owner_change(GtkClipboard *cb, GdkEvent *ev, gpointer ud) {
    (void)ev; (void)ud;

    if (s_ignore) { s_ignore = FALSE; return; }

    /* Count file URIs on the clipboard */
    int n_uris = 0;
    if (has_gnome_files(cb) || count_uris(cb) > 0)
        n_uris = count_uris(cb);

    /* More than one file selected → skip entirely */
    if (n_uris > 1) return;

    /* Single file or no files: check for image data first */
    if (gtk_clipboard_wait_is_image_available(cb)) {
        GdkPixbuf *pb = gtk_clipboard_wait_for_image(cb);
        if (pb) {
            storage_add_image(pb);
            g_object_unref(pb);
            ui_refresh_list();
        }
        return;
    }

    /* No image — only store text if there are no file URIs */
    if (n_uris == 0 && gtk_clipboard_wait_is_text_available(cb)) {
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
