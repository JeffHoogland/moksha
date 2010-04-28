#include "e.h"
#include "e_mod_main.h"

/* TODO */
/* - animations
 * - mouse handlers
 * - show item descriptions
 * - keybinding configuration
 * - shortcuts for plugins
 */
#define INPUTLEN 256
#define MATCH_LAG 0.15
#define INITIAL_MATCH_LAG 0.2

/* #undef DBG
 * #define DBG(...) ERR(__VA_ARGS__) */

typedef struct _Evry_Window Evry_Window;
typedef struct _Evry_List_Window Evry_List_Window;

struct _Evry_Window
{
  E_Popup *popup;
  Evas_Object *o_main;

  Eina_Bool request_selection;
  Eina_Bool plugin_dedicated;
};

struct _Evry_List_Window
{
  E_Popup     *popup;
  Evas_Object *o_main;
  Eina_Bool    visible;
};

static void _evry_matches_update(Evry_Selector *sel, int async);
static void _evry_plugin_action(Evry_Selector *sel, int finished);
static void _evry_plugin_select(Evry_State *s, Evry_Plugin *p);
static void _evry_plugin_list_insert(Evry_State *s, Evry_Plugin *p);
static int  _evry_backspace(Evry_Selector *sel);
static void _evry_update(Evry_Selector *sel, int fetch);
static void _evry_update_text_label(Evry_State *s);
static int  _evry_clear(Evry_Selector *sel);
static int  _evry_cb_update_timer(void *data);

static Evry_State *_evry_state_new(Evry_Selector *sel, Eina_List *plugins);
static void _evry_state_pop(Evry_Selector *sel);

static Evry_Selector *_evry_selector_new(int type);
static void _evry_selector_free(Evry_Selector *sel);
static void _evry_selector_activate(Evry_Selector *sel);
static void _evry_selectors_switch(int dir);
static void _evry_selector_update(Evry_Selector *sel);
static void _evry_selector_icon_set(Evry_Selector *sel);
static int  _evry_selector_subjects_get(const char *plugin_name);
static int  _evry_selector_actions_get(Evry_Item *it);
static int  _evry_selector_objects_get(Evry_Action *act);
static void _evry_selector_update_actions(Evry_Selector *sel);
static Evry_Selector *_evry_selector_for_plugin_get(Evry_Plugin *p);

static Evry_Window *_evry_window_new(E_Zone *zone);
static void _evry_window_free(Evry_Window *win);

static Evry_List_Window *_evry_list_win_new(E_Zone *zone);
static void _evry_list_win_free(Evry_List_Window *win);
static void _evry_list_win_show(void);
static void _evry_list_win_update(Evry_State *s);
static void _evry_list_win_clear(int hide);

static void _evry_view_clear(Evry_State *s);
static void _evry_view_update(Evry_State *s, Evry_Plugin *plugin);
static int  _evry_view_key_press(Evry_State *s, Ecore_Event_Key *ev);
static int  _evry_view_toggle(Evry_State *s, const char *trigger);
static void _evry_view_show(Evry_View *v);
static void _evry_view_hide(Evry_View *v, int slide);

static void _evry_item_desel(Evry_State *s, Evry_Item *it);
static void _evry_item_sel(Evry_State *s, Evry_Item *it);

static int  _evry_cb_key_down(void *data, int type, void *event);
static int  _evry_cb_selection_notify(void *data, int type, void *event);

/* local subsystem globals */
static Evry_Window *win = NULL;
static Evry_List_Window *list = NULL;
static Ecore_X_Window input_window = 0;
static Eina_List   *handlers = NULL;

static Evry_Selector *selector = NULL;
static const char *thumb_types = NULL;

Evry_Selector **selectors = NULL;
const char *action_selector;

/* externally accessible functions */
int
evry_init(void)
{
   action_selector = eina_stringshare_add(_("Select Action"));
   thumb_types = eina_stringshare_add("FILE");
   return 1;
}

int
evry_shutdown(void)
{
   evry_hide();

   eina_stringshare_del(thumb_types);
   eina_stringshare_del(action_selector);
   return 1;
}

static int
_evry_cb_item_changed(void *data, int type, void *event)
{
   Evry_Event_Item_Changed *ev = event;
   Evry_Selector *sel;
   int i;

   for (i = 0; i < 3; i++)
     {
	sel = selectors[i];

	if (sel->state && sel->state->cur_item == ev->item)
	  {
	     _evry_selector_update(sel);
	     _evry_selector_update_actions(sel);
	     break;
	  }
     }

   return 1;
}

static Ecore_Timer *_show_timer = NULL;

static int
_cb_show_timer(void *data)
{
   _show_timer = NULL;

   if (evry_conf->views && selector->state)
     {
   	Evry_View *view =evry_conf->views->data;
   	Evry_State *s = selector->state;

	if (evry_conf->first_run)
	  {
	     _evry_view_toggle(s, "?");
	     evry_conf->first_run = EINA_FALSE;
	  }
	else
	  {
	     s->view = view->create(view, s, list->o_main);

	     _evry_view_show(s->view);
	  }
     }
   else
     {
	return 0;
     }

   _evry_list_win_show();

   return 0;
}

int
evry_show(E_Zone *zone, const char *params)
{
   if (win) return 0;

   E_OBJECT_CHECK_RETURN(zone, 0);
   E_OBJECT_TYPE_CHECK_RETURN(zone, E_ZONE_TYPE, 0);

   input_window = ecore_x_window_input_new(zone->container->win,
					   zone->x, zone->y,
					   zone->w, zone->h);
   ecore_x_window_show(input_window);
   if (!e_grabinput_get(input_window, 1, input_window))
     goto error1;

   win = _evry_window_new(zone);
   if (!win) goto error2;

   list = _evry_list_win_new(zone);
   if (!list) goto error2;

   list->visible = EINA_FALSE;

   evry_history_load();

   if (params)
     win->plugin_dedicated = EINA_TRUE;

   selectors = E_NEW(Evry_Selector*, 3);
   selectors[0] = _evry_selector_new(EVRY_PLUGIN_SUBJECT);
   selectors[1] = _evry_selector_new(EVRY_PLUGIN_ACTION);
   selectors[2] = _evry_selector_new(EVRY_PLUGIN_OBJECT);

   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (ECORE_EVENT_KEY_DOWN, _evry_cb_key_down, NULL));
   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (ECORE_X_EVENT_SELECTION_NOTIFY,
       _evry_cb_selection_notify, win));
   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (EVRY_EVENT_ITEM_CHANGED,
       _evry_cb_item_changed, NULL));

   e_popup_layer_set(list->popup, 255);
   e_popup_layer_set(win->popup, 255);
   e_popup_show(win->popup);
   e_popup_show(list->popup);

   _evry_selector_subjects_get(params);
   _evry_selector_update(selectors[0]);
   _evry_selector_activate(selectors[0]);

   if (!evry_conf->hide_input)
     {
	edje_object_part_text_set(win->o_main, "e.text.label", "Search:");
	edje_object_part_text_set(list->o_main, "e.text.label", "Search:");
	edje_object_signal_emit(list->o_main, "e,state,entry_show", "e");
     }

   if (!evry_conf->hide_list)
     _show_timer = ecore_timer_add(0.01, _cb_show_timer, NULL);

   return 1;

 error2:
   if (win)
     _evry_window_free(win);
   if (list)
     _evry_list_win_free(list);
   win = NULL;
   list = NULL;

 error1:
   ecore_x_window_free(input_window);
   input_window = 0;

   return 0;
}

void
evry_hide(void)
{
   Ecore_Event_Handler *ev;

   if (!win) return;

   /* _evry_view_clear(selector->state); */
   if (_show_timer)
     ecore_timer_del(_show_timer);
   _show_timer = NULL;

   list->visible = EINA_FALSE;
   _evry_selector_free(selectors[0]);
   _evry_selector_free(selectors[1]);
   _evry_selector_free(selectors[2]);
   E_FREE(selectors);

   selectors = NULL;
   selector = NULL;

   _evry_list_win_free(list);
   list = NULL;

   _evry_window_free(win);
   win = NULL;

   EINA_LIST_FREE(handlers, ev)
     ecore_event_handler_del(ev);

   ecore_x_window_free(input_window);
   e_grabinput_release(input_window, input_window);
   input_window = 0;

   evry_history_unload();
}

EAPI void
evry_clear_input(Evry_Plugin *p)
{
   Evry_Selector *sel = _evry_selector_for_plugin_get(p);

   if (sel != selector) return;

   Evry_State *s = sel->state;

   if (!s) return;

   if (s->inp[0] != 0)
     {
       	s->inp[0] = 0;
     }
   s->input = s->inp;

   _evry_update_text_label(s);
}

//#define CHECK_REFS 1

#ifdef CHECK_REFS
static int item_cnt = 0;
#endif

EAPI Evry_Item *
evry_item_new(Evry_Item *base, Evry_Plugin *p, const char *label,
	      Evas_Object *(*icon_get) (Evry_Item *it, Evas *e),
	      void (*cb_free) (Evry_Item *item))
{
   Evry_Item *it;
   if (base)
     {
	it = base;
     }
   else
     {
	it = E_NEW(Evry_Item, 1);
	if (!it) return NULL;
     }

   if (p && EVRY_ITEM(p)->subtype)
     it->type = eina_stringshare_ref(EVRY_ITEM(p)->subtype);
   it->plugin = p;

   if (label) it->label = eina_stringshare_add(label);
   it->free = cb_free;
   it->icon_get = icon_get;

   it->ref = 1;
   /* it->usage = -1; */

#ifdef CHECK_REFS
   item_cnt++;
#endif

   return it;
}

EAPI void
evry_item_free(Evry_Item *it)
{
   if (!it) return;

   it->ref--;

#ifdef CHECK_REFS
   printf("%d, %d\t free: %s\n", it->ref, item_cnt - 1, it->label);
#endif

   if (it->ref > 0) return;

#ifdef CHECK_REFS
   item_cnt--;
#endif

   if (it->label) eina_stringshare_del(it->label);
   if (it->id) eina_stringshare_del(it->id);
   if (it->context) eina_stringshare_del(it->context);
   if (it->detail) eina_stringshare_del(it->detail);
   if (it->type) eina_stringshare_del(it->type);

   if (it->free)
     it->free(it);
   else
     E_FREE(it);
}

EAPI int
evry_item_type_check(const Evry_Item *it, const char *type, const char *subtype)
{
   int ok = 0;

   if (it)
     {
	if (type)
	  {
	     if (it->type && type)
	       ok = (!strcmp(it->type, type));
	  }

	if (!(type && !ok) || subtype)
	  {
	     if (it->subtype && subtype)
	       ok = (!strcmp(it->subtype, subtype));
	  }
     }

   return ok;
}

static Evry_Selector *
_evry_selector_for_plugin_get(Evry_Plugin *p)
{
   Evry_State *s;
   int i;

   for (i = 0; i < 3; i++)
     {
	s = selectors[i]->state;
	if (s && eina_list_data_find(s->plugins, p))
	  return selectors[i];
     }

   return NULL;
}

static int
_evry_timer_cb_actions_get(void *data)
{
   Evry_Item *it = data;
   Evry_Selector *sel = selectors[1];
   Evry_State *s;

   sel->update_timer = NULL;

   _evry_selector_actions_get(it);
   _evry_selector_update(sel);

   if (selector == sel && selector->state)
     {
	s = sel->state;
	if (s->view)
	  s->view->update(s->view, 0);
	else
	  _evry_view_update(s, NULL);
     }

   return 0;
}

static void
_evry_selector_update_actions(Evry_Selector *sel)
{
   Evry_Item *it = sel->state->cur_item;
   sel = selectors[1];
   if (sel->update_timer)
     ecore_timer_del(sel->update_timer);

   _evry_timer_cb_actions_get(it);
   /* sel->update_timer = ecore_timer_add(0.1, _evry_timer_cb_actions_get, it); */
}

EAPI void
evry_item_select(const Evry_State *state, Evry_Item *it)
{
   Evry_State *s = (Evry_State *)state;
   Evry_Selector *sel = selector;

   if (!s && it)
     {
   	sel = _evry_selector_for_plugin_get(it->plugin);
   	s = sel->state;
     }
   if (!s) return;

   s->plugin_auto_selected = EINA_FALSE;
   s->item_auto_selected = EINA_FALSE;

   _evry_item_sel(s, it);

   if (s == sel->state)
     {
	_evry_selector_update(sel);
	if (selector ==  selectors[0])
	  _evry_selector_update_actions(sel);
     }
}

EAPI void
evry_item_mark(const Evry_State *state, Evry_Item *it, Eina_Bool mark)
{
   Evry_State *s = (Evry_State *)state;

   if (mark && !it->marked)
     {
	it->marked = EINA_TRUE;
	s->sel_items = eina_list_append(s->sel_items, it);
     }
   else if (it->marked)
     {
	it->marked = EINA_FALSE;
	s->sel_items = eina_list_remove(s->sel_items, it);
     }
}

EAPI void
evry_item_ref(Evry_Item *it)
{
   it->ref++;
}

EAPI int
evry_list_win_show(void)
{
   if (list->visible) return 0;

   _evry_list_win_show();
   return 1;
}

EAPI void
evry_list_win_hide(void)
{
   _evry_list_win_clear(1);
}

EAPI void
evry_plugin_async_update(Evry_Plugin *p, int action)
{
   Evry_State *s;
   Evry_Plugin *agg;
   Evry_Selector *sel;

   if (!win) return;

   DBG("plugin: %s", p->name);

   sel = _evry_selector_for_plugin_get(p);
   if (!sel || !sel->state) return;

   s = sel->state;
   agg = sel->aggregator;

   if (action == EVRY_ASYNC_UPDATE_ADD)
     {
	if (!p->items)
	  {
	     /* remove plugin */
	     if (!eina_list_data_find(s->cur_plugins, p)) return;

	     s->cur_plugins = eina_list_remove(s->cur_plugins, p);

	     if (s->plugin == p)
	       _evry_plugin_select(s, NULL);
	  }
	else
	  {
	     /* add plugin to current plugins*/
	     _evry_plugin_list_insert(s, p);

	     if (!s->plugin || !eina_list_data_find_list(s->cur_plugins, s->plugin))
	       _evry_plugin_select(s, NULL);
	  }

	/* update aggregator */
	if ((eina_list_count(s->cur_plugins) > 1 ) ||
	    /* add aggregator for actions */
	    (sel == selectors[1] &&
	     (eina_list_count(s->cur_plugins) > 0 )))
	  {
	     /* add aggregator */
	     if (!(s->cur_plugins->data == agg))
	       {
		  s->cur_plugins = eina_list_prepend(s->cur_plugins, agg);

		  if (s->plugin_auto_selected)
		    _evry_plugin_select(s, NULL);
	       }
	     agg->fetch(agg, s->input[0] ? s->input : NULL);
	  }
	else
	  {
	     if (s->cur_plugins && s->cur_plugins->data == agg)
	       {
		  agg->finish(agg);
		  s->cur_plugins = eina_list_remove(s->cur_plugins, agg);
	       }
	  }

	if (s->sel_items)
	  eina_list_free(s->sel_items);
	s->sel_items = NULL;

	/* plugin is visible */
	if ((s->plugin == p) || (s->plugin == agg))
	  {
	     _evry_selector_update(sel);
	  }

	_evry_view_update(s, NULL);
     }
   else if (action == EVRY_ASYNC_UPDATE_REFRESH)
     {
	_evry_view_clear(s);
	_evry_view_update(s, NULL);
     }
}


/* local subsystem functions */

static Evry_List_Window *
_evry_list_win_new(E_Zone *zone)
{
   int x, y, w, mw, mh;
   Evry_List_Window *list_win;
   E_Popup *popup;
   Evas_Object *o;
   const char *tmp;
   int offset_x, offset_y, offset_s = 0;

   if (!evry_conf->views) return NULL;

   popup = e_popup_new(zone, 0, 0, 1, 1);
   if (!popup) return NULL;

   list_win = E_NEW(Evry_List_Window, 1);
   if (!list_win)
     {
	e_object_del(E_OBJECT(popup));
	return NULL;
     }
   list_win->popup = popup;

   o = edje_object_add(popup->evas);
   list_win->o_main = o;
   e_theme_edje_object_set(o, "base/theme/everything",
			   "e/modules/everything/list");

   tmp = edje_object_data_get(o, "offset_x");
   offset_x = tmp ? atoi(tmp) : 0;
   tmp = edje_object_data_get(o, "offset_y");
   offset_y = tmp ? atoi(tmp) : 0;

   if (e_config->use_composite)
     {
	edje_object_signal_emit(o, "e,state,composited", "e");
	edje_object_message_signal_process(o);
	edje_object_calc_force(o);

	tmp = edje_object_data_get(o, "shadow_offset");
	offset_s = tmp ? atoi(tmp) : 0;
	offset_y -= offset_s;
     }

   edje_object_size_min_calc(o, &mw, &mh);

   if (mh == 0) mh = 200;
   if (mw == 0) mw = win->popup->w / 2;

   evry_conf->min_h = mh;
   if (evry_conf->height > mh)
     mh = evry_conf->height;

   x =  win->popup->x + offset_x;
   y = (win->popup->y + win->popup->h) + offset_y;

   w = win->popup->w - offset_x*2;
   mh += offset_s;
   e_popup_move_resize(popup, x, y, w, mh);

   o = list_win->o_main;
   evas_object_move(o, 0, 0);
   evas_object_resize(o, list_win->popup->w, list_win->popup->h);
   evas_object_show(o);
   e_popup_edje_bg_object_set(popup, o);

   return list_win;
}

static void
_evry_list_win_free(Evry_List_Window *list_win)
{
   e_popup_hide(list_win->popup);
   evas_event_freeze(list_win->popup->evas);
   evas_object_del(list_win->o_main);
   e_object_del(E_OBJECT(list_win->popup));

   E_FREE(list_win);
}

static void
_evry_list_win_show(void)
{
   if (list->visible) return;

   list->visible = EINA_TRUE;
   _evry_list_win_update(selector->state);

   edje_object_signal_emit(list->o_main, "e,state,list_show", "e");
   edje_object_signal_emit(list->o_main, "e,state,entry_show", "e");
}

static void
_evry_list_win_clear(int hide)
{
   if (!list->visible) return;

   if (selector->state)
     _evry_view_clear(selector->state);

   if (hide)
     {
	list->visible = EINA_FALSE;
	edje_object_signal_emit(list->o_main, "e,state,list_hide", "e");
	if (evry_conf->hide_input &&
	    (!selector->state || !selector->state->input ||
	     strlen(selector->state->input) == 0))
	  edje_object_signal_emit(list->o_main, "e,state,entry_hide", "e");
     }
}

static Evry_Window *
_evry_window_new(E_Zone *zone)
{
   int x, y, mw, mh;
   Evry_Window *win;
   E_Popup *popup;
   Evas_Object *o;
   const char *tmp;
   int offset_s = 0;

   popup = e_popup_new(zone, 0, 0, 1, 1);
   if (!popup) return NULL;

   win = E_NEW(Evry_Window, 1);
   if (!win)
     {
	e_object_del(E_OBJECT(popup));
	return NULL;
     }

   win->popup = popup;

   o = edje_object_add(popup->evas);
   win->o_main = o;
   e_theme_edje_object_set(o, "base/theme/everything",
			   "e/modules/everything/main");

   if (e_config->use_composite)
     {
	edje_object_signal_emit(o, "e,state,composited", "e");
	edje_object_message_signal_process(o);
	edje_object_calc_force(o);

	tmp = edje_object_data_get(o, "shadow_offset");
	offset_s = tmp ? atoi(tmp) : 0;
     }

   edje_object_size_min_calc(o, &mw, &mh);

   if (evry_conf->width > mw)
     mw = evry_conf->width;
   evry_conf->width = mw;

   mw += offset_s*2;
   mh += offset_s*2;
   x = (zone->w * evry_conf->rel_x) - (mw / 2);
   y = (zone->h * evry_conf->rel_y) - (mh / 2);

   e_popup_move_resize(popup, x, y, mw, mh);

   o = win->o_main;
   e_popup_edje_bg_object_set(win->popup, o);
   evas_object_move(o, 0, 0);
   evas_object_resize(o, mw, mh);
   evas_object_show(o);

   ecore_x_netwm_window_type_set(popup->evas_win, ECORE_X_WINDOW_TYPE_UTILITY);

   return win;
}

static void
_evry_window_free(Evry_Window *win)
{
   e_popup_hide(win->popup);
   evas_event_freeze(win->popup->evas);
   evas_object_del(win->o_main);
   /* evas_event_thaw(win->popup->evas); */
   e_object_del(E_OBJECT(win->popup));
   E_FREE(win);
}

static Evry_Selector *
_evry_selector_new(int type)
{
   Plugin_Config *pc;
   Eina_List *l, *pcs;
   Evry_Selector *sel = E_NEW(Evry_Selector, 1);
   Evas_Object *o = edje_object_add(win->popup->evas);
   sel->o_main = o;
   e_theme_edje_object_set(o, "base/theme/everything",
			   "e/modules/everything/selector_item");
   evas_object_show(o);

   sel->aggregator = evry_plug_aggregator_new(sel, type);

   if (type == EVRY_PLUGIN_SUBJECT)
     {
	sel->history = evry_hist->subjects;
	sel->actions = evry_plug_actions_new(sel, type);
	edje_object_part_swallow(win->o_main, "e.swallow.subject_selector", o);
	pcs = evry_conf->conf_subjects;
     }
   else if (type == EVRY_PLUGIN_ACTION)
     {
	sel->history = evry_hist->actions;
	sel->actions = evry_plug_actions_new(sel, type);
	edje_object_part_swallow(win->o_main, "e.swallow.action_selector", o);
	pcs = evry_conf->conf_actions;
     }
   else if (type == EVRY_PLUGIN_OBJECT)
     {
	sel->history = evry_hist->subjects;
	edje_object_part_swallow(win->o_main, "e.swallow.object_selector", o);
	pcs = evry_conf->conf_objects;
     }

   EINA_LIST_FOREACH(pcs, l, pc)
     {
	if (!pc->enabled && !win->plugin_dedicated) continue;
	if (!pc->plugin) continue;
	if (pc->plugin == sel->aggregator) continue;
	sel->plugins = eina_list_append(sel->plugins, pc->plugin);
     }

   return sel;
}

static void
_evry_selector_free(Evry_Selector *sel)
{
   if (sel->do_thumb)
     e_thumb_icon_end(sel->o_thumb);
   if (sel->o_thumb)
     evas_object_del(sel->o_thumb);
   if (sel->o_icon)
     evas_object_del(sel->o_icon);
   if (sel->o_main)
     evas_object_del(sel->o_main);

   if (list->visible && (sel == selector))
     _evry_view_clear(sel->state);

   while (sel->states)
     _evry_state_pop(sel);

   EVRY_PLUGIN_FREE(sel->aggregator);
   EVRY_PLUGIN_FREE(sel->actions);

   if (sel->plugins) eina_list_free(sel->plugins);

   if (sel->update_timer)
     ecore_timer_del(sel->update_timer);

   E_FREE(sel);
}

static void
_evry_selector_activate(Evry_Selector *sel)
{
   Evry_State *s;

   if (selector)
     {
	s = selector->state;

	edje_object_signal_emit(selector->o_main, "e,state,unselected", "e");
	edje_object_part_text_set(selector->o_main, "e.text.plugin", "");

	if (s && s->view)
	  {
	     /* s->view->clear(s->view, 0); */
	     _evry_view_hide(s->view, 0);
	  }

	_evry_list_win_clear(evry_conf->hide_list);
     }

   selector = sel;
   s = selector->state;

   edje_object_signal_emit(sel->o_main, "e,state,selected", "e");

   if (s)
     {
	_evry_update_text_label(s);

	if (sel->state->cur_item)
	  edje_object_part_text_set(sel->o_main, "e.text.plugin",
				    EVRY_ITEM(s->cur_item->plugin)->label);

	_evry_view_show(s->view);
	_evry_list_win_update(s);
     }
}

static void
_evry_selector_thumb_gen(void *data, Evas_Object *obj, void *event_info)
{
   Evas_Coord w, h;
   Evry_Selector *sel = data;

   if (sel->o_icon)
     {
       evas_object_del(sel->o_icon);
       sel->o_icon = NULL;
     }

   e_icon_size_get(sel->o_thumb, &w, &h);
   edje_extern_object_aspect_set(sel->o_thumb, EDJE_ASPECT_CONTROL_BOTH, w, h);
   edje_object_part_swallow(sel->o_main, "e.swallow.thumb", sel->o_thumb);
   evas_object_show(sel->o_thumb);
   edje_object_signal_emit(sel->o_main, "e,action,thumb,show", "e");
   sel->do_thumb = EINA_FALSE;
}

static int
_evry_selector_thumb(Evry_Selector *sel, const Evry_Item *it)
{
   Evas_Coord w, h;

   if (sel->do_thumb)
     e_thumb_icon_end(sel->o_thumb);

   if (sel->o_thumb)
     evas_object_del(sel->o_thumb);
   sel->o_thumb = NULL;

   if (it->type != thumb_types) return 0;

   GET_FILE(file, it);

   if (!file->path || !file->mime) return 0;

   if (!strncmp(file->mime, "image/", 6))
     {
   	sel->o_thumb = e_thumb_icon_add(win->popup->evas);
   	evas_object_smart_callback_add(sel->o_thumb, "e_thumb_gen", _evry_selector_thumb_gen, sel);
   	edje_object_part_geometry_get(sel->o_main, "e.swallow.thumb", NULL, NULL, &w, &h);
   	e_thumb_icon_file_set(sel->o_thumb, file->path, NULL);
   	e_thumb_icon_size_set(sel->o_thumb, w, h);
   	e_thumb_icon_begin(sel->o_thumb);
   	sel->do_thumb = EINA_TRUE;
   	return 1;
     }

   return 0;
}

static void
_evry_selector_icon_set(Evry_Selector *sel)
{
   Evry_State *s = sel->state;
   Evry_Item *it;
   Evas_Object *o;

   if (!edje_object_part_exists(sel->o_main, "e.swallow.icons")) return;

   if (sel->o_icon)
     {
	evas_object_del(sel->o_icon);
	sel->o_icon = NULL;
     }

   if (!s) return;

   it = s->cur_item;

   if (it)
     {
	if (!_evry_selector_thumb(sel, it))
	  {
	     o = evry_util_icon_get(it, win->popup->evas);

	     if (!o && it->plugin)
	       o = evry_util_icon_get(EVRY_ITEM(it->plugin), win->popup->evas);

	     if (o)
	       {
		  edje_object_part_swallow(sel->o_main, "e.swallow.icons", o);
		  evas_object_show(o);
		  sel->o_icon = o;
	       }
	  }
     }
   else if (it)
     {
   	_evry_selector_thumb(sel, it);
     }

   if (!sel->o_icon && s->plugin && EVRY_ITEM(s->plugin)->icon)
     {
	o = evry_icon_theme_get(EVRY_ITEM(s->plugin)->icon, win->popup->evas);
	if (o)
	  {
	     edje_object_part_swallow(sel->o_main, "e.swallow.icons", o);
	     evas_object_show(o);
	     sel->o_icon = o;
	  }
     }
}

static void
_evry_selector_update(Evry_Selector *sel)
{
   Evry_State *s = sel->state;
   Evry_Item *it = NULL;
   Eina_Bool item_changed = EINA_FALSE;

   if (s)
     {
	it = s->cur_item;

	if (!s->plugin && it)
	  _evry_item_desel(s, NULL);
	else if (it && !eina_list_data_find_list(s->plugin->items, it))
	  _evry_item_desel(s, NULL);

	it = s->cur_item;

	if (s->plugin && (!it || s->item_auto_selected))
	  {
	     it = NULL;

	     /* get first item */
	     if (!it && s->plugin->items)
	       {
		  it = s->plugin->items->data;
	       }

	     if (it)
	       {
		  s->item_auto_selected = EINA_TRUE;
		  _evry_item_sel(s, it);
	       }
	     item_changed = EINA_TRUE;
	  }
     }

   _evry_selector_icon_set(sel);

   if (it)
     {
	edje_object_part_text_set(sel->o_main, "e.text.label", it->label);

	if (sel == selector)
	  edje_object_part_text_set(sel->o_main, "e.text.plugin", EVRY_ITEM(it->plugin)->label);
	else
	  edje_object_part_text_set(sel->o_main, "e.text.plugin", "");
     }
   else
     {
	/* no items for this state - clear selector */
	edje_object_part_text_set(sel->o_main, "e.text.label", "");
	if (sel == selector && s && s->plugin)
	  edje_object_part_text_set(sel->o_main, "e.text.plugin", EVRY_ITEM(s->plugin)->label);
	else
	  edje_object_part_text_set(sel->o_main, "e.text.plugin", "");
     }

   if (item_changed && sel == selectors[0])
     {
	_evry_selector_update_actions(sel);
     }
}

static void
_evry_list_win_update(Evry_State *s)
{
   if (s != selector->state) return;
   if (!list->visible) return;

   if (s->changed)
     _evry_view_update(s, s->plugin);
}

static int
_evry_selector_subjects_get(const char *plugin_name)
{
   Eina_List *l, *plugins = NULL;
   Evry_Plugin *p, *plugin;
   Evry_Selector *sel = selectors[0];

   EINA_LIST_FOREACH(sel->plugins, l, plugin)
     {
	if (plugin_name && strcmp(plugin_name, plugin->name))
	  continue;

	if (plugin->begin)
	  {
	     if ((p = plugin->begin(plugin, NULL)))
	       plugins = eina_list_append(plugins, p);
	  }
	else
	  plugins = eina_list_append(plugins, plugin);
     }

   if (!plugins) return 0;

   _evry_state_new(sel, plugins);
   _evry_matches_update(sel, 1);

   return 1;
}

static int
_evry_selector_actions_get(Evry_Item *it)
{
   Eina_List *l, *plugins = NULL;
   Evry_Plugin *p, *pp;
   Evry_Selector *sel = selectors[1];

   while (sel->state)
     _evry_state_pop(sel);

   if (!it) return 0;

   EINA_LIST_FOREACH(sel->plugins, l, p)
     {
	if (p->begin)
	  {
	     if ((pp = p->begin(p, it)))
	       plugins = eina_list_append(plugins, pp);
	  }
     }

   if (!plugins) return 0;

   _evry_state_new(sel, plugins);
   _evry_matches_update(sel, 1);

   return 1;
}

/* find plugins that provide the second item required for an action */
static int
_evry_selector_objects_get(Evry_Action *act)
{
   Eina_List *l, *plugins = NULL;
   Evry_Plugin *p, *pp;
   Evry_Selector *sel = selectors[2];
   Evry_Item *it;
   /* required type */
   /* const char *type_in = act->type_in2; */

   while (sel->state)
     _evry_state_pop(sel);

   it = selectors[0]->state->cur_item;

   EINA_LIST_FOREACH(sel->plugins, l, p)
     {
	printf("check %s %s\n", EVRY_ITEM(p)->subtype, act->type_in2);

	if (!evry_item_type_check(EVRY_ITEM(p), NULL, act->type_in2))
	  continue;

	if (p->begin)
	  {
	     if ((pp = p->begin(p, it)) || (pp = p->begin(p, NULL)))
	       plugins = eina_list_append(plugins, pp);
	  }
	else
	  {
	     plugins = eina_list_append(plugins, pp);
	  }
     }

   if (!plugins) return 0;

   _evry_state_new(sel, plugins);
   _evry_matches_update(sel, 1);

   return 1;
}

static Evry_State *
_evry_state_new(Evry_Selector *sel, Eina_List *plugins)
{
   Evry_State *s = E_NEW(Evry_State, 1);
   s->inp = malloc(INPUTLEN);
   s->inp[0] = 0;
   s->input = s->inp;

   s->plugins = plugins;

   sel->states = eina_list_prepend(sel->states, s);
   sel->state = s;

   return s;
}

static void
_evry_state_pop(Evry_Selector *sel)
{
   Evry_Plugin *p;
   Evry_State *s;

   s = sel->state;

   _evry_item_desel(s, NULL);

   free(s->inp);

   if (s->view)
     s->view->destroy(s->view);

   EINA_LIST_FREE(s->plugins, p)
     p->finish(p);

   /* sel->aggregator->finish(sel->aggregator); */

   if (s->sel_items)
     eina_list_free(s->sel_items);

   E_FREE(s);

   sel->states = eina_list_remove_list(sel->states, sel->states);

   if (sel->states)
     sel->state = sel->states->data;
   else
     sel->state = NULL;
}

int
evry_browse_item(Evry_Selector *sel)
{
   if (!sel) sel = selector;
   Evry_State *s = sel->state;
   Evry_Item *it;
   Eina_List *l, *plugins = NULL;
   Evry_Plugin *p, *pp;
   Evry_View *view = NULL;

   if (!s)
     return 0;

   it = s->cur_item;

   if (!it || !it->browseable)
     return 0;

   if (it->plugin->begin && (pp = it->plugin->begin(it->plugin, it)))
     plugins = eina_list_append(plugins, pp);

   if (!plugins)
     {
	EINA_LIST_FOREACH(sel->plugins, l, p)
	  {
	     if (p == it->plugin)
	       continue;

	     if (!p->begin)
	       continue;

	     if ((pp = p->begin(p, it)))
	       plugins = eina_list_append(plugins, pp);
	  }
     }

   if (!plugins)
     return 0;

   evry_history_add(sel->history, s->cur_item, NULL, s->input);

   if (s->view)
     {
	_evry_view_hide(s->view, 1);
	view = s->view;
     }

   _evry_state_new(sel, plugins);
   _evry_matches_update(sel, 1);
   _evry_selector_update(sel);
   s = sel->state;

   if (view && list->visible && s)
     {
	s->view = view->create(view, s, list->o_main);
	if (s->view)
	  {
	     _evry_view_show(s->view);
	     s->view->update(s->view, -1);
	  }
     }

   _evry_update_text_label(sel->state);

   return 1;
}

EAPI int
evry_browse_back(Evry_Selector *sel)
{
   Evry_State *s = sel->state;

   if (!s || !sel->states->next)
     return 0;

   _evry_state_pop(sel);

   s = sel->state;
   sel->aggregator->fetch(sel->aggregator, s->input);
   _evry_selector_update(sel);
   _evry_update_text_label(s);
   _evry_view_show(s->view);
   s->view->update(s->view, 1);

   return 1;
}

static void
_evry_selectors_switch(int dir)
{
   Evry_State *s = selector->state;

   if (_show_timer)
     _cb_show_timer(NULL);

   if (selector->update_timer)
     {
	if ((selector == selectors[0]) ||
	    (selector == selectors[1]))
	  {
	     _evry_matches_update(selector, 0);
	     _evry_selector_update(selector);
	  }
     }

   if (selector == selectors[0] && dir > 0)
     {
	if (s->cur_item)
	  _evry_selector_activate(selectors[1]);
     }
   else if (selector == selectors[1] && dir > 0)
     {
	int next_selector = 0;
	Evry_Item *it;

	if ((it = s->cur_item) &&
	    (it->plugin == selector->actions))
	  {
	     GET_ACTION(act,it);
	     if (act->type_in2)
	       {
		  _evry_selector_objects_get(act);
		  _evry_selector_update(selectors[2]);
		  edje_object_signal_emit(win->o_main, "e,state,object_selector_show", "e");
		  next_selector = 2;
	       }
	  }
	_evry_selector_activate(selectors[next_selector]);
     }
   else if (selector == selectors[1] && dir < 0)
     {
	  _evry_selector_activate(selectors[0]);
	  edje_object_signal_emit(win->o_main, "e,state,object_selector_hide", "e");
     }
   else if (selector == selectors[2] && dir > 0)
     {
	while (selector->states)
	  _evry_state_pop(selector);

	edje_object_signal_emit(win->o_main, "e,state,object_selector_hide", "e");
	_evry_selector_activate(selectors[0]);
     }
   else if (selector == selectors[2] && dir < 0)
     {
	_evry_selector_activate(selectors[1]);
     }
}

static int
_evry_cb_key_down(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_Event_Key *ev = event;
   Evry_State *s;
   const char *key = NULL, *old;

   if (ev->event_window != input_window)
     return 1;

   if (!strcmp(ev->key, "Escape"))
     {
	evry_hide();
	return 1;
     }

   if (!selector || !selector->state)
     return 1;
   s = selector->state;

   win->request_selection = EINA_FALSE;

   old = ev->key;

   if (((evry_conf->quick_nav == 1) && (ev->modifiers & ECORE_EVENT_MODIFIER_ALT)) ||
       ((evry_conf->quick_nav == 2) && (ev->modifiers & ECORE_EVENT_MODIFIER_CTRL)))
     {
	if (!strcmp(ev->key, "k") || (!strcmp(ev->key, "K")))
	  key = eina_stringshare_add("Up");
	else if (!strcmp(ev->key, "j") || (!strcmp(ev->key, "J")))
	  key = eina_stringshare_add("Down");
	else if (!strcmp(ev->key, "n") || (!strcmp(ev->key, "N")))
	  key = eina_stringshare_add("Next");
	else if (!strcmp(ev->key, "p") || (!strcmp(ev->key, "P")))
	  key = eina_stringshare_add("Prior");
	else if (!strcmp(ev->key, "l") || (!strcmp(ev->key, "L")))
	  key = eina_stringshare_add("Right");
	else if (!strcmp(ev->key, "h") || (!strcmp(ev->key, "H")))
	  key = eina_stringshare_add("Left");
	else if (!strcmp(ev->key, "i") || (!strcmp(ev->key, "I")))
	  key = eina_stringshare_add("Tab");
	else if (!strcmp(ev->key, "m") || (!strcmp(ev->key, "M")))
	  key = eina_stringshare_add("Return");
	else
	  key = eina_stringshare_add(ev->key);

	ev->key = key;
     }
   else if (((evry_conf->quick_nav == 3) && (ev->modifiers & ECORE_EVENT_MODIFIER_ALT)) ||
	    ((evry_conf->quick_nav == 4) && (ev->modifiers & ECORE_EVENT_MODIFIER_CTRL)))
     {
	if (!strcmp(ev->key, "p") || (!strcmp(ev->key, "P")))
	  key = eina_stringshare_add("Up");
	else if (!strcmp(ev->key, "n") || (!strcmp(ev->key, "N")))
	  key = eina_stringshare_add("Down");
	else if (!strcmp(ev->key, "f") || (!strcmp(ev->key, "F")))
	  key = eina_stringshare_add("Right");
	else if (!strcmp(ev->key, "b") || (!strcmp(ev->key, "B")))
	  key = eina_stringshare_add("Left");
	else if (!strcmp(ev->key, "i") || (!strcmp(ev->key, "I")))
	  key = eina_stringshare_add("Tab");
	else if (!strcmp(ev->key, "m") || (!strcmp(ev->key, "M")))
	  key = eina_stringshare_add("Return");
	else
	  key = eina_stringshare_add(ev->key);

	ev->key = key;
     }
   else

     {
	key = eina_stringshare_add(ev->key);
	ev->key = key;
     }

   if (!list->visible && (!strcmp(key, "Down")))
     _evry_list_win_show();
   else if ((!strcmp(key, "ISO_Left_Tab") ||
	     (((ev->modifiers & ECORE_EVENT_MODIFIER_CTRL) ||
	       (ev->modifiers & ECORE_EVENT_MODIFIER_SHIFT)) &&
	      (!strcmp(key, "Tab")))))
     {
	int action = 0;
	char *input = NULL;
	Evry_Item *it = s->cur_item;

	evry_item_ref(it);

	s->item_auto_selected = EINA_FALSE;

	if (it->plugin->complete)
	  action = it->plugin->complete(it->plugin, it, &input);
	else
	  evry_browse_item(selector);

	if (input && action == EVRY_COMPLETE_INPUT)
	  {
	     snprintf(s->input, INPUTLEN - 1, "%s", input);
	     _evry_update_text_label(s);
	     _evry_cb_update_timer(selector);
	     evry_item_select(s, it);
	  }

	E_FREE(input);

	evry_item_free(it);
     }
   else if ((ev->modifiers & ECORE_EVENT_MODIFIER_CTRL) &&
	    (!strcmp(key, "Delete") || !strcmp(key, "Insert")))
     {
	if (!s || !s->cur_item)
	  goto end;

	int delete = (!strcmp(key, "Delete") && (ev->modifiers & ECORE_EVENT_MODIFIER_SHIFT));
	int promote = (!strcmp(key, "Insert"));
	History_Entry *he;
	History_Item *hi;
	Eina_List *l, *ll;
	Evry_Item *it = s->cur_item;

	if (!(he = eina_hash_find
	      (selector->history, (it->id ? it->id : it->label))))
	  goto end;

	EINA_LIST_FOREACH_SAFE(he->items, l, ll, hi)
	  {
	     if (hi->plugin != it->plugin->name)
	       continue;

	     if (delete)
	       {
		  if (hi->input)
		    eina_stringshare_del(hi->input);
		  if (hi->plugin)
		    eina_stringshare_del(hi->plugin);
		  if (hi->context)
		    eina_stringshare_del(hi->context);
		  E_FREE(hi);

		  he->items = eina_list_remove_list(he->items, l);
	       }
	     else if (promote)
	       {
		  hi->count += 5;
		  hi->last_used = ecore_time_get();
	       }
	     else
	       {
		  hi->count -= 5;
		  if (hi->count < 0) hi->count = 1;
	       }
	  }
	if (s->plugin == selector->aggregator)
	  selector->aggregator->fetch(selector->aggregator, s->input);
	if (s->view)
	  s->view->update(s->view, 0);
     }
   else if (ev->modifiers & ECORE_EVENT_MODIFIER_CTRL)
     {
	if (!strcmp(key, "u"))
	  {
	     if (!_evry_clear(selector))
	       evry_browse_back(selector);
	  }
	else if (!strcmp(key, "1"))
	  _evry_view_toggle(s, NULL);
	else if (!strcmp(key, "Return"))
	  _evry_plugin_action(selector, 0);
	else if (!strcmp(key, "v"))
	  {
	     win->request_selection = EINA_TRUE;
	     ecore_x_selection_primary_request
	       (win->popup->evas_win, ECORE_X_SELECTION_TARGET_UTF8_STRING);
	  }
	else if (_evry_view_key_press(s, ev))
	  goto end;
     }
   /* let plugin intercept keypress */
   else if (s->plugin && s->plugin->cb_key_down &&
	    s->plugin->cb_key_down(s->plugin, ev))
     goto end;
   /* let view intercept keypress */
   else if (_evry_view_key_press(s, ev))
     goto end;
   else if (!strcmp(key, "Right"))
     {
	if (!evry_browse_item(selector) &&
	    (selector != selectors[2]))
	  _evry_selectors_switch(1);
     }
   else if (!strcmp(key, "Left"))
     {
	if (!evry_browse_back(selector))
	  _evry_selectors_switch(-1);
     }
   else if (!strcmp(key, "Return"))
     {
	if (ev->modifiers & ECORE_EVENT_MODIFIER_SHIFT)
	  _evry_plugin_action(selector, 0);
	else /*if (!_evry_browse_item(selector))*/
	  _evry_plugin_action(selector, 1);
     }
   else if (!strcmp(key, "Tab"))
     _evry_selectors_switch(1);
   else if (!strcmp(key, "BackSpace"))
     {
	if (!_evry_backspace(selector))
	  evry_browse_back(selector);
     }
   else if (_evry_view_key_press(s, ev))
     goto end;
   else if ((ev->compose && !(ev->modifiers & ECORE_EVENT_MODIFIER_ALT)))
     {
	int len = strlen(s->inp);

	if (len == 0 && (_evry_view_toggle(s, ev->compose)))
	  goto end;

	if (len < (INPUTLEN - strlen(ev->compose)))
	  {
	     strcat(s->inp, ev->compose);

	     _evry_update(selector, 1);
	  }
     }

 end:
   eina_stringshare_del(ev->key);
   ev->key = old;
   return 1;
}

static int
_evry_backspace(Evry_Selector *sel)
{
   Evry_State *s = sel->state;
   int len, val, pos;

   len = strlen(s->inp);
   if (len > 0)
     {
	pos = evas_string_char_prev_get(s->inp, len, &val);
	if ((pos < len) && (pos >= 0))
	  {
	     val = *(s->inp + pos);

	     s->inp[pos] = 0;

	     if (s->trigger_active && s->inp[0] != 0)
	       s->input = s->inp + 1;
	     else
	       s->input = s->inp;

	     if ((pos == 0) || !isspace(val))
	       _evry_update(sel, 1);

	     return 1;
	  }
     }

   return 0;
}

static void
_evry_update_text_label(Evry_State *s)
{
   if (!list->visible && evry_conf->hide_input)
     {
	if (strlen(s->inp) > 0)
	  edje_object_signal_emit(list->o_main, "e,state,entry_show", "e");
	else
	  edje_object_signal_emit(list->o_main, "e,state,entry_hide", "e");
     }

   edje_object_part_text_set(win->o_main, "e.text.label", s->inp);
   edje_object_part_text_set(list->o_main, "e.text.label", s->inp);

}

static void
_evry_update(Evry_Selector *sel, int fetch)
{
   Evry_State *s = sel->state;

   _evry_update_text_label(s);

   if (fetch)
     {
	if (sel->update_timer)
	  ecore_timer_del(sel->update_timer);

	sel->update_timer = ecore_timer_add(MATCH_LAG, _evry_cb_update_timer, sel);

	if (s->plugin && !s->plugin->trigger)
	  edje_object_signal_emit(list->o_main, "e,signal,update", "e");
     }
}

static int
_evry_cb_update_timer(void *data)
{
   Evry_Selector *sel = data;

   _evry_matches_update(sel, 1);
   _evry_selector_update(sel);
   _evry_list_win_update(sel->state);
   sel->update_timer = NULL;

   return 0;
}

static int
_evry_clear(Evry_Selector *sel)
{
   Evry_State *s = sel->state;

   if (s->inp && s->inp[0] != 0)
     {
	if (s->trigger_active && s->inp[1] != 0)
	  {
	     s->inp[1] = 0;
	     s->input = s->inp + 1;
	  }
	else
	  {
	     s->inp[0] = 0;
	     s->input = s->inp;
	  }

	_evry_update(sel, 1);
	if (!list->visible && evry_conf->hide_input)
	  edje_object_signal_emit(list->o_main, "e,state,entry_hide", "e");
	return 1;
     }
   return 0;
}

static void
_evry_plugin_action(Evry_Selector *sel, int finished)
{
   Evry_State *s_subj, *s_act, *s_obj = NULL;
   Evry_Item *it_subj, *it_act, *it_obj, *it;
   Eina_List *l;

   if (selectors[0]->update_timer)
     {
	_evry_matches_update(selectors[0], 0);
	_evry_selector_update(selectors[0]);
     }

   if (!(s_subj = selectors[0]->state))
     return;

   if (!(it_subj = s_subj->cur_item))
     return;

   if (selector == selectors[0] &&
       selectors[1]->update_timer)
     {
	_evry_selector_actions_get(it_subj);

	if (!selectors[1]->state)
	  return;

	_evry_selector_update(selectors[1]);
     }

   if (!(s_act = selectors[1]->state))
     return;

   if (!(it_act = s_act->cur_item))
     return;

   if (evry_item_type_check(it_act, "ACTION", NULL) ||
       evry_item_type_check(it_act, NULL, "ACTION"))
     {
	GET_ACTION(act, it_act);

	if (!act->action)
	  return;

	/* get object item for action, when required */
	if (act->type_in2)
	  {
	     /* check if object is provided */
	     if (selectors[2]->state)
	       {
		  s_obj = selectors[2]->state;
		  it_obj = s_obj->cur_item;
	       }

	     if (!it_obj)
	       {
		  if (selectors[1] == selector)
		    _evry_selectors_switch(1);
		  return;
	       }

	     act->item2 = it_obj;
	  }

	if (s_subj->sel_items)
	  {
	     EINA_LIST_REVERSE_FOREACH(s_subj->sel_items, l, it)
	       {
		  if (it->type != act->type_in1)
		    continue;
		  act->item1 = it;
		  act->action(act);
	       }
	  }
	else if (s_obj && s_obj->sel_items)
	  {
	     EINA_LIST_FOREACH(s_obj->sel_items, l, it)
	       {
		  if (it->type != act->type_in2)
		    continue;
		  act->item2 = it;
		  act->action(act);
	       }
	  }
	else
	  {
	     act->item1 = it_subj;
	     if (!act->action(act))
	       return;
	  }
     }
   /* else if (s_act->plugin->action)
    *   {
    * 	if (s_subj->sel_items)
    * 	  {
    * 	     EINA_LIST_REVERSE_FOREACH(s_subj->sel_items, l, it)
    * 	       {
    * 		  if (it->plugin->type_out != it_act->plugin->type_in)
    * 		    continue;
    * 		  s_act->plugin->action(s_act->plugin, it_act, it);
    * 	       }
    * 	  }
    * 	else
    * 	  {
    * 	     if (!s_act->plugin->action(s_act->plugin, it_act, it_subj))
    * 	       return;
    * 	  }
    *   } */
   else return;

   if (s_subj && it_subj)
     evry_history_add(evry_hist->subjects, it_subj, NULL, s_subj->input);

   if (s_act && it_act)
     evry_history_add(evry_hist->actions, it_act, it_subj->context, s_act->input);

   if (s_obj && it_obj)
     evry_history_add(evry_hist->subjects, it_obj, it_act->context, s_obj->input);

   /* let subject and object plugin know that an action was performed */
   /* if (s_subj->plugin->action)
    *   s_subj->plugin->action(s_subj->plugin, s_act->cur_item, s_subj->cur_item); */
   /* if (s_object && s_obj->plugin->action)
    *   s_object->plugin->action(s_obj->plugin, s_act->cur_item, s_obj->cur_item); */

   if (finished)
     evry_hide();
}

static void
_evry_view_show(Evry_View *v)
{
   if (!v) return;

   if (v->o_list)
     {
	edje_object_part_swallow(list->o_main, "e.swallow.list", v->o_list);
	evas_object_show(v->o_list);
     }

   if (v->o_bar)
     {
	edje_object_part_swallow(list->o_main, "e.swallow.bar", v->o_bar);
	evas_object_show(v->o_bar);
     }
}

static void
_evry_view_hide(Evry_View *v, int slide)
{
   if (!v) return;

   v->clear(v, slide);

   if (v->o_list)
     {
	edje_object_part_unswallow(list->o_main, v->o_list);
	evas_object_hide(v->o_list);
     }

   if (v->o_bar)
     {
	edje_object_part_unswallow(list->o_main, v->o_bar);
	evas_object_hide(v->o_bar);
     }
}

static void
_evry_view_update(Evry_State *s, Evry_Plugin *p)
{
   if (!list->visible) return;

   if (!s->view)
     {
	Evry_View *view = evry_conf->views->data;
	s->view = view->create(view, s, list->o_main);
	_evry_view_show(s->view);
     }

   if (s->view)
     s->view->update(s->view, 0);
}

static void
_evry_view_clear(Evry_State *s)
{
   if (!s || !s->view) return;

   s->view->clear(s->view, 0);
}

static int
_evry_view_key_press(Evry_State *s, Ecore_Event_Key *ev)
{
   if (!s || !s->view || !s->view->cb_key_down) return 0;

   return s->view->cb_key_down(s->view, ev);
}

static int
_evry_view_toggle(Evry_State *s, const char *trigger)
{
   Evry_View *view, *v = NULL;
   Eina_List *l, *ll;
   Eina_Bool triggered = FALSE;

   if (trigger)
     {
	EINA_LIST_FOREACH(evry_conf->views, ll, view)
	  {
	     if (view->trigger && !strncmp(trigger, view->trigger, 1) &&
		 (v = view->create(view, s, list->o_main)))
	       {
		  triggered = EINA_TRUE;
		  goto found;
	       }
	  }
     }
   else
     {
	if (s->view)
	  l = eina_list_data_find_list(evry_conf->views, s->view->id);
	else
	  {
	     view = evry_conf->views->data;
	     v = view->create(view, s, list->o_main);
	     goto found;
	  }

	if (l && l->next)
	  l = l->next;
	else
	  l = evry_conf->views;

	EINA_LIST_FOREACH(l, ll, view)
	  {
	     if ((!view->trigger) &&
		 ((view == s->view->id) ||
		  (v = view->create(view, s, list->o_main))))
	       goto found;
	  }

	EINA_LIST_FOREACH(evry_conf->views, ll, view)
	  {
	     if ((!view->trigger) &&
		 ((view == s->view->id) ||
		  (v = view->create(view, s, list->o_main))))
	       goto found;
	  }
     }

 found:
   if (!v) return 0;

   _evry_list_win_show();

   if (s->view)
     {
	_evry_view_hide(s->view, 0);
	s->view->destroy(s->view);
     }

   s->view = v;
   _evry_view_show(s->view);
   view->update(s->view, 0);

   return triggered;
}

static void
_evry_matches_update(Evry_Selector *sel, int async)
{
   Evry_State *s = sel->state;
   Evry_Plugin *p;
   Eina_List *l;
   Evry_Item *it;
   const char *input;

   s->changed = 1;

   if (s->inp[0])
     input = s->inp;
   else
     input = NULL;

   if (s->sel_items)
     eina_list_free(s->sel_items);
   s->sel_items = NULL;

   if (!input || !s->trigger_active)
     {
	EINA_LIST_FREE(s->cur_plugins, p);
	s->trigger_active = EINA_FALSE;
     }
   else
     {
	EINA_LIST_FOREACH(s->cur_plugins, l, p)
	  p->fetch(p, s->input);
     }

   if (!s->cur_plugins && input)
     {
	int len_trigger = 0;
	int len_inp = strlen(s->inp);

	EINA_LIST_FOREACH(s->plugins, l, p)
	  {
	     /* input matches plugin trigger? */
	     if (!p->config->trigger) continue;
	     int len = strlen(p->config->trigger);

	     if (len_trigger && len != len_trigger)
	       continue;

	     if ((len_inp >= len) &&
		 (!strncmp(s->inp, p->config->trigger, len)))
	       {
		  len_trigger = len;
		  s->cur_plugins = eina_list_append(s->cur_plugins, p);
		  if(len_inp == len)
		    p->fetch(p, NULL);
		  else
		    p->fetch(p, s->input + len);
	       }
	  }
	if (s->cur_plugins)
	  {
	     s->trigger_active = EINA_TRUE;
	     /* replace trigger with indicator */
	     if (len_trigger > 1)
	       {
		  s->inp[0] = '>';
		  s->inp[1] = '\0';
	       }
	     s->input = s->inp + 1;
	     _evry_update_text_label(s);
	  }
     }

   if (!s->cur_plugins)
     {
	s->input = s->inp;

	EINA_LIST_FOREACH(s->plugins, l, p)
	  {
	     if (!(win->plugin_dedicated) &&
		 (p->config->trigger_only) &&
		 (p->config->trigger))
	       continue;
	     if (p == sel->aggregator)
	       continue;
	     /* dont wait for async plugin. use their current items */
	     if (!async && p->async_fetch && p->items)
	       {
		  s->cur_plugins = eina_list_append(s->cur_plugins, p);
	       }
	     else
	       {
		  if ((p->fetch(p, input)) ||
		      (sel->states->next)  ||
		      (win->plugin_dedicated))
		    s->cur_plugins = eina_list_append(s->cur_plugins, p);
	       }
	  }
     }

   if (sel->aggregator->fetch(sel->aggregator, input))
     _evry_plugin_list_insert(s, sel->aggregator);

   if (s->plugin_auto_selected ||
       (s->plugin && (!eina_list_data_find(s->cur_plugins, s->plugin))))
     _evry_plugin_select(s, NULL);
   else
     _evry_plugin_select(s, s->plugin);

   if (sel->update_timer)
     ecore_timer_del(sel->update_timer);
   sel->update_timer = NULL;

   if (s->plugin)
     {
	EINA_LIST_FOREACH(s->plugin->items, l, it)
	  if (it->marked)
	    s->sel_items = eina_list_append(s->sel_items, it);
     }
}

static void
_evry_item_desel(Evry_State *s, Evry_Item *it)
{
   if (!it)
     it = s->cur_item;

   if (s->cur_item)
     {
	it->selected = EINA_FALSE;
	evry_item_free(it);
     }

   s->cur_item = NULL;
}

static void
_evry_item_sel(Evry_State *s, Evry_Item *it)
{
   if (s->cur_item == it) return;

   _evry_item_desel(s, NULL);

   evry_item_ref(it);
   it->selected = EINA_TRUE;

   s->cur_item = it;
}

static void
_evry_plugin_select(Evry_State *s, Evry_Plugin *p)
{
   if (!s || !s->cur_plugins) return;

   if (p) s->plugin_auto_selected = EINA_FALSE;

   if (!p && s->cur_plugins)
     {
	p = s->cur_plugins->data;
	s->plugin_auto_selected = EINA_TRUE;
     }

   if (s->plugin != p)
     _evry_item_desel(s, NULL);

   s->plugin = p;
}

void
evry_plugin_select(const Evry_State *s, Evry_Plugin *p)
{
   Evry_Selector *sel;

   _evry_plugin_select((Evry_State *) s, p);

   sel = _evry_selector_for_plugin_get(p);

   if (sel)
     _evry_selector_update(sel);
}

static void
_evry_plugin_list_insert(Evry_State *s, Evry_Plugin *p)
{
   Eina_List *l;
   Evry_Plugin *plugin;

   EINA_LIST_FOREACH(s->cur_plugins, l, plugin)
     if (p == plugin)
       return;
     else
       if (p->config->priority < plugin->config->priority)
       break;

   if (l)
     s->cur_plugins = eina_list_prepend_relative_list(s->cur_plugins, p, l);
   else
     s->cur_plugins = eina_list_append(s->cur_plugins, p);
}

static int
_evry_cb_selection_notify(void *data, int type, void *event)
{
   Ecore_X_Event_Selection_Notify *ev;
   /* FIXME Evry_Selector *sel = data; */
   Evry_State *s = selector->state;

   if (!s || (data != win)) return 1;
   if (!win->request_selection) return 1;

   win->request_selection = EINA_FALSE;

   ev = event;
   if ((ev->selection == ECORE_X_SELECTION_CLIPBOARD) ||
       (ev->selection == ECORE_X_SELECTION_PRIMARY))
     {
	if (strcmp(ev->target, ECORE_X_SELECTION_TARGET_UTF8_STRING) == 0)
	  {
	     Ecore_X_Selection_Data_Text *text_data;

	     text_data = ev->data;

	     strncat(s->input, text_data->text, (INPUTLEN - strlen(s->input)) - 1);
	     _evry_update(selector, 1);
	  }
     }

   return 1;
}
