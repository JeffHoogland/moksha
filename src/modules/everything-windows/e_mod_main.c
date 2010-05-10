/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */

#include "Evry.h"
#include "e_mod_main.h"

#define BORDER_SHOW		1
#define BORDER_HIDE		2
#define BORDER_FULLSCREEN	3
#define BORDER_TODESK		4
#define BORDER_CLOSE		5

static Evry_Plugin *p1;
static Eina_List *handlers = NULL;
static Eina_Hash *border_hash = NULL;
static Eina_List *_actions = NULL;

/* static char _module_icon[] = "preferences-winlist"; */

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

   EVRY_PLUGIN_UPDATE(p, EVRY_UPDATE_ADD);

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

static Evas_Object *
_icon_get(Evry_Item *it, Evas *e)
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

   it = EVRY_ITEM_NEW(Evry_Item, p, e_border_name_get(bd), _icon_get, _item_free);

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

static int
_check_border(Evry_Action *act, const Evry_Item *it)
{
   int action = EVRY_ITEM_DATA_INT_GET(act);
   
   E_Border *bd = it->data;
   E_Zone *zone = e_util_zone_current_get(e_manager_current_get());

   switch (action)
     {
      case BORDER_CLOSE:
	 if (bd->lock_close)
	   return 0;
	 break;

      case BORDER_SHOW:
	 if (bd->lock_focus_in)
	   return 0;
	 break;

      case BORDER_HIDE:
	 if (bd->lock_user_iconify)
	   return 0;
	 break;

      case BORDER_FULLSCREEN:
	 if (!bd->lock_user_fullscreen)
	   return 0;
	 break;

      case BORDER_TODESK:
	 if (bd->desk == (e_desk_current_get(zone)))
	   return 0;
	 break;
     }

   return 1;
}

static int
_act_border(Evry_Action *act)
{
   int action = EVRY_ITEM_DATA_INT_GET(act);

   E_Border *bd = act->it1.item->data;
   E_Zone *zone = e_util_zone_current_get(e_manager_current_get());

   int focus = 0;

   switch (action)
     {
      case BORDER_CLOSE:
	e_border_act_close_begin(bd);
	break;

      case BORDER_SHOW:
	 if (bd->desk != (e_desk_current_get(zone)))
	   e_desk_show(bd->desk);
	 focus = 1;
	 break;

      case BORDER_HIDE:
	 e_border_iconify(bd);
	 break;
	 
      case BORDER_FULLSCREEN:
	 if (!bd->fullscreen)
	   e_border_fullscreen(bd, E_FULLSCREEN_RESIZE);
	 else
	   e_border_unfullscreen(bd);
	 break;
	 
      case BORDER_TODESK:
	 if (bd->desk != (e_desk_current_get(zone)))
	   e_border_desk_set(bd, e_desk_current_get(zone));
	 focus = 1;
	 break;
      default:
	 break;
     }

   if (focus)
     {
	if (bd->shaded)
	  e_border_unshade(bd, E_DIRECTION_UP);

	if (bd->iconic)
	  e_border_uniconify(bd);
	else
	  e_border_raise(bd);

	e_border_focus_set_with_pointer(bd);
     }

   return 1;
}


static Eina_Bool
_plugins_init(void)
{
   Evry_Action *act;

   if (!evry_api_version_check(EVRY_API_VERSION))
     return EINA_FALSE;

   p1 = EVRY_PLUGIN_NEW(Evry_Plugin, N_("Windows"), NULL, EVRY_TYPE_BORDER,
			_begin, _cleanup, _fetch, NULL);

   p1->transient = EINA_TRUE;
   evry_plugin_register(p1, EVRY_PLUGIN_SUBJECT, 2);


   act = EVRY_ACTION_NEW(_("Switch to Window"),
			 EVRY_TYPE_BORDER, 0, "go-next",
			 _act_border, _check_border);
   EVRY_ITEM_DATA_INT_SET(act, BORDER_SHOW);
   evry_action_register(act, 1);

   _actions = eina_list_append(_actions, act);

   act = EVRY_ACTION_NEW(_("Iconify"),
			 EVRY_TYPE_BORDER, 0, "iconic",
			 _act_border, _check_border);
   EVRY_ITEM_DATA_INT_SET(act, BORDER_HIDE);
   _actions = eina_list_append(_actions, act);
   evry_action_register(act, 2);

   act = EVRY_ACTION_NEW(_("Toggle Fullscreen"),
			 EVRY_TYPE_BORDER, 0, "view-fullscreen",
			 _act_border, _check_border);
   EVRY_ITEM_DATA_INT_SET(act, BORDER_FULLSCREEN);
   _actions = eina_list_append(_actions, act);
   evry_action_register(act, 4);

   act = EVRY_ACTION_NEW(_("Close"),
			 EVRY_TYPE_BORDER, 0, "view-fullscreen",
			 _act_border, _check_border);
   EVRY_ITEM_DATA_INT_SET(act, BORDER_CLOSE);
   _actions = eina_list_append(_actions, act);
   evry_action_register(act, 3);

   act = EVRY_ACTION_NEW(_("Send to Desktop"),
			 EVRY_TYPE_BORDER, 0, "go-previous",
			 _act_border, _check_border);
   EVRY_ITEM_DATA_INT_SET(act, BORDER_TODESK);
   _actions = eina_list_append(_actions, act);
   evry_action_register(act, 3);

   return EINA_TRUE;
}

static void
_plugins_shutdown(void)
{
   Evry_Action *act;

   EVRY_PLUGIN_FREE(p1);

   EINA_LIST_FREE(_actions, act)
     evry_action_free(act);
}

/***************************************************************************/

static E_Module *module = NULL;
static Eina_Bool active = EINA_FALSE;

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
     active = _plugins_init();

   e_module_delayed_set(m, 1);

   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   if (active && e_datastore_get("everything_loaded"))
     _plugins_shutdown();

   module = NULL;

   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   return 1;
}

/***************************************************************************/
