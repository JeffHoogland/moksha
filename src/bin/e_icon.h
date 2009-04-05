/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS
#else
#ifndef E_ICON_H
#define E_ICON_H

EAPI Evas_Object *e_icon_add              (Evas *evas);
EAPI void         e_icon_file_set         (Evas_Object *obj, const char *file);
EAPI void         e_icon_file_key_set     (Evas_Object *obj, const char *file, const char *key);
EAPI void         e_icon_file_edje_set    (Evas_Object *obj, const char *file, const char *part);
EAPI void         e_icon_object_set       (Evas_Object *obj, Evas_Object *o);    
EAPI const char  *e_icon_file_get         (Evas_Object *obj);
EAPI void         e_icon_smooth_scale_set (Evas_Object *obj, int smooth);
EAPI int          e_icon_smooth_scale_get (Evas_Object *obj);
EAPI void         e_icon_alpha_set        (Evas_Object *obj, int smooth);
EAPI int          e_icon_alpha_get        (Evas_Object *obj);
EAPI void         e_icon_preload_set      (Evas_Object *obj, int preload);
EAPI int          e_icon_preload_get      (Evas_Object *obj);
EAPI void         e_icon_size_get         (Evas_Object *obj, int *w, int *h);
EAPI int          e_icon_fill_inside_get  (Evas_Object *obj);
EAPI void         e_icon_fill_inside_set  (Evas_Object *obj, int fill_inside);
EAPI int          e_icon_scale_up_get     (Evas_Object *obj);
EAPI void         e_icon_scale_up_set     (Evas_Object *obj, int scale_up);
EAPI void         e_icon_data_set         (Evas_Object *obj, void *data, int w, int h);
EAPI void        *e_icon_data_get         (Evas_Object *obj, int *w, int *h);
EAPI void         e_icon_scale_size_set   (Evas_Object *obj, int size);
EAPI int          e_icon_scale_size_get   (Evas_Object *obj);

#endif
#endif
