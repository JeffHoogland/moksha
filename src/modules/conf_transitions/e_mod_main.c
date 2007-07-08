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
     "Configuration - Transitions"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   e_configure_registry_category_add("appearance", 10, _("Appearance"), NULL, "enlightenment/appearance");
   e_configure_registry_item_add("appearance/transitions", 80, _("Transitions"), NULL, "enlightenment/transitions", e_int_config_transitions);
   conf_module = m;
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   e_configure_registry_item_del("appearance/transitions");
   e_configure_registry_category_del("appearance");
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
			_("Enlightenment Configuration Module - Transitions"),
			_("Configuration dialog for transitions."));
   return 1;
}
