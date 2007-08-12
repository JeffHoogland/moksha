/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

/***************************************************************************/
/**/
/* actual module specifics */

static E_Module *conf_module = NULL;

/**/
/***************************************************************************/

/***************************************************************************/
/**/

/**/
/***************************************************************************/

/***************************************************************************/
/**/
/* module setup */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
     "Wizard"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   conf_module = m;
   e_wizard_init();
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   e_wizard_shutdown();
   conf_module = NULL;
   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   return 1;
}

EAPI int
e_modapi_about(E_Module *m)
{
   e_module_dialog_show(m,
			_("Enlightenment First Run Wizard Module"),
			_("A module for setting up configuration for Enlightenment for the first time."));
   return 1;
}
