#ifdef E_TYPEDEFS
//typedef struct _E_Config_Wallpaper E_Config_Wallpaper;
#else
#ifndef E_INT_CONFIG_WALLPAPER_H
#define E_INT_CONFIG_WALLPAPER_H

E_Config_Dialog *wp_conf_show(E_Container *con, const char *params);
void wp_conf_hide(void);

#endif
#endif
