#ifndef E_ICON_H
#define E_ICON_H

Evas_Object *e_icon_add              (Evas *evas);
void         e_icon_file_set         (Evas_Object *obj, const char *file);
const char  *e_icon_file_get         (Evas_Object *obj);
void         e_icon_smooth_scale_set (Evas_Object *obj, int smooth);
int          e_icon_smooth_scale_get (Evas_Object *obj);
void         e_icon_size_get         (Evas_Object *obj, int *w, int *h);
int          e_icon_fill_inside_get  (Evas_Object *obj);
void         e_icon_fill_inside_set  (Evas_Object *obj, int fill_inside);

#endif
