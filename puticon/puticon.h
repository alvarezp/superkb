/*
 * puticon.h
 *
 * Copyright (C) 2006, Octavio Alvarez Piza.
 * License: GNU General Public License v2.
 *
 */

#ifndef __PUTICON_H
#define __PUTICON_H

#include <X11/Xlib.h>

typedef struct {
	void *original;
	void *scaled;
} imagelib_image_t;

typedef imagelib_image_t * (*imagelib_newimage_t)();
typedef int  (*imagelib_loadimage_t)(imagelib_image_t *this, const char *file);
typedef int  (*imagelib_resizeimage_t)(imagelib_image_t *this, int width, int height);
typedef void (*imagelib_drawimage_t)(imagelib_image_t *this, Drawable w, int left, int top);
typedef void (*imagelib_freeimage_t)(imagelib_image_t *this);


typedef int  (*imagelib_init_t)(Display *dpy,
	imagelib_newimage_t *ret_init,
	imagelib_loadimage_t *ret_load,
	imagelib_resizeimage_t *ret_resize,
	imagelib_drawimage_t *ret_draw,
	imagelib_freeimage_t *free);


#endif /* __PUTICON_H */
