#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#define E_TYPEDEFS 1
#include "e_int_config_wallpaper.h"
#undef E_TYPEDEFS
#include "e_int_config_wallpaper.h"

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init     (E_Module *m);
EAPI int   e_modapi_shutdown (E_Module *m);
EAPI int   e_modapi_save     (E_Module *m);

/**
 * @addtogroup Optional_Conf
 * @{
 *
 * @defgroup Module_Conf_Wallpaper2 Wallpaper2 (Alternative Selector)
 *
 * More graphically appealing wallpaper selector. It is targeted at
 * mobile devices.
 *
 * @note under development and not recommended for use yet.
 *
 * @see Module_Conf_Theme
 * @}
 */
#endif
