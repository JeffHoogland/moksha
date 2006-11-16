/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS
#else
#ifndef E_INT_CONFIG_APPS_H
#define E_INT_CONFIG_APPS_H

EAPI E_Config_Dialog *e_int_config_apps(E_Container *con);
EAPI E_Config_Dialog *e_int_config_apps_once(E_Container *con, const char *title, const char *label, const char *path, int (*func) (void *data, const char *path), void *data);

#endif
#endif
