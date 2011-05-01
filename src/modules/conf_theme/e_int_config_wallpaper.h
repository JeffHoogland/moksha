#ifdef E_TYPEDEFS
typedef struct _E_Config_Wallpaper E_Config_Wallpaper;
#else
#ifndef E_INT_CONFIG_WALLPAPER_H
#define E_INT_CONFIG_WALLPAPER_H

E_Config_Dialog *e_int_config_wallpaper(E_Container *con, const char *params __UNUSED__);
E_Config_Dialog *e_int_config_wallpaper_desk(E_Container *con, const char *params);

void e_int_config_wallpaper_update(E_Config_Dialog *dia, char *file);
void e_int_config_wallpaper_import_done(E_Config_Dialog *dia);
void e_int_config_wallpaper_web_done(E_Config_Dialog *dia);

#endif
#endif
