#include "settings.h"

static GKeyFile *s_kf   = NULL;
static char     *s_path = NULL;

void settings_init(void) {
    char *dir = g_build_filename(g_get_home_dir(), ".config", APP_NAME, NULL);
    g_mkdir_with_parents(dir, 0700);
    s_path = g_build_filename(dir, "config.ini", NULL);
    g_free(dir);

    s_kf = g_key_file_new();
    g_key_file_load_from_file(s_kf, s_path, G_KEY_FILE_NONE, NULL);
}

const char *settings_get_hotkey(void) {
    static char *hk = NULL;
    g_free(hk);
    hk = g_key_file_get_string(s_kf, "General", "hotkey", NULL);
    return hk ? hk : "super+v";
}

void settings_set_hotkey(const char *hk) {
    g_key_file_set_string(s_kf, "General", "hotkey", hk);
    GError *err = NULL;
    if (!g_key_file_save_to_file(s_kf, s_path, &err) && err) {
        g_warning("settings save: %s", err->message);
        g_error_free(err);
    }
}

void settings_cleanup(void) {
    if (s_kf) { g_key_file_free(s_kf); s_kf = NULL; }
    g_free(s_path); s_path = NULL;
}
