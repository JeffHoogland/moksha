#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

int e_syscon_init(void);
int e_syscon_shutdown(void);

int  e_syscon_show(E_Zone *zone, const char *defact);
void e_syscon_hide(void);

E_Config_Dialog *e_int_config_syscon(E_Container *con, const char *params __UNUSED__);

void e_syscon_gadget_init(E_Module *m);
void e_syscon_gadget_shutdown(void);
void e_syscon_menu_fill(E_Menu *m);

/**
 * @addtogroup Optional_Control
 * @{
 *
 * @defgroup Module_Syscon SysCon (System Control)
 *
 * Controls your system actions such as shutdown, reboot, hibernate or
 * suspend.
 *
 * @}
 */
#endif
