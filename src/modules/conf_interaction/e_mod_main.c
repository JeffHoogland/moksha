#include "e.h"
#include "e_mod_main.h"

/* actual module specifics */
static E_Module *conf_module = NULL;

/* module setup */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
     "Settings - Interaction"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   e_configure_registry_category_add("keyboard_and_mouse", 80, _("Input"), 
                                     NULL, "preferences-behavior");
   e_configure_registry_item_add("keyboard_and_mouse/interaction", 40, 
                                 _("Touch"), NULL, 
                                 "preferences-interaction", 
                                 e_int_config_interaction);
   e_configure_registry_item_add("keyboard_and_mouse/mouse_settings", 50,
                                 _("Mouse"), NULL,
                                 "preferences-desktop-mouse",
                                 e_int_config_mouse);
   conf_module = m;
   e_module_delayed_set(m, 1);
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   E_Config_Dialog *cfd;

   while ((cfd = e_config_dialog_get("E", "keyboard_and_mouse/mouse_settings")))
      e_object_del(E_OBJECT(cfd));
   while ((cfd = e_config_dialog_get("E", "keyboard_and_mouse/interaction"))) 
     e_object_del(E_OBJECT(cfd));
   
   e_configure_registry_item_del("keyboard_and_mouse/mouse_settings");
   e_configure_registry_item_del("keyboard_and_mouse/interaction");
   
   e_configure_registry_category_del("keyboard_and_mouse");
   
   conf_module = NULL;
   return 1;
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   return 1;
}
