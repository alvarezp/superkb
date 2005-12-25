/* License: GPL. */
/* To be compiled with:
 *    gcc -o superkb proto.c -ansi -lX11 -L/usr/X11/lib
 */

#include <X11/Xlib.h>

#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>

#include "superkb.h"

#define       timerisset(tvp)\
              ((tvp)->tv_sec || (tvp)->tv_usec)
#define       timercmp(tvp, uvp, cmp)\
              ((tvp)->tv_sec cmp (uvp)->tv_sec ||\
              (tvp)->tv_sec == (uvp)->tv_sec &&\
              (tvp)->tv_usec cmp (uvp)->tv_usec)
#define       timerclear(tvp)\
              ((tvp)->tv_sec = (tvp)->tv_usec = 0)

/* Wrappers for easy dynamic array element adding and removing. */
#define list_add_element(x, xn, y) {x = (y *)realloc(x, (++(xn))*sizeof(y));}
#define list_rmv_element(x, xn, y) {x = (y *)realloc(x, (--(xn))*sizeof(y));}

struct _kbwin {
    void (*init) (Display *);
    void (*map) (Display *);
    void (*unmap) (Display *);
    void (*draw) (Display *);
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

struct kb *kb = NULL;
unsigned int kb_n = 0;

struct _kbwin kbwin = { NULL, NULL, NULL, NULL, NULL };
struct config conf = { "", 0, 0};
struct instance inst = { 0, 0, NULL, 0 };

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
 * RETURNS <= 0 ? Timed out before getting an event.
 * RETURNS > 0 ? Event catched.
 */
int
XNextEventWithTimeout(Display * display, XEvent * event_return,
                      struct timeval *to)
{
    fd_set fd;
    int r;

    FD_ZERO(&fd);
    FD_SET(XConnectionNumber(display), &fd);

    r = select(FD_SETSIZE, &fd, NULL, NULL, to);

    if (r <= 0) {
        /* Timeout */
        return r;
    }

    XNextEvent(display, event_return);
    return r;
}

void
superkb_addkb(KeySym keysym, unsigned int state,
               enum action_type action_type, const char *command)
{
    list_add_element(kb, kb_n, struct kb);
    kb[kb_n - 1].keycode = XKeysymToKeycode(inst.dpy, keysym);
    kb[kb_n - 1].state = state;
    kb[kb_n - 1].action_type = action_type;
    strcpy(kb[kb_n - 1].command, command);
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

    for (;;) {

        struct timeval hold_start;
        struct timeval hold_end;
        XEvent ev;
        int timed_out;

        timed_out = 0;

        /* Decide wether to use XNextEvent or my own XNextEventWithTimeout
         * and do accordingly. If WithTimeout was used, substract the
         * time actually elapsed to the set timeout. */
        if (!timerisset(&to))
        {
            XNextEvent(inst.dpy, &ev);
        }
        else {
            gettimeofday(&hold_start, NULL);

            if (XNextEventWithTimeout(inst.dpy, &ev, &to) <= 0)
                /* Timed out */
                timed_out = 1;
            else
                timed_out = 0;

            gettimeofday(&hold_end, NULL);
            timerdiff(&to, &hold_start, &hold_end);
        }

        if (timed_out) {
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

                /* Restore saved_autorepeat_mode. */
                XKeyboardControl xkbc;
                /*xkbc.auto_repeat_mode = saved_autorepeat_mode;*/
                xkbc.auto_repeat_mode = AutoRepeatModeOn;
                XChangeKeyboardControl(inst.dpy, KBAutoRepeatMode, &xkbc);

                XUngrabKeyboard(inst.dpy, CurrentTime);
                kbwin.unmap(inst.dpy);
            }
        } else if (ev.type == KeyPress) {
            to.tv_sec = 3;
            to.tv_usec = 0;
        } else if (ev.type == KeyRelease && !ignore_release) {
            /* User might have asked for binding configuration, so ignore key
             * release.
             */
            int i;
            timerclear(&to);
            printf("KeyRelease: %d %d\n", kb[0].keycode, kb[0].state);
            printf("KeyRelease: %d %d\n", ev.xkey.keycode, ev.xkey.state);
            for (i = 0; i < kb_n; i++) {
                if (kb[i].keycode == ev.xkey.keycode &&
                    kb[i].state == (ev.xkey.state & kb[i].state)) {
                    switch (kb[i].action_type) {
                    case AT_COMMAND:
                        system(kb[kb_n - 1].command);
                        break;
                    }
                }
            }

        } else {
            /* According to manual, this should not be necessary. */
            /* XAllowEvents(inst.dpy, ReplayKeyboard, CurrentTime); */
        }
    }
}

int superkb_load(char *display,
                 void (*kbwin_init) (Display *),
                 void (*kbwin_map) (Display *),
                 void (*kbwin_unmap) (Display *),
                 void (*kbwin_draw) (Display *),
                 void (*kbwin_event) (Display *, XEvent ev),
                 const char *kblayout, KeySym key1, KeySym key2)
{
    /* FIXME: Validate parameters. */

    /* Set configuration values. Parameters should be already validated. */
    kbwin.init = kbwin_init;
    kbwin.map = kbwin_map;
    kbwin.unmap = kbwin_unmap;
    kbwin.draw = kbwin_draw;
    kbwin.event = kbwin_event;
    strcpy(conf.kblayout, kblayout);
    conf.key1 = key1;
    conf.key2 = key2;


    /* 2. Connect to display. */
    if (!(inst.dpy = XOpenDisplay(display)))
        return 1;

    inst.key1 = XKeysymToKeycode(inst.dpy, key1);
    inst.key2 = XKeysymToKeycode(inst.dpy, key2);

    inst.rootwin = DefaultRootWindow(inst.dpy);

    /* Create the keyboard window. */
    kbwin.init(inst.dpy);

    XFlush(inst.dpy);

   return 0;
}
