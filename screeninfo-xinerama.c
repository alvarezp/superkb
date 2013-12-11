/*
 * screeninfo-xinerama.c
 *
 * Copyright (C) 2008, Octavio Alvarez Piza.
 * License: GNU General Public License v2.
 *
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <X11/extensions/Xinerama.h>
#include <X11/Xlib.h>

#include "debug.h"

#include "screeninfo.h"

void screeninfo_get_screens(Display *dpy, screeninfo_t **screens, int *screens_n) {

	XineramaScreenInfo *xinerama_screens=NULL;
	int xinerama_screens_n=0;

	Bool xinerama_queryextension_result;	
	Status xinerama_queryversion_result;
	Bool xinerama_is_active;

	int dummyint;
	xinerama_queryextension_result = XineramaQueryExtension (dpy, &dummyint, &dummyint);
	xinerama_queryversion_result = XineramaQueryVersion (dpy, &dummyint, &dummyint);
	xinerama_is_active = XineramaIsActive(dpy);

	if (xinerama_queryextension_result != True
		|| xinerama_queryversion_result == 0
		|| xinerama_is_active != True)
	{
		fprintf(stderr, "superkb: Xinerama extension not available! Falling back.\n");
		*screens = (screeninfo_t *) malloc(sizeof(screeninfo_t));
		*screens_n = 1;

		screens[0]->screen_number = 0;
		screens[0]->x_org = 0;
		screens[0]->y_org = 0;
		screens[0]->width = DisplayWidth(dpy, DefaultScreen(dpy));
		screens[0]->height = DisplayHeight(dpy, DefaultScreen(dpy));

		return;
	}

	xinerama_screens = XineramaQueryScreens (dpy, &xinerama_screens_n);

	if (xinerama_screens_n == 0) {
		fprintf(stderr, "superkb: Xinerama extension available, but query failed! Falling back.\n");
		*screens = (screeninfo_t *) malloc(sizeof(screeninfo_t));
		*screens_n = 1;

		screens[0]->screen_number = 0;
		screens[0]->x_org = 0;
		screens[0]->y_org = 0;
		screens[0]->width = DisplayWidth(dpy, DefaultScreen(dpy));
		screens[0]->height = DisplayHeight(dpy, DefaultScreen(dpy));

		return;
	}

	/* Everything went well */

	debug(3, "[xs] Xinerama detected %d screens.\n", xinerama_screens_n);
	int i;
	for (i=0; i < xinerama_screens_n; i++) {
#define xsi ((xinerama_screens)[i])
		debug (4, "[xs] Screen #%d: number=%d, x=%d, y=%d, w=%d, h=%d\n",
			i, xsi.screen_number, xsi.x_org, xsi.y_org, xsi.width, xsi.height);
#undef xsi
	}

	/* We should dump all information from Xinerama to screeninfo. However,
	 * given the struct is the same (intentionally) we are going to simply
	 * memcpy() it. If the structure ever changes, this code should be
	 * definitely updated.
	 */
	*screens = malloc(sizeof(screeninfo_t)*xinerama_screens_n);
	memcpy(*screens, xinerama_screens, sizeof(screeninfo_t)*xinerama_screens_n);
	*screens_n = xinerama_screens_n;

}


