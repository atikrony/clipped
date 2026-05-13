#ifndef SETTINGS_H
#define SETTINGS_H

#include "app.h"

void        settings_init      (void);
void        settings_cleanup   (void);
const char *settings_get_hotkey(void);
void        settings_set_hotkey(const char *hk);

#endif
