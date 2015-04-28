#ifdef E_TYPEDEFS
#else
#ifndef E_ICON_H
#define E_ICON_H

EINTERN int e_icon_init(void);
EINTERN int e_icon_shutdown(void);

EAPI Evas_Object *e_icon_add              (Evas *evas);
EAPI Eina_Bool    e_icon_file_set         (Evas_Object *obj, const char *file);
EAPI Eina_Bool    e_icon_file_key_set     (Evas_Object *obj, const char *file, const char *key);
EAPI Eina_Bool    e_icon_file_edje_set    (Evas_Object *obj, const char *file, const char *part);
EAPI Eina_Bool    e_icon_fdo_icon_set     (Evas_Object *obj, const char *icon);
EAPI void         e_icon_edje_object_set(Evas_Object *obj, Evas_Object *edje);
EAPI void         e_icon_object_set       (Evas_Object *obj, Evas_Object *o) EINA_DEPRECATED;
EAPI Eina_Bool   e_icon_file_get(const Evas_Object *obj, const char **file, const char **group);
EAPI void         e_icon_smooth_scale_set (Evas_Object *obj, Eina_Bool smooth);
EAPI Eina_Bool    e_icon_smooth_scale_get (const Evas_Object *obj);
EAPI void         e_icon_alpha_set        (Evas_Object *obj, Eina_Bool smooth);
EAPI Eina_Bool    e_icon_alpha_get        (const Evas_Object *obj);
EAPI void         e_icon_preload_set      (Evas_Object *obj, Eina_Bool preload);
EAPI Eina_Bool    e_icon_preload_get      (const Evas_Object *obj);
EAPI void         e_icon_size_get         (const Evas_Object *obj, int *w, int *h);
EAPI Eina_Bool    e_icon_fill_inside_get  (const Evas_Object *obj);
EAPI void         e_icon_fill_inside_set  (Evas_Object *obj, Eina_Bool fill_inside);
EAPI Eina_Bool    e_icon_scale_up_get     (const Evas_Object *obj);
EAPI void         e_icon_scale_up_set     (Evas_Object *obj, Eina_Bool scale_up);
EAPI void         e_icon_data_set         (Evas_Object *obj, void *data, int w, int h);
EAPI void        *e_icon_data_get         (const Evas_Object *obj, int *w, int *h);
EAPI void         e_icon_scale_size_set   (Evas_Object *obj, int size);
EAPI int          e_icon_scale_size_get   (const Evas_Object *obj);
EAPI void         e_icon_selected_set	  (const Evas_Object *obj, Eina_Bool selected);

#endif
#endif
