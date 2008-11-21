/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS
#else
#ifndef E_WIDGET_TOOLBAR_H
#define E_WIDGET_TOOLBAR_H

EAPI Evas_Object *e_widget_toolbar_add(Evas *evas, int icon_w, int icon_h);
EAPI void e_widget_toolbar_item_append(Evas_Object *obj, Evas_Object *icon, const char *label, void (*func) (void *data1, void *data2), const void *data1, const void *data2);
EAPI void e_widget_toolbar_item_select(Evas_Object *obj, int num);
EAPI void e_widget_toolbar_scrollable_set(Evas_Object *obj, Evas_Bool scrollable);

#endif
#endif
