/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS
#else
#ifndef E_WIDGET_FRAMETABLE_H
#define E_WIDGET_FRAMETABLE_H

EAPI Evas_Object *e_widget_frametable_add(Evas *evas, char *label, int homogenous);
EAPI void e_widget_frametable_object_append(Evas_Object *obj, Evas_Object *sobj, int col, int row, int colspan, int rowspan, int fill_w, int fill_h, int expand_w, int expand_h);
    

#endif
#endif
