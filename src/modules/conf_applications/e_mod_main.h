#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#define E_TYPEDEFS 1
#include "e_int_config_apps.h"
#include "e_int_config_defapps.h"
#include "e_int_config_deskenv.h"
#include "e_int_config_apps_personal.h"

#undef E_TYPEDEFS
#include "e_int_config_apps.h"
#include "e_int_config_defapps.h"
#include "e_int_config_deskenv.h"
#include "e_int_config_apps_personal.h"

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init     (E_Module *m);
EAPI int   e_modapi_shutdown (E_Module *m);
EAPI int   e_modapi_save     (E_Module *m);

/**
 * @addtogroup Optional_Conf
 * @{
 *
 * @defgroup Module_Conf_Applications Applications Configuration
 *
 * Configure application icons (launchers ".desktop"), which
 * applications to use on start or restart of Enlightenment.
 *
 * @}
 */
#endif
