/*
 * drawkblib.c
 *
 * Copyright (C) 2006, Octavio Alvarez Piza.
 * License: GNU General Public License v2.
 *
 */

#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdint.h>

#include <X11/Xlib.h>

#include "drawkblib.h"
#include "drawkblibs/drawkblibs.h"

#include "drawkblibs/drawkblibs-xlib.h"

#define LIB_PREFIX "/usr/lib/"

#ifndef WITH_DRAWKBLIBS_XLIB
	#define drawkblibs_xlib_init NULL
#endif

drawkblib_compiled_in_t drawkblib_compiled_in[] = {
	{ "xlib",    drawkblibs_xlib_init },
	{ NULL, NULL }
};

void drawkblib_GetValues(char *buf, unsigned long buf_n)
{
	drawkblib_compiled_in_t *p;

	if (buf == NULL) return;

	strcpy(buf, "");

	for (p = &drawkblib_compiled_in[0]; p->code != NULL; p++)
	{
		if (p->initlib) {
			strncat(buf, p->code, buf_n);
			strncat(buf, " ", buf_n);
		}
	}

	DIR *dir;
	struct dirent *entry;

	dir = opendir(LIB_PREFIX "superkb");
	if (dir == NULL)
		return;

	while ((entry = readdir(dir))) {
		if (strncmp(entry->d_name, "drawkblibs-", 8) == 0
			&& strncmp(&entry->d_name[strlen(entry->d_name)-3], ".so", 3) == 0) {
			strncat(buf, entry->d_name+8, strlen(entry->d_name)-11 > buf_n ? buf_n : strlen(entry->d_name)-11);
			strncat(buf, " ", buf_n);
		}
	}
	

}

int Init_drawkblib(const char *userlib)
{
	/* First, find an appropriate drawkblib interface. */
	/* ++ Try 1: Try loading user-specified lib (first compiled in, then
	 *    with dlopen()). */

	drawkblib_compiled_in_t *p;

	for (p = &drawkblib_compiled_in[0]; p->code != NULL; p++)
	{
		if (strcmp(p->code, userlib) == 0 && p->initlib != NULL) {
//			fprintf(stderr, "Before: %p, %p\n", &drawkblib.create, &drawkblib.draw);
//			fprintf(stderr, "Before: %p, %p\n", drawkblib.create, drawkblib.draw);
			fprintf(stderr, "Found an initlib at %p\n", (void *)(intptr_t)p->initlib);
			p->initlib(&drawkblib.create, &drawkblib.draw);
//			fprintf(stderr, "After : %p, %p\n", &drawkblib.create, &drawkblib.draw);
//			fprintf(stderr, "After: %p, %p\n", drawkblib.create, drawkblib.draw);
//			drawkblib.createlib = p->initlib;
			return EXIT_SUCCESS;
		}
	}

	char *fn = malloc(strlen(LIB_PREFIX) + strlen("superkb/drawkblibs-") + strlen(userlib)
		+ strlen(".so") + 1);

	strcpy(fn, LIB_PREFIX);
	strcat(fn, "superkb/drawkblibs-");
	strcat(fn, userlib);
	strcat(fn, ".so");

	void *drawkbdlib = dlopen(fn, RTLD_LAZY);
	if (drawkbdlib) {
		 drawkblib_init_t initlib = (drawkblib_init_t)(intptr_t)dlsym(drawkbdlib, "Init");
		if ((initlib)(&drawkblib.create, &drawkblib.draw) == EXIT_SUCCESS) {
			return EXIT_SUCCESS;
		}
	} else {
		fprintf(stderr, "(superkb: %s)\n\n", dlerror());
	}

	free(fn);

	return EXIT_FAILURE;

}

drawkb_p drawkb_create(Display *dpy, const char *font, IQF_t IQF, painting_mode_t painting_mode, float scale, debug_t *debug) {
	return drawkblib.create(dpy, font, IQF, painting_mode, scale, debug);
}

void drawkb_draw(drawkb_p this, Drawable d, GC gc, unsigned int width, unsigned int height, XkbDescPtr kbdesc, puticon_t puticon) {
	drawkblib.draw(this, d, gc, width, height, kbdesc, puticon);
}
