#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

E_Config_Dialog *e_int_config_edgebindings(E_Container *con, const char *params __UNUSED__);
E_Config_Dialog *e_int_config_signalbindings(E_Container *con, const char *params);


/**
 * @addtogroup Optional_Conf
 * @{
 *
 * @defgroup Module_Conf_EdgeBindings Edge Bindings Configuration
 *
 * Controls action on screen edges and corners.
 *
 * @see Module_Conf_MouseBindings
 * @see Module_Conf_KeyBindings
 * @}
 */
#endif
