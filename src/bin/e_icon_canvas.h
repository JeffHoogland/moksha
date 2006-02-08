/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS
#else
#ifndef E_ICON_CANVAS_H
#define E_ICON_CANVAS_H

EAPI Evas_Object *e_icon_canvas_add                (Evas *evas);
EAPI int          e_icon_canvas_freeze             (Evas_Object *obj);
EAPI int          e_icon_canvas_thaw               (Evas_Object *obj);
EAPI void         e_icon_canvas_virtual_size_set   (Evas_Object *obj, Evas_Coord w, Evas_Coord h);
EAPI void         e_icon_canvas_virtual_size_get   (Evas_Object *obj, Evas_Coord *w, Evas_Coord *h);
EAPI void         e_icon_canvas_width_fix          (Evas_Object *obj, Evas_Coord w);
EAPI void         e_icon_canvas_height_fix         (Evas_Object *obj, Evas_Coord h);
EAPI void         e_icon_canvas_pack               (Evas_Object *obj, Evas_Object *child, Evas_Object *(*create)(void *data), void (*destroy)(Evas_Object *obj, void *data), void *data);
EAPI void         e_icon_canvas_pack_at_location   (Evas_Object *obj, Evas_Object *child, Evas_Object *(*create)(void *data), void (*destroy)(Evas_Object *obj, void *data), void *data, Evas_Coord x, Evas_Coord y);
EAPI void         e_icon_canvas_child_resize       (Evas_Object *obj, Evas_Coord w, Evas_Coord h);
EAPI void         e_icon_canvas_child_move         (Evas_Object *obj, Evas_Coord x, Evas_Coord y);
EAPI void         e_icon_canvas_unpack             (Evas_Object *obj);
EAPI void         e_icon_canvas_spacing_set        (Evas_Object *obj, Evas_Coord xs, Evas_Coord ys);
EAPI void         e_icon_canvas_redraw_force       (Evas_Object *obj);
EAPI void         e_icon_canvas_clip_freeze        (Evas_Object *obj);
EAPI void         e_icon_canvas_clip_thaw          (Evas_Object *obj);
EAPI void         e_icon_canvas_viewport_set       (Evas_Object *obj, Evas_Object *viewport);
EAPI void         e_icon_canvas_xy_freeze          (Evas_Object *obj);
EAPI void         e_icon_canvas_xy_thaw            (Evas_Object *obj);
EAPI void         e_icon_canvas_reset              (Evas_Object *obj);

#endif
#endif
