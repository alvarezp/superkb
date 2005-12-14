/* License: GPL. */
/* To be compiled with:
 *    gcc -o superkb proto.c -ansi -lX11 -L/usr/X11/lib
 */

#include <X11/Xlib.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>
#include <errno.h>

#define       timerisset(tvp)\
              ((tvp)->tv_sec || (tvp)->tv_usec)
#define       timercmp(tvp, uvp, cmp)\
              ((tvp)->tv_sec cmp (uvp)->tv_sec ||\
              (tvp)->tv_sec == (uvp)->tv_sec &&\
              (tvp)->tv_usec cmp (uvp)->tv_usec)
#define       timerclear(tvp)\
              ((tvp)->tv_sec = (tvp)->tv_usec = 0)

struct config {
  void (*_DrawKeyboard)(Display *, Window, GC, Pixmap, int, int, int);
};

struct config c;

void DrawKeyboard(Display * dpy, Window w, GC gc, Pixmap kb_pix, int pmw,
		  int pmh, int ix1)
{
    XCopyArea(dpy, kb_pix, w, gc, 0, 0, pmw, pmh, 0, 0);
    XCopyArea(dpy, kb_pix, w, gc, 0, 0, pmw, pmh, pmw * 1.8, 0);
    XCopyArea(dpy, kb_pix, w, gc, 0, 0, pmw, pmh, pmw * 1.8 + pmw, 0);
    XCopyArea(dpy, kb_pix, w, gc, 0, 0, pmw, pmh, pmw * 1.8 + 2 * pmw, 0);
    XCopyArea(dpy, kb_pix, w, gc, 0, 0, pmw, pmh, pmw * 1.8 + 3 * pmw, 0);

    XCopyArea(dpy, kb_pix, w, gc, 0, 0, pmw, pmh, 0, pmh * 1.6);
    XCopyArea(dpy, kb_pix, w, gc, 0, 0, pmw, pmh, pmw, pmh * 1.6);
    XCopyArea(dpy, kb_pix, w, gc, 0, 0, pmw, pmh, pmw * 2, pmh * 1.6);
    XCopyArea(dpy, kb_pix, w, gc, 0, 0, pmw, pmh, pmw * 3, pmh * 1.6);

    XCopyArea(dpy, kb_pix, w, gc, 0, 0, pmw, pmh, 0, pmh * 1.6 + pmh);
    XCopyArea(dpy, kb_pix, w, gc, ix1 + 1, 0, pmw, pmh, ix1 * 4,
	      pmh * 1.6 + pmh);
    XCopyArea(dpy, kb_pix, w, gc, 0, 0, pmw, pmh,
	      ix1 * 3 + pmw - 1, pmh * 1.6 + pmh);
    XCopyArea(dpy, kb_pix, w, gc, 0, 0, pmw, pmh,
	      ix1 * 3 + pmw * 2 - 1, pmh * 1.6 + pmh);

    XCopyArea(dpy, kb_pix, w, gc, 0, 0, pmw, pmh, 0, pmh * 1.6 + pmh * 2);
    XCopyArea(dpy, kb_pix, w, gc, ix1 + 1, 0, pmw, pmh,
	      ix1 * 8, pmh * 1.6 + pmh * 2);
    XCopyArea(dpy, kb_pix, w, gc, 0, 0, pmw, pmh,
	      ix1 * 7 + pmw - 1, pmh * 1.6 + pmh * 2);
    XCopyArea(dpy, kb_pix, w, gc, 0, 0, pmw, pmh,
	      ix1 * 7 + pmw * 2 - 1, pmh * 1.6 + pmh * 2);

    XCopyArea(dpy, kb_pix, w, gc, 0, 0, pmw, pmh, 0, pmh * 1.6 + pmh * 3);
    XCopyArea(dpy, kb_pix, w, gc, ix1 + 1, 0, pmw, pmh,
	      ix1 * 8, pmh * 1.6 + pmh * 3);
    XCopyArea(dpy, kb_pix, w, gc, ix1 + 1, 0, pmw, pmh,
	      ix1 * 12, pmh * 1.6 + pmh * 3);
    XCopyArea(dpy, kb_pix, w, gc, 0, 0, pmw, pmh,
	      ix1 * 11 + pmw - 1, pmh * 1.6 + pmh * 3);
    XCopyArea(dpy, kb_pix, w, gc, 0, 0, pmw, pmh,
	      ix1 * 11 + pmw * 2 - 1, pmh * 1.6 + pmh * 3);

    XDrawString(dpy, w, gc, ix1 * 7 + pmw + pmw / 2,
		pmh * 1.6 + pmh * 2 + pmh / 2, "A", 1);
    XDrawString(dpy, w, gc, 5, 295,
		"Hold down combination to reconfigure.",
		strlen("Hold down combination to reconfigure."));
    XFlush(dpy);
}

void superkb_init()
{

}

static void timerdiff(struct timeval *dst, struct timeval *tv0,
		      struct timeval *tv1)
{
    if (tv1->tv_usec >= tv0->tv_usec) {
	dst->tv_usec = tv1->tv_usec - tv0->tv_usec;
	dst->tv_sec = tv1->tv_sec - tv0->tv_sec;
    } else {
	dst->tv_usec = tv1->tv_usec - tv0->tv_usec + 1000000;
	dst->tv_sec = tv1->tv_sec - tv0->tv_usec - 1;
    }
}

int XNextEventWithTimeout(Display * display, XEvent * event_return,
			  struct timeval *tv)
{
    fd_set fd;
    int r;

    FD_ZERO(&fd);
    FD_SET(XConnectionNumber(display), &fd);

    r = select(FD_SETSIZE, &fd, NULL, NULL, tv);

    if (r <= 0) {
	/* Timeout */
	return r;
    }

    XNextEvent(display, event_return);
    return r;
}

int main()
{

    /* Conectar e inicializar */
    Display *dpy;
    Window root;
    if (!(dpy = XOpenDisplay(NULL)))
	return 1;
    Font f = XLoadFont(dpy, "*-bitstream vera sans-bold-r-*");
    Colormap cm = DefaultColormap(dpy, 0);
    root = DefaultRootWindow(dpy);

    KeyCode super_l = XKeysymToKeycode(dpy, XStringToKeysym("Super_L"));
    KeyCode super_r = XKeysymToKeycode(dpy, XStringToKeysym("Super_R"));
    KeyCode KB_A = XKeysymToKeycode(dpy, XStringToKeysym("A"));

    unsigned long black = BlackPixel(dpy, DefaultScreen(dpy));
    unsigned long white = WhitePixel(dpy, DefaultScreen(dpy));
    unsigned long bgcolor = (0 << 16) + (20 << 8) + 220;

    /* Establecer la función de pintado del teclado */
    _DrawKeyboard=DrawKeyboard;

    /* Crear la ventana con el tecladito: w */
    Window w;
    XSetWindowAttributes attr;

    w = XCreateSimpleWindow(dpy, root, 400, 400, 400, 300, 0, white,
			    bgcolor);
    assert(w);
    attr.override_redirect = True;	/* Adios WM */
    XChangeWindowAttributes(dpy, w, CWOverrideRedirect, &attr);

    /* Crearle un contexto grafico a w. */
    GC gc = XCreateGC(dpy, w, 0, NULL);
    XSetFont(dpy, gc, f);

    /* Solicitar los eventos */
    XGrabKey(dpy, super_l, AnyModifier, root, True, GrabModeAsync,
	     GrabModeAsync);
    XGrabKey(dpy, super_r, AnyModifier, root, True, GrabModeAsync,
	     GrabModeAsync);
    XSelectInput(dpy, w, ExposureMask);

    /* Crear el pixmap de la teclita */
    int pmw = 45, pmh = 45;
    int ox1 = 0, oy1 = 0, ox2 = 44, oy2 = 44;
    int ix1 = 4, iy1 = 2, ix2 = 40, iy2 = 38;
    Screen *s = XDefaultScreenOfDisplay(dpy);
    Pixmap kb_pix =
	XCreatePixmap(dpy, w, pmw, pmh, XDefaultDepthOfScreen(s));
    assert(kb_pix);

    XSetForeground(dpy, gc, black);
    XFillRectangle(dpy, kb_pix, gc, 0, 0, pmw, pmh);
    XSetForeground(dpy, gc, white);
    XDrawRectangle(dpy, kb_pix, gc, ox1, oy1, ox2 - ox1, oy2 - oy1);	/* outer box */
    XDrawRectangle(dpy, kb_pix, gc, ix1, iy1, ix2 - ix1, iy2 - iy1);	/* inner box */
    XDrawLine(dpy, kb_pix, gc, ox1, oy1, ix1, iy1);	/* rest of lines */
    XDrawLine(dpy, kb_pix, gc, ox1, oy2, ix1, iy2);
    XDrawLine(dpy, kb_pix, gc, ox2, oy1, ix2, iy1);
    XDrawLine(dpy, kb_pix, gc, ox2, oy2, ix2, iy2);

    XFlush(dpy);

    struct timeval to = { 0, 0 };
    int ignore_release = 0;
    for (;;) {

	int still_pressed = 0;
	int saved_autorepeat_mode;
	struct timeval hold_start;
	struct timeval hold_end;
	XEvent ev;
	int timed_out;

	timed_out = 0;

	if (!timerisset(&to))
	    XNextEvent(dpy, &ev);

	else {
	    gettimeofday(&hold_start, NULL);

	    if (!XNextEventWithTimeout(dpy, &ev, &to))
		/* Timed out */
		timed_out = 1;
	    else
		timed_out = 0;

	    gettimeofday(&hold_end, NULL);
	    timerdiff(&to, &hold_start, &hold_end);
	}

	if (timed_out) {
	    XDrawString(dpy, w, gc, 5, 275,
			"User asked for MENU+A configuration.",
			strlen("User asked for MENU+A configuration."));
	    printf("CONFIGURE BINDING!\n");
	    timerclear(&to);
	    ignore_release = 1;
	} else if (ev.type == KeyPress && ev.xkey.subwindow != None) {
	    if (ev.xkey.keycode == super_l || ev.xkey.keycode == super_r) {
		ignore_release = 0;

		XMapWindow(dpy, w);
		XKeyboardState xkbs;
		XGetKeyboardControl(dpy, &xkbs);
		saved_autorepeat_mode = xkbs.global_auto_repeat;
		XKeyboardControl xkbc;
		xkbc.auto_repeat_mode = AutoRepeatModeOff;
		XChangeKeyboardControl(dpy, KBAutoRepeatMode, &xkbc);

		XGrabKey(dpy, KB_A, Mod4Mask, root, True,
			 GrabModeAsync, GrabModeAsync);
	    } else {
		to.tv_sec = 3;
		to.tv_usec = 0;
	    }
	} else if (ev.type == KeyRelease && ev.xkey.subwindow != None) {
	    XKeyboardControl xkbc;
	    xkbc.auto_repeat_mode = AutoRepeatModeOn;
	    XChangeKeyboardControl(dpy, KBAutoRepeatMode, &xkbc);

	    if (!ignore_release) {
		ignore_release = 1;

                /* pseudo-code: if (action_key == KB_A) { */
		XDrawString(dpy, w, gc, 5, 275,
			    "User asked to run gedit.",
			    strlen("User asked to run gedit."));
		printf("RUN APP!\n");
		/*system("/usr/bin/gedit");*/
		timerclear(&to);
		/* } */
	    }
	    XUngrabKey(dpy, KB_A, Mod4Mask, root);
	    XUnmapWindow(dpy, w);
	} else if (ev.type == Expose) {
	    _DrawKeyboard(dpyg, w, gc, kb_pix, pmw, pmh, ix1);
	} else
	    XAllowEvents(dpy, ReplayKeyboard, CurrentTime);
    }
}
