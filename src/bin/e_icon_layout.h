/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS
#else
#ifndef E_ICON_LAYOUT_H
#define E_ICON_LAYOUT_H

EAPI Evas_Object *e_icon_layout_add               (Evas *evas);
EAPI int          e_icon_layout_freeze            (Evas_Object *obj);
EAPI int          e_icon_layout_thaw              (Evas_Object *obj);
EAPI void         e_icon_layout_virtual_size_set  (Evas_Object *obj, Evas_Coord w, Evas_Coord h);
EAPI void         e_icon_layout_virtual_size_get  (Evas_Object *obj, Evas_Coord *w, Evas_Coord *h);
EAPI void         e_icon_layout_pack              (Evas_Object *obj, Evas_Object *child);
EAPI void         e_icon_layout_child_resize      (Evas_Object *obj, Evas_Coord w, Evas_Coord h);
EAPI void         e_icon_layout_child_stack_above (Evas_Object *obj, Evas_Object *above);
EAPI void         e_icon_layout_child_stack_below (Evas_Object *obj, Evas_Object *below);
EAPI void         e_icon_layout_unpack            (Evas_Object *obj);
EAPI void         e_icon_layout_spacing_set(Evas_Object *obj, Evas_Coord xs, Evas_Coord ys);
EAPI void         e_icon_layout_redraw_force      (Evas_Object *obj);

#endif
#endif
