/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS
#else
#ifndef E_PAN_H
#define E_PAN_H

EAPI Evas_Object *e_pan_add(Evas *evas);
void e_pan_set(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
void e_pan_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y);
void e_pan_child_set(Evas_Object *obj, Evas_Object *child);
Evas_Object *e_pan_child_get(Evas_Object *obj);
void e_pan_set(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
void e_pan_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y);
void e_pan_max_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y);
void e_pan_child_size_get(Evas_Object *obj, Evas_Coord *w, Evas_Coord *h);
    
#endif
#endif
