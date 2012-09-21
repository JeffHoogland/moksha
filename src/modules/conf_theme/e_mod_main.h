#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#define E_TYPEDEFS 1
#include "e_int_config_borders.h"
#include "e_int_config_color_classes.h"
#include "e_int_config_fonts.h"
#include "e_int_config_scale.h"
#include "e_int_config_startup.h"
#include "e_int_config_theme.h"
#include "e_int_config_theme_import.h"
#include "e_int_config_transitions.h"
#include "e_int_config_wallpaper.h"

#undef E_TYPEDEFS
#include "e_int_config_borders.h"
#include "e_int_config_color_classes.h"
#include "e_int_config_fonts.h"
#include "e_int_config_scale.h"
#include "e_int_config_startup.h"
#include "e_int_config_theme.h"
#include "e_int_config_theme_import.h"
#include "e_int_config_transitions.h"
#include "e_int_config_wallpaper.h"

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init     (E_Module *m);
EAPI int   e_modapi_shutdown (E_Module *m);
EAPI int   e_modapi_save     (E_Module *m);

E_Config_Dialog *e_int_config_xsettings(E_Container *con, const char *params __UNUSED__);

/**
 * @addtogroup Optional_Conf
 * @{
 *
 * @defgroup Module_Conf_Theme Theme Configuration
 *
 * Changes the graphical user interface theme and wallpaper.
 *
 * @}
 */
#endif
