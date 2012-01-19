#ifndef E_MOD_MAIN_H
# define E_MOD_MAIN_H

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init(E_Module *m);
EAPI int e_modapi_shutdown(E_Module *m);
EAPI int e_modapi_save(E_Module *m);

/**
 * @addtogroup Optional_Mobile
 * @{
 *
 * @defgroup Module_Illume2 Illume2
 *
 * Second generation of Illume mobile environment for Enlightenment.
 *
 * @see @ref Illume_Main_Page
 *
 * @}
 */
#endif
