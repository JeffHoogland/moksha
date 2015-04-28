#ifdef E_TYPEDEFS
#else
#ifndef E_THEME_H
#define E_THEME_H

EINTERN int         e_theme_init(void);
EINTERN int         e_theme_shutdown(void);

EAPI int         e_theme_edje_object_set(Evas_Object *o, const char *category, const char *group);
EAPI const char *e_theme_edje_file_get(const char *category, const char *group);
EAPI const char *e_theme_edje_icon_fallback_file_get(const char *group);
EAPI void        e_theme_file_set(const char *category, const char *file);

EAPI int             e_theme_config_set(const char *category, const char *file);
EAPI E_Config_Theme *e_theme_config_get(const char *category);
EAPI int             e_theme_config_remove(const char *category);
EAPI Eina_List      *e_theme_config_list(void);

EAPI int        e_theme_category_find(const char *category);
EAPI Eina_List *e_theme_category_list(void);
EAPI int        e_theme_transition_find(const char *transition);
EAPI Eina_List *e_theme_transition_list(void);
EAPI int        e_theme_border_find(const char *border);
EAPI Eina_List *e_theme_border_list(void);
EAPI int        e_theme_shelf_find(const char *shelf);
EAPI Eina_List *e_theme_shelf_list(void);
EAPI int        e_theme_comp_find(const char *shelf);
EAPI Eina_List *e_theme_comp_list(void);

#endif
#endif
