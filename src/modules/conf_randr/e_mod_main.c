/*
 * vim:ts=8:sw=3:sts=8:expandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

/* actual module specifics */
E_Module *conf_randr_module = NULL;

/* module setup */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
     "Settings - Screen Setup"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   e_configure_registry_category_add("screen", 30, _("Screen"), NULL, "preferences-desktop-display");
   e_configure_registry_item_add("screen/randr", 20, _("Screen Setup"), NULL, "preferences-system-screen-resolution", e_int_config_randr);
   conf_randr_module = m;
   e_module_delayed_set(m, 1);
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   E_Config_Dialog *cfd;
   while ((cfd = e_config_dialog_get("E", "screen/randr"))) e_object_del(E_OBJECT(cfd));
   e_configure_registry_item_del("screen/randr");
   e_configure_registry_category_del("screen");
   conf_randr_module = NULL;
   return 1;
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   return 1;
}
