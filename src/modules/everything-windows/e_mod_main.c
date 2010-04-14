/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */

#include "Evry.h"
#include "e_mod_main.h"

static Evry_Plugin *plugin;
static Eina_List *handlers = NULL;
static Eina_Hash *border_hash = NULL;

static int
_cb_border_remove(void *data, int type,  void *event)
{
   E_Event_Border_Remove *ev = event;
   Evry_Item *it;
   Evry_Plugin *p = data;

   it = eina_hash_find(border_hash, &(ev->border));

   if (!it) return 1;

   p->items = eina_list_remove(p->items, it);
   eina_hash_del_by_key(border_hash, &(ev->border)); 

   evry_plugin_async_update(p, EVRY_ASYNC_UPDATE_ADD);

   return 1;
}

static void _hash_free(void *data)
{
   Evry_Item *it = data;
   evry_item_free(it);
}

static Evry_Plugin *
_begin(Evry_Plugin *p, const Evry_Item *it)
{
   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (E_EVENT_BORDER_REMOVE, _cb_border_remove, p));

   border_hash = eina_hash_pointer_new(_hash_free); 
   
   return p;
}

static void
_cleanup(Evry_Plugin *p)
{
   Ecore_Event_Handler *h;
      
   EINA_LIST_FREE(handlers, h)
     ecore_event_handler_del(h);

   if (border_hash) eina_hash_free(border_hash);
   border_hash = NULL;
   
   EVRY_PLUGIN_ITEMS_CLEAR(p);
}

static void
_item_free(Evry_Item *it)
{
   if (it->data)
     e_object_unref(E_OBJECT(it->data));

   E_FREE(it);
}

static void
_item_add(Evry_Plugin *p, E_Border *bd, int match, int *prio)
{
   Evry_Item *it = NULL;

   if ((it = eina_hash_find(border_hash, &bd)))
     {
	it->priority = *prio;
	EVRY_PLUGIN_ITEM_APPEND(p, it);
	it->fuzzy_match = match;
	*prio += 1;
	return;
     }
   
   it = evry_item_new(NULL, p, e_border_name_get(bd), _item_free);

   e_object_ref(E_OBJECT(bd));
   it->data = bd;
   it->fuzzy_match = match;
   it->priority = *prio;
   it->id = eina_stringshare_add(e_util_winid_str_get(bd->win));
   
   *prio += 1;

   eina_hash_add(border_hash, &bd, it);

   EVRY_PLUGIN_ITEM_APPEND(p, it);
}

static int
_cb_sort(const void *data1, const void *data2)
{
   const Evry_Item *it1 = data1;
   const Evry_Item *it2 = data2;

   if (it1->fuzzy_match - it2->fuzzy_match)
     return (it1->fuzzy_match - it2->fuzzy_match);

   return (it1->priority - it2->priority);
}

static int
_fetch(Evry_Plugin *p, const char *input)
{
   E_Zone *zone;
   E_Border *bd;
   Eina_List *l;
   int prio = 0;
   int m1, m2;

   EVRY_PLUGIN_ITEMS_CLEAR(p);

   zone = e_util_zone_current_get(e_manager_current_get());

   EINA_LIST_FOREACH(e_border_focus_stack_get(), l, bd)
     {
	if (zone == bd->zone)
	  {
	     if (!input)
	       _item_add(p, bd, 0, &prio);
	     else
	       {
		  m1 = evry_fuzzy_match(e_border_name_get(bd), input);

		  if (bd->client.icccm.name)
		    {
		       m2 = evry_fuzzy_match(bd->client.icccm.name, input);
		       if (!m1 || (m2 && m2 < m1))
			 m1 = m2;
		    }

		  if (bd->desktop)
		    {
		       m2 = evry_fuzzy_match(bd->desktop->name, input);
		       if (!m1 || (m2 && m2 < m1))
			 m1 = m2;
		    }

		  if (m1)
		    _item_add(p, bd, m1, &prio);
	       }
	  }
     }

   if (!p->items) return 0;
   
   EVRY_PLUGIN_ITEMS_SORT(p, _cb_sort);

   return 1;
}

static Evas_Object *
_item_icon_get(Evry_Plugin *p __UNUSED__, const Evry_Item *it, Evas *e)
{
   Evas_Object *o = NULL;
   E_Border *bd = it->data;

   if (bd->internal)
     {
	o = edje_object_add(e);
	if (!bd->internal_icon)
	  e_util_edje_icon_set(o, "enlightenment/e");
	else
	  {
	     if (!bd->internal_icon_key)
	       {
		  char *ext;
		  ext = strrchr(bd->internal_icon, '.');
		  if ((ext) && ((!strcmp(ext, ".edj"))))
		    {
		       if (!edje_object_file_set(o, bd->internal_icon, "icon"))
			 e_util_edje_icon_set(o, "enlightenment/e");
		    }
		  else if (ext)
		    {
		       evas_object_del(o);
		       o = e_icon_add(e);
		       e_icon_file_set(o, bd->internal_icon);
		    }
		  else
		    {
		       if (!e_util_edje_icon_set(o, bd->internal_icon))
			 e_util_edje_icon_set(o, "enlightenment/e");
		    }
	       }
	     else
	       {
		  edje_object_file_set(o, bd->internal_icon,
				       bd->internal_icon_key);
	       }
	  }
	return o;
     }

   if (!o && bd->desktop)
     o = e_util_desktop_icon_add(bd->desktop, 128, e);

   if (!o && bd->client.netwm.icons)
     {
	int i, size, tmp, found = 0;
	o = e_icon_add(e);

	size = bd->client.netwm.icons[0].width;

	for (i = 1; i < bd->client.netwm.num_icons; i++)
	  {
	     if ((tmp = bd->client.netwm.icons[i].width) > size)
	       {
		  size = tmp;
		  found = i;
	       }
	  }

	e_icon_data_set(o, bd->client.netwm.icons[found].data,
			bd->client.netwm.icons[found].width,
			bd->client.netwm.icons[found].height);
	e_icon_alpha_set(o, 1);
	return o;
     }

   if (!o)
     o = e_border_icon_add(bd, e);

   return o;
}


/********* border actions ********/

typedef struct _Inst Inst;
struct _Inst
{
  E_Border *border;
};

static Evry_Plugin *plugin2 = NULL;
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
_act_begin(Evry_Plugin *p __UNUSED__, const Evry_Item *item)
{
   E_Border *bd;

   bd = item->data;
   /* e_object_ref(E_OBJECT(bd)); */
   inst->border = bd;

   return p;
}

static int
_act_cb_sort(const void *data1, const void *data2)
{
   const Evry_Item *it1 = data1;
   const Evry_Item *it2 = data2;

   return (it1->fuzzy_match - it2->fuzzy_match);
}

static void
_act_item_add(Evry_Plugin *p, const char *label, void (*action_cb) (E_Border *bd), const char *icon, const char *input)
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
_act_cleanup(Evry_Plugin *p)
{
   EVRY_PLUGIN_ITEMS_FREE(p);
}

static int
_act_fetch(Evry_Plugin *p, const char *input)
{
   E_Zone *zone;
   E_Desk *desk;

   zone = e_util_zone_current_get(e_manager_current_get());
   desk = e_desk_current_get(zone);

   _act_cleanup(p);

   _act_item_add(p, _("Switch to Window"),
		 _act_cb_border_switch_to,
		 "go-next", input);

   if (desk != inst->border->desk)
     _act_item_add(p, _("Send to Deskop"),
		   _act_cb_border_to_desktop,
		   "go-previous", input);

   if (inst->border->iconic)
     _act_item_add(p, _("Uniconify"),
		   _act_cb_border_unminimize,
		   "window-minimize", input);
   else
     _act_item_add(p, _("Iconify"),
		   _act_cb_border_minimize,
		   "window-minimize", input);

   if (!inst->border->fullscreen)
     _act_item_add(p, _("Fullscreen"),
		   _act_cb_border_fullscreen,
		   "view-fullscreen", input);
   else
     _act_item_add(p, _("Unfullscreen"),
		   _act_cb_border_fullscreen,
		   "view-restore", input);

   _act_item_add(p, _("Close"),
		 _act_cb_border_close,
		 "window-close", input);

   if (!p->items) return 0;

   EVRY_PLUGIN_ITEMS_SORT(p, _act_cb_sort);

   return 1;
}

static int
_act_action(Evry_Plugin *p __UNUSED__, const Evry_Item *item)
{
   void (*border_action) (E_Border *bd);
   border_action = item->data;
   border_action(inst->border);

   return EVRY_ACTION_FINISHED;
}

static Evas_Object *
_act_item_icon_get(Evry_Plugin *p __UNUSED__, const Evry_Item *it, Evas *e)
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
module_init(void)
{
   if (!evry_api_version_check(EVRY_API_VERSION))
     return EINA_FALSE;

   plugin = evry_plugin_new(NULL, "Windows", type_subject, NULL, "BORDER", 0, NULL, NULL,
			    _begin, _cleanup, _fetch, NULL, _item_icon_get, NULL);
   plugin->transient = EINA_TRUE;
   evry_plugin_register(plugin, 2);

   
   plugin2 = evry_plugin_new(NULL, "Window Action", type_action, "BORDER", NULL, 0, NULL, NULL,
			     _act_begin, _act_cleanup, _act_fetch, _act_action, _act_item_icon_get, NULL);
   evry_plugin_register(plugin2, 1);

   act = evry_action_new("Open File...", "BORDER", "FILE", "APPLICATION",
			 "everything-launch",
			 _exec_border_action, _exec_border_check_item,
			 _exec_border_cleanup, _exec_border_intercept, NULL, NULL);
   evry_action_register(act, 10);

   return EINA_TRUE;
}

static void
module_shutdown(void)
{
   EVRY_PLUGIN_FREE(plugin);
   EVRY_PLUGIN_FREE(plugin2);

   evry_action_free(act);
}

/***************************************************************************/
/**/
/* actual module specifics */

static E_Module *module = NULL;
static Eina_Bool active = EINA_FALSE;

/***************************************************************************/
/**/
/* module setup */
EAPI E_Module_Api e_modapi = 
{
   E_MODULE_API_VERSION,
   "everything-windows"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   module = m;

   if (e_datastore_get("everything_loaded"))
     active = module_init();
   
   e_module_delayed_set(m, 1); 

   inst = E_NEW(Inst, 1);
   
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   if (active && e_datastore_get("everything_loaded"))
     module_shutdown();

   E_FREE(inst);
   
   module = NULL;
   
   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   return 1;
}

/**/
/***************************************************************************/

