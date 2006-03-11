/* License: GPL v2. */

/* This functions are called by drawkb.c to draw icons on the screen. */

#ifndef __PUTICON_GDK_PIXBUF_XLIB_H
#define __PUTICON_GDK_PIXBUF_XLIB_H

void LoadAndDrawImage(Display *dpy, GC gc, Window w, int left, int top, int width, int height, char *buf, int buf_n);

#endif
