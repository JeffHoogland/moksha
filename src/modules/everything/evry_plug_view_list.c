#include "Evry.h"

typedef struct _List_View List_View;
typedef struct _List_Tab  List_Tab;

struct _List_Tab
{
  Evry_Plugin *plugin;
  Evas_Object *o_tab;
};



struct _List_View
{
  Evry_View view;
  Evas *evas;
  const Evas_Object *swallow;

  const Evry_State *state;

  Evas_Object *o_list;
  Evas_Object *o_tabs;

  Eina_List *items;
  Eina_List *tabs;

  Ecore_Idler *item_idler;

  double       scroll_align_to;
  double       scroll_align;

  /* int          ev_last_is_mouse;
   * Evry_Item   *item_mouseover; */
  /* Ecore_Timer *scroll_timer; */
  /* Ecore_Animator *scroll_animator; */
};


static Evry_View *view = NULL;


static void
_list_clear(List_View *v)
{
   Evry_Item *it;

   if (v->item_idler)
     {
	ecore_idler_del(v->item_idler);
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
_list_scroll_to(List_View *v, const Evry_Item *it)
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
   List_View *v = data;
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
_list_update(List_View *v)
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

   v->item_idler = ecore_idler_add(_list_item_idler, v);

   return 1;
}


static void
_list_item_sel(List_View *v, const Evry_Item *it)
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
_list_item_next(List_View *v)
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
_list_item_prev(List_View *v)
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
_list_item_first(List_View *v)
{
   Evry_Item *it;

   if (!v->items) return;

   it = v->items->data;
   _list_item_sel(v, it);
   evry_item_select(v->state, it);
}

static void
_list_item_last(List_View *v)
{
   Evry_Item *it;

   if (!v->items) return;

   it = eina_list_last(v->items)->data;
   _list_item_sel(v, it);
   evry_item_select(v->state, it);
}

static void
_list_tab_scroll_to(List_View *v, Evry_Plugin *p)
{
   int n, w, mw, i;
   double align;
   Eina_List *l;
   const Evry_State *s = v->state;

   for(i = 0, l = s->cur_plugins; l; l = l->next, i++)
     if (l->data == p) break;

   n = eina_list_count(s->cur_plugins);

   e_box_min_size_get(v->o_tabs, &mw, NULL);
   evas_object_geometry_get(v->o_tabs, NULL, NULL, &w, NULL);

   if (mw <= w + 5)
     {
	e_box_align_set(v->o_tabs, 0.0, 0.5);
	return;
     }

   if (n > 1)
     {
	align = (double)i / (double)(n - 1);
	/* if (evry_conf->scroll_animate)
	 *   {
	 *      if (!scroll_timer)
	 *        scroll_timer = ecore_timer_add(0.01, _evry_list_scroll_timer, NULL);
	 *      if (!scroll_animator)
	 *        scroll_animator = ecore_animator_add(_evry_list_animator, NULL);
	 *   }
	 * else */
	e_box_align_set(v->o_tabs, 1.0 - align, 0.5);
     }
   else
     e_box_align_set(v->o_tabs, 1.0, 0.5);
}

static void
_list_tabs_update(List_View *v)
{

   Eina_List *l, *ll;
   Evry_Plugin *p;
   const Evry_State *s = v->state;
   List_Tab *tab;
   Evas_Coord mw, cw, w;
   Evas_Object *o;

   evas_object_geometry_get(v->o_tabs, NULL, NULL, &w, NULL);

   /* remove tabs for not active plugins */
   e_box_freeze(v->o_tabs);

   EINA_LIST_FOREACH(v->tabs, l, tab)
     {
	e_box_unpack(tab->o_tab);
	evas_object_hide(tab->o_tab);
     }

   /* show/update tabs of active plugins */
   EINA_LIST_FOREACH(s->cur_plugins, l, p)
     {
	EINA_LIST_FOREACH(v->tabs, ll, tab)
	  if (tab->plugin == p) break;

	if (!tab && (strlen(p->name) > 0))
	  {
	     tab = E_NEW(List_Tab, 1);
	     tab->plugin = p;

	     o = edje_object_add(v->evas);
	     e_theme_edje_object_set(o, "base/theme/everything",
				     "e/modules/everything/tab_item");
	     edje_object_part_text_set(o, "e.text.label", p->name);

	     tab->o_tab = o;

	     v->tabs = eina_list_append(v->tabs, tab);
	  }

	if (!tab) continue;

	o = tab->o_tab;
	evas_object_show(o);
	e_box_pack_end(v->o_tabs, o);

	edje_object_size_min_calc(o, &cw, NULL);
	edje_object_size_min_get(o, &mw, NULL);

	e_box_pack_options_set(o, 1, 1, 1, 0, 0.0, 0.5,
			       (mw < cw ? cw : mw), 10,
			       (w ? w/3 : 150), 9999);
	if (s->plugin == p)
	  edje_object_signal_emit(o, "e,state,selected", "e");
	else
	  edje_object_signal_emit(o, "e,state,unselected", "e");
     }

   e_box_thaw(v->o_tabs);

   if (s->plugin)
     _list_tab_scroll_to(v, s->plugin);
}


static void
_list_tabs_clear(List_View *v)
{
   Eina_List *l;
   List_Tab *tab;

   e_box_freeze(v->o_tabs);
   EINA_LIST_FOREACH(v->tabs, l, tab)
     {
	e_box_unpack(tab->o_tab);
	evas_object_hide(tab->o_tab);
     }
   e_box_thaw(v->o_tabs);
}

static void
_list_plugin_select(List_View *v, Evry_Plugin *p)
{
   evry_plugin_select(v->state, p);

   _list_tabs_update(v);
   _list_tab_scroll_to(v, p);
   _list_update(v);
}

static void
_list_plugin_next(List_View *v)
{
   Eina_List *l;
   Evry_Plugin *p = NULL;
   const Evry_State *s = v->state;

   if (!s->plugin) return;

   l = eina_list_data_find_list(s->cur_plugins, s->plugin);

   if (l && l->next)
     p = l->next->data;
   else if (s->plugin != s->cur_plugins->data)
     p = s->cur_plugins->data;

   if (p) _list_plugin_select(v, p);
}

static void
_list_plugin_next_by_name(List_View *v, const char *key)
{
   Eina_List *l;
   Evry_Plugin *p, *first = NULL, *next = NULL;
   int found = 0;
   const Evry_State *s = v->state;

   if (!s->plugin) return;

   EINA_LIST_FOREACH(s->cur_plugins, l, p)
     {
	if (p->name && (!strncasecmp(p->name, key, 1)))
	  {
	     if (!first) first = p;

	     if (found && !next)
	       next = p;
	  }
	if (p == s->plugin) found = 1;
     }

   if (next)
     p = next;
   else if (first != s->plugin)
     p = first;
   else
     p = NULL;

   if (p) _list_plugin_select(v, p);
}

static void
_list_plugin_prev(List_View *v)
{
   Eina_List *l;
   Evry_Plugin *p = NULL;
   const Evry_State *s = v->state;

   if (!s->plugin) return;

   l = eina_list_data_find_list(s->cur_plugins, s->plugin);

   if (l && l->prev)
     p = l->prev->data;
   else
     {
	l = eina_list_last(s->cur_plugins);
	if (s->plugin != l->data)
	  p = l->data;
     }

   if (p) _list_plugin_select(v, p);
}

static int
_update(Evry_View *view)
{
   List_View *v = (List_View *) view;
   _list_update(v);
   _list_tabs_update(v);
   return 1;
}

static void
_clear(Evry_View *view)
{
   List_View *v = (List_View *) view;
   _list_clear(v);
   _list_tabs_clear(v);
}


static int
_cb_key_down(Evry_View *view, const Ecore_Event_Key *ev)
{
   List_View *v = (List_View *) view;

   const char *key = ev->key;

   if (!strcmp(key, "Up"))
     _list_item_prev(v);
   else if (!strcmp(key, "Down"))
     _list_item_next(v);
   else if (!strcmp(key, "End"))
     _list_item_last(v);
   else if (!strcmp(key, "Home"))
     _list_item_first(v);
   else if (!strcmp(key, "Next"))
     _list_plugin_next(v);
   else if (!strcmp(key, "Prior"))
     _list_plugin_prev(v);
   else if (ev->modifiers & ECORE_EVENT_MODIFIER_SHIFT)
     {
	if (!strcmp(key, "Left"))
	  _list_plugin_prev(v);
	else if (!strcmp(key, "Right"))
	  _list_plugin_next(v);
	else return 0;
	
	return 1;
     }
   else if (ev->modifiers & ECORE_EVENT_MODIFIER_CTRL)
     {
	if (!strcmp(key, "Left"))
	  _list_plugin_prev(v);
	else if (!strcmp(key, "Right"))
	  _list_plugin_next(v);
	else if (ev->compose)
	  _list_plugin_next_by_name(v, key);
     }   
   else return 0;

   return 1;
}

static Evry_View *
_create(Evry_View *view, const Evry_State *s, const Evas_Object *swallow)
{
   List_View *v;
   Evas_Object *o;

   v = E_NEW(List_View, 1);
   v->view = *view;
   v->evas = evas_object_evas_get(swallow);
   v->swallow = swallow;
   v->state = s;

   o = e_box_add(v->evas);
   e_box_orientation_set(o, 0);
   e_box_homogenous_set(o, 1);
   v->view.o_list = o;
   v->o_list = o;

   o = e_box_add(v->evas);
   e_box_orientation_set(o, 1);
   e_box_homogenous_set(o, 1);
   v->view.o_bar = o;
   v->o_tabs = o;

   return &v->view;
}

static void
_destroy(Evry_View *view)
{
   List_View *v = (List_View *) view;
   List_Tab *tab;

   _clear(view);

   EINA_LIST_FREE(v->tabs, tab)
     {
	evas_object_del(tab->o_tab);
	E_FREE(tab);
     }

   evas_object_del(v->o_list);
   evas_object_del(v->o_tabs);

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

