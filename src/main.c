/* License: GPL v2. */
/* To be compiled with:
 *    gcc -o superkb proto.c -ansi -lX11 -L/usr/X11/lib
 */

#include <X11/Xlib.h>

#include <X11/XKBlib.h>
#include <X11/extensions/XKBgeom.h>

#include <gdk-pixbuf-xlib/gdk-pixbuf-xlib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib.h>

#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>
#include <errno.h>

#include "superkb.h"
#include "drawkb.h"

Window w;

Window prev_w_focus;
int prev_w_revert;

XkbDescPtr kbdesc;
XkbGeometryPtr kb_geom;

double scale;
GdkPixbuf *kb;
Font f;
GC gc;

void kbwin_event(Display * dpy, XEvent ev)
{

	if (ev.type == Expose) {
	    KbDrawKeyboard(dpy, w, gc, 0, scale, 0, 0, kbdesc);
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

    XkbQueryExtension(dpy, NULL, NULL, NULL, NULL, NULL);

    kbdesc = XkbGetKeyboard(dpy, XkbAllComponentsMask, XkbUseCoreKbd);

    int status;
    status = XkbGetGeometry(dpy, kbdesc); 

    kb_geom = kbdesc->geom;


    /* unsigned long black = BlackPixel(dpy, DefaultScreen(dpy));*/
    unsigned long white = WhitePixel(dpy, DefaultScreen(dpy));

    int winh = DisplayWidth(dpy, 0);
    int winv = DisplayHeight(dpy, 0);

    double scalew = (float) winh/kb_geom->width_mm;
    double scaleh = (float) winv/kb_geom->height_mm;
    
    if (scalew < scaleh) {
        scale = scalew;
        winv = kb_geom->height_mm * scale;
    } else {
        scale = scaleh;
        winh = kb_geom->width_mm * scale;
    }

    w = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy),
        (DisplayWidth(dpy, 0)-winh)/2, (DisplayHeight(dpy, 0)-winv)/2,
        winh, winv, 0, 0, ((200<<16)+(200<<8)+(220)));

    gc = XCreateGC(dpy, w, 0, NULL);

    XSetWindowAttributes attr;
    attr.override_redirect = True;

    XChangeWindowAttributes(dpy, w, CWOverrideRedirect, &attr);

    XSetTransientForHint(dpy, w, DefaultRootWindow(dpy));

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

