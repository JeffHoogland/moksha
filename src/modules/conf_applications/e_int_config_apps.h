#ifdef E_TYPEDEFS
#else
#ifndef E_INT_CONFIG_APPS_H
#define E_INT_CONFIG_APPS_H

EAPI E_Config_Dialog *e_int_config_apps_favs(E_Container *con, const char *params __UNUSED__);
EAPI E_Config_Dialog *e_int_config_apps_add(E_Container *con, const char *params __UNUSED__);
EAPI E_Config_Dialog *e_int_config_apps_ibar(E_Container *con, const char *params __UNUSED__);
EAPI E_Config_Dialog *e_int_config_apps_ibar_other(E_Container *con, const char *path);
EAPI E_Config_Dialog *e_int_config_apps_startup(E_Container *con, const char *params __UNUSED__);
EAPI E_Config_Dialog *e_int_config_apps_restart(E_Container *con, const char *params __UNUSED__);

#endif
#endif
