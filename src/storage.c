#include "storage.h"

ClipItem *g_history       = NULL;
int       g_history_count = 0;

static char *s_data_dir = NULL;

static const char *data_dir(void) {
    if (!s_data_dir) {
        s_data_dir = g_build_filename(g_get_home_dir(),
                                      ".local", "share", APP_NAME, NULL);
        g_mkdir_with_parents(s_data_dir, 0700);
    }
    return s_data_dir;
}

static void item_free(ClipItem *it) {
    if (!it) return;
    g_free(it->text);
    g_free(it->preview);
    if (it->image) g_object_unref(it->image);
    g_free(it);
}

static void trim_to_limit(void) {
    while (g_history_count > MAX_HISTORY) {
        ClipItem *cur = g_history;
        if (!cur) break;
        if (!cur->next) {
            item_free(cur);
            g_history = NULL;
            g_history_count = 0;
            break;
        }
        while (cur->next && cur->next->next)
            cur = cur->next;
        item_free(cur->next);
        cur->next = NULL;
        g_history_count--;
    }
}

static ClipItem *new_item(ClipType type) {
    ClipItem *it = g_new0(ClipItem, 1);
    it->type = type;
    it->ts   = time(NULL);
    return it;
}

ClipItem *storage_add_text(const char *text) {
    if (!text || !*text) return NULL;

    /* Skip whitespace-only */
    const char *scan = text;
    while (*scan && g_ascii_isspace((guchar)*scan)) scan++;
    if (!*scan) return NULL;

    /* Skip if identical to the most-recent entry — no point re-adding */
    if (g_history && g_history->type == CLIP_TEXT && g_history->text &&
        strcmp(g_history->text, text) == 0)
        return g_history;

    /* Deduplicate: remove existing same-text entry */
    ClipItem *prev = NULL, *cur = g_history;
    while (cur) {
        if (cur->type == CLIP_TEXT && cur->text &&
            strcmp(cur->text, text) == 0) {
            if (prev) prev->next = cur->next;
            else      g_history  = cur->next;
            item_free(cur);
            g_history_count--;
            break;
        }
        prev = cur;
        cur  = cur->next;
    }

    ClipItem *it = new_item(CLIP_TEXT);
    it->text = g_strdup(text);

    /* Build single-line preview */
    char *p = g_strdup(text);
    for (int i = 0; p[i]; i++)
        if (p[i] == '\n' || p[i] == '\r' || p[i] == '\t') p[i] = ' ';
    if ((int)strlen(p) > PREVIEW_LEN) { p[PREVIEW_LEN-3]='.'; p[PREVIEW_LEN-2]='.'; p[PREVIEW_LEN-1]='.'; p[PREVIEW_LEN]='\0'; }
    it->preview = p;

    it->next  = g_history;
    g_history = it;
    g_history_count++;
    trim_to_limit();
    return it;
}

ClipItem *storage_add_image(GdkPixbuf *pb) {
    if (!pb) return NULL;
    ClipItem *it  = new_item(CLIP_IMAGE);
    it->image     = g_object_ref(pb);
    it->preview   = g_strdup("[Image]");
    it->next      = g_history;
    g_history     = it;
    g_history_count++;
    trim_to_limit();
    return it;
}

void storage_remove(ClipItem *item) {
    ClipItem *prev = NULL, *cur = g_history;
    while (cur) {
        if (cur == item) {
            if (prev) prev->next = cur->next;
            else      g_history  = cur->next;
            item_free(cur);
            g_history_count--;
            return;
        }
        prev = cur; cur = cur->next;
    }
}

void storage_clear(void) {
    ClipItem *cur = g_history;
    while (cur) { ClipItem *n = cur->next; item_free(cur); cur = n; }
    g_history       = NULL;
    g_history_count = 0;
}

void storage_save(void) {
    char *path = g_build_filename(data_dir(), "history.txt", NULL);
    FILE *f = fopen(path, "w");
    g_free(path);
    if (!f) return;

    /* Save newest→oldest; on load we reverse */
    ClipItem *cur = g_history;
    while (cur) {
        if (cur->type == CLIP_TEXT && cur->text)
            fprintf(f, "---ENTRY---\n%s\n", cur->text);
        cur = cur->next;
    }
    fclose(f);
}

static void load_history(void) {
    char *path = g_build_filename(data_dir(), "history.txt", NULL);
    FILE *f = fopen(path, "r");
    g_free(path);
    if (!f) return;

    GPtrArray *entries = g_ptr_array_new_with_free_func(g_free);
    GString   *buf     = g_string_new(NULL);
    char       line[8192];
    gboolean   in_entry = FALSE;

    while (fgets(line, sizeof line, f)) {
        if (strcmp(line, "---ENTRY---\n") == 0) {
            if (in_entry && buf->len)
                g_ptr_array_add(entries, g_strdup(buf->str));
            g_string_truncate(buf, 0);
            in_entry = TRUE;
        } else if (in_entry) {
            g_string_append(buf, line);
        }
    }
    if (in_entry && buf->len)
        g_ptr_array_add(entries, g_strdup(buf->str));

    g_string_free(buf, TRUE);
    fclose(f);

    /* Add oldest first so newest ends at head */
    for (int i = (int)entries->len - 1; i >= 0; i--) {
        char *t = g_ptr_array_index(entries, i);
        size_t len = strlen(t);
        if (len && t[len-1] == '\n') t[len-1] = '\0';
        storage_add_text(t);
    }
    g_ptr_array_free(entries, TRUE);
}

void storage_init(void) {
    data_dir();
    load_history();
}

void storage_cleanup(void) {
    storage_save();
    storage_clear();
    g_free(s_data_dir);
    s_data_dir = NULL;
}
