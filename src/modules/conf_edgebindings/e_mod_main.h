#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#define E_TYPEDEFS 1
#include "e_int_config_edgebindings.h"

#undef E_TYPEDEFS
#include "e_int_config_edgebindings.h"

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init     (E_Module *m);
EAPI int   e_modapi_shutdown (E_Module *m);
EAPI int   e_modapi_save     (E_Module *m);

E_Config_Dialog *e_int_config_signalbindings(E_Container *con, const char *params);


/**
 * @addtogroup Optional_Conf
 * @{
 *
 * @defgroup Module_Conf_EdgeBindings Edge Bindings Configuration
 *
 * Controls action on screen edges and corners.
 *
 * @see Module_Conf_MouseBindings
 * @see Module_Conf_KeyBindings
 * @}
 */
#endif
