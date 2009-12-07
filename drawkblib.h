/*
 * drawkblib.h
 *
 * Copyright (C) 2006, Octavio Alvarez Piza.
 * License: GNU General Public License v2.
 *
 */

#ifndef __drawkblib_H
#define __drawkblib_H

#include "drawkblibs/drawkblibs.h"

drawkb_p drawkb_create(Display *dpy, const char *font, IQF_t IQF, painting_mode_t painting_mode, float scale, debug_t *debug);

typedef struct {
	const char *code;
	drawkblib_init_t initlib;
} drawkblib_compiled_in_t, *drawkblib_compiled_in_p;

void drawkb_draw(drawkb_p this, Drawable d, GC gc, unsigned int width, unsigned int height, XkbDescPtr kbdesc, puticon_t puticon);

int Init_drawkblib(const char *userlib);

#endif

