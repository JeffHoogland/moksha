#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init(E_Module *m);
EAPI int e_modapi_shutdown(E_Module *m);
EAPI int e_modapi_save(E_Module *m);

/**
 * @addtogroup Optional_Gadgets
 * @{
 *
 * @defgroup Module_Gadman Gadman (Gadget Manager)
 *
 * Manages @ref Optional_Gadgets on desktop or overlay.
 *
 * @}
 */

#endif
