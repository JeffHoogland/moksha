#include "e.h"
#include "e_mod_main.h"

/* TODO */
/* - animations
 * - mouse handlers
 * - show item descriptions
 * - keybinding configuration
 * - shortcuts for plugins
 */
#define INPUTLEN 40
#define MATCH_LAG 0.33
#define MAX_FUZZ 200


typedef struct _Evry_State Evry_State;
typedef struct _Evry_Window Evry_Window;
typedef struct _Evry_List_Window Evry_List_Window;
typedef struct _Evry_Selector Evry_Selector;

struct _Evry_State
{
  char *input;
  /* all available plugins for current state */
  Eina_List   *plugins;

  /* currently active plugins, i.e. those that provide items */
  Eina_List   *cur_plugins;

  /* active plugin */
  Evry_Plugin *plugin;

  /* selected item */
  Evry_Item   *sel_item;

  /* Eina_List *sel_items; */

  /* this is for the case when the current plugin was not selected
     manually and a higher priority (async) plugin retrieves
     candidates, the higher priority plugin is made current */
  Eina_Bool plugin_auto_selected;
  Eina_Bool item_auto_selected;

  Eina_List *items;
};

/* */
struct _Evry_Selector
{
  Evas_Object *o_main;
  Evas_Object *o_icon;

  /* current state */
  Evry_State  *state;

  /* stack of states (for browseable plugins) */
  Eina_List   *states;

  /* provides collection of items from other plugins */
  Evry_Plugin *aggregator;

  /* */
  Eina_List   *actions;

  /* all plugins that belong to this selector*/
  Eina_List   *plugins;
};

struct _Evry_Window
{
  E_Popup *popup;
  Evas_Object *o_main;

  Eina_Bool request_selection;
  /* E_Popup *input_win; */
};

struct _Evry_List_Window
{
  E_Popup     *popup;
  Evas_Object *o_main;
  Evas_Object *o_list;
  Evas_Object *o_tabs;

  int          ev_last_is_mouse;
  Evry_Item   *item_mouseover;
  Ecore_Animator *scroll_animator;
  Ecore_Timer *scroll_timer;
  double       scroll_align_to;
  double       scroll_align;
  Ecore_Idler *item_idler;

  Eina_Bool visible;
};


static void _evry_matches_update(Evry_Selector *sel);
static void _evry_plugin_action(Evry_Selector *sel, int finished);
static void _evry_backspace(Evry_State *s);
static void _evry_update(Evry_State *s);
static void _evry_clear(Evry_State *s);
static int  _evry_update_timer(void *data);
static Evry_State *_evry_state_new(Evry_Selector *sel, Eina_List *plugins);
static void _evry_state_pop(Evry_Selector *sel);
static void _evry_select_plugin(Evry_State *s, Evry_Plugin *p);
static void _evry_update_text_label(Evry_State *s);
static void _evry_browse_item(Evry_Selector *sel);
static void _evry_browse_back(Evry_Selector *sel);
static void _evry_selectors_switch(void);
static void _evry_plugin_list_insert(Evry_State *s, Evry_Plugin *p);

static Evry_Window *_evry_window_new(E_Zone *zone);
static void _evry_window_free(Evry_Window *win);
static Evry_Selector *_evry_selector_new(int type);
static void _evry_selector_free(Evry_Selector *sel);
static void _evry_selector_activate(Evry_Selector *sel);
static void _evry_selector_update(Evry_Selector *sel);
static void _evry_selector_icon_set(Evry_Selector *sel);
static int  _evry_selector_subjects_get(void);
static int  _evry_selector_actions_get(Evry_Item *it);
static int  _evry_selector_objects_get(const char *type);

static Evry_List_Window *_evry_list_win_new(E_Zone *zone);
static void _evry_list_win_free(Evry_List_Window *win);
static void _evry_list_win_show(void);
static void _evry_list_win_hide(void);
static void _evry_list_clear_list(Evry_State *s);
static void _evry_list_update(Evry_State *s);
static void _evry_list_show_items(Evry_State *s, Evry_Plugin *plugin);
static void _evry_list_item_next(Evry_State *s);
static void _evry_list_item_prev(Evry_State *s);
static void _evry_list_plugin_next(Evry_State *s);
static void _evry_list_plugin_prev(Evry_State *s);
static void _evry_list_plugin_next_by_name(Evry_State *s, const char *key);
static void _evry_list_scroll_to(Evry_State *s, Evry_Item *it);
static void _evry_list_item_desel(Evry_State *s, Evry_Item *it);
static void _evry_list_item_sel(Evry_State *s, Evry_Item *it);
static void _evry_list_tab_scroll_to(Evry_State *s, Evry_Plugin *p);
static void _evry_list_tab_show(Evry_State *s, Evry_Plugin *p);
static void _evry_list_tabs_update(Evry_State *s);
static int  _evry_list_animator(void *data);
static int  _evry_list_scroll_timer(void *data);
static int  _evry_list_item_idler(void *data);

static int  _evry_plug_actions_init(void);
static void _evry_plug_actions_free(void);
static int  _evry_plug_actions_begin(Evry_Plugin *p, const Evry_Item *it);
static int  _evry_plug_actions_fetch(Evry_Plugin *p, const char *input);
static void _evry_plug_actions_cleanup(Evry_Plugin *p);
static Evas_Object *_evry_plug_actions_item_icon_get(Evry_Plugin *p, const Evry_Item *it, Evas *e);

static Evry_Plugin *_evry_plug_aggregator_new(void);
static void _evry_plug_aggregator_free(Evry_Plugin *p);
static int  _evry_plug_aggregator_begin(Evry_Plugin *p, const Evry_Item *it);
static int  _evry_plug_aggregator_fetch(Evry_Plugin *p, const char *input);
static int  _evry_plug_aggregator_action(Evry_Plugin *p, const Evry_Item *item, const char *input);
static void _evry_plug_aggregator_cleanup(Evry_Plugin *p);
static Evas_Object *_evry_plug_aggregator_item_icon_get(Evry_Plugin *p, const Evry_Item *it, Evas *e);

static int  _evry_cb_key_down(void *data, int type, void *event);
static int  _evry_cb_selection_notify(void *data, int type, void *event);

/* static int  _evry_cb_mouse_down(void *data, int type, void *event);
 * static int  _evry_cb_mouse_up(void *data, int type, void *event);
 * static int  _evry_cb_mouse_move(void *data, int type, void *event);
 * static int  _evry_cb_mouse_wheel(void *data, int type, void *event);
 * static void _evry_cb_item_mouse_in(void *data, Evas *evas, Evas_Object *obj, void *event_info);
 * static void _evry_cb_item_mouse_out(void *data, Evas *evas, Evas_Object *obj, void *event_info); */

/* local subsystem globals */
static Evry_Window *win = NULL;
static Evry_List_Window *list = NULL;
static Ecore_X_Window input_window = 0;
static Eina_List   *handlers = NULL;
static Ecore_Timer *update_timer = NULL;
static Evry_Selector *selectors[3];
static Evry_Selector *selector = NULL;
static Evry_Plugin *action_selector = NULL;

/* externally accessible functions */
int
evry_init(void)
{
   _evry_plug_actions_init();

   return 1;
}

int
evry_shutdown(void)
{
   evry_hide();
   _evry_plug_actions_free();
   return 1;
}

int
evry_show(E_Zone *zone)
{
   if (win) return 0;

   E_OBJECT_CHECK_RETURN(zone, 0);
   E_OBJECT_TYPE_CHECK_RETURN(zone, E_ZONE_TYPE, 0);

   input_window = ecore_x_window_input_new(zone->container->win, zone->x,
					   zone->y, zone->w, zone->h);
   ecore_x_window_show(input_window);
   if (!e_grabinput_get(input_window, 1, input_window))
     goto error;

   win = _evry_window_new(zone);
   if (!win) goto error;

   list = _evry_list_win_new(zone);
   if (!list) goto error;

   list->visible = EINA_FALSE;

   /* TODO check NULL */
   selectors[0] = _evry_selector_new(type_subject);
   selectors[1] = _evry_selector_new(type_action);
   selectors[2] = _evry_selector_new(type_object);

   _evry_selector_subjects_get();
   _evry_selector_activate(selectors[0]);
   _evry_selector_update(selector);

   e_popup_layer_set(list->popup, 255);
   e_popup_layer_set(win->popup, 255);
   e_popup_show(win->popup);
   e_popup_show(list->popup);

   if (list->visible)
     {
	e_popup_show(list->popup);
	_evry_list_update(selector->state);
     }
   e_box_align_set(list->o_tabs, 0.0, 0.5);

   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (ECORE_EVENT_KEY_DOWN, _evry_cb_key_down, NULL));
   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (ECORE_X_EVENT_SELECTION_NOTIFY,
       _evry_cb_selection_notify, win));

   /* handlers = eina_list_append
    *   (handlers, ecore_event_handler_add
    *    (ECORE_EVENT_MOUSE_BUTTON_DOWN, _evry_cb_mouse_down, NULL));
    * handlers = eina_list_append
    *   (handlers, ecore_event_handler_add
    *    (ECORE_EVENT_MOUSE_BUTTON_UP, _evry_cb_mouse_up, NULL));
    * handlers = eina_list_append
    *   (handlers, ecore_event_handler_add
    *    (ECORE_EVENT_MOUSE_MOVE, _evry_cb_mouse_move, NULL));
    * handlers = eina_list_append
    *   (handlers, ecore_event_handler_add
    *    (ECORE_EVENT_MOUSE_WHEEL, _evry_cb_mouse_wheel, NULL)); */

   return 1;

 error:
   if (win && selectors[0])
     _evry_selector_free(selectors[0]);
   if (win && selectors[1])
     _evry_selector_free(selectors[1]);
   if (win && selectors[2])
     _evry_selector_free(selectors[2]);

   if (win)
     _evry_window_free(win);
   if (list)
     _evry_list_win_free(list);
   win = NULL;
   list = NULL;
   ecore_x_window_free(input_window);
   input_window = 0;

   return 0;
}

void
evry_hide(void)
{
   Ecore_Event *ev;

   if (!win) return;

   _evry_list_clear_list(selector->state);

   if (update_timer)
     ecore_timer_del(update_timer);
   update_timer = NULL;

   list->visible = EINA_FALSE;
   _evry_selector_free(selectors[0]);
   _evry_selector_free(selectors[1]);
   _evry_selector_free(selectors[2]);
   selectors[0] = NULL;
   selectors[1] = NULL;
   selectors[2] = NULL;
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
}


void
evry_clear_input(void)
{
   Evry_State *s = selector->state;

   if (s->input[0] != 0)
     {
       	s->input[0] = 0;
     }
}


/* static int item_cnt = 0; */

Evry_Item *
evry_item_new(Evry_Plugin *p, const char *label, void (*cb_free) (Evry_Item *item))
{
   Evry_Item *it;

   it = E_NEW(Evry_Item, 1);
   if (!it) return NULL;

   it->plugin = p;
   if (label) it->label = eina_stringshare_add(label);

   it->ref = 1;

   /* item_cnt++; */

   return it;
}

void
evry_item_free(Evry_Item *it)
{
   if (!it) return;

   it->ref--;

   if (it->ref > 0) return;

   /* printf("%d, %d\t 0x%x 0x%x 0x%x free: %s\n",
    * 	  it->ref, item_cnt - 1,
    * 	  it->label, it->uri, it->mime,
    * 	  it->label); */
   /* item_cnt--; */

   if (it->cb_free) it->cb_free(it);

   if (it->label) eina_stringshare_del(it->label);
   if (it->uri) eina_stringshare_del(it->uri);
   if (it->mime) eina_stringshare_del(it->mime);

   if (it->o_bg) evas_object_del(it->o_bg);
   if (it->o_icon) evas_object_del(it->o_icon);

   E_FREE(it);
}

void
_evry_item_ref(Evry_Item *it)
{
   it->ref++;
}

void
evry_plugin_async_update(Evry_Plugin *p, int action)
{
   Evry_State *s;
   Evry_Plugin *agg;

   if (!win) return;

   s = selector->state;
   agg = selector->aggregator;

   /* received data from a plugin of the current selector ? */
   if (!s || !eina_list_data_find(s->plugins, p)) return;

   if (action == EVRY_ASYNC_UPDATE_ADD)
     {
	if (!p->items)
	  {
	     /* remove plugin */
	     if (!eina_list_data_find(s->cur_plugins, p)) return;

	     s->cur_plugins = eina_list_remove(s->cur_plugins, p);

	     if (s->plugin == p)
	       _evry_select_plugin(s, NULL);
	  }
	else
	  {
	     /* add plugin to current plugins*/
	     _evry_plugin_list_insert(s, p);

	     if (!s->plugin || !eina_list_data_find_list(s->cur_plugins, s->plugin))
	       _evry_select_plugin(s, NULL);
	  }

	/* update aggregator */
	if (eina_list_count(s->cur_plugins) > 1)
	  {
	     agg->fetch(agg, s->input);

	     /* add aggregator */
	     if (!(s->cur_plugins->data == agg))
	       {
		  s->cur_plugins = eina_list_prepend(s->cur_plugins, agg);

		  if (s->plugin_auto_selected)
		    _evry_select_plugin(s, agg);
	       }
	  }

	/* plugin is visible */
	if ((s->plugin == p) || (s->plugin == agg))
	  {
	     _evry_selector_update(selector);
	     p = s->plugin;

	     _evry_list_clear_list(s);
	     _evry_list_show_items(s, p);
	     _evry_list_scroll_to(s, s->sel_item);
	  }

	/* plugin box was updated: realign */
	if (s->plugin)
	  {
	     _evry_list_tabs_update(s);
	     _evry_list_tab_scroll_to(s, s->plugin);
	  }
     }
}

int
evry_fuzzy_match(const char *str, const char *match)
{
   const char *p, *m;
   char mc;
   unsigned int cnt = 1;
   unsigned int pos = 0;
   unsigned int last = 0;
   unsigned char first;
   unsigned int spaces = 0;
   
   if (!match || !str) return 0;

   for (m = match; *m != 0; m++)
     {
	mc = tolower(*m);
	first = 0;

	for (p = str; *p != 0 && *m != 0; p++)
	  {
	     if (tolower(*p) == mc)
	       {
		  cnt += pos + (pos - last) * 10;
		  last = pos;
		  m++;
		  mc = tolower(*m);
		  first = 1;
	       }
	     else pos++;

	     if (!first)
	       last = pos;

	     if (cnt > MAX_FUZZ) return 0;

	     if (isspace(mc) && !strchr(p, ' '))
	       break;
	  }

	/* search next word */
	if (isspace(mc))
	  {
	     pos = last = 0;
	     /* add some weight */
	     cnt += pos;

	  }
	else break;
     }

   if (*m == 0)
     return cnt;
   else
     return 0;
}

/* local subsystem functions */

static Evry_List_Window *
_evry_list_win_new(E_Zone *zone)
{
   int x, y;
   Evry_List_Window *list_win;
   E_Popup *popup;
   Evas_Object *o;

   x = (zone->w / 2) - (win->popup->w / 3);
   y = (zone->h / 2);

   /* TODO get offsets from theme */
   popup = e_popup_new(zone, x + 50, y - 4,
		       win->popup->w * 2/3 - 100,
		       evry_conf->height);
   if (!popup) return NULL;

   list_win = E_NEW(Evry_List_Window, 1);
   if (!list_win)
     {
	e_object_del(E_OBJECT(popup));
	return NULL;
     }
   list_win->popup = popup;

   evas_event_freeze(popup->evas);
   evas_event_feed_mouse_in(popup->evas, ecore_x_current_time_get(), NULL);
   evas_event_feed_mouse_move(popup->evas, -1000000, -1000000, ecore_x_current_time_get(), NULL);
   o = edje_object_add(popup->evas);
   list_win->o_main = o;
   e_theme_edje_object_set(o, "base/theme/everything",
			   "e/widgets/everything/list");

   o = e_box_add(popup->evas);
   list_win->o_list = o;
   e_box_orientation_set(o, 0);
   e_box_homogenous_set(o, 1);
   edje_object_part_swallow(list_win->o_main, "e.swallow.list", o);
   evas_object_show(o);

   o = list_win->o_main;
   evas_object_move(o, 0, 0);
   evas_object_resize(o, list_win->popup->w, list_win->popup->h);
   evas_object_show(o);
   e_popup_edje_bg_object_set(popup, o);

   o = e_box_add(popup->evas);
   e_box_orientation_set(o, 1);
   /* e_box_homogenous_set(o, 1); */
   edje_object_part_swallow(list_win->o_main, "e.swallow.bar", o);
   evas_object_show(o);
   list_win->o_tabs = o;

   evas_event_thaw(popup->evas);

   return list_win;
}

static void
_evry_list_win_free(Evry_List_Window *list_win)
{
   if (list_win->scroll_timer)
     ecore_timer_del(list_win->scroll_timer);
   if (list_win->scroll_animator)
     ecore_animator_del(list_win->scroll_animator);
   if (list_win->item_idler)
     ecore_idler_del(list_win->item_idler);

   e_popup_hide(list_win->popup);
   evas_event_freeze(list_win->popup->evas);
   evas_object_del(list_win->o_list);
   evas_object_del(list_win->o_tabs);
   evas_object_del(list_win->o_main);
   /* evas_event_thaw(list_win->popup->evas); */
   e_object_del(E_OBJECT(list_win->popup));

   E_FREE(list_win);
}

static void
_evry_list_win_show(void)
{
   if (list->visible) return;

   list->visible = EINA_TRUE;

   _evry_list_update(selector->state);

   edje_object_signal_emit(list->o_main, "e,state,list_show", "e");
}

static void
_evry_list_win_hide(void)
{
   Eina_List *l;
   Evry_Plugin *p;

   if (!list->visible) return;

   list->visible = EINA_FALSE;

   EINA_LIST_FOREACH(selector->plugins, l, p)
     {
	e_box_unpack(p->tab);
	evas_object_del(p->tab);
	p->tab = NULL;
     }
   _evry_list_clear_list(selector->state);

   edje_object_signal_emit(list->o_main, "e,state,list_hide", "e");
   /* e_popup_hide(list->popup); */
}


static Evry_Window *
_evry_window_new(E_Zone *zone)
{
   int x, y, mw, mh;
   Evry_Window *win;
   E_Popup *popup;
   Evas_Object *o;

   popup = e_popup_new(zone, 100, 100, 1, 1);
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
			   "e/widgets/everything/main");

   edje_object_size_min_get(o, &mw, &mh);

   x = (zone->w / 2) - (mw / 2);
   y = (zone->h / 2) - mh;

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
   /* evas_event_thaw(win->popup->evas);    */
   e_object_del(E_OBJECT(win->popup));
   E_FREE(win);
}
static Evry_Selector *
_evry_selector_new(int type)
{
   Evry_Plugin *p;
   Eina_List *l;
   Evry_Selector *sel = E_NEW(Evry_Selector, 1);
   Evas_Object *o = edje_object_add(win->popup->evas);
   sel->o_main = o;
   e_theme_edje_object_set(o, "base/theme/everything",
			   "e/widgets/everything/selector_item");
   evas_object_show(o);

   if (type == type_subject)
     edje_object_part_swallow(win->o_main, "e.swallow.subject_selector", o);
   else if (type == type_action)
     edje_object_part_swallow(win->o_main, "e.swallow.action_selector", o);
   else if (type == type_object)
     edje_object_part_swallow(win->o_main, "e.swallow.object_selector", o);

   p = _evry_plug_aggregator_new();
   p->private = sel;
   sel->plugins = eina_list_append(sel->plugins, p);
   sel->aggregator = p;

   EINA_LIST_FOREACH(evry_conf->plugins, l, p)
     {
	if (!p->config->enabled) continue;
	if (p->type != type) continue;
	sel->plugins = eina_list_append(sel->plugins, p);
     }
   if (type == type_action)
     sel->plugins = eina_list_append(sel->plugins, action_selector);

   return sel;
}

static void
_evry_selector_free(Evry_Selector *sel)
{
   Evry_State *s;
   Evry_Plugin *p;

   if (sel->o_icon)
     evas_object_del(sel->o_icon);
   evas_object_del(sel->o_main);

   if (list->visible && (sel == selector))
     _evry_list_clear_list(sel->state);

   while (sel->states)
     _evry_state_pop(sel);

   EINA_LIST_FREE(sel->plugins, p)
     {
	if (p->tab) evas_object_del(p->tab);
	p->tab = NULL;
     }

   _evry_plug_aggregator_free(sel->aggregator);

   if (sel->plugins) eina_list_free(sel->plugins);
   E_FREE(sel);
}

static void
_evry_selector_activate(Evry_Selector *sel)
{
   if (selector)
     {
	Evry_State *s = selector->state;
	Evry_Plugin *p;
	Eina_List *l;

	edje_object_signal_emit(selector->o_main, "e,state,unselected", "e");
	edje_object_part_text_set(selector->o_main, "e.text.plugin", "");

	EINA_LIST_FOREACH(selector->plugins, l, p)
	  {
	     e_box_unpack(p->tab);
	     evas_object_del(p->tab);
	     p->tab = NULL;
	  }

	_evry_list_win_hide();
     }

   selector = sel;

   edje_object_signal_emit(sel->o_main, "e,state,selected", "e");

   if (sel->state)
     {
	_evry_update_text_label(sel->state);

	if (sel->state->sel_item)
	  edje_object_part_text_set(sel->o_main, "e.text.plugin",
				    sel->state->sel_item->plugin->name);

	if (list->visible && sel->state->plugin)
	  {
	     _evry_list_tabs_update(sel->state);
	     _evry_list_show_items(sel->state, sel->state->plugin);
	  }
     }
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

   it = s->sel_item;

   if (it && it->plugin && it->plugin->icon_get)
     {
	o = it->plugin->icon_get(it->plugin, it, win->popup->evas);
	if (o)
	  {
	     edje_object_part_swallow(sel->o_main, "e.swallow.icons", o);
	     evas_object_show(o);
	     sel->o_icon = o;
	  }
     }
   else if (s->plugin && s->plugin->icon)
     {
	o = e_icon_add(win->popup->evas);
	evry_icon_theme_set(o, s->plugin->icon);
	edje_object_part_swallow(sel->o_main, "e.swallow.icons", o);
	evas_object_show(o);
	sel->o_icon = o;
     }
}

static void
_evry_selector_update(Evry_Selector *sel)
{
   Evry_State *s = sel->state;
   Evry_Item *it = s->sel_item;

   if (!s->plugin && it)
     _evry_list_item_desel(s, NULL);
   else if (it && !eina_list_data_find_list(s->plugin->items, it))
     _evry_list_item_desel(s, NULL);

   it = s->sel_item;

   if (s->plugin && (!it || s->item_auto_selected))
     {
	/* get first item */
	if (s->plugin->items)
	  {
	     it = s->plugin->items->data;
	     s->item_auto_selected = EINA_TRUE;
	     _evry_list_item_sel(s, it);
	  }
     }

   _evry_selector_icon_set(sel);

   if (it)
     {
	edje_object_part_text_set(sel->o_main, "e.text.label", it->label);

	if (sel == selector)
	  edje_object_part_text_set(sel->o_main, "e.text.plugin", it->plugin->name);
	else
	  edje_object_part_text_set(sel->o_main, "e.text.plugin", "");
     }
   else
     {
	/* no items for this state - clear selector */
	edje_object_part_text_set(sel->o_main, "e.text.label", "");
	if (sel == selector && s && s->plugin)
	  edje_object_part_text_set(sel->o_main, "e.text.plugin", s->plugin->name);
	else
	  edje_object_part_text_set(sel->o_main, "e.text.plugin", "");
     }

   if (sel == selectors[0])
     {
	if (_evry_selector_actions_get(it))
	  _evry_selector_update(selectors[1]);
     }
}

static void
_evry_list_update(Evry_State *s)
{
   if (!list->visible) return;

   _evry_list_clear_list(s);
   _evry_list_tabs_update(s);

   if (!s->plugin) return;

   _evry_list_show_items(s, s->plugin);
   _evry_list_scroll_to(s, s->sel_item);
}

static int
_evry_selector_subjects_get(void)
{
   Eina_List *l, *plugins = NULL;
   Evry_Plugin *p;
   Evry_Selector *sel = selectors[0];

   EINA_LIST_FOREACH(sel->plugins, l, p)
     {
	if (p->begin)
	  {
	     if (p->begin(p, NULL))
	       plugins = eina_list_append(plugins, p);
	  }
	else
	  plugins = eina_list_append(plugins, p);
     }

   if (!plugins) return 0;

   _evry_state_new(sel, plugins);

   _evry_matches_update(sel);

   return 1;
}

static int
_evry_selector_actions_get(Evry_Item *it)
{
   Eina_List *l, *plugins = NULL;
   Evry_Plugin *p;
   Evry_Selector *sel = selectors[1];

   while (sel->state)
     _evry_state_pop(sel);

   if (!it) return 0;

   EINA_LIST_FOREACH(sel->plugins, l, p)
     {
	if (strcmp(p->type_in, it->plugin->type_out) &&
	    (p != action_selector) &&
	    (p != sel->aggregator)) continue;

	if (p->begin)
	  {
	     if (p->begin(p, it))
	       plugins = eina_list_append(plugins, p);
	  }
	else
	  plugins = eina_list_append(plugins, p);
     }

   if (!plugins) return 0;

   _evry_state_new(sel, plugins);

   _evry_matches_update(sel);

   return 1;
}

static int
_evry_selector_objects_get(const char *type)
{
   Eina_List *l, *plugins = NULL;
   Evry_Plugin *p;
   Evry_Selector *sel = selectors[2];
   Evry_Item *it;

   while (sel->state)
     _evry_state_pop(sel);

   it = selectors[0]->state->sel_item;

   EINA_LIST_FOREACH(sel->plugins, l, p)
     {
	if (strcmp(p->type_out, type) &&
	    (p != sel->aggregator)) continue;

	if (p->begin)
	  {
	     if (p->begin(p, it))
	       plugins = eina_list_append(plugins, p);
	  }
	else
	  plugins = eina_list_append(plugins, p);
     }

   if (!plugins) return 0;

   _evry_state_new(sel, plugins);

   _evry_matches_update(sel);

   return 1;
}


static Evry_State *
_evry_state_new(Evry_Selector *sel, Eina_List *plugins)
{
   Evry_State *s = E_NEW(Evry_State, 1);
   s->input = malloc(INPUTLEN);
   s->input[0] = 0;
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

   _evry_list_item_desel(s, NULL);

   free(s->input);

   EINA_LIST_FREE(s->plugins, p)
     p->cleanup(p);

   E_FREE(s);

   sel->states = eina_list_remove_list(sel->states, sel->states);

   if (sel->states)
     sel->state = sel->states->data;
   else
     sel->state = NULL;
}

static void
_evry_browse_item(Evry_Selector *sel)
{
   Evry_State *s = sel->state;
   Evry_Item *it;
   Eina_List *l, *plugins = NULL;
   Evry_Plugin *p;

   it = s->sel_item;

   if (!it || !it->browseable) return;

   _evry_list_clear_list(sel->state);

   EINA_LIST_FOREACH(sel->plugins, l, p)
     {
	if (!p->browse) continue;
	if (!strstr(p->type_in, it->plugin->type_out)) continue;

	if (p->browse(p, it))
	  plugins = eina_list_append(plugins, p);
     }

   if (plugins)
     {
	_evry_list_win_show();
	_evry_state_new(sel, plugins);
	_evry_matches_update(sel);
	_evry_selector_update(sel);
     }

   _evry_list_update(sel->state);
   _evry_update_text_label(sel->state);
}


static void
_evry_browse_back(Evry_Selector *sel)
{
   Evry_State *s = sel->state;

   if (!s || !sel->states->next) return;

   _evry_list_clear_list(s);
   _evry_state_pop(sel);

   sel->aggregator->fetch(sel->aggregator, sel->state->input);
   _evry_selector_update(sel);
   _evry_list_update(sel->state);
   _evry_update_text_label(sel->state);
}

static void
_evry_selectors_switch(void)
{
   Evry_State *s = selector->state;

   if (update_timer)
     {
	if ((s && !s->plugin->async_query) &&
	    ((selector == selectors[0]) ||
	     (selector == selectors[1])))
	  {
	     _evry_list_clear_list(s);
	     _evry_matches_update(selector);
	     _evry_selector_update(selector);
	  }

	ecore_timer_del(update_timer);
	update_timer = NULL;
     }

   if (selector == selectors[0])
     {
	if (s->sel_item)
	  _evry_selector_activate(selectors[1]);
     }
   else if (selector == selectors[1])
     {
	int next_selector = 0;

	if (s->sel_item && s->sel_item->plugin == action_selector)
	  {
	     Evry_Action *act = s->sel_item->data[0];

	     if (act && act->type_in2)
	       {
		  _evry_selector_objects_get(act->type_in2);
		  _evry_selector_update(selectors[2]);
		  edje_object_signal_emit(win->o_main,
					  "e,state,object_selector_show", "e");
		  next_selector = 2;
	       }
	  }
	_evry_selector_activate(selectors[next_selector]);
     }
   else if (selector == selectors[2])
     {
	_evry_list_clear_list(s);

	while (selector->states)
	  _evry_state_pop(selector);

	edje_object_signal_emit(win->o_main,
				"e,state,object_selector_hide", "e");

	_evry_selector_activate(selectors[0]);
     }
}

 static int
_evry_cb_key_down(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_Event_Key *ev;
   Evry_State *s = selector->state;
   /* ev_last_is_mouse = 0;
    * item_mouseover = NULL; */

   win->request_selection = EINA_FALSE;

   ev = event;
   if (ev->event_window != input_window) return 1;

   if       (!strcmp(ev->key, "Up"))
     _evry_list_item_prev(s);
   else if (!strcmp(ev->key, "Down"))
     _evry_list_item_next(s);
   else if (!strcmp(ev->key, "Right") &&
	    ((ev->modifiers & ECORE_EVENT_MODIFIER_CTRL) ||
	     (ev->modifiers & ECORE_EVENT_MODIFIER_SHIFT)))
     _evry_list_plugin_next(s);
   else if (!strcmp(ev->key, "Left") &&
	    ((ev->modifiers & ECORE_EVENT_MODIFIER_CTRL) ||
	     (ev->modifiers & ECORE_EVENT_MODIFIER_SHIFT)))
     _evry_list_plugin_prev(s);
   else if (!strcmp(ev->key, "Right"))
     _evry_browse_item(selector);
   else if (!strcmp(ev->key, "Left"))
     _evry_browse_back(selector);
   else if ((!strcmp(ev->key, "Return")) &&
	   ((ev->modifiers & ECORE_EVENT_MODIFIER_CTRL) ||
	    (ev->modifiers & ECORE_EVENT_MODIFIER_SHIFT)))
     _evry_plugin_action(selector, 0);
   else if (!strcmp(ev->key, "Return"))
     _evry_plugin_action(selector, 1);
   else if (!strcmp(ev->key, "Tab") &&
	    ((ev->modifiers & ECORE_EVENT_MODIFIER_ALT) ||
	     (ev->modifiers & ECORE_EVENT_MODIFIER_CTRL)))
     _evry_list_plugin_next(s);
   else if (!strcmp(ev->key, "Tab"))
     _evry_selectors_switch();
   else if (!strcmp(ev->key, "u") &&
	    (ev->modifiers & ECORE_EVENT_MODIFIER_CTRL))
     _evry_clear(s);
   else  if ((!strcmp(ev->key, "Escape")) ||
	     (!strcmp(ev->key, "e") &&
	      (ev->modifiers & ECORE_EVENT_MODIFIER_CTRL)))
     evry_hide();
   else if ((!strcmp(ev->key, "BackSpace")) ||
	    (!strcmp(ev->key, "Delete")))
     _evry_backspace(s);
   else if (!strcmp(ev->key, "v") &&
	    (ev->modifiers & ECORE_EVENT_MODIFIER_CTRL))
     {
	win->request_selection = EINA_TRUE;
	ecore_x_selection_primary_request(win->popup->evas_win,
					  ECORE_X_SELECTION_TARGET_UTF8_STRING);
     }
   else if ((ev->key) &&
	    (ev->modifiers & ECORE_EVENT_MODIFIER_CTRL))
     _evry_list_plugin_next_by_name(s, ev->key);
   else if ((ev->compose) &&
	    (!(ev->modifiers & ECORE_EVENT_MODIFIER_ALT) ||
	      (ev->modifiers & ECORE_EVENT_MODIFIER_WIN)))
     {
	if (strlen(s->input) < (INPUTLEN - strlen(ev->compose)))
	  {
	     strcat(s->input, ev->compose);
	     _evry_update(s);
	  }
     }
   return 1;
}

static void
_evry_backspace(Evry_State *s)
{
   int len, val, pos;

   len = strlen(s->input);
   if (len > 0)
     {
	pos = evas_string_char_prev_get(s->input, len, &val);
	if ((pos < len) && (pos >= 0))
	  {
	     s->input[pos] = 0;
	     _evry_update(s);
	  }
     }
}

static void
_evry_update_text_label(Evry_State *s)
{
   if (strlen(s->input) > 0)
     edje_object_signal_emit(list->o_main, "e,state,entry_show", "e");
   else
     edje_object_signal_emit(list->o_main, "e,state,entry_hide", "e");

   edje_object_part_text_set(win->o_main, "e.text.label", s->input);
   edje_object_part_text_set(list->o_main, "e.text.label", s->input);

}

static void
_evry_update(Evry_State *s)
{
   _evry_update_text_label(s);

   if (update_timer) ecore_timer_del(update_timer);
   update_timer = ecore_timer_add(MATCH_LAG, _evry_update_timer, s);
}

static int
_evry_update_timer(void *data)
{
   Evry_State *s = data;
   /* XXX pass selector as data? */
   _evry_matches_update(selector);
   _evry_selector_update(selector);
   _evry_list_update(selector->state);
   _evry_list_tabs_update(selector->state);
   update_timer = NULL;

   return 0;
}

static void
_evry_clear(Evry_State *s)
{
   if ((s->plugin && s->plugin->trigger && s->input) &&
       (!strncmp(s->plugin->trigger, s->input, strlen(s->plugin->trigger))))
     {
	s->input[strlen(s->plugin->trigger)] = 0;
	_evry_update(s);
     }
   else if (s->input[0] != 0)
     {
	s->input[0] = 0;
	_evry_update(s);
	edje_object_signal_emit(list->o_main, "e,state,entry_hide", "e");
     }
}

static void
_evry_plugin_action(Evry_Selector *sel, int finished)
{
   Evry_State *s_subject, *s_action, *s_object;

   s_subject = selectors[0]->state;
   s_action  = selectors[1]->state;
   s_object = NULL;

   if (!s_subject || !s_action) return;

   if (update_timer)
     {
	if ((selector->state->plugin) &&
	    (!selector->state->plugin->async_query))
	  {
	     _evry_matches_update(selector);
	     _evry_selector_update(selector);
	  }

	ecore_timer_del(update_timer);
	update_timer = NULL;
     }

   if (!s_subject->sel_item || !s_action->sel_item) return;

   if (s_action->sel_item->plugin == action_selector)
     {
	Evry_Action *act = s_action->sel_item->data[0];
	Evry_Item *it_object = NULL;

	if (selectors[2] == selector) /* && selector->state ? */
	  it_object = selector->state->sel_item;

	if (act->type_in2 && !it_object) return;

	act->action(act, s_subject->sel_item, it_object, NULL);
     }
   else
     {
	Evry_Item *it = s_action->sel_item;
	s_action->plugin->action(s_action->plugin, it, selector->state->input);
     }

   if (s_subject->plugin->action)
     s_subject->plugin->action(s_subject->plugin,
			       s_subject->sel_item,
			       s_subject->input);

   if (s_object && s_object->plugin->action)
     s_object->plugin->action(s_object->plugin,
			      s_object->sel_item,
			      s_object->input);
   if (finished)
     evry_hide();
}

static void
_evry_list_show_items(Evry_State *s, Evry_Plugin *p)
{
   Evry_Item *it;
   Eina_List *l;
   int mw, mh, h;
   Evas_Object *o;

   if (!p) return;

   if (p->realize_items) p->realize_items(p, list->popup->evas);

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

   if (!list->visible) return;

   list->scroll_align = 0;

   evas_event_freeze(list->popup->evas);
   e_box_freeze(list->o_list);

   EINA_LIST_FOREACH(p->items, l, it)
     {
	o = it->o_bg;

	if (!o)
	  {
	     o = edje_object_add(list->popup->evas);
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

	_evry_item_ref(it);
	s->items = eina_list_append(s->items, it);
     }
   e_box_thaw(list->o_list);

   list->item_idler = ecore_idler_add(_evry_list_item_idler, p);

   e_box_min_size_get(list->o_list, NULL, &mh);
   evas_object_geometry_get(list->o_list, NULL, NULL, NULL, &h);
   if (mh <= h)
     e_box_align_set(list->o_list, 0.5, 0.0);
   else
     e_box_align_set(list->o_list, 0.5, 1.0);

   evas_event_thaw(list->popup->evas);

   _evry_list_tab_scroll_to(s, p);
}

static void
_evry_list_clear_list(Evry_State *s)
{
   Evry_Item *it;

   if (list->item_idler)
     {
	ecore_idler_del(list->item_idler);
	list->item_idler = NULL;
     }

   if (s && s->items)
     {
	evas_event_freeze(list->popup->evas);
	e_box_freeze(list->o_list);
	EINA_LIST_FREE(s->items, it)
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
   evas_event_thaw(list->popup->evas);
   /* } */
}

static int
_evry_list_item_idler(void *data)
{
   Evry_Plugin *p = data;
   int cnt = 5;
   Eina_List *l;
   Evry_Item *it;

   if (!list->item_idler) return 0;

   if (!p->icon_get) goto end;
   e_box_freeze(list->o_list);

   EINA_LIST_FOREACH(p->items, l, it)
     {
	if (it->o_icon) continue;

	it->o_icon = p->icon_get(p, it, list->popup->evas);

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


static void
_evry_matches_update(Evry_Selector *sel)
{
   Evry_State *s = sel->state;
   Evry_Plugin *p;
   Eina_List *l;
   Eina_Bool has_items;

   EINA_LIST_FREE(s->cur_plugins, p);

   if (s->input)
     {
	EINA_LIST_FOREACH(s->plugins, l, p)
	  {
	     /* input matches plugin trigger? */
	     if (!p->trigger) continue;

	     if ((strlen(s->input) >= strlen(p->trigger)) &&
		 (!strncmp(s->input, p->trigger, strlen(p->trigger))))
	       {
		  s->cur_plugins = eina_list_append(s->cur_plugins, p);
		  p->fetch(p, s->input);
		  break;
	       }
	  }
     }

   if (!s->cur_plugins)
     {
	EINA_LIST_FOREACH(s->plugins, l, p)
	  {
	     if (p->trigger) continue;
	     if (p == sel->aggregator) continue;

	     if (strlen(s->input) == 0)
	       {
		  if (!p->need_query)
		    has_items = p->fetch(p, NULL);
	       }
	     else
	       {
		  has_items = p->fetch(p, s->input);
	       }

	     if (has_items || sel->states->next)
	       s->cur_plugins = eina_list_append(s->cur_plugins, p);
	  }

	if (eina_list_count(s->cur_plugins) > 1)
	  {
	     sel->aggregator->fetch(sel->aggregator, s->input);
	     s->cur_plugins = eina_list_prepend(s->cur_plugins, sel->aggregator);
	  }
	else
	  sel->aggregator->cleanup(sel->aggregator);
     }

   if (s->plugin && !eina_list_data_find_list(s->cur_plugins, s->plugin))
     s->plugin = NULL;

   _evry_select_plugin(s, s->plugin);
}


static void
_evry_list_scroll_to(Evry_State *s, Evry_Item *it)
{
   int n, h, mh, i = 0, max_w, max_h;
   Eina_List *l;

   if (!it) return;

   for(l = s->plugin->items; l; l = l->next, i++)
     if (l->data == it) break;
   n = eina_list_count(s->plugin->items);

   e_box_min_size_get(list->o_list, NULL, &mh);
   evas_object_geometry_get(list->o_list, NULL, NULL, NULL, &h);

   /* edje_object_part_geometry_get(list->o_main, "e.swallow.list", NULL, NULL, &max_w, &max_h); */
   /* printf("MAX %d %d %d\n", max_h, h, mh);
    * if (max_h != h)
    *   evas_object_resize(list->o_list, max_w, max_h); */

   if (i >= n || mh <= h)
     {
	e_box_align_set(list->o_list, 0.5, 0.0);
	return;
     }

   if (n > 1)
     {
	list->scroll_align_to = (double)i / (double)(n - 1);
	if (evry_conf->scroll_animate)
 	  {
	     if (!list->scroll_timer)
	       list->scroll_timer = ecore_timer_add(0.01, _evry_list_scroll_timer, NULL);
	     if (!list->scroll_animator)
	       list->scroll_animator = ecore_animator_add(_evry_list_animator, NULL);
	  }
	else
	  {
	     list->scroll_align = list->scroll_align_to;
	     e_box_align_set(list->o_list, 0.5, 1.0 - list->scroll_align);
	  }
     }
   else
     e_box_align_set(list->o_list, 0.5, 1.0);
}

static void
_evry_list_tab_scroll_to(Evry_State *s, Evry_Plugin *p)
{
   int n, w, mw, i;
   double align;

   Eina_List *l;

   for(i = 0, l = s->cur_plugins; l; l = l->next, i++)
     if (l->data == p) break;

   n = eina_list_count(s->cur_plugins);

   e_box_min_size_get(list->o_tabs, &mw, NULL);
   evas_object_geometry_get(list->o_tabs, NULL, NULL, &w, NULL);

   if (mw <= w)
     {
	e_box_align_set(list->o_tabs, 0.0, 0.5);
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
	{
	   e_box_align_set(list->o_tabs, 1.0 - align, 0.5);
	}
     }
   else
     e_box_align_set(list->o_tabs, 1.0, 0.5);
}

static void
_evry_list_item_desel(Evry_State *s, Evry_Item *it)
{
   if (s->sel_item)
     {
	it = s->sel_item;

	if (list->visible)
	  {
	     if (it->o_bg)
	       edje_object_signal_emit(it->o_bg, "e,state,unselected", "e");
	     if (it->o_icon)
	       edje_object_signal_emit(it->o_icon, "e,state,unselected", "e");
	  }

	evry_item_free(it);
	s->sel_item = NULL;
     }
}

static void
_evry_list_item_sel(Evry_State *s, Evry_Item *it)
{
   if (s->sel_item == it) return;

   _evry_list_item_desel(s, NULL);

   if (list->visible)
     {
	if (it->o_bg)
	  edje_object_signal_emit(it->o_bg, "e,state,selected", "e");
	if (it->o_icon)
	  edje_object_signal_emit(it->o_icon, "e,state,selected", "e");
	if (it->browseable)
	  edje_object_signal_emit(it->o_bg, "e,state,arrow_show", "e");
	if (s == selector->state)
	  _evry_list_scroll_to(s, it);
     }

   _evry_item_ref(it);
   s->sel_item = it;
}


static void
_evry_list_item_next(Evry_State *s)
{
   Eina_List *l;
   Evry_Item *it;

   if (!s->plugin || !s->plugin->items) return;

   if (!list->visible)
     {
	_evry_list_win_show();
	return;
     }

   s->plugin_auto_selected = EINA_FALSE;
   s->item_auto_selected = EINA_FALSE;

   if (!s->sel_item)
     {
	_evry_list_item_sel(s, s->plugin->items->data);
	_evry_selector_update(selector);
	return;
     }

   EINA_LIST_FOREACH (s->plugin->items, l, it)
     {
	if (it == s->sel_item)
	  {
	     if (l->next)
	       {
		  _evry_list_item_sel(s, l->next->data);
		  _evry_selector_update(selector);
	       }
	     break;
	  }
     }
}

static void
_evry_list_item_prev(Evry_State *s)
{
   Eina_List *l;
   Evry_Item *it;

   if (!s->plugin || !s->plugin->items) return;

   s->plugin_auto_selected = EINA_FALSE;
   s->item_auto_selected = EINA_FALSE;

   if (!s->sel_item) return;

   EINA_LIST_FOREACH (s->plugin->items, l, it)
     {
	if (it == s->sel_item)
	  {
	     if (l->prev)
	       {
		  _evry_list_item_sel(s, l->prev->data);
		  _evry_selector_update(selector);
		  return;
	       }
	     break;
	  }
     }
   _evry_list_win_hide();
}

static void
_evry_select_plugin(Evry_State *s, Evry_Plugin *p)
{
   if (!s || !s->cur_plugins) return;

   if (!p && s->cur_plugins)
     {
	p = s->cur_plugins->data;
	s->plugin_auto_selected = EINA_TRUE;
     }

   if (p && list->visible)
     {
	if (s->plugin && s->plugin != p)
	  edje_object_signal_emit(s->plugin->tab, "e,state,unselected", "e");

	edje_object_signal_emit(p->tab, "e,state,selected", "e");
     }

   if ((p || !s->plugin) &&  s->plugin != p)
     {
	_evry_list_item_desel(s, NULL);
	s->plugin = p;
     }
}

static void
_evry_list_plugin_next(Evry_State *s)
{
   Eina_List *l;
   Evry_Plugin *p = NULL;

   if (!s->plugin) return;

   /* _evry_list_win_show(); */

   l = eina_list_data_find_list(s->cur_plugins, s->plugin);

   if (l && l->next)
     p = l->next->data;
   else if (s->plugin != s->cur_plugins->data)
     p = s->cur_plugins->data;

   if (p)
     {
	s->plugin_auto_selected = EINA_FALSE;
	_evry_list_clear_list(s);
	_evry_select_plugin(s, p);
	_evry_list_show_items(s, p);
	_evry_selector_update(selector);
     }
}

static void
_evry_list_plugin_next_by_name(Evry_State *s, const char *key)
{
   Eina_List *l;
   Evry_Plugin *p, *first = NULL, *next = NULL;
   int found = 0;

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

   if (p)
     {
	s->plugin_auto_selected = EINA_FALSE;
	_evry_list_clear_list(s);
	_evry_select_plugin(s, p);
	_evry_list_show_items(s, p);
	_evry_selector_update(selector);
     }
}

static void
_evry_list_plugin_prev(Evry_State *s)
{
   Eina_List *l;
   Evry_Plugin *p = NULL;

   if (!s->plugin) return;

   /* _evry_list_win_show(); */

   l = eina_list_data_find_list(s->cur_plugins, s->plugin);

   if (l && l->prev)
     p = l->prev->data;
   else
     {
	l = eina_list_last(s->cur_plugins);
	if (s->plugin != l->data)
	  p = l->data;
     }

   if (p)
     {
	s->plugin_auto_selected = EINA_FALSE;
	_evry_list_clear_list(s);
	_evry_select_plugin(s, p);
	_evry_list_show_items(s, p);
	_evry_selector_update(selector);
     }
}

static int
_evry_list_scroll_timer(void *data __UNUSED__)
{
   if (list->scroll_animator)
     {
	double spd;
	spd = evry_conf->scroll_speed;
	list->scroll_align = (list->scroll_align * (1.0 - spd)) +
	  (list->scroll_align_to * spd);
	return 1;
     }
   list->scroll_timer = NULL;
   return 0;
}

static int
_evry_list_animator(void *data __UNUSED__)
{
   double da;
   Eina_Bool scroll_to = 1;

   da = list->scroll_align - list->scroll_align_to;
   if (da < 0.0) da = -da;
   if (da < 0.01)
     {
	list->scroll_align = list->scroll_align_to;
	scroll_to = 0;
     }
   e_box_align_set(list->o_list, 0.5, 1.0 - list->scroll_align);
   if (scroll_to) return 1;
   list->scroll_animator = NULL;
   return 0;
}

static void
_evry_list_tabs_update(Evry_State *s)
{

   Eina_List *l;
   Evry_Plugin *p;

   /* remove tabs for not active plugins */
   EINA_LIST_FOREACH(selector->plugins, l, p)
     {
	if (p->tab && !eina_list_data_find(s->cur_plugins, p))
	  {
	     e_box_unpack(p->tab);
	     evas_object_del(p->tab);
	     p->tab = NULL;
	  }
     }

   /* show/update tabs of active plugins */
   EINA_LIST_FOREACH(s->cur_plugins, l, p)
     {
	_evry_list_tab_show(s, p);
	if (s->plugin == p)
	  edje_object_signal_emit(p->tab, "e,state,selected", "e");
	else
	  edje_object_signal_emit(p->tab, "e,state,unselected", "e");
     }
}

static void
_evry_list_tab_show(Evry_State *s, Evry_Plugin *p)
{
   Evas_Object *o;
   Evas_Coord mw = 0, mh = 0;
   char buf[64];
   Eina_List *l;

   e_box_freeze(list->o_tabs);

   if (p->tab)
     {
	o = p->tab;
	e_box_unpack(p->tab);
     }
   else
     {
	o = edje_object_add(list->popup->evas);

	e_theme_edje_object_set(o, "base/theme/everything",
				"e/widgets/everything/tab_item");
     }

   snprintf(buf, 64, "%s (%d)", p->name, eina_list_count(p->items));

   edje_object_part_text_set(o, "e.text.label", buf);

   edje_object_size_min_calc(o, &mw, &mh);

   l = eina_list_data_find_list(s->cur_plugins, p);
   if (l && l->prev)
     {
	Evry_Plugin *p2 = l->prev->data;
	e_box_pack_after(list->o_tabs, o, p2->tab);
     }
   else
     e_box_pack_end(list->o_tabs, o);

   evas_object_show(o);
   e_box_pack_options_set(o, 1, 1, 0, 0, 0.5, 0.5, mw, mh, 9999, 9999);
   e_box_thaw(list->o_tabs);

   p->tab = o;
}

static int
_evry_fuzzy_sort_cb(const void *data1, const void *data2)
{
   const Evry_Item *it1 = data1;
   const Evry_Item *it2 = data2;

   if (it1->fuzzy_match && !it2->fuzzy_match)
     return -1;

   if (!it1->fuzzy_match && it2->fuzzy_match)
     return 1;

   if (it1->fuzzy_match - it2->fuzzy_match)
     return (it1->fuzzy_match - it2->fuzzy_match);

   if (it1->plugin->config->priority - it2->plugin->config->priority)
     return (it1->plugin->config->priority - it2->plugin->config->priority);

   return (it1->priority - it2->priority);
}

/* action selector plugin: provides list of actions registered for
   candidate types provided by current plugin */
static int
_evry_plug_actions_init(void)
{
   Plugin_Config *pc;
   Evry_Plugin *p = E_NEW(Evry_Plugin, 1);
   p->name = "Select Action";
   p->type = type_action;
   p->type_in  = "ANY";
   p->type_out = "NONE";
   p->begin    = &_evry_plug_actions_begin;
   p->cleanup  = &_evry_plug_actions_cleanup;
   p->fetch    = &_evry_plug_actions_fetch;
   p->icon_get = &_evry_plug_actions_item_icon_get;

   pc = E_NEW(Plugin_Config, 1);
   pc->name = eina_stringshare_add(p->name);
   pc->enabled = 1;
   pc->priority = 1;
   p->config = pc;

   action_selector = p;

   return 1;
}

static void
_evry_plug_actions_free(void)
{
   Evry_Plugin *p = action_selector;

   eina_stringshare_del(p->config->name);
   E_FREE(p->config);
   E_FREE(p);
}

static int
_evry_plug_actions_begin(Evry_Plugin *p, const Evry_Item *it)
{
   Evry_Action *act;
   Eina_List *l;
   Evry_Selector *sel = selectors[1];

   _evry_plug_actions_cleanup(p);

   if (!it) return 0;

   const char *type = it->plugin->type_out;

   EINA_LIST_FOREACH(evry_conf->actions, l, act)
     {
	if ((strstr(act->type_in1, type)) &&
	    (!act->check_item || act->check_item(act, it)))
	  {
	     sel->actions = eina_list_append(sel->actions, act);
	  }
     }

   if (sel->actions) return 1;

   return 0;
}

static int
_evry_plug_actions_fetch(Evry_Plugin *p, const char *input)
{
   Evry_Action *act;
   Eina_List *l;
   Evry_Item *it;
   Evry_Selector *sel = selectors[1];
   int match = 0;

   EINA_LIST_FREE(p->items, it)
     evry_item_free(it);

   EINA_LIST_FOREACH(sel->actions, l, act)
     {
	if (input)
	  match = evry_fuzzy_match(act->name, input);

	if (!input || match)
	  {
	     it = evry_item_new(p, act->name, NULL);
	     it->fuzzy_match = match;
	     it->data[0] = act;
	     p->items = eina_list_append(p->items, it);
	  }
     }

   if (input)
     p->items = eina_list_sort(p->items, eina_list_count(p->items), _evry_fuzzy_sort_cb);

   if (p->items) return 1;

   return 0;
}

static void
_evry_plug_actions_cleanup(Evry_Plugin *p)
{
   Evry_Item *it;
   Evry_Selector *sel = selectors[1];

   EINA_LIST_FREE(p->items, it)
     evry_item_free(it);

   if (sel->actions) eina_list_free(sel->actions);
   sel->actions = NULL;
}

static Evas_Object *
_evry_plug_actions_item_icon_get(Evry_Plugin *p __UNUSED__, const Evry_Item *it, Evas *e)
{
   Evas_Object *o;
   Evry_Action *act = it->data[0];

   if (!act) return NULL;

   if (act->icon_get)
     o = act->icon_get(act, e);
   else if (act->icon)
     {
	o = e_icon_add(e);
	evry_icon_theme_set(o, act->icon);
     }

   return o;
}

static Evry_Plugin *
_evry_plug_aggregator_new(void)
{
   Plugin_Config *pc;
   Evry_Plugin *p = E_NEW(Evry_Plugin, 1);
   p->name = "All";
   p->type_in  = "NONE";
   p->type_out = "NONE";
   p->cleanup  = &_evry_plug_aggregator_cleanup;
   p->fetch    = &_evry_plug_aggregator_fetch;
   p->action   = &_evry_plug_aggregator_action;
   p->icon_get = &_evry_plug_aggregator_item_icon_get;

   pc = E_NEW(Plugin_Config, 1);
   pc->name = eina_stringshare_add(p->name);
   pc->enabled = 1;
   pc->priority = -1;
   p->config = pc;

   return p;
}

static void
_evry_plug_aggregator_free(Evry_Plugin *p)
{
   if (p->config->name) eina_stringshare_del(p->config->name);
   E_FREE(p->config);
   E_FREE(p);
}


static int
_evry_plug_aggregator_fetch(Evry_Plugin *p, const char *input)
{
   Evry_Selector *selector = p->private;
   Evry_State *s = selector->state;
   Eina_List *l, *ll;
   Evry_Plugin *plugin;
   Evry_Item *it;
   int cnt = 0;
   Eina_List *items = NULL;

   EINA_LIST_FREE(p->items, it)
     evry_item_free(it);

   EINA_LIST_FOREACH(s->cur_plugins, l, plugin)
     cnt += eina_list_count(plugin->items);


   if (input[0])
     {
	EINA_LIST_FOREACH(s->cur_plugins, l, plugin)
	  {
	     EINA_LIST_FOREACH(plugin->items, ll, it)
	       {
		  if (!it->fuzzy_match)
		    it->fuzzy_match = evry_fuzzy_match(it->label, input);

		  if (it->fuzzy_match)
		    {
		       _evry_item_ref(it);
		       items = eina_list_append(items, it);
		       p->items = eina_list_append(p->items, it);
		    }
	       }
	  }
     }

   if (!input[0] || eina_list_count(items) < 20)
     {
	EINA_LIST_FOREACH(s->cur_plugins, l, plugin)
   	  {
   	     for (cnt = 0, ll = plugin->items; ll && cnt < 15; ll = ll->next, cnt++)
   	       {
		  if (!items || !eina_list_data_find_list(items, ll->data))
		    {
		       it = ll->data;
		       _evry_item_ref(it);
		       it->fuzzy_match = 0;
		       p->items = eina_list_append(p->items, it);
		    }
   	       }
   	  }
     }

   eina_list_free(items);

   if (input[0])
     p->items = eina_list_sort(p->items, eina_list_count(p->items), _evry_fuzzy_sort_cb);

   return 1;
}

static int
_evry_plug_aggregator_action(Evry_Plugin *p, const Evry_Item *it, const char *input)
{
   if (it->plugin && it->plugin->action)
     return it->plugin->action(it->plugin, it, input);

   return 0;
}

static void
_evry_plug_aggregator_cleanup(Evry_Plugin *p)
{
   Evry_Item *it;

   EINA_LIST_FREE(p->items, it)
     evry_item_free(it);
}

static Evas_Object *
_evry_plug_aggregator_item_icon_get(Evry_Plugin *p, const Evry_Item *it, Evas *e)
{
   if (it->plugin && it->plugin->icon_get)
     return it->plugin->icon_get(it->plugin, it, e);

   return NULL;
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


/* taken from e_utils. just changed 48 to 72.. we need
   evry_icon_theme_set(Evas_Object *obj, const char *icon,
   size:small, mid, large) imo */
static int
_evry_icon_theme_set(Evas_Object *obj, const char *icon)
{
   const char *file;
   char buf[4096];

   if ((!icon) || (!icon[0])) return 0;
   snprintf(buf, sizeof(buf), "e/icons/%s", icon);
   file = e_theme_edje_file_get("base/theme/icons", buf);
   if (file[0])
     {
	e_icon_file_edje_set(obj, file, buf);
	return 1;
     }
   return 0;
}

static int
_evry_icon_fdo_set(Evas_Object *obj, const char *icon)
{
   char *path = NULL;
   unsigned int size;

   if ((!icon) || (!icon[0])) return 0;
   size = e_util_icon_size_normalize(72 * e_scale);
   path = efreet_icon_path_find(e_config->icon_theme, icon, size);
   if (!path) return 0;
   e_icon_file_set(obj, path);
   E_FREE(path);
   return 1;
}

EAPI int
evry_icon_theme_set(Evas_Object *obj, const char *icon)
{
   if (e_config->icon_theme_overrides)
     {
	if (_evry_icon_fdo_set(obj, icon))
	  return 1;
	return _evry_icon_theme_set(obj, icon);
     }
   else
     {
	if (_evry_icon_theme_set(obj, icon))
	  return 1;
	return _evry_icon_fdo_set(obj, icon);
     }
}

static int
_evry_cb_selection_notify(void *data, int type, void *event)
{
   Ecore_X_Event_Selection_Notify *ev;
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
	     _evry_update(s);
          }
     }

   return 1;
}


/* static int
 * _evry_cb_mouse_down(void *data __UNUSED__, int type __UNUSED__, void *event)
 * {
 *    Ecore_Event_Mouse_Button *ev;
 *    Evry_State *s =state;
 *
 *    ev = event;
 *    if (ev->event_window != input_window) return 1;
 *
 *    if (item_mouseover)
 *      {
 * 	if (s->sel_item != item_mouseover)
 * 	  {
 * 	     if (s->sel_item) _evry_list_item_desel(s->sel_item);
 * 	     s->sel_item = item_mouseover;
 * 	     _evry_list_item_sel(s->sel_item);
 * 	  }
 *      }
 *    else
 *      {
 * 	evas_event_feed_mouse_up(popup_list->evas, ev->buttons, 0, ev->timestamp, NULL);
 *      }
 *
 *    return 1;
 * }
 *
 * static int
 * _evry_cb_mouse_up(void *data __UNUSED__, int type __UNUSED__, void *event)
 * {
 *    Ecore_Event_Mouse_Button *ev;
 *
 *    ev = event;
 *    if (ev->event_window != input_window) return 1;
 *
 *    if (item_mouseover)
 *      {
 * 	if (ev->buttons == 1)
 * 	  _evry_plugin_action(1);
 * 	else if (ev->buttons == 3)
 * 	  _evry_plugin_action(0);
 *      }
 *    else
 *      {
 * 	evas_event_feed_mouse_up(popup_list->evas, ev->buttons, 0, ev->timestamp, NULL);
 *      }
 *
 *    return 1;
 * }
 *
 * static int
 * _evry_cb_mouse_move(void *data __UNUSED__, int type __UNUSED__, void *event)
 * {
 *    Ecore_Event_Mouse_Move *ev;
 *    Evry_State *s =state;
 *
 *    ev = event;
 *    if (ev->event_window != input_window) return 1;
 *
 *    if (!ev_last_is_mouse)
 *      {
 *         ev_last_is_mouse = 1;
 *         if (item_mouseover)
 *           {
 *              if (s->sel_item && (s->sel_item != item_mouseover))
 *                _evry_list_item_desel(s->sel_item);
 *              if (!s->sel_item || (s->sel_item != item_mouseover))
 *                {
 *                   s->sel_item = item_mouseover;
 *                   _evry_list_item_sel(s->sel_item);
 *                }
 *           }
 *      }
 *
 *    evas_event_feed_mouse_move(popup_list->evas, ev->x - popup_list->x,
 * 			      ev->y - popup_list->y, ev->timestamp, NULL);
 *
 *    return 1;
 * }
 *
 * static int
 * _evry_cb_mouse_wheel(void *data __UNUSED__, int type __UNUSED__, void *event)
 * {
 *    Ecore_Event_Mouse_Wheel *ev;
 *
 *    ev = event;
 *    if (ev->event_window != input_window) return 1;
 *
 *    ev_last_is_mouse = 0;
 *
 *    if (ev->z < 0) /\* up *\/
 *      {
 * 	int i;
 *
 * 	for (i = ev->z; i < 0; i++) _evry_list_item_prev();
 *      }
 *    else if (ev->z > 0) /\* down *\/
 *      {
 * 	int i;
 *
 * 	for (i = ev->z; i > 0; i--) _evry_list_item_next();
 *      }
 *    return 1;
 * }
 *
 * static void
 * _evry_cb_item_mouse_in(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
 * {
 *    Evry_State *s =state;
 *
 *    if (!ev_last_is_mouse) return;
 *
 *    item_mouseover = data;
 *
 *    if (s->sel_item) _evry_list_item_desel(s->sel_item);
 *    if (!(s->sel_item = data)) return;
 *    _evry_list_item_sel(s->sel_item);
 * }
 *
 * static void
 * _evry_cb_item_mouse_out(void *data __UNUSED__, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
 * {
 *    item_mouseover = NULL;
 * } */


/* static int
 * _evry_cb_plugin_sort_by_trigger(const void *data1, const void *data2)
 * {
 *    const Evry_Plugin *p1 = data1;
 *    const Evry_Plugin *p2 = data2;
 *    if (p1->trigger)
 *      {
 * 	if (!strncmp(state->input, p1->trigger, strlen(p1->trigger)))
 * 	  return 1;
 *      }
 *
 *    return p1->config->priority - p2->config->priority;
 * } */
