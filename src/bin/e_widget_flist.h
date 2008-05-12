#ifdef E_TYPEDEFS
#else
#ifndef E_WIDGET_FLIST_H
#define E_WIDGET_FLIST_H

EAPI Evas_Object *e_widget_flist_add(Evas *evas);
EAPI void e_widget_flist_path_set(Evas_Object *obj, const char *dev, const char *path);
EAPI Evas_List *e_widget_flist_all_list_get(Evas_Object *obj);
EAPI Evas_List *e_widget_flist_selected_list_get(Evas_Object *obj);
EAPI const char *e_widget_flist_real_path_get(Evas_Object *obj);
EAPI int e_widget_flist_has_parent_get(Evas_Object *obj);
EAPI void e_widget_flist_select_set(Evas_Object *obj, const char *file, int select);
EAPI void e_widget_flist_file_show(Evas_Object *obj, const char *file);
EAPI void e_widget_flist_parent_go(Evas_Object *obj);
EAPI void e_widget_flist_refresh(Evas_Object *obj);

#endif
#endif
