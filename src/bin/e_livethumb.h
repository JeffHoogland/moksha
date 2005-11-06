/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS
#else
#ifndef E_LIVETHUMB_H
#define E_LIVETHUMB_H

EAPI Evas_Object *e_livethumb_add                   (Evas *e);
EAPI Evas        *e_livethumb_evas_get              (Evas_Object *obj);
EAPI void         e_livethumb_vsize_set             (Evas_Object *obj, Evas_Coord w, Evas_Coord h);
EAPI void         e_livethumb_vsize_get             (Evas_Object *obj, Evas_Coord *w, Evas_Coord *h);
EAPI void         e_livethumb_thumb_set             (Evas_Object *obj, Evas_Object *thumb);
EAPI Evas_Object *e_livethumb_thumb_get             (Evas_Object *obj);
    
#endif
#endif
