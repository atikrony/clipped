#include "app.h"
#include "storage.h"
#include "clipboard.h"
#include "ui.h"
#include "hotkey.h"
#include "settings.h"
#include "tray.h"
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    /* ── Single-instance guard ──────────────────────────────────
     * Try to acquire an exclusive non-blocking lock on a per-user
     * file.  If another clipman is already running the lock fails
     * and we exit immediately instead of spawning a second copy. */
    {
        char lp[256];
        snprintf(lp, sizeof(lp), "/tmp/clipman-%d.lock", (int)getuid());
        int lfd = open(lp, O_CREAT | O_RDWR, 0600);
        if (lfd < 0 || flock(lfd, LOCK_EX | LOCK_NB) != 0) {
            if (lfd >= 0) close(lfd);
            return 0;   /* already running — quit silently */
        }
        /* lfd is intentionally left open; the OS releases the lock
         * automatically when the process exits (clean or crash). */
    }

    gtk_init(&argc, &argv);

    settings_init();
    storage_init();
    ui_init();
    clipboard_init();
    tray_init();

    /* Hotkey: init X11 watcher then set saved/default key */
    hotkey_init();
    hotkey_set(settings_get_hotkey());

    gtk_main();

    hotkey_cleanup();
    tray_cleanup();
    clipboard_cleanup();
    storage_cleanup();  /* saves history */
    settings_cleanup();
    ui_cleanup();

    return 0;
}
