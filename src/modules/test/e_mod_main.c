/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

EAPI E_Module_Api e_modapi = 
{
   E_MODULE_API_VERSION,
   "Test"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   E_Menu *mn;
   E_Menu_Item *mi;

   mn = e_menu_new();
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Module Config Menu Item 1"));
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Module Config Menu Item 2"));
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Module Config Menu Item 3"));
   mi = e_menu_item_new(mn);
   e_menu_item_separator_set(mi, 1);
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Something Else"));
   m->config_menu = mn;

   return e_modapi_init; /* bogus pointer - just to say we worked */
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   if (m->config_menu)
     {
	e_menu_deactivate(m->config_menu);
	e_object_unref(E_OBJECT(m->config_menu));
	m->config_menu = NULL;
     }
   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   return 1;
}

EAPI int
e_modapi_info(E_Module *m)
{
   return 1;
}

EAPI int
e_modapi_about(E_Module *m)
{
   e_module_dialog_show(_("Enlightenment Test Module"),
			_("This module is VERY simple and is only used to test the basic<br>"
			  "interface of the Enlightenment 0.17.0 module system. Please<br>"
			  "ignore this module unless you are working on the module system."));
   return 1;
}
