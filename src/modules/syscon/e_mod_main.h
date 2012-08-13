#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#define E_TYPEDEFS 1
#include "e_syscon.h"
#include "e_int_config_syscon.h"

#undef E_TYPEDEFS
#include "e_syscon.h"
#include "e_int_config_syscon.h"

EAPI void *e_modapi_init     (E_Module *m);
EAPI int   e_modapi_shutdown (E_Module *m);
EAPI int   e_modapi_save     (E_Module *m);

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
