/*
 * superkb.c
 *
 * Copyright (C) 2006, Octavio Alvarez Piza.
 * License: GNU General Public License v2.
 *
 */

/* Superkb: This modules does all the key and event handling magic. */

#include <X11/Xlib.h>

#include <unistd.h>
#include <sys/types.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>

#include "superkb.h"
#include "globals.h"
#include "debug.h"

void (*__Action)(KeyCode keycode, unsigned int state);

struct _kbwin {
	int (*init) (Display *);
	void (*map) (Display *);
	void (*unmap) (Display *);
	void (*event) (Display *, XEvent ev);
};

struct config {
	char kblayout[3];
	KeySym key1;
	KeySym key2;
};

struct instance {
	KeyCode key1;
	KeyCode key2;
	double drawkb_delay;
	Display *dpy;
	Window rootwin;
	int superkey_replay;
	int superkey_release_cancels;
};

XEvent sigev;

struct _kbwin kbwin = { NULL, NULL, NULL, NULL };
struct config conf = { "", 0, 0 };
struct instance inst = { 0, 0, 0, NULL, 0 };

/* Start PRESSED KEYS STACK */

typedef struct pressed_keys_element {
	int keycode;
	int state;
} pressed_key_t;

pressed_key_t *pressed_keys = NULL;
int pressed_keys_n = 0;

void remove_from_pressed_key_stack(int keycode, int state)
{
	int x;
	int y;

	for (x = pressed_keys_n-1; x >=0; x--) {
		if (pressed_keys[x].keycode == keycode &&
			pressed_keys[x].state == state) {
				/* Item to be removed found. */
				for (y = x; y < pressed_keys_n-1; y++) {
					pressed_keys[y].keycode = pressed_keys[y+1].keycode;
					pressed_keys[y].state = pressed_keys[y+1].state;
				}
				list_rmv_element(pressed_keys, pressed_keys_n, pressed_key_t);
		}
	}
}

void push_into_pressed_key_stack(int keycode, int state)
{
	list_add_element(pressed_keys, pressed_keys_n, pressed_key_t);
	pressed_keys[pressed_keys_n-1].keycode = keycode;
	pressed_keys[pressed_keys_n-1].state = state;
}

void clear_pressed_key_stack() {
	if (pressed_keys != NULL) {
		free (pressed_keys);
		pressed_keys = NULL;
	}

	pressed_keys_n = 0;
}

/* End PRESSED KEYS STACK */

static void
timerdiff(struct timeval *dst, struct timeval *tv0, struct timeval *tv1)
{
	if (tv1->tv_usec >= tv0->tv_usec) {
		dst->tv_usec = tv1->tv_usec - tv0->tv_usec;
		dst->tv_sec = tv1->tv_sec - tv0->tv_sec;
	} else {
		dst->tv_usec = tv1->tv_usec - tv0->tv_usec + 1000000;
		dst->tv_sec = tv1->tv_sec - tv0->tv_sec - 1;
	}
}

static void
timer_sub(struct timeval *tv1, struct timeval *tv0)
{
	if (tv1->tv_usec >= tv0->tv_usec) {
		tv1->tv_usec = tv1->tv_usec - tv0->tv_usec;
		tv1->tv_sec = tv1->tv_sec - tv0->tv_sec;
	} else {
		tv1->tv_usec = tv1->tv_usec - tv0->tv_usec + 1000000;
		tv1->tv_sec = tv1->tv_sec - tv0->tv_sec - 1;
	}
}

/* This function is the same as XNextEvent but a timeout can be
 * specified in "to". It's supposed to accept NULL for no timeout
 * as the function works like a wrapper for select().
 *
 * It works by select()ing on XConnectionNumber(display) to wait either
 * for incoming traffic or timeout. If the former is found, return the
 * next event.
 *
 * RETURNS == 0 ? Timed out before getting an event.
 * RETURNS <  0 ? Error. Useful to get EINTR. Return set to -errno.
 * RETURNS >  0 ? Event catched.
 */
int
XNextEventWithTimeout(Display * display, XEvent * event_return,
					  struct timeval *to)
{
	fd_set fd;
	int r;

	XFlush(display);

	if (QLength(display) > 0) {
		XNextEvent(display, event_return);
		return 1;
	}

	FD_ZERO(&fd);
	FD_SET(XConnectionNumber(display), &fd);

	r = select(FD_SETSIZE, &fd, NULL, NULL, to);

	if (r == 0) {
		/* Timeout */
		return r;
	}

	if (r == -1) {
		/* Error. Hopefully it's EINTR. */
		return -errno;
	}

	XNextEvent(display, event_return);
	return 1;
}

enum to_index {
  TO_CONFIG = 0,
  TO_DRAWKB = 1
};

#define to_n 2

struct timeval to[to_n];

void superkb_start()
{
	/* Solicitar los eventos */
	if (inst.key1)
		XGrabKey(inst.dpy, inst.key1, AnyModifier, inst.rootwin, True,
				 GrabModeAsync, GrabModeAsync);
	if (inst.key2)
		XGrabKey(inst.dpy, inst.key2, AnyModifier, inst.rootwin, True,
				 GrabModeAsync, GrabModeAsync);

	XKeyboardState xkbs;

	int saved_key1_autorepeat_mode = 0;
	int saved_key2_autorepeat_mode = 0;

	int x;

	/* Save autorepeat previous state. Then turn off. */

	XGetKeyboardControl(inst.dpy, &xkbs);
	saved_key1_autorepeat_mode = (xkbs.auto_repeats[(int) inst.key1/8] & inst.key1 % 8) > 0;

	XGetKeyboardControl(inst.dpy, &xkbs);
	saved_key2_autorepeat_mode = (xkbs.auto_repeats[(int) inst.key2/8] & inst.key2 % 8) > 0;

	/* FIXME: Autorepeat must be restored on end */

	XKeyboardControl xkbc;
	xkbc.key = inst.key1;
	xkbc.auto_repeat_mode = AutoRepeatModeOff;
	XChangeKeyboardControl(inst.dpy, KBAutoRepeatMode | KBKey, &xkbc);

	xkbc.key = inst.key2;
	xkbc.auto_repeat_mode = AutoRepeatModeOff;
	XChangeKeyboardControl(inst.dpy, KBAutoRepeatMode | KBKey, &xkbc);

	int ignore_release = 0;
	int saved_autorepeat_mode = 0;

	/* Counter, in case user presses both Supers and releases only one. */
	int super_was_active = 0;
	debug (2, "[ar] super_was_active has been initialized to %d.\n", super_was_active);

	/* Initialize timeouts to be inactive. */
	timerclear(&to[TO_DRAWKB]);
	timerclear(&to[TO_CONFIG]);

	while (1) {

		struct timeval hold_start;
		struct timeval hold_end;
		XEvent ev;
		int XNEWT_ret;
		int i;
		int to_reason;
		int super_replay;
		XEvent event_save_for_replay;
		Window event_saved_window;

		/* Decide wether to use XNextEvent or my own XNextEventWithTimeout
		 * and do accordingly. If WithTimeout was used, substract the
		 * time actually elapsed to the set timeout. */

		for (i = 0; i < to_n; i++) {
			if (timerisset(&to[i])) {
				to_reason = i;
				break;
			}
		}

		if (i == to_n) {
			XNEWT_ret = XNextEventWithTimeout(inst.dpy, &ev, NULL);
		} else {
			gettimeofday(&hold_start, NULL);

			for (i = to_reason + 1; i < to_n; i++) {
				if (timerisset(&to[i])) {
					if (timercmp(&to[i], &to[to_reason], <))
						to_reason = i;
				}
			}

			/* Man pages say that Linux select() modifies timeout where
			 * other implementations don't. We oughta save timeout and restore
			 * it after select().
			 */
			struct timeval timeout_save;
			memcpy(&timeout_save, &to[to_reason], sizeof(struct timeval));

			/* I hope select() takes proper care of negative timeouts.
			 * It happens sometimes.
			 */
			if (&to[to_reason].tv_sec > 0 &&
				&to[to_reason].tv_usec > 0) {
				XNEWT_ret = XNextEventWithTimeout(inst.dpy, &ev, &to[to_reason]);
			} 

			/* Restore. */
			memcpy(&to[to_reason], &timeout_save, sizeof(struct timeval));

			gettimeofday(&hold_end, NULL);

			struct timeval hold_time;
			/* Update all timers */
			timerdiff(&hold_time, &hold_start, &hold_end);
			for (i = 0; i < to_n; i++) {
				if (timerisset(&to[i])) {
					timer_sub(&to[i], &hold_time);
				}
			}
		}

		if (XNEWT_ret == -EINTR)
			break;
		if (XNEWT_ret == 0) {
			/* Timed out */
			if (to_reason == TO_CONFIG) {
				debug(9, "[kc] Placeholder: key config window should pop up here.\n");
				timerclear(&to[TO_CONFIG]);
				ignore_release = 1;
			}
			if (to_reason == TO_DRAWKB) {

				super_replay = 0;

				timerclear(&to[TO_DRAWKB]);

				/* Map Window. */
				kbwin.map(inst.dpy);

			}
		} else if (ev.xany.window != inst.rootwin) {
			kbwin.event(inst.dpy, ev);
		} else if (ev.xkey.keycode == inst.key1
				   || ev.xkey.keycode == inst.key2) {
			if (ev.type == KeyPress) {
				ignore_release = 0;

				debug(1, "[sk] Super key has been pressed, code: %d, name: %s.\n", ev.xkey.keycode,
					XKeysymToString(XKeycodeToKeysym(inst.dpy, ev.xkey.keycode, 0)));

				if (super_was_active++) {
					super_replay = 0;
					debug(2, "[sa] super_was_active increased to %d, not taking action.\n", super_was_active);
					continue;
				}

				debug(2, "[sa] super_was_active increased to %d, taking action.\n", super_was_active);

				super_replay = inst.superkey_replay;
				memcpy(&event_save_for_replay, &ev, sizeof(XEvent));

				int revert_to_return;
				XGetInputFocus(inst.dpy, &event_saved_window, &revert_to_return);

				XKeyboardState xkbs;

				/* Save autorepeat previous state. Then turn off. */
				XGetKeyboardControl(inst.dpy, &xkbs);
				saved_autorepeat_mode = xkbs.global_auto_repeat;

				debug(1, "[ar] AutoRepeat state has been saved: %d.\n", saved_autorepeat_mode);

				XKeyboardControl xkbc;
				xkbc.auto_repeat_mode = AutoRepeatModeOff;
				XChangeKeyboardControl(inst.dpy, KBAutoRepeatMode, &xkbc);

				debug(1, "[ar] AutoRepeat state has been turned off.\n");

				/* Grab the keyboard. */
				XGrabKeyboard(inst.dpy, inst.rootwin, False, GrabModeAsync,
							  GrabModeAsync, CurrentTime);

				if (inst.drawkb_delay > 0) {
					to[TO_DRAWKB].tv_sec = (int) inst.drawkb_delay;
					to[TO_DRAWKB].tv_usec = (int) ((inst.drawkb_delay - to[TO_DRAWKB].tv_sec) * 1000000);
				} else {
					/* Map Window. */
					kbwin.map(inst.dpy);
				}
			} else if (ev.type == KeyRelease) {

				debug(1, "[sk] Super key has been released, code: %d, name: %s.\n", ev.xkey.keycode,
					XKeysymToString(XKeycodeToKeysym(inst.dpy, ev.xkey.keycode, 0)));

				if (--super_was_active) {
					debug(2, "[sa] super_was_active decreased to %d, ignoring release.\n", super_was_active);
					continue;
				}

				debug(2, "[sa] super_was_active decreased to %d, taking action.\n", super_was_active);

				timerclear(&to[TO_DRAWKB]);
				timerclear(&to[TO_CONFIG]);

				if (super_replay) {
					/* Since Xlib only supports Replaying a key before getting the next keyboard event,
					 * we can't really use XAllowEvents() to replay the Super key in case the user
					 * asked to. So we try XSendEvent() with the Press from the saved event on KeyPress,
					 * and the Release we are currently using.
					 */
					event_save_for_replay.xkey.window = event_saved_window;
					ev.xkey.window = event_saved_window;
					event_save_for_replay.xkey.subwindow = 0;
					ev.xkey.subwindow = 0;

					XSendEvent(inst.dpy, event_saved_window, 1, KeyPressMask, &event_save_for_replay);
					XSendEvent(inst.dpy, event_saved_window, 1, KeyReleaseMask, &ev);
					XSync(inst.dpy, True);
					debug(1, "[sr] Super key has been replayed\n");
				}

				/* Restore saved_autorepeat_mode. */
				XKeyboardControl xkbc;
				/*xkbc.auto_repeat_mode = saved_autorepeat_mode; */
				xkbc.auto_repeat_mode = AutoRepeatModeOn;
				XChangeKeyboardControl(inst.dpy, KBAutoRepeatMode, &xkbc);

				debug(1, "[ar] AutoRepeat has been restored to: %d\n", saved_autorepeat_mode);

				XUngrabKeyboard(inst.dpy, CurrentTime);
				kbwin.unmap(inst.dpy);

				for (x = 0; x < pressed_keys_n; x++) {
					__Action(pressed_keys[x].keycode, pressed_keys[x].state);

					debug(1, "[ac] Due to Super key release, executed action for key code = %d, name: %s\n", pressed_keys[x].keycode, 
						XKeysymToString(XKeycodeToKeysym
							(inst.dpy, pressed_keys[x].keycode, 0)));

				}

				clear_pressed_key_stack();

				debug(1, "---------------------------------------------\n");

			}
		} else if (ev.type == KeyPress) {
			super_replay = 0;

			to[TO_CONFIG].tv_sec = 3;
			to[TO_CONFIG].tv_usec = 0;

			push_into_pressed_key_stack(ev.xkey.keycode, ev.xkey.state);
		} else if ((ev.type == KeyRelease && !ignore_release &&
				   super_was_active > 0)) {
			/* User might have asked for binding configuration, so ignore key
			 * release. That's what ignore_release is for.
			 */
			timerclear(&to[TO_CONFIG]);
			__Action(ev.xkey.keycode, ev.xkey.state);

			debug(1, "[ac] Due to bound key release, executed action for key code = %d, name: %s\n", ev.xkey.keycode, 
				   XKeysymToString(XKeycodeToKeysym
								   (inst.dpy, ev.xkey.keycode, 0)));
			debug(2, "     ... and because super_was_active value was > 0: %d\n", super_was_active);

			remove_from_pressed_key_stack(ev.xkey.keycode, ev.xkey.state);

		} else {
			/* According to manual, this should not be necessary. */
			/* XAllowEvents(inst.dpy, ReplayKeyboard, CurrentTime); */
		}

	}

}

int
superkb_init(Display *display,
			 int (*kbwin_init) (Display *),
			 void (*kbwin_map) (Display *),
			 void (*kbwin_unmap) (Display *),
			 void (*kbwin_event) (Display *, XEvent ev),
			 const char *kblayout, KeyCode key1, KeyCode key2,
			 double drawkb_delay,
			 void (*f)(KeyCode keycode, unsigned int state),
			 int superkey_replay,
             int superkey_release_cancels)
			 
{

	__Action = f;
	inst.dpy = display;
	int r;

	inst.drawkb_delay = drawkb_delay;
	inst.superkey_replay = superkey_replay;
	inst.superkey_release_cancels = superkey_release_cancels;

	/* FIXME: Validate parameters. */

	/* Set configuration values. Parameters should be already validated. */
	kbwin.init = kbwin_init;
	kbwin.map = kbwin_map;
	kbwin.unmap = kbwin_unmap;
	kbwin.event = kbwin_event;
	strcpy(conf.kblayout, kblayout);
	conf.key1 = key1;
	conf.key2 = key2;

	//inst.key1 = XKeysymToKeycode(inst.dpy, key1);
	//inst.key2 = XKeysymToKeycode(inst.dpy, key2);
	inst.key1 = key1;
	inst.key2 = key2;

	inst.rootwin = DefaultRootWindow(inst.dpy);

	inst.dpy = display;

	/* Create the keyboard window. */
	r = kbwin.init(inst.dpy);
	if (r == EXIT_FAILURE)
		return EXIT_FAILURE;

	XFlush(inst.dpy);

	return 0;
}
