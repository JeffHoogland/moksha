#ifdef E_TYPEDEFS
#else
#ifndef E_INT_CONFIG_WALLPAPER_IMPORT_H
#define E_INT_CONFIG_WALLPAPER_IMPORT_H

EAPI E_Win *e_int_config_wallpaper_fsel(E_Config_Dialog *parent);
EAPI E_Win *e_int_config_wallpaper_import(void *data, const char *path);
EAPI void e_int_config_wallpaper_del(E_Win *win);
    
#endif
#endif
