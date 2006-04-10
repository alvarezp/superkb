/* License: GPL v2. */
/* To be compiled with:
 *    gcc -o superkb proto.c -ansi -lX11 -L/usr/X11/lib
 *
 * Bibliography: XKBlib.pdf.
 */

/* This module does all the keyboard drawing magic. */

/* DO NOT use XkbComputeShapeBounds(). This module corrects some bounds
 * derived from the bug. Calling the function again will unfix them.
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
#include <ctype.h>
#include <math.h>

#include "drawkb.h"
#include "puticon_gdk_pixbuf_xlib.h"

IconQueryFunc IQF;

char *fontname;
Font F;

int g_size;
double g_baseline;

void WorkaroundBoundsBug(Display * dpy, XkbDescPtr _kb)
{
    int i, j;

    /* To workaround an X11R7.0 bug */
    if (VendorRelease(dpy) < 80000000 &&
        !strcmp(ServerVendor(dpy), "The X.Org Foundation")) {
        for (i = 0; i < _kb->geom->num_shapes; i++) {
            XkbShapePtr s;      /* shapes[i] */
            s = &_kb->geom->shapes[i];
            for (j = 0; j < s->num_outlines; j++)
                if (s->outlines[j].num_points == 1) {
                    s->bounds.x1 = s->bounds.y1 = 0;
                }
        }
    }

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
    char newname[500];          /* big enough for a long font name */
    int res_x, res_y;           /* resolution values for this screen */
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
            case 7:            /* pixel size */
            case 12:           /* average width */
                /* change from "-0-" to "-*-" */
                newname[j] = '*';
                j++;
                if (name[i + 1] != '\0')
                    i++;
                break;
            case 8:            /* point size */
                /* change from "-0-" to "-<size>-" */
                sprintf(&newname[j], "%d", size);
                while (newname[j] != '\0')
                    j++;
                if (name[i + 1] != '\0')
                    i++;
                break;
            case 9:            /* x-resolution */
            case 10:           /* y-resolution */
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

void
RotatePoint(int left, int top, int angle, unsigned int rot_left,
            unsigned int rot_top, int *new_left, int *new_top)
{
    double r;
    double a;

    const double PI = 3.14159265358979323846;

    unsigned int l = left - rot_left;
    unsigned int t = top - rot_top;

    r = sqrt(l * l + t * t);
    //printf("+ + + r=%f, l=%d, t=%d\n", r, l, t);

    if (l == 0) {
        if (t > 0) {
            a = PI * 1 / 2;
        } else if (t == 0) {
            a = 0;
        } else {
            a = PI * 3 / 2;
        }
    } else {
        a = atan((float) t / l);
    }

    //printf ("+ + l=%d, t=%d, angle=%d, rl=%d, rt=%d, a=%.10f ", left, top, angle, rot_left, rot_top, a);

    if (new_left != NULL)
        *new_left =
            rot_left + r * cos((float) a + (float) angle / 1800 * PI);
    if (new_top != NULL)
        *new_top =
            rot_top + r * sin((float) a + (float) angle / 1800 * PI);
    //printf ("nl=%d, nt=%d\n", *new_left, *new_top);

}

void
RotateArc(int left, int top, unsigned int width, unsigned int height,
          int start, int end, int angle, unsigned int rot_left,
          unsigned int rot_top, int *new_x, int *new_y,
          unsigned int *new_width, unsigned int *new_height,
          int *new_start, int *new_end)
{
    
    int center_x, center_y;

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

int XSetFontNameToScalable(char *name, char *newname, int newname_n)
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
            case 7:            /* pixel size */
            case 12:           /* average width */
                /* change from "-whatever-" to "-0-" */
                newname[j] = '0';
                j++;
                while (name[i + 1] != '\0' && name[i + 1] != '-')
                    i++;
                break;
            case 8:            /* point size */
                /* change from "-whatever-" to "-0-" */
                newname[j] = '0';
                j++;
                while (name[i + 1] != '\0' && name[i + 1] != '-')
                    i++;
                break;
            case 9:            /* x-resolution */
            case 10:           /* y-resolution */
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

void
KbDrawBounds(Display * dpy, Drawable w, GC gc, unsigned int angle,
             double scale, unsigned int left, unsigned int top,
             XkbDescPtr _kb, XkbBoundsPtr bounds)
{
    GC gc_backup;

    /* FIXME: I know, there must be fast ways, but this was faster to code. */
    memcpy(&gc_backup, &gc, sizeof(GC));

    XSetLineAttributes(dpy, gc, 2, LineOnOffDash, CapRound, JoinRound);
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
//    KbDrawBounds(dpy, w, gc, angle, scale, left, top, _kb, &shape->bounds);

    XkbOutlinePtr source;
    int i;
    int t, l, b, r;
    int j;

    for (i = 0; i < (is_key ? 1 : shape->num_outlines); i++) {
        source = &shape->outlines[i];

        XSetLineAttributes(dpy, gc, 2, LineSolid, CapRound, JoinRound);

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
            /* FIXME: Should take care of angle and corner radius */
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
            int ax, ay, bx, by;
            int arch, arcw, arcs, arce;

            //printf("%d, %d, %d, %d, %d, %d, %d\n", source->num_points, rot_left, rot_top, l, t, r, b);

            RotatePoint(left + l + source->corner_radius, top + t, angle,
                        rot_left, rot_top, &ax, &ay);
            RotatePoint(left + r - source->corner_radius, top + t, angle,
                        rot_left, rot_top, &bx, &by);
            XDrawLine(dpy, w, gc, scale * (ax), scale * (ay), scale * (bx),
                      scale * (by));

            //printf("+ %d, %d, %d, %d\n", ax, ay, bx, by);
            RotateArc(left + l, top + t, 2 * source->corner_radius,
                      2 * source->corner_radius, 5760, 5760, angle,
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

            //printf("+ %d, %d, %d, %d\n", ax, ay, bx, by);
            RotateArc(left + r - 2 * source->corner_radius, top + t,
                      2 * source->corner_radius, 2 * source->corner_radius,
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

            //printf("+ %d, %d, %d, %d\n", ax, ay, bx, by);
            RotateArc(left + r - 2 * source->corner_radius,
                      top + b - 2 * source->corner_radius,
                      2 * source->corner_radius, 2 * source->corner_radius,
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

            //printf("+ %d, %d, %d, %d\n", ax, ay, bx, by);
            RotateArc(left + l, top + b - 2 * source->corner_radius,
                      2 * source->corner_radius, 2 * source->corner_radius,
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
}

void
KbDrawKey(Display * dpy, Drawable w, GC gc, unsigned int angle,
          unsigned int section_left, unsigned int section_top,
          double scale, unsigned int left, unsigned int top,
          XkbDescPtr _kb, XkbKeyPtr key)
{
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
        char *kss;

        if (!strncmp(key->name.name, _kb->names->keys[i].name, 4)) {
            strncpy(name, _kb->names->keys[i].name, 4);
            KeySym ks;
            ks = XKeycodeToKeysym(dpy, i, 0);
            kss = XKeysymToString(ks);
            if (kss) {
                /* FIXME: Better compare integer, not strings. Fixable. */
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
                int size;


    unsigned int ax, ay;

    IQF(XStringToKeysym(glyph), 0, buf, buf_n);
    if (strcmp(buf, "") != 0) {
        k = &_kb->geom->shapes[key->shape_ndx].bounds;

        RotatePoint((left + key->gap+8), (top+8), 
                    angle, section_left, section_top, &ax,
                    &ay);
        LoadAndDrawImage(dpy, gc, w, scale*ax, scale*ay, scale*(k->x2-k->x1)-8, scale*(k->y2-k->y1)-8, buf, buf_n);

        /* FIXME: These +- 8 are fixed now, which means they are resolution
           dependant. This is wrong. */
        /* FIXME: Key label vertical position is miscalculated. */
                    fs = XLoadQueryScalableFont(dpy, 0,
                                                fontname,
                                                400*scale);
        
                XSetFont(dpy, gc, fs->fid);

                    RotatePoint(left + key->gap +
                                (fs->max_bounds.width - (XTextWidth(fs, glyph, strlen(glyph))))/2/scale,
                                (top + (fs->max_bounds.ascent) / scale),
                                angle, section_left, section_top, &ax,
                                &ay);
                    XDrawString(dpy, w, gc, scale*ax, scale*ay, glyph,
                                strlen(glyph));

    } else {
                XSetForeground(dpy, gc, (160 << 16) + (160 << 8) + (176));
                if (strlen(kss) == 1) {
                    //glyph[0] = toupper(glyph[0]);
                    size = scale * 600;
                   fs = XLoadQueryScalableFont(dpy, 0,
                                                fontname,
                                                g_size);
                    tw = XTextWidth(fs, glyph, strlen(glyph));
                } else {
                    size = scale * 300;
                    do {
                        fs = XLoadQueryScalableFont(dpy, 0,
                                                    fontname,
                                                    size);
                        if (!fs) {
                            printf("Could not load font: %s",
                                   fontname);
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
                                (top +
                                 (b->y1 + (b->y2 - b->y1)) * g_baseline),
                                angle, section_left, section_top, &ax,
                                &ay);
                    XDrawString(dpy, w, gc, scale*ax, scale*ay, glyph,
                                1);
                } else {
                    RotatePoint(left + key->gap + 4/scale,
                                top + (b->y1 +
                                       b->y2 * fs->ascent / (fs->descent +
                                                             fs->ascent)),
                                angle, section_left, section_top, &ax,
                                &ay);
                    XDrawString(dpy, w, gc, scale * (ax), scale * (ay),
                                kss, strlen(kss));
                }

                XFreeFontInfo(NULL, fs, 1);
                XSetForeground(dpy, gc, 0);
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

//    KbDrawBounds(dpy, w, gc, angle, scale, left, top, _kb, &row->bounds);

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

//  KbDrawBounds(dpy, w, gc, angle, scale, left, top, _kb, &section->bounds);

    /* if (section->name) printf("Drawing section: %s\n", XGetAtomName(dpy,
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

void
KbDrawKeyboard(Display * dpy, Drawable w, GC gc, unsigned int angle,
               double scale, unsigned int left, unsigned int top,
               XkbDescPtr _kb, IconQueryFunc iqf)
{

    IQF = iqf;

    int i, j;

    WorkaroundBoundsBug(dpy, _kb);

    fontname = "-*-bitstream vera sans-bold-r-*-*-0-0-*-*-*-0-iso10646-*";

    /* Determine font size */
    int norm_h = 0, norm_w = 0;

    /* 1. Get norm with and height. */
    for (i = 0; i < _kb->geom->num_shapes; i++) {
        XkbShapePtr s;          /* shapes[i] */
        s = &_kb->geom->shapes[i];
        if (strncmp(XGetAtomName(dpy, s->name), "NORM", 4))
            continue;

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

    if (norm_h == norm_w == 0) {
        printf("superkb: Couldn't find norm.\n");
        return;
    }

    /* 2. Determine max point size that fits in norm_w and norm_h. */
    int max_w, max_h;

    XFontStruct *fs;
    fs = XLoadQueryScalableFont(dpy, 0, fontname, 1000);
    if (!fs) {
        fontname = "-*-helvetica-bold-r-*-*-0-0-*-*-*-0-iso10646-*";
        fs = XLoadQueryScalableFont(dpy, 0, fontname, 1000);
    }

    if (!fs) {
        printf("You need the Bitstream Vera Sans or Helvetica font. Sorry.\n");
        exit(EXIT_FAILURE);
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

    /* Draw top-level rectangle */
    XDrawRectangle(dpy, w, gc, left, top, scale * _kb->geom->width_mm,
                   scale * _kb->geom->height_mm);

    /* Draw each component (section or doodad) of the top-level _kb->geometry, in
     * priority order. Note that priority handling is left to the function. */
    KbDrawComponents(dpy, w, gc, angle, scale, left, top, _kb,
                     _kb->geom->sections, _kb->geom->num_sections,
                     _kb->geom->doodads, _kb->geom->num_doodads);

    XFlush(dpy);

}
