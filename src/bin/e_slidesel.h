/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS
#else
#ifndef E_SLIDESEL_H
#define E_SLIDESEL_H

EAPI Evas_Object *e_slidesel_add               (Evas *evas);
EAPI void         e_slidesel_item_distance_set (Evas_Object *obj, Evas_Coord dist);
EAPI void         e_slidesel_item_add          (Evas_Object *obj, const char *label, const char *icon,  void (*func) (void *data), void *data);
EAPI void         e_slidesel_jump              (Evas_Object *obj, int num);
    
#endif
#endif
