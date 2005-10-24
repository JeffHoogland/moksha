/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS
#else
#ifndef E_WIDGET_IMAGE_H
#define E_WIDGET_IMAGE_H

EAPI Evas_Object     *e_widget_image_add_from_object(Evas *evas, Evas_Object *object, int minw, int minh);
EAPI Evas_Object     *e_widget_image_add_from_file(Evas *evas, char *file, int aspect, int minh);

#endif
#endif
