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

extern double drawkb_delay;

void superkb_start();

int superkb_load(Display *display,
             void (*kbwin_init) (Display *),
             void (*kbwin_map) (Display *),
             void (*kbwin_unmap) (Display *),
             void (*kbwin_event) (Display *, XEvent ev),
             const char *kblayout, KeySym key1, KeySym key2,
             void (*f)(KeyCode keycode, unsigned int state));

#endif                          /* __SUPERKB_H */
