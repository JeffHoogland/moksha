#ifndef E_WIDGET_DESK_PREVIEW_H
#define E_WIDGET_DESK_PREVIEW_H

EAPI Evas_Object *e_widget_deskpreview_add(Evas *evas, int nx, int ny);
EAPI void e_widget_deskpreview_num_desks_set(Evas_Object *obj, int nx, int ny);
EAPI Evas_Object *e_widget_deskpreview_desk_add(Evas_Object *obj, E_Zone *zone, int x, int y, int w, int h);
EAPI void e_widget_deskpreview_desk_configurable_set(Evas_Object *obj, Eina_Bool enable);

#endif
