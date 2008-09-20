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
     "Configuration - Mouse Settings"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   e_configure_registry_category_add("keyboard_and_mouse", 40, _("Keyboard & Mouse"), NULL, "enlightenment/behavior");
   e_configure_registry_item_add("keyboard_and_mouse/mouse_settings", 30, _("Mouse Settings"), NULL, "enlightenment/mouse_clean", e_int_config_mouse);
   conf_module = m;
   e_module_delayed_set(m, 1);
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   E_Config_Dialog *cfd;
   while ((cfd = e_config_dialog_get("E", "_config_mouse_dialog"))) e_object_del(E_OBJECT(cfd));
   e_configure_registry_item_del("keyboard_and_mouse/mouse_acceleration");
   e_configure_registry_category_del("keyboard_and_mouse");
   conf_module = NULL;
   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   return 1;
}
