/*
 * imagelib.c
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

#include "imagelib.h"
#include "puticon/puticon.h"

#include "puticon/puticon-gdkpixbuf.h"
#include "puticon/puticon-imlib2.h"

#define LIB_PREFIX "/usr/lib/"

#ifndef WITH_GDKPIXBUF
	#define pi_gdkpixbuf_init NULL
#endif

#ifndef WITH_IMLIB2
	#define pi_imlib2_init NULL
#endif

imagelib_compiled_in_t imagelib_compiled_in[] = {
	{ "gdkpixbuf",    pi_gdkpixbuf_init },
	{ "imlib2", pi_imlib2_init },
	{ NULL, NULL }
};

struct {
	imagelib_init_t InitImage;
	imagelib_newimage_t NewImage;
	imagelib_loadimage_t LoadImage;
	imagelib_resizeimage_t ResizeImage;
	imagelib_drawimage_t PaintImage;
	imagelib_freeimage_t FreeImage;
} image;

void Imagelib_GetValues(char *buf, unsigned long buf_n)
{
	imagelib_compiled_in_t *p;

	if (buf == NULL) return;

	strcpy(buf, "");

	for (p = &imagelib_compiled_in[0]; p->code != NULL; p++)
	{
		if (p->init) {
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
		if (strncmp(entry->d_name, "puticon-", 8) == 0
			&& strncmp(&entry->d_name[strlen(entry->d_name)-3], ".so", 3) == 0) {
			strncat(buf, entry->d_name+8, strlen(entry->d_name)-11 > buf_n ? buf_n : strlen(entry->d_name)-11);
			strncat(buf, " ", buf_n);
		}
	}
	

}

int Init_Imagelib(Display *dpy, const char *userlib)
{
	/* First, find an appropriate imagelib interface. */
	/* ++ Try 1: Try loading user-specified lib (first compiled in, then
	 *    with dlopen()). */

	imagelib_compiled_in_t *p;

	for (p = &imagelib_compiled_in[0]; p->code != NULL; p++)
	{
		if (strcmp(p->code, userlib) == 0 && p->init != NULL) {
			p->init(dpy, &image.NewImage, &image.LoadImage, &image.ResizeImage,
				&image.PaintImage, &image.FreeImage);
			image.InitImage = p->init;
			return EXIT_SUCCESS;
		}
	}

	char *fn = malloc(strlen(LIB_PREFIX) + strlen("superkb/puticon-") + strlen(userlib)
		+ strlen(".so") + 1);

	strcpy(fn, LIB_PREFIX);
	strcat(fn, "superkb/puticon-");
	strcat(fn, userlib);
	strcat(fn, ".so");

	void *imagelib = dlopen(fn, RTLD_LAZY);
	if (imagelib) {
//		image.NewImage = dlsym(imagelib, "NewImage");
		image.InitImage = (imagelib_init_t)(intptr_t) dlsym(imagelib, "Init");
//		image.LoadImage = dlsym(imagelib, "LoadImage");
//		image.ResizeImage = dlsym(imagelib, "ResizeImage");
//		image.PaintImage = dlsym(imagelib, "PaintImage");
//		image.FreeImage = dlsym(imagelib, "FreeImage");
		if ((image.InitImage)(dpy, &image.NewImage, &image.LoadImage,
				&image.ResizeImage,	&image.PaintImage, &image.FreeImage) == EXIT_SUCCESS) {
			return EXIT_SUCCESS;
		}
	} else {
		fprintf(stderr, "(superkb: %s)\n\n", dlerror());
	}

	free(fn);

	/* ++ Try 2: Query X for WM and use try the best for it. */

	/* It's not ready yet, though.

	Atom actual_type;
	int actual_format;
	unsigned long nitems;
	unsigned long bytes_after;
	Window *prop;
	unsigned char *s=NULL;
	
	Window root_win = DefaultRootWindow(dpy);

	Atom atom_net_supp;

	atom_net_supp = XInternAtom(dpy, "_NET_SUPPORTING_WM_CHECK", False);

	XGetWindowProperty(dpy, root_win, atom_net_supp, 0, 128, False,
	AnyPropertyType, &actual_type, &actual_format, &nitems, &bytes_after,
	(unsigned char **) &prop);

	atom_net_supp = XInternAtom(dpy, "_NET_WM_NAME", False);

	XGetWindowProperty(dpy, *prop, atom_net_supp, 0, 128, False,
	AnyPropertyType, &actual_type, &actual_format, &nitems, &bytes_after,
	 (unsigned char **) &s);

	int i = 0;
	void *hintinit;
	int hintinit_success;

	while ((hintinit = hints[i][HINTINIT]) != NULL) {
		hintinit_success = hintinit(dpy);

		if (hintinit_success == EXIT_SUCCESS)
		  break;

		i++;
	}

	if (hintinit == NULL)
		return EXIT_FAILURE;

	hintwork = hints[i][HINTWORK];

	*/

	/* Try 3 should be checking for available image libs like gdk-pixbuf,
	 * imlib2, etc., but not libpng, libtiff... */

	/* Try 4 should be checking for available format libs like libpng,
	 * libjpeg, etc., and use those. */

	/* At the end, I _think_ xpm should be at least availble, so we will
	 * always return EXIT_SUCCESS. */

   /* However, we don't know how to handle xpm files, so we return
	 * EXIT_FAILURE until this is fixed. */

	return EXIT_FAILURE;

}

imagelib_image_t * NewImage()
{
	return image.NewImage();
}

int LoadImage(imagelib_image_t *this, const char *fn)
{
	return image.LoadImage(this, fn);
}

void ResizeImage(imagelib_image_t *this, int width, int height)
{
	image.ResizeImage(this, width, height);
}

void DrawImage(imagelib_image_t *this, Drawable d, int x, int y)
{
	image.PaintImage(this, d, x, y);
}

void FreeImage(imagelib_image_t *this)
{
	image.FreeImage(this);
}
