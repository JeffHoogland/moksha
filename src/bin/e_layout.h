/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS
#else
#ifndef E_LAYOUT_H
#define E_LAYOUT_H

EAPI Evas_Object *e_layout_add               (Evas *evas);
EAPI int          e_layout_freeze            (Evas_Object *obj);
EAPI int          e_layout_thaw              (Evas_Object *obj);
EAPI void         e_layout_virtual_size_set  (Evas_Object *obj, Evas_Coord w, Evas_Coord h);
EAPI void         e_layout_virtual_size_get  (Evas_Object *obj, Evas_Coord *w, Evas_Coord *h);

EAPI void e_layout_coord_canvas_to_virtual (Evas_Object *obj, Evas_Coord cx, Evas_Coord cy, Evas_Coord *vx, Evas_Coord *vy);
EAPI void e_layout_coord_virtual_to_canvas (Evas_Object *obj, Evas_Coord vx, Evas_Coord vy, Evas_Coord *cx, Evas_Coord *cy);

EAPI void         e_layout_pack              (Evas_Object *obj, Evas_Object *child);
EAPI void         e_layout_child_move        (Evas_Object *obj, Evas_Coord x, Evas_Coord y);
EAPI void         e_layout_child_resize      (Evas_Object *obj, Evas_Coord w, Evas_Coord h);
EAPI void         e_layout_child_raise       (Evas_Object *obj);
EAPI void         e_layout_child_lower       (Evas_Object *obj);
EAPI void         e_layout_child_raise_above (Evas_Object *obj, Evas_Object *above);
EAPI void         e_layout_child_lower_below (Evas_Object *obj, Evas_Object *below);
EAPI void         e_layout_child_geometry_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y, Evas_Coord *w, Evas_Coord *h);
EAPI void         e_layout_unpack            (Evas_Object *obj);

#endif
#endif
