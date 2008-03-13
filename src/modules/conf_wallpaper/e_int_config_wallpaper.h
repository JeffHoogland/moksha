/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS
typedef struct _E_Config_Wallpaper E_Config_Wallpaper;
#else
#ifndef E_INT_CONFIG_WALLPAPER_H
#define E_INT_CONFIG_WALLPAPER_H

EAPI E_Config_Dialog *e_int_config_wallpaper(E_Container *con, const char *params __UNUSED__);
EAPI E_Config_Dialog *e_int_config_wallpaper_desk(E_Container *con, const char *params);

EAPI void e_int_config_wallpaper_update(E_Config_Dialog *dia, char *file);
EAPI void e_int_config_wallpaper_import_done(E_Config_Dialog *dia);
EAPI void e_int_config_wallpaper_gradient_done(E_Config_Dialog *dia);
EAPI void e_int_config_wallpaper_web_done(E_Config_Dialog *dia);

EAPI void e_int_config_wallpaper_handler_set(Evas_Object *obj, const char *path, void *data);
EAPI int e_int_config_wallpaper_handler_test(Evas_Object *obj, const char *path, void *data);

#endif
#endif
