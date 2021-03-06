/*
 * puticon-gdkpixbuf.h
 *
 * Copyright (C) 2006, Octavio Alvarez Piza.
 * License: GNU General Public License v2.
 *
 * This is the actual implementation of puticon by using gdkpixbuf to render a
 * an icon into an X11 drawable.
 *
 * This code is not by drawkblib-xlib directly, but by imagelib.c This allows
 * imagelib.c to provide multiple options to image rendering.
 *
 * See drawkblibs/drawkblib-xlib.h and imagelib.h for details.
 *
 */

#ifndef __PUTICON_GDKPIXBUF_PIXBUF_XLIB_H
#define __PUTICON_GDKPIXBUF_PIXBUF_XLIB_H

int pi_gdkpixbuf_newimage(imagelib_image_t *this);
int pi_gdkpixbuf_loadimage(imagelib_image_t *this, const char *file);
int pi_gdkpixbuf_resizeimage(imagelib_image_t *this, int width, int height);
void pi_gdkpixbuf_paintimage(imagelib_image_t *this, Drawable d, int left, int top);
void pi_gdkpixbuf_freeimage(imagelib_image_t *this);

int pi_gdkpixbuf_init(Display *dpy,
	imagelib_newimage_t *ret_init,
	imagelib_loadimage_t *ret_load,
	imagelib_resizeimage_t *ret_resize,
	imagelib_drawimage_t *ret_draw,
	imagelib_freeimage_t *free);

#endif
