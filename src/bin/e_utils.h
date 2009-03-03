/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS
#else
#ifndef E_UTILS_H
#define E_UTILS_H

#define e_util_dialog_show(title, args...) \
{ \
   char __tmpbuf[4096]; \
   \
   snprintf(__tmpbuf, sizeof(__tmpbuf), ##args); \
   e_util_dialog_internal(title, __tmpbuf); \
}


EAPI void         e_util_wakeup(void);
EAPI void         e_util_env_set(const char *var, const char *val);
EAPI E_Zone      *e_util_zone_current_get(E_Manager *man);
EAPI int          e_util_glob_match(const char *str, const char *glob);
EAPI int          e_util_glob_case_match(const char *str, const char *glob);
EAPI E_Container *e_util_container_number_get(int num);
EAPI E_Zone      *e_util_container_zone_id_get(int con_num, int id);
EAPI E_Zone      *e_util_container_zone_number_get(int con_num, int zone_num);
EAPI int          e_util_head_exec(int head, const char *cmd);
EAPI int          e_util_strcmp(const char *s1, const char *s2);    
EAPI int          e_util_both_str_empty(const char *s1, const char *s2);
EAPI int          e_util_immortal_check(void);
EAPI int          e_util_edje_icon_list_check(const char *list);
EAPI int          e_util_edje_icon_list_set(Evas_Object *obj, const char *list);
EAPI int          e_util_menu_item_edje_icon_list_set(E_Menu_Item *mi, const char *list);
EAPI int          e_util_edje_icon_check(const char *name);
EAPI int          e_util_edje_icon_set(Evas_Object *obj, const char *name);
EAPI int          e_util_menu_item_edje_icon_set(E_Menu_Item *mi, const char *name);
EAPI int          e_util_menu_item_fdo_icon_set(E_Menu_Item *mi, const char *icon);
EAPI E_Container *e_util_container_window_find(Ecore_X_Window win);
EAPI E_Border    *e_util_desk_border_above(E_Border *bd);
EAPI E_Border    *e_util_desk_border_below(E_Border *bd);
EAPI int          e_util_edje_collection_exists(const char *file, const char *coll);
EAPI void         e_util_dialog_internal(const char *title, const char *txt);
EAPI const char  *e_util_filename_escape(const char *filename);
EAPI int          e_util_icon_save(Ecore_X_Icon *icon, const char *filename);
EAPI char        *e_util_shell_env_path_eval(char *path);
EAPI char        *e_util_size_string_get(off_t size);
EAPI char        *e_util_file_time_get(time_t ftime);
EAPI void         e_util_library_path_strip(void);
EAPI void         e_util_library_path_restore(void);
EAPI Evas_Object *e_util_icon_add(const char *path, Evas *evas);
EAPI Evas_Object *e_util_desktop_icon_add(Efreet_Desktop *desktop, unsigned int size, Evas *evas);
EAPI Evas_Object *e_util_icon_theme_icon_add(const char *icon_name, unsigned int size, Evas *evas);
EAPI void         e_util_desktop_menu_item_icon_add(Efreet_Desktop *desktop, unsigned int size, E_Menu_Item *mi);
EAPI int          e_util_dir_check(const char *dir);
EAPI void         e_util_defer_object_del(E_Object *obj);
EAPI const char  *e_util_winid_str_get(Ecore_X_Window win);
EAPI void         e_util_win_auto_resize_fill(E_Win *win);
    
#endif
#endif
