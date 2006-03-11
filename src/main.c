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
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "superkb.h"
#include "drawkb.h"

Window kbwin;
Window confwin;

Window prev_kbwin_focus;
int prev_kbwin_revert;

XkbDescPtr kbdesc;
XkbGeometryPtr kb_geom;

double scale;
GdkPixbuf *kb;

GC gc;

Display *dpy;

/* Wrappers for easy dynamic array element adding and removing. */
#define list_add_element(x, xn, y) {x = (y *)realloc(x, (++(xn))*sizeof(y));}
#define list_rmv_element(x, xn, y) {x = (y *)realloc(x, (--(xn))*sizeof(y));}


enum action_type {
    AT_COMMAND = 1,
    AT_FUNCTION
};


/* key_bindings is a dynamic list of keybindings */
struct key_bindings {
    KeyCode keycode;            /* Like in XKeyEvent. */
    unsigned int state;         /* Like in XKeyEvent. */
    unsigned int statemask;
    enum action_type action_type;
    union {
        void (*function)(void *p);
        char *command;
    } action;
    char *icon;
    /* FIXME: Implement startup notification. */
    /* FIXME: Implement tooltips. */
} *key_bindings = NULL;

unsigned int key_bindings_n = 0;

void IconQuery(KeySym keysym, unsigned int state, char buf[], int buf_n)
{
    int i;
    for (i = 0; i < key_bindings_n; i++)
    {
        if (keysym == XKeycodeToKeysym(dpy, key_bindings[i].keycode, 0)) {
            strncpy(buf, key_bindings[i].icon, buf_n);
        }
    }
}

void kbwin_event(Display * dpy, XEvent ev)
{

    if (ev.type == Expose) {
        KbDrawKeyboard(dpy, kbwin, gc, 0, scale, 0, 0, kbdesc, IconQuery);
        XFlush(dpy);
    } else if (ev.type == VisibilityNotify &&
               ev.xvisibility.state != VisibilityUnobscured) {
        XRaiseWindow(dpy, kbwin);
    }

}

void kbwin_map(Display * dpy)
{
    /* XGetInputFocus(dpy, &prev_kbwin_focus, &prev_kbwin_revert); */
    XMapWindow(dpy, kbwin);
}

void kbwin_unmap(Display * dpy)
{
    XUnmapWindow(dpy, kbwin);
    /* XSetInputFocus(dpy, prev_kbwin_focus, prev_kbwin_revert, CurrentTime); */
}

void kbwin_init(Display * dpy)
{

    gdk_pixbuf_xlib_init(dpy, 0);

/*  kb = gdk_pixbuf_new_from_file ("superkb.png", NULL); */

/*  assert (kb); */

    XkbQueryExtension(dpy, NULL, NULL, NULL, NULL, NULL);

    kbdesc = XkbGetKeyboard(dpy, XkbAllComponentsMask, XkbUseCoreKbd);

    int status;
    status = XkbGetGeometry(dpy, kbdesc);

    kb_geom = kbdesc->geom;

    /* unsigned long black = BlackPixel(dpy, DefaultScreen(dpy)); */
    /* unsigned long white = WhitePixel(dpy, DefaultScreen(dpy)); */

    int winh = DisplayWidth(dpy, 0);
    int winv = DisplayHeight(dpy, 0);

    double scalew = (float) winh / kb_geom->width_mm;
    double scaleh = (float) winv / kb_geom->height_mm;

    if (scalew < scaleh) {
        scale = scalew;
        winv = kb_geom->height_mm * scale;
    } else {
        scale = scaleh;
        winh = kb_geom->width_mm * scale;
    }

    kbwin = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy),
                            (DisplayWidth(dpy, 0) - winh) / 2,
                            (DisplayHeight(dpy, 0) - winv) / 2, winh, winv,
                            0, 0, ((200 << 16) + (200 << 8) + (220)));

    gc = XCreateGC(dpy, kbwin, 0, NULL);

    XSetWindowAttributes attr;
    attr.override_redirect = True;

    XChangeWindowAttributes(dpy, kbwin, CWOverrideRedirect, &attr);

    XSetTransientForHint(dpy, kbwin, DefaultRootWindow(dpy));

    XSelectInput(dpy, kbwin, ExposureMask | VisibilityChangeMask);

    XFlush(dpy);

}

void __Superkb_Action(KeyCode keycode, unsigned int state)
{
    int i;
    for (i = 0; i < key_bindings_n; i++) {
        if (key_bindings[i].keycode == keycode &&
          key_bindings[i].state ==
          (state & key_bindings[i].state)) {
            switch (key_bindings[i].action_type) {
            case AT_COMMAND:
                if (fork() == 0) {
                    system(key_bindings[i].action.command);
                    exit(EXIT_SUCCESS);
                }
                break;
            case AT_FUNCTION:
                if (fork() == 0) {
                    /* FIXME: Value should not be NULL. */
                    key_bindings[i].action.function(NULL);
                    exit(EXIT_SUCCESS);
                }

            }
        }
    }
}

void
add_binding(KeySym keysym, unsigned int state,
              enum action_type action_type, const char *command,
              const char *icon)
{
    list_add_element(key_bindings, key_bindings_n, struct key_bindings);
    key_bindings[key_bindings_n - 1].keycode =
        XKeysymToKeycode(dpy, keysym);
    key_bindings[key_bindings_n - 1].state = state;
     key_bindings[key_bindings_n - 1].action_type = action_type;
    key_bindings[key_bindings_n - 1].action.command = malloc(strlen(command)+1);
    key_bindings[key_bindings_n - 1].icon = malloc(strlen(icon)+1);
    strcpy(key_bindings[key_bindings_n - 1].action.command, command);
    strcpy(key_bindings[key_bindings_n - 1].icon, icon);
}


int main()
{

    g_type_init();

    /* 2. Connect to display. */
    if (!(dpy = XOpenDisplay(NULL)))
    {
        fprintf(stderr, "Couldn't open display.\n");
        return EXIT_FAILURE;
    }

    superkb_load(dpy, kbwin_init, kbwin_map, kbwin_unmap, kbwin_event,
                 "en", XStringToKeysym("Super_R"),
                 XStringToKeysym("Super_L"), __Superkb_Action);
    add_binding(XStringToKeysym("n"), 0, AT_COMMAND, "/usr/bin/gedit", "/usr/share/pixmaps/gedit-icon.png");
    add_binding(XStringToKeysym("t"), 0, AT_COMMAND, "/usr/X11R6/bin/xterm", "/usr/share/pixmaps/gedit-icon.png");
    add_binding(XStringToKeysym("c"), 0, AT_COMMAND, "/usr/bin/gcalctool", "/usr/share/pixmaps/gnome-calc2.png");
    superkb_start();

    return EXIT_SUCCESS;
}
