/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS
#else
#ifndef E_WIDGET_TOOLBOOK_H
#define E_WIDGET_TOOLBOOK_H

EAPI Evas_Object *e_widget_toolbook_add(Evas *evas, int icon_w, int icon_h);
EAPI void e_widget_toolbook_page_append(Evas_Object *toolbook, Evas_Object *icon, const char *label, Evas_Object *content, int expand_w, int expand_h, int fill_w, int fill_h, double ax, double ay);
EAPI void e_widget_toolbook_page_show(Evas_Object *toolbook, int n);
    
#endif
#endif
