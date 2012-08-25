#ifdef E_TYPEDEFS
#else
#ifndef E_WIDGET_FILEPREVIEW_H
#define E_WIDGET_FILEPREVIEW_H

EAPI Evas_Object *e_widget_filepreview_add(Evas *evas, int w, int h, int horiz);
EAPI void e_widget_filepreview_path_set(Evas_Object *obj, const char *path, const char *mime);
EAPI void e_widget_filepreview_filemode_force(Evas_Object *obj);

#endif
#endif
