#include "e.h"
#include "e_mod_main.h"

void *
init(E_Module *m)
{
   if (m->api->version < E_MODULE_API_VERSION)
     {
	e_error_dialog_show("Module API Error",
			    "Error initializing Module: test\n"
			    "It requires a minimum module API version of: %i.\n"
			    "The module API advertized by Enlightenment is: %i.\n"
			    "Aborting module.",
			    E_MODULE_API_VERSION,
			    m->api->version);
	return NULL;
     }
     {
	E_Menu *mn;
	E_Menu_Item *mi;
	
	mn = e_menu_new();
	mi = e_menu_item_new(mn);
	e_menu_item_label_set(mi, "Module Config Menu Item 1");
	mi = e_menu_item_new(mn);
	e_menu_item_label_set(mi, "Module Config Menu Item 2");
	mi = e_menu_item_new(mn);
	e_menu_item_label_set(mi, "Module Config Menu Item 3");
	mi = e_menu_item_new(mn);
	e_menu_item_separator_set(mi, 1);
	mi = e_menu_item_new(mn);
	e_menu_item_label_set(mi, "Something Else");
	m->config_menu = mn;
     }
   return init; /* bogus pointer - just to say we worked */
}

int
shutdown(E_Module *m)
{
   if (m->config_menu)
     {
	e_menu_deactivate(m->config_menu);
	e_object_unref(E_OBJECT(m->config_menu));
	m->config_menu = NULL;
     }
   return 1;
}

int
save(E_Module *m)
{
   return 1;
}

int
info(E_Module *m)
{
   m->label = strdup("Test!!!");
   return 1;
}

int
about(E_Module *m)
{
   e_error_dialog_show("Enlightenment Test Module",
		       "This module is VERY simple and is only used to test the basic\n"
		       "interface of the Enlightenment 0.17.0 module system. Please\n"
		       "Ignore this module unless you are working on the module system.");
   return 1;
}
