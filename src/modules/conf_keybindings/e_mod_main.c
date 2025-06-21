#include "e.h"
#include "e_mod_main.h"

/* actual module specifics */
static E_Module *conf_module = NULL;
static E_Int_Menu_Augmentation *maug = NULL;
static E_Action *act = NULL;

/* module setup */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
   "Settings - Input Controls"
};

static void
_show_keybidings_menu_cb(void *data __UNUSED__, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   show_keybidings();
}

static void
_show_keybidings_cb(E_Object *obj __UNUSED__, const char *params __UNUSED__)
{
   show_keybidings();
}

static void
_e_mod_menu_add(void *data __UNUSED__, E_Menu *m)
{
   E_Menu_Item *mi;

   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Key Bindings"));
   e_util_menu_item_theme_icon_set(mi, "preferences-desktop-keyboard-shortcuts");
   e_menu_item_callback_set(mi, _show_keybidings_menu_cb, NULL);
}

EAPI void *
e_modapi_init(E_Module *m)
{
   e_configure_registry_category_add("keyboard_and_mouse", 40, _("Input"),
                                     NULL, "preferences-behavior");

   e_configure_registry_item_add("keyboard_and_mouse/key_bindings", 10,
                                 _("Key Bindings"), NULL,
                                 "preferences-desktop-keyboard-shortcuts",
                                 e_int_config_keybindings);
   e_configure_registry_item_add("keyboard_and_mouse/mouse_bindings", 20,
                                 _("Mouse Bindings"), NULL,
                                 "preferences-desktop-mouse",
                                 e_int_config_mousebindings);
   e_configure_registry_item_add("keyboard_and_mouse/acpi_bindings", 30,
                                 _("ACPI Bindings"), NULL,
                                 "preferences-system-power-management",
                                 e_int_config_acpibindings);

   act = e_action_add("show_keybinds");
   if (act)
     {
        act->func.go = _show_keybidings_cb;
        e_action_predef_name_set(_("Keybindings"), _("View Moksha Keybindings"),
                                 "show_keybinds", NULL, NULL, 0);
     }

   maug = e_int_menus_menu_augmentation_add_sorted
      ("config/1",  _("Show bindings"), _e_mod_menu_add, NULL, NULL, NULL);

   conf_module = m;
   e_module_delayed_set(m, 1);
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   E_Config_Dialog *cfd;

   while ((cfd = e_config_dialog_get("E", "keyboard_and_mouse/acpi_bindings")))
     e_object_del(E_OBJECT(cfd));
   while ((cfd = e_config_dialog_get("E", "keyboard_and_mouse/mouse_bindings")))
     e_object_del(E_OBJECT(cfd));
   while ((cfd = e_config_dialog_get("E", "keyboard_and_mouse/key_bindings")))
     e_object_del(E_OBJECT(cfd));

   e_configure_registry_item_del("keyboard_and_mouse/acpi_bindings");
   e_configure_registry_item_del("keyboard_and_mouse/mouse_bindings");
   e_configure_registry_item_del("keyboard_and_mouse/key_bindings");

   e_configure_registry_category_del("keyboard_and_mouse");

   if (act)
     {
        e_action_predef_name_del("Keybindings", "View Moksha Keybindings");
        e_action_del("show_keybinds");
        act = NULL;
     }

   conf_module = NULL;
   return 1;
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   return 1;
}

