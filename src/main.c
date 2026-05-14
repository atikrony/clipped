#include "app.h"
#include "storage.h"
#include "clipboard.h"
#include "ui.h"
#include "hotkey.h"
#include "settings.h"
#include "tray.h"
#ifndef _WIN32
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>
#endif

int main(int argc, char *argv[]) {
    /* ── Single-instance guard ──────────────────────────────────
     * Prevent a second copy from running. On Windows use a named
     * mutex; on POSIX use an exclusive lock file. */
#if defined(_WIN32)
    {
        HANDLE mutex = CreateMutexA(NULL, TRUE, "Global\\clipman-single-instance");
        if (!mutex || GetLastError() == ERROR_ALREADY_EXISTS) {
            if (mutex) CloseHandle(mutex);
            return 0;   /* already running — quit silently */
        }
        /* mutex is intentionally left open until process exits */
    }
#else
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
#endif

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
