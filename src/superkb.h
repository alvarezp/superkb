/* License: GPL. */
/* To be compiled with:
 *    gcc -o superkb proto.c -ansi -lX11 -L/usr/X11/lib
 */

#ifndef __SUPERKB_H
#define __SUPERKB_H

#include <X11/Xlib.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>
#include <errno.h>

enum action_type {
    AT_COMMAND = 1
};

/* superkb_kb is a dynamic list of keybindings */
struct superkb_kb {
    KeyCode keycode;       /* Like in XKeyEvent. */
    unsigned int state;         /* Like in XKeyEvent. */
    unsigned int statemask;
    enum action_type action_type;
    char command[2048];
    /* FIXME: Implement icons. */
    /* FIXME: Implement startup notification. */
    /* FIXME: Implement tooltips. */
};

void
superkb_addkb(KeySym keysym, unsigned int state,
               enum action_type action_type, const char *command);

void superkb_start();

int superkb_load(char *display,
                 void (*kbwin_init) (Display *),
                 void (*kbwin_map) (Display *),
                 void (*kbwin_unmap) (Display *),
                 void (*kbwin_event) (Display *, XEvent ev),
                 const char *kblayout, KeySym key1, KeySym key2);

#endif /* __SUPERKB_H */
