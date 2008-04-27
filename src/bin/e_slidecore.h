/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS
#else
#ifndef E_SLIDECORE_H
#define E_SLIDECORE_H

EAPI Evas_Object *e_slidecore_add              (Evas *evas);
EAPI void         e_slidecore_item_distance_set(Evas_Object *obj, Evas_Coord dist);
EAPI void         e_slidecore_item_add         (Evas_Object *obj, const char *label, const char *icon, void (*func) (void *data), void *data);
EAPI void         e_slidecore_jump             (Evas_Object *obj, int num);
EAPI void         e_slidecore_slide_time_set   (Evas_Object *obj, double t);

#endif
#endif
