/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS
#else
#ifndef E_WIDGET_FRAMELIST_H
#define E_WIDGET_FRAMELIST_H

EAPI Evas_Object *e_widget_framelist_add(Evas *evas, const char *label, int horiz);
EAPI void e_widget_framelist_object_append(Evas_Object *obj, Evas_Object *sobj);
EAPI void e_widget_framelist_content_align_set(Evas_Object *obj, double halign, double valign);
EAPI void e_widget_framelist_label_set(Evas_Object *obj, const char *label);

#endif
#endif
