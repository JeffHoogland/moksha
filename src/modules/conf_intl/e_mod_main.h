#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

E_Config_Dialog *e_int_config_imc(E_Container *con, const char *params __UNUSED__);
void e_int_config_imc_import_done(E_Config_Dialog *dia);
void e_int_config_imc_update(E_Config_Dialog *dia, const char *file);

E_Win *e_int_config_imc_import(E_Config_Dialog *parent);
void e_int_config_imc_import_del(E_Win *win);

E_Config_Dialog *e_int_config_intl(E_Container *con, const char *params __UNUSED__);
E_Config_Dialog *e_int_config_desklock_intl(E_Container *con, const char *params __UNUSED__);

/**
 * @addtogroup Optional_Conf
 * @{
 *
 * @defgroup Module_Conf_Intl Internationalization Configuration
 *
 * Configure language and input method (e.g. Asian languages)
 *
 * @}
 */
#endif
