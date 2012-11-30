#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

E_Config_Dialog *e_int_config_engine(E_Container *con, const char *params __UNUSED__);
E_Config_Dialog *e_int_config_performance(E_Container *con, const char *params __UNUSED__);
E_Config_Dialog *e_int_config_powermanagement(E_Container *con, const char *params __UNUSED__);

/**
 * @addtogroup Optional_Conf
 * @{
 *
 * @defgroup Module_Conf_Performance Performance Configuration
 *
 * Configures the frame rate, graphics engine and other performance
 * switches.
 *
 * @}
 */
#endif
