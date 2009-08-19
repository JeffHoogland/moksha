#include "Evry.h"

typedef struct _Image_View Image_View;


struct _Image_View
{
  Evry_View view;
  Evas *evas;

  const Evry_State *state;

  Evas_Object *o_main;

  Eina_List *items;
  Evas_Object *o_thumb[4];
};


static Evry_View *view = NULL;
static const char *view_types = NULL;

static int
_check_item(const Evry_Item *it)
{
   if ((!it || !it->uri || !it->mime) ||
       (it->plugin->type_out != view_types))
     return 0;

   if (!strncmp(it->mime, "image/", 6))
     return 1;
   
   return 0;
}

static void
_cb_preview_thumb_gen(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Coord w, h;
   Image_View *v = data;
   
   e_icon_size_get(v->o_thumb[1], &w, &h);
   edje_extern_object_min_size_set(v->o_thumb[1], w, h);
   edje_object_part_swallow(v->o_main, "e.swallow.icon2", v->o_thumb[1]);
   evas_object_show(v->o_thumb[1]);
}

void
_show_item(Image_View *v, const Evry_Item *it, int dir)
{
   int w, h;

   if (v->o_thumb[1 + dir])
     {
	e_thumb_icon_end(v->o_thumb[1+dir]);
	edje_object_part_unswallow(v->o_main, v->o_thumb[1+dir]);
	evas_object_hide(v->o_thumb[1+dir]);
	evas_object_del(v->o_thumb[1+dir]);
     }

   if (dir && v->o_thumb[1])
     {
	edje_object_part_unswallow(v->o_main, v->o_thumb[1]);

	if (dir > 0)
	  {
	     v->o_thumb[2] = v->o_thumb[1];
	     edje_object_part_swallow(v->o_main, "e.swallow.icon1", v->o_thumb[2]);
	  }
	else
	  {
	     v->o_thumb[0] = v->o_thumb[1];
	     edje_object_part_swallow(v->o_main, "e.swallow.icon3", v->o_thumb[0]);
	  }
     }

   v->o_thumb[1] = e_thumb_icon_add(v->evas);
   e_thumb_icon_file_set(v->o_thumb[1], it->uri, NULL);
   evas_object_smart_callback_add(v->o_thumb[1], "e_thumb_gen", _cb_preview_thumb_gen, v);
   edje_object_part_geometry_get(v->o_main, "e.swallow.icon2", NULL, NULL, &w, &h);
   e_thumb_icon_size_set(v->o_thumb[1], w, h);
   e_thumb_icon_begin(v->o_thumb[1]);

   if (dir)
     {
	if (dir > 0)
	  edje_object_signal_emit(v->o_main, "e,signal,slide_left", "e");
	else
	  edje_object_signal_emit(v->o_main, "e,signal,slide_right", "e");
     }
}

static int
_cb_key_down(const Evry_View *view, const Ecore_Event_Key *ev)
{
   Image_View *v = (Image_View *) view;
   
   Eina_List *l;
   Evry_Item *it = NULL, *cur_item;

   cur_item  = v->state->sel_item;

   if (!strcmp(ev->key, "Down"))
     {
	if (!v->items) return 1;
	
	l = eina_list_data_find_list(v->items, cur_item);

	if (l && l->next)
	  it = l->next->data;
	else    
	  it = v->items->data;
	
	if (it && (it != cur_item))
	  {
	     _show_item(v, it, 1);
	     evry_item_select(v->state, it);
	  }
	return 1;
     }
   else if (!strcmp(ev->key, "Up"))
     {
	if (!v->items) return 1;
	
	l = eina_list_data_find_list(v->items, cur_item);

	if (l && l->prev)
	  it = l->prev->data;
	else    
	  it = eina_list_last(v->items)->data;
	
	if (it && (it != cur_item))
	  {
	     _show_item(v, it, -1);
	     evry_item_select(v->state, it);
	  }
	return 1;
     }

   return 0;
}

static void
_view_clear(const Evry_View *view)
{
   Image_View *v = (Image_View *) view;
   
   if (v->o_thumb[0]) evas_object_del(v->o_thumb[0]);
   v->o_thumb[0] = NULL;
   if (v->o_thumb[1]) evas_object_del(v->o_thumb[1]);
   v->o_thumb[1] = NULL;
   if (v->o_thumb[2]) evas_object_del(v->o_thumb[2]);
   v->o_thumb[2] = NULL;
   if (v->items) eina_list_free(v->items); 
   v->items = NULL;
}

static Eina_List *
_get_list(const Evry_State *s)
{
   Eina_List *l, *items = NULL;
   Evry_Item *it;
   
   EINA_LIST_FOREACH(s->plugin->items, l, it)
     if (_check_item(it)) 
       items = eina_list_append(items, it);

   return items;
}

static int
_view_update(const Evry_View *view)
{
   Image_View *v = (Image_View *) view;
   Evry_Item *it;

   v->items = _get_list(v->state);
   if (!v->items) return 0;

   it = eina_list_data_find(v->items, v->state->sel_item);
   if (!it)
     {
	it = v->items->data;
	evry_item_select(v->state, it);
     }
   
   _show_item(v, it, 0);
   
   return 1;
}

static Evas_Object *
_view_object(const Evry_View *view)
{
   return ((Image_View*) view)->o_main;
}

static Evry_View *
_view_create(const Evry_View *view, const Evry_State *s, const Evas_Object *swallow)
{
   Image_View *v;
   int w, h, x, y;

   if (!_get_list(s))
     return NULL;

   v = E_NEW(Image_View, 1);
   v->view = *view;
   v->state = s;   
   v->evas = evas_object_evas_get(swallow);
   v->o_main = edje_object_add(v->evas);
   e_theme_edje_object_set(v->o_main, "base/theme/everything",
			   "e/widgets/everything/preview");

   edje_object_part_geometry_get(swallow, "e.swallow.list", &x, &y, &w, &h);
   edje_extern_object_min_size_set(v->o_main, w * 3, 100);
   evas_object_resize(v->o_main, w * 3, h);

   return &v->view;
}

static void
_view_destroy(const Evry_View *view)
{
   Image_View *v = (Image_View *) view;

   _view_clear(view);
   evas_object_del(v->o_main);

   E_FREE(v);
}

static Eina_Bool
_init(void)
{
   view = E_NEW(Evry_View, 1);
   view->id = view;
   view->name = "Image Viewer";
   view->create = &_view_create;
   view->destroy = &_view_destroy;
   view->object = &_view_object;
   view->update = &_view_update;
   view->clear = &_view_clear;
   view->cb_key_down = &_cb_key_down;
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
