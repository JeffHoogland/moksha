#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

typedef struct _E_Config_Wallpaper E_Config_Wallpaper;

E_Config_Dialog *e_int_config_xsettings(E_Container *con, const char *params __UNUSED__);

/**
 * @addtogroup Optional_Conf
 * @{
 *
 * @defgroup Module_Conf_Theme Theme Configuration
 *
 * Changes the graphical user interface theme and wallpaper.
 *
 * @}
 */

E_Config_Dialog *e_int_config_borders(E_Container *con, const char *params __UNUSED__);
E_Config_Dialog *e_int_config_borders_border(E_Container *con, const char *params);

E_Config_Dialog *e_int_config_color_classes(E_Container *con, const char *params __UNUSED__);

E_Config_Dialog *e_int_config_fonts(E_Container *con, const char *params __UNUSED__);

E_Config_Dialog *e_int_config_scale(E_Container *con, const char *params __UNUSED__);

E_Config_Dialog *e_int_config_startup(E_Container *con, const char *params __UNUSED__);

E_Config_Dialog *e_int_config_theme(E_Container *con, const char *params __UNUSED__);

void             e_int_config_theme_import_done(E_Config_Dialog *dia);
void             e_int_config_theme_update(E_Config_Dialog *dia, char *file);
void             e_int_config_theme_web_done(E_Config_Dialog *dia);

E_Win *e_int_config_theme_import (E_Config_Dialog *parent);
void   e_int_config_theme_del    (E_Win *win);

E_Config_Dialog *e_int_config_transitions(E_Container *con, const char *params __UNUSED__);

E_Config_Dialog *e_int_config_wallpaper(E_Container *con, const char *params __UNUSED__);
E_Config_Dialog *e_int_config_wallpaper_desk(E_Container *con, const char *params);

void e_int_config_wallpaper_update(E_Config_Dialog *dia, char *file);
void e_int_config_wallpaper_import_done(E_Config_Dialog *dia);
void e_int_config_wallpaper_web_done(E_Config_Dialog *dia);

#endif
