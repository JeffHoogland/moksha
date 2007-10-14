/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS
#else
#ifndef E_THEME_H
#define E_THEME_H

EAPI int         e_theme_init(void);
EAPI int         e_theme_shutdown(void);

EAPI int         e_theme_edje_object_set(Evas_Object *o, const char *category, const char *group);
EAPI const char *e_theme_edje_file_get(const char *category, const char *group);
EAPI void        e_theme_file_set(const char *category, const char *file);

EAPI int             e_theme_config_set(const char *category, const char *file);
EAPI E_Config_Theme *e_theme_config_get(const char *category);
EAPI int             e_theme_config_remove(const char *category);
EAPI Evas_List      *e_theme_config_list(void);

EAPI int        e_theme_category_find(const char *category);
EAPI Evas_List *e_theme_category_list(void);
EAPI int        e_theme_transition_find(const char *transition);
EAPI Evas_List *e_theme_transition_list(void);
EAPI int        e_theme_border_find(const char *border);
EAPI Evas_List *e_theme_border_list(void);
EAPI int        e_theme_shelf_find(const char *shelf);
EAPI Evas_List *e_theme_shelf_list(void);

EAPI void e_theme_handler_set(Evas_Object *obj, const char *path, void *data);
EAPI int e_theme_handler_test(Evas_Object *obj, const char *path, void *data);

#endif
#endif
