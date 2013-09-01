/*
 * drawkblibs-xlib.h
 *
 * Copyright (C) 2006, Octavio Alvarez Piza.
 * License: GNU General Public License v2.
 *
 * This is the actual implementation of drawkb by using X11 to render a
 * keyboard onto an X11 drawable.
 *
 * This code is not by Superkb directly, but by drawkblib.c This allows
 * drawkblib.c to provide multiple options to keyboard rendering.
 *
 * See drawkblib.h for details.
 *
 */

#ifndef __DRAWKBLIBS_XLIB_H
#define __DRAWKBLIBS_XLIB_H

#include "drawkblibs.h"

int drawkb_xlib_create(Display *dpy, const char *imagelib, const char *font, IQF_t IQF, painting_mode_t painting_mode, float scale, XkbDescPtr kbdesc, int use_gradients);
void drawkb_xlib_draw(drawkb_p this, Drawable d, GC gc, unsigned int width, unsigned int height, XkbDescPtr kbdesc, puticon_t puticon);

int drawkblibs_xlib_init (
	drawkb_create_t *ret_create,
	drawkb_draw_t *ret_draw
);

#endif
