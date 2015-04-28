#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

E_Config_Dialog *e_int_config_env(E_Container *con, const char *params __UNUSED__);
E_Config_Dialog *e_int_config_paths(E_Container *con, const char *params __UNUSED__);

/**
 * @addtogroup Optional_Conf
 * @{
 *
 * @defgroup Module_Conf_Paths Paths & Environment Configuration
 *
 * Configures where to search for fonts, icons, images, themes,
 * walpapers and others.
 *
 * Can also configure environment variables used and propagated by
 * Enlightenment to child process and applications.
 *
 * @}
 */
#endif
