#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#define E_TYPEDEFS 1
#include "e_wizard.h"

#undef E_TYPEDEFS
#include "e_wizard.h"


EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init     (E_Module *m);
EAPI int   e_modapi_shutdown (E_Module *m);
EAPI int   e_modapi_save     (E_Module *m);

extern E_Module *wiz_module;

/**
 * @addtogroup Optional_Conf
 * @{
 *
 * @defgroup Module_Wizard Wizard
 *
 * Configures the whole Enlightenment in a nice walk-through wizard.
 *
 * Usually is presented on the first run of Enlightenment. The wizard
 * pages are configurable and should be extended by distributions that
 * want to ship Enlightenment as the default window manager.
 *
 * @}
 */
#endif
