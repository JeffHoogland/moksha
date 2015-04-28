#ifndef E_WIDGET_ASPECT_H
#define E_WIDGET_ASPECT_H

EAPI Evas_Object *e_widget_aspect_add(Evas *evas, int w, int h);
EAPI void e_widget_aspect_aspect_set(Evas_Object *obj, int w, int h);
EAPI void e_widget_aspect_child_set(Evas_Object *obj, Evas_Object *child);
EAPI void e_widget_aspect_align_set(Evas_Object *obj, double align_x, double align_y);

#endif
