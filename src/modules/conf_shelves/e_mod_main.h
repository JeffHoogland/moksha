#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#define E_TYPEDEFS 1
#include "e_int_config_shelf.h"

#undef E_TYPEDEFS
#include "e_int_config_shelf.h"

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init     (E_Module *m);
EAPI int   e_modapi_shutdown (E_Module *m);
EAPI int   e_modapi_save     (E_Module *m);

/**
 * @addtogroup Optional_Conf
 * @{
 *
 * @defgroup Module_Conf_Shelves Shelves (Panels) Configuration
 *
 * Configure shelves (panels) that exist on screen. They may be
 * horizontal or vertical, exist on top of below windows and usually
 * contain sequence of @ref Optional_Gadgets.
 *
 * @}
 */
#endif
