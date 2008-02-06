/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS
#else
#ifndef E_WIDGET_TLIST_H
#define E_WIDGET_TLIST_H

EAPI Evas_Object   *e_widget_tlist_add(Evas * evas, char **value);
EAPI void           e_widget_tlist_append(Evas_Object * obj, const char *label,
					  void (*func) (void *data), void *data,
					  const char *val);
EAPI void           e_widget_tlist_markup_append(Evas_Object * obj, const char *label,
						 void (*func) (void *data),
						 void *data, const char *val);
EAPI void           e_widget_tlist_selected_set(Evas_Object * obj, int n);
EAPI void           e_widget_tlist_selector_set(Evas_Object * obj,
						int selector);
EAPI void           e_widget_tlist_go(Evas_Object * obj);
EAPI int            e_widget_tlist_selected_get(Evas_Object * obj);
EAPI const char    *e_widget_tlist_selected_label_get(Evas_Object * obj);
EAPI void           e_widget_tlist_remove_num(Evas_Object * obj, int n);
EAPI void           e_widget_tlist_remove_label(Evas_Object * obj, const char *label);
EAPI int            e_widget_tlist_count(Evas_Object * obj);
EAPI void           e_widget_tlist_clear(Evas_Object * obj);

#endif
#endif
