#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

E_Config_Dialog *e_int_config_acpibindings(E_Container *con, const char *params __UNUSED__);
E_Config_Dialog *e_int_config_keybindings(E_Container *con, const char *params __UNUSED__);
E_Config_Dialog *e_int_config_mousebindings(E_Container *con, const char *params __UNUSED__);

/**
 * @addtogroup Optional_Conf
 * @{
 *
 * @defgroup Module_Conf_KeyBinding Key Bindings (Shortcuts) Configuration
 *
 * Configure global keyboard shortcuts.
 *
 * @see Module_Conf_MouseBinding
 * @see Module_Conf_EdgeBinding
 * @}
 */
#endif
