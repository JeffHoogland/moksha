#include "e_mod_main.h"

typedef struct _View View;

struct _View
{
  Evry_View view;
  Evas *evas;
  const Evry_State *state;
  Tab_View *tabs;
  
  Evas_Object *o_list;
  Eina_List *items;

  
  Ecore_Idle_Enterer *item_idler;

  double       scroll_align_to;
  double       scroll_align;


  
  /* int          ev_last_is_mouse;
   * Evry_Item   *item_mouseover; */
  /* Ecore_Timer *scroll_timer; */
  /* Ecore_Animator *scroll_animator; */
};


static Evry_View *view = NULL;


static void
_list_clear(View *v)
{
   Evry_Item *it;

   if (v->item_idler)
     {
	ecore_idle_enterer_del(v->item_idler);
	v->item_idler = NULL;
     }

   /* if (v->scroll_timer)
    *   {
    * 	ecore_timer_del(v->scroll_timer);
    * 	v->scroll_timer = NULL;
    *   }
    *
    * if (v->scroll_animator)
    *   {
    * 	ecore_animator_del(v->scroll_animator);
    * 	v->scroll_animator = NULL;
    *   } */

   /* v->scroll_align = 0; */

   if (!v->items) return;

   evas_event_freeze(v->evas);
   e_box_freeze(v->o_list);

   EINA_LIST_FREE(v->items, it)
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

   e_box_thaw(v->o_list);
   evas_event_thaw(v->evas);
}

static void
_list_scroll_to(View *v, const Evry_Item *it)
{
   int n, h, mh, i = 0;
   Eina_List *l;

   if (!it) return;

   for(l = v->items; l; l = l->next, i++)
     if (l->data == it) break;
   n = eina_list_count(v->items);

   e_box_min_size_get(v->o_list, NULL, &mh);
   evas_object_geometry_get(v->o_list, NULL, NULL, NULL, &h);

   if (mh <= h)
     {
	e_box_align_set(v->o_list, 0.5, 0.0);
	return;
     }

   /* FIXME: how to get size of list before it is completely shown? */
   if (n > 6)
     {
	v->scroll_align_to = (double)i / (double)(n - 1);
	{
	   v->scroll_align = v->scroll_align_to;
	   e_box_align_set(v->o_list, 0.5, 1.0 - v->scroll_align);
	}
     }
   else
     e_box_align_set(v->o_list, 0.5, 0.0);
}

static int
_list_item_idler(void *data)
{
   View *v = data;
   Evry_Plugin *p = v->state->plugin;
   Eina_List *l;
   Evry_Item *it;
   int cnt = 5;

   if (!v->item_idler) return 0;

   if (!p->icon_get) goto end;
   e_box_freeze(v->o_list);

   EINA_LIST_FOREACH(v->items, l, it)
     {
	if (it->o_icon) continue;

	it->o_icon = p->icon_get(p, it, v->evas);

	if (it->o_icon)
	  {
	     edje_object_part_swallow(it->o_bg, "e.swallow.icons", it->o_icon);
	     evas_object_show(it->o_icon);
	     cnt--;
	  }

	if (cnt == 0) break;
     }
   e_box_thaw(v->o_list);

   e_util_wakeup();
   
   if (cnt == 0) return 1;
 end:
   v->item_idler = NULL;
   return 0;
}

static void
_list_item_set_selected(const Evry_Item *it)
{
   if (it->o_bg)
     edje_object_signal_emit(it->o_bg, "e,state,selected", "e");
   if (it->o_icon)
     edje_object_signal_emit(it->o_icon, "e,state,selected", "e");
   if (it->browseable)
     edje_object_signal_emit(it->o_bg, "e,state,arrow_show", "e");
}

static void
_list_item_set_unselected(const Evry_Item *it)
{
   if (it->o_bg)
     edje_object_signal_emit(it->o_bg, "e,state,unselected", "e");
   if (it->o_icon)
     edje_object_signal_emit(it->o_icon, "e,state,unselected", "e");
}

static int
_list_update(View *v)
{
   Evry_Item *it;
   Eina_List *l;
   int mw = -1, mh;
   Evas_Object *o;
   int divider = 1;

   _list_clear(v);

   if (!v->state->plugin)
     return 1;

   evas_event_freeze(v->evas);
   e_box_freeze(v->o_list);

   EINA_LIST_FOREACH(v->state->plugin->items, l, it)
     {
	o = it->o_bg;

	if (!o)
	  {
	     o = edje_object_add(v->evas);
	     it->o_bg = o;
	     e_theme_edje_object_set(o, "base/theme/everything",
				     "e/modules/everything/item");

	     edje_object_part_text_set(o, "e.text.title", it->label);
	  }

	if (mw < 0) edje_object_size_min_get(o, &mw, &mh);

	e_box_pack_end(v->o_list, o);
	e_box_pack_options_set(o, 1, 1, 1, 0, 0.5, 0.5, mw, mh, 9999, mh);
	evas_object_show(o);

	if (divider)
	  edje_object_signal_emit(it->o_bg, "e,state,odd", "e");
	else
	  edje_object_signal_emit(it->o_bg, "e,state,even", "e");

	divider = divider ? 0 : 1;

	if (it->o_icon && edje_object_part_exists(o, "e.swallow.icons"))
	  {
	     edje_object_part_swallow(o, "e.swallow.icons", it->o_icon);
	     evas_object_show(it->o_icon);
	  }

	if (it == v->state->sel_item)
	  _list_item_set_selected(it);
	else
	  _list_item_set_unselected(it);

	evry_item_ref(it);
	v->items = eina_list_append(v->items, it);
     }

   e_box_thaw(v->o_list);
   evas_event_thaw(v->evas);

   _list_scroll_to(v, v->state->sel_item);

   v->item_idler = ecore_idle_enterer_add(_list_item_idler, v);

   return 1;
}


static void
_list_item_sel(View *v, const Evry_Item *it)
{
   if (v->state->sel_item)
     {
	Evry_Item *it2 = v->state->sel_item;

	if (it == it2) return;
	_list_item_set_unselected(it2);
     }

   _list_item_set_selected(it);
   _list_scroll_to(v, it);
}

static void
_list_item_next(View *v)
{
   Eina_List *l;
   Evry_Item *it;

   EINA_LIST_FOREACH (v->items, l, it)
     {
	if (it != v->state->sel_item) continue;

	if (l->next)
	  {
	     it = l->next->data;
	     _list_item_sel(v, it);
	     evry_item_select(v->state, it);
	  }
	break;
     }
}

static void
_list_item_prev(View *v)
{
   Eina_List *l;
   Evry_Item *it;

   EINA_LIST_FOREACH (v->items, l, it)
     {
	if (it != v->state->sel_item) continue;

	if (l->prev)
	  {
	     it = l->prev->data;
	     _list_item_sel(v, it);
	     evry_item_select(v->state, it);
	     return;
	  }
	break;
     }

   evry_list_win_hide();
}

static void
_list_item_first(View *v)
{
   Evry_Item *it;

   if (!v->items) return;

   it = v->items->data;
   _list_item_sel(v, it);
   evry_item_select(v->state, it);
}

static void
_list_item_last(View *v)
{
   Evry_Item *it;

   if (!v->items) return;

   it = eina_list_last(v->items)->data;
   _list_item_sel(v, it);
   evry_item_select(v->state, it);
}

static int
_update(Evry_View *view)
{
   VIEW(v, view);

   v->tabs->update(v->tabs);
   _list_update(v);

   return 1;
}

static void
_clear(Evry_View *view)
{
   VIEW(v, view);

   v->tabs->clear(v->tabs);
   _list_clear(v);
}

static int
_cb_key_down(Evry_View *view, const Ecore_Event_Key *ev)
{
   VIEW(v, view);

   const char *key = ev->key;

   if (v->tabs->key_down(v->tabs, ev))
     {
	_list_update(v);
	return 1;
     }
   else if (!strcmp(key, "Up"))
     _list_item_prev(v);
   else if (!strcmp(key, "Down"))
     _list_item_next(v);
   else if (!strcmp(key, "End"))
     _list_item_last(v);
   else if (!strcmp(key, "Home"))
     _list_item_first(v);
   else return 0;

   return 1;
}

static Evry_View *
_create(Evry_View *view, const Evry_State *s, const Evas_Object *swallow)
{
   View *v;
   Evas_Object *o;

   v = E_NEW(View, 1);
   v->view = *view;
   v->evas = evas_object_evas_get(swallow);
   v->state = s;

   o = e_box_add(v->evas);
   e_box_orientation_set(o, 0);
   e_box_homogenous_set(o, 1);
   v->view.o_list = o;
   v->o_list = o;

   v->tabs = evry_tab_view_new(s, v->evas);
   v->view.o_bar = v->tabs->o_tabs;

   return EVRY_VIEW(v);
}

static void
_destroy(Evry_View *view)
{
   VIEW(v, view);

   _clear(view);
   evas_object_del(v->o_list);
   evry_tab_view_free(v->tabs);
   
   E_FREE(v);
}

static Eina_Bool
_init(void)
{
   view = E_NEW(Evry_View, 1);
   view->id = view;
   view->name = "List View";
   view->create = &_create;
   view->destroy = &_destroy;
   view->update = &_update;
   view->clear = &_clear;
   view->cb_key_down = &_cb_key_down;
   evry_view_register(view, 1);

   return EINA_TRUE;
}

static void
_shutdown(void)
{
   evry_view_unregister(view);
   E_FREE(view);
}


EINA_MODULE_INIT(_init);
EINA_MODULE_SHUTDOWN(_shutdown);



/* static int
 * _list_scroll_timer(void *data __UNUSED__)
 * {
 *    if (v->scroll_animator)
 *      {
 * 	double spd;
 * 	spd = evry_conf->scroll_speed;
 * 	v->scroll_align = (v->scroll_align * (1.0 - spd)) +
 * 	  (v->scroll_align_to * spd);
 * 	return 1;
 *      }
 *    v->scroll_timer = NULL;
 *    return 0;
 * }
 *
 * static int
 * _list_animator(void *data __UNUSED__)
 * {
 *    double da;
 *    Eina_Bool scroll_to = 1;
 *
 *    da = v->scroll_align - v->scroll_align_to;
 *    if (da < 0.0) da = -da;
 *    if (da < 0.01)
 *      {
 * 	v->scroll_align = v->scroll_align_to;
 * 	scroll_to = 0;
 *      }
 *    e_box_align_set(v->o_list, 0.5, 1.0 - v->scroll_align);
 *    if (scroll_to) return 1;
 *    v->scroll_animator = NULL;
 *    return 0;
 * } */

/* if (evry_conf->scroll_animate)
 *   {
 *      if (!v->scroll_timer)
 *        v->scroll_timer = ecore_timer_add(0.01, _list_scroll_timer, NULL);
 *      if (!v->scroll_animator)
 *        v->scroll_animator = ecore_animator_add(_list_animator, NULL);
 *   }
 * else */

