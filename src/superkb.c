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
#include <signal.h>
#include <sys/time.h>

#include "superkb.h"

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

struct sigaction action;

static void
timerdiff(struct timeval *dst, struct timeval *tv0, struct timeval *tv1)
{
    if (tv1->tv_usec >= tv0->tv_usec) {
        dst->tv_usec = tv1->tv_usec - tv0->tv_usec;
        dst->tv_sec = tv1->tv_sec - tv0->tv_sec;
    } else {
        dst->tv_usec = tv1->tv_usec - tv0->tv_usec + 1000000;
        dst->tv_sec = tv1->tv_sec - tv0->tv_usec - 1;
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

void sighandler(int sig)
{
    switch (sig) {
    case SIGUSR1:
        break;
    }
}

void superkb_start()
{
    /* Solicitar los eventos */
    if (inst.key1)
        XGrabKey(inst.dpy, inst.key1, AnyModifier, inst.rootwin, True,
                 GrabModeAsync, GrabModeAsync);
    if (inst.key2)
        XGrabKey(inst.dpy, inst.key2, AnyModifier, inst.rootwin, True,
                 GrabModeAsync, GrabModeAsync);

    struct timeval to = { 0, 0 };
    int ignore_release = 0;
    int saved_autorepeat_mode = 0;

    /* Counter, in case user presses both Supers and releases only one. */
    int super_was_active = 0;

    while (1) {

        struct timeval hold_start;
        struct timeval hold_end;
        XEvent ev;
        int XNEWT_ret;

        /* Decide wether to use XNextEvent or my own XNextEventWithTimeout
         * and do accordingly. If WithTimeout was used, substract the
         * time actually elapsed to the set timeout. */
        if (!timerisset(&to)) {
            XNEWT_ret = XNextEventWithTimeout(inst.dpy, &ev, NULL);
        } else {
            gettimeofday(&hold_start, NULL);

            XNEWT_ret = XNextEventWithTimeout(inst.dpy, &ev, &to);

            gettimeofday(&hold_end, NULL);
            timerdiff(&to, &hold_start, &hold_end);
        }

        if (XNEWT_ret == -EINTR)
            break;
        if (XNEWT_ret == 0) {
            /* Timed out */
            printf("CONFIGURE BINDING!\n");
            timerclear(&to);
            ignore_release = 1;
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

                /* Map Window. */
                kbwin.map(inst.dpy);

            } else if (ev.type == KeyRelease) {
                if (--super_was_active)
                    continue;

                timerclear(&to);

                /* Restore saved_autorepeat_mode. */
                XKeyboardControl xkbc;
                /*xkbc.auto_repeat_mode = saved_autorepeat_mode; */
                xkbc.auto_repeat_mode = AutoRepeatModeOn;
                XChangeKeyboardControl(inst.dpy, KBAutoRepeatMode, &xkbc);

                XUngrabKeyboard(inst.dpy, CurrentTime);
                kbwin.unmap(inst.dpy);
            }
        } else if (ev.type == KeyPress) {
            to.tv_sec = 3;
            to.tv_usec = 0;
        } else if (ev.type == KeyRelease && !ignore_release &&
                   super_was_active > 0) {
            /* User might have asked for binding configuration, so ignore key
             * release. That's what ignore_release is for.
             */
            timerclear(&to);
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

    /* SIGUSR1: Exit. */
    action.sa_handler = sighandler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGUSR1, &action, NULL);

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
