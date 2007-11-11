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

typedef int (*IQF_t)(KeySym keysym, unsigned int state, char buf[], int buf_n);

int drawkb_init(Display *dpy, const char *imagelib, const char *font, IQF_t IQF, float scale);
void drawkb_draw(Display * dpy, Drawable d, GC gc, unsigned int width, unsigned int height, XkbDescPtr kbdesc);

#endif                          /* __HINTS_H */