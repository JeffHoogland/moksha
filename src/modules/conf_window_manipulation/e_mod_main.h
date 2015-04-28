#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

E_Config_Dialog *e_int_config_window_geometry(E_Container *con, const char *params __UNUSED__);
E_Config_Dialog *e_int_config_window_process(E_Container *con, const char *params __UNUSED__);

E_Config_Dialog *e_int_config_window_display(E_Container *con, const char *params __UNUSED__);

E_Config_Dialog *e_int_config_focus(E_Container *con, const char *params __UNUSED__);

E_Config_Dialog *e_int_config_clientlist(E_Container *con, const char *params __UNUSED__);

/**
 * @addtogroup Optional_Conf
 * @{
 *
 * @defgroup Module_Conf_Window_Manipulation Window Manipulation Configuration
 *
 * Configures how windows are placed, maximized, focused, stacked and
 * so on.
 *
 * @see Module_Conf_Window_Remembers
 * @}
 */
#endif
