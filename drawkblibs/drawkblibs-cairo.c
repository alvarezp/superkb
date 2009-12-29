/*
 * drawkblibs-cairo.c
 *
 * Copyright (C) 2006, Octavio Alvarez Piza.
 * License: GNU General Public License v2.
 *
 */

#define _GNU_SOURCE

#include <stdlib.h>

#include "drawkblibs.h"
// #include "drawkblibs-cairo.h"

#ifndef WITH_DRAWKBLIBS_CAIRO
	#define drawkblibs_cairo_init Init
#endif

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
 * call drawkb_cairo_WorkaroundBoundsBug() afterwards. This works around a bug present
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

#include <X11/XKBlib.h>
#include <X11/extensions/XKBgeom.h>

#include <cairo.h>
#include <cairo-xlib.h>
#include <cairo-ft.h>

#include <pango/pangocairo.h>

#include "../drawkblib.h"
#include "../imagelib.h"
#include "../globals.h"

#define LINE_WIDTH 2

#define DEFAULT_SCREEN(dpy) (DefaultScreen(dpy))
#define DEFAULT_VISUAL(dpy) (DefaultVisual(dpy, DEFAULT_SCREEN(dpy)))
#define DEFAULT_COLORMAP(dpy) (DefaultColormap(dpy, DEFAULT_SCREEN(dpy)))

cairo_surface_t *surface;

double __scale;

XkbDescPtr kbdesc;
XkbGeometryPtr kb_geom;

IQF_t IQF;

int g_size;
double g_baseline;

XColor lightcolor;
XColor darkcolor;
XColor foreground;
XColor background;

typedef struct {
	char *keystring;
	char *keylabel;
} drawkb_cairo_keystrings_t;

drawkb_cairo_keystrings_t drawkb_cairo_keystrings[] = {
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
	XkbBoundsRec fullbox;
	const char *glyph;
} key_data_t;

char *drawkb_cairo_LookupKeylabelFromKeystring(char *kss) {
	int i = 0;
	while (strcmp((drawkb_cairo_keystrings[i]).keystring, "") != 0) {
		if (!strcmp(kss, (drawkb_cairo_keystrings[i]).keystring))
			return (drawkb_cairo_keystrings[i]).keylabel;
		i++;
	}
	return kss;
}

void drawkb_cairo_WorkaroundBoundsBug(Display * dpy, XkbDescPtr _kb)
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
drawkb_cairo_KbDrawBounds(cairo_t *cr, 
			 unsigned int left, unsigned int top,
			 XkbBoundsPtr bounds, float line_width)
{
	cairo_set_line_width (cr, line_width);
	cairo_set_source_rgb (cr, 1, 1, 1);
	cairo_rectangle (cr, (left + bounds->x1), (top + bounds->y1), (bounds->x2 - bounds->x1), (bounds->y2 - bounds->y1));
	cairo_stroke (cr);

}

void drawkb_cairo_DrawHollowPolygon(Display * dpy, cairo_t *cr,
	unsigned int t,
	unsigned int l,
	unsigned int b,
	unsigned int r,
	unsigned int corner_radius,
	float line_width) {

	int npoints;
	int shape;
	int mode;

	npoints = 8;
	shape = Convex;
	mode = CoordModeOrigin;

	double local_corner_radius = corner_radius;

	cairo_set_line_width (cr, line_width);

	cairo_move_to (cr, l + local_corner_radius, t);
	cairo_line_to (cr, r - local_corner_radius, t);
	cairo_arc (cr, r - local_corner_radius, t + local_corner_radius, local_corner_radius, -M_PI/2.0, 0);
	cairo_line_to (cr, r, b - local_corner_radius);
	cairo_arc (cr, r - local_corner_radius, b - local_corner_radius, local_corner_radius, 0, M_PI/2.0);
	cairo_line_to (cr, l + local_corner_radius, b);
	cairo_arc (cr, l + local_corner_radius, b - local_corner_radius, local_corner_radius, M_PI/2.0, M_PI);
	cairo_line_to (cr, l, t + local_corner_radius);
	cairo_arc (cr, l + local_corner_radius, t + local_corner_radius, local_corner_radius, M_PI, 3/2.0 * M_PI);
	cairo_stroke (cr);

	XFlush(dpy);

}

void drawkb_cairo_DrawFilledPolygon(Display * dpy, cairo_t *cr,
	unsigned int t,
	unsigned int l,
	unsigned int b,
	unsigned int r,
	unsigned int corner_radius,
	float line_width) {

	int npoints;
	int shape;
	int mode;

	npoints = 8;
	shape = Convex;
	mode = CoordModeOrigin;

	double local_corner_radius = corner_radius;

	cairo_set_line_width (cr, line_width);

	cairo_move_to (cr, l + local_corner_radius, t);
	cairo_line_to (cr, r - local_corner_radius, t);
	cairo_arc (cr, r - local_corner_radius, t + local_corner_radius, local_corner_radius, -M_PI/2.0, 0);
	cairo_line_to (cr, r, b - local_corner_radius);
	cairo_arc (cr, r - local_corner_radius, b - local_corner_radius, local_corner_radius, 0, M_PI/2.0);
	cairo_line_to (cr, l + local_corner_radius, b);
	cairo_arc (cr, l + local_corner_radius, b - local_corner_radius, local_corner_radius, M_PI/2.0, M_PI);
	cairo_line_to (cr, l, t + local_corner_radius);
	cairo_arc (cr, l + local_corner_radius, t + local_corner_radius, local_corner_radius, M_PI, 3/2.0 * M_PI);
	cairo_fill (cr);

	XFlush(dpy);

}

/* Graphic context should have already been set. */
void
drawkb_cairo_KbDrawShape(drawkb_p this, cairo_t *cr, signed int angle,
			unsigned int left, unsigned int top,
			XkbDescPtr _kb, XkbShapePtr shape, XkbColorPtr color,
			Bool is_key, float line_width)
{
	printf ("       + This shape is: left=%d, top=%d, angle=%d\n", left, top, angle);

	cairo_save(cr);
	cairo_translate(cr, left, top);
	cairo_rotate(cr, angle*M_PI/1800.0);

	// drawkb_cairo_KbDrawBounds(this->dpy, cr, angle, left, top, _kb, &shape->bounds, line_width);

	XkbOutlinePtr source;
	int i;
	int t, l, b, r;
	//int j;
	int shapes_to_paint = 1;

	if (this->painting_mode == FULL_SHAPE) shapes_to_paint = shape->num_outlines;

	for (i = 0; i < (is_key ? shapes_to_paint : shape->num_outlines); i++) {
		source = &shape->outlines[i];

		double corner_radius = source->corner_radius;

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
			;
		}
		if (source->num_points <= 2) {

			if (this->painting_mode == FULL_SHAPE || this->painting_mode == FLAT_KEY) {

				printf ("       + Painting shape filled. ");
				if ( i % 2 == 0 ) {
					printf ("Color: %f, %f, %f\n", darkcolor.red/65535.0, darkcolor.green/65535.0, darkcolor.blue/65535.0);
					cairo_set_source_rgb(cr, darkcolor.red/65535.0, darkcolor.green/65535.0, darkcolor.blue/65535.0);
				} else {
					printf ("Color: %f, %f, %f\n", background.red/65535.0, background.green/65535.0, background.blue/65535.0);
					cairo_set_source_rgb(cr, background.red/65535.0, background.green/65535.0, background.blue/65535.0);
				}

				drawkb_cairo_DrawFilledPolygon(this->dpy, cr, t, l, b, r, corner_radius, line_width);

			} else {
				printf ("       + Painting shape hollow. ");
				printf ("Color: %f, %f, %f\n", darkcolor.red/65535.0, darkcolor.green/65535.0, darkcolor.blue/65535.0);
				cairo_set_source_rgb(cr, darkcolor.red/65535.0, darkcolor.green/65535.0, darkcolor.blue/65535.0);

				drawkb_cairo_DrawHollowPolygon(this->dpy, cr, t, l, b, r, corner_radius, line_width);

			}
			

		}
	}

	cairo_restore(cr);

	XFlush(this->dpy);

}

void
drawkb_cairo_KbDrawDoodad(drawkb_p this, cairo_t *cr, /*XftFont *f, */signed int angle,
			 unsigned int left, unsigned int top,
			 XkbDescPtr _kb, XkbDoodadPtr doodad, float line_width)
{

	switch (doodad->any.type) {
	case XkbOutlineDoodad:
		drawkb_cairo_KbDrawShape(this, cr, angle + doodad->shape.angle,
					left + doodad->shape.left,
					top + doodad->shape.top, _kb,
					&_kb->geom->shapes[doodad->shape.shape_ndx],
					&_kb->geom->colors[doodad->shape.color_ndx], False, line_width);
		break;
	case XkbSolidDoodad:
		drawkb_cairo_KbDrawShape(this, cr, angle + doodad->shape.angle,
					left + doodad->shape.left,
					top + doodad->shape.top, _kb,
					&_kb->geom->shapes[doodad->shape.shape_ndx],
					&_kb->geom->colors[doodad->shape.color_ndx], False, line_width);
		break;
	case XkbTextDoodad:
/*		XftDrawString8(dw, current, NULL, scale * (left + doodad->text.left), scale * (top + doodad->text.top) + 6, (unsigned char *)doodad->text.text, strlen(doodad->text.text));*/
		break;
	case XkbIndicatorDoodad:
		drawkb_cairo_KbDrawShape(this, cr, angle + doodad->indicator.angle,
					left + doodad->indicator.left,
					top + doodad->indicator.top, _kb,
					&_kb->geom->shapes[doodad->indicator.shape_ndx],
					&_kb->geom->colors[doodad->indicator.on_color_ndx],
					False, line_width);
		break;
	case XkbLogoDoodad:
		drawkb_cairo_KbDrawShape(this, cr, angle + doodad->logo.angle,
					left + doodad->logo.left,
					top + doodad->logo.top, _kb,
					&_kb->geom->shapes[doodad->logo.shape_ndx],
					&_kb->geom->colors[doodad->logo.color_ndx], False,
					line_width);
		break;
	}

}


typedef enum {
	JUST_LEFT,
	JUST_CENTER,
	JUST_RIGHT
} justify_t;

void drawkb_cairo_pango_echo(cairo_t *cr, PangoFontDescription *fontdesc, const char *s, justify_t justify)
{
	PangoLayout *layout;
	
	cairo_save(cr);

	/* Create a PangoLayout, set the font and text */
	layout = pango_cairo_create_layout (cr);

	pango_layout_set_text (layout, s, -1);

	pango_layout_set_font_description (layout, fontdesc);

	PangoRectangle logical_rect;

	pango_layout_get_extents(layout, NULL, &logical_rect);

	if (justify == JUST_CENTER) {

		cairo_translate(cr, (logical_rect.x - logical_rect.width / 2) / PANGO_SCALE, 0 / PANGO_SCALE);

	} else if (justify == JUST_LEFT) {

		cairo_translate(cr, logical_rect.x / PANGO_SCALE, 0 / PANGO_SCALE);

	} else {

		cairo_translate(cr, (logical_rect.x - logical_rect.width) / PANGO_SCALE, 0 / PANGO_SCALE);

	}

//	Inform Pango to re-layout the text with the new transformation
	pango_cairo_update_layout (cr, layout);

	pango_cairo_show_layout (cr, layout);

	/* free the layout object */
	g_object_unref (layout);

	cairo_restore(cr);

}

void drawkb_cairo_load_and_draw_icon(cairo_t *cr, const int x, const int y, const float screen_width, const float screen_height, const char * icon) {

	debug(4, "--> drawkb_cairo_load_and_draw_icon(%p, %d, %d, %f, %f, %s);\n", cr, x, y, screen_width, screen_height, icon);

	if (screen_width <= 0) {
		debug(4, "-----> BAD CALL: width is <= 0\n");
		return;
	}

	if (screen_height <= 0) {
		debug(4, "-----> BAD CALL: height is <= 0\n");
		return;
	}

	cairo_save(cr);

	cairo_surface_t *iqf = cairo_image_surface_create_from_png(icon);

	// If surface is not a "nil" surface...
	if (cairo_surface_get_reference_count (iqf) > 0) {	
		unsigned int file_width = cairo_image_surface_get_width(iqf);
		unsigned int file_height = cairo_image_surface_get_height(iqf);
		if (file_width > 0 && file_height > 0) {
			cairo_translate(cr, x, y);
			printf (" + screen_width, screen_height, file_width, file_height: %f, %f, %d, %d\n", screen_width, screen_height, file_width, file_height);
			cairo_scale (cr, (screen_width/file_width), (screen_height/file_height));
			cairo_set_source_surface(cr, iqf, 0, 0);
			cairo_paint (cr);
		}
	}
	cairo_surface_destroy (iqf);
	cairo_restore(cr);

	debug(4, "<-- drawkb_cairo_load_and_draw_icon();\n");
}

void
drawkb_cairo_KbDrawKey(drawkb_p this, cairo_t *cr, signed int angle,
		  unsigned int left, unsigned int top,
		  XkbDescPtr _kb, XkbKeyPtr key, key_data_t key_data, puticon_t PutIcon,
		  PangoFontDescription *font_unbound_char, PangoFontDescription *font_unbound_string, PangoFontDescription *font_bound, float line_width)
{

	printf ("     + This key is: left=%d, top=%d, angle=%d\n", left, top, angle);

	cairo_save(cr);
	cairo_translate(cr, left, top);
	cairo_rotate(cr, angle*M_PI/1800.0);

	unsigned int fixed_num_keys;
	unsigned long i;

	char buf[1024]="";
	int buf_n = 1023;

	drawkb_cairo_KbDrawShape(this, cr, angle,
				0, 0, _kb,
				&_kb->geom->shapes[key->shape_ndx],
				&_kb->geom->colors[key->color_ndx], True, line_width);

	/* This is to work around an XKB apparent bug. */
	fixed_num_keys = _kb->names->num_keys;
	if (!fixed_num_keys)
		fixed_num_keys = 256;

	for (i = 0; i < fixed_num_keys; i++) {
		char name[5] = "";
		char glyph[256] = "";
		char keystring[256] = "";
		char *kss;

		debug(19, "%d / %s / %s /\n", i, key->name.name, _kb->names->keys[i].name);
		if (!strncmp(key->name.name, _kb->names->keys[i].name, 4)) {
			strncpy(name, _kb->names->keys[i].name, 4);
			KeySym ks;
			ks = XKeycodeToKeysym(this->dpy, i, 0);
			kss = XKeysymToString(ks);
			if (kss) {
				strncpy(keystring, kss, 255);
				printf ("     + Key name: %s\n", kss);
				kss = drawkb_cairo_LookupKeylabelFromKeystring(kss);

				if (!kss) continue;
		
				strncpy(glyph, kss, 255);

				if (this->IQF(XStringToKeysym(keystring), 0, buf, buf_n) == EXIT_SUCCESS) {

					cairo_translate(cr, key_data.labelbox.x1, key_data.labelbox.y1);

					cairo_set_source_rgb(cr, foreground.red/65535.0, foreground.green/65535.0, foreground.blue/65535.0);

					debug(8, "[pe] a1: %s\n", cairo_status_to_string(cairo_status(cr)));
					drawkb_cairo_pango_echo(cr, font_bound, glyph, JUST_LEFT);
					debug(8, "[pe] a2: %s\n", cairo_status_to_string(cairo_status(cr)));

					if (strcmp(buf, "") != 0) {

						int size = key_data.labelbox.x2 - key_data.labelbox.x1;

						if (key_data.fullbox.y2 - key_data.labelbox.y2 < size) 
							size = key_data.fullbox.y2 - key_data.labelbox.y2;

						cairo_save(cr);

						drawkb_cairo_load_and_draw_icon(cr, key_data.fullbox.x2 - size, key_data.fullbox.y2 - size, size, size, buf);

						cairo_restore(cr);

					}

				} else {
					if (this->painting_mode == FLAT_KEY) {
						cairo_set_source_rgb(cr, background.red/65535.0, background.green/65535.0, background.blue/65535.0);
					} else {
						cairo_set_source_rgb(cr, lightcolor.red/65535.0, lightcolor.green/65535.0, lightcolor.blue/65535.0);
					}

					if (strlen(kss) == 1) {

						cairo_save(cr);

						printf("labelbox: %d, %d\n", key_data.labelbox.x1, key_data.labelbox.y1);

						cairo_translate(cr, (key_data.labelbox.x2 + key_data.labelbox.x1) / 2, key_data.labelbox.y1);

						debug(8, "[pe] b1: %s\n", cairo_status_to_string(cairo_status(cr)));
						drawkb_cairo_pango_echo(cr, font_unbound_char, glyph, JUST_CENTER);
						debug(8, "[pe] b2: %s\n", cairo_status_to_string(cairo_status(cr)));

						cairo_restore(cr);

					} else {

						debug (12, "[ft] baseline: %f\n", g_baseline);

						cairo_save(cr);

						cairo_translate(cr, key_data.labelbox.x1, key_data.labelbox.y1);

						debug(8, "[pe] c1: %s\n", cairo_status_to_string(cairo_status(cr)));
						drawkb_cairo_pango_echo(cr, font_unbound_string, kss, JUST_LEFT);
						debug(8, "[pe] c2: %s\n", cairo_status_to_string(cairo_status(cr)));

						cairo_restore(cr);

					}

				}
			}
			break;
		}
	}

	cairo_restore(cr);
}

PangoRectangle * drawkb_cairo_get_rendered_extents_alloc(drawkb_p this, cairo_t *cr, PangoFontDescription **fontdesc, const char *s) {

	cairo_save(cr);

	PangoLayout *layout;
	PangoRectangle *ink_rect;
    PangoRectangle *logical_rect;

	ink_rect = malloc(sizeof(PangoRectangle));
	logical_rect = malloc(sizeof(PangoRectangle));

	layout = pango_cairo_create_layout(cr);

	pango_layout_set_font_description (layout, *fontdesc);

	pango_layout_set_text (layout, s, -1);

	pango_cairo_update_layout (cr, layout);

	pango_layout_get_extents(layout, ink_rect, logical_rect);

	g_object_unref (layout);

	free(ink_rect);

	cairo_restore(cr);

	return logical_rect;

}

void my_pango_font_description_set_size (PangoFontDescription *desc, gint size)
{
	if (size <= 0) return;
	pango_font_description_set_size(desc, size);
}

int drawkb_cairo_increase_to_best_size_by_height(drawkb_p this, cairo_t *cr, XkbBoundsRec labelbox, PangoFontDescription **fontdesc, const char *s, unsigned int *size)
{

	PangoRectangle *extents;

	debug (10, " --> %s (labelbox(x1=%d, y1=%d, x2=%d, y2=%d), s=%s, size=%d\n", __func__, labelbox.x1, labelbox.y1, labelbox.x2, labelbox.y2, s, *size);

	int labelbox_height = labelbox.y2 - labelbox.y1;

	float size_now = 100000.0;
	float size_last = 0;
	float saved_size_now = 0;

	size_now = *size;
	size_last = *size / 2;

	if (*size == 0) {
		size_now = 100000.0;
		size_last = 0;
	}

	my_pango_font_description_set_size(*fontdesc, size_now);

	extents = drawkb_cairo_get_rendered_extents_alloc(this, cr, fontdesc, s);

	debug(11, " == size_now, size_last: %f, %f\n", size_now, size_last);

	debug(11, " == extents_h vs labelbox_h: %d, %d\n", extents->height / PANGO_SCALE, labelbox_height);

	#define W_PRECISION 20
	#define H_PRECISION 20

	while ( fabs(size_now - size_last) > PANGO_SCALE )
	{
		debug(13, " ===== Not within height precision yet: %f %f\n", size_now, size_last );
		saved_size_now = size_now;
		if ((extents->height / PANGO_SCALE) < labelbox_height) {
			debug(13, " ===== (extents->height / PANGO_SCALE) < labelbox_height\n");
			if (size_now > size_last) size_now = size_now * 2;
			if (size_now < size_last) size_now = (size_now + size_last ) / 2;
		} else if ((extents->height / PANGO_SCALE) > labelbox_height) {
			debug(13, " ===== (extents->height / PANGO_SCALE) > labelbox_height\n");
			if (size_now < size_last) size_now = size_now / 2;
			if (size_now > size_last) size_now = (size_now + size_last ) / 2;
		}
		size_last = saved_size_now;

		free(extents);

		my_pango_font_description_set_size(*fontdesc, size_now);

		extents = drawkb_cairo_get_rendered_extents_alloc(this, cr, fontdesc, s);

		debug(11, " == size_now, size_last: %f, %f\n", size_now, size_last);

		debug(11, " == extents_h vs labelbox_h: %d, %d\n", extents->height / PANGO_SCALE, labelbox_height);

	}

	debug(13, " ===== Enough precision: %f %f\n", size_now, size_last );

	debug(10, " <-- %s final size value: %f\n", __func__, size_now);

	*size = size_now;

	return size_now;

}

int drawkb_cairo_reduce_to_best_size_by_width(drawkb_p this, cairo_t *cr, XkbBoundsRec labelbox, PangoFontDescription **fontdesc, const char *s, unsigned int *size)
{

	PangoRectangle *extents;

	debug (10, " --> %s (labelbox(x1=%d, y1=%d, x2=%d, y2=%d), s=%s, size=%d\n", __func__, labelbox.x1, labelbox.y1, labelbox.x2, labelbox.y2, s, *size);

	int labelbox_width = labelbox.x2 - labelbox.x1;

	float size_now = 100000.0;
	float size_last = 0;
	float saved_size_now = 0;

	size_now = *size;
	size_last = *size / 2;

	if (*size == 0) {
		size_now = 100000.0;
		size_last = 0;
	}

	my_pango_font_description_set_size(*fontdesc, size_now);

	extents = drawkb_cairo_get_rendered_extents_alloc(this, cr, fontdesc, s);

	debug(11, " == size_now, size_last: %f, %f\n", size_now, size_last);

	debug(11, " == extents_w vs labelbox_w: %d, %d\n", extents->width / PANGO_SCALE, labelbox_width);

	#define W_PRECISION 20
	#define H_PRECISION 20

	if (extents->width / PANGO_SCALE <= labelbox_width)
		return size_now;

	/* Find best for width */
	while ( abs(size_now - size_last) > PANGO_SCALE )
	{
		debug(13, " ===== Not within height precision yet: %f %f\n", size_now, size_last );
		saved_size_now = size_now;
		if ((extents->width / PANGO_SCALE) < labelbox_width) {
			debug(13, " ===== (extents->width / PANGO_SCALE) < labelbox_width\n");
			if (size_now > size_last) size_now = size_now * 2;
			if (size_now < size_last) size_now = (size_now + size_last ) / 2;
		} else if ((extents->width / PANGO_SCALE) > labelbox_width) {
			debug(13, " ===== (extents->width / PANGO_SCALE) > labelbox_width\n");
			if (size_now < size_last) size_now = size_now / 2;
			if (size_now > size_last) size_now = (size_now + size_last ) / 2;
		}
		size_last = saved_size_now;

		free(extents);

		my_pango_font_description_set_size(*fontdesc, size_now);

		extents = drawkb_cairo_get_rendered_extents_alloc(this, cr, fontdesc, s);

		debug(11, " == size_now, size_last: %f, %f\n", size_now, size_last);

		debug(11, " == extents_w vs labelbox_w: %d, %d\n", extents->width / PANGO_SCALE, labelbox_width);

	}

	debug(13, " ===== Enough precision: %f %f\n", size_now, size_last );

	debug(10, " <-- %s final size value: %f\n", __func__, size_now);

	*size = size_now;

	return size_now;

}


void
drawkb_cairo_KbDrawRow(drawkb_p this, cairo_t *cr, signed int angle,
		  unsigned int left, unsigned int top,
		  XkbDescPtr _kb, XkbRowPtr row, puticon_t PutIcon, float line_width)
{

	PangoFontDescription *font_unbound_char;
	PangoFontDescription *font_unbound_string;
	PangoFontDescription *font_bound;

	font_unbound_string = pango_font_description_from_string(this->font);
	font_unbound_char = pango_font_description_from_string(this->font);
	font_bound = pango_font_description_from_string(this->font);

	printf ("   + This row is: left=%d, top=%d, angle=%d\n", left, top, angle);

	cairo_save(cr);
	cairo_translate(cr, left, top);
	cairo_rotate(cr, angle*M_PI/1800.0);

	unsigned int i;
	unsigned int next_piece = 0;

	XkbBoundsRec labelbox;
	XkbBoundsRec fullbox;

	// drawkb_cairo_KbDrawBounds(this->dpy, w, cr, angle, scale, row->left, row->top, _kb, &row->bounds);

	unsigned int size_bound = 0;
	unsigned int size_unbound_char = 0;
	unsigned int size_unbound_string = 0;

	/* This is to work around an XKB apparent bug. */
	unsigned int fixed_num_keys = _kb->names->num_keys;
	if (!fixed_num_keys)
		fixed_num_keys = 256;

	unsigned int j;

	key_data_t *key_data = NULL;
	unsigned int key_data_n = 0;

	int already_increased_size_bound = 0;
	int already_increased_size_unbound_char = 0;
	int already_increased_size_unbound_string = 0;

	for (j = 0; j < row->num_keys; j++) {
		XkbKeyPtr key = &row->keys[j];

		debug (4, "drawkb_cairo_KbDrawRow: processing key j=%d\n ", j);

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
			ks = XKeycodeToKeysym(this->dpy, i, 0);
			kss = XKeysymToString(ks);

			if (!kss)
				continue;

			strncpy(keystring, kss, 255);

			printf ("     + Calculating best font size for \"%s\"\n", kss);

			kss = drawkb_cairo_LookupKeylabelFromKeystring(kss);

			if (!kss)
				continue;

			strncpy(glyph, kss, 255);

			/* Calculate label + icon box bounds */
			int fullbox_border = line_width;
			int fullbox_margin = line_width / 2;

			XkbBoundsRec kr, *k = &kr;
			
			if (this->painting_mode == FULL_SHAPE) {
				XkbComputeShapeTop(&_kb->geom->shapes[key->shape_ndx], k);
			} else if (this->painting_mode == BASE_OUTLINE_ONLY) {
				k = &_kb->geom->shapes[key->shape_ndx].bounds;
				fullbox_border = line_width;
			} else if (this->painting_mode == FLAT_KEY) {
				k = &_kb->geom->shapes[key->shape_ndx].bounds;
			} else {
				assert (0);
			}

			fullbox.x1 = k->x1 + fullbox_margin + fullbox_border;
			fullbox.x2 = k->x2 - fullbox_margin - fullbox_border;
			fullbox.y1 = k->y1 + fullbox_margin + fullbox_border;
			fullbox.y2 = k->y2 - fullbox_margin - fullbox_border;
			/* End calculate label + icon box bounds */

			/* Default labelbox. Overriden according to key binding status. */
			labelbox.x1 = fullbox.x1;
			labelbox.x2 = fullbox.x2;
			labelbox.y1 = fullbox.y1;
			labelbox.y2 = fullbox.y2;

			if (strcmp(glyph, "") != 0) {
				if (this->IQF(XStringToKeysym(keystring), 0, NULL, 0) == EXIT_SUCCESS) {
					/* If this key is a bound key... */
					labelbox.y2 = labelbox.y1 + (labelbox.y2 - labelbox.y1) * 0.33;
					if (!already_increased_size_bound) {
						drawkb_cairo_increase_to_best_size_by_height(this, cr, labelbox, &font_bound, glyph, &size_bound);
						already_increased_size_bound = 1;
					}
					drawkb_cairo_reduce_to_best_size_by_width(this, cr, labelbox, &font_bound, glyph, &size_bound);
					printf ("       + Computed size %d as a bound key.\n", size_bound);
				} else if (strlen(glyph) == 1) {
					/* If this key is a single char unbound key... */
					if (!already_increased_size_unbound_char) {
						drawkb_cairo_increase_to_best_size_by_height(this, cr, labelbox, &font_bound, glyph, &size_unbound_char);
						already_increased_size_unbound_char = 1;
					}
					drawkb_cairo_reduce_to_best_size_by_width(this, cr, labelbox, &font_unbound_char, glyph, &size_unbound_char);
					printf ("       + Computed size %d as a single-char unbound key.\n", size_unbound_char);
				} else {
					/* This is a multiple char unbound key. */
					labelbox.x1 += 20;
					labelbox.x2 -= 20;
					labelbox.y1 = fullbox.y1 + (fullbox.y2 - fullbox.y1) * 0.5;
					labelbox.y2 = fullbox.y1 + (fullbox.y2 - fullbox.y1) * 0.75;
					if (!already_increased_size_unbound_string) {
						drawkb_cairo_increase_to_best_size_by_height(this, cr, labelbox, &font_bound, glyph, &size_unbound_string);
						already_increased_size_unbound_string = 1;
					}
					drawkb_cairo_reduce_to_best_size_by_width(this, cr, labelbox, &font_unbound_string, glyph, &size_unbound_string);
					printf ("       + Computed size %d as a multichar unbound key.\n", size_unbound_string);
				}
				printf ("       + Its labelbox is (x1, x2, y1, y2): %d, %d, %d, %d\n", labelbox.x1, labelbox.x2, labelbox.y1, labelbox.y2);
				printf ("       + Its fullbox is (x1, x2, y1, y2): %d, %d, %d, %d\n", fullbox.x1, fullbox.x2, fullbox.y1, fullbox.y2);
			}


			memcpy(&(key_data[key_data_n-1].labelbox), &labelbox, sizeof(XkbBoundsRec));
			memcpy(&(key_data[key_data_n-1].fullbox), &fullbox, sizeof(XkbBoundsRec));
			key_data[key_data_n-1].glyph = glyph;

			break;

		}
	}

	printf(" -- Best font sizes calculated: %d, %d, %d\n", size_unbound_string, size_unbound_char, size_bound);

	my_pango_font_description_set_size(font_unbound_string, size_unbound_string);
	my_pango_font_description_set_size(font_unbound_char, size_unbound_char);
	my_pango_font_description_set_size(font_bound, size_bound);

	for (i = 0; i < row->num_keys; i++) {

		for (j = 0; j < key_data_n && key_data[j].index != i; j++);
		assert(j < key_data_n);

		if (!row->vertical) {
			drawkb_cairo_KbDrawKey(this, cr, 0,
					  row->left + next_piece + row->keys[i].gap,
					  row->top,
					  _kb, &row->keys[i], key_data[i], PutIcon, font_unbound_char, font_unbound_string, font_bound, line_width);
			next_piece +=
				_kb->geom->shapes[row->keys[i].shape_ndx].bounds.x2 + row->keys[i].gap;

			cairo_save(cr);
//			drawkb_cairo_DrawHollowPolygon(this->dpy, cr, row->left + next_piece + row->keys[i].gap+labelbox.x1, row->top+labelbox.y1, row->top+labelbox.y2, row->left + next_piece + row->keys[i].gap+labelbox.x2, 0, line_width);
			cairo_restore(cr);

		} else {
			drawkb_cairo_KbDrawKey(this, cr, 0,
					  row->left, row->top + next_piece + row->keys[i].gap,
					  _kb, &row->keys[i], key_data[i], PutIcon, font_unbound_char, font_unbound_string, font_bound, line_width);
			next_piece +=
				_kb->geom->shapes[row->keys[i].shape_ndx].bounds.y2 + row->keys[i].gap;

			cairo_save(cr);
//			drawkb_cairo_DrawHollowPolygon(this->dpy, cr, row->left + next_piece + row->keys[i].gap, row->top+labelbox.y1, row->top+labelbox.y2, row->left + next_piece + row->keys[i].gap+labelbox.x2, 0, line_width);
			cairo_restore(cr);

		}
	}

	free(key_data);

	cairo_restore(cr);

}

void
drawkb_cairo_KbDrawSection(drawkb_p this, cairo_t *cr, signed int angle,
			  unsigned int left, unsigned int top,
			  XkbDescPtr _kb, XkbSectionPtr section, puticon_t PutIcon, float line_width)
{
	int i, p;

    // drawkb_cairo_KbDrawBounds(this->dpy, w, gc, angle, left + section->left, top + section->top, _kb, &section->bounds, line_width);

	if (section->name)
		debug(7, "[dr] Drawing section: %s\n", XGetAtomName(this->dpy, section->name));

	if (section->name)
		printf (" + This section is: mame=%s, left=%d, top=%d, angle=%d\n", XGetAtomName(this->dpy, section->name), left, top, angle);
	else 
		printf (" + This section is: mame=%s, left=%d, top=%d, angle=%d\n", "(Unnamed)", left, top, angle);

	cairo_save(cr);
	cairo_translate(cr, left, top);
	cairo_rotate(cr, angle*M_PI/1800.0);

	for (i = 0; i < section->num_rows; i++) {
		XkbComputeRowBounds(_kb->geom, section, &section->rows[i]);
		drawkb_cairo_KbDrawRow(this, cr, angle + section->angle,
				  section->left, top + section->top, _kb,
				  &section->rows[i], PutIcon, line_width);
	}

	for (p = 0; p <= 255; p++) {
		for (i = 0; i < section->num_doodads; i++) {
			if (section->doodads[i].any.priority == p) {
				drawkb_cairo_KbDrawDoodad(this, cr, angle + section->angle,
							 section->left, top + section->top, _kb,
							 &section->doodads[i], line_width);
			}
		}
	}

	cairo_restore(cr);

}

void
drawkb_cairo_drawkb_cairo_KbDrawComponents(drawkb_p this, cairo_t *cr, signed int angle,
				 unsigned int left, unsigned int top,
				 XkbDescPtr _kb, XkbSectionPtr sections,
				 int sections_n, XkbDoodadPtr doodads, int doodads_n, puticon_t PutIcon, float line_width)
{
	int i, p;

	/* FIXME: This algorithm REALLY NEEDS AND CRYING BEGS for optimization.
	 * Indexing sections and doodads into a binary or balanced tree would be
	 * the best.
	 */

	printf ("This component is: left=%d, top=%d, angle=%d\n", left, top, angle);
	cairo_save(cr);
	cairo_translate(cr, left, top);
	cairo_rotate(cr, angle*M_PI/1800.0);

	for (p = 0; p <= 255; p++) {
		for (i = 0; i < sections_n; i++) {
			if (sections[i].priority == p) {
				drawkb_cairo_KbDrawSection(this, cr, 0, left,
							  top, _kb, &sections[i], PutIcon, line_width);
			}
		}

		for (i = 0; i < doodads_n; i++) {
			if (doodads[i].any.priority == p) {
				drawkb_cairo_KbDrawDoodad(this, cr, 0, left,
							 0, _kb, &doodads[i], line_width);
//							 top, _kb, &doodads[i]);
			}
		}
	}

	cairo_restore(cr);

}

void drawkb_cairo_draw(drawkb_p this, Drawable d, GC gc, unsigned int width, unsigned int height, XkbDescPtr kbdesc, puticon_t PutIcon)
{

	cairo_t *cr;

	float scale;

	Display *dpy = this->dpy;

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

	debug(10, "rgb: %d, %d, %d\n", background.red, background.green, background.blue);
	debug(10, "rgb: %d, %d, %d\n", foreground.red, foreground.green, foreground.blue);
	debug(10, "rgb: %d, %d, %d\n", lightcolor.red, lightcolor.green, lightcolor.blue);
	debug(10, "rgb: %d, %d, %d\n", darkcolor.red, darkcolor.green, darkcolor.blue);

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
	/* scale = 0.1; */

	int left = 0;
	int top = 0;
	int angle = 0;

	/* Draw top-level rectangle */
//	XDrawRectangle(this->dpy, d, cr, left, top, scale * kbdesc->geom->width_mm,
//				   scale * kbdesc->geom->height_mm);

	surface = cairo_xlib_surface_create (dpy, d, DEFAULT_VISUAL(dpy), kbgeom->width_mm, kbgeom->height_mm);
	cr = cairo_create (surface);

	cairo_scale(cr, scale, scale);

	PangoContext *pc = pango_cairo_create_context(cr);
	PangoFontDescription *fontdesc;
	PangoFontMetrics *pm;

	fontdesc = pango_font_description_from_string(this->font);

	pm = pango_font_get_metrics(pango_context_load_font(pc, fontdesc), NULL);
	int asc = pango_font_metrics_get_ascent(pm);
	int des = pango_font_metrics_get_descent(pm);

	debug (11, "[fm] asc, desc = %d, %d\n", asc, des);

	g_baseline =
		(float) asc / (asc + des);

	g_object_unref(pc);

	float line_width = 3 / scale;

	/* Draw each component (section or doodad) of the top-level kbdesc->geometry, in
	 * priority order. Note that priority handling is left to the function. */
	drawkb_cairo_drawkb_cairo_KbDrawComponents(this, cr, angle, left, top, kbdesc,
					 kbdesc->geom->sections, kbdesc->geom->num_sections,
					 kbdesc->geom->doodads, kbdesc->geom->num_doodads, PutIcon, line_width);

	XFlush(this->dpy);


}

/* Checks for font existance and tries to fallback if not. */
int drawkb_cairo_Init_Font(drawkb_p this, const char *font)
{

	if (!font) {
		fprintf(stderr, "User didn't specify font.\n");
	}

	strncpy(this->font, font, 499);

	/* Try 1: User drawkb_configured. */
	if (this->font) {


		/* Dummy */
		return EXIT_SUCCESS;

		fprintf(stderr, "Failed to initialize user configured font.\n");

	}

	/* FILLTHEGAP */
	/* Try 2: Ask NETWM (like in a skin). */

	/* Try 3: Fallback to XKB's. */
	if (kbdesc->geom->label_font) {

		/* Dummy */
		return EXIT_SUCCESS;

	}

	/* FILLTHEGAP */
	/* Try 4: Ask for whatever ("*-iso8859-1" && !"*-symbol") font. */

	return EXIT_FAILURE;
}

drawkb_p drawkb_cairo_create(Display *dpy, const char *font,
	IQF_t IQF, painting_mode_t painting_mode, float scale, debug_t *debug)
{

	drawkb_p this = (drawkb_p) malloc(sizeof(drawkb_t));

	this->IQF = IQF;

	this->painting_mode = painting_mode;

	this->dpy = dpy;

	this->debug = debug;

	/* drawkb_cairo_Init_Font needs drawkb_cairo_Init_Geometry to succeed, because one of
	 * the fallback fonts is the XKB's specified font label, and
	 * therefore, geometry must be loaded. */

	if (drawkb_cairo_Init_Font(this, font) == EXIT_FAILURE)
	{
		fprintf(stderr, "Failed to initialize font: %s.\n"
			"Possible causes are:\n"
			" + You did not quote the name and the name contains spaces.\n"
			" + The font doesn't exist.\n", font);
		return NULL;
	}

	drawkb_cairo_WorkaroundBoundsBug(dpy, kbdesc);

	return this;

}

int drawkblibs_cairo_init(
	drawkb_create_t *ret_create,
	drawkb_draw_t *ret_draw)
{

	*ret_create = drawkb_cairo_create;
	*ret_draw = drawkb_cairo_draw;

	return EXIT_SUCCESS;
}

