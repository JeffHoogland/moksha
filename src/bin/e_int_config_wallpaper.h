/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS
#else
#ifndef E_INT_CONFIG_WALLPAPER_H
#define E_INT_CONFIG_WALLPAPER_H

EAPI E_Config_Dialog *e_int_config_wallpaper(E_Container *con);
EAPI void             e_int_config_wallpaper_update(E_Config_Dialog *dia, char *file);
EAPI void             e_int_config_wallpaper_import_done(E_Config_Dialog *dia);
EAPI void             e_int_config_wallpaper_gradient_done(E_Config_Dialog *dia);

#endif
#endif
