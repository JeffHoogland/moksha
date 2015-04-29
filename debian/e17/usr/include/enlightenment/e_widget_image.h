#ifdef E_TYPEDEFS
#else
#ifndef E_WIDGET_IMAGE_H
#define E_WIDGET_IMAGE_H

EAPI Evas_Object     *e_widget_image_add_from_object(Evas *evas, Evas_Object *object, int minw, int minh);
EAPI Evas_Object     *e_widget_image_add_from_file(Evas *evas, const char *file, int minw, int minh);
EAPI void             e_widget_image_edje_set(Evas_Object *obj, const char *file, const char *part);
EAPI void             e_widget_image_file_set(Evas_Object *obj, const char *file);
EAPI void             e_widget_image_file_key_set(Evas_Object *obj, const char *file, const char *key);

#endif
#endif
