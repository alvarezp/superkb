/*
 * xinerama-support.c
 *
 * Copyright (C) 2008, Octavio Alvarez Piza.
 * License: GNU General Public License v2.
 *
 */

#include <malloc.h>

#include <X11/Xlib.h>

/* Conditionally includes X11/extensions/Xinerama.h */
#include "xinerama-support.h"

#include "debug.h"

void emulate_xinerama(Display *dpy, XineramaScreenInfo **xinerama_screens, int *xinerama_screens_n) {

	*xinerama_screens = (XineramaScreenInfo *) malloc(sizeof(XineramaScreenInfo));
	*xinerama_screens_n = 1;

	xinerama_screens[0]->screen_number = 0;
	xinerama_screens[0]->x_org = 0;
	xinerama_screens[0]->y_org = 0;
	xinerama_screens[0]->width = DisplayWidth(dpy, DefaultScreen(dpy));
	xinerama_screens[0]->height = DisplayHeight(dpy, DefaultScreen(dpy));

}

void get_xinerama_screens(Display *dpy, XineramaScreenInfo **xinerama_screens, int *xinerama_screens_n) {

#ifndef WITHOUT_XINERAMA

	Bool xinerama_queryextension_result;	
	Status xinerama_queryversion_result;
	Bool xinerama_is_active;

	int dummyint;
	xinerama_queryextension_result = XineramaQueryExtension (dpy, &dummyint, &dummyint);
	xinerama_queryversion_result = XineramaQueryVersion (dpy, &dummyint, &dummyint);
	xinerama_is_active = XineramaIsActive(dpy);

	*xinerama_screens = XineramaQueryScreens (dpy, xinerama_screens_n);

	if (xinerama_queryextension_result == True
		&& xinerama_queryversion_result != 0
		&& xinerama_is_active == True
		&& xinerama_screens_n) {

		/* Everything went well */

		debug(3, "[xs] Xinerama detected %d screens.\n", *xinerama_screens_n);
		int i;
		for (i=0; i < *xinerama_screens_n; i++) {
#define xsi ((*xinerama_screens)[i])
			debug (4, "[xs] Screen #%d: number=%d, x=%d, y=%d, w=%d, h=%d\n",
				i, xsi.screen_number, xsi.x_org, xsi.y_org, xsi.width, xsi.height);
#undef xsi
		}

	} else {
		emulate_xinerama(dpy, xinerama_screens, xinerama_screens_n);
	}

#else

	emulate_xinerama(dpy, xinerama_screens, xinerama_screens_n);

#endif

}


