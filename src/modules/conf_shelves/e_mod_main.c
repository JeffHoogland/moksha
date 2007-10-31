/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

/***************************************************************************/
/**/
/* actual module specifics */

static void  _e_mod_run_cb(void *data, E_Menu *m, E_Menu_Item *mi);
static void  _e_mod_menu_add(void *data, E_Menu *m);

static E_Module *conf_module = NULL;
static E_Int_Menu_Augmentation *maug = NULL;

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
     "Configuration - Shelves"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   e_configure_registry_category_add("extensions", 90, _("Extensions"), NULL, "enlightenment/extensions");
   e_configure_registry_item_add("extensions/shelves", 20, _("Shelves"), NULL, "enlightenment/shelf", e_int_config_shelf);
   maug = e_int_menus_menu_augmentation_add("config/1", _e_mod_menu_add, NULL, NULL, NULL);

   conf_module = m;
   e_module_delayed_set(m, 1);
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   E_Config_Dialog *cfd;
   /* remove module-supplied menu additions */
   if (maug)
     {
	e_int_menus_menu_augmentation_del("config/1", maug);
	maug = NULL;
     }
   while ((cfd = e_config_dialog_get("E", "_config_shelf_dialog"))) e_object_del(E_OBJECT(cfd));
   e_configure_registry_item_del("extensions/shelves");
   e_configure_registry_category_del("extensions");
   conf_module = NULL;
   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   return 1;
}

/* menu item callback(s) */
static void 
_e_mod_run_cb(void *data, E_Menu *m, E_Menu_Item *mi)
{
   e_configure_registry_call("extensions/shelves", m->zone->container, NULL);
}

/* menu item add hook */
static void
_e_mod_menu_add(void *data, E_Menu *m)
{
   E_Menu_Item *mi;
   
   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Shelves"));
   e_util_menu_item_edje_icon_set(mi, "enlightenment/shelf");
   e_menu_item_callback_set(mi, _e_mod_run_cb, NULL);
}
