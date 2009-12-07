/*
 * screeninfo.h
 *
 * Copyright (C) 2008, Octavio Alvarez Piza.
 * License: GNU General Public License v2.
 *
 */

#ifndef __SCREENINFO_H
#define __SCREENINFO_H

typedef struct {
   int   screen_number;
   short x_org;
   short y_org;
   short width;
   short height;
} screeninfo_t;

void screeninfo_get_screens(Display *dpy, screeninfo_t **xinerama_screens, int *screens_n);

#endif
