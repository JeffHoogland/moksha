#include "e.h"
#include "e_mod_main.h"

/* actual module specifics */
static void      _e_mod_action_syscon_cb(E_Object *obj, const char *params);
static Eina_Bool _e_mod_syscon_defer_cb(void *data);
static void      _e_mod_syscon_cb(void *data, E_Menu *m, E_Menu_Item *mi);
static void      _e_mod_menu_add(void *data, E_Menu *m);

static E_Module *conf_module = NULL;
static E_Action *act = NULL;
static E_Int_Menu_Augmentation *maug = NULL;

/* module setup */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
   "Syscon"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   conf_module = m;
   e_syscon_init();
   /* add module supplied action */
   act = e_action_add("syscon");
   if (act)
     {
        act->func.go = _e_mod_action_syscon_cb;
        e_action_predef_name_set(_("System"), _("System Control"), "syscon",
                                 NULL, NULL, 0);
     }
   maug = e_int_menus_menu_augmentation_add_sorted
       ("main/8", _("System"), _e_mod_menu_add, NULL, NULL, NULL);
   e_configure_registry_category_add("advanced", 80, _("Advanced"), NULL, "preferences-advanced");
   e_configure_registry_item_add("advanced/syscon", 10, _("Syscon"), NULL, "system-shutdown", e_int_config_syscon);
   e_syscon_gadget_init(m);
   e_module_delayed_set(m, 1);
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   E_Config_Dialog *cfd;
   while ((cfd = e_config_dialog_get("E", "advanced/conf_syscon")))
     e_object_del(E_OBJECT(cfd));
   e_configure_registry_item_del("advanced/syscon");
   e_configure_registry_category_del("advanced");
   /* remove module-supplied menu additions */
   if (maug)
     {
        e_int_menus_menu_augmentation_del("main/8", maug);
        maug = NULL;
     }
   /* remove module-supplied action */
   if (act)
     {
        e_action_predef_name_del(_("System"), _("System Controls"));
        e_action_del("syscon");
        act = NULL;
     }
   e_syscon_gadget_shutdown();
   e_syscon_shutdown();
   conf_module = NULL;
   return 1;
}

/* action callback */
static void
_e_mod_action_syscon_cb(E_Object *obj, const char *params)
{
   E_Zone *zone = NULL;

   // params = syscon action + now:
   // desk_lock
   // logout
   // halt
   // reboot
   // suspend
   // hibernate
   if (obj)
     {
        if (obj->type == E_MANAGER_TYPE)
          zone = e_util_zone_current_get((E_Manager *)obj);
        else if (obj->type == E_CONTAINER_TYPE)
          zone = e_util_zone_current_get(((E_Container *)obj)->manager);
        else if (obj->type == E_ZONE_TYPE)
          zone = e_util_zone_current_get(((E_Zone *)obj)->container->manager);
        else
          zone = e_util_zone_current_get(e_manager_current_get());
     }
   if (!zone) zone = e_util_zone_current_get(e_manager_current_get());
   if (zone) e_syscon_show(zone, params);
}

/* menu item callback(s) */
static Eina_Bool
_e_mod_syscon_defer_cb(void *data)
{
   E_Zone *zone;

   zone = data;
   if (zone) e_syscon_show(zone, NULL);
   return ECORE_CALLBACK_CANCEL;
}

static void
_e_mod_syscon_cb(void *data __UNUSED__, E_Menu *m, E_Menu_Item *mi __UNUSED__)
{
   ecore_idle_enterer_add(_e_mod_syscon_defer_cb, m->zone);
}

static void
_e_mod_menu_generate(void *data __UNUSED__, E_Menu *m)
{
   e_syscon_menu_fill(m);
}

/* menu item add hook */
static void
_e_mod_menu_add(void *data __UNUSED__, E_Menu *m)
{
   E_Menu *sub;
   E_Menu_Item *mi;

   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("System"));
   e_util_menu_item_theme_icon_set(mi, "system");
   e_menu_item_callback_set(mi, _e_mod_syscon_cb, NULL);
   sub = e_menu_new();
   e_menu_item_submenu_set(mi, sub);
   e_object_unref(E_OBJECT(sub));
   e_menu_pre_activate_callback_set(sub, _e_mod_menu_generate, NULL);
}

