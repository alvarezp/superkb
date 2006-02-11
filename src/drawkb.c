/* License: GPL v2. */
/* To be compiled with:
 *    gcc -o superkb proto.c -ansi -lX11 -L/usr/X11/lib
 *
 * Bibliography: XKBlib.pdf.
 */

#include <X11/Xlib.h>

#include <X11/XKBlib.h>
#include <X11/extensions/XKBgeom.h>

#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

Font F;

void KbDrawBounds(Display *dpy, Drawable w, GC gc, unsigned int angle,
    double scale, unsigned int left, unsigned int top, XkbDescPtr _kb,
    XkbBoundsPtr bounds)
{
    GC gc_backup;

    /* FIXME: I know, there must be fast ways, but this was faster to code.*/
    memcpy(&gc_backup, &gc, sizeof(GC));

    XSetLineAttributes(dpy, gc, 1, LineOnOffDash, CapRound, JoinRound);
    XDrawRectangle(dpy, w, gc, scale*(left+bounds->x1),
        scale*(top+bounds->y1),
        scale*(bounds->x2-bounds->x1),
        scale*(bounds->y2-bounds->y1));

    memcpy(&gc, &gc_backup, sizeof(GC));
}

/* Graphic context should have already been set. */
void KbDrawShape(Display *dpy, Drawable w, GC gc, unsigned int angle,
    double scale, unsigned int left, unsigned int top, XkbDescPtr _kb,
    XkbShapePtr shape, XkbColorPtr color)
{
    /* FIXME: Should manage angled shapes. */
    /* FIXME: Set color too. */
    /* FIXME: Should paint actual shape. */

    XkbComputeShapeBounds(shape);

    /*KbDrawBounds(dpy, w, gc, angle, scale, left, top, _kb, &shape->bounds);*/

    XkbOutlinePtr source;
    int line_by_line;

    if (shape->primary != NULL) {
        source = shape->primary;
        line_by_line = 1;
    } else if (shape->approx != NULL) {
        source = shape->approx;
        line_by_line = 0;
    } else {
        source = &shape->outlines[0];
        line_by_line = 0;
    }

    XSetLineAttributes(dpy, gc, 2, LineSolid, CapRound, JoinRound);
    if (line_by_line) {
        int j;
        for (j=0; j<source->num_points-1; j++) {
            XDrawLine(dpy, w, gc, scale*(left+source->points[j].x),
                scale*(top+source->points[j].y),
                scale*(left+source->points[j+1].x),
                scale*(top+source->points[j+1].y));
        }
        XDrawLine(dpy, w, gc, scale*(left+source->points[j].x),
            scale*(top+source->points[j].y),
            scale*(left+source->points[0].x),
            scale*(top+source->points[0].y));
    } else {

        /* Looks like some times points are swapped. Test to workaround. */

        int t, l, b, r;
        if (source->points[1].x >= source->points[0].x) {
            t = source->points[0].y;
            l = source->points[0].x;
            b = source->points[1].y;
            r = source->points[1].x;
        } else {
            t = source->points[1].y;
            l = source->points[1].x;
            b = source->points[0].y;
            r = source->points[0].x;
        }

        XDrawLine(dpy, w, gc, scale*(left+l+source->corner_radius),
            scale*(top+t),
            scale*(left+r-source->corner_radius),
            scale*(top+t));
        XDrawArc(dpy, w, gc, scale*(left+l), scale*(top+t),
            2*scale*(source->corner_radius), 2*scale*(source->corner_radius), 5760, 5760);
        XDrawLine(dpy, w, gc, scale*(left+r),
            scale*(top+t+source->corner_radius),
            scale*(left+r),
            scale*(top+b-source->corner_radius));
        XDrawArc(dpy, w, gc, scale*(left+r-2*source->corner_radius), scale*(top+t),
            2*scale*(source->corner_radius), 2*scale*(source->corner_radius), 0, 5760);
        XDrawLine(dpy, w, gc, scale*(left+l+source->corner_radius),
            scale*(top+b),
            scale*(left+r-source->corner_radius),
            scale*(top+b));
        XDrawArc(dpy, w, gc, scale*(left+r-2*source->corner_radius), scale*(top+b-2*source->corner_radius),
            2*scale*(source->corner_radius), 2*scale*(source->corner_radius), 17280, 5760);
        XDrawLine(dpy, w, gc, scale*(left+l),
            scale*(top+t+source->corner_radius),
            scale*(left+l),
            scale*(top+b-source->corner_radius));
        XDrawArc(dpy, w, gc, scale*(left+l), scale*(top+b-2*source->corner_radius),
            2*scale*(source->corner_radius), 2*scale*(source->corner_radius), 11520, 5760);
            
        
    }
    XFlush(dpy);
/*    sleep(1);*/

}

void KbDrawDoodad(Display *dpy, Drawable w, GC gc, unsigned int angle,
    double scale, unsigned int left, unsigned int top,
    XkbDescPtr _kb, XkbDoodadPtr doodad)
{
    switch (doodad->any.type) {
    case XkbOutlineDoodad:
        KbDrawShape(dpy, w, gc, angle+doodad->shape.angle, scale,
        left + doodad->shape.left, top+doodad->shape.top,
        _kb, &_kb->geom->shapes[doodad->shape.shape_ndx], &_kb->geom->colors[doodad->shape.color_ndx]);
        break;
    case XkbSolidDoodad:
        KbDrawShape(dpy, w, gc, angle+doodad->shape.angle, scale,
        left + doodad->shape.left, top+doodad->shape.top,
        _kb, &_kb->geom->shapes[doodad->shape.shape_ndx], &_kb->geom->colors[doodad->shape.color_ndx]);
        break;
    case XkbTextDoodad:
        XDrawString(dpy, w, gc, scale*(left+doodad->text.left), scale*(top+doodad->text.top)+6, doodad->text.text, strlen(doodad->text.text));
        break;
    case XkbIndicatorDoodad:
        KbDrawShape(dpy, w, gc, angle+doodad->indicator.angle, scale,
        left + doodad->indicator.left, top+doodad->indicator.top,
        _kb, &_kb->geom->shapes[doodad->indicator.shape_ndx], &_kb->geom->colors[doodad->indicator.on_color_ndx]);
        break;
    case XkbLogoDoodad:
        KbDrawShape(dpy, w, gc, angle+doodad->logo.angle, scale,
        left + doodad->logo.left, top+doodad->logo.top,
        _kb, &_kb->geom->shapes[doodad->logo.shape_ndx], &_kb->geom->colors[doodad->logo.color_ndx]);
        break;
    }
}

void KbDrawKey(Display *dpy, Drawable w, GC gc, unsigned int angle,
    double scale, unsigned int left, unsigned int top,
    XkbDescPtr _kb, XkbKeyPtr key)
{
    int fixed_num_keys;
    unsigned long i;

    KbDrawShape(dpy, w, gc, angle, scale, left+key->gap,
    top, _kb, &_kb->geom->shapes[key->shape_ndx], &_kb->geom->colors[key->color_ndx]);

    /* This is to work around an XKB apparent bug. */
    fixed_num_keys = _kb->names->num_keys;
    if (!fixed_num_keys) fixed_num_keys = 256;

    for (i=0; i<fixed_num_keys; i++)
    {
        char name[5]="";
        char glyph[256]="";
        char *kss;

        if (!strncmp(key->name.name, _kb->names->keys[i].name, 4)) {
            strncpy(name, _kb->names->keys[i].name, 4);
            KeySym ks;
            ks = XKeycodeToKeysym(dpy, i, 0);
            kss = XKeysymToString(ks);
            if (kss){
                /* FIXME: Better compare integer, not strings. Fixable. */
                if (!strcmp(kss, "Escape")) kss = "Esc";
                if (!strcmp(kss, "comma")) kss = ",";
                if (!strcmp(kss, "period")) kss = ".";
                if (!strcmp(kss, "slash")) kss = "/";
                if (!strcmp(kss, "minus")) kss = "-";
                if (!strcmp(kss, "equal")) kss = "=";
                if (!strcmp(kss, "Caps_Lock")) kss = "Caps Lock";
                if (!strcmp(kss, "Shift_L")) kss = "Shift";
                if (!strcmp(kss, "Shift_R")) kss = "Shift";
                if (!strcmp(kss, "semicolon")) kss = ";";
                if (!strcmp(kss, "Return")) kss = "Enter";
                if (!strcmp(kss, "Control_L")) kss = "Control";
                if (!strcmp(kss, "Control_R")) kss = "Control";
                if (!strcmp(kss, "Alt_L")) kss = "Alt";
                if (!strcmp(kss, "KP_Enter")) kss = "Enter";
                if (!strcmp(kss, "KP_Add")) kss = "+";
                if (!strcmp(kss, "KP_Subtract")) kss = "-";
                if (!strcmp(kss, "KP_Multiply")) kss = "*";
                if (!strcmp(kss, "KP_Divide")) kss = "/";
                if (!strcmp(kss, "Num_Lock")) kss = "Num Lock";
                if (!strcmp(kss, "KP_Home")) kss = "Home";
                if (!strcmp(kss, "KP_End")) kss = "End";
                if (!strcmp(kss, "KP_Prior")) kss = "PgUp";
                if (!strcmp(kss, "KP_Up")) kss = "Up";
                if (!strcmp(kss, "KP_Down")) kss = "Down";
                if (!strcmp(kss, "KP_Left")) kss = "Left";
                if (!strcmp(kss, "KP_Right")) kss = "Right";
                if (!strcmp(kss, "KP_Next")) kss = "Next";
                if (!strcmp(kss, "KP_Begin")) kss = "Begin";
                if (!strcmp(kss, "KP_Insert")) kss = "Ins";
                if (!strcmp(kss, "KP_Delete")) kss = "Del";
                if (!strcmp(kss, "Scroll_Lock")) kss = "Scroll Lock";
                if (!strcmp(kss, "bracketleft")) kss = "[";
                if (!strcmp(kss, "bracketright")) kss = "]";
                if (!strcmp(kss, "backslash")) kss = "\\";
                strncpy(glyph, kss, 255);

                XFontStruct *fs = XQueryFont(dpy, F);
                unsigned int tw = XTextWidth(fs, kss, strlen(kss));

                /* "b" is just to abbreviate the otherwise long code. */
                XkbBoundsPtr b = &(_kb->geom->shapes[key->shape_ndx].bounds);
                XDrawString(dpy, w, gc,
                    scale*(left+key->gap+(b->x1+b->x2)/2)-tw/2,
                    scale*(top+(b->y1+b->y2)/2)+6, kss, strlen(kss));

                XFreeFontInfo(NULL, fs, 1);
            }
        }
    }

}

void KbDrawRow(Display *dpy, Drawable w, GC gc, unsigned int angle,
    double scale, unsigned int left, unsigned int top,
    XkbDescPtr _kb, XkbRowPtr row)
{
    int i;
    unsigned int next_piece=0;

    /*KbDrawBounds(dpy, w, gc, angle, scale, left, top, _kb, &row->bounds); */

    for (i=0; i<row->num_keys; i++)
    {
        if (!row->vertical) {
            KbDrawKey(dpy, w, gc, angle, scale,
                left+row->left+next_piece, top+row->top,
                _kb, &row->keys[i]);
            next_piece += _kb->geom->shapes[row->keys[i].shape_ndx].bounds.x2 + row->keys[i].gap;
        } else {
            KbDrawKey(dpy, w, gc, angle, scale,
                left+row->left, top+row->top+next_piece,
                _kb, &row->keys[i]);
            next_piece += _kb->geom->shapes[row->keys[i].shape_ndx].bounds.y2 + row->keys[i].gap;
        }
    }

}

void KbDrawSection(Display *dpy, Drawable w, GC gc, unsigned int angle,
    double scale, unsigned int left, unsigned int top,
    XkbDescPtr _kb, XkbSectionPtr section)
{
    int i, p;

    /* KbDrawBounds(dpy, w, gc, angle, scale, left, top, _kb, &section->bounds); */

    if (section->name) printf("Drawing section: %s\n", XGetAtomName(dpy, section->name));

    for (i=0; i<section->num_rows; i++)
    {
        XkbComputeRowBounds(_kb->geom, section, &section->rows[i]);
        KbDrawRow(dpy, w, gc, angle, scale, left+section->left, top+section->top,
            _kb, &section->rows[i]);
    }

    for (p=0; p<=255; p++)
    {
        for (i=0; i<section->num_doodads; i++)
        {
            if (section->doodads[i].any.priority == p) {
                KbDrawDoodad(dpy, w, gc, angle, scale, left,
                top, _kb, &section->doodads[i]);
            }
        }
    }
}

void KbDrawComponents(Display *dpy, Drawable w, GC gc, unsigned int angle,
    double scale, unsigned int left, unsigned int top,
    XkbDescPtr _kb, XkbSectionPtr sections, int sections_n, 
    XkbDoodadPtr doodads, int doodads_n)
{
    int i, p;

    /* FIXME: This algorithm REALLY NEEDS AND CRYING BEGS for optimization. */
    /* Indexing sections and doodads into a binary tree would be the best. */

    for (p=0; p<=255; p++)
    {
        for (i=0; i<sections_n; i++)
        {
            if (sections[i].priority == p) {
               /* char *s = XGetAtomName(sections[i].name);
                printf("Printing section: %s\n", s);
                XFree(s);*/
                KbDrawSection(dpy, w, gc, angle, scale, left,
                top, _kb, &sections[i]);

            }
        }
        
        for (i=0; i<doodads_n; i++)
        {
            if (doodads[i].any.priority == p) {
                KbDrawDoodad(dpy, w, gc, angle, scale, left,
                top, _kb, &doodads[i]);
            }
        }
    }
}

void KbDrawKeyboard(Display *dpy, Drawable w, GC gc, unsigned int angle,
    double scale, unsigned int left, unsigned int top,
    XkbDescPtr _kb)
{

    /* Draw top-level rectangle */
    XDrawRectangle(dpy, w, gc, left, top, scale*_kb->geom->width_mm,
        scale*_kb->geom->height_mm);

//    F = XLoadFont(dpy, _kb->geom->label_font);
    F = XLoadFont(dpy, "-*-bitstream vera sans-bold-r-*-*-0-0-*-*-*-0-iso8859-1");

    XSetFont(dpy, gc, F);
    char size[200]="";
    printf("%s\n", _kb->geom->label_font);

    printf("%s\n", size);

    /* Draw each component (section or doodad) of the top-level _kb->geometry, in
     * priority order. Note that priority handling is left to the function. */
    KbDrawComponents(dpy, w, gc, 0, scale, left, top, _kb, _kb->geom->sections,
        _kb->geom->num_sections, _kb->geom->doodads, _kb->geom->num_doodads);

}
