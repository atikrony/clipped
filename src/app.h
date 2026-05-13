#ifndef APP_H
#define APP_H

#define _GNU_SOURCE

#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <cairo.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* ── Platform-specific headers ───────────────────────────────── */
#if defined(_WIN32)
  #include <windows.h>

#elif defined(__APPLE__)
  #include <Carbon/Carbon.h>
  #include <ApplicationServices/ApplicationServices.h>

#else  /* Linux / X11 */
  #include <gdk/gdkx.h>
  #include <X11/Xlib.h>
  #include <X11/keysym.h>
  #include <X11/extensions/XTest.h>
#endif

#define APP_NAME    "clipman"
#define APP_VERSION "1.0.0"
#define MAX_HISTORY 50           /* max clipboard entries kept in history */
#define PREVIEW_LEN 120          /* max chars shown in one-line preview   */
#define WIN_WIDTH   340          /* SIZE: panel width  in pixels */
#define WIN_HEIGHT  520          /* SIZE: panel height in pixels */

typedef enum { CLIP_TEXT, CLIP_IMAGE } ClipType;

typedef struct ClipItem {
    ClipType     type;
    char        *text;
    GdkPixbuf   *image;
    char        *preview;
    time_t       ts;
    struct ClipItem *next;
} ClipItem;

/* Globals shared across modules */
extern GtkWidget *g_main_window;
extern ClipItem  *g_history;
extern int        g_history_count;

#endif /* APP_H */
