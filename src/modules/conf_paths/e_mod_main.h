#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#define E_TYPEDEFS 1
#include "e_int_config_paths.h"
#include "e_int_config_env.h"

#undef E_TYPEDEFS
#include "e_int_config_paths.h"
#include "e_int_config_env.h"

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init     (E_Module *m);
EAPI int   e_modapi_shutdown (E_Module *m);
EAPI int   e_modapi_save     (E_Module *m);

/**
 * @addtogroup Optional_Conf
 * @{
 *
 * @defgroup Module_Conf_Paths Paths & Environment Configuration
 *
 * Configures where to search for fonts, icons, images, themes,
 * walpapers and others.
 *
 * Can also configure environment variables used and propagated by
 * Enlightenment to child process and applications.
 *
 * @}
 */
#endif
