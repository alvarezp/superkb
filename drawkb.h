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

typedef int (*IQF_t)(KeySym keysym, unsigned int state, char buf[], int buf_n);
typedef enum {
	FULL_SHAPE,
	BASE_OUTLINE_ONLY,
	FLAT_KEY
} painting_mode_t;

typedef struct {
	Display *dpy;
	char *imagelib;
	char *font;
	IQF_t IQF;
	painting_mode_t painting_mode;
	float scale;
} drawkb_t, *drawkb_p;

drawkb_p drawkb_create();

int drawkb_init(Display *dpy, const char *imagelib, const char *font, IQF_t IQF, painting_mode_t painting_mode, float scale);
void drawkb_draw(Display * dpy, Drawable d, GC gc, unsigned int width, unsigned int height, XkbDescPtr kbdesc);

#endif /* __DRAWKB_H */
