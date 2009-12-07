/*
 * screeninfo-xlib.c
 *
 * Copyright (C) 2008, Octavio Alvarez Piza.
 * License: GNU General Public License v2.
 *
 */

#include <malloc.h>

#include <X11/Xlib.h>

#include "debug.h"

#include "screeninfo.h"

void screeninfo_get_screens(Display *dpy, screeninfo_t **screens, int *screens_n) {
	*screens = (screeninfo_t *) malloc(sizeof(screeninfo_t));
	*screens_n = 1;

	screens[0]->screen_number = 0;
	screens[0]->x_org = 0;
	screens[0]->y_org = 0;
	screens[0]->width = DisplayWidth(dpy, DefaultScreen(dpy));
	screens[0]->height = DisplayHeight(dpy, DefaultScreen(dpy));
}


