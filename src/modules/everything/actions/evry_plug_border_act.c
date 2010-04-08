#include "Evry.h"


typedef struct _Inst Inst;

struct _Inst
{
  E_Border *border;
};

static Evry_Plugin *plugin = NULL;
static Inst *inst = NULL;
static Evry_Action *act = NULL;

static void
_act_cb_border_switch_to(E_Border *bd)
{
   E_Zone *zone;

   zone = e_util_zone_current_get(e_manager_current_get());

   if (bd->desk != (e_desk_current_get(zone)))
     e_desk_show(bd->desk);

   if (bd->shaded)
     e_border_unshade(bd, E_DIRECTION_UP);

   if (bd->iconic)
     e_border_uniconify(bd);
   else
     e_border_raise(bd);

   /* e_border_focus_set(bd, 1, 1); */
   e_border_focus_set_with_pointer(bd);
}

static void
_act_cb_border_to_desktop(E_Border *bd)
{
   E_Zone *zone;
   E_Desk *desk;
   zone = e_util_zone_current_get(e_manager_current_get());
   desk = e_desk_current_get(zone);

   e_border_desk_set(bd, desk);

   if (bd->shaded)
     e_border_unshade(bd, E_DIRECTION_UP);

   if (bd->iconic)
     e_border_uniconify(bd);
   else
     e_border_raise(bd);

   /* e_border_focus_set(bd, 1, 1); */
   e_border_focus_set_with_pointer(bd);
}

static void
_act_cb_border_fullscreen(E_Border *bd)
{
   if (!bd->fullscreen)
     e_border_fullscreen(bd, E_FULLSCREEN_RESIZE);
   else
     e_border_unfullscreen(bd);
}

static void
_act_cb_border_close(E_Border *bd)
{
   if (!bd->lock_close) e_border_act_close_begin(bd);
}

static void
_act_cb_border_minimize(E_Border *bd)
{
   if (!bd->lock_user_iconify) e_border_iconify(bd);
}

static void
_act_cb_border_unminimize(E_Border *bd)
{
   if (!bd->lock_user_iconify) e_border_uniconify(bd);
}

static Evry_Plugin *
_begin(Evry_Plugin *p __UNUSED__, const Evry_Item *item)
{
   E_Border *bd;

   bd = item->data;
   /* e_object_ref(E_OBJECT(bd)); */
   inst->border = bd;

   return p;
}

static int
_cb_sort(const void *data1, const void *data2)
{
   const Evry_Item *it1 = data1;
   const Evry_Item *it2 = data2;

   return (it1->fuzzy_match - it2->fuzzy_match);
}

static void
_item_add(Evry_Plugin *p, const char *label, void (*action_cb) (E_Border *bd), const char *icon, const char *input)
{
   Evry_Item *it;
   int match = 1;

   if (input)
     match = evry_fuzzy_match(label, input);

   if (!match) return;

   it = evry_item_new(NULL, p, label, NULL);
   it->icon = eina_stringshare_add(icon);
   it->data = action_cb;
   it->fuzzy_match = match;

   p->items = eina_list_prepend(p->items, it);
}

static void
_cleanup(Evry_Plugin *p)
{
   EVRY_PLUGIN_ITEMS_FREE(p);
}

static int
_fetch(Evry_Plugin *p, const char *input)
{
   E_Zone *zone;
   E_Desk *desk;

   zone = e_util_zone_current_get(e_manager_current_get());
   desk = e_desk_current_get(zone);

   _cleanup(p);

   _item_add(p, _("Switch to Window"),
	     _act_cb_border_switch_to,
	     "go-next", input);

   if (desk != inst->border->desk)
     _item_add(p, _("Send to Deskop"),
	       _act_cb_border_to_desktop,
	       "go-previous", input);

   if (inst->border->iconic)
     _item_add(p, _("Uniconify"),
	       _act_cb_border_unminimize,
	       "window-minimize", input);
   else
     _item_add(p, _("Iconify"),
	       _act_cb_border_minimize,
	       "window-minimize", input);

   if (!inst->border->fullscreen)
     _item_add(p, _("Fullscreen"),
	       _act_cb_border_fullscreen,
	       "view-fullscreen", input);
   else
     _item_add(p, _("Unfullscreen"),
	       _act_cb_border_fullscreen,
	       "view-restore", input);

   _item_add(p, _("Close"),
	     _act_cb_border_close,
	     "window-close", input);

   if (!p->items) return 0;

   EVRY_PLUGIN_ITEMS_SORT(p, _cb_sort);

   return 1;
}

static int
_action(Evry_Plugin *p __UNUSED__, const Evry_Item *item)
{
   void (*border_action) (E_Border *bd);
   border_action = item->data;
   border_action(inst->border);

   return EVRY_ACTION_FINISHED;
}

static Evas_Object *
_item_icon_get(Evry_Plugin *p __UNUSED__, const Evry_Item *it, Evas *e)
{
   Evas_Object *o;

   o = evry_icon_theme_get(it->icon, e);

   return o;
}


static int
_exec_border_check_item(Evry_Action *act __UNUSED__, const Evry_Item *it)
{
   E_Border *bd = it->data;
   E_OBJECT_CHECK_RETURN(bd, 0);
   E_OBJECT_TYPE_CHECK_RETURN(bd, E_BORDER_TYPE, 0);

   if ((bd->desktop && bd->desktop->exec) &&
       ((strstr(bd->desktop->exec, "%u")) ||
	(strstr(bd->desktop->exec, "%U")) ||
	(strstr(bd->desktop->exec, "%f")) ||
	(strstr(bd->desktop->exec, "%F"))))
     return 1;

   return 0;
}

static int
_exec_border_action(Evry_Action *act)
{
   return evry_util_exec_app(act->item1, act->item2);
}

static int
_exec_border_intercept(Evry_Action *act)
{
   Evry_Item_App *app = E_NEW(Evry_Item_App, 1);
   E_Border *bd = act->item1->data;

   app->desktop = bd->desktop;
   act->item1 = EVRY_ITEM(app);

   return 1;
}


static void
_exec_border_cleanup(Evry_Action *act)
{
   ITEM_APP(app, act->item1);
   E_FREE(app);
}



static Eina_Bool
_init(void)
{
   if (!evry_api_version_check(EVRY_API_VERSION))
     return EINA_FALSE;

   plugin = evry_plugin_new(NULL, "Window Action", type_action, "BORDER", NULL, 0, NULL, NULL,
		       _begin, _cleanup, _fetch, _action, _item_icon_get, NULL, NULL);

   evry_plugin_register(plugin, 1);
   inst = E_NEW(Inst, 1);

   act = evry_action_new("Open File...", "BORDER", "FILE", "APPLICATION",
			  "everything-launch",
			  _exec_border_action, _exec_border_check_item,
			  _exec_border_cleanup, _exec_border_intercept, NULL);
   evry_action_register(act, 10);
   
   return EINA_TRUE;
}

static void
_shutdown(void)
{
   EVRY_PLUGIN_FREE(plugin);
   E_FREE(inst);
   
   evry_action_free(act);
}


EINA_MODULE_INIT(_init);
EINA_MODULE_SHUTDOWN(_shutdown);
