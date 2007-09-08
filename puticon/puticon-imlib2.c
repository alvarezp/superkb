/*
 * puticon-imlib2.c
 *
 * Copyright (C) 2006, Octavio Alvarez Piza.
 * License: GNU General Public License v2.
 *
 */

#include <stdlib.h>

#include <Imlib2.h>

#include "puticon.h"

#ifndef WITH_IMLIB2
#define pi_imlib2_init Init
#endif

int pi_imlib2_scaled_width;
int pi_imlib2_scaled_height;

imagelib_image_t *pi_imlib2_newimage()
{
	imagelib_image_t *this = malloc(sizeof(imagelib_image_t));

	if (this)
		return this;
	else
		return NULL;
}

int pi_imlib2_loadimage(imagelib_image_t *this, const char *file)
{

	this->original = imlib_load_image(file);

	if (this->original)
		return EXIT_SUCCESS;
	else
		return EXIT_FAILURE;

}

int pi_imlib2_resizeimage(imagelib_image_t *this, int width, int height)
{

	this->scaled = imlib_create_image(width, height);
	if (!this->scaled)
		return EXIT_FAILURE;

//	imlib_context_set_image(this->scaled);
//	imlib_context_set_color(0, 0, 255, 0);
//	imlib_image_fill_rectangle(0, 0, width, height);

//	imlib_blend_image_onto_image(this->original, 0, 0, 0, imlib_image_get_width(), imlib_image_get_height(), 0, 0, width, height);

	pi_imlib2_scaled_width = width;
	pi_imlib2_scaled_height = height;

	return EXIT_SUCCESS;	

}

void pi_imlib2_paintimage(imagelib_image_t *this, Drawable d, int left, int top)
{

//	imlib_context_set_image(this->scaled);
//	imlib_context_set_drawable(d);
//	imlib_render_image_on_drawable_at_size(left, top, imlib_image_get_width(), imlib_image_get_height());

	imlib_context_set_image(this->original);
	imlib_context_set_drawable(d);
	imlib_render_image_on_drawable_at_size(left, top, pi_imlib2_scaled_width, pi_imlib2_scaled_height);
}

void pi_imlib2_freeimage(imagelib_image_t *this)
{
	imlib_context_set_image(this->original);
	imlib_free_image();

	imlib_context_set_image(this->scaled);
	imlib_free_image();
	free(this);
}

int pi_imlib2_init(Display *dpy,
	imagelib_newimage_t *ret_new,
	imagelib_loadimage_t *ret_load,
	imagelib_resizeimage_t *ret_resize,
	imagelib_drawimage_t *ret_draw,
	imagelib_freeimage_t *ret_free)
{
	imlib_context_set_display(dpy);
	imlib_context_set_visual(DefaultVisual(dpy, 0));
	imlib_context_set_colormap(DefaultColormap(dpy, 0));

	*ret_new = pi_imlib2_newimage;
	*ret_load = pi_imlib2_loadimage;
	*ret_resize = pi_imlib2_resizeimage;
	*ret_draw = pi_imlib2_paintimage;
	*ret_free = pi_imlib2_freeimage;

	return EXIT_SUCCESS;
}

