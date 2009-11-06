/*
 * drawkblibs-xlib.h
 *
 * Copyright (C) 2006, Octavio Alvarez Piza.
 * License: GNU General Public License v2.
 *
 */

#ifndef __DRAWKBLIBS_XLIB_H
#define __DRAWKBLIBS_XLIB_H

#include "drawkblibs.h"

int drawkb_xlib_create(Display *dpy, const char *imagelib, const char *font, IQF_t IQF, painting_mode_t painting_mode, float scale);
void drawkb_xlib_draw(drawkb_p this, Drawable d, GC gc, unsigned int width, unsigned int height, XkbDescPtr kbdesc, puticon_t puticon);

int drawkblibs_xlib_init (
	drawkb_create_t *ret_create,
	drawkb_draw_t *ret_draw
);

#endif
