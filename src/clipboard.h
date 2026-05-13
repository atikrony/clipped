#ifndef CLIPBOARD_H
#define CLIPBOARD_H

#include "app.h"

void clipboard_init      (void);
void clipboard_cleanup   (void);
void clipboard_paste_item(ClipItem *item);

#endif
