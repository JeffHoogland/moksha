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
     "Configuration - Wallpaper"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   e_configure_registry_category_add("appearance", 10, _("Appearance"), NULL, "enlightenment/appearance");
   e_configure_registry_item_add("appearance/wallpaper", 10, _("Wallpaper"), NULL, "enlightenment/background", e_int_config_wallpaper);
   e_configure_registry_category_add("internal", -1, _("Internal"), NULL, "enlightenment/internal");
   e_configure_registry_item_add("internal/wallpaper_desk", -1, _("Wallpaper"), NULL, "enlightenment/windows", e_int_config_wallpaper_desk);
   conf_module = m;
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   E_Config_Dialog *cfd;
   while ((cfd = e_config_dialog_get("E", "_config_wallpaper_dialog"))) e_object_del(E_OBJECT(cfd));
   e_configure_registry_item_del("internal/wallpaper_desk");
   e_configure_registry_category_del("internal");
   e_configure_registry_item_del("appearance/wallpaper");
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
			_("Enlightenment Configuration Module - Wallpaper"),
			_("Configuration dialog for wallpaper configuration."));
   return 1;
}
