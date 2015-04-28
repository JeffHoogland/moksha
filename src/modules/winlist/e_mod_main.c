#include "e.h"
#include "e_mod_main.h"

/* actual module specifics */
static void _e_mod_action_winlist_cb(E_Object *obj, const char *params);
static void _e_mod_action_winlist_mouse_cb(E_Object *obj, const char *params, Ecore_Event_Mouse_Button *ev);
static void _e_mod_action_winlist_key_cb(E_Object *obj, const char *params, Ecore_Event_Key *ev);
static void _e_mod_action_winlist_edge_cb(E_Object *obj, const char *params, E_Event_Zone_Edge *ev);
static void _e_mod_action_winlist_signal_cb(E_Object *obj, const char *params, const char *sig, const char *src);
static void _e_mod_action_winlist_acpi_cb(E_Object *obj, const char *params, E_Event_Acpi *ev);

static E_Module *conf_module = NULL;
const char *_winlist_act = NULL;
E_Action *_act_winlist = NULL;

/* module setup */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
   "Winlist"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   conf_module = m;
   e_configure_registry_category_add("windows", 50, _("Windows"), NULL, "preferences-system-windows");
   e_configure_registry_item_add("windows/window_list", 70, _("Window Switcher"), NULL, "preferences-winlist", e_int_config_winlist);
   e_winlist_init();
   _winlist_act = eina_stringshare_add("winlist");
   /* add module supplied action */
   _act_winlist = e_action_add(_winlist_act);
   if (_act_winlist)
     {
        _act_winlist->func.go = _e_mod_action_winlist_cb;
        _act_winlist->func.go_mouse = _e_mod_action_winlist_mouse_cb;
        _act_winlist->func.go_key = _e_mod_action_winlist_key_cb;
        _act_winlist->func.go_edge = _e_mod_action_winlist_edge_cb;
        _act_winlist->func.go_signal = _e_mod_action_winlist_signal_cb;
        _act_winlist->func.go_acpi = _e_mod_action_winlist_acpi_cb;
        e_action_predef_name_set(N_("Window : List"), N_("Next Window"),
                                 "winlist", "next", NULL, 0);
        e_action_predef_name_set(N_("Window : List"), N_("Previous Window"),
                                 "winlist", "prev", NULL, 0);
        e_action_predef_name_set(N_("Window : List"),
                                 N_("Next window of same class"), "winlist",
                                 "class-next", NULL, 0);
        e_action_predef_name_set(N_("Window : List"),
                                 N_("Previous window of same class"),
                                 "winlist", "class-prev", NULL, 0);
        e_action_predef_name_set(N_("Window : List"),
                                 N_("Next window class"), "winlist",
                                 "classes-next", NULL, 0);
        e_action_predef_name_set(N_("Window : List"),
                                 N_("Previous window class"),
                                 "winlist", "classes-prev", NULL, 0);
        e_action_predef_name_set(N_("Window : List"), N_("Window on the Left"),
                                 "winlist", "left", NULL, 0);
        e_action_predef_name_set(N_("Window : List"), N_("Window Down"),
                                 "winlist", "down", NULL, 0);
        e_action_predef_name_set(N_("Window : List"), N_("Window Up"),
                                 "winlist", "up", NULL, 0);
        e_action_predef_name_set(N_("Window : List"), N_("Window on the Right"),
                                 "winlist", "right", NULL, 0);
     }
   e_module_delayed_set(m, 1);
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   E_Config_Dialog *cfd;

   /* remove module-supplied action */
   if (_act_winlist)
     {
        e_action_predef_name_del("Window : List", "Previous Window");
        e_action_predef_name_del("Window : List", "Next Window");
        e_action_predef_name_del("Window : List",
                                 "Previous window of same class");
        e_action_predef_name_del("Window : List",
                                 "Next window of same class");
        e_action_predef_name_del("Window : List", "Window on the Left");
        e_action_predef_name_del("Window : List", "Window Down");
        e_action_predef_name_del("Window : List", "Window Up");
        e_action_predef_name_del("Window : List", "Window on the Right");
        e_action_del("winlist");
        _act_winlist = NULL;
     }
   e_winlist_shutdown();

   while ((cfd = e_config_dialog_get("E", "windows/window_list")))
     e_object_del(E_OBJECT(cfd));
   e_configure_registry_item_del("windows/window_list");
   e_configure_registry_category_del("windows");
   conf_module = NULL;
   eina_stringshare_replace(&_winlist_act, NULL);
   return 1;
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   return 1;
}

/* action callback */
static void
_e_mod_action_winlist_cb_helper(E_Object *obj, const char *params, int modifiers, E_Winlist_Activate_Type type)
{
   E_Zone *zone = NULL;
   E_Winlist_Filter filter = E_WINLIST_FILTER_NONE;
   int direction = 0; // -1 for prev, 1 for next;
   int udlr = -1; // 0 for up, 1 for down, 2 for left, 3 for right
   Eina_Bool ok = EINA_TRUE;

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
   if (!zone) return;
   if (params)
     {
        if (!strcmp(params, "next"))
          direction = 1;
        else if (!strcmp(params, "prev"))
          direction = -1;
        else if (!strcmp(params, "class-next"))
          direction = 1, filter = E_WINLIST_FILTER_CLASS_WINDOWS;
        else if (!strcmp(params, "class-prev"))
          direction = -1, filter = E_WINLIST_FILTER_CLASS_WINDOWS;
        else if (!strcmp(params, "classes-next"))
          direction = 1, filter = E_WINLIST_FILTER_CLASSES;
        else if (!strcmp(params, "classes-prev"))
          direction = -1, filter = E_WINLIST_FILTER_CLASSES;
        else if (!strcmp(params, "up"))
          udlr = 0;
        else if (!strcmp(params, "down"))
          udlr = 1;
        else if (!strcmp(params, "left"))
          udlr = 2;
        else if (!strcmp(params, "right"))
          udlr = 3;
        else return;
     }
   else
     direction = 1;
   if (direction)
     ok = !e_winlist_show(zone, filter);
   if (!ok)
     {
        if (!type) return;
        if (!direction) return;
        e_winlist_modifiers_set(modifiers, type);
        return;
     }
   if (direction == 1)
     e_winlist_next();
   else if (direction == -1)
     e_winlist_prev();
   if (direction) return;
   switch (udlr)
     {
      case 0:
        e_winlist_up(zone);
        break;
      case 1:
        e_winlist_down(zone);
        break;
      case 2:
        e_winlist_left(zone);
        break;
      case 3:
        e_winlist_right(zone);
        break;
     }
}

static void
_e_mod_action_winlist_cb(E_Object *obj, const char *params)
{
   _e_mod_action_winlist_cb_helper(obj, params, 0, 0);
}

static void
_e_mod_action_winlist_mouse_cb(E_Object *obj, const char *params, Ecore_Event_Mouse_Button *ev)
{
   _e_mod_action_winlist_cb_helper(obj, params, ev->modifiers, E_WINLIST_ACTIVATE_TYPE_MOUSE);
}

static void
_e_mod_action_winlist_key_cb(E_Object *obj, const char *params, Ecore_Event_Key *ev)
{
   _e_mod_action_winlist_cb_helper(obj, params, ev->modifiers, E_WINLIST_ACTIVATE_TYPE_KEY);
}

static void
_e_mod_action_winlist_edge_cb(E_Object *obj EINA_UNUSED, const char *params EINA_UNUSED, E_Event_Zone_Edge *ev EINA_UNUSED)
{
   e_util_dialog_show(_("Winlist Error"), _("Winlist cannot be activated from an edge binding"));
}

static void
_e_mod_action_winlist_signal_cb(E_Object *obj EINA_UNUSED, const char *params EINA_UNUSED, const char *sig EINA_UNUSED, const char *src EINA_UNUSED)
{
   e_util_dialog_show(_("Winlist Error"), _("Winlist cannot be activated from a signal binding"));
}

static void
_e_mod_action_winlist_acpi_cb(E_Object *obj EINA_UNUSED, const char *params EINA_UNUSED, E_Event_Acpi *ev EINA_UNUSED)
{
   e_util_dialog_show(_("Winlist Error"), _("Winlist cannot be activated from an ACPI binding"));
}
