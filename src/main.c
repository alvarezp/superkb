/* License: GPL. */
/* To be compiled with:
 *    gcc -o superkb proto.c -ansi -lX11 -L/usr/X11/lib
 */

#include <X11/Xlib.h>

#include <gdk-pixbuf-xlib/gdk-pixbuf-xlib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib.h>

#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>
#include <errno.h>

#include "superkb.h"

Window w;

Window prev_w_focus;
int prev_w_revert;

GdkPixbuf *kb;
Font f;
GC gc;

void DrawKeyboard()
{
    gdk_pixbuf_xlib_render_to_drawable(kb, w, gc, 0, 0, 0, 0, 200, 300,
      XLIB_RGB_DITHER_NORMAL, 0, 0);
    return;
}

void kbwin_event(Display * dpy, XEvent ev)
{

	if (ev.type == Expose) {
	    DrawKeyboard();
        XFlush(dpy);
    } else if (ev.type == VisibilityNotify &&
      ev.xvisibility.state != VisibilityUnobscured ) {
        XRaiseWindow(dpy, w);
    }

}

void kbwin_map (Display *dpy)
{
    /* XGetInputFocus(dpy, &prev_w_focus, &prev_w_revert); */
    XMapWindow(dpy, w);
}

void kbwin_unmap (Display *dpy)
{
    XUnmapWindow(dpy, w);
    /* XSetInputFocus(dpy, prev_w_focus, prev_w_revert, CurrentTime); */
}

void kbwin_init (Display *dpy)
{

    gdk_pixbuf_xlib_init(dpy, 0);

    kb = gdk_pixbuf_new_from_file("superkb.png", NULL);

    assert(kb);

    f = XLoadFont(dpy, "*-bitstream vera sans-bold-r-*");

    /* unsigned long black = BlackPixel(dpy, DefaultScreen(dpy));*/
    unsigned long white = WhitePixel(dpy, DefaultScreen(dpy));
    unsigned long bgcolor = (128 << 16) + (148 << 8) + 220;

    w = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 400, 400, 400, 300,
      0, white, bgcolor);

    gc = XCreateGC(dpy, w, 0, NULL);

    XSetWindowAttributes attr;
    attr.override_redirect = True;

    XChangeWindowAttributes(dpy, w, CWOverrideRedirect, &attr);

    XSelectInput(dpy, w, ExposureMask | VisibilityChangeMask);

    XFlush(dpy);

}

int main()
{

    g_type_init();

    superkb_load(NULL, kbwin_init, kbwin_map, kbwin_unmap, kbwin_event,
        "en", XStringToKeysym("Super_L"),
        XStringToKeysym("Super_R"));
    superkb_addkb(XStringToKeysym("n"), 0, AT_COMMAND, "/usr/bin/gedit");
    superkb_addkb(XStringToKeysym("space"), 0, AT_COMMAND,
      "/usr/bin/gcalctool");
    superkb_start();

    return EXIT_SUCCESS;
}

