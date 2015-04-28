#ifdef E_TYPEDEFS
#else
#ifndef E_PAN_H
#define E_PAN_H

EAPI Evas_Object *e_pan_add            (Evas *evas);
EAPI void         e_pan_child_set      (Evas_Object *obj, Evas_Object *child);
EAPI Evas_Object *e_pan_child_get      (Evas_Object *obj);
EAPI void         e_pan_set            (Evas_Object *obj, Evas_Coord x, Evas_Coord y);
EAPI void         e_pan_get            (Evas_Object *obj, Evas_Coord *x, Evas_Coord *y);
EAPI void         e_pan_max_get        (Evas_Object *obj, Evas_Coord *x, Evas_Coord *y);
EAPI void         e_pan_child_size_get (Evas_Object *obj, Evas_Coord *w, Evas_Coord *h);

#endif
#endif
