/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS
#else
#ifndef E_DESKPREVIEW_H
#define E_DESKPREVIEW_H

EAPI Evas_Object *e_deskpreview_add                      (Evas *evas);
EAPI void         e_deskpreview_mini_size_set            (Evas_Object *obj, Evas_Coord w, Evas_Coord h);
EAPI void         e_deskpreview_region_set               (Evas_Object *obj, int x, int y, int w, int h, int zone, int container);

#endif
#endif
