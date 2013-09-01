/*
 * superkb.h
 *
 * Copyright (C) 2006, Octavio Alvarez Piza.
 * License: GNU General Public License v2.
 *
 * Provides Superkb with the mechanism to handle the keyboard. This includes:
 *  - handling of the hotkey (the one that triggers the keyboard display on
 *    the screen)
 *  - calling of the callback functions that actually launch the actions
 *    (application, document or whatever), 
 *  - timer management (two at the moment):
 *    :: One that triggers the configuration program for an action key if
 *       held down for 3 seconds.
 *    :: The one that triggers the drawing of the keyboard.
 *
 * This code only does the handling of the keyboard. Every action is done by
 * callbacks. This allows the module to be plugged out if needed.
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
	int state_mask;
	void (*welcome_message)();
} superkb_t, *superkb_p;

void superkb_start(superkb_p this);

superkb_p superkb_create(void);

void superkb_restore_auto_repeat(superkb_p this);

int superkb_init(superkb_p this,
             Display *display,
             const char *kblayout, KeyCode key1, KeyCode key2,
			 double drawkb_delay,
             void (*f)(KeyCode keycode, unsigned int state),
             int superkey_replay,
             int superkey_release_cancels,
             int states_mask,
             void (*welcome_message)());

void superkb_kbwin_set(superkb_p this,
			 int (*superkb_kbwin_init) (Display *),
			 void (*superkb_kbwin_map) (Display *),
			 void (*superkb_kbwin_unmap) (Display *),
			 void (*superkb_kbwin_event) (Display *, XEvent ev));

#endif     /* __SUPERKB_H */
