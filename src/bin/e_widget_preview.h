/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS
#else
#ifndef E_WIDGET_PREVIEW_H
#define E_WIDGET_PREVIEW_H

EAPI Evas_Object     *e_widget_preview_add(Evas *evas, int minw, int minh);
EAPI Evas            *e_widget_preview_evas_get(Evas_Object *obj);
EAPI void             e_widget_preview_extern_object_set(Evas_Object *obj, Evas_Object *eobj);
EAPI int              e_widget_preview_file_set(Evas_Object *obj, const char *file, const char *key);
EAPI int	      e_widget_preview_thumb_set(Evas_Object *obj, const char *file, const char *key, int w, int h);
EAPI int              e_widget_preview_edje_set(Evas_Object *obj, const char *file, const char *group);
    
#endif
#endif
