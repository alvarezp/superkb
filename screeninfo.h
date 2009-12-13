/*
 * screeninfo.h
 *
 * Copyright (C) 2008, Octavio Alvarez Piza.
 * License: GNU General Public License v2.
 *
 * This code provides Superkb with a way to get information about the one
 * or many monitors available on the system, in order to draw one keyboard
 * per monitor instead of one big keyboard among all monitors.
 *
 * It does so by providing a function.
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
