/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS
#else
#ifndef E_FLOWLAYOUT_H
#define E_FLOWLAYOUT_H

EAPI Evas_Object *e_flowlayout_add               (Evas *evas);
EAPI int          e_flowlayout_freeze            (Evas_Object *obj);
EAPI int          e_flowlayout_thaw              (Evas_Object *obj);
EAPI void         e_flowlayout_orientation_set   (Evas_Object *obj, int horizontal);
EAPI int          e_flowlayout_orientation_get   (Evas_Object *obj);
EAPI void         e_flowlayout_flowdirection_set (Evas_Object *obj, int right, int bottom);
EAPI void         e_flowlayout_flowdirection_get (Evas_Object *obj, int *right, int *bottom);
EAPI void         e_flowlayout_homogenous_set    (Evas_Object *obj, int homogenous);
EAPI int          e_flowlayout_fill_get          (Evas_Object *obj);
EAPI void         e_flowlayout_fill_set          (Evas_Object *obj, int fill);
EAPI int          e_flowlayout_pack_start        (Evas_Object *obj, Evas_Object *child);
EAPI int          e_flowlayout_pack_end          (Evas_Object *obj, Evas_Object *child);
EAPI int          e_flowlayout_pack_before       (Evas_Object *obj, Evas_Object *child, Evas_Object *before);
EAPI int          e_flowlayout_pack_after        (Evas_Object *obj, Evas_Object *child, Evas_Object *after);
EAPI int          e_flowlayout_pack_count_get    (Evas_Object *obj);
EAPI Evas_Object *e_flowlayout_pack_object_nth   (Evas_Object *obj, int n);
EAPI Evas_Object *e_flowlayout_pack_object_first (Evas_Object *obj);
EAPI Evas_Object *e_flowlayout_pack_object_last  (Evas_Object *obj);
EAPI void         e_flowlayout_pack_options_set  (Evas_Object *obj, int fill_w, int fill_h, int expand_w, int expand_h, double align_x, double align_y, Evas_Coord min_w, Evas_Coord min_h, Evas_Coord max_w, Evas_Coord max_h);
EAPI void         e_flowlayout_unpack            (Evas_Object *obj);
EAPI void         e_flowlayout_min_size_get      (Evas_Object *obj, Evas_Coord *minw, Evas_Coord *minh);
EAPI void         e_flowlayout_max_size_get      (Evas_Object *obj, Evas_Coord *maxw, Evas_Coord *maxh);
EAPI void         e_flowlayout_align_get         (Evas_Object *obj, double *ax, double *ay);
EAPI void         e_flowlayout_align_set         (Evas_Object *obj, double ax, double ay);
// This function only works if homogenous is set
EAPI int          e_flowlayout_max_children      (Evas_Object *obj);

#endif
#endif
