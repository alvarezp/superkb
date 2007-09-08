/*
 * main.c
 *
 * Copyright (C) 2006, Octavio Alvarez Piza.
 * License: GNU General Public License v2.
 *
 */

/* Thanks to Natan "Whatah" Zohar for helping with tokenizer. */

#include <X11/Xlib.h>

#include <X11/XKBlib.h>
#include <X11/extensions/XKBgeom.h>

#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <signal.h>
#include <stdarg.h>

#include "superkb.h"
#include "drawkb.h"
#include "superkbrc.h"

#define VERSION "0.10"

struct sigaction action;

Window kbwin;
Pixmap kbwin_backup;
GC kbwin_gc;

config_t *config;

double scale;

Window prev_kbwin_focus;
int prev_kbwin_revert;

Display *dpy;

XkbDescPtr kbdesc;
XkbGeometryPtr kbgeom;

XColor background;
XColor foreground;

int winv;
int winh;

int fatal_error(const char * format, ...) {
	va_list args;
	va_start (args, format);
	fprintf(stderr, format, args);
	abort();
	return 0;
}

int IconQuery(KeySym keysym, unsigned int state, char buf[], int buf_n)
{
	int i;
	for (i = 0; i < config->key_bindings_n; i++)
	{
		if (keysym == XKeycodeToKeysym(dpy, config->key_bindings[i].keycode, 0)) {
			if (config->key_bindings[i].icon != NULL) {
				strncpy(buf, config->key_bindings[i].icon, buf_n);
			} else {
				strncpy(buf, "", buf_n);
			}
			return EXIT_SUCCESS;
		}
	}
	return EXIT_FAILURE;
}

void kbwin_event(Display * dpy, XEvent ev)
{

	if (ev.type == Expose) {
/*		drawkb_draw(dpy, kbwin, kbwin_gc, DisplayWidth(dpy, 0), DisplayHeight(dpy, 0), kbdesc);*/
		XCopyArea(dpy, kbwin_backup, kbwin, kbwin_gc, 0, 0, winh, winv, 0, 0);
		XFlush(dpy);
	} else if (ev.type == VisibilityNotify &&
			   ev.xvisibility.state != VisibilityUnobscured) {
		XRaiseWindow(dpy, kbwin);
	}

}

void kbwin_map(Display * dpy)
{
	/* XGetInputFocus(dpy, &prev_kbwin_focus, &prev_kbwin_revert); */
	XMapWindow(dpy, kbwin);
}

void kbwin_unmap(Display * dpy)
{
	XUnmapWindow(dpy, kbwin);
	/* XSetInputFocus(dpy, prev_kbwin_focus, prev_kbwin_revert, CurrentTime); */
}

int kbwin_init(Display * dpy)
{
	int r;

	winv = DisplayHeight(dpy, 0);
	winh = DisplayWidth(dpy, 0);

	/* Initialize Window and pixmap to back it up. */

	XColor background;
	XColor foreground;

	background.red = config->backcolor.red;
	background.green = config->backcolor.green;
	background.blue = config->backcolor.blue;

	foreground.red = config->forecolor.red;
	foreground.green = config->forecolor.green;
	foreground.blue = config->forecolor.blue;

	XAllocColor(dpy, XDefaultColormap(dpy, 0), &background);
	XAllocColor(dpy, XDefaultColormap(dpy, 0), &foreground);

	/* Get what scale should drawkb work with, according to drawable's
	 * width and height. */
	double scalew = (float) winh / kbgeom->width_mm;
	double scaleh = (float) winv / kbgeom->height_mm;

	/* Work with the smallest scale. */
	if (scalew < scaleh) {
		scale = scalew;
	} else { /* scalew >= scaleh */
		scale = scaleh;
	}
	
	winv = kbgeom->height_mm * scale;
	winh = kbgeom->width_mm * scale;

	/* Create window. */
	kbwin = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy),
							(DisplayWidth(dpy, 0) - winh) / 2,
							(DisplayHeight(dpy, 0) - winv) / 2, winh, winv,
							0, 0, (background.pixel));

	kbwin_gc = XCreateGC(dpy, kbwin, 0, NULL);

	XSetForeground(dpy, kbwin_gc, foreground.pixel);
	XSetBackground(dpy, kbwin_gc, background.pixel);

	XSetWindowAttributes attr;
	attr.override_redirect = True;

	XChangeWindowAttributes(dpy, kbwin, CWOverrideRedirect, &attr);

	XSetTransientForHint(dpy, kbwin, DefaultRootWindow(dpy));

	XSelectInput(dpy, kbwin, ExposureMask | VisibilityChangeMask);

	XFlush(dpy);

	r = drawkb_init(dpy, config->drawkb_imagelib, config->drawkb_font, IconQuery, scale);

	if (r != EXIT_SUCCESS) {
		return EXIT_FAILURE;
	}

	kbwin_backup = XCreatePixmap(dpy, kbwin, winh, winv, DefaultDepth(dpy, DefaultScreen(dpy)));

	XSetForeground(dpy, kbwin_gc, background.pixel);

	XFillRectangle(dpy, kbwin_backup, kbwin_gc, 0, 0, winh, winv);

	XSetForeground(dpy, kbwin_gc, foreground.pixel);
	XSetBackground(dpy, kbwin_gc, background.pixel);

	drawkb_draw(dpy, kbwin_backup, kbwin_gc, winh, winv, kbdesc);

	if (r == EXIT_FAILURE)
		fprintf(stderr, "superkb: Failed to initialize drawkb. Quitting.\n");


	return r;
}

void __Superkb_Action(KeyCode keycode, unsigned int state)
{
	int i;
	for (i = 0; i < config->key_bindings_n; i++) {
		if (config->key_bindings[i].keycode == keycode &&
		  config->key_bindings[i].state ==
		  (state & config->key_bindings[i].state)) {
			switch (config->key_bindings[i].action_type) {
			case AT_COMMAND:
				if (fork() == 0) {
					system(config->key_bindings[i].action.command);
					exit(EXIT_SUCCESS);
				}
				break;
			case AT_FUNCTION:
				if (fork() == 0) {
					/* FIXME: Value should not be NULL. */
					config->key_bindings[i].action.function(NULL);
					exit(EXIT_SUCCESS);
				}

			}
		}
	}
}

void sighandler(int sig)
{
	switch (sig) {
	case SIGUSR1:
		break;
	}
}

int main()
{

	int status;

	printf("\nsuperkb " VERSION ": Welcome. This program is under development.\n\n"
		"It's strongly recommended to set the following on xorg.conf:\n\n"
		"| Section \"ServerFlags\"\n"
		"|   Option \"AllowDeactivateGrabs\" \"On\"\n"
		"|   Option \"AllowClosedownGrabs\" \"On\"\n"
		"| EndSection\n\n"
		"With these, if the program fails while drawing the keyboard, you "
			" will be able\n"
		"to kill it by pressing Ctrl-Alt-*, and restore Autorepeat with "
			"'xset r on'.\n\n"
	);

	/* Set SIGUSR1 handler. */
	action.sa_handler = sighandler;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;
	if (sigaction(SIGUSR1, &action, NULL) != 0) {
		fprintf(stderr, "superkb: Error setting SIGUSR1 signal handler. "
			"Quitting.\n");
		return EXIT_FAILURE;
	}

	/* Needed to avoid zombie child processes. According to
     * http://www.faqs.org/faqs/unix-faq/faq/part3/section-13.html
     * this is not portable.
     */
	signal(SIGCHLD, SIG_IGN);

	/* Connect to display. */
	dpy = XOpenDisplay(NULL);
	if (dpy == NULL) {
		fprintf(stderr, "superkb: Couldn't open display. Quitting.\n");
		return EXIT_FAILURE;
	}

	config = config_new(dpy);
	config_load(config, dpy);

	/* Init XKB extension. */
	status = XkbQueryExtension(dpy, NULL, NULL, NULL, NULL, NULL);
	if (status == False) {
		fprintf(stderr, "superkb: Couldn't initialize XKB extension. "
			"Quitting.\n");
		return EXIT_FAILURE;
	}

	kbdesc = XkbGetKeyboard(dpy, XkbAllComponentsMask, XkbUseCoreKbd);
	if (kbdesc == NULL) {
		fprintf(stderr, "superkb: Could not load keyboard information from "
			"X. Quitting.\n");
		return EXIT_FAILURE;
	}

	status = XkbGetGeometry(dpy, kbdesc);
	kbgeom = kbdesc->geom;
	if (status != Success && kbgeom != NULL) {
		fprintf(stderr, "superkb: Could not load keyboard geometry "
			"information. Quitting.\n");
		return EXIT_FAILURE;
	}

	status = superkb_init(dpy, kbwin_init, kbwin_map, kbwin_unmap,
		kbwin_event, "en", config->superkb_super1,
		config->superkb_super2, config->drawkb_delay, __Superkb_Action);

	if (status != EXIT_SUCCESS) {
		return EXIT_FAILURE;
	}

	superkb_start();

	return EXIT_SUCCESS;
}
