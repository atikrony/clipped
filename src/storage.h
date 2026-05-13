#ifndef STORAGE_H
#define STORAGE_H

#include "app.h"

void      storage_init    (void);
void      storage_cleanup (void);
ClipItem *storage_add_text (const char *text);
ClipItem *storage_add_image(GdkPixbuf *pb);
void      storage_remove  (ClipItem *item);
void      storage_clear   (void);
void      storage_save    (void);

#endif
