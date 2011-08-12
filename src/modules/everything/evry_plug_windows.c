#include "e.h"
#include "evry_api.h"

#define BORDER_SHOW		1
#define BORDER_HIDE		2
#define BORDER_FULLSCREEN	3
#define BORDER_TODESK		4
#define BORDER_CLOSE		5

typedef struct _Plugin Plugin;
typedef struct _Border_Item Border_Item;

struct _Plugin
{
  Evry_Plugin base;
  Eina_List *borders;
  Eina_List *handlers;
  const char *input;
};

struct _Border_Item
{
  Evry_Item base;
  E_Border *border;
};

#define GET_BORDER(_bd, _it) Border_Item *_bd = (Border_Item *)_it;

static const Evry_API *evry = NULL;
static Evry_Module *evry_module = NULL;
static Evry_Plugin *_plug;
static Eina_List *_actions = NULL;

static Evas_Object *_icon_get(Evry_Item *it, Evas *e);

/***************************************************************************/

static void
_border_item_free(Evry_Item *it)
{
   GET_BORDER(bi, it);

   e_object_unref(E_OBJECT(bi->border));

   E_FREE(bi);
}

static int
_border_item_add(Plugin *p, E_Border *bd)
{
   Border_Item *bi;
   char buf[1024];

   if (bd->client.netwm.state.skip_taskbar)
     return 0;
   if (bd->client.netwm.state.skip_pager)
     return 0;

   bi = EVRY_ITEM_NEW(Border_Item, p, e_border_name_get(bd), _icon_get, _border_item_free);
   snprintf(buf, sizeof(buf), "%d:%d %s",
	    bd->desk->x, bd->desk->y,
	    (bd->desktop ? bd->desktop->name : ""));
   EVRY_ITEM_DETAIL_SET(bi, buf);

   bi->border = bd;
   e_object_ref(E_OBJECT(bd));

   p->borders = eina_list_append(p->borders, bi);

   return 1;
}

static Eina_Bool
_cb_border_remove(void *data, __UNUSED__ int type,  void *event)
{
   E_Event_Border_Remove *ev = event;
   Border_Item *bi;
   Eina_List *l;
   Plugin *p = data;

   EINA_LIST_FOREACH(p->borders, l, bi)
     if (bi->border == ev->border)
       break;

   if (!bi) return ECORE_CALLBACK_PASS_ON;

   p->borders = eina_list_remove(p->borders, bi);
   p->base.items = eina_list_remove(p->base.items, bi);
   EVRY_ITEM_FREE(bi);

   EVRY_PLUGIN_UPDATE(p, EVRY_UPDATE_ADD);

   return ECORE_CALLBACK_PASS_ON;
}
static Eina_Bool
_cb_border_add(void *data, __UNUSED__ int type,  void *event)
{
   E_Event_Border_Add *ev = event;
   Plugin *p = data;
   unsigned int min;

   if (!_border_item_add(p, ev->border))
     return ECORE_CALLBACK_PASS_ON;

   EVRY_PLUGIN_ITEMS_CLEAR(p);

   min = EVRY_PLUGIN(p)->config->min_query;

   if ((!p->input && (min == 0)) ||
       (p->input && (strlen(p->input) >= min)))
     {
	EVRY_PLUGIN_ITEMS_ADD(p, p->borders, p->input, 1, 0);

	EVRY_PLUGIN_UPDATE(p, EVRY_UPDATE_ADD);
     }

   return ECORE_CALLBACK_PASS_ON;
}

static void
_get_borderlist(Plugin *p)
{
   E_Border *bd;
   Eina_List *l;

   p->handlers = eina_list_append
     (p->handlers, ecore_event_handler_add
      (E_EVENT_BORDER_REMOVE, _cb_border_remove, p));

   p->handlers = eina_list_append
     (p->handlers, ecore_event_handler_add
      (E_EVENT_BORDER_ADD, _cb_border_add, p));

   EINA_LIST_FOREACH(e_border_focus_stack_get(), l, bd)
     _border_item_add(p, bd);
}

static Evry_Plugin *
_begin(Evry_Plugin *plugin, const Evry_Item *item __UNUSED__)
{
   Plugin *p;
   EVRY_PLUGIN_INSTANCE(p, plugin);

   return EVRY_PLUGIN(p);
}

static void
_finish(Evry_Plugin *plugin)
{
   Ecore_Event_Handler *h;
   Border_Item *bi;

   GET_PLUGIN(p, plugin);

   IF_RELEASE(p->input);

   EVRY_PLUGIN_ITEMS_CLEAR(p);

   EINA_LIST_FREE(p->borders, bi)
     EVRY_ITEM_FREE(bi);

   EINA_LIST_FREE(p->handlers, h)
     ecore_event_handler_del(h);

   E_FREE(p);
}

static int
_fetch(Evry_Plugin *plugin, const char *input)
{
   int len = (input ? strlen(input) : 0);

   GET_PLUGIN(p, plugin);

   EVRY_PLUGIN_ITEMS_CLEAR(p);

   if (len >= plugin->config->min_query)
     {
	IF_RELEASE(p->input);

	if (input)
	  p->input = eina_stringshare_add(input);

	if (!p->handlers)
	  _get_borderlist(p);

	EVRY_PLUGIN_ITEMS_ADD(p, p->borders, input, 1, 0);
     }

   return !!(p->base.items);
}

static Evas_Object *
_icon_get(Evry_Item *it, Evas *e)
{
   GET_BORDER(bi, it);

   Evas_Object *o = NULL;
   E_Border *bd = bi->border;

   if (bd->internal)
     {
	o = edje_object_add(e);
	if (!bd->internal_icon)
	  e_util_edje_icon_set(o, "enlightenment/e");
	else if (!bd->internal_icon_key)
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


/***************************************************************************/

static int
_check_border(Evry_Action *act, const Evry_Item *it)
{
   GET_BORDER(bi, it);

   int action = EVRY_ITEM_DATA_INT_GET(act);
   E_Border *bd = bi->border;
   E_Zone *zone = e_util_zone_current_get(e_manager_current_get());

   if (!bd)
     {
	ERR("no border");
	return 0;
     }

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
   GET_BORDER(bi, act->it1.item);

   int action = EVRY_ITEM_DATA_INT_GET(act);
   E_Border *bd = bi->border;
   E_Zone *zone = e_util_zone_current_get(e_manager_current_get());
   int focus = 0;

   if (!bd)
     {
	ERR("no border");
	return 0;
     }

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

	if (!bd->lock_focus_out)
	  {
	     e_border_focus_set(bd, 1, 1);
	     e_border_focus_latest_set(bd);
	  }

	if ((e_config->focus_policy != E_FOCUS_CLICK) ||
	    (e_config->winlist_warp_at_end) ||
	    (e_config->winlist_warp_while_selecting))
	  {
	     int warp_to_x = bd->x + (bd->w / 2);
	     if (warp_to_x < (bd->zone->x + 1))
	       warp_to_x = bd->zone->x + ((bd->x + bd->w - bd->zone->x) / 2);
	     else if (warp_to_x >= (bd->zone->x + bd->zone->w - 1))
	       warp_to_x = (bd->zone->x + bd->zone->w + bd->x) / 2;

	     int warp_to_y = bd->y + (bd->h / 2);
	     if (warp_to_y < (bd->zone->y + 1))
	       warp_to_y = bd->zone->y + ((bd->y + bd->h - bd->zone->y) / 2);
	     else if (warp_to_y >= (bd->zone->y + bd->zone->h - 1))
	       warp_to_y = (bd->zone->y + bd->zone->h + bd->y) / 2;

	     ecore_x_pointer_warp(bd->zone->container->win, warp_to_x, warp_to_y);
	  }
	/* e_border_focus_set_with_pointer(bd); */
     }

   return 1;
}

static int
_plugins_init(const Evry_API *_api)
{
   Evry_Action *act;

   evry = _api;

   if (!evry->api_version_check(EVRY_API_VERSION))
     return EINA_FALSE;

   _plug = EVRY_PLUGIN_BASE("Windows", "preferences-system-windows",
			    EVRY_TYPE_BORDER, _begin, _finish, _fetch);
   _plug->transient = EINA_TRUE;
   evry->plugin_register(_plug, EVRY_PLUGIN_SUBJECT, 2);

   act = EVRY_ACTION_NEW(_("Switch to Window"),
			 EVRY_TYPE_BORDER, 0, "go-next",
			 _act_border, _check_border);
   EVRY_ITEM_DATA_INT_SET(act, BORDER_SHOW);
   evry->action_register(act, 1);

   _actions = eina_list_append(_actions, act);

   act = EVRY_ACTION_NEW(_("Iconify"),
			 EVRY_TYPE_BORDER, 0, "go-down",
			 _act_border, _check_border);
   EVRY_ITEM_DATA_INT_SET(act, BORDER_HIDE);
   _actions = eina_list_append(_actions, act);
   evry->action_register(act, 2);

   act = EVRY_ACTION_NEW(_("Toggle Fullscreen"),
			 EVRY_TYPE_BORDER, 0, "view-fullscreen",
			 _act_border, _check_border);
   EVRY_ITEM_DATA_INT_SET(act, BORDER_FULLSCREEN);
   _actions = eina_list_append(_actions, act);
   evry->action_register(act, 4);

   act = EVRY_ACTION_NEW(_("Close"),
			 EVRY_TYPE_BORDER, 0, "list-remove",
			 _act_border, _check_border);
   EVRY_ITEM_DATA_INT_SET(act, BORDER_CLOSE);
   _actions = eina_list_append(_actions, act);
   evry->action_register(act, 3);

   act = EVRY_ACTION_NEW(_("Send to Desktop"),
			 EVRY_TYPE_BORDER, 0, "go-previous",
			 _act_border, _check_border);
   EVRY_ITEM_DATA_INT_SET(act, BORDER_TODESK);
   _actions = eina_list_append(_actions, act);
   evry->action_register(act, 3);

   return EINA_TRUE;
}

static void
_plugins_shutdown(void)
{
   Evry_Action *act;

   EVRY_PLUGIN_FREE(_plug);

   EINA_LIST_FREE(_actions, act)
     EVRY_ACTION_FREE(act);
}

/***************************************************************************/

Eina_Bool
evry_plug_windows_init(E_Module *m)
{
   EVRY_MODULE_NEW(evry_module, evry, _plugins_init, _plugins_shutdown);

   return EINA_TRUE;
}

void
evry_plug_windows_shutdown(void)
{
   EVRY_MODULE_FREE(evry_module);
}

void
evry_plug_windows_save(void){}
