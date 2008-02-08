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

#include <X11/XKBlib.h>
#include <X11/extensions/XKBgeom.h>

#include "drawkb.h"
#include "imagelib.h"

#define LINE_WIDTH 2

double __scale;

typedef struct {
	char font[500];
	XFontStruct fs;
	Display *dpy;
	IQF_t IQF;
} drawkb_config_t;

drawkb_config_t drawkb_config;

XkbDescPtr kbdesc;
XkbGeometryPtr kb_geom;

IQF_t IQF;

int g_size;
double g_baseline;

XColor lightcolor;
XColor foreground;
XColor background;

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
 * an XFontStruct for the given font scaled to the specified size and the
 * exact resolution of the screen.  The font name is assumed to be a
 * well-formed XLFD name, and to have pixel size, point size, and average
 * width fields of "0" and arbitrary x-resolution and y-resolution fields.
 * Size is specified in tenths of points.  Returns NULL if the name is
 * malformed or no such font exists.
 *
 * FIXME: It doesn't check for i < 500*sizeof(char).
 */
XFontStruct *XLoadQueryScalableFont(Display * dpy, int screen, char *name,
	int size)
{
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
			case 7:			/* pixel size */
			case 12:		   /* average width */
				/* change from "-0-" to "-*-" */
				newname[j] = '*';
				j++;
				if (name[i + 1] != '\0')
					i++;
				break;
			case 8:			/* point size */
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
	return XLoadQueryFont(dpy, newname);
}

int drawkb_set_font(const char * font)
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

	XFontStruct *fs;
	fs = XLoadQueryScalableFont(drawkb_config.dpy, 0, drawkb_config.font, 1000);
	if (fs) {
		return EXIT_SUCCESS;
	}

	XFreeFontInfo(NULL, fs, 0);
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

	for (i = 0; i < (is_key ? 1 : shape->num_outlines); i++) {
		source = &shape->outlines[i];

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

			RotatePoint(left + l + source->corner_radius, top + t, angle,
						rot_left, rot_top, &ax, &ay);
			RotatePoint(left + r - source->corner_radius, top + t, angle,
						rot_left, rot_top, &bx, &by);
			XDrawLine(dpy, w, gc, scale * (ax), scale * (ay), scale * (bx),
					  scale * (by));

			RotateArc(left + l, top + t, 2 * source->corner_radius + LINE_WIDTH/scale,
					  2 * source->corner_radius + LINE_WIDTH/scale, 5760, 5760, angle,
					  rot_left, rot_top, &ax, &ay, &arcw, &arch, &arcs,
					  &arce);
			XDrawArc(dpy, w, gc, scale * ax, scale * ay, scale * arcw,
					 scale * arch, arcs, arce);

			RotatePoint(left + r, top + t + source->corner_radius, angle,
						rot_left, rot_top, &ax, &ay);
			RotatePoint(left + r, top + b - source->corner_radius, angle,
						rot_left, rot_top, &bx, &by);
			XDrawLine(dpy, w, gc, scale * (ax), scale * (ay), scale * (bx),
					  scale * (by));

			RotateArc(left + r - 2 * source->corner_radius - LINE_WIDTH/scale, top + t,
					  2 * source->corner_radius + LINE_WIDTH/scale, 2 * source->corner_radius + LINE_WIDTH/scale,
					  0, 5760, angle, rot_left, rot_top, &ax, &ay, &arcw,
					  &arch, &arcs, &arce);
			XDrawArc(dpy, w, gc, scale * ax, scale * ay, scale * arcw,
					 scale * arch, arcs, arce);

			RotatePoint(left + r - source->corner_radius, top + b, angle,
						rot_left, rot_top, &ax, &ay);
			RotatePoint(left + l + source->corner_radius, top + b, angle,
						rot_left, rot_top, &bx, &by);

			XDrawLine(dpy, w, gc, scale * (ax), scale * (ay), scale * (bx),
					  scale * (by));

			RotateArc(left + r - 2 * source->corner_radius - LINE_WIDTH/scale,
					  top + b - 2 * source->corner_radius - LINE_WIDTH/scale,
					  2 * source->corner_radius + LINE_WIDTH/scale, 2 * source->corner_radius + LINE_WIDTH/scale,
					  17280, 5760, angle, rot_left, rot_top, &ax, &ay,
					  &arcw, &arch, &arcs, &arce);
			XDrawArc(dpy, w, gc, scale * ax, scale * ay, scale * arcw,
					 scale * arch, arcs, arce);

			RotatePoint(left + l, top + b - source->corner_radius, angle,
						rot_left, rot_top, &ax, &ay);
			RotatePoint(left + l, top + t + source->corner_radius, angle,
						rot_left, rot_top, &bx, &by);
			XDrawLine(dpy, w, gc, scale * (ax), scale * (ay), scale * (bx),
					  scale * (by));

			RotateArc(left + l, top + b - 2 * source->corner_radius - LINE_WIDTH/scale,
					  2 * source->corner_radius + LINE_WIDTH/scale, 2 * source->corner_radius + LINE_WIDTH/scale,
					  11520, 5760, angle, rot_left, rot_top, &ax, &ay,
					  &arcw, &arch, &arcs, &arce);
			XDrawArc(dpy, w, gc, scale * ax, scale * ay, scale * arcw,
					 scale * arch, arcs, arce);

			

		}
	}

	XFlush(dpy);

}

void
KbDrawDoodad(Display * dpy, Drawable w, GC gc, unsigned int angle,
			 double scale, unsigned int left, unsigned int top,
			 XkbDescPtr _kb, XkbDoodadPtr doodad)
{
	XSetForeground(dpy, gc, lightcolor.pixel);
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
		XDrawString(dpy, w, gc, scale * (left + doodad->text.left),
					scale * (top + doodad->text.top) + 6,
					doodad->text.text, strlen(doodad->text.text));
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
		  XkbDescPtr _kb, XkbKeyPtr key)
{

	Font F;

	int fixed_num_keys;
	unsigned long i;

	char buf[1024]="";
	int buf_n = 1023;

	KbDrawShape(dpy, w, gc, angle, section_left, section_top, scale,
				left + key->gap, top, _kb,
				&_kb->geom->shapes[key->shape_ndx],
				&_kb->geom->colors[key->color_ndx], True);

	/* This is to work around an XKB apparent bug. */
	fixed_num_keys = _kb->names->num_keys;
	if (!fixed_num_keys)
		fixed_num_keys = 256;

	for (i = 0; i < fixed_num_keys; i++) {
		char name[5] = "";
		char glyph[256] = "";
		char keystring[256] = "";
		char *kss;

		if (!strncmp(key->name.name, _kb->names->keys[i].name, 4)) {
			strncpy(name, _kb->names->keys[i].name, 4);
			KeySym ks;
			ks = XKeycodeToKeysym(dpy, i, 0);
			kss = XKeysymToString(ks);
			strncpy(keystring, kss, 255);
			if (kss) {
				/* FIXME: Better compare integers, not strings. Fixable. */
				if (!strcmp(kss, "dead_grave"))
					kss = "`";
				if (!strcmp(kss, "dead_acute"))
					kss = "'";
				if (!strcmp(kss, "grave"))
					kss = "`";
				if (!strcmp(kss, "apostrophe"))
					kss = "'";
				if (!strcmp(kss, "space"))
					kss = "";
				if (!strcmp(kss, "Escape"))
					kss = "Esc";
				if (!strcmp(kss, "comma"))
					kss = ",";
				if (!strcmp(kss, "period"))
					kss = ".";
				if (!strcmp(kss, "slash"))
					kss = "/";
				if (!strcmp(kss, "minus"))
					kss = "-";
				if (!strcmp(kss, "equal"))
					kss = "=";
				if (!strcmp(kss, "Caps_Lock"))
					kss = "Caps Lock";
				if (!strcmp(kss, "Shift_L"))
					kss = "Shift";
				if (!strcmp(kss, "Shift_R"))
					kss = "Shift";
				if (!strcmp(kss, "semicolon"))
					kss = ";";
				if (!strcmp(kss, "BackSpace"))
					kss = "Backspace";
				if (!strcmp(kss, "Return"))
					kss = "Enter";
				if (!strcmp(kss, "Control_L"))
					kss = "Ctrl";
				if (!strcmp(kss, "Control_R"))
					kss = "Ctrl";
				if (!strcmp(kss, "Alt_L"))
					kss = "Alt";
				if (!strcmp(kss, "KP_Enter"))
					kss = "Enter";
				if (!strcmp(kss, "KP_Add"))
					kss = "+";
				if (!strcmp(kss, "KP_Subtract"))
					kss = "-";
				if (!strcmp(kss, "KP_Multiply"))
					kss = "*";
				if (!strcmp(kss, "KP_Divide"))
					kss = "/";
				if (!strcmp(kss, "Num_Lock"))
					kss = "Num Lock";
				if (!strcmp(kss, "KP_Home"))
					kss = "Home";
				if (!strcmp(kss, "KP_End"))
					kss = "End";
				if (!strcmp(kss, "KP_Prior"))
					kss = "PgUp";
				if (!strcmp(kss, "KP_Up"))
					kss = "Up";
				if (!strcmp(kss, "KP_Down"))
					kss = "Down";
				if (!strcmp(kss, "KP_Left"))
					kss = "Left";
				if (!strcmp(kss, "KP_Right"))
					kss = "Right";
				if (!strcmp(kss, "KP_Next"))
					kss = "Next";
				if (!strcmp(kss, "KP_Begin"))
					kss = "Begin";
				if (!strcmp(kss, "KP_Insert"))
					kss = "Ins";
				if (!strcmp(kss, "KP_Delete"))
					kss = "Del";
				if (!strcmp(kss, "Scroll_Lock"))
					kss = "Scroll Lock";
				if (!strcmp(kss, "bracketleft"))
					kss = "[";
				if (!strcmp(kss, "bracketright"))
					kss = "]";
				if (!strcmp(kss, "backslash"))
					kss = "\\";
				strncpy(glyph, kss, 255);

				/* "b" is just to abbreviate the otherwise long code. */
				XkbBoundsRec b1, *b = &b1;
				XkbBoundsRec k1, *k = &k1;

				XkbComputeShapeTop(&_kb->geom->shapes[key->shape_ndx], b);

				XFontStruct *fs;
				unsigned int tw;
				unsigned int size;

	double ax, ay;

	if (drawkb_config.IQF(XStringToKeysym(keystring), 0, buf, buf_n) == EXIT_SUCCESS) {

		k = &_kb->geom->shapes[key->shape_ndx].bounds;

		/* FIXME: Key label vertical position is miscalculated. */
		fs = XLoadQueryScalableFont(dpy, 0, drawkb_config.font, 400*scale);

		XSetFont(dpy, gc, fs->fid);

		if (strcmp(buf, "") != 0) {

			int size = (k->x2-k->x1)-4/scale;
			if (k->y2-k->y1-(4+fs->max_bounds.ascent-fs->max_bounds.descent)/scale < size)
				size = k->y2-k->y1-(4+fs->max_bounds.ascent-fs->max_bounds.descent)/scale;

			RotatePoint((left + key->gap + k->x2 - size - 4), (top + k->y2 - size - 4), 
						angle, section_left, section_top, &ax,
						&ay);

			PutIcon(w, scale*ax, scale*ay, scale*size, scale*size, buf);

		}

		/* FIXME: These +- 8 are fixed now, which means they are resolution
		   dependant. This is wrong. */

		RotatePoint(left + key->gap +
					(5 /*- (XTextWidth(fs, glyph, strlen(glyph)))*/)/LINE_WIDTH/scale,
					(top + (fs->max_bounds.ascent) / scale),
					angle, section_left, section_top, &ax,
					&ay);
		XDrawString(dpy, w, gc, scale*ax, scale*ay, glyph,
					strlen(glyph));


	} else {
				XSetForeground(dpy, gc, lightcolor.pixel);
				if (strlen(kss) == 1) {

					//glyph[0] = toupper(glyph[0]);
					size = scale * 600;
				   fs = XLoadQueryScalableFont(dpy, 0,
												drawkb_config.font,
												g_size);
					tw = XTextWidth(fs, glyph, strlen(glyph));
				} else {
					size = scale * 300;
					do {
						fs = XLoadQueryScalableFont(dpy, 0,
													drawkb_config.font,
													size);
						if (!fs) {
							fprintf(stderr, "Could not load font: %s, %d\n",
								   drawkb_config.font, size);
							exit(EXIT_FAILURE);
						}

						tw = XTextWidth(fs, kss, strlen(kss));
						size -= 3;
					}
					while (tw > scale * (b->x2 - b->x1));
				}
				size += 3;
				F = fs->fid;

				XSetFont(dpy, gc, F);

				/* FIXME: Text position works properly, but text prints unrotated. */

				/* FIXME: Vertical position is miscalculated. */
				if (strlen(kss) == 1) {
					RotatePoint((left + key->gap + (b->x1 + b->x2) / 2) -
								tw / 2 / scale,
								top + b->y1 + LINE_WIDTH/scale + (b->y2 - b->y1) * g_baseline,
								angle, section_left, section_top, &ax,
								&ay);
					XDrawString(dpy, w, gc, scale*ax, scale*ay, glyph,
								1);
				} else {
					RotatePoint(left + key->gap + 4/scale,
								top +  LINE_WIDTH/scale + (b->y2 - 4/scale) * g_baseline,
								angle, section_left, section_top, &ax,
								&ay);
					XDrawString(dpy, w, gc, scale * (ax), scale * (ay),
								kss, strlen(kss));
				}

				XFreeFontInfo(NULL, fs, 1);
				XSetForeground(dpy, gc, foreground.pixel);
			}
		}
	}
  }
}

void
KbDrawRow(Display * dpy, Drawable w, GC gc, unsigned int angle,
		  double scale, unsigned int left, unsigned int top,
		  XkbDescPtr _kb, XkbRowPtr row)
{
	int i;
	unsigned int next_piece = 0;

//	KbDrawBounds(dpy, w, gc, angle, scale, left + row->left, top + row->top, _kb, &row->bounds);

	for (i = 0; i < row->num_keys; i++) {
		if (!row->vertical) {
			KbDrawKey(dpy, w, gc, angle, left, top, scale,
					  left + row->left + next_piece, top + row->top,
					  _kb, &row->keys[i]);
			next_piece +=
				_kb->geom->shapes[row->keys[i].shape_ndx].bounds.x2 +
				row->keys[i].gap;
		} else {
			KbDrawKey(dpy, w, gc, angle, left, top, scale,
					  left + row->left, top + row->top + next_piece,
					  _kb, &row->keys[i]);
			next_piece +=
				_kb->geom->shapes[row->keys[i].shape_ndx].bounds.y2 +
				row->keys[i].gap;
		}
	}

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

	XGCValues xgcv;
	XGetGCValues(dpy, gc, GCForeground | GCBackground, &xgcv);

	background.pixel = xgcv.background;
	foreground.pixel = xgcv.foreground;

	XQueryColor(dpy, XDefaultColormap(dpy, 0), &background);
	XQueryColor(dpy, XDefaultColormap(dpy, 0), &foreground);

	lightcolor.red = ((background.red - foreground.red) * 0.8) + foreground.red;
	lightcolor.green = ((background.green - foreground.green) * 0.8) + foreground.green;
	lightcolor.blue = ((background.blue - foreground.blue) * 0.8) + foreground.blue;
	XAllocColor(dpy, XDefaultColormap(dpy, 0), &lightcolor);

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

		XFontStruct *fs;
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

		XFontStruct *fs;
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
	IQF_t IQF, float scale)
{

	drawkb_config.IQF = IQF;

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
			" + The font doesn't exist. Try using xfontsel to find a suitable "
				"font.\n", font);
		return EXIT_FAILURE;
	}

	/* Determine font size for NORM. */
	int norm_h = 0, norm_w = 0;

	/* NORM is an XKB shape describing "normal" keys, like alphanumeric
	 * characters. */

	/* 1. Get NORM with and height. */

	/* Iterate through all shapes. */
	int i,j;
	for (i = 0; i < kbdesc->geom->num_shapes; i++) {
		XkbShapePtr s;		  /* shapes[i] */

		s = &kbdesc->geom->shapes[i];
		if (strncmp(XGetAtomName(dpy, s->name), "NORM", 4) != 0)
			continue; /* Loop until NORM is found. */

		/* A NORM found and stored at "s". */
		/* s->outlines[0] == bounding box */
		if (s->outlines[0].num_points == 1) {
			norm_h = s->outlines[0].points[0].y;
			norm_w = s->outlines[0].points[0].x;
		} else if (s->outlines[0].num_points == 2) {
			norm_h =
				s->outlines[0].points[1].y - s->outlines[0].points[0].y;
			norm_w =
				s->outlines[0].points[1].x - s->outlines[0].points[0].x;
		} else {
			norm_h = 0;
			norm_w = 0;
			for (j = 1; j < s->outlines[0].num_points; j++) {
				if (s->outlines[0].points[j].y -
					s->outlines[0].points[j - 1].y > norm_h)
					norm_h =
						s->outlines[0].points[j].y -
						s->outlines[0].points[j - 1].y;
				if (s->outlines[0].points[j].x -
					s->outlines[0].points[j - 1].x > norm_w)
					norm_w =
						s->outlines[0].points[j].x -
						s->outlines[0].points[j - 1].x;
			}
		}
	}

	if (norm_h == 0 && norm_w == 0) {
		fprintf(stderr, "superkb: Couldn't find NORM shape. Couldn't determine font size for NORM shape.\n");
		return EXIT_FAILURE;
	}

	/* 2. Determine max point size that fits in norm_w and norm_h. */
	int max_w, max_h;

	XFontStruct *fs;
	fs = XLoadQueryScalableFont(drawkb_config.dpy, 0, drawkb_config.font, 1000);
	if (!fs) {
		fprintf(stderr, "superkb: Couldn't XLoadQueryScalableFont. This shouldn't have happened.\n");
		return EXIT_FAILURE;
	}

	max_w =
		norm_w * scale * 1000 / (fs->max_bounds.rbearing -
								 fs->max_bounds.lbearing);
	max_h =
		norm_h * scale * 1000 / (fs->max_bounds.ascent +
								 fs->max_bounds.descent);

	if (max_w < max_h) {
		g_size = max_w;
	} else {
		g_size = max_h;
	}

	g_baseline =
		(float) fs->max_bounds.ascent / (fs->max_bounds.ascent +
										 fs->max_bounds.descent);

	WorkaroundBoundsBug(dpy, kbdesc);

	return EXIT_SUCCESS;

}
