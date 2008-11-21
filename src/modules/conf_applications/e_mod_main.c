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
     "Configuration - Applications"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   e_configure_registry_category_add("applications", 20, _("Apps"), NULL, "enlightenment/applications");
   e_configure_registry_item_add("applications/new_application", 10, _("New Application"), NULL, "enlightenment/add_application", e_int_config_apps_add);
   e_configure_registry_item_add("applications/ibar_applications", 20, _("IBar Applications"), NULL, "enlightenment/ibar_applications", e_int_config_apps_ibar);
   e_configure_registry_item_add("applications/restart_applications", 30, _("Restart Applications"), NULL, "enlightenment/restart_applications", e_int_config_apps_restart);
   e_configure_registry_item_add("applications/startup_applications", 40, _("Startup Applications"), NULL, "enlightenment/startup_applications", e_int_config_apps_startup);
   e_configure_registry_category_add("internal", -1, _("Internal"), NULL, "enlightenment/internal");
   e_configure_registry_item_add("internal/ibar_other", -1, _("IBar Other"), NULL, "enlightenment/windows", e_int_config_apps_ibar_other);
   e_configure_registry_category_add("menus", 60, _("Menus"), NULL, "enlightenment/menus");
   e_configure_registry_item_add("menus/favorites_menu", 10, _("Favorites Menu"), NULL, "enlightenment/favorites", e_int_config_apps_favs);
   conf_module = m;
   e_module_delayed_set(m, 1);
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   E_Config_Dialog *cfd;
   while ((cfd = e_config_dialog_get("E", "_config_apps_dialog"))) e_object_del(E_OBJECT(cfd));
   e_configure_registry_item_del("menus/favorites_menu");
   e_configure_registry_category_del("menus");
   e_configure_registry_item_del("internal/ibar_other");
   e_configure_registry_category_del("internal");
   e_configure_registry_item_del("applications/new_application");
   e_configure_registry_item_del("applications/ibar_applications");
   e_configure_registry_item_del("applications/restart_applications");
   e_configure_registry_item_del("applications/startup_applications");
   e_configure_registry_category_del("applications");
   conf_module = NULL;
   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   return 1;
}
