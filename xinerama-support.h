/*
 * xinerama-support.h
 *
 * Copyright (C) 2008, Octavio Alvarez Piza.
 * License: GNU General Public License v2.
 *
 */

#ifndef __XINERAMA_SUPPORT_H
#define __XINERAMA_SUPPORT_H

#ifndef WITHOUT_XINERAMA

#include <X11/extensions/Xinerama.h>
#include <X11/Xlib.h>

#else

typedef struct {
   int   screen_number;
   short x_org;
   short y_org;
   short width;
   short height;
} XineramaScreenInfo;

#endif

void get_xinerama_screens(Display *dpy, XineramaScreenInfo **xinerama_screens, int *xinerama_screens_n);

#endif

