#include "e.h"
#include "e_mod_main.h"

/* actual module specifics */
static E_Module *conf_module = NULL;

/* module setup */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
     "Settings - Screen"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   e_configure_registry_category_add("screen", 30, _("Screen"), NULL, 
                                     "preferences-desktop-display");
   
   e_configure_registry_item_add("screen/virtual_desktops", 10,
                                 _("Virtual Desktops"), NULL,
                                 "preferences-desktop", e_int_config_desks);
   e_configure_registry_item_add("screen/screen_resolution", 20, 
                                 _("Screen Resolution"), NULL, 
                                 "preferences-system-screen-resolution", 
                                 e_int_config_display);
   e_configure_registry_item_add("screen/screen_lock", 30,
                                 _("Screen Lock"), NULL,
                                 "preferences-system-lock-screen",
                                 e_int_config_desklock);
   e_configure_registry_item_add("screen/screen_saver", 40, 
                                 _("Screen Saver"), NULL, 
                                 "preferences-desktop-screensaver", 
                                 e_int_config_screensaver);
   e_configure_registry_item_add("screen/power_management", 50, 
                                 _("Power Management"), NULL, 
                                 "preferences-system-power-management",
                                 e_int_config_dpms);
   
   e_configure_registry_category_add("internal", -1, _("Internal"), NULL,
                                     "enlightenment/internal");
   
   e_configure_registry_item_add("internal/desk", -1,
                                 _("Desk"), NULL,
                                 "preferences-system-windows",
                                 e_int_config_desk);
   
   conf_module = m;
   e_module_delayed_set(m, 1);
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   E_Config_Dialog *cfd;

   while ((cfd = e_config_dialog_get("E", "internal/desk")))
      e_object_del(E_OBJECT(cfd));
   
   e_configure_registry_item_del("internal/desk");
   
   e_configure_registry_category_del("internal");
   
   while ((cfd = e_config_dialog_get("E", "screen/power_management")))
      e_object_del(E_OBJECT(cfd));
   while ((cfd = e_config_dialog_get("E", "screen/screen_saver")))
      e_object_del(E_OBJECT(cfd));
   while ((cfd = e_config_dialog_get("E", "screen/screen_lock")))
      e_object_del(E_OBJECT(cfd));
   while ((cfd = e_config_dialog_get("E", "screen/screen_resolution"))) 
      e_object_del(E_OBJECT(cfd));
   while ((cfd = e_config_dialog_get("E", "screen/virtual_desktops")))
      e_object_del(E_OBJECT(cfd));

   e_configure_registry_item_del("screen/power_management");
   e_configure_registry_item_del("screen/screen_saver");
   e_configure_registry_item_del("screen/screen_lock");
   e_configure_registry_item_del("screen/screen_resolution");
   e_configure_registry_item_del("screen/virtual_desktops");
   
   e_configure_registry_category_del("screen");
   conf_module = NULL;
   return 1;
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   return 1;
}
