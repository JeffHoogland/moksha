#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#define E_TYPEDEFS 1
#include "e_int_config_dialogs.h"
#include "e_int_config_profiles.h"

#undef E_TYPEDEFS
#include "e_int_config_dialogs.h"
#include "e_int_config_profiles.h"

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init     (E_Module *m);
EAPI int   e_modapi_shutdown (E_Module *m);
EAPI int   e_modapi_save     (E_Module *m);

/**
 * @addtogroup Optional_Conf
 * @{
 *
 * @defgroup Module_Conf_Dialogs Configuration Dialogs
 *
 * Controls how configuration dialogs behave (basic or advanced), as
 * well as the configuration profile in use.
 *
 * @}
 */
#endif
