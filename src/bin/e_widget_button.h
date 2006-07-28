/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS
#else
#ifndef E_WIDGET_BUTTON_H
#define E_WIDGET_BUTTON_H

EAPI Evas_Object *e_widget_button_add(Evas *evas, const char *label, const char *icon, void (*func) (void *data, void *data2), void *data, void *data2);
EAPI void e_widget_button_label_set(Evas_Object *obj, const char *label);
EAPI void e_widget_button_icon_set(Evas_Object *obj, Evas_Object *icon);

#endif
#endif
