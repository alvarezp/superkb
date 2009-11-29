/*
 * superkb.h
 *
 * Copyright (C) 2006, Octavio Alvarez Piza.
 * License: GNU General Public License v2.
 *
 */

#ifndef __SUPERKB_H
#define __SUPERKB_H

#include <X11/Xlib.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>
#include <errno.h>

typedef struct {
	int (*init) (Display *);
	void (*map) (Display *);
	void (*unmap) (Display *);
	void (*event) (Display *, XEvent ev);
} kbwin_t, *kbwin_p;

typedef struct {
	KeyCode key1;
	KeyCode key2;
	double drawkb_delay;
	Display *dpy;
	Window rootwin;
	int superkey_replay;
	int superkey_release_cancels;
	kbwin_t kbwin;
	char kblayout[3];
} superkb_t, *superkb_p;

void superkb_start(superkb_p this);

superkb_p superkb_create(void);

int superkb_init(superkb_p this,
             Display *display,
             const char *kblayout, KeyCode key1, KeyCode key2,
			 double drawkb_delay,
             void (*f)(KeyCode keycode, unsigned int state),
             int superkey_replay,
             int superkey_release_cancels);

void superkb_kbwin_set(superkb_p this,
			 int (*superkb_kbwin_init) (Display *),
			 void (*superkb_kbwin_map) (Display *),
			 void (*superkb_kbwin_unmap) (Display *),
			 void (*superkb_kbwin_event) (Display *, XEvent ev));

#endif     /* __SUPERKB_H */
