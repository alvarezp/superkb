/* License: GPL. */
/* To be compiled with:
 *    gcc -o superkb proto.c -ansi -lX11 -L/usr/X11/lib
 */

#include <X11/Xlib.h>

#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>
#include <errno.h>

#include "superkb.h"

Window w;

void kbwin_event(Display * dpy, XEvent ev)
{

	if (ev.type == Expose) {
	    /* DrawKeyboard(); */
    }

}

void kbwin_map (Display *dpy)
{
    XMapWindow(dpy, w);
}

void kbwin_unmap (Display *dpy)
{
    XUnmapWindow(dpy, w);
}

void kbwin_init (Display *dpy)
{

    /* Conectar e inicializar */
    /* Font f = XLoadFont(dpy, "*-bitstream vera sans-bold-r-*");*/

    /* unsigned long black = BlackPixel(dpy, DefaultScreen(dpy));*/
    unsigned long white = WhitePixel(dpy, DefaultScreen(dpy));
    unsigned long bgcolor = (128 << 16) + (148 << 8) + 220;

    /* Crear la ventana con el tecladito: w */
    /* XSetWindowAttributes attr; */

    w = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 400, 400, 400, 300,
        0, white, bgcolor);

    XSelectInput(dpy, w, ExposureMask);

    XFlush(dpy);

}

void kbwin_draw (Display *dpy)
{
  return;
}

int main()
{

    superkb_load(NULL, kbwin_init, kbwin_map, kbwin_unmap, kbwin_draw,
        kbwin_event, "en", XStringToKeysym("Super_L"),
        XStringToKeysym("Super_R"));
    superkb_addkb(XStringToKeysym("n"), 0, AT_COMMAND, "/usr/bin/gedit");
    superkb_addkb(XStringToKeysym("space"), 0, AT_COMMAND, "/usr/bin/gcalctool");
    superkb_start();

    return EXIT_SUCCESS;
}
