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
#include "globals.h"
#include "debug.h"
#include "version.h"

/* Conditionally includes X11/extensions/Xinerama.h */
#include "xinerama-support.h"

#define WINH(i) (kbgeom->width_mm * scale[i])
#define WINV(i) (kbgeom->width_mm * scale[i])

struct sigaction action;

XineramaScreenInfo *xinerama_screens=NULL;
int xinerama_screens_n=0;
Window *kbwin=NULL;
Pixmap *kbwin_backup=NULL;
GC *kbwin_gc=NULL;

config_t *config;

double *scale;

Window prev_kbwin_focus;
int prev_kbwin_revert;

Display *dpy;

XkbDescPtr kbdesc;
XkbGeometryPtr kbgeom;

XColor background;
XColor foreground;

int fatal_error(const char * format, ...) {
	va_list args;
	va_start (args, format);
	fprintf(stderr, format, args);
	abort();
	return 0;
}

int IconQuery(KeySym keysym, unsigned int state, char buf[], int buf_n)
{
	unsigned int i;
	for (i = 0; i < config->key_bindings_n; i++)
	{
		if (keysym == XKeycodeToKeysym(dpy, config->key_bindings[i].keycode, 0) &&
			state == config->key_bindings[i].state) {
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

	int i;
	if (ev.type == Expose) {
/*		drawkb_draw(dpy, kbwin[i], kbwin_gc[i], DisplayWidth(dpy, 0), DisplayHeight(dpy, 0), kbdesc);*/
		for (i=0; i < xinerama_screens_n; i++) {
			XCopyArea(dpy, kbwin_backup[i], kbwin[i], kbwin_gc[i], 0, 0, WINH(i), WINV(i), 0, 0);
		}
		XFlush(dpy);
	} else if (ev.type == VisibilityNotify &&
			   ev.xvisibility.state != VisibilityUnobscured) {
		for (i=0; i < xinerama_screens_n; i++) {
			XRaiseWindow(dpy, kbwin[i]);
		}
	}

}

void kbwin_map(Display * dpy)
{
	/* XGetInputFocus(dpy, &prev_kbwin_focus, &prev_kbwin_revert); */
	int i;
	for (i=0; i < xinerama_screens_n; i++) {
		XSetTransientForHint(dpy, kbwin[i], DefaultRootWindow(dpy));
		XMapWindow(dpy, kbwin[i]);
	}
}

void kbwin_unmap(Display * dpy)
{
	int i;
	for (i=0; i < xinerama_screens_n; i++) {
		XUnmapWindow(dpy, kbwin[i]);
	}
	/* XSetInputFocus(dpy, prev_kbwin_focus, prev_kbwin_revert, CurrentTime); */
}

int kbwin_init(Display * dpy)
{
	int r;

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

	XSetWindowAttributes attr;
	attr.override_redirect = True;

	/* Create one windows per Xinerama screen. */

	XineramaScreenInfo *xsi;

	int i;
	kbwin = malloc(xinerama_screens_n * sizeof(Window));
	kbwin_backup = malloc(xinerama_screens_n * sizeof(Pixmap));
	scale = malloc(xinerama_screens_n * sizeof(double));
	kbwin_gc = malloc(xinerama_screens_n * sizeof(GC));
	for (i = 0; i < xinerama_screens_n; i++) {

		int winv;
		int winh;

		/* Just as little shortcut. */
		xsi = &xinerama_screens[i];

		debug (3, "Preparing screen #%d: %d, %d, %d, %d\n", i, xsi->x_org, xsi->y_org, xsi->width, xsi->height);

		/* Get what scale should drawkb work with, according to drawable's
		 * width and height. */
		winv = xsi->height;
		winh = xsi->width;

		double scalew = (float) winh / kbgeom->width_mm;
		double scaleh = (float) winv / kbgeom->height_mm;

		/* Work with the smallest scale. */
		if (scalew < scaleh) {
			scale[i] = scalew;
		} else { /* scalew >= scaleh */
			scale[i] = scaleh;
		}
		
		winv = kbgeom->height_mm * scale[i];
		winh = kbgeom->width_mm * scale[i];

		kbwin[i] = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy),
								(xsi->width - winh) / 2 + xsi->x_org,
								(xsi->height - winv) / 2 + xsi->y_org, winh, winv,
								0, 0, (background.pixel));

		kbwin_gc[i] = XCreateGC(dpy, kbwin[i], 0, NULL);

		XSetForeground(dpy, kbwin_gc[i], foreground.pixel);
		XSetBackground(dpy, kbwin_gc[i], background.pixel);

		XChangeWindowAttributes(dpy, kbwin[i], CWOverrideRedirect, &attr);

		XSelectInput(dpy, kbwin[i], ExposureMask | VisibilityChangeMask);

		r = drawkb_init(dpy, config->drawkb_imagelib, config->drawkb_font, IconQuery, config->drawkb_painting_mode, scale[i]);

		if (r != EXIT_SUCCESS) {
			return EXIT_FAILURE;
		}

		kbwin_backup[i] = XCreatePixmap(dpy, kbwin[i], winh, winv, DefaultDepth(dpy, DefaultScreen(dpy)));

		XSetForeground(dpy, kbwin_gc[i], background.pixel);

		XFillRectangle(dpy, kbwin_backup[i], kbwin_gc[i], 0, 0, winh, winv);

		XSetForeground(dpy, kbwin_gc[i], foreground.pixel);
		XSetBackground(dpy, kbwin_gc[i], background.pixel);

		drawkb_draw(dpy, kbwin_backup[i], kbwin_gc[i], winh, winv, kbdesc);

	}

	XFlush(dpy);


	if (r == EXIT_FAILURE)
		fprintf(stderr, "superkb: Failed to initialize drawkb. Quitting.\n");


	return r;
}

void __Superkb_Action(KeyCode keycode, unsigned int state)
{
	unsigned int i;
	for (i = 0; i < config->key_bindings_n; i++) {
		if (config->key_bindings[i].keycode == keycode &&
		  config->key_bindings[i].state ==
		  (state & config->key_bindings[i].state)) {
			switch (config->key_bindings[i].action_type) {
			case AT_COMMAND:
				if (fork() == 0) {
					if (config->key_bindings[i].feedback_string) {
						char *cmdline = malloc(strlen(config->feedback_handler) + strlen(config->key_bindings[i].feedback_string) + 4);
						if (cmdline != NULL) {
							strcpy (cmdline, config->feedback_handler);
							strcat (cmdline, " ");
							strcat (cmdline, config->key_bindings[i].feedback_string);
							strcat (cmdline, " &");
							system(cmdline);
						}
					}
					system(config->key_bindings[i].action.command);
					exit(EXIT_SUCCESS);
				}
				break;
			case AT_DOCUMENT:
				if (fork() == 0) {
					if (config->key_bindings[i].feedback_string) {
						char *cmdline = malloc(strlen(config->feedback_handler) + strlen(config->key_bindings[i].feedback_string) + 4);
						if (cmdline != NULL) {
							strcpy (cmdline, config->feedback_handler);
							strcat (cmdline, " ");
							strcat (cmdline, config->key_bindings[i].feedback_string);
							strcat (cmdline, " &");
							system(cmdline);
						}
					}
					char *cmdline = malloc(strlen(config->document_handler) + strlen(config->key_bindings[i].action.document) + 2);
					if (cmdline != NULL) {
						strcpy (cmdline, config->document_handler);
						strcat (cmdline, " ");
						strcat (cmdline, config->key_bindings[i].action.document);
						system(cmdline);
					}
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

int main(int argc, char *argv[])
{

	int cancel_after_ready = 0;
	int c;
	int errflg = 0;
	int help = 0;
	extern char *optarg;
	extern int optind, optopt;

	running_debug_level = 0;

	while ((c = getopt(argc, argv, ":0d:h")) != -1) {
		switch(c) {
		case '0':
			cancel_after_ready++;
			break;
		case 'd':
			/* FIXME: doesn't check if it is really an integer */
			running_debug_level = atoi(optarg);
			break;
		case 'h':
			/* FIXME: doesn't check if it is really an integer */
			help++;
			break;
		case ':':
			/* -d level is optional, defaults to 1. */
			if (optopt == 'd') {
				running_debug_level++;
			} else {
				fprintf(stderr,
					"superkb: option -%c requires an argument\n", optopt);
				exit(EXIT_FAILURE);
			}
			break;
		case '?':
			fprintf(stderr,	"superkb: unrecognized option: -%c\n", optopt);
			exit(EXIT_FAILURE);
			break;
		}
	}
	if (errflg || help) {
		printf("usage: superkb [options]\n");
		printf("\n");
		printf("Options:\n");
		printf("	-0         Quit when Superkb is ready (for timing and debugging).\n");
		printf("	-d level   Show debug messages up to the specified verbosity level.\n");
		printf("	-h         Shows this help.\n");
		printf("\n");
		if (help)
			exit(EXIT_SUCCESS);	
		else 
			exit(EXIT_FAILURE);
	}

	int status;

	fprintf(stderr, "\nsuperkb " VERSION ": Welcome. This program is under development.\n\n"
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

	debug(1, "*** Debugging has been set to verbosity level %d.\n\n", running_debug_level);

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
		fprintf(stderr, "\nSuperkb must be run from inside the X Window System."
			"\n");
		return EXIT_FAILURE;
	}

	config = config_new(dpy);
	if (config_load(config, dpy) == EXIT_FAILURE) {
		fprintf(stderr, "\n");
		fprintf(stderr, "== Make sure the .superkbrc file exists in your $HOME "
			"directory. ==\n");
		fprintf(stderr, "\n");
		fprintf(stderr, "There is a sample configuration located at the "
			"following URL:\n");
		fprintf(stderr, "http://blog.alvarezp.org/files/superkbrc.sample"
			"\n");
		fprintf(stderr, "\n");
		return EXIT_FAILURE;
	}

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
		fprintf(stderr, "\nIf using GNOME you might want to try adding a"
			"keyboard layout and then\nremoving it, and making sure your"
			"default layout is effectively selected as\ndefault.\n");
		return EXIT_FAILURE;
	}

	status = XkbGetGeometry(dpy, kbdesc);
	kbgeom = kbdesc->geom;
	if (status != Success || kbgeom == NULL) {
		fprintf(stderr, "superkb: Could not load keyboard geometry "
			"information. Quitting.\n");
		fprintf(stderr, "\nIf using GNOME you might want to try adding a"
			"keyboard layout and then\nremoving it, and making sure your"
			"default layout is effectively selected as\ndefault.\n");
		return EXIT_FAILURE;
	}

	get_xinerama_screens(dpy, &xinerama_screens, &xinerama_screens_n);

	status = superkb_init(dpy, kbwin_init, kbwin_map, kbwin_unmap,
		kbwin_event, "en", config->superkb_super1,
		config->superkb_super2, config->drawkb_delay, __Superkb_Action, config->superkb_superkey_replay, config->superkb_superkey_release_cancels);

	if (status != EXIT_SUCCESS) {
		return EXIT_FAILURE;
	}

	if (!cancel_after_ready)
		superkb_start();

	return EXIT_SUCCESS;
}
