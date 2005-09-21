/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

void *
e_modapi_init(E_Module *m)
{
   if (m->api->version < E_MODULE_API_VERSION)
     {
	E_Dialog *dia;
	char buf[4096];

	dia = e_dialog_new(e_container_current_get(e_manager_current_get()));
	if (!dia) return NULL;

	snprintf(buf, sizeof(buf), _("Module API Error<br>Error initializing Module: Test<br>"
				     "It requires a minimum module API version of: %i.<br>"
				     "The module API advertized by Enlightenment is: %i.<br>"), 
				   E_MODULE_API_VERSION, m->api->version);

	e_dialog_title_set(dia, "Enlightenment Test Module");
	e_dialog_icon_set(dia, "enlightenment/e", 64);
	e_dialog_text_set(dia, buf);
	e_dialog_button_add(dia, _("Ok"), NULL, NULL, NULL);
	e_win_centered_set(dia->win, 1);
	e_dialog_show(dia);
	return NULL;
     }
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
     }
   return e_modapi_init; /* bogus pointer - just to say we worked */
}

int
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

int
e_modapi_save(E_Module *m)
{
   return 1;
}

int
e_modapi_info(E_Module *m)
{
   m->label = strdup(_("Test!!!"));
   return 1;
}

int
e_modapi_about(E_Module *m)
{
   E_Dialog *dia;

   dia = e_dialog_new(e_container_current_get(e_manager_current_get()));
   if (!dia) return 0;
   e_dialog_title_set(dia, "Enlightenment Test Module");
   e_dialog_icon_set(dia, "enlightenment/e", 64);
   e_dialog_text_set(dia, _("This module is VERY simple and is only used to test the basic<br>"
			    "interface of the Enlightenment 0.17.0 module system. Please<br>"
			    "ignore this module unless you are working on the module system."));
   e_dialog_button_add(dia, _("Ok"), NULL, NULL, NULL);
   e_win_centered_set(dia->win, 1);
   e_dialog_show(dia);
   return 1;
}
