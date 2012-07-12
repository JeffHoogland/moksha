#ifdef E_TYPEDEFS
#else
#ifndef E_WIDGET_TABLE_H
#define E_WIDGET_TABLE_H

EAPI Evas_Object *e_widget_table_add(Evas *evas, int homogenous);
EAPI void e_widget_table_object_append(Evas_Object *obj, Evas_Object *sobj, int col, int row, int colspan, int rowspan, int fill_w, int fill_h, int expand_w, int expand_h);
EAPI void e_widget_table_object_align_append(Evas_Object *obj, Evas_Object *sobj, int col, int row, int colspan, int rowspan, int fill_w, int fill_h, int expand_w, int expand_h, double ax, double ay);
EAPI void e_widget_table_object_repack(Evas_Object *obj, Evas_Object *sobj, int col, int row, int colspan, int rowspan, int fill_w, int fill_h, int expand_w, int expand_h);
EAPI void e_widget_table_unpack(Evas_Object *obj, Evas_Object *sobj);
EAPI void e_widget_table_freeze(Evas_Object *obj);
EAPI void e_widget_table_thaw(Evas_Object *obj);

#endif
#endif
