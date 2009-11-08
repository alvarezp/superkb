/*
 * imagelib.h
 *
 * Copyright (C) 2006, Octavio Alvarez Piza.
 * License: GNU General Public License v2.
 *
 */

#ifndef __IMAGELIB_H
#define __IMAGELIB_H

#include <X11/Xlib.h>

#include "puticon/puticon.h"

typedef struct {
	char *code;
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
