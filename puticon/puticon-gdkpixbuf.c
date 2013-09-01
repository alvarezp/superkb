/*
 * puticon-gdkpixbuf.c
 *
 * Copyright (C) 2006, Octavio Alvarez Piza.
 * License: GNU General Public License v2.
 *
 */

#include <stdlib.h>

#include <gdk-pixbuf-xlib/gdk-pixbuf-xlib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib.h>

#include "puticon.h"

#ifndef WITH_GDKPIXBUF
#define pi_gdkpixbuf_init Init
#endif

imagelib_image_t *pi_gdkpixbuf_newimage()
{
	imagelib_image_t *this = malloc(sizeof(imagelib_image_t));

	if (this)
		return this;
	else
		return NULL;
}

int pi_gdkpixbuf_loadimage(imagelib_image_t *this, const char *file)
{

	this->original = gdk_pixbuf_new_from_file (file, NULL);
	if (this->original)
		return EXIT_SUCCESS;
	else
		return EXIT_FAILURE;

}

int pi_gdkpixbuf_resizeimage(imagelib_image_t *this, int width, int height)
{

	this->scaled = gdk_pixbuf_scale_simple(this->original, width, height, GDK_INTERP_BILINEAR);

	if (this->scaled)
		return EXIT_SUCCESS;
	else
		return EXIT_FAILURE;

}

void pi_gdkpixbuf_paintimage(imagelib_image_t *this, Drawable d, int left, int top)
{
	gdk_pixbuf_xlib_render_to_drawable_alpha(this->scaled, d, 0, 0, left, top, gdk_pixbuf_get_width(this->scaled), gdk_pixbuf_get_width(this->scaled), GDK_PIXBUF_ALPHA_FULL, 255, XLIB_RGB_DITHER_NORMAL, 0, 0);
}

void pi_gdkpixbuf_freeimage(imagelib_image_t *this)
{
	g_object_unref(this->original);
	g_object_unref(this->scaled);
	free(this);
}

int pi_gdkpixbuf_init(Display *dpy,
	imagelib_newimage_t *ret_new,
	imagelib_loadimage_t *ret_load,
	imagelib_resizeimage_t *ret_resize,
	imagelib_drawimage_t *ret_draw,
	imagelib_freeimage_t *ret_free)
{
	/* Neither does gdk_pixbuf_xlib_init(). */
	gdk_pixbuf_xlib_init(dpy, 0);

	*ret_new = pi_gdkpixbuf_newimage;
	*ret_load = pi_gdkpixbuf_loadimage;
	*ret_resize = pi_gdkpixbuf_resizeimage;
	*ret_draw = pi_gdkpixbuf_paintimage;
	*ret_free = pi_gdkpixbuf_freeimage;

	return EXIT_SUCCESS;
}

