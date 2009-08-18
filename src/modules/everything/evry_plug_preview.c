#include "Evry.h"


static Evry_View *view = NULL;
static Evas_Object *o_thumb[5];
static Evas_Object *o_main = NULL;
static Eina_List *items = NULL;
static const char *view_types;

static int
_check_item(const Evry_Item *it)
{
   if (!it) return 0;

   if (it->plugin->type_out != view_types) return 0;

   if (!it->uri || !it->mime) return 0;

   if (!strncmp(it->mime, "image/", 6))
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
   /* evas_object_size_hint_aspect_set(o_thumb[1], EVAS_ASPECT_CONTROL_HORIZONTAL, w, h); */
   evas_object_show(o_thumb[1]);
}

void
_show_item(const Evry_Item *it, int dir)
{
   int w, h;

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
   /* evas_object_geometry_get(o_main, &x, &y, &w, &h); */
   edje_object_part_geometry_get(o_main, "e.swallow.icon2", NULL, NULL, &w, &h);
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

static int
_cb_key_down(Evry_View *v, const Ecore_Event_Key *ev)
{
   Eina_List *l;
   Evry_Item *it = NULL, *cur_item;

   cur_item  = view->state->sel_item;

   if (!strcmp(ev->key, "Down"))
     {
	if (!items) return 1;
	
	l = eina_list_data_find_list(items, cur_item);

	if (l && l->next)
	  it = l->next->data;
	else    
	  it = items->data;
	
	if (it && (it != cur_item))
	  {
	     _show_item(it, 1);
	     evry_item_select(view->state, it);
	  }
	return 1;
     }
   else if (!strcmp(ev->key, "Up"))
     {
	if (!items) return 1;
	
	l = eina_list_data_find_list(items, cur_item);

	if (l && l->prev)
	  it = l->prev->data;
	else    
	  it = eina_list_last(items)->data;
	
	if (it && (it != cur_item))
	  {
	     _show_item(it, -1);
	     evry_item_select(view->state, it);
	  }
	return 1;
     }

   return 0;
}

static void
_clear(Evry_View *v, const Evry_State *s)
{
   if (o_thumb[0]) evas_object_del(o_thumb[0]);
   if (o_thumb[1]) evas_object_del(o_thumb[1]);
   if (o_thumb[2]) evas_object_del(o_thumb[2]);
   o_thumb[0] = NULL;
   o_thumb[1] = NULL;
   o_thumb[2] = NULL;
}

static void
_cleanup(Evry_View *v)
{
   _clear(v, NULL);
   
   if (o_main) evas_object_del(o_main);
   o_main = NULL;
}

static Evry_Item *
_find_first(const Evry_State *s)
{
   Eina_List *l;
   Evry_Item *it, *found = NULL;

   eina_list_free(items);
   items = NULL;
   
   EINA_LIST_FOREACH(s->plugin->items, l, it)
     {
	if (!_check_item(it)) continue;
	if (!found) found = it;
	items = eina_list_append(items, it);
     }

   if (_check_item(s->sel_item))
     return s->sel_item;

   return found;
}

static int
_update(Evry_View *v, const Evry_State *s)
{
   Evry_Item *it;

   v->state = s;
   it = _find_first(s);

   if (!it) return 0;

   _show_item(it, 0);
   evry_item_select(view->state, it);

   return 1;
}

static Evas_Object *
_begin(Evry_View *v, const Evry_State *s, const Evas_Object *swallow)
{
   int w, h, x, y;
   Evry_Item *it;

   if (!(it = _find_first(s))) return NULL;

   o_main = edje_object_add(evas_object_evas_get(swallow));
   e_theme_edje_object_set(o_main, "base/theme/everything",
			   "e/widgets/everything/preview");

   edje_object_part_geometry_get(swallow, "e.swallow.list", &x, &y, &w, &h);
   edje_extern_object_min_size_set(o_main, w * 3, 100);
   evas_object_resize(o_main, w * 3, h);

   if (!o_main) return NULL;

   view->state = s;

   _show_item(it, 0);

   return o_main;
}

static Eina_Bool
_init(void)
{
   view = E_NEW(Evry_View, 1);
   view->name = "Image Preview";
   view->begin = &_begin;
   view->update = &_update;
   view->clear = &_clear;
   view->cb_key_down = &_cb_key_down;
   view->cleanup = &_cleanup;
   evry_view_register(view, 2);

   view_types = eina_stringshare_add("FILE");
   
   return EINA_TRUE;
}

static void
_shutdown(void)
{
   eina_stringshare_del(view_types); 
   evry_view_unregister(view);
   E_FREE(view);
}


EINA_MODULE_INIT(_init);
EINA_MODULE_SHUTDOWN(_shutdown);
