#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#define E_TYPEDEFS 1
#include "e_winlist.h"
#include "e_int_config_winlist.h"

#undef E_TYPEDEFS
#include "e_winlist.h"
#include "e_int_config_winlist.h"

extern const char *_winlist_act;
extern E_Action *_act_winlist;
EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init     (E_Module *m);
EAPI int   e_modapi_shutdown (E_Module *m);
EAPI int   e_modapi_save     (E_Module *m);

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
