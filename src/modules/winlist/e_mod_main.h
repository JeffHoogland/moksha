#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#include "e_winlist.h"

extern const char *_winlist_act;
extern E_Action *_act_winlist;
E_Config_Dialog *e_int_config_winlist(E_Container *con, const char *params __UNUSED__);
/**
 * @addtogroup Optional_Control
 * @{
 *
 * @defgroup Module_Winlist WinList (Windows Listing)
 *
 * Lists and switch windows, usually with "Alt-Tab" or "Alt-Shift-Tab"
 * keyboard shortcuts. Can show a popup during the action.
 *
 * @}
 */
#endif
