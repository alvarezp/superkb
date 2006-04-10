/* License: GPL v2. */
/* To be compiled with:
 *    gcc -o superkb proto.c -ansi -lX11 -L/usr/X11/lib
 */

/* Superkb: This modules does all the key handling magic. */

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

double drawkb_delay = 0;

void (*__Action)(KeyCode keycode, unsigned int state);

struct _kbwin {
    void (*init) (Display *);
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
    Display *dpy;
    Window rootwin;
};

XEvent sigev;

struct _kbwin kbwin = { NULL, NULL, NULL, NULL };
struct config conf = { "", 0, 0 };
struct instance inst = { 0, 0, NULL, 0 };

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

    if (QLength(display) > 0) {
        XNextEvent(display, event_return);
        return 1;
    }

    XFlush(display);

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

    int ignore_release = 0;
    int saved_autorepeat_mode = 0;

    /* Counter, in case user presses both Supers and releases only one. */
    int super_was_active = 0;

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
                printf("Placeholder: key config window should pop up here.\n");
                timerclear(&to[TO_CONFIG]);
                ignore_release = 1;
            }
            if (to_reason == TO_DRAWKB) {
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

                if (super_was_active++)
                    continue;

                XKeyboardState xkbs;

                /* Save autorepeat previous state. Then turn off. */
                XGetKeyboardControl(inst.dpy, &xkbs);
                saved_autorepeat_mode = xkbs.global_auto_repeat;

                XKeyboardControl xkbc;
                xkbc.auto_repeat_mode = AutoRepeatModeOff;
                XChangeKeyboardControl(inst.dpy, KBAutoRepeatMode, &xkbc);

                /* Grab the keyboard. */
                XGrabKeyboard(inst.dpy, inst.rootwin, False, GrabModeAsync,
                              GrabModeAsync, CurrentTime);

                if (drawkb_delay > 0) {
                    to[TO_DRAWKB].tv_sec = (int) drawkb_delay;
                    to[TO_DRAWKB].tv_usec = (int) ((drawkb_delay - to[TO_DRAWKB].tv_sec) * 1000000);
                } else {
                    /* Map Window. */
                    kbwin.map(inst.dpy);
                }
            } else if (ev.type == KeyRelease) {

                if (--super_was_active)
                    continue;

                timerclear(&to[TO_DRAWKB]);
                timerclear(&to[TO_CONFIG]);

                /* Restore saved_autorepeat_mode. */
                XKeyboardControl xkbc;
                /*xkbc.auto_repeat_mode = saved_autorepeat_mode; */
                xkbc.auto_repeat_mode = AutoRepeatModeOn;
                XChangeKeyboardControl(inst.dpy, KBAutoRepeatMode, &xkbc);

                XUngrabKeyboard(inst.dpy, CurrentTime);
                kbwin.unmap(inst.dpy);
            }
        } else if (ev.type == KeyPress) {
            to[TO_CONFIG].tv_sec = 3;
            to[TO_CONFIG].tv_usec = 0;
        } else if ((ev.type == KeyRelease && !ignore_release &&
                   super_was_active > 0) || (ev.type == KeyRelease)) {
            /* User might have asked for binding configuration, so ignore key
             * release. That's what ignore_release is for.
             */
            timerclear(&to[TO_CONFIG]);
            printf("KeyRelease: %s\n",
                   XKeysymToString(XKeycodeToKeysym
                                   (inst.dpy, ev.xkey.keycode, 0)));

            __Action(ev.xkey.keycode, ev.xkey.state);


        } else {
            /* According to manual, this should not be necessary. */
            /* XAllowEvents(inst.dpy, ReplayKeyboard, CurrentTime); */
        }

    }

}

int
superkb_load(Display *display,
             void (*kbwin_init) (Display *),
             void (*kbwin_map) (Display *),
             void (*kbwin_unmap) (Display *),
             void (*kbwin_event) (Display *, XEvent ev),
             const char *kblayout, KeySym key1, KeySym key2,
             void (*f)(KeyCode keycode, unsigned int state))
             
{

    __Action = f;
    inst.dpy = display;

    /* FIXME: Validate parameters. */

    /* Set configuration values. Parameters should be already validated. */
    kbwin.init = kbwin_init;
    kbwin.map = kbwin_map;
    kbwin.unmap = kbwin_unmap;
    kbwin.event = kbwin_event;
    strcpy(conf.kblayout, kblayout);
    conf.key1 = key1;
    conf.key2 = key2;

    inst.key1 = XKeysymToKeycode(inst.dpy, key1);
    inst.key2 = XKeysymToKeycode(inst.dpy, key2);

    inst.rootwin = DefaultRootWindow(inst.dpy);

    inst.dpy = display;

    /* Create the keyboard window. */
    kbwin.init(inst.dpy);

    XFlush(inst.dpy);

    return 0;
}
