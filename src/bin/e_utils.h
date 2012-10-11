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
EAPI int          e_util_strcasecmp(const char *s1, const char *s2);
EAPI int          e_util_strcmp(const char *s1, const char *s2);
EAPI int          e_util_both_str_empty(const char *s1, const char *s2);
EAPI int          e_util_immortal_check(void);
EAPI int          e_util_edje_icon_list_check(const char *list);
EAPI int          e_util_edje_icon_list_set(Evas_Object *obj, const char *list);
EAPI int          e_util_menu_item_edje_icon_list_set(E_Menu_Item *mi, const char *list);
EAPI int          e_util_edje_icon_check(const char *name);
EAPI int          e_util_edje_icon_set(Evas_Object *obj, const char *name);
EAPI int          e_util_icon_theme_set(Evas_Object *obj, const char *icon);
EAPI unsigned int e_util_icon_size_normalize(unsigned int desired);
EAPI int          e_util_menu_item_theme_icon_set(E_Menu_Item *mi, const char *icon);
EAPI const char *e_util_mime_icon_get(const char *mime, unsigned int size);
EAPI E_Container *e_util_container_window_find(Ecore_X_Window win);
EAPI E_Zone      *e_util_zone_window_find(Ecore_X_Window win);
EAPI E_Border    *e_util_desk_border_above(E_Border *bd);
EAPI E_Border    *e_util_desk_border_below(E_Border *bd);
EAPI int          e_util_edje_collection_exists(const char *file, const char *coll);
EAPI E_Dialog     *e_util_dialog_internal(const char *title, const char *txt);
EAPI const char  *e_util_filename_escape(const char *filename);
EAPI int          e_util_icon_save(Ecore_X_Icon *icon, const char *filename);
EAPI char        *e_util_shell_env_path_eval(const char *path);
EAPI char        *e_util_size_string_get(off_t size);
EAPI char        *e_util_file_time_get(time_t ftime);
EAPI Evas_Object *e_util_icon_add(const char *path, Evas *evas);
EAPI Evas_Object *e_util_desktop_icon_add(Efreet_Desktop *desktop, unsigned int size, Evas *evas);
EAPI Evas_Object *e_util_icon_theme_icon_add(const char *icon_name, unsigned int size, Evas *evas);
EAPI void         e_util_desktop_menu_item_icon_add(Efreet_Desktop *desktop, unsigned int size, E_Menu_Item *mi);
EAPI int          e_util_dir_check(const char *dir);
EAPI void         e_util_defer_object_del(E_Object *obj);
EAPI const char  *e_util_winid_str_get(Ecore_X_Window win);
EAPI void         e_util_win_auto_resize_fill(E_Win *win);
/* check if loaded config version matches the current version, show a
   dialog warning if loaded version is older or newer than current */
EAPI Eina_Bool    e_util_module_config_check(const char *module_name, int loaded, int current);

EAPI int e_util_container_desk_count_get(E_Container *con);
EAPI E_Config_Binding_Key *e_util_binding_match(const Eina_List *bindlist, Ecore_Event_Key *ev, unsigned int *num, const E_Config_Binding_Key *skip);
EAPI Eina_Bool e_util_fullscreen_current_any(void);
EAPI Eina_Bool e_util_fullscreen_any(void);
EAPI const char *e_util_time_str_get(long int seconds);
EAPI void e_util_size_debug_set(Evas_Object *obj, Eina_Bool enable);
EAPI Efreet_Desktop *e_util_terminal_desktop_get(void);
EAPI void e_util_gadcon_orient_icon_set(E_Gadcon_Orient orient, Evas_Object *obj);
EAPI void e_util_gadcon_orient_menu_item_icon_set(E_Gadcon_Orient orient, E_Menu_Item *mi);

EAPI char *e_util_string_append_char(char *str, size_t *size, size_t *len, char c);
EAPI char *e_util_string_append_quoted(char *str, size_t *size, size_t *len, const char *src);

static inline E_Container *
e_util_container_current_get(void)
{
   return e_container_current_get(e_manager_current_get());
}

#endif
#endif
