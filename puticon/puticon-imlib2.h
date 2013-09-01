/*
 * puticon-imlib2.h
 *
 * Copyright (C) 2006, Octavio Alvarez Piza.
 * License: GNU General Public License v2.
 *
 * This is the actual implementation of puticon by using imblib2 to render a
 * an icon into an X11 drawable.
 *
 * This code is not by drawkblib-xlib directly, but by imagelib.c This allows
 * imagelib.c to provide multiple options to image rendering.
 *
 * See drawkblibs/drawkblib-xlib.h and imagelib.h for details.
 *
 */

#ifndef __PUTICON_IMLIB2_H
#define __PUTICON_IMLIB2_H

int pi_imlib2_newimage(imagelib_image_t *this);
int pi_imlib2_loadimage(imagelib_image_t *this, const char *file);
int pi_imlib2_resizeimage(imagelib_image_t *this, int width, int height);
void pi_imlib2_paintimage(imagelib_image_t *this, Drawable d, int left, int top);
void pi_imlib2_freeimage(imagelib_image_t *this);

int pi_imlib2_init(Display *dpy,
	imagelib_newimage_t *ret_init,
	imagelib_loadimage_t *ret_load,
	imagelib_resizeimage_t *ret_resize,
	imagelib_drawimage_t *ret_draw,
	imagelib_freeimage_t *free);

#endif
