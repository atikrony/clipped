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

/* Returns TRUE if clipboard has any URI list (files/folders) */
static gboolean has_uri_list(GtkClipboard *cb) {
    GdkAtom *targets; gint n = 0;
    gboolean found = FALSE;
    if (gtk_clipboard_wait_for_targets(cb, &targets, &n)) {
        for (int i = 0; i < n; i++) {
            char *name = gdk_atom_name(targets[i]);
            if (name && strcmp(name, "text/uri-list") == 0) found = TRUE;
            g_free(name);
            if (found) break;
        }
        g_free(targets);
    }
    return found;
}

static void on_owner_change(GtkClipboard *cb, GdkEvent *ev, gpointer ud) {
    (void)ev; (void)ud;

    if (s_ignore) { s_ignore = FALSE; return; }

    /* Skip anything from file manager (files / folders) */
    if (has_gnome_files(cb)) return;
    if (has_uri_list(cb))    return;

    /* Single image from image viewer / screenshot tool */
    if (gtk_clipboard_wait_is_image_available(cb)) {
        GdkPixbuf *pb = gtk_clipboard_wait_for_image(cb);
        if (pb) {
            storage_add_image(pb);
            g_object_unref(pb);
            ui_refresh_list();
        }
        return;
    }

    /* Plain text */
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
