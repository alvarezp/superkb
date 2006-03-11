/* License: GPL. */
/* To be compiled with:
 *    gcc -o superkb proto.c -ansi -lX11 -L/usr/X11/lib
 *
 * Bibliography: XKBlib.pdf.
 */

#ifndef __DRAWKB_H
#define __DRAWKB_H

typedef void (*IconQueryFunc)(KeySym keysym, unsigned int state, char buf[],
    int buf_n);

void KbDrawKeyboard(Display * dpy, Drawable w, GC gc,
                    unsigned int parent_angle, double scale,
                    unsigned int left, unsigned int top, XkbDescPtr _kb,
                    IconQueryFunc iqf);

#endif
