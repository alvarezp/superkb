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

/* Graphic context should have already been set. */
void KbDrawShape(Display *dpy, Drawable w, GC gc, unsigned int angle,
    double scale, unsigned int left, unsigned int top, XkbDescPtr _kb,
    XkbShapePtr shape, XkbColorPtr color)
{
    /* FIXME: Should manage angled shapes. */
    /* FIXME: Set color too. */
    /* FIXME: Should paint actual shape. */

    XkbComputeShapeBounds(shape);
    XDrawRectangle(dpy, w, gc, scale*(left+shape->bounds.x1),
        scale*(top+shape->bounds.y1),
        scale*(shape->bounds.x2-shape->bounds.x1),
        scale*(shape->bounds.y2-shape->bounds.y1));

}

void KbDrawDoodad(Display *dpy, Drawable w, GC gc, unsigned int parent_angle,
    double scale, unsigned int left, unsigned int top,
    XkbDescPtr _kb, XkbDoodadPtr doodad)
{
    switch (doodad->any.type) {
    case XkbOutlineDoodad:
        KbDrawShape(dpy, w, gc, parent_angle+doodad->shape.angle, scale,
        left + doodad->shape.left, top+doodad->shape.top,
        _kb, &_kb->geom->shapes[doodad->shape.shape_ndx], &_kb->geom->colors[doodad->shape.color_ndx]);
        break;
    case XkbSolidDoodad:
        KbDrawShape(dpy, w, gc, parent_angle+doodad->shape.angle, scale,
        left + doodad->shape.left, top+doodad->shape.top,
        _kb, &_kb->geom->shapes[doodad->shape.shape_ndx], &_kb->geom->colors[doodad->shape.color_ndx]);
        break;
    case XkbTextDoodad:
        XDrawString(dpy, w, gc, left+doodad->text.left, top+doodad->text.top, doodad->text.text, strlen(doodad->text.text));
        break;
    case XkbIndicatorDoodad:
        KbDrawShape(dpy, w, gc, parent_angle+doodad->indicator.angle, scale,
        left + doodad->indicator.left, top+doodad->indicator.top,
        _kb, &_kb->geom->shapes[doodad->indicator.shape_ndx], &_kb->geom->colors[doodad->indicator.on_color_ndx]);
        break;
    case XkbLogoDoodad:
        KbDrawShape(dpy, w, gc, parent_angle+doodad->logo.angle, scale,
        left + doodad->logo.left, top+doodad->logo.top,
        _kb, &_kb->geom->shapes[doodad->logo.shape_ndx], &_kb->geom->colors[doodad->logo.color_ndx]);
        break;
    }
}

void KbDrawKey(Display *dpy, Drawable w, GC gc, unsigned int parent_angle,
    double scale, unsigned int left, unsigned int top,
    XkbDescPtr _kb, XkbKeyPtr key)
{
    int fixed_num_keys;
    unsigned long i;

    KbDrawShape(dpy, w, gc, parent_angle, scale, left+key->gap,
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
                strncpy(glyph, kss, 255);
                XDrawString(dpy, w, gc,
                    scale*(left+key->gap+_kb->geom->shapes[key->shape_ndx].bounds.x2/2),
                    scale*top+30, kss, strlen(kss));
            }
        }
    }

}

void KbDrawRow(Display *dpy, Drawable w, GC gc, unsigned int parent_angle,
    double scale, unsigned int left, unsigned int top,
    XkbDescPtr _kb, XkbRowPtr row)
{
    int i;
    unsigned int next_piece=0;

    /* XDrawRectangle(dpy, w, gc, scale*(left+row->bounds.x1),
        scale*(top+row->bounds.y1),
        scale*(row->bounds.x2-row->bounds.x1),
        scale*(row->bounds.y2-row->bounds.y1)); */

    for (i=0; i<row->num_keys; i++)
    {
        if (!row->vertical) {
            KbDrawKey(dpy, w, gc, parent_angle, scale, left+row->left+next_piece, top+row->top,
            _kb, &row->keys[i]);
            next_piece += _kb->geom->shapes[row->keys[i].shape_ndx].bounds.x2 + row->keys[i].gap;
        } else {
            KbDrawKey(dpy, w, gc, parent_angle, scale, left+row->left, top+row->top+next_piece,
            _kb, &row->keys[i]);
            next_piece += _kb->geom->shapes[row->keys[i].shape_ndx].bounds.y2 + row->keys[i].gap;
        }
    }

}

void KbDrawSection(Display *dpy, Drawable w, GC gc, unsigned int parent_angle,
    double scale, unsigned int left, unsigned int top,
    XkbDescPtr _kb, XkbSectionPtr section)
{
    int i, p;

    /* XDrawRectangle(dpy, w, gc, scale*(left+section->left),
        scale*(top+section->top),
        scale*(section->width),
        scale*(section->height)); */

    for (i=0; i<section->num_rows; i++)
    {
        XkbComputeRowBounds(_kb->geom, section, &section->rows[i]);
        KbDrawRow(dpy, w, gc, parent_angle, scale, left+section->left, top+section->top,
            _kb, &section->rows[i]);   
    }

    for (p=0; p<=255; p++)
    {
        for (i=0; i<section->num_doodads; i++)
        {
            if (section->doodads[i].any.priority == p) {
                KbDrawDoodad(dpy, w, gc, parent_angle, scale, left,
                top, _kb, &section->doodads[i]);
            }
        }
    }
}

void KbDrawComponents(Display *dpy, Drawable w, GC gc, unsigned int parent_angle,
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
                KbDrawSection(dpy, w, gc, parent_angle, scale, left,
                top, _kb, &sections[i]);

            }
        }
        
        for (i=0; i<doodads_n; i++)
        {
            if (doodads[i].any.priority == p) {
                KbDrawDoodad(dpy, w, gc, parent_angle, scale, left,
                top, _kb, &doodads[i]);
            }
        }
    }
}

void KbDrawKeyboard(Display *dpy, Drawable w, GC gc, unsigned int parent_angle,
    double scale, unsigned int left, unsigned int top,
    XkbDescPtr _kb)
{

    /* Draw top-level rectangle */
    XDrawRectangle(dpy, w, gc, left, top, scale*_kb->geom->width_mm,
        scale*_kb->geom->height_mm);

    /* Draw each component (section or doodad) of the top-level _kb->geometry, in
     * priority order. Note that priority handling is left to the function. */
    KbDrawComponents(dpy, w, gc, 0, scale, left, top, _kb, _kb->geom->sections,
        _kb->geom->num_sections, _kb->geom->doodads, _kb->geom->num_doodads);

}
