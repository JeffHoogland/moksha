#ifndef E_BOX_H
#define E_BOX_H

Evas_Object *e_box_add               (Evas *evas);

int          e_box_freeze            (Evas_Object *obj);
int          e_box_thaw              (Evas_Object *obj);
void         e_box_orientation_set   (Evas_Object *obj, int horizontal);
void         e_box_homogenous_set    (Evas_Object *obj, int homogenous);
int          e_box_pack_start        (Evas_Object *obj, Evas_Object *child);
int          e_box_pack_end          (Evas_Object *obj, Evas_Object *child);
int          e_box_pack_before       (Evas_Object *obj, Evas_Object *child, Evas_Object *before);
int          e_box_pack_after        (Evas_Object *obj, Evas_Object *child, Evas_Object *after);
int          e_box_pack_count_get    (Evas_Object *obj);
Evas_Object *e_box_pack_object_nth   (Evas_Object *obj, int n);
Evas_Object *e_box_pack_object_first (Evas_Object *obj);
Evas_Object *e_box_pack_object_last  (Evas_Object *obj);
void         e_box_pack_options_set  (Evas_Object *obj, int fill_w, int fill_h, int expand_w, int expand_h, double align_x, double align_y, Evas_Coord min_w, Evas_Coord min_h, Evas_Coord max_w, Evas_Coord max_h);
void         e_box_unpack            (Evas_Object *obj);
void         e_box_min_size_get      (Evas_Object *obj, Evas_Coord *minw, Evas_Coord *minh);
void         e_box_max_size_get      (Evas_Object *obj, Evas_Coord *maxw, Evas_Coord *maxh);
void         e_box_align_get         (Evas_Object *obj, double *ax, double *ay);
void         e_box_align_set         (Evas_Object *obj, double ax, double ay);

#endif
