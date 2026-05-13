#include "ui.h"
#include "storage.h"
#include "clipboard.h"
#include "hotkey.h"
#include "settings.h"
#include "emoji_data.h"
#include <math.h>
#include <unistd.h>
#include <libgen.h>

/* ── Globals ─────────────────────────────────────────────────── */
GtkWidget *g_main_window = NULL;

/* ── Remote message URL ──────────────────────────────────────────
 * Put a plain-text file at this URL (e.g. a raw GitHub Gist).
 * Edit the file online and every user sees the new message the
 * next time they open Settings.                                  */
#define MESSAGE_URL "https://raw.githubusercontent.com/youruser/clipman/main/message.txt"

/* ── Page indices ────────────────────────────────────────────── */
typedef enum { PAGE_HISTORY=0, PAGE_ICONS, PAGE_SYMBOLS, PAGE_SETTINGS } Page;
static const char *PAGE_NAMES[] = {"history","icons","symbols","settings"};

/* ── State ───────────────────────────────────────────────────── */
static GtkWidget  *s_stack           = NULL;
static GtkWidget  *s_list_box        = NULL;
static GtkWidget  *s_icon_grid       = NULL;
static GtkWidget  *s_sym_grid        = NULL;
static GtkWidget  *s_search[4];               /* per-page search bar  */
static char       *s_search_query[4];         /* per-page query text  */
static GtkWidget  *s_nav_btns[4];
static int         s_current_page    = PAGE_HISTORY;
static gboolean    s_shown           = FALSE;
static gboolean    s_pinned          = FALSE;
static GtkWidget  *s_msg_label       = NULL;  /* remote message label */
static gboolean    s_msg_fetched     = FALSE;

/* Previous focused X11 window — for auto-paste */
static Window      s_prev_focus      = None;
static int         s_prev_revert     = RevertToNone;
static gboolean    s_do_paste        = FALSE;

/* ═══════════════════════════════════════════════════════════════
 * DESIGN / THEME  — edit values here to restyle the panel
 * Padding shorthand:  top right bottom left
 * ═══════════════════════════════════════════════════════════════ */
static const char *CSS =

    "window.clipman { background: transparent; }"

    /* ── Panel background ──────────────────────────────────────── */
    ".clip-bg {"
    "  background-color: #4B70A0;"  /* COLOUR: panel background */
    "  border-radius: 14px;"        /* SHAPE:  panel corner radius */
    "}"

    /* ── Header ────────────────────────────────────────────────── */
    ".clip-header {"
    "  padding: 0px;"  /* spacing is controlled per-widget with margins below */
    "}"

    /* ── Search box ────────────────────────────────────────────── */
    ".clip-search {"
    "  background-color: rgba(255,255,255,0.15);" /* COLOUR: search fill */
    "  border: none;"
    "  border-radius: 8px;"     /* SHAPE:  search roundness */
    "  color: white;"
    "  padding: 7px 14px;"       /* SPACING: inside search */
    "  margin: 0 10px 6px 10px;" /* SPACING: outside search */
    "  font-size: 13px;"
    "}"
    ".clip-search:focus {"
    "  background-color: rgba(255,255,255,0.22);"
    "  outline: none; box-shadow: none;"
    "}"

    /* ── Clipboard item row ────────────────────────────────────── */
    /* clip-item-row is on the GtkListBoxRow so :hover works correctly */
    ".clip-item-row {"
    "  background-color: #5C80AC;"           /* COLOUR: item background */
    "  border-radius: 6px;"                  /* SHAPE:  item corner radius */
    "  margin: 3px 8px;"                     /* SPACING: gap between items */
    "  padding: 0;"
    "  min-height: 42px;"                    /* SIZE:   row height */
    "  border: 2px solid transparent;"       /* keeps layout stable on hover */
    "}"
    ".clip-item-row:hover {"
    "  background-color: #40546d;"
    // "  margin: 2px 0px 2px 0px;"                     /* SPACING: gap between items */


    "}"

    ".clip-item-btn {"
    "  background: transparent; border: none; box-shadow: none;"
    "  padding: 10px 12px;"                  /* SPACING: text padding in item */
    "  color: white;"
    "  font-size: 13px;"
    "}"
    ".clip-item-btn:hover, .clip-item-btn:focus {"
    "  background: transparent; box-shadow: none;"
    "}"

    ".clip-more-btn {"
    "  background: transparent; border: none; box-shadow: none;"
    "  color: rgba(255,255,255,0.65);"
    "  padding: 4px 10px;"         /* SPACING: ⋮ button hit area */
    "  font-size: 18px; min-width: 0;"
    "}"
    ".clip-more-btn:hover {"
    "  color: white; background: rgba(255,255,255,0.15); border-radius: 6px;"
    "}"

    /* ── Header buttons ────────────────────────────────────────── */
    ".hdr-btn {"
    "  background: transparent; border: none; box-shadow: none;"
    "  color: rgba(255,255,255,0.85);"
    "  padding: 5px 8px;"          /* SPACING: header button area */
    "  margin: 2px 2px;"           /* SPACING: gap between buttons */
    "  font-size: 16px;"
    "  border-radius: 6px; min-width: 0;"
    "}"
    ".hdr-btn:hover { background: rgba(255,255,255,0.2); color: white; }"
    ".hdr-btn.close {"
    "  color: #FF7070;"
    "  border-radius: 4px;"
    "}"
    ".hdr-btn.close:hover { background: rgba(255,80,80,0.25); border-radius: 6px; }"

    /* ── Bottom navigation bar ─────────────────────────────────── */
    ".nav-bar {"
    "  background-color: #3A5880;"  /* COLOUR: nav bar background (darker) */
    "  border-radius: 0 0 14px 14px;" /* SHAPE: rounded bottom corners */
    "  padding: 4px 6px;"           /* SPACING: nav bar inner padding */
    "}"
    ".nav-btn {"
    "  background: transparent; border: none; box-shadow: none;"
    "  color: rgba(255,255,255,0.55);" /* COLOUR: inactive nav icon */
    "  font-size: 20px;"
    "  padding: 6px 4px;"           /* SPACING: nav button size */
    "  margin: 4px;"              /* fixed side margin — keeps icons absolutely in place */
    "  border-radius: 8px; min-width: 0;"
    "}"
    ".nav-btn:hover { background: rgba(255,255,255,0.1); color: rgba(255,255,255,0.8); }"
    ".nav-btn.active {"
    "  background: rgba(255,255,255,0.18);"    /* COLOUR: active nav button fill */
    "  color: white;"

    "}"

    /* ── Emoji / symbol grid buttons ───────────────────────────── */
    ".glyph-btn {"
    "  background: rgba(255,255,255,0.08);"
    "  border: none; box-shadow: none;"
    "  border-radius: 6px;"
    "  color: white;"
    "  font-size: 22px;"            /* TEXT:   glyph display size */
    "  padding: 4px;"
    "  margin: 2px;"
    "  min-width: 40px; min-height: 40px;" /* SIZE: glyph button */
    "}"
    ".glyph-btn:hover { background: rgba(255,255,255,0.22); }"

    /* ── Settings page ─────────────────────────────────────────── */
    ".settings-section {"
    "  color: rgba(255,255,255,0.55);"
    "  font-size: 11px;"
    "  padding: 8px 14px 2px 14px;"
    "  letter-spacing: 1px;"
    "}"
    ".settings-row {"
    "  background: rgba(255,255,255,0.08);"
    "  border-radius: 8px;"
    "  margin: 2px 8px;"
    "  padding: 8px 12px;"
    "}"
    ".message-box {"
    "  background: rgba(255,255,255,0.1);"
    "  border-radius: 8px;"
    "  margin: 6px 8px 2px 8px;"
    "  padding: 8px 12px;"
    "}"
    ".message-label {"
    "  color: rgba(255,255,255,0.9);"
    "  font-size: 12px;"
    "}"

    ".empty-label {"
    "  color: rgba(255,255,255,0.45); font-size: 13px; padding: 30px;"
    "}"
    "scrolledwindow { background: transparent; }"
    "scrolledwindow undershoot, scrolledwindow overshoot { background: none; }";

/* ═══════════════════════════════════════════════════════════════
 * END DESIGN / THEME
 * ═══════════════════════════════════════════════════════════════ */

/* ── Window drawing ──────────────────────────────────────────── */
static void set_rgba_visual(GtkWidget *w) {
    GdkScreen *scr = gtk_widget_get_screen(w);
    GdkVisual *vis = gdk_screen_get_rgba_visual(scr);
    if (vis) gtk_widget_set_visual(w, vis);
}

static gboolean on_draw_bg(GtkWidget *w, cairo_t *cr, gpointer ud) {
    (void)ud;
    GtkAllocation a;
    gtk_widget_get_allocation(w, &a);
    double r=14.0, x=0, y=0, ww=a.width, hh=a.height;
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_rgba(cr, 0,0,0,0);
    cairo_paint(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    cairo_set_source_rgba(cr, 0.294,0.435,0.627,1.0);
    cairo_move_to(cr,x+r,y);
    cairo_line_to(cr,x+ww-r,y);
    cairo_arc(cr,x+ww-r,y+r,r,-G_PI/2,0);
    cairo_line_to(cr,x+ww,y+hh-r);
    cairo_arc(cr,x+ww-r,y+hh-r,r,0,G_PI/2);
    cairo_line_to(cr,x+r,y+hh);
    cairo_arc(cr,x+r,y+hh-r,r,G_PI/2,G_PI);
    cairo_line_to(cr,x,y+r);
    cairo_arc(cr,x+r,y+r,r,G_PI,3*G_PI/2);
    cairo_close_path(cr);
    cairo_fill(cr);
    return FALSE;
}

/* ── Cat image for header ────────────────────────────────────── */
static GtkWidget *make_header_cat(int size) {
    char exe[512] = {0}, rel[560] = {0};
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
        if (pb) {
            GtkWidget *img = gtk_image_new_from_pixbuf(pb);
            g_object_unref(pb);
            return img;
        }
        if (err) g_error_free(err);
    }
    return gtk_label_new("🐱");  /* fallback if file not found */
}

/* ── Window drag ─────────────────────────────────────────────── */
static gboolean on_header_press(GtkWidget *w, GdkEventButton *ev, gpointer ud) {
    (void)w; (void)ud;
    if (ev->button == 1)
        gtk_window_begin_move_drag(GTK_WINDOW(g_main_window),
                                   ev->button, ev->x_root, ev->y_root, ev->time);
    return FALSE;
}

/* ── Paste simulation ────────────────────────────────────────── */
static gboolean do_paste_cb(gpointer ud) {
    (void)ud;
    if (!s_do_paste) return FALSE;
    s_do_paste = FALSE;
    if (s_prev_focus == None || s_prev_focus == (Window)PointerRoot) return FALSE;
    Display *dpy    = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
    Window   target = s_prev_focus;
    s_prev_focus    = None;
    XSetInputFocus(dpy, target, RevertToParent, CurrentTime);
    KeyCode ctrl_kc = XKeysymToKeycode(dpy, XK_Control_L);
    KeyCode v_kc    = XKeysymToKeycode(dpy, XK_v);
    XTestFakeKeyEvent(dpy, ctrl_kc, True,  0);
    XTestFakeKeyEvent(dpy, v_kc,    True,  0);
    XTestFakeKeyEvent(dpy, v_kc,    False, 0);
    XTestFakeKeyEvent(dpy, ctrl_kc, False, 0);
    XFlush(dpy);
    return FALSE;
}

static void paste_text_and_close(const char *text) {
    /* Set clipboard */
    GtkClipboard *cb = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
    gtk_clipboard_set_text(cb, text, -1);
    if (!s_pinned) {
        s_do_paste = TRUE;
        ui_hide_window();
        g_timeout_add(200, do_paste_cb, NULL);
    }
}

/* ── History page — item actions ─────────────────────────────── */
static void item_copy_and_close(GtkWidget *w, gpointer ud) {
    (void)w;
    ClipItem *item = ud;
    clipboard_paste_item(item);
    if (item->type == CLIP_TEXT && item->text) {
        char *txt = g_strdup(item->text);
        storage_remove(item);
        storage_add_text(txt);
        g_free(txt);
    }
    ui_refresh_list();
    if (!s_pinned) {
        s_do_paste = TRUE;
        ui_hide_window();
        g_timeout_add(200, do_paste_cb, NULL);
    }
}

static void item_delete_cb(GtkWidget *w, gpointer ud) {
    (void)w;
    storage_remove((ClipItem *)ud);
    ui_refresh_list();
}

static void item_copy_only_cb(GtkWidget *w, gpointer ud) {
    (void)w;
    clipboard_paste_item((ClipItem *)ud);
    ui_refresh_list();
    if (!s_pinned) ui_hide_window();
}

static void show_item_menu(GtkWidget *anchor, ClipItem *item) {
    GtkWidget *menu    = gtk_menu_new();
    GtkWidget *mi_copy = gtk_menu_item_new_with_label("Copy to clipboard");
    GtkWidget *mi_del  = gtk_menu_item_new_with_label("Delete");
    GtkWidget *sep     = gtk_separator_menu_item_new();
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi_copy);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), sep);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi_del);
    g_signal_connect(mi_copy, "activate", G_CALLBACK(item_copy_only_cb), item);
    g_signal_connect(mi_del,  "activate", G_CALLBACK(item_delete_cb),    item);
    gtk_widget_show_all(menu);
    gtk_menu_popup_at_widget(GTK_MENU(menu), anchor,
                             GDK_GRAVITY_SOUTH_EAST, GDK_GRAVITY_NORTH_EAST, NULL);
}

static void on_more_clicked(GtkButton *b, gpointer ud) {
    show_item_menu(GTK_WIDGET(b), (ClipItem *)ud);
}

/* ── History list filter ─────────────────────────────────────── */
static gboolean list_filter(GtkListBoxRow *row, gpointer ud) {
    (void)ud;
    const char *q = s_search_query[PAGE_HISTORY];
    if (!q || !*q) return TRUE;
    ClipItem *item = g_object_get_data(G_OBJECT(row), "clip-item");
    if (!item) return TRUE;
    if (item->preview && strcasestr(item->preview, q)) return TRUE;
    if (item->text    && strcasestr(item->text,    q)) return TRUE;
    return FALSE;
}

static void on_search_changed(GtkSearchEntry *e, gpointer ud) {
    int page = GPOINTER_TO_INT(ud);
    g_free(s_search_query[page]);
    s_search_query[page] = g_strdup(gtk_entry_get_text(GTK_ENTRY(e)));
    if (page == PAGE_HISTORY)
        gtk_list_box_invalidate_filter(GTK_LIST_BOX(s_list_box));
    else if (page == PAGE_ICONS)
        gtk_flow_box_invalidate_filter(GTK_FLOW_BOX(s_icon_grid));
    else if (page == PAGE_SYMBOLS)
        gtk_flow_box_invalidate_filter(GTK_FLOW_BOX(s_sym_grid));
}

/* ── Build one history row ───────────────────────────────────── */
static GtkWidget *make_row(ClipItem *item) {
    /* clip-item-row goes on the ListBoxRow, NOT the inner box.
     * GtkListBoxRow tracks pointer state so :hover CSS works.
     * A plain GtkBox is not interactive — :hover would never fire on it. */
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

    GtkWidget *cbtn = gtk_button_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(cbtn), "clip-item-btn");
    gtk_widget_set_hexpand(cbtn, TRUE);
    gtk_button_set_relief(GTK_BUTTON(cbtn), GTK_RELIEF_NONE);

    if (item->type == CLIP_IMAGE && item->image) {
        int sw=gdk_pixbuf_get_width(item->image), sh=gdk_pixbuf_get_height(item->image);
        int th=50, tw=(sw*th)/MAX(sh,1);
        if (tw>240){tw=240;th=(sh*tw)/MAX(sw,1);}
        GdkPixbuf *thumb=gdk_pixbuf_scale_simple(item->image,tw,th,GDK_INTERP_BILINEAR);
        GtkWidget *img=gtk_image_new_from_pixbuf(thumb);
        g_object_unref(thumb);
        gtk_button_set_image(GTK_BUTTON(cbtn), img);
        gtk_button_set_always_show_image(GTK_BUTTON(cbtn), TRUE);
    } else {
        GtkWidget *lbl = gtk_label_new(item->preview ? item->preview : "");
        gtk_label_set_xalign(GTK_LABEL(lbl), 0.0f);
        gtk_label_set_ellipsize(GTK_LABEL(lbl), PANGO_ELLIPSIZE_END);
        gtk_label_set_max_width_chars(GTK_LABEL(lbl), 34);
        gtk_widget_set_halign(lbl, GTK_ALIGN_FILL);
        gtk_container_add(GTK_CONTAINER(cbtn), lbl);
    }
    g_signal_connect(cbtn, "clicked", G_CALLBACK(item_copy_and_close), item);

    GtkWidget *mbtn = gtk_button_new_with_label("⋮");
    gtk_style_context_add_class(gtk_widget_get_style_context(mbtn), "clip-more-btn");
    gtk_button_set_relief(GTK_BUTTON(mbtn), GTK_RELIEF_NONE);
    gtk_widget_set_valign(mbtn, GTK_ALIGN_CENTER);
    g_signal_connect(mbtn, "clicked", G_CALLBACK(on_more_clicked), item);

    gtk_box_pack_start(GTK_BOX(box), cbtn, TRUE, TRUE, 0);
    gtk_box_pack_end  (GTK_BOX(box), mbtn, FALSE, FALSE, 2);

    GtkWidget *row = gtk_list_box_row_new();
    /* Apply the style class here so :hover state is tracked */
    gtk_style_context_add_class   (gtk_widget_get_style_context(row), "clip-item-row");
    gtk_style_context_remove_class(gtk_widget_get_style_context(row), "activatable");
    gtk_list_box_row_set_activatable(GTK_LIST_BOX_ROW(row), FALSE);
    gtk_list_box_row_set_selectable (GTK_LIST_BOX_ROW(row), FALSE);
    g_object_set_data(G_OBJECT(row), "clip-item", item);
    gtk_container_add(GTK_CONTAINER(row), box);
    return row;
}

/* ── Build history page ──────────────────────────────────────── */
static GtkWidget *build_history_page(void) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    s_search[PAGE_HISTORY] = gtk_search_entry_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(s_search[PAGE_HISTORY]), "clip-search");
    gtk_entry_set_placeholder_text(GTK_ENTRY(s_search[PAGE_HISTORY]), "Search clipboard history...");
    g_signal_connect(s_search[PAGE_HISTORY], "search-changed",
                     G_CALLBACK(on_search_changed), GINT_TO_POINTER(PAGE_HISTORY));
    gtk_box_pack_start(GTK_BOX(vbox), s_search[PAGE_HISTORY], FALSE, FALSE, 0);

    GtkWidget *sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(sw, TRUE);

    s_list_box = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(s_list_box), GTK_SELECTION_NONE);
    gtk_list_box_set_filter_func(GTK_LIST_BOX(s_list_box), list_filter, NULL, NULL);
    gtk_style_context_add_class(gtk_widget_get_style_context(s_list_box), "background");
    GtkCssProvider *lb_css = gtk_css_provider_new();
    gtk_css_provider_load_from_data(lb_css, "list{background:transparent;}", -1, NULL);
    gtk_style_context_add_provider(gtk_widget_get_style_context(s_list_box),
        GTK_STYLE_PROVIDER(lb_css), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(lb_css);

    gtk_container_add(GTK_CONTAINER(sw), s_list_box);
    gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 0);
    return vbox;
}

/* ── Glyph grid filter (icons + symbols) ────────────────────── */
static gboolean glyph_filter(GtkFlowBoxChild *child, gpointer ud) {
    int page = GPOINTER_TO_INT(ud);
    const char *q = s_search_query[page];
    if (!q || !*q) return TRUE;
    const char *keys = g_object_get_data(G_OBJECT(child), "keys");
    if (!keys) return FALSE;
    return strcasestr(keys, q) != NULL;
}

static void on_glyph_clicked(GtkButton *btn, gpointer ud) {
    (void)ud;
    const char *text = gtk_button_get_label(btn);
    storage_add_text(text);
    ui_refresh_list();
    paste_text_and_close(text);
}

static GtkWidget *build_glyph_page(const Glyph *data, int page_idx,
                                   const char *placeholder,
                                   GtkWidget **grid_out) {
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    s_search[page_idx] = gtk_search_entry_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(s_search[page_idx]), "clip-search");
    gtk_entry_set_placeholder_text(GTK_ENTRY(s_search[page_idx]), placeholder);
    g_signal_connect(s_search[page_idx], "search-changed",
                     G_CALLBACK(on_search_changed), GINT_TO_POINTER(page_idx));
    gtk_box_pack_start(GTK_BOX(vbox), s_search[page_idx], FALSE, FALSE, 0);

    GtkWidget *sw = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(sw, TRUE);

    GtkWidget *grid = gtk_flow_box_new();
    gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(grid), GTK_SELECTION_NONE);
    gtk_flow_box_set_min_children_per_line(GTK_FLOW_BOX(grid), 6);
    gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(grid), 99);
    gtk_flow_box_set_homogeneous(GTK_FLOW_BOX(grid), TRUE);
    gtk_flow_box_set_filter_func(GTK_FLOW_BOX(grid), glyph_filter,
                                 GINT_TO_POINTER(page_idx), NULL);
    gtk_widget_set_margin_start(grid, 6);
    gtk_widget_set_margin_end  (grid, 6);
    gtk_widget_set_margin_top  (grid, 4);
    *grid_out = grid;

    /* Populate */
    for (int i = 0; data[i].glyph; i++) {
        GtkWidget *btn = gtk_button_new_with_label(data[i].glyph);
        gtk_style_context_add_class(gtk_widget_get_style_context(btn), "glyph-btn");
        gtk_button_set_relief(GTK_BUTTON(btn), GTK_RELIEF_NONE);
        gtk_widget_set_tooltip_text(btn, data[i].keys);
        g_signal_connect(btn, "clicked", G_CALLBACK(on_glyph_clicked), NULL);

        GtkWidget *child = gtk_flow_box_child_new();
        g_object_set_data(G_OBJECT(child), "keys", (gpointer)data[i].keys);
        gtk_container_add(GTK_CONTAINER(child), btn);
        /* remove FlowBoxChild default styling */
        gtk_style_context_add_class(gtk_widget_get_style_context(child), "background");
        GtkCssProvider *cp = gtk_css_provider_new();
        gtk_css_provider_load_from_data(cp,
            "flowboxchild{background:transparent;padding:0;margin:0;border:none;}", -1, NULL);
        gtk_style_context_add_provider(gtk_widget_get_style_context(child),
            GTK_STYLE_PROVIDER(cp), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
        g_object_unref(cp);

        gtk_flow_box_insert(GTK_FLOW_BOX(grid), child, -1);
    }

    gtk_container_add(GTK_CONTAINER(sw), grid);
    gtk_box_pack_start(GTK_BOX(vbox), sw, TRUE, TRUE, 0);
    return vbox;
}

/* ── Remote message fetch ────────────────────────────────────── */
static void on_fetch_done(GObject *proc, GAsyncResult *res, gpointer data) {
    GtkLabel *label = GTK_LABEL(data);
    char *out = NULL;
    GError *err = NULL;
    if (g_subprocess_communicate_utf8_finish(G_SUBPROCESS(proc), res, &out, NULL, &err)) {
        if (out) {
            g_strstrip(out);
            if (*out) gtk_label_set_text(label, out);
        }
    }
    g_free(out);
    if (err) g_error_free(err);
    g_object_unref(proc);
}

static void fetch_remote_message(void) {
    if (!s_msg_label || !MESSAGE_URL[0]) return;
    gtk_label_set_text(GTK_LABEL(s_msg_label), "Checking for messages…");
    GError *err = NULL;
    GSubprocess *sp = g_subprocess_new(
        G_SUBPROCESS_FLAGS_STDOUT_PIPE | G_SUBPROCESS_FLAGS_STDERR_SILENCE,
        &err, "curl", "-s", "--max-time", "5", MESSAGE_URL, NULL);
    if (!sp) { if (err) g_error_free(err); return; }
    g_subprocess_communicate_utf8_async(sp, NULL, NULL, on_fetch_done, s_msg_label);
}

/* ── Settings page ───────────────────────────────────────────── */
static GtkWidget *s_hotkey_entry = NULL;
static GtkWidget *s_spin_btn     = NULL;

static gboolean restore_entry_sensitivity(gpointer ud) {
    if (s_hotkey_entry) gtk_widget_set_sensitive(s_hotkey_entry, TRUE);
    if (s_spin_btn)     gtk_widget_set_sensitive(s_spin_btn,     TRUE);
    (void)ud;
    return G_SOURCE_REMOVE;
}

static void on_save_settings(GtkButton *b, gpointer ud) {
    (void)b; (void)ud;
    if (s_hotkey_entry) {
        const char *hk = gtk_entry_get_text(GTK_ENTRY(s_hotkey_entry));
        if (hk && *hk) { hotkey_set(hk); settings_set_hotkey(hk); }
    }
    /* Brief visual feedback: dim the controls for 600 ms */
    if (s_hotkey_entry) gtk_widget_set_sensitive(s_hotkey_entry, FALSE);
    if (s_spin_btn)     gtk_widget_set_sensitive(s_spin_btn,     FALSE);
    g_timeout_add(600, restore_entry_sensitivity, NULL);
}

static void on_clear_history(GtkButton *b, gpointer ud) {
    (void)b; (void)ud;
    storage_clear();
    ui_refresh_list();
}

static GtkWidget *build_settings_page(void) {
    GtkWidget *sw   = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_bottom(vbox, 10);
    gtk_container_add(GTK_CONTAINER(sw), vbox);

    /* ── Hotkey ── */
    GtkWidget *lbl_hk = gtk_label_new("HOTKEY");
    gtk_style_context_add_class(gtk_widget_get_style_context(lbl_hk), "settings-section");
    gtk_label_set_xalign(GTK_LABEL(lbl_hk), 0);
    gtk_box_pack_start(GTK_BOX(vbox), lbl_hk, FALSE, FALSE, 0);

    GtkWidget *row_hk = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_style_context_add_class(gtk_widget_get_style_context(row_hk), "settings-row");
    GtkWidget *lbl_hk2 = gtk_label_new("Toggle key");
    gtk_label_set_markup(GTK_LABEL(lbl_hk2),
        "<span foreground='white' size='small'>Toggle key</span>");
    gtk_box_pack_start(GTK_BOX(row_hk), lbl_hk2, FALSE, FALSE, 0);
    s_hotkey_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(s_hotkey_entry), hotkey_get());
    gtk_widget_set_hexpand(s_hotkey_entry, TRUE);
    gtk_entry_set_has_frame(GTK_ENTRY(s_hotkey_entry), FALSE);
    GtkCssProvider *e_css = gtk_css_provider_new();
    gtk_css_provider_load_from_data(e_css,
        "entry{background:rgba(255,255,255,0.12);color:white;"
        "border-radius:6px;padding:4px 8px;border:none;}", -1, NULL);
    gtk_style_context_add_provider(gtk_widget_get_style_context(s_hotkey_entry),
        GTK_STYLE_PROVIDER(e_css), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(e_css);
    gtk_box_pack_end(GTK_BOX(row_hk), s_hotkey_entry, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), row_hk, FALSE, FALSE, 0);

    GtkWidget *hint = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(hint),
        "<span foreground='#aabbcc' size='x-small'>"
        "  Modifiers: ctrl  shift  alt  super   e.g.  super+v"
        "</span>");
    gtk_label_set_xalign(GTK_LABEL(hint), 0);
    gtk_widget_set_margin_start(hint, 14);
    gtk_widget_set_margin_bottom(hint, 4);
    gtk_box_pack_start(GTK_BOX(vbox), hint, FALSE, FALSE, 0);

    /* ── History ── */
    GtkWidget *lbl_hist = gtk_label_new("HISTORY");
    gtk_style_context_add_class(gtk_widget_get_style_context(lbl_hist), "settings-section");
    gtk_label_set_xalign(GTK_LABEL(lbl_hist), 0);
    gtk_box_pack_start(GTK_BOX(vbox), lbl_hist, FALSE, FALSE, 0);

    GtkWidget *row_max = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_style_context_add_class(gtk_widget_get_style_context(row_max), "settings-row");
    GtkWidget *lbl_max = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(lbl_max),
        "<span foreground='white' size='small'>Max entries</span>");
    gtk_box_pack_start(GTK_BOX(row_max), lbl_max, TRUE, TRUE, 0);
    s_spin_btn = gtk_spin_button_new_with_range(10, 200, 5);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(s_spin_btn), MAX_HISTORY);
    GtkCssProvider *sp_css = gtk_css_provider_new();
    gtk_css_provider_load_from_data(sp_css,
        "spinbutton{background:rgba(255,255,255,0.12);color:white;"
        "border-radius:6px;border:none;}", -1, NULL);
    gtk_style_context_add_provider(gtk_widget_get_style_context(s_spin_btn),
        GTK_STYLE_PROVIDER(sp_css), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(sp_css);
    gtk_box_pack_end(GTK_BOX(row_max), s_spin_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), row_max, FALSE, FALSE, 0);

    GtkWidget *row_clr = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(row_clr), "settings-row");
    GtkWidget *clr_btn = gtk_button_new_with_label("🗑  Clear all history");
    gtk_button_set_relief(GTK_BUTTON(clr_btn), GTK_RELIEF_NONE);
    GtkCssProvider *clr_css = gtk_css_provider_new();
    gtk_css_provider_load_from_data(clr_css,
        "button{background:rgba(255,80,80,0.25);color:white;border-radius:6px;"
        "border:none;padding:5px 12px;}"
        "button:hover{background:rgba(255,80,80,0.45);}", -1, NULL);
    gtk_style_context_add_provider(gtk_widget_get_style_context(clr_btn),
        GTK_STYLE_PROVIDER(clr_css), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(clr_css);
    gtk_widget_set_halign(clr_btn, GTK_ALIGN_START);
    g_signal_connect(clr_btn, "clicked", G_CALLBACK(on_clear_history), NULL);
    gtk_box_pack_start(GTK_BOX(row_clr), clr_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), row_clr, FALSE, FALSE, 0);

    /* ── Save button ── */
    GtkWidget *save_btn = gtk_button_new_with_label("Save Settings");
    gtk_button_set_relief(GTK_BUTTON(save_btn), GTK_RELIEF_NONE);
    GtkCssProvider *sv_css = gtk_css_provider_new();
    gtk_css_provider_load_from_data(sv_css,
        "button{background:rgba(255,255,255,0.2);color:white;border-radius:6px;"
        "border:none;padding:6px 14px;margin:8px;}"
        "button:hover{background:rgba(255,255,255,0.35);}", -1, NULL);
    gtk_style_context_add_provider(gtk_widget_get_style_context(save_btn),
        GTK_STYLE_PROVIDER(sv_css), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(sv_css);
    gtk_widget_set_halign(save_btn, GTK_ALIGN_CENTER);
    g_signal_connect(save_btn, "clicked", G_CALLBACK(on_save_settings), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), save_btn, FALSE, FALSE, 0);

    /* ── Remote message ── */
    GtkWidget *lbl_msg = gtk_label_new("ANNOUNCEMENTS");
    gtk_style_context_add_class(gtk_widget_get_style_context(lbl_msg), "settings-section");
    gtk_label_set_xalign(GTK_LABEL(lbl_msg), 0);
    gtk_box_pack_start(GTK_BOX(vbox), lbl_msg, FALSE, FALSE, 0);

    GtkWidget *msg_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(msg_box), "message-box");
    s_msg_label = gtk_label_new("");
    gtk_style_context_add_class(gtk_widget_get_style_context(s_msg_label), "message-label");
    gtk_label_set_line_wrap(GTK_LABEL(s_msg_label), TRUE);
    gtk_label_set_xalign(GTK_LABEL(s_msg_label), 0);
    gtk_box_pack_start(GTK_BOX(msg_box), s_msg_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), msg_box, FALSE, FALSE, 0);

    /* ── Credit row ── */
    GtkWidget *credit_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(credit_box), "message-box");
    GtkWidget *credit_lbl = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(credit_lbl),
        "<span foreground='#aabbcc' size='small' style='italic'>"
        "Developped by irony! (vibe coded)"
        "</span>");
    gtk_label_set_xalign(GTK_LABEL(credit_lbl), 0);
    gtk_box_pack_start(GTK_BOX(credit_box), credit_lbl, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), credit_box, FALSE, FALSE, 0);

    s_search[PAGE_SETTINGS] = NULL;  /* settings has no search bar */
    return sw;
}

/* ── Key handler ─────────────────────────────────────────────── */
static gboolean on_key_press(GtkWidget *w, GdkEventKey *ev, gpointer ud) {
    (void)w; (void)ud;
    if (ev->keyval == GDK_KEY_Escape) { ui_hide_window(); return TRUE; }
    return FALSE;
}

/* ── Navigation ──────────────────────────────────────────────── */
static void switch_page(Page p) {
    gtk_stack_set_visible_child_name(GTK_STACK(s_stack), PAGE_NAMES[p]);
    for (int i = 0; i < 4; i++) {
        GtkStyleContext *ctx = gtk_widget_get_style_context(s_nav_btns[i]);
        if (i == (int)p) gtk_style_context_add_class   (ctx, "active");
        else             gtk_style_context_remove_class(ctx, "active");
    }
    s_current_page = p;
    /* Focus the search bar for the new page */
    if (p != PAGE_SETTINGS && s_search[p])
        gtk_widget_grab_focus(s_search[p]);
    /* Fetch remote message the first time settings is opened */
    if (p == PAGE_SETTINGS && !s_msg_fetched) {
        s_msg_fetched = TRUE;
        fetch_remote_message();
    }
}

static void on_nav_clicked(GtkButton *b, gpointer ud) {
    (void)b;
    switch_page((Page)GPOINTER_TO_INT(ud));
}

/* ── Window position ─────────────────────────────────────────── */
static void position_window(void) {
    GdkDisplay  *disp = gdk_display_get_default();
    GdkMonitor  *mon  = gdk_display_get_primary_monitor(disp);
    GdkRectangle geom;
    gdk_monitor_get_geometry(mon, &geom);
    gtk_window_move(GTK_WINDOW(g_main_window),
                    geom.x + geom.width  - WIN_WIDTH  - 20,
                    geom.y + 40);
}

/* ── ui_init ─────────────────────────────────────────────────── */
void ui_init(void) {
    /* CSS */
    GtkCssProvider *css = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css, CSS, -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(css), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css);

    /* Window */
    g_main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title           (GTK_WINDOW(g_main_window), "Clipped");
    gtk_window_set_default_size    (GTK_WINDOW(g_main_window), WIN_WIDTH, WIN_HEIGHT);
    gtk_window_set_resizable       (GTK_WINDOW(g_main_window), FALSE);
    gtk_window_set_decorated       (GTK_WINDOW(g_main_window), FALSE);
    gtk_window_set_keep_above      (GTK_WINDOW(g_main_window), TRUE);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(g_main_window), TRUE);
    gtk_window_set_skip_pager_hint (GTK_WINDOW(g_main_window), TRUE);
    gtk_window_set_type_hint       (GTK_WINDOW(g_main_window), GDK_WINDOW_TYPE_HINT_UTILITY);
    gtk_widget_set_app_paintable   (g_main_window, TRUE);
    gtk_style_context_add_class    (gtk_widget_get_style_context(g_main_window), "clipman");
    set_rgba_visual(g_main_window);
    g_signal_connect(g_main_window, "screen-changed",  G_CALLBACK(set_rgba_visual),         NULL);
    g_signal_connect(g_main_window, "draw",             G_CALLBACK(on_draw_bg),              NULL);
    g_signal_connect(g_main_window, "delete-event",     G_CALLBACK(gtk_widget_hide_on_delete),NULL);
    g_signal_connect(g_main_window, "key-press-event",  G_CALLBACK(on_key_press),            NULL);

    /* Root vbox */
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(g_main_window), vbox);

    /* ── Header (title + close only) ── */
    GtkWidget *hdr = gtk_event_box_new();
    gtk_style_context_add_class(gtk_widget_get_style_context(hdr), "clip-header");
    g_signal_connect(hdr, "button-press-event", G_CALLBACK(on_header_press), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), hdr, FALSE, FALSE, 0);

    GtkWidget *hdr_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_container_add(GTK_CONTAINER(hdr), hdr_box);

    /* Cat icon — 15 px from left edge, 15 px from top, 10 px bottom gap */
    GtkWidget *ico = make_header_cat(28);
    gtk_widget_set_margin_start (ico, 10);
    gtk_widget_set_margin_top   (ico, 5);
    gtk_widget_set_margin_bottom(ico, 10);
    gtk_box_pack_start(GTK_BOX(hdr_box), ico, FALSE, FALSE, 0);

    /* Title — 8 px gap after icon, same 15 px top */
    GtkWidget *title = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title),
        "<span foreground='white' weight='bold' size='medium'>Clipped</span>");
    gtk_widget_set_margin_start (title, 8);
    gtk_widget_set_margin_top   (title, 15);
    gtk_widget_set_margin_bottom(title, 10);
    gtk_box_pack_start(GTK_BOX(hdr_box), title, FALSE, FALSE, 0);

    /* spacer */
    gtk_box_pack_start(GTK_BOX(hdr_box), gtk_label_new(""), TRUE, TRUE, 0);

    /* Close button — 15 px from right edge, 15 px from top */
    GtkWidget *cls_btn = gtk_button_new_with_label("✕");
    gtk_style_context_add_class(gtk_widget_get_style_context(cls_btn), "hdr-btn");
    gtk_style_context_add_class(gtk_widget_get_style_context(cls_btn), "close");
    gtk_button_set_relief(GTK_BUTTON(cls_btn), GTK_RELIEF_NONE);
    gtk_widget_set_tooltip_text(cls_btn, "Close");
    gtk_widget_set_margin_end   (cls_btn, 10);
    gtk_widget_set_margin_top   (cls_btn, 5);
    gtk_widget_set_margin_bottom(cls_btn, 6);
    g_signal_connect_swapped(cls_btn, "clicked", G_CALLBACK(ui_hide_window), NULL);
    gtk_box_pack_start(GTK_BOX(hdr_box), cls_btn, FALSE, FALSE, 0);

    /* ── Page stack ── */
    s_stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(s_stack), GTK_STACK_TRANSITION_TYPE_NONE);
    gtk_widget_set_vexpand(s_stack, TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), s_stack, TRUE, TRUE, 0);

    gtk_stack_add_named(GTK_STACK(s_stack), build_history_page(), PAGE_NAMES[PAGE_HISTORY]);
    gtk_stack_add_named(GTK_STACK(s_stack),
        build_glyph_page(EMOJIS, PAGE_ICONS, "Search icons (cat, fire, heart…)", &s_icon_grid),
        PAGE_NAMES[PAGE_ICONS]);
    gtk_stack_add_named(GTK_STACK(s_stack),
        build_glyph_page(SYMBOLS, PAGE_SYMBOLS, "Search symbols (arrow, math, euro…)", &s_sym_grid),
        PAGE_NAMES[PAGE_SYMBOLS]);
    gtk_stack_add_named(GTK_STACK(s_stack), build_settings_page(), PAGE_NAMES[PAGE_SETTINGS]);

    /* ── Bottom navigation bar ── */
    GtkWidget *nav = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_style_context_add_class(gtk_widget_get_style_context(nav), "nav-bar");
    gtk_box_pack_end(GTK_BOX(vbox), nav, FALSE, FALSE, 0);

    static const char *nav_icons[]  = {"📋","😀","∑","⚙"};
    static const char *nav_tips[]   = {"Clipped","Icons","Symbols","Settings"};

    for (int i = 0; i < 4; i++) {
        s_nav_btns[i] = gtk_button_new_with_label(nav_icons[i]);
        gtk_style_context_add_class(gtk_widget_get_style_context(s_nav_btns[i]), "nav-btn");
        gtk_button_set_relief(GTK_BUTTON(s_nav_btns[i]), GTK_RELIEF_NONE);
        gtk_widget_set_tooltip_text(s_nav_btns[i], nav_tips[i]);
        gtk_widget_set_hexpand(s_nav_btns[i], TRUE);
        g_signal_connect(s_nav_btns[i], "clicked",
                         G_CALLBACK(on_nav_clicked), GINT_TO_POINTER(i));
        gtk_box_pack_start(GTK_BOX(nav), s_nav_btns[i], TRUE, TRUE, 0);
    }
    /* mark history as active by default */
    gtk_style_context_add_class(
        gtk_widget_get_style_context(s_nav_btns[PAGE_HISTORY]), "active");

    position_window();
    ui_refresh_list();
}

/* ── Public: refresh history list ───────────────────────────── */
void ui_refresh_list(void) {
    if (!s_list_box) return;
    GList *kids = gtk_container_get_children(GTK_CONTAINER(s_list_box));
    g_list_foreach(kids, (GFunc)(void*)gtk_widget_destroy, NULL);
    g_list_free(kids);

    if (!g_history) {
        GtkWidget *empty = gtk_label_new("Nothing copied yet.\nCopy some text or an image.");
        gtk_style_context_add_class(gtk_widget_get_style_context(empty), "empty-label");
        gtk_label_set_justify(GTK_LABEL(empty), GTK_JUSTIFY_CENTER);
        GtkWidget *row = gtk_list_box_row_new();
        gtk_list_box_row_set_activatable(GTK_LIST_BOX_ROW(row), FALSE);
        gtk_list_box_row_set_selectable (GTK_LIST_BOX_ROW(row), FALSE);
        gtk_container_add(GTK_CONTAINER(row), empty);
        gtk_list_box_insert(GTK_LIST_BOX(s_list_box), row, -1);
    } else {
        for (ClipItem *it = g_history; it; it = it->next)
            gtk_list_box_insert(GTK_LIST_BOX(s_list_box), make_row(it), -1);
    }
    gtk_widget_show_all(s_list_box);
    gtk_list_box_invalidate_filter(GTK_LIST_BOX(s_list_box));
}

/* ── Show / Hide / Toggle ────────────────────────────────────── */
void ui_show_window(void) {
    Display *dpy = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
    XGetInputFocus(dpy, &s_prev_focus, &s_prev_revert);
    position_window();
    gtk_widget_show_all(g_main_window);
    gtk_window_present_with_time(GTK_WINDOW(g_main_window), g_last_hotkey_time);
    GdkWindow *gwin = gtk_widget_get_window(g_main_window);
    if (gwin) gdk_window_raise(gwin);
    /* Focus search on current page */
    if (s_current_page != PAGE_SETTINGS && s_search[s_current_page])
        gtk_widget_grab_focus(s_search[s_current_page]);
    s_shown = TRUE;
}

void ui_hide_window(void) {
    gtk_widget_hide(g_main_window);
    /* Clear all search bars */
    for (int i = 0; i < 4; i++) {
        g_free(s_search_query[i]); s_search_query[i] = NULL;
        if (s_search[i]) gtk_entry_set_text(GTK_ENTRY(s_search[i]), "");
    }
    if (s_list_box) gtk_list_box_invalidate_filter(GTK_LIST_BOX(s_list_box));
    if (s_icon_grid) gtk_flow_box_invalidate_filter(GTK_FLOW_BOX(s_icon_grid));
    if (s_sym_grid)  gtk_flow_box_invalidate_filter(GTK_FLOW_BOX(s_sym_grid));
    s_shown = FALSE;
}

void ui_toggle_window(void) {
    if (s_shown) ui_hide_window();
    else         ui_show_window();
}

void ui_cleanup(void) {
    for (int i = 0; i < 4; i++) g_free(s_search_query[i]);
}
