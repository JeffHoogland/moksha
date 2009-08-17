#include "Evry.h"


static Evry_Plugin *p1;
static Evas_Object *o_thumb[5];
static const Evry_Item *cur_item = NULL;
static Evas_Object *o_main = NULL;
static Evas_Object *o_swallow = NULL;
static Eina_List *cur_items = NULL;

static int
_check_item(const Evry_Item *it)
{
   if (strcmp(it->plugin->type_out, "FILE")) return 0;

   if (!it->uri || !it->mime) return 0;

   if (e_util_glob_case_match(it->mime, "*image/*"))
     return 1;

   return 0;
}

static void
_cb_preview_thumb_gen(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Coord w, h;

   if (!o_main) return;

   e_icon_size_get(o_thumb[1], &w, &h);
   edje_extern_object_min_size_set(o_thumb[1], w, h);
   edje_object_part_swallow(o_main, "e.swallow.icon2", o_thumb[1]);
   evas_object_show(o_thumb[1]);
}

void
_show_item(const Evry_Item *it, int dir)
{
   int w, h, x, y;

   if (o_thumb[1 + dir])
     {
	e_thumb_icon_end(o_thumb[1+dir]);
	edje_object_part_unswallow(o_main, o_thumb[1+dir]);
	evas_object_hide(o_thumb[1+dir]);
	evas_object_del(o_thumb[1+dir]);
     }

   if (dir && o_thumb[1])
     {
	edje_object_part_unswallow(o_main, o_thumb[1]);

	if (dir > 0)
	  {
	     o_thumb[2] = o_thumb[1];
	     edje_object_part_swallow(o_main, "e.swallow.icon1", o_thumb[2]);
	  }
	else
	  {
	     o_thumb[0] = o_thumb[1];
	     edje_object_part_swallow(o_main, "e.swallow.icon3", o_thumb[0]);
	  }
     }

   o_thumb[1] = e_thumb_icon_add(evas_object_evas_get(o_main));
   e_thumb_icon_file_set(o_thumb[1], it->uri, NULL);
   evas_object_smart_callback_add(o_thumb[1], "e_thumb_gen", _cb_preview_thumb_gen, NULL);
   evas_object_geometry_get(o_main, &x, &y, &w, &h);
   e_thumb_icon_size_set(o_thumb[1], w, h);
   e_thumb_icon_begin(o_thumb[1]);

   if (dir)
     {
	if (dir > 0)
	  edje_object_signal_emit(o_main, "e,signal,slide_left", "e");
	else
	  edje_object_signal_emit(o_main, "e,signal,slide_right", "e");
     }
}

static Evas_Object *
_show(Evry_Plugin *p, Evas_Object *swallow, Eina_List *items)
{
   int w, h, x, y;
   o_main = edje_object_add(evas_object_evas_get(swallow));
   e_theme_edje_object_set(o_main, "base/theme/everything",
			   "e/widgets/everything/preview");

   edje_object_part_geometry_get(swallow, "e.swallow.list", &x, &y, &w, &h);
   edje_extern_object_min_size_set(o_main, w * 3, 100);
   evas_object_resize(o_main, w * 3, h);

   if (!o_main) return NULL;
   o_swallow = swallow;

   _show_item(cur_item, 0);

   cur_items = items;

   return o_main;
}

static int
_cb_key_down(Evry_Plugin *p, Ecore_Event_Key *ev)
{
   Eina_List *l, *ll;
   const Evry_Item *found = NULL, *it;

   if (!strcmp(ev->key, "Right") ||
       !strcmp(ev->key, "Down"))
     {
	l = eina_list_data_find_list(cur_items, cur_item);

	if (l && l->next)
	  {
	     EINA_LIST_FOREACH(l->next, ll, it)
	       {
		  if (!_check_item(it)) continue;
		  found = it;
		  break;
	       }
	  }
	if (!found)
	  {
	     EINA_LIST_FOREACH(cur_items, ll, it)
	       {
		  if (!_check_item(it)) continue;
		  found = it;
		  break;
	       }
	  }

	if (found && (found != cur_item))
	  {
	     _show_item(found, 1);
	     cur_item = found;
	  }
	return 1;
     }
   else if (!strcmp(ev->key, "Left") ||
	    !strcmp(ev->key, "Up"))
     {
	EINA_LIST_FOREACH(cur_items, ll, it)
	  {
	     if (it == cur_item)
	       break;

	     if (_check_item(it))
	       found = it;
	  }
	if (!found && ll)
	  {
	     EINA_LIST_FOREACH(ll->next, l, it)
	       {
		  if (_check_item(it))
		    found = it;
	       }
	  }
	if (found && (found != cur_item))
	  {
	     _show_item(found, -1);
	     cur_item = found;
	  }
	return 1;
     }

   return 0;
}

static int
_begin(Evry_Plugin *p, const Evry_Item *it)
{
   if (_check_item(it))
     {
	cur_item = it;
	return 1;
     }

   return 0;
}

static void
_cleanup(Evry_Plugin *p)
{
   if (o_thumb[0]) evas_object_del(o_thumb[0]);
   if (o_thumb[1]) evas_object_del(o_thumb[1]);
   if (o_thumb[2]) evas_object_del(o_thumb[2]);
   if (o_main) evas_object_del(o_main);
   o_thumb[0] = NULL;
   o_thumb[1] = NULL;
   o_thumb[2] = NULL;
   o_main = NULL;
   cur_items = NULL;
}

static int
_action(Evry_Plugin *p, const Evry_Item *it, const char *input __UNUSED__)
{
   return 0;
}

static int
_fetch(Evry_Plugin *p, const char *input __UNUSED__)
{
   return 1;
}
static Eina_Bool
_init(void)
{
   p1 = E_NEW(Evry_Plugin, 1);
   p1->name = "Preview";
   p1->type = type_action;
   p1->type_in  = "FILE";
   p1->type_out = "NONE";
   p1->icon = "preferences-desktop-wallpaper";
   p1->need_query = 0;
   p1->async_query = 1;
   p1->begin = &_begin;
   p1->fetch = &_fetch;
   p1->show = &_show;
   p1->cb_key_down = &_cb_key_down;
   p1->action = &_action;
   p1->cleanup = &_cleanup;
   evry_plugin_register(p1, 3);

   return EINA_TRUE;
}

static void
_shutdown(void)
{
   evry_plugin_unregister(p1);
   E_FREE(p1);
}


EINA_MODULE_INIT(_init);
EINA_MODULE_SHUTDOWN(_shutdown);
