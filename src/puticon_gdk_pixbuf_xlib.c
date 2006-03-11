/* License: GPL v2. */

#include <gdk-pixbuf-xlib/gdk-pixbuf-xlib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib.h>

void LoadAndDrawImage(Display *dpy, GC gc, Window w, int left, int top, int width, int height, char *buf, int buf_n)
{
    GdkPixbuf *pb = gdk_pixbuf_new_from_file (buf, NULL);

    GdkPixbuf *scaled = gdk_pixbuf_scale_simple(pb, width, height, GDK_INTERP_BILINEAR);

    gdk_pixbuf_xlib_render_to_drawable_alpha(scaled, w, 0, 0, left, top, width, height, GDK_PIXBUF_ALPHA_FULL, 255, XLIB_RGB_DITHER_NORMAL, 0, 0);

    gdk_pixbuf_unref(pb);
}
