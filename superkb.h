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

void superkb_start();

int superkb_init(Display *display,
             int (*kbwin_init) (Display *),
             void (*kbwin_map) (Display *),
             void (*kbwin_unmap) (Display *),
             void (*kbwin_event) (Display *, XEvent ev),
             const char *kblayout, KeyCode key1, KeyCode key2,
			 double drawkb_delay,
             void (*f)(KeyCode keycode, unsigned int state),
             int superkey_replay);

#endif                          /* __SUPERKB_H */
