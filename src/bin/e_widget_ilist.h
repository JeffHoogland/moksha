/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS
#else
#ifndef E_WIDGET_ILIST_H
#define E_WIDGET_ILIST_H

EAPI Evas_Object *e_widget_ilist_add(Evas *evas, int icon_w, int icon_h, char **value);
EAPI void e_widget_ilist_append(Evas_Object *obj, Evas_Object *icon, char *label, void (*func) (void *data), void *data, char *val);
EAPI void e_widget_ilist_select_set(Evas_Object *obj, int n);
EAPI void e_widget_ilist_selector_set(Evas_Object *obj, int selector);
EAPI void e_widget_ilist_go(Evas_Object *obj);
    
#endif
#endif
