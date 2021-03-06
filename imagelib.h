/*
 * imagelib.h
 *
 * Copyright (C) 2006, Octavio Alvarez Piza.
 * License: GNU General Public License v2.
 *
 * This code provides drawkblib-xlib with a way to render icons on the
 * drawable it asks to. This allows it to draw the icons on bound keys.
 *
 * Multiple different codes may provide the functionality, and are located
 * under puticon/ as are puticon-imlib2 and puticon-gdkpixbuf.
 *
 */

#ifndef __IMAGELIB_H
#define __IMAGELIB_H

#include <X11/Xlib.h>

#include "puticon/puticon.h"

typedef struct {
	const char *code;
	imagelib_init_t init;
} imagelib_compiled_in_t;

void Imagelib_GetValues(char *buf, unsigned long buf_n);
imagelib_image_t * NewImage();
int  LoadImage(imagelib_image_t *this, const char *fn);
void ResizeImage(imagelib_image_t *this, int width, int height);
void DrawImage(imagelib_image_t *this, Drawable d, int left, int top);
void FreeImage(imagelib_image_t *this);

int Init_Imagelib(Display *dpy, const char *userlib);

#endif
