#include "e.h"
#include "e_mod_main.h"

/* actual module specifics */
static E_Module *conf_module = NULL;

/* module setup */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
   "Settings - Edge Bindings"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   e_configure_registry_category_add("keyboard_and_mouse", 40, _("Input"),
                                     NULL, "preferences-behavior");
   e_configure_registry_category_add("advanced", 80, _("Advanced"), NULL, "preferences-advanced");
   e_configure_registry_item_add("keyboard_and_mouse/edge_bindings", 10,
                                 _("Edge Bindings"), NULL,
                                 "preferences-desktop-edge-bindings",
                                 e_int_config_edgebindings);
   e_configure_registry_item_add("advanced/signal_bindings", 10,
                                 _("Signal Bindings"), NULL,
                                 "preferences-desktop-signal-bindings",
                                 e_int_config_signalbindings);
   conf_module = m;
   e_module_delayed_set(m, 1);
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   E_Config_Dialog *cfd;

   while ((cfd = e_config_dialog_get("E", "keyboard_and_mouse/edge_bindings")))
     e_object_del(E_OBJECT(cfd));
   while ((cfd = e_config_dialog_get("E", "advanced/signal_bindings")))
     e_object_del(E_OBJECT(cfd));
   e_configure_registry_item_del("keyboard_and_mouse/edge_bindings");
   e_configure_registry_item_del("advanced/signal_bindings");
   e_configure_registry_category_del("keyboard_and_mouse");
   e_configure_registry_category_del("advanced");
   conf_module = NULL;
   return 1;
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   return 1;
}

