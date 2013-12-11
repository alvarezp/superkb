/*
 * main.c
 *
 * Copyright (C) 2006, Octavio Alvarez Piza.
 * License: GNU General Public License v2.
 *
 */

/* Thanks to Natan "Whatah" Zohar for helping with tokenizer. */

#define _POSIX_C_SOURCE 2

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
#include <sys/wait.h>
#include <locale.h>

#include "superkb.h"
#include "imagelib.h"
#include "drawkblib.h"
#include "superkbrc.h"
#include "globals.h"
#include "debug.h"
#include "version.h"

#include "screeninfo.h"

#define WINH(i) (kbgeom->width_mm * scale[i])
#define WINV(i) (kbgeom->width_mm * scale[i])

superkb_p superkb;

struct sigaction action;

screeninfo_t *screens=NULL;
int screens_n=0;

Window *kbwin=NULL;
Pixmap *kbwin_backup=NULL;
GC *kbwin_gc=NULL;

config_t *config;

double *scale;

Window prev_kbwin_focus;
int prev_kbwin_revert;

Display *dpy;

drawkb_p draw1;

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
		if (keysym == XkbKeycodeToKeysym(dpy, config->key_bindings[i].keycode, 0, 0) &&
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

int PutIcon(Drawable kbwin, int x, int y, int width, int height, const char *fn)
{

	void *i;

	i = NewImage(fn);
	if (i == NULL) {
		perror("puticon");
		return EXIT_FAILURE;
	}

	LoadImage(i, fn);

	ResizeImage(i, width, height);

	DrawImage(i, kbwin, x, y);

	FreeImage(i);


	return EXIT_SUCCESS;

}

void kbwin_event(Display * dpy, XEvent ev)
{

	int i;
	if (ev.type == Expose) {
/*		drawkb_draw(dpy, kbwin[i], kbwin_gc[i], DisplayWidth(dpy, 0), DisplayHeight(dpy, 0), kbdesc);*/
		for (i=0; i < screens_n; i++) {
			XCopyArea(dpy, kbwin_backup[i], kbwin[i], kbwin_gc[i], 0, 0, WINH(i), WINV(i), 0, 0);
		}
		XFlush(dpy);
	} else if (ev.type == VisibilityNotify &&
			   ev.xvisibility.state != VisibilityUnobscured) {
		for (i=0; i < screens_n; i++) {
			XRaiseWindow(dpy, kbwin[i]);
		}
	}

}

void kbwin_map(Display * dpy)
{
	/* XGetInputFocus(dpy, &prev_kbwin_focus, &prev_kbwin_revert); */
	int i;
	for (i=0; i < screens_n; i++) {
		XSetTransientForHint(dpy, kbwin[i], DefaultRootWindow(dpy));
		XMapWindow(dpy, kbwin[i]);
		XRaiseWindow(dpy, kbwin[i]);
	}
}

void kbwin_unmap(Display * dpy)
{
	int i;
	for (i=0; i < screens_n; i++) {
		XUnmapWindow(dpy, kbwin[i]);
	}
	/* XSetInputFocus(dpy, prev_kbwin_focus, prev_kbwin_revert, CurrentTime); */
}

int kbwin_init(Display * dpy)
{
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

	/* Create one windows per screen. */

	screeninfo_t *xsi;

	int i;
	kbwin = malloc(screens_n * sizeof(Window));
	if (kbwin == NULL) {
		perror("superkb: kbwin_init(): kbwin = malloc() failed");
		return EXIT_FAILURE;
	}

	kbwin_backup = malloc(screens_n * sizeof(Pixmap));
	if (kbwin == NULL) {
		perror("superkb: kbwin_init(): kbwin_backup = malloc() failed");
		return EXIT_FAILURE;
	}

	scale = malloc(screens_n * sizeof(double));
	if (kbwin == NULL) {
		perror("superkb: kbwin_init(): scale = malloc() failed");
		return EXIT_FAILURE;
	}

	kbwin_gc = malloc(screens_n * sizeof(GC));
	if (kbwin == NULL) {
		perror("superkb: kbwin_init(): kbwin_gc = malloc() failed");
		return EXIT_FAILURE;
	}

	for (i = 0; i < screens_n; i++) {

		int winv;
		int winh;

		/* Just as little shortcut. */
		xsi = &screens[i];

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

		draw1 = drawkb_create(dpy, config->drawkb_font, IconQuery, config->drawkb_painting_mode, scale[i], &debug, kbdesc, config->use_gradients);

		if (draw1 == NULL) {
			return EXIT_FAILURE;
		}

		kbwin_backup[i] = XCreatePixmap(dpy, kbwin[i], winh, winv, DefaultDepth(dpy, DefaultScreen(dpy)));

		XSetForeground(dpy, kbwin_gc[i], background.pixel);

		XFillRectangle(dpy, kbwin_backup[i], kbwin_gc[i], 0, 0, winh, winv);

		XSetForeground(dpy, kbwin_gc[i], foreground.pixel);
		XSetBackground(dpy, kbwin_gc[i], background.pixel);

		drawkb_draw(draw1, kbwin_backup[i], kbwin_gc[i], winh, winv, kbdesc, PutIcon);

	}

	XFlush(dpy);

	return EXIT_SUCCESS;
}

void __Superkb_Action(KeyCode keycode, unsigned int state)
{
	unsigned int i;
	char *argv[4] = { "sh", "-c", NULL, NULL };
	for (i = 0; i < config->key_bindings_n; i++) {
		if (config->key_bindings[i].keycode == keycode &&
		  config->key_bindings[i].state == state) {
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
						free(cmdline);
					}
					argv[2] = config->key_bindings[i].action.command;
					execvp(*argv, argv);
					exit(EXIT_FAILURE);
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
						free(cmdline);
						}
					}
					char *cmdline = malloc(strlen(config->document_handler) + strlen(config->key_bindings[i].action.document) + 2);
					if (cmdline != NULL) {
						strcpy (cmdline, config->document_handler);
						strcat (cmdline, " ");
						strcat (cmdline, config->key_bindings[i].action.document);
						argv[2] = cmdline;
						execvp(*argv, argv);
					}
					exit(EXIT_FAILURE);
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
	int chld_status;
	int chld_p;

	switch (sig) {
	case SIGUSR1:
		break;
	case SIGCHLD:
		chld_p = wait(&chld_status);
		debug(6, "[chld] Got SIGCHLD. Process: %d. Status: %d\n", chld_p, chld_status);
		break;
	case SIGINT:
		superkb_restore_auto_repeat(superkb);
		signal(SIGINT, SIG_DFL);
		kill(getpid(), SIGINT);
		break;
	}
}

void welcome_message() {
	if (strcmp(config->welcome_cmd, "") != 0) {

		char *cmdline = malloc(strlen(config->welcome_cmd)+3);
		if (cmdline == NULL) {
			perror("superkb: welcome_message(): malloc() failed");
			return;
		}

		strcpy(cmdline, config->welcome_cmd);
		strcat(cmdline, " &");
		system(cmdline);
		free(cmdline);
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

	while ((c = getopt(argc, argv, ":0d:hv")) != -1) {
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
		case 'v':
			printf("%s\n", VERSION VEXTRA);
			exit(EXIT_SUCCESS);
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
		printf("	-v         Shows program version.\n");
		printf("\n");
		if (help)
			exit(EXIT_SUCCESS);	
		else 
			exit(EXIT_FAILURE);
	}

	int status;

	fprintf(stderr, "\nsuperkb " VERSION VEXTRA ": Welcome. This program is under development.\n\n"
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

	/* Set SIGCHLD handler. Needed to avoid zombie child processes. */
	action.sa_handler = sighandler;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;
	if (sigaction(SIGCHLD, &action, NULL) != 0) {
		fprintf(stderr, "superkb: Error setting SIGCHLD signal handler. "
			"Quitting.\n");
		return EXIT_FAILURE;
	}

	/* Set SIGINT handler. Needed to restore autorepeat on Super keys. */
	action.sa_handler = sighandler;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;
	if (sigaction(SIGINT, &action, NULL) != 0) {
		fprintf(stderr, "superkb: Error setting SIGINT signal handler. "
			"Quitting.\n");
		return EXIT_FAILURE;
	}

	/* Connect to display. */
	dpy = XOpenDisplay(NULL);
	if (dpy == NULL) {
		fprintf(stderr, "superkb: Couldn't open display. Quitting.\n");
		fprintf(stderr, "\nSuperkb must be run from inside the X Window System."
			"\n");
		return EXIT_FAILURE;
	}

	config = config_new(dpy);

	setlocale(LC_ALL, "");

	if (config == NULL) {
		fprintf(stderr, "Superkb could not load configuration. Quitting.\n");
		return EXIT_FAILURE;
	}

	if (config_load(config, dpy) == EXIT_FAILURE) {
		fprintf(stderr, "\n");
		fprintf(stderr, "== Make sure the .superkbrc file exists in your $HOME "
			"directory. ==\n");
		fprintf(stderr, "\n");
		fprintf(stderr, "There is a sample configuration located at the "
			"following URL:\n");
		fprintf(stderr, "http://blog.alvarezp.org/files/superkbrc.sample "
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
		fprintf(stderr, "\nIf using GNOME you might want to try adding a "
			"keyboard layout and then\nremoving it, and making sure your "
			"current layout is effectively selected as\ndefault.\n");
		return EXIT_FAILURE;
	}

	status = XkbGetGeometry(dpy, kbdesc);
	kbgeom = kbdesc->geom;
	if (status != Success || kbgeom == NULL) {
		fprintf(stderr, "superkb: Could not load keyboard geometry "
			"information. Quitting.\n");
		fprintf(stderr, "\nIf using GNOME you might want to try adding a "
			"keyboard layout and then\nremoving it, and making sure your "
			"default layout is effectively selected as\ndefault.\n");
		return EXIT_FAILURE;
	}

	screeninfo_get_screens(dpy, &screens, &screens_n);

	if (Init_Imagelib(dpy, config->drawkb_imagelib) == EXIT_FAILURE)
	{
		char vals[500] = "";
		Imagelib_GetValues((char *) &vals, 499);
		fprintf(stderr, "Failed to initialize image library: %s.\n\n"
			"You might try any of the following as the value for IMAGELIB in\n"
			"your $HOME/.superkbrc file: %s\n", config->drawkb_imagelib, vals);
		return EXIT_FAILURE;
	}

	status = Init_drawkblib(config->drawkb_drawkblib);
	if (status == EXIT_FAILURE) {
		char vals[500] = "";
		drawkblib_GetValues((char *) &vals, 499);
		fprintf(stderr, "Failed to initialize drawkb library: %s.\n\n"
			"You might try any of the following as the value for DRAWKBLIB in\n"
			"your $HOME/.superkbrc file: %s\n", config->drawkb_drawkblib, vals);
		return EXIT_FAILURE;
	}

	superkb = superkb_create();

	superkb_kbwin_set(superkb, kbwin_init, kbwin_map, kbwin_unmap, kbwin_event);

	status = superkb_init(superkb, dpy, "en", config->superkb_super1,
		config->superkb_super2, config->drawkb_delay, __Superkb_Action, config->superkb_superkey_replay, config->superkb_superkey_release_cancels,
		~config->squashed_states, &welcome_message);

	if (status != EXIT_SUCCESS) {
		return EXIT_FAILURE;
	}

	if (!cancel_after_ready)
		superkb_start(superkb);

	return EXIT_SUCCESS;
}
