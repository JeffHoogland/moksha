#ifdef E_TYPEDEFS

typedef struct _E_Util_Image_Import_Handle E_Util_Image_Import_Handle;

typedef enum _E_Image_Import_Mode E_Image_Import_Mode;
enum _E_Image_Import_Mode
{
  E_IMAGE_IMPORT_STRETCH = 0,
  E_IMAGE_IMPORT_TILE = 1,
  E_IMAGE_IMPORT_CENTER = 2,
  E_IMAGE_IMPORT_SCALE_ASPECT_IN = 3,
  E_IMAGE_IMPORT_SCALE_ASPECT_OUT = 4
};

#else
#ifndef E_UTILS_H
#define E_UTILS_H

#define E_UTIL_IMAGE_IMPORT_SETTINGS 0xE0b0104f

#define e_util_dialog_show(title, args...) \
{ \
   char __tmpbuf[PATH_MAX]; \
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
EAPI int          e_util_icon_theme_set(Evas_Object *obj, const char *icon);
EAPI unsigned int e_util_icon_size_normalize(unsigned int desired);
EAPI int          e_util_menu_item_theme_icon_set(E_Menu_Item *mi, const char *icon);
EAPI E_Container *e_util_container_window_find(Ecore_X_Window win);
EAPI E_Zone      *e_util_zone_window_find(Ecore_X_Window win);
EAPI E_Border    *e_util_desk_border_above(E_Border *bd);
EAPI E_Border    *e_util_desk_border_below(E_Border *bd);
EAPI int          e_util_edje_collection_exists(const char *file, const char *coll);
EAPI void         e_util_dialog_internal(const char *title, const char *txt);
EAPI const char  *e_util_filename_escape(const char *filename);
EAPI int          e_util_icon_save(Ecore_X_Icon *icon, const char *filename);
EAPI char        *e_util_shell_env_path_eval(const char *path);
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
/* check if loaded config version matches the current version, show a
   dialog warning if loaded version is older or newer than current */
EAPI Eina_Bool    e_util_module_config_check(const char *module_name, int loaded, int current);

EAPI E_Dialog                   *e_util_image_import_settings_new(const char *path, void (*cb)(void *data, const char *path, Eina_Bool ok, Eina_Bool external, int quality, E_Image_Import_Mode mode), const void *data);
EAPI E_Util_Image_Import_Handle *e_util_image_import(const char *image_path, const char *edje_path, const char *edje_group, Eina_Bool external, int quality, E_Image_Import_Mode mode, void (*cb)(void *data, Eina_Bool ok, const char *image_path, const char *edje_path), const void *data);
EAPI void                        e_util_image_import_cancel(E_Util_Image_Import_Handle *handle);

EAPI int e_util_container_desk_count_get(E_Container *con);

EAPI Eina_Bool e_util_fullscreen_curreny_any(void);
EAPI Eina_Bool e_util_fullscreen_any(void);

#endif
#endif
