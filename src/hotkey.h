#ifndef HOTKEY_H
#define HOTKEY_H

#include "app.h"

/* Timestamp of the last hotkey KeyPress — read by ui_show_window so it can
 * call gtk_window_present_with_time and the WM will grant focus. */
extern guint32 g_last_hotkey_time;

void        hotkey_init   (void);
void        hotkey_cleanup(void);
void        hotkey_set    (const char *keystr);
const char *hotkey_get    (void);

#endif
