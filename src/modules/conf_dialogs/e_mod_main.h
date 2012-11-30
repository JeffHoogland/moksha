#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

E_Config_Dialog *e_int_config_profiles(E_Container *con, const char *params __UNUSED__);
E_Config_Dialog *e_int_config_dialogs(E_Container *con, const char *params __UNUSED__);

/**
 * @addtogroup Optional_Conf
 * @{
 *
 * @defgroup Module_Conf_Dialogs Configuration Dialogs
 *
 * Controls how configuration dialogs behave (basic or advanced), as
 * well as the configuration profile in use.
 *
 * @}
 */
#endif
