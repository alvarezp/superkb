/*
 * drawkb.c
 *
 * Copyright (C) 2006, Octavio Alvarez Piza.
 * License: GNU General Public License v2.
 *
 */

/*
 * Bibliography: XKBlib.pdf.
 */

/* This module does all the keyboard drawing magic. */

/* If ever needed to call XkbComputeShapeBounds() you must immediately
 * call WorkaroundBoundsBug() afterwards. This works around a bug present
 * in Xorg < 7.1.
 */

#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <dlfcn.h>

#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include <X11/Xft/XftCompat.h>

#include <X11/XKBlib.h>
#include <X11/extensions/XKBgeom.h>

#include <X11/extensions/Xrender.h>

#include "drawkb.h"
#include "imagelib.h"
#include "debug.h"
#include "globals.h"

#define LINE_WIDTH 2

#define pxl (1 / scale)

#define DEFAULT_SCREEN(dpy) (DefaultScreen(dpy))
#define DEFAULT_VISUAL(dpy) (DefaultVisual(dpy, DEFAULT_SCREEN(dpy)))
#define DEFAULT_COLORMAP(dpy) (DefaultColormap(dpy, DEFAULT_SCREEN(dpy)))

double __scale;

typedef struct {
	char font[500];
	XftFont * fs;
	Display *dpy;
	IQF_t IQF;
	painting_mode_t painting_mode;
} drawkb_config_t;

drawkb_config_t drawkb_config;

XkbDescPtr kbdesc;
XkbGeometryPtr kb_geom;

IQF_t IQF;

int g_size;
double g_baseline;

XColor lightcolor;
XColor darkcolor;
XColor foreground;
XColor background;

XftColor xftlightcolor;
XftColor xftdarkcolor;
XftColor xftbackground;
XftColor xftforeground;

XftColor *current;

XftDraw *dw = NULL;

typedef struct {
	char *keystring;
	char *keylabel;
} keystrings_t;

keystrings_t keystrings[] = {
	{ "dead_grave",               "`" },
	{ "dead_acute",               "'" },
	{ "grave",                    "`" },
	{ "apostrophe",               "'" },
	{ "space",                    "" },
	{ "Escape",                   "Esc" },
	{ "comma",                    "," },
	{ "period",                   "." },
	{ "slash",                    "/" },
	{ "minus",                    "-" },
	{ "equal",                    "=" },
	{ "Caps_Lock",                "Caps Lock" },
	{ "Shift_L",                  "Shift" },
	{ "Shift_R",                  "Shift" },
	{ "semicolon",                ";" },
	{ "BackSpace",                "Backspace" },
	{ "Return",                   "Enter" },
	{ "Control_L",                "Ctrl" },
	{ "Control_R",                "Ctrl" },
	{ "Alt_L",                    "Alt" },
	{ "KP_Enter",                 "Enter" },
	{ "KP_Add",                   "+" },
	{ "KP_Subtract",              "-" },
	{ "KP_Multiply",              "*" },
	{ "KP_Divide",                "/" },
	{ "Num_Lock",                 "NumLk" },
	{ "KP_Home",                  "Home" },
	{ "KP_End",                   "End" },
	{ "KP_Prior",                 "PgUp" },
	{ "KP_Up",                    "Up" },
	{ "KP_Down",                  "Down" },
	{ "KP_Left",                  "Left" },
	{ "KP_Right",                 "Right" },
	{ "KP_Next",                  "Next" },
	{ "KP_Begin",                 "Begin" },
	{ "KP_Insert",                "Ins" },
	{ "KP_Delete",                "Del" },
	{ "Scroll_Lock",              "ScrLk" },
	{ "bracketleft",              "[" },
	{ "bracketright",             "]" },
	{ "braceleft",                "{" },
	{ "braceright",               "}" },
	{ "backslash",                "\\" },
	{ "ntilde",                   "ñ" },
	{ "plus",                     "+" },
	{ "ISO_Level3_Shift",         "AltGr" },
	{ "Insert",                   "Ins" },
	{ "Delete",                   "Del" },
	{ "Prior",                    "PgUp" },
	{ "Next",                     "PgDn" },
	{ "questiondown",             "¿" },
	{ "Print",                    "PrScr" },
	{ "", "" }
};

typedef struct {
	unsigned int index;
	XkbBoundsRec labelbox;
	unsigned int size;
	const char *glyph;
} key_data_t;

char *LookupKeylabelFromKeystring(char *kss) {
	int i = 0;
	while (strcmp((keystrings[i]).keystring, "") != 0) {
		if (!strcmp(kss, (keystrings[i]).keystring))
			return (keystrings[i]).keylabel;
		i++;
	}
	return kss;
}

int MyXftTextWidth(Display *dpy, XftFont *fs, const char *s, int len) {
	XGlyphInfo xgi;
	XftTextExtents8(dpy, fs, (unsigned char *) s, len, &xgi);
	return xgi.xOff;
}

/* FIXME: Same problem as XLoadQueryScalableFont(). It doesn't check
 * for i < 500*sizeof(char).
 */
int XSetFontNameToScalable(const char *name, char *newname, int newname_n)
{
	int i, j, field;
	/* catch obvious errors */
	if ((name == NULL) || (name[0] != '-'))
		return 0;
	/* copy the font name, changing the scalable fields as we do so */

	/* FIXME: "&& i < buf_n - 1": better safe than sorry (i < buf_n?) */
	for (i = j = field = 0;
		 name[i] != '\0' && field <= 14 && i < newname_n - 1; i++) {
		newname[j++] = name[i];
		if (name[i] == '-') {
			field++;
			switch (field) {
			case 7:			/* pixel size */
			case 12:		   /* average width */
				/* change from "-whatever-" to "-0-" */
				newname[j] = '0';
				j++;
				while (name[i + 1] != '\0' && name[i + 1] != '-')
					i++;
				break;
			case 8:			/* point size */
				/* change from "-whatever-" to "-0-" */
				newname[j] = '0';
				j++;
				while (name[i + 1] != '\0' && name[i + 1] != '-')
					i++;
				break;
			case 9:			/* x-resolution */
			case 10:		   /* y-resolution */
				/* change from "-whatever-" to "-*-" */
				newname[j] = '*';
				j++;
				while (name[i + 1] != '\0' && name[i + 1] != '-')
					i++;
				break;
			}
		}
	}
	newname[j] = '\0';
	/* if there aren't 14 hyphens, it isn't a well formed name */
	return field;
}

/* TAKEN FROM O'REILLY XLIB PROGRAMMING MANUAL.
 *
 * This routine is passed a scalable font name and a point size.  It returns
 * an XftFont * for the given font scaled to the specified size and the
 * exact resolution of the screen.  The font name is assumed to be a
 * well-formed XLFD name, and to have pixel size, point size, and average
 * width fields of "0" and arbitrary x-resolution and y-resolution fields.
 * Size is specified in tenths of points.  Returns NULL if the name is
 * malformed or no such font exists.
 *
 * FIXME: It doesn't check for i < 500*sizeof(char).
 */
XftFont *XLoadQueryScalableFont(Display * dpy, int screen, char *name,
	int size)
{
	XftFont *fs;
	int i, j, field;
	char newname[500];		  /* big enough for a long font name */
	int res_x, res_y;		   /* resolution values for this screen */
	/* catch obvious errors */
	if ((name == NULL) || (name[0] != '-'))
		return NULL;
	/* calculate our screen resolution in dots per inch. 25.4mm = 1 inch */
	res_x =
		DisplayWidth(dpy, screen) / (DisplayWidthMM(dpy, screen) / 25.4);
	res_y =
		DisplayHeight(dpy, screen) / (DisplayHeightMM(dpy, screen) / 25.4);
	/* copy the font name, changing the scalable fields as we do so */
	for (i = j = field = 0; name[i] != '\0' && field <= 14; i++) {
		newname[j++] = name[i];
		if (name[i] == '-') {
			field++;
			switch (field) {
			case 8:			/* pixel size */
			case 12:		   /* average width */
				/* change from "-0-" to "-*-" */
				newname[j] = '*';
				j++;
				if (name[i + 1] != '\0')
					i++;
				break;
			case 7:			/* point size */
				/* change from "-0-" to "-<size>-" */
				sprintf(&newname[j], "%d", size);
				while (newname[j] != '\0')
					j++;
				if (name[i + 1] != '\0')
					i++;
				break;
			case 9:			/* x-resolution */
			case 10:		   /* y-resolution */
				/* change from an unspecified resolution to res_x or res_y */
				sprintf(&newname[j], "%d", (field == 9) ? res_x : res_y);
				while (newname[j] != '\0')
					j++;
				while ((name[i + 1] != '-') && (name[i + 1] != '\0'))
					i++;
				break;
			}
		}
	}
	newname[j] = '\0';
	/* if there aren't 14 hyphens, it isn't a well formed name */
	if (field != 14)
		return NULL;
	fs = XftFontOpenXlfd(dpy, 0, newname);
	return fs;
}

int drawkb_set_font(Display *dpy, const char * font)
{

	/* FIXME: Validate font name. */

	/* FILLTHEGAP: Support font-family-only values. */

	XSetFontNameToScalable(font, drawkb_config.font, 500);

	char * ptr;
	ptr = realloc(drawkb_config.font, strlen(font));
	if (!ptr)
		return EXIT_FAILURE;

	strcpy(drawkb_config.font, font);
	return EXIT_SUCCESS;

	XftFont *fs;
	fs = XLoadQueryScalableFont(drawkb_config.dpy, 0, drawkb_config.font, 1000);
	if (fs) {
		return EXIT_SUCCESS;
	}

	XftFontClose(dpy, fs);
}

void drawkb_set_dpy(Display *dpy)
{
	drawkb_config.dpy = dpy;
}

void WorkaroundBoundsBug(Display * dpy, XkbDescPtr _kb)
{
	int i, j;

	/* To workaround an X11R7.0 and previous bug */
	if (VendorRelease(dpy) < 70100000 &&
		!strcmp(ServerVendor(dpy), "The X.Org Foundation"))
	{
		for (i = 0; i < _kb->geom->num_shapes; i++)
		{
			XkbShapePtr s;	  /* shapes[i] */
			s = &_kb->geom->shapes[i];
			for (j = 0; j < s->num_outlines; j++)
			{
				if (s->outlines[j].num_points == 1)
				{
					s->bounds.x1 = s->bounds.y1 = 0;
				}
			}
		}
	}
}

void
RotatePoint(double left, double top, double angle, double rot_left,
			double rot_top, double *new_left, double *new_top)
{

	if (angle == 0) {
		*new_left = left;
		*new_top = top;
		return;
	}

	double r;
	double a;

	const double PI = 3.14159265358979323846;

	double l = left - rot_left;
	double t = top - rot_top;

	r = sqrt(l * l + t * t);

	if (l == 0) {
		if (t > 0) {
			a = PI * 1 / 2;
		} else if (t == 0) {
			a = 0;
		} else {
			a = PI * 3 / 2;
		}
	} else {
		a = atan((double) t / l);
	}

	if (new_left != NULL)
		*new_left =
			(double) rot_left + (double) r * cos((double) a + (double) angle / 1800 * PI);
	if (new_top != NULL)
		*new_top =
			(double) rot_top + (double) r * sin((double) a + (double) angle / 1800 * PI);

}

void
RotateArc(double left, double top, double width, double height,
		  double start, double end, double angle, double rot_left,
		  double rot_top, double *new_x, double *new_y,
		  double *new_width, double *new_height,
		  double *new_start, double *new_end)
{
	
	double center_x, center_y;

	center_x = left + width / 2 - rot_left;
	center_y = top + height / 2 - rot_top;

	RotatePoint(center_x, center_y, angle, 0, 0, &center_x, &center_y);

	if (new_x)
		*new_x = rot_left + center_x - width / 2;
	if (new_y)
		*new_y = rot_top + center_y - height / 2;

	if (new_start)
		*new_start = start - 6.4 * angle / 10;
	if (new_end)
		*new_end = end - 6.4 * angle / 10;

	/* FIXME: Since these are always the same as the input values, function
	 * can be significantly reduced. */
	if (new_width)
		*new_width = width;
	if (new_height)
		*new_height = height;

}

void
KbDrawBounds(Display * dpy, Drawable w, GC gc, unsigned int angle,
			 double scale, unsigned int left, unsigned int top,
			 XkbDescPtr _kb, XkbBoundsPtr bounds)
{
	GC gc_backup;

	/* FIXME: I know, there must be faster ways, but this was faster to code. */
	memcpy(&gc_backup, &gc, sizeof(GC));

	XSetLineAttributes(dpy, gc, LINE_WIDTH, LineOnOffDash, CapButt, JoinMiter);
	XDrawRectangle(dpy, w, gc, scale * (left + bounds->x1),
				   scale * (top + bounds->y1),
				   scale * (bounds->x2 - bounds->x1),
				   scale * (bounds->y2 - bounds->y1));

	memcpy(&gc, &gc_backup, sizeof(GC));
}

void DrawFilledPolygon(Display * dpy, Drawable w, GC gc, double scale,
	unsigned int left,
	unsigned int top,
	unsigned int angle,
	unsigned int rot_left,
	unsigned int rot_top,
	unsigned int t,
	unsigned int l,
	unsigned int b,
	unsigned int r,
	unsigned int corner_radius) {

	XPoint point[8];
	int npoints;
	int shape;
	int mode;

	double px, py;

	npoints = 8;
	shape = Convex;
	mode = CoordModeOrigin;

	double local_corner_radius = corner_radius + 0 * pxl;

	RotatePoint(left + l + local_corner_radius, top + t, angle,
				rot_left, rot_top, &px, &py);
	point[0].x = scale * px;
	point[0].y = scale * py;
	RotatePoint(left + r - local_corner_radius, top + t, angle,
				rot_left, rot_top, &px, &py);
	point[1].x = scale * px;
	point[1].y = scale * py;
	RotatePoint(left + r, top + t + local_corner_radius, angle,
				rot_left, rot_top, &px, &py);
	point[2].x = scale * px;
	point[2].y = scale * py;
	RotatePoint(left + r, top + b - local_corner_radius, angle,
				rot_left, rot_top, &px, &py);
	point[3].x = scale * px;
	point[3].y = scale * py;
	RotatePoint(left + r - local_corner_radius, top + b, angle,
				rot_left, rot_top, &px, &py);
	point[4].x = scale * px;
	point[4].y = scale * py;
	RotatePoint(left + l + local_corner_radius, top + b, angle,
				rot_left, rot_top, &px, &py);
	point[5].x = scale * px;
	point[5].y = scale * py;
	RotatePoint(left + l, top + b - local_corner_radius, angle,
				rot_left, rot_top, &px, &py);
	point[6].x = scale * px;
	point[6].y = scale * py;
	RotatePoint(left + l, top + t + local_corner_radius, angle,
				rot_left, rot_top, &px, &py);
	point[7].x = scale * px;
	point[7].y = scale * py;


	XFillPolygon(dpy, w, gc, point, npoints, shape, mode);

}

/* Graphic context should have already been set. */
void
KbDrawShape(Display * dpy, Drawable w, GC gc, unsigned int angle,
			unsigned int rot_left, unsigned int rot_top, double scale,
			unsigned int left, unsigned int top,
			XkbDescPtr _kb, XkbShapePtr shape, XkbColorPtr color,
			Bool is_key)
{
/*	KbDrawBounds(dpy, w, gc, angle, scale, left, top, _kb, &shape->bounds); */

	XkbOutlinePtr source;
	int i;
	int t, l, b, r;
	int j;
	int shapes_to_paint = 1;

	if (drawkb_config.painting_mode == FULL_SHAPE) shapes_to_paint = shape->num_outlines;

	for (i = 0; i < (is_key ? shapes_to_paint : shape->num_outlines); i++) {
		source = &shape->outlines[i];

		double corner_radius = source->corner_radius + 1 * pxl;

		XSetLineAttributes(dpy, gc, LINE_WIDTH, LineSolid, CapButt, JoinMiter);

		switch (source->num_points) {
		case 1:
			t = l = 0;
			b = source->points[0].y;
			r = source->points[0].x;
			break;
		case 2:
			t = source->points[0].y;
			l = source->points[0].x;
			b = source->points[1].y;
			r = source->points[1].x;
			break;
		default:
			/* FIXME: Should also take care of angle and corner radius */
			for (j = 0; j < source->num_points - 1; j++) {
				XDrawLine(dpy, w, gc, scale * (left + source->points[j].x),
						  scale * (top + source->points[j].y),
						  scale * (left + source->points[j + 1].x),
						  scale * (top + source->points[j + 1].y));
			}
			XDrawLine(dpy, w, gc, scale * (left + source->points[j].x),
					  scale * (top + source->points[j].y),
					  scale * (left + source->points[0].x),
					  scale * (top + source->points[0].y));
			break;
		}
		if (source->num_points <= 2) {

			double ax, ay, bx, by;
			double arch, arcw, arcs, arce;

			unsigned long color;

if (drawkb_config.painting_mode == FULL_SHAPE || drawkb_config.painting_mode == FLAT_KEY) {

			if ( i % 2 == 0 ) {
				color = darkcolor.pixel;
				current = &xftdarkcolor;
			} else {
				color = background.pixel;
				current = &xftbackground;
			}

			XSetForeground(dpy, gc, color);

			is_key ? DrawFilledPolygon(dpy, w, gc, scale, left, top, angle, rot_left, rot_top, t, l, b, r, corner_radius) : 0;

			RotateArc(left + l - 1 * pxl, top + t - 1 * pxl, 2 * corner_radius + LINE_WIDTH * pxl + 2 * pxl,
					  2 * corner_radius + LINE_WIDTH * pxl + 2 * pxl, 5760, 5759, angle,
					  rot_left, rot_top, &ax, &ay, &arcw, &arch, &arcs,
					  &arce);
			XFillArc(dpy, w, gc, scale * ax, scale * ay, scale * arcw,
					 scale * arch, arcs, arce);

			RotateArc(left + r - 2 * corner_radius - LINE_WIDTH * pxl, top + t - 1 * pxl,
					  2 * corner_radius + LINE_WIDTH * pxl, 2 * corner_radius + LINE_WIDTH * pxl + 2 * pxl,
					  0, 5759, angle, rot_left, rot_top, &ax, &ay, &arcw,
					  &arch, &arcs, &arce);
			XFillArc(dpy, w, gc, scale * ax, scale * ay, scale * arcw,
					 scale * arch, arcs, arce);

			RotateArc(left + r - 2 * corner_radius - LINE_WIDTH * pxl,
					  top + b - 2 * corner_radius - LINE_WIDTH * pxl,
					  2 * corner_radius + LINE_WIDTH * pxl, 2 * corner_radius + LINE_WIDTH * pxl,
					  17280, 5759, angle, rot_left, rot_top, &ax, &ay,
					  &arcw, &arch, &arcs, &arce);
			XFillArc(dpy, w, gc, scale * ax, scale * ay, scale * arcw,
					 scale * arch, arcs, arce);

			RotateArc(left + l - 1 * pxl, top + b - 2 * corner_radius - LINE_WIDTH * pxl,
					  2 * corner_radius + LINE_WIDTH * pxl + 2 * pxl, 2 * corner_radius + LINE_WIDTH * pxl,
					  11521, 5759, angle, rot_left, rot_top, &ax, &ay,
					  &arcw, &arch, &arcs, &arce);
			XFillArc(dpy, w, gc, scale * ax, scale * ay, scale * arcw,
					 scale * arch, arcs, arce);

} else {
			XSetForeground(dpy, gc, foreground.pixel);
			current = &xftforeground;
			RotatePoint(left + l + corner_radius, top + t, angle,
						rot_left, rot_top, &ax, &ay);
			RotatePoint(left + r - corner_radius, top + t, angle,
						rot_left, rot_top, &bx, &by);
			XDrawLine(dpy, w, gc, scale * (ax), scale * (ay), scale * (bx),
					  scale * (by));

			RotateArc(left + l - 1 * pxl, top + t - 1 * pxl, 2 * corner_radius + LINE_WIDTH * pxl + 2 * pxl,
					  2 * corner_radius + LINE_WIDTH * pxl + 2 * pxl, 5760, 5759, angle,
					  rot_left, rot_top, &ax, &ay, &arcw, &arch, &arcs,
					  &arce);
			XDrawArc(dpy, w, gc, scale * ax, scale * ay, scale * arcw,
					 scale * arch, arcs, arce); 

			RotatePoint(left + r, top + t + corner_radius, angle,
						rot_left, rot_top, &ax, &ay);
			RotatePoint(left + r, top + b - corner_radius, angle,
						rot_left, rot_top, &bx, &by);
			XDrawLine(dpy, w, gc, scale * (ax), scale * (ay), scale * (bx),
					  scale * (by)); 

			RotateArc(left + r - 2 * corner_radius - LINE_WIDTH * pxl, top + t - 1 * pxl,
					  2 * corner_radius + LINE_WIDTH * pxl, 2 * corner_radius + LINE_WIDTH * pxl + 2 * pxl,
					  0, 5759, angle, rot_left, rot_top, &ax, &ay, &arcw,
					  &arch, &arcs, &arce);
			XDrawArc(dpy, w, gc, scale * ax, scale * ay, scale * arcw,
					 scale * arch, arcs, arce); 

			RotatePoint(left + r - corner_radius, top + b, angle,
						rot_left, rot_top, &ax, &ay);
			RotatePoint(left + l + corner_radius, top + b, angle,
						rot_left, rot_top, &bx, &by);

			XDrawLine(dpy, w, gc, scale * (ax), scale * (ay), scale * (bx),
					  scale * (by)); 

			RotateArc(left + r - 2 * corner_radius - LINE_WIDTH * pxl,
					  top + b - 2 * corner_radius - LINE_WIDTH * pxl,
					  2 * corner_radius + LINE_WIDTH * pxl, 2 * corner_radius + LINE_WIDTH * pxl,
					  17280, 5759, angle, rot_left, rot_top, &ax, &ay,
					  &arcw, &arch, &arcs, &arce);
			XDrawArc(dpy, w, gc, scale * ax, scale * ay, scale * arcw,
					 scale * arch, arcs, arce); 

			RotatePoint(left + l, top + b - corner_radius, angle,
						rot_left, rot_top, &ax, &ay);
			RotatePoint(left + l, top + t + corner_radius, angle,
						rot_left, rot_top, &bx, &by);
			XDrawLine(dpy, w, gc, scale * (ax), scale * (ay), scale * (bx),
					  scale * (by));

			RotateArc(left + l - 1 * pxl, top + b - 2 * corner_radius - LINE_WIDTH * pxl,
					  2 * corner_radius + LINE_WIDTH * pxl + 2 * pxl, 2 * corner_radius + LINE_WIDTH * pxl,
					  11521, 5759, angle, rot_left, rot_top, &ax, &ay,
					  &arcw, &arch, &arcs, &arce);
			XDrawArc(dpy, w, gc, scale * ax, scale * ay, scale * arcw,
					 scale * arch, arcs, arce);

}
			

		}
	}

	XFlush(dpy);

}

void
KbDrawDoodad(Display * dpy, Drawable w, GC gc, /*XftFont *f, */unsigned int angle,
			 double scale, unsigned int left, unsigned int top,
			 XkbDescPtr _kb, XkbDoodadPtr doodad)
{

	XSetForeground(dpy, gc, lightcolor.pixel);
	current = &xftlightcolor;
	switch (doodad->any.type) {
	case XkbOutlineDoodad:
		KbDrawShape(dpy, w, gc, angle + doodad->shape.angle,
					left + doodad->shape.left, top + doodad->shape.top,
					scale, left + doodad->shape.left,
					top + doodad->shape.top, _kb,
					&_kb->geom->shapes[doodad->shape.shape_ndx],
					&_kb->geom->colors[doodad->shape.color_ndx], False);
		break;
	case XkbSolidDoodad:
		KbDrawShape(dpy, w, gc, angle + doodad->shape.angle,
					left + doodad->shape.left, top + doodad->shape.top,
					scale, left + doodad->shape.left,
					top + doodad->shape.top, _kb,
					&_kb->geom->shapes[doodad->shape.shape_ndx],
					&_kb->geom->colors[doodad->shape.color_ndx], False);
		break;
	case XkbTextDoodad:
/*		XftDrawString8(dw, current, NULL, scale * (left + doodad->text.left), scale * (top + doodad->text.top) + 6, (unsigned char *)doodad->text.text, strlen(doodad->text.text));*/
		break;
	case XkbIndicatorDoodad:
		KbDrawShape(dpy, w, gc, angle + doodad->indicator.angle,
					left + doodad->indicator.left,
					top + doodad->indicator.top, scale,
					left + doodad->indicator.left,
					top + doodad->indicator.top, _kb,
					&_kb->geom->shapes[doodad->indicator.shape_ndx],
					&_kb->geom->colors[doodad->indicator.on_color_ndx],
					False);
		break;
	case XkbLogoDoodad:
		KbDrawShape(dpy, w, gc, angle + doodad->logo.angle,
					left + doodad->logo.left, top + doodad->logo.top,
					scale, left + doodad->logo.left,
					top + doodad->logo.top, _kb,
					&_kb->geom->shapes[doodad->logo.shape_ndx],
					&_kb->geom->colors[doodad->logo.color_ndx], False);
		break;
	}
	XSetForeground(dpy, gc, foreground.pixel);
	current = &xftforeground;
}

int PutIcon(Drawable kbwin, int x, int y, int width, int height, const char *fn)
{

	void *i;

	i = NewImage(fn);
	if (i == NULL) {
		perror("puticon");
		return EXIT_FAILURE;
	}

	LoadImage(i, fn);

	ResizeImage(i, width, height);

	DrawImage(i, kbwin, x, y);

	FreeImage(i);


	return EXIT_SUCCESS;

}


void
KbDrawKey(Display * dpy, Drawable w, GC gc, unsigned int angle,
		  unsigned int section_left, unsigned int section_top,
		  double scale, unsigned int left, unsigned int top,
		  XkbDescPtr _kb, XkbKeyPtr key, key_data_t key_data)
{

	int fixed_num_keys;
	unsigned long i;

	char buf[1024]="";
	int buf_n = 1023;

	KbDrawShape(dpy, w, gc, angle, section_left, section_top, scale,
				left, top, _kb,
				&_kb->geom->shapes[key->shape_ndx],
				&_kb->geom->colors[key->color_ndx], True);

	XSetForeground(dpy, gc, foreground.pixel);
	current = &xftforeground;

	/* This is to work around an XKB apparent bug. */
	fixed_num_keys = _kb->names->num_keys;
	if (!fixed_num_keys)
		fixed_num_keys = 256;

	for (i = 0; i < fixed_num_keys; i++) {
		char name[5] = "";
		char glyph[256] = "";
		char keystring[256] = "";
		char *kss;

		debug(13, "%d / %s / %s /\n", i, key->name.name, _kb->names->keys[i].name);
		if (!strncmp(key->name.name, _kb->names->keys[i].name, 4)) {
			strncpy(name, _kb->names->keys[i].name, 4);
			KeySym ks;
			ks = XKeycodeToKeysym(dpy, i, 0);
			kss = XKeysymToString(ks);
			strncpy(keystring, kss, 255);
			if (kss) {
				kss = LookupKeylabelFromKeystring(kss);
				strncpy(glyph, kss, 255);

				XftFont *fs;
				unsigned int tw;

				double ax, ay;

				if (drawkb_config.IQF(XStringToKeysym(keystring), 0, buf, buf_n) == EXIT_SUCCESS) {

					/* FIXME: Key label vertical position is miscalculated. */
					fs = XLoadQueryScalableFont(dpy, 0, drawkb_config.font, key_data.size);

					if (strcmp(buf, "") != 0) {

						int size = key_data.labelbox.x2 - key_data.labelbox.x1;

						if (key_data.labelbox.y2 - key_data.labelbox.y1 - (fs->ascent + 1) * pxl < size) 
							size = key_data.labelbox.y2 - key_data.labelbox.y1 - (fs->ascent + 1) * pxl;

						RotatePoint((left + key_data.labelbox.x2 - size),
									(top + key_data.labelbox.y2 - size),
									angle, section_left, section_top, &ax,
									&ay);

						PutIcon(w, scale*ax, scale*ay, scale*size, scale*size, buf);

					/* KbDrawBounds(dpy, w, gc, angle, scale, left, top, _kb, &(key_data.labelbox)); */

					}

					RotatePoint(left + key_data.labelbox.x1,
								(top + key_data.labelbox.y1 + fs->ascent / scale),
								angle, section_left, section_top, &ax,
								&ay);

					XftDrawString8(dw, current, fs, scale*ax, scale*ay, (unsigned char *)glyph,
								strlen(glyph));


				} else {
					if (drawkb_config.painting_mode == FLAT_KEY) {
						XSetForeground(dpy, gc, background.pixel);
						current = &xftbackground;
					} else {
						XSetForeground(dpy, gc, lightcolor.pixel);
						current = &xftlightcolor;
					}

					fs = XLoadQueryScalableFont(dpy, 0,
												drawkb_config.font,
												key_data.size);
					if (strlen(kss) == 1) {
						tw = MyXftTextWidth(dpy, fs, glyph, strlen(glyph));
						RotatePoint((left + (key_data.labelbox.x1 + key_data.labelbox.x2) / 2) -
									tw / 2 / scale,
									top + key_data.labelbox.y1 + (key_data.labelbox.y2 - key_data.labelbox.y1) * g_baseline,
									angle, section_left, section_top, &ax,
									&ay);
						XftDrawString8(dw, current, fs, scale*ax, scale*ay, (unsigned char *)glyph,
									1);
					} else {
						RotatePoint(left + key_data.labelbox.x1,
									top + key_data.labelbox.y1 + (key_data.labelbox.y2 - key_data.labelbox.y1) * g_baseline,
									angle, section_left, section_top, &ax,
									&ay);
						XftDrawString8(dw, current, fs, scale * (ax), scale * (ay),
									(unsigned char *)kss, strlen(kss));
					}

					XftFontClose(dpy, fs);
					XSetForeground(dpy, gc, foreground.pixel);
					current = &xftforeground;
				}
			}
			break;
		}
	}
}

void AdjustSize(Display *dpy, XkbBoundsRec labelbox, const char *glyph, double initial_key_height_percent, double scale, int *size)
{

	int labelbox_width = labelbox.x2 - labelbox.x1;
	int labelbox_height = labelbox.y2 - labelbox.y1;

	XftFont *fs;

	debug (10, " --> AdjustSize (labelbox(x1=%d, y1=%d, x2=%d, y2=%d), glyph=%s, initial_key_height_percent=%lf, scale=%lf, size=%d\n", labelbox.x1, labelbox.y1, labelbox.x2, labelbox.y2, glyph, initial_key_height_percent, scale, *size);

	if (!*size) {
		*size = labelbox_height * initial_key_height_percent * scale;

		fs = XLoadQueryScalableFont(dpy, 0, drawkb_config.font, *size);

		while (MyXftTextWidth(dpy, fs, glyph, strlen(glyph)) <= (int) labelbox_width*scale
			&& fs->ascent <= labelbox_height*initial_key_height_percent*scale) {
			XftFontClose(dpy, fs);
			(*size)++;
			fs = XLoadQueryScalableFont(dpy, 0, drawkb_config.font, *size);
			debug (10, "Iterating in %s:%d\n", __FILE__, __LINE__);
		}
	} else {
		fs = XLoadQueryScalableFont(dpy, 0, drawkb_config.font, *size);
	}

	debug (10, " ::: AdjustSize interim size value: %d\n", *size);

	/* Reduce the *size point by point as less as possible. */
	while (MyXftTextWidth(dpy, fs, glyph, strlen(glyph)) > (int) labelbox_width*scale) {
		XftFontClose(dpy, fs);
		(*size)--;
		fs = XLoadQueryScalableFont(dpy, 0, drawkb_config.font, *size);
		debug (10, "Iterating in %s:%d\n", __FILE__, __LINE__);
	}

	XftFontClose(dpy, fs);

	debug (10, " <-- AdjustSize final size value: %d\n", *size);
}

void
KbDrawRow(Display * dpy, Drawable w, GC gc, unsigned int angle,
		  double scale, unsigned int left, unsigned int top,
		  XkbDescPtr _kb, XkbRowPtr row)
{

	int i;
	unsigned int next_piece = 0;

	XkbBoundsRec labelbox;

//	KbDrawBounds(dpy, w, gc, angle, scale, left + row->left, top + row->top, _kb, &row->bounds);

	int size_bound = 0;
	int size_unbound_char = 0;
	int size_unbound_string = 0;

	/* This is to work around an XKB apparent bug. */
	int fixed_num_keys = _kb->names->num_keys;
	if (!fixed_num_keys)
		fixed_num_keys = 256;

	int j;

	key_data_t *key_data = NULL;
	int key_data_n = 0;

	for (j = 0; j < row->num_keys; j++) {
		XkbKeyPtr key = &row->keys[j];

		debug (4, "KbDrawRow: processing key j=%d\n ", j);

		list_add_element (key_data, key_data_n, key_data_t);
		memset(&(key_data[key_data_n-1]), 0, sizeof(key_data_t));
		key_data[key_data_n-1].index = j;

		for (i = 0; i < fixed_num_keys; i++) {

			char name[5] = "";
			char glyph[256] = "";
			char keystring[256] = "";
			char *kss;

			debug(13, "%d / %s / %s /\n", i, key->name.name, _kb->names->keys[i].name);
			if (strncmp(key->name.name, _kb->names->keys[i].name, 4) != 0)
				continue;

			strncpy(name, _kb->names->keys[i].name, 4);
			KeySym ks;
			ks = XKeycodeToKeysym(dpy, i, 0);
			kss = XKeysymToString(ks);
			strncpy(keystring, kss, 255);

			if (!kss)
				continue;

			kss = LookupKeylabelFromKeystring(kss);
			strncpy(glyph, kss, 255);

			/* Calculate label + icon box bounds */
			int labelbox_border = 0 / scale;
			int labelbox_margin = 2 / scale;

			XkbBoundsRec kr, *k = &kr;
			
			if (drawkb_config.painting_mode == FULL_SHAPE) {
				XkbComputeShapeTop(&_kb->geom->shapes[key->shape_ndx], k);
			} else if (drawkb_config.painting_mode == BASE_OUTLINE_ONLY) {
				k = &_kb->geom->shapes[key->shape_ndx].bounds;
				labelbox_border = LINE_WIDTH * pxl;
			} else if (drawkb_config.painting_mode == FLAT_KEY) {
				k = &_kb->geom->shapes[key->shape_ndx].bounds;
			} else {
				assert (0);
			}

			labelbox.x1 = k->x1 + labelbox_margin + labelbox_border;
			labelbox.x2 = k->x2 - labelbox_margin - labelbox_border;
			labelbox.y1 = k->y1 + labelbox_margin + labelbox_border;
			labelbox.y2 = k->y2 - labelbox_margin - labelbox_border;
			/* End calculate label + icon box bounds */

			if (drawkb_config.IQF(XStringToKeysym(keystring), 0, NULL, 0) == EXIT_SUCCESS) {
				/* If this key is a bound key... */
				AdjustSize(dpy, labelbox, glyph, 0.28, scale, &size_bound);
				key_data[key_data_n-1].size = size_bound;
			} else if (strlen(glyph) == 1) {
				/* If this key is a single char unbound key... */
				AdjustSize(dpy, labelbox, glyph, 0.9, scale, &size_unbound_char);
				key_data[key_data_n-1].size = size_unbound_char;
			} else {
				/* This is a multiple char unbound key. */
				labelbox.x1 += 4 / scale;
				labelbox.x2 -= 4 / scale;
				AdjustSize(dpy, labelbox, glyph, 0.25, scale, &size_unbound_string);
				key_data[key_data_n-1].size = size_unbound_string;
			}
			memcpy(&(key_data[key_data_n-1].labelbox), &labelbox, sizeof(XkbBoundsRec));
			key_data[key_data_n-1].glyph = glyph;

			break;

		}
	}

	for (i = 0; i < row->num_keys; i++) {

		for (j = 0; j < key_data_n && key_data[j].index != i; j++);
		assert(j < key_data_n);

		if (!row->vertical) {
			KbDrawKey(dpy, w, gc, angle, left, top, scale,
					  left + row->left + next_piece + row->keys[i].gap,
					  top + row->top,
					  _kb, &row->keys[i], key_data[i]);
			next_piece +=
				_kb->geom->shapes[row->keys[i].shape_ndx].bounds.x2 + row->keys[i].gap;
		} else {
			KbDrawKey(dpy, w, gc, 
angle, left, top, scale,
					  left + row->left, top + row->top + next_piece + row->keys[i].gap,
					  _kb, &row->keys[i], key_data[i]);
			next_piece +=
				_kb->geom->shapes[row->keys[i].shape_ndx].bounds.y2 + row->keys[i].gap;
		}
	}

	free(key_data);

}

void
KbDrawSection(Display * dpy, Drawable w, GC gc, unsigned int angle,
			  double scale, unsigned int left, unsigned int top,
			  XkbDescPtr _kb, XkbSectionPtr section)
{
	int i, p;

/*  KbDrawBounds(dpy, w, gc, angle, scale, left + section->left, top + section->top, _kb, &section->bounds); */

	/* if (section->name) fprintf(stderr, "Drawing section: %s\n", XGetAtomName(dpy,
	   section->name)); */

	for (i = 0; i < section->num_rows; i++) {
		XkbComputeRowBounds(_kb->geom, section, &section->rows[i]);
		KbDrawRow(dpy, w, gc, angle + section->angle, scale,
				  left + section->left, top + section->top, _kb,
				  &section->rows[i]);
	}

	for (p = 0; p <= 255; p++) {
		for (i = 0; i < section->num_doodads; i++) {
			if (section->doodads[i].any.priority == p) {
				KbDrawDoodad(dpy, w, gc, angle + section->angle, scale,
							 left + section->left, top + section->top, _kb,
							 &section->doodads[i]);
			}
		}
	}
}

void
KbDrawComponents(Display * dpy, Drawable w, GC gc, unsigned int angle,
				 double scale, unsigned int left, unsigned int top,
				 XkbDescPtr _kb, XkbSectionPtr sections,
				 int sections_n, XkbDoodadPtr doodads, int doodads_n)
{
	int i, p;

	/* FIXME: This algorithm REALLY NEEDS AND CRYING BEGS for optimization.
	 * Indexing sections and doodads into a binary or balanced tree would be
	 * the best.
	 */

	for (p = 0; p <= 255; p++) {
		for (i = 0; i < sections_n; i++) {
			if (sections[i].priority == p) {
				KbDrawSection(dpy, w, gc, angle, scale, left,
							  top, _kb, &sections[i]);

			}
		}

		for (i = 0; i < doodads_n; i++) {
			if (doodads[i].any.priority == p) {
				KbDrawDoodad(dpy, w, gc, angle, scale, left,
							 top, _kb, &doodads[i]);
			}
		}
	}
}

void drawkb_draw(Display * dpy, Drawable d, GC gc, unsigned int width, unsigned int height, XkbDescPtr kbdesc)
{

	float scale;

	dw = XftDrawCreate (dpy, d, DEFAULT_VISUAL(dpy), DEFAULT_COLORMAP(dpy));

	XGCValues xgcv;
	XGetGCValues(dpy, gc, GCForeground | GCBackground, &xgcv);

	background.pixel = xgcv.background;
	foreground.pixel = xgcv.foreground;

	XQueryColor(dpy, DEFAULT_COLORMAP(dpy), &background);
	XQueryColor(dpy, DEFAULT_COLORMAP(dpy), &foreground);

	lightcolor.red = ((background.red - foreground.red) * 0.8) + foreground.red;
	lightcolor.green = ((background.green - foreground.green) * 0.8) + foreground.green;
	lightcolor.blue = ((background.blue - foreground.blue) * 0.8) + foreground.blue;
	XAllocColor(dpy, DEFAULT_COLORMAP(dpy), &lightcolor);

	darkcolor.red = ((background.red - 0) * 0.7);
	darkcolor.green = ((background.green - 0) * 0.7);
	darkcolor.blue = ((background.blue - 0) * 0.7);
	XAllocColor(dpy, DEFAULT_COLORMAP(dpy), &darkcolor);

	XRenderColor xr;

	xr.red = background.red;
	xr.green = background.green;
	xr.blue = background.blue;
	xr.alpha = 0xffff;
	XftColorAllocValue(dpy, DEFAULT_VISUAL(dpy), DEFAULT_COLORMAP(dpy), &xr, &xftbackground);

	xr.red = foreground.red;
	xr.green = foreground.green;
	xr.blue = foreground.blue;
	xr.alpha = 0xffff;
	XftColorAllocValue(dpy, DEFAULT_VISUAL(dpy), DEFAULT_COLORMAP(dpy), &xr, &xftforeground);

	xr.red = darkcolor.red;
	xr.green = darkcolor.green;
	xr.blue = darkcolor.blue;
	xr.alpha = 0xffff;
	XftColorAllocValue(dpy, DEFAULT_VISUAL(dpy), DEFAULT_COLORMAP(dpy), &xr, &xftdarkcolor);

	xr.red = lightcolor.red;
	xr.green = lightcolor.green;
	xr.blue = lightcolor.blue;
	xr.alpha = 0xffff;
	XftColorAllocValue(dpy, DEFAULT_VISUAL(dpy), DEFAULT_COLORMAP(dpy), &xr, &xftlightcolor);

	XkbGeometryPtr kbgeom = kbdesc->geom;

	/* Get what scale should drawkb work with, according to drawable's
	 * width and height. */
	double scalew = (float) width / kbgeom->width_mm;
	double scaleh = (float) height / kbgeom->height_mm;

	/* Work with the smallest scale. */
	if (scalew < scaleh) {
		scale = scalew;
	} else { /* scalew >= scaleh */
		scale = scaleh;
	}

	/* Override scale for debugging purposes */
	/*scale = 2;*/

	int left = 0;
	int top = 0;
	int angle = 0;

	/* Draw top-level rectangle */
	XDrawRectangle(drawkb_config.dpy, d, gc, left, top, scale * kbdesc->geom->width_mm,
				   scale * kbdesc->geom->height_mm);

	/* Draw each component (section or doodad) of the top-level kbdesc->geometry, in
	 * priority order. Note that priority handling is left to the function. */
	KbDrawComponents(drawkb_config.dpy, d, gc, angle, scale, left, top, kbdesc,
					 kbdesc->geom->sections, kbdesc->geom->num_sections,
					 kbdesc->geom->doodads, kbdesc->geom->num_doodads);

	XFlush(dpy);
}

/* Checks for font existance and tries to fallback if not. */
int Init_Font(const char *font)
{

	if (!font) {
		fprintf(stderr, "User didn't specify font.\n");
	}

	strncpy(drawkb_config.font, font, 499);

	/* Try 1: User drawkb_configured. */
	if (drawkb_config.font) {

		/* FIXME: Validate font. */
		XSetFontNameToScalable(drawkb_config.font, drawkb_config.font, 500);

		XftFont *fs;
		fs = XLoadQueryScalableFont(drawkb_config.dpy, 0, drawkb_config.font, 1000);

		if (fs) {
			return EXIT_SUCCESS;
		}

		fprintf(stderr, "Failed to initialize user configured font.\n");

	}

	/* FILLTHEGAP */
	/* Try 2: Ask NETWM (like in a skin). */

	/* Try 3: Fallback to XKB's. */
	if (kbdesc->geom->label_font) {
		XSetFontNameToScalable(kbdesc->geom->label_font, drawkb_config.font, 500);

		XftFont *fs;
		fs = XLoadQueryScalableFont(drawkb_config.dpy, 0, drawkb_config.font, 1000);
		if (fs) {
			return EXIT_SUCCESS;
		}
	}

	/* FILLTHEGAP */
	/* Try 4: Ask for whatever ("*-iso8859-1" && !"*-symbol") font. */

	return EXIT_FAILURE;
}



int drawkb_init(Display *dpy, const char *imagelib, const char *font,
	IQF_t IQF, painting_mode_t painting_mode, float scale)
{

	drawkb_config.IQF = IQF;

	drawkb_config.painting_mode = painting_mode;

	drawkb_config.dpy = dpy;

	if (Init_Imagelib(dpy, imagelib) == EXIT_FAILURE)
	{
		char vals[500] = "";
		Imagelib_GetValues((char *) &vals, 499);
		fprintf(stderr, "Failed to initialize image library: %s. You might try: %s\n", imagelib, vals);
		return EXIT_FAILURE;
	}

	/* Init_Font needs Init_Geometry to succeed, because one of
	 * the fallback fonts is the XKB's specified font label, and
	 * therefore, geometry must be loaded. */

	if (Init_Font(font) == EXIT_FAILURE)
	{
		fprintf(stderr, "Failed to initialize font: %s.\n"
			"Possible causes are:\n"
			" + You did not use the complete font name, as in\n"
				"	\"-*-bitstream vera sans-bold-r-*-*-*-*-*-*-*-*-*-*\"\n"
			" + You did not quote the name and the name contains spaces.\n"
			" + The font doesn't exist. Try using XftFont *sel to find a suitable "
				"font.\n", font);
		return EXIT_FAILURE;
	}

	XftFont *fs;
	fs = XLoadQueryScalableFont(drawkb_config.dpy, 0, drawkb_config.font, 1000);
	if (!fs) {
		fprintf(stderr, "superkb: Couldn't XLoadQueryScalableFont. This shouldn't have happened.\n");
		return EXIT_FAILURE;
	}

	g_baseline =
		(float) fs->ascent / (fs->ascent +
										 fs->descent);

	WorkaroundBoundsBug(dpy, kbdesc);

	return EXIT_SUCCESS;

}
