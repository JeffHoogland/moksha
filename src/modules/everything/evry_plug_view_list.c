#include "Evry.h"

typedef struct _List_View List_View;


struct _List_View
{
  Evas *evas;

  Evas_Object *o_list;

  int          ev_last_is_mouse;
  Evry_Item   *item_mouseover;
  Ecore_Animator *scroll_animator;
  Ecore_Timer *scroll_timer;
  double       scroll_align_to;
  double       scroll_align;
  Ecore_Idler *item_idler;

  Eina_List *items;
};

static void _list_item_next(const Evry_State *s);
static void _list_item_prev(const Evry_State *s);
static void _list_item_first(const Evry_State *s);
static void _list_item_last(const Evry_State *s);
static void _list_scroll_to(Evry_Item *it);
/* static int  _list_animator(void *data);
 * static int  _list_scroll_timer(void *data); */
static int  _list_item_idler(void *data);
static void _list_clear_list(const Evry_State *s);

static Evry_View *view = NULL;
static List_View *list = NULL;


static void
_list_show_items(const Evry_State *s, Evry_Plugin *p)
{
   Evry_Item *it;
   Eina_List *l;
   int mw, mh, h;
   Evas_Object *o;

   _list_clear_list(s);

   if (!p) return;

   if (list->scroll_timer)
     {
	ecore_timer_del(list->scroll_timer);
	list->scroll_timer = NULL;
     }
   if (list->scroll_animator)
     {
	ecore_animator_del(list->scroll_animator);
	list->scroll_animator = NULL;
     }

   list->scroll_align = 0;

   evas_event_freeze(list->evas);
   e_box_freeze(list->o_list);

   EINA_LIST_FOREACH(p->items, l, it)
     {
	o = it->o_bg;

	if (!o)
	  {
	     o = edje_object_add(list->evas);
	     it->o_bg = o;
	     e_theme_edje_object_set(o, "base/theme/everything",
				     "e/widgets/everything/item");

	     edje_object_part_text_set(o, "e.text.title", it->label);

	     /* evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_IN,
	      * 			       _evry_cb_item_mouse_in, it);
	      * evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_OUT,
	      * 			       _evry_cb_item_mouse_out, it); */
	  }

	edje_object_size_min_calc(o, &mw, &mh);
	e_box_pack_end(list->o_list, o);
	e_box_pack_options_set(o, 1, 1, 1, 0, 0.5, 0.5, mw, mh, 9999, mh);
	evas_object_show(o);

	if (it->o_icon && edje_object_part_exists(o, "e.swallow.icons"))
	  {
	     edje_object_part_swallow(o, "e.swallow.icons", it->o_icon);
	     evas_object_show(it->o_icon);
	  }

	if (it == s->sel_item)
	  {
	     edje_object_signal_emit(it->o_bg, "e,state,selected", "e");
	     if (it->o_icon)
	       edje_object_signal_emit(it->o_icon, "e,state,selected", "e");
	     if (it->browseable)
	       edje_object_signal_emit(it->o_bg, "e,state,arrow_show", "e");
	  }
	else
	  {
	     edje_object_signal_emit(it->o_bg, "e,state,unselected", "e");
	  }
	
	evry_item_ref(it);
	list->items = eina_list_append(list->items, it);
     }
   e_box_thaw(list->o_list);

   list->item_idler = ecore_idler_add(_list_item_idler, p);

   e_box_min_size_get(list->o_list, NULL, &mh);
   evas_object_geometry_get(list->o_list, NULL, NULL, NULL, &h);
   if (mh <= h)
     e_box_align_set(list->o_list, 0.5, 0.0);
   else
     e_box_align_set(list->o_list, 0.5, 1.0);

   evas_event_thaw(list->evas);

   _list_scroll_to(s->sel_item);
}

static void
_list_clear_list(const Evry_State *s)
{
   Evry_Item *it;

   if (list->item_idler)
     {
	ecore_idler_del(list->item_idler);
	list->item_idler = NULL;
     }

   if (list->items)
     {
	evas_event_freeze(list->evas);
	e_box_freeze(list->o_list);
	EINA_LIST_FREE(list->items, it)
	  {
	     if (it->o_bg)
	       {
		  e_box_unpack(it->o_bg);
		  evas_object_hide(it->o_bg);
	       }

	     if (it->o_icon)
	       evas_object_hide(it->o_icon);

	     evry_item_free(it);
	  }
     }

   e_box_thaw(list->o_list);
   evas_event_thaw(list->evas);
   /* } */
}

static int
_list_item_idler(void *data)
{
   Evry_Plugin *p = data;
   int cnt = 5;
   Eina_List *l;
   Evry_Item *it;

   if (!list->item_idler) return 0;

   if (!p->icon_get) goto end;
   e_box_freeze(list->o_list);

   EINA_LIST_FOREACH(list->items, l, it)
     {
	if (it->o_icon) continue;

	it->o_icon = p->icon_get(p, it, list->evas);

	if (it->o_icon)
	  {
	     edje_object_part_swallow(it->o_bg, "e.swallow.icons", it->o_icon);
	     evas_object_show(it->o_icon);
	     cnt--;
	  }

	if (cnt == 0) break;
     }
   e_box_thaw(list->o_list);

   if (cnt == 0) return 1;
 end:
   list->item_idler = NULL;
   return 0;
}


/* static int
 * _list_scroll_timer(void *data __UNUSED__)
 * {
 *    if (list->scroll_animator)
 *      {
 * 	double spd;
 * 	spd = evry_conf->scroll_speed;
 * 	list->scroll_align = (list->scroll_align * (1.0 - spd)) +
 * 	  (list->scroll_align_to * spd);
 * 	return 1;
 *      }
 *    list->scroll_timer = NULL;
 *    return 0;
 * }
 *
 * static int
 * _list_animator(void *data __UNUSED__)
 * {
 *    double da;
 *    Eina_Bool scroll_to = 1;
 *
 *    da = list->scroll_align - list->scroll_align_to;
 *    if (da < 0.0) da = -da;
 *    if (da < 0.01)
 *      {
 * 	list->scroll_align = list->scroll_align_to;
 * 	scroll_to = 0;
 *      }
 *    e_box_align_set(list->o_list, 0.5, 1.0 - list->scroll_align);
 *    if (scroll_to) return 1;
 *    list->scroll_animator = NULL;
 *    return 0;
 * } */

static void
_list_item_desel(const Evry_State *s, Evry_Item *it)
{
   if (s->sel_item)
     {
	it = s->sel_item;

	if (it->o_bg)
	  edje_object_signal_emit(it->o_bg, "e,state,unselected", "e");
	if (it->o_icon)
	  edje_object_signal_emit(it->o_icon, "e,state,unselected", "e");
     }
}

static void
_list_item_sel(const Evry_State *s, Evry_Item *it)
{
   if (s->sel_item == it) return;

   _list_item_desel(s, NULL);

   if (it->o_bg)
     edje_object_signal_emit(it->o_bg, "e,state,selected", "e");
   if (it->o_icon)
     edje_object_signal_emit(it->o_icon, "e,state,selected", "e");
   if (it->browseable)
     edje_object_signal_emit(it->o_bg, "e,state,arrow_show", "e");

   _list_scroll_to(it);

   evry_item_select(s, it);
}

static void
_list_scroll_to(Evry_Item *it)
{
   int n, h, mh, i = 0;
   Eina_List *l;

   if (!it) return;

   for(l = list->items; l; l = l->next, i++)
     if (l->data == it) break;
   n = eina_list_count(list->items);

   e_box_min_size_get(list->o_list, NULL, &mh);
   evas_object_geometry_get(list->o_list, NULL, NULL, NULL, &h);

   if (i >= n || mh <= h)
     {
	e_box_align_set(list->o_list, 0.5, 0.0);
	return;
     }

   if (n > 1)
     {
	list->scroll_align_to = (double)i / (double)(n - 1);
	/* if (evry_conf->scroll_animate)
 	 *   {
	 *      if (!list->scroll_timer)
	 *        list->scroll_timer = ecore_timer_add(0.01, _list_scroll_timer, NULL);
	 *      if (!list->scroll_animator)
	 *        list->scroll_animator = ecore_animator_add(_list_animator, NULL);
	 *   }
	 * else */
	  {
	     list->scroll_align = list->scroll_align_to;
	     e_box_align_set(list->o_list, 0.5, 1.0 - list->scroll_align);
	  }
     }
   else
     e_box_align_set(list->o_list, 0.5, 1.0);
}

static void
_list_item_next(const Evry_State *s)
{
   Eina_List *l;
   Evry_Item *it;

   if (!list->items) return;

   EINA_LIST_FOREACH (list->items, l, it)
     {
	if (it == s->sel_item)
	  {
	     if (l->next)
	       {
		  _list_item_sel(s, l->next->data);
	       }

	     break;
	  }
     }
}

static void
_list_item_prev(const Evry_State *s)
{
   Eina_List *l;
   Evry_Item *it;

   if (!list->items) return;

   if (!s->sel_item) return;

   EINA_LIST_FOREACH (list->items, l, it)
     {
	if (it == s->sel_item)
	  {
	     if (l->prev)
	       {
		  _list_item_sel(s, l->prev->data);
		  return;
	       }
	     break;
	  }
     }

   evry_list_win_hide();
}

static void
_list_item_first(const Evry_State *s)
{
   if (!list->items) return;

   _list_item_sel(s, list->items->data);
}

static void
_list_item_last(const Evry_State *s)
{
   if (!list->items) return;

   _list_item_sel(s, eina_list_last(list->items)->data);
}

static int
_cb_key_down(Evry_View *v, const Ecore_Event_Key *ev)
{
   const char *key = ev->key;

   if (!strcmp(key, "Up"))
     _list_item_prev(v->state);
   else if (!strcmp(key, "Down"))
     _list_item_next(v->state);
   else if (!strcmp(key, "End"))
     _list_item_last(v->state);
   else if (!strcmp(key, "Home"))
     _list_item_first(v->state);
   else return 0;

   return 1;
}

static Evas_Object *
_begin(Evry_View *v, const Evry_State *s, const Evas_Object *swallow)
{
   Evas_Object *o;
   list->evas = evas_object_evas_get(swallow);

   o = e_box_add(list->evas);
   list->o_list = o;
   e_box_orientation_set(o, 0);
   e_box_homogenous_set(o, 1);

   return o;
}

static void
_cleanup(Evry_View *v)
{
   if (list->scroll_timer)
     ecore_timer_del(list->scroll_timer);
   if (list->scroll_animator)
     ecore_animator_del(list->scroll_animator);
   if (list->item_idler)
     ecore_idler_del(list->item_idler);

   evas_object_del(list->o_list);
}

static void
_clear(Evry_View *v, const Evry_State *s)
{
   _list_clear_list(s);
}

static int
_update(Evry_View *v, const Evry_State *s)
{
   v->state = s;
   _list_show_items(s, s->plugin);

   return 1;
}

static Eina_Bool
_init(void)
{
   view = E_NEW(Evry_View, 1);
   view->name = "List View";
   view->begin = &_begin;
   view->update = &_update;
   view->clear = &_clear;
   view->cb_key_down = &_cb_key_down;
   view->cleanup = &_cleanup;
   evry_view_register(view, 1);

   list = E_NEW(List_View, 1);

   return EINA_TRUE;
}

static void
_shutdown(void)
{
   evry_view_unregister(view);
   E_FREE(view);
   E_FREE(list);
}


EINA_MODULE_INIT(_init);
EINA_MODULE_SHUTDOWN(_shutdown);
