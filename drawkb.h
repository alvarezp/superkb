/*
 * drawkb.h
 *
 * Copyright (C) 2006, Octavio Alvarez Piza.
 * License: GNU General Public License v2.
 *
 */

#ifndef __DRAWKB_H
#define __DRAWKB_H

#include <X11/Xlib.h>
#include <X11/XKBlib.h>

#include <X11/Xft/Xft.h>
#include <X11/Xft/XftCompat.h>

typedef int (*IQF_t)(KeySym keysym, unsigned int state, char buf[], int buf_n);
typedef enum {
	FULL_SHAPE,
	BASE_OUTLINE_ONLY,
	FLAT_KEY
} painting_mode_t;

typedef struct {
	char font[500];
	XftFont * fs;
	Display *dpy;
	IQF_t IQF;
	painting_mode_t painting_mode;
} drawkb_t, *drawkb_p;

drawkb_p drawkb_create(Display *dpy, const char *imagelib, const char *font,
	IQF_t IQF, painting_mode_t painting_mode, float scale);

void drawkb_draw(drawkb_p this, Drawable d, GC gc, unsigned int width, unsigned int height, XkbDescPtr kbdesc);

#endif /* __DRAWKB_H */
