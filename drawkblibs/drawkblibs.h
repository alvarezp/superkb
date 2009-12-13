/*
 * drawkblibs.h
 *
 * Copyright (C) 2006, Octavio Alvarez Piza.
 * License: GNU General Public License v2.
 *
 * This header should be included by all implementation providers of drawkblib
 * in order to comply and interoperate with the needed by drawkblibs.
 *
 */

#ifndef __DRAWKBLIBS_H
#define __DRAWKBLIBS_H

#include <X11/Xlib.h>
#include <X11/XKBlib.h>

#include <X11/Xft/Xft.h>
#include <X11/Xft/XftCompat.h>

#include "../debug.h"

typedef int (*IQF_t)(KeySym keysym, unsigned int state, char buf[], int buf_n);

typedef enum {
	FULL_SHAPE,
	BASE_OUTLINE_ONLY,
	FLAT_KEY
} painting_mode_t;

typedef struct {
	char font[500];
	XftFont *fs;
	Display *dpy;
	IQF_t IQF;
	painting_mode_t painting_mode;
	debug_t *debug;
} drawkb_t, *drawkb_p;

typedef int (puticon_t)(Drawable kbwin, int x, int y, int width, int height, const char *fn);

typedef drawkb_p (*drawkb_create_t)(Display *dpy, const char *font, IQF_t IQF, painting_mode_t painting_mode, float scale, debug_t debug);

typedef void (*drawkb_draw_t)(drawkb_p this, Drawable d, GC gc, unsigned int width, unsigned int height, XkbDescPtr kbdesc, puticon_t puticon);

typedef int (*drawkblib_init_t)(
	drawkb_create_t *ret_create,
	drawkb_draw_t *ret_draw
);


#endif /* __DRAWKBLIBS_H */
