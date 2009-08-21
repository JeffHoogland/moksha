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
#define MAX_FUZZ 150
#define MAX_WORDS 5



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


static void _evry_matches_update(Evry_Selector *sel);
static void _evry_plugin_action(Evry_Selector *sel, int finished);
static void _evry_plugin_select(Evry_State *s, Evry_Plugin *p);
static void _evry_plugin_list_insert(Evry_State *s, Evry_Plugin *p);
static void _evry_backspace(Evry_State *s);
static void _evry_update(Evry_State *s, int fetch);
static void _evry_update_text_label(Evry_State *s);
static int  _evry_clear(Evry_State *s);
static int  _evry_update_timer(void *data);

static Evry_State *_evry_state_new(Evry_Selector *sel, Eina_List *plugins);
static void _evry_state_pop(Evry_Selector *sel);

static Evry_Selector *_evry_selector_new(int type);
static void _evry_selector_free(Evry_Selector *sel);
static void _evry_selector_activate(Evry_Selector *sel);
static void _evry_selectors_switch(void);
static void _evry_selector_update(Evry_Selector *sel);
static void _evry_selector_icon_set(Evry_Selector *sel);
static int  _evry_selector_subjects_get(const char *plugin_name);
static int  _evry_selector_actions_get(Evry_Item *it);
static int  _evry_selector_objects_get(Evry_Action *act);

static void _evry_browse_item(Evry_Selector *sel);
static void _evry_browse_back(Evry_Selector *sel);

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
static void _evry_view_hide(Evry_View *v);

static void _evry_item_desel(Evry_State *s, Evry_Item *it);
static void _evry_item_sel(Evry_State *s, Evry_Item *it);

static int  _evry_cb_key_down(void *data, int type, void *event);
static int  _evry_cb_selection_notify(void *data, int type, void *event);

/* local subsystem globals */
static Evry_Window *win = NULL;
static Evry_List_Window *list = NULL;
static Ecore_X_Window input_window = 0;
static Eina_List   *handlers = NULL;
static Ecore_Timer *update_timer = NULL;
static Evry_Plugin *action_selector = NULL;
static Evry_Selector *selector = NULL;

Evry_Selector **selectors;

/* externally accessible functions */
int
evry_init(void)
{
   action_selector = evry_plug_actions_new();
   return 1;
}

int
evry_shutdown(void)
{
   evry_hide();

   evry_plug_actions_free(action_selector);

   return 1;
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
     goto error;

   win = _evry_window_new(zone);
   if (!win) goto error;

   list = _evry_list_win_new(zone);
   if (!list) goto error;

   list->visible = EINA_FALSE;

   selectors = E_NEW(Evry_Selector*, 3);
   selectors[0] = _evry_selector_new(type_subject);
   selectors[1] = _evry_selector_new(type_action);
   selectors[2] = _evry_selector_new(type_object);

   if (params)
     win->plugin_dedicated = EINA_TRUE;

   _evry_selector_subjects_get(params);
   _evry_selector_activate(selectors[0]);

   _evry_selector_update(selector);

   e_popup_layer_set(list->popup, 255);
   e_popup_layer_set(win->popup, 255);
   e_popup_show(win->popup);
   e_popup_show(list->popup);

   if (!evry_conf->hide_input)
     edje_object_signal_emit(list->o_main, "e,state,entry_show", "e");

   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (ECORE_EVENT_KEY_DOWN, _evry_cb_key_down, NULL));
   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (ECORE_X_EVENT_SELECTION_NOTIFY,
       _evry_cb_selection_notify, win));

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

   _evry_view_clear(selector->state);

   if (update_timer)
     ecore_timer_del(update_timer);
   update_timer = NULL;

   list->visible = EINA_FALSE;
   _evry_selector_free(selectors[0]);
   _evry_selector_free(selectors[1]);
   _evry_selector_free(selectors[2]);
   selector = NULL;
   E_FREE(selectors);

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
   if (cb_free) it->cb_free = cb_free;

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

   /* printf("%d, %d\t free: %s\n",
    * 	  it->ref, item_cnt - 1,
    * 	  it->label);
    * item_cnt--; */

   if (it->cb_free) it->cb_free(it);

   if (it->label) eina_stringshare_del(it->label);
   if (it->uri) eina_stringshare_del(it->uri);
   if (it->mime) eina_stringshare_del(it->mime);

   if (it->o_bg) evas_object_del(it->o_bg);
   if (it->o_icon) evas_object_del(it->o_icon);

   E_FREE(it);
}

void
evry_item_select(const Evry_State *state, Evry_Item *it)
{
   Evry_State *s = (Evry_State *)state;

   s->plugin_auto_selected = EINA_FALSE;
   s->item_auto_selected = EINA_FALSE;
   _evry_item_sel(s, it);
   _evry_selector_update(selector);
}

void
evry_item_ref(Evry_Item *it)
{
   it->ref++;
}

int
evry_list_win_show(void)
{
   if (list->visible) return 0;

   _evry_list_win_show();
   return 1;
}


void
evry_list_win_hide(void)
{
   _evry_list_win_clear(1);
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
   if (!s || !eina_list_data_find(s->plugins, p))
     {
	/* if (p->type == type_action) */
	/* TODO */
	return;
     }

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
	if (eina_list_count(s->cur_plugins) > 1)
	  {
	     agg->fetch(agg, s->input);

	     /* add aggregator */
	     if (!(s->cur_plugins->data == agg))
	       {
		  s->cur_plugins = eina_list_prepend(s->cur_plugins, agg);

		  if (s->plugin_auto_selected)
		    _evry_plugin_select(s, agg);
	       }
	  }
	else
	  {
	     if (s->cur_plugins && s->cur_plugins->data == agg)
	       {
		  agg->cleanup(agg);
		  s->cur_plugins = eina_list_remove(s->cur_plugins, agg);
	       }
	  }

	/* plugin is visible */
	if ((s->plugin == p) || (s->plugin == agg))
	  _evry_selector_update(selector);

	_evry_view_update(s, NULL);
     }
}

int
evry_fuzzy_match(const char *str, const char *match)
{
   const char *p, *m, *next;
   int sum = 0;

   unsigned int last = 0;
   unsigned int offset = 0;
   unsigned int min = 0;
   unsigned char first = 0;
   /* ignore punctuation */
   unsigned char ip = 1;

   unsigned int cnt = 0;
   /* words in match */
   unsigned int m_num = 0;
   unsigned int m_cnt = 0;
   unsigned int m_min[MAX_WORDS];

   if (!match || !str) return 0;

   /* remove white spaces at the beginning */
   for (; (*match != 0) && isspace(*match); match++);
   for (; (*str != 0)   && isspace(*str);   str++);

   /* count words in match */
   for (m = match; (*m != 0) && (m_num < MAX_WORDS);)
     {
	for (; (*m != 0) && !isspace(*m); m++);
	for (; (*m != 0) &&  isspace(*m); m++);
	m_min[m_num++] = MAX_FUZZ;
     }
   for (m = match; ip && (*m != 0); m++)
     if (ip && ispunct(*m)) ip = 0;

   next = str;
   m = match;

   while((m_cnt < m_num) && (*next != 0))
     {
	/* reset match */
	if (m_cnt == 0) m = match;

	/* end of matching */
	if (*m == 0) break;

	offset = 0;
	last = 0;
	min = 1;
	first = 0;

	/* match current word of string against current match */
	for (p = next; *next != 0; p++)
	  {
	     /* new word of string begins */
	     if ((*p == 0) || isspace(*p) || (ip && ispunct(*p)))
	       {
		  if (m_cnt < m_num - 1)
		    {
		       /* test next match */
		       for (; (*m != 0) && !isspace(*m); m++);
		       for (; (*m != 0) &&  isspace(*m); m++);
		       m_cnt++;
		       break;
		    }
		  else
		    {
		       /* go to next word */
		       for (; (*p != 0) && ((isspace(*p) || (ip && ispunct(*p)))); p++);
		       cnt++;
		       next = p;
		       m_cnt = 0;
		       break;
		    }
	       }

	     /* current char matches? */
	     if (tolower(*p) != tolower(*m))
	       {
		  if (!first)
		    offset += 1;
		  else
		    offset += 3;

		  if (offset < 10)
		    continue;
	       }

	     if (min < MAX_FUZZ && offset < 10)
	       {
		  /* first offset of match in word */
		  if (!first)
		    {
		       first = 1;
		       last = offset;
		    }

		  min += offset + (offset - last) * 5;
		  last = offset;

		  /* try next char of match */
		  if (*(++m) != 0 && !isspace(*m))
		    continue;

		  /* end of match: store min weight of match */
		  min += (cnt - m_cnt) > 0 ? (cnt - m_cnt) : 0;

		  if (min < m_min[m_cnt])
		    m_min[m_cnt] = min;
	       }
	     else
	       {
		  /* go to next match */
		  for (; (*m != 0) && !isspace(*m); m++);
	       }

	     if (m_cnt < m_num - 1)
	       {
		  /* test next match */
		  for (; (*m != 0) && isspace(*m); m++);
		  m_cnt++;
		  break;
	       }
	     else if(*p != 0)
	       {
		  /* go to next word */
		  for (; (*p != 0) && !((isspace(*p) || (ip && ispunct(*p)))); p++);
		  for (; (*p != 0) &&  ((isspace(*p) || (ip && ispunct(*p)))); p++);
		  cnt++;
		  next = p;
		  m_cnt = 0;
		  break;
	       }
	     else
	       {
		  next = p;
		  break;
	       }
	  }
     }

   for (m_cnt = 0; m_cnt < m_num; m_cnt++)
     {
	sum += m_min[m_cnt];

	if (sum >= MAX_FUZZ)
	  {
	     sum = 0;
	     break;
	  }
     }

   return sum;
}


/* local subsystem functions */

static Evry_List_Window *
_evry_list_win_new(E_Zone *zone)
{
   int x, y, mw, mh;
   Evry_List_Window *list_win;
   E_Popup *popup;
   Evas_Object *o;
   const char *offset_y;

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

   evas_event_freeze(popup->evas);
   evas_event_feed_mouse_in(popup->evas, ecore_x_current_time_get(), NULL);
   evas_event_feed_mouse_move(popup->evas, -1000000, -1000000, ecore_x_current_time_get(), NULL);
   o = edje_object_add(popup->evas);
   list_win->o_main = o;
   e_theme_edje_object_set(o, "base/theme/everything",
			   "e/modules/everything/list");
   offset_y = edje_object_data_get(o, "offset_y");
   edje_object_size_min_get(o, &mw, &mh);
   if (mh == 0) mh = 200;
   if (mw == 0) mw = win->popup->w / 2;

   x = (win->popup->x + win->popup->w / 2) - (mw / 2);
   y = (win->popup->y + win->popup->h) + (offset_y ? atoi(offset_y) : 0);

   e_popup_move_resize(popup, x, y, mw, mh);

   o = list_win->o_main;
   evas_object_move(o, 0, 0);
   evas_object_resize(o, list_win->popup->w, list_win->popup->h);
   evas_object_show(o);
   e_popup_edje_bg_object_set(popup, o);

   evas_event_thaw(popup->evas);

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
     }
}


static Evry_Window *
_evry_window_new(E_Zone *zone)
{
   int x, y, mw, mh;
   Evry_Window *win;
   E_Popup *popup;
   Evas_Object *o;

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
			   "e/modules/everything/selector_item");
   evas_object_show(o);

   if (type == type_subject)
     edje_object_part_swallow(win->o_main, "e.swallow.subject_selector", o);
   else if (type == type_action)
     edje_object_part_swallow(win->o_main, "e.swallow.action_selector", o);
   else if (type == type_object)
     edje_object_part_swallow(win->o_main, "e.swallow.object_selector", o);

   p = evry_plug_aggregator_new(sel);

   sel->plugins = eina_list_append(sel->plugins, p);
   sel->aggregator = p;

   EINA_LIST_FOREACH(evry_conf->plugins, l, p)
     {
	if (!p->config->enabled) continue;
	if (p->type != type) continue;
	sel->plugins = eina_list_append(sel->plugins, p);
     }

   return sel;
}

static void
_evry_selector_free(Evry_Selector *sel)
{
   if (sel->o_icon)
     evas_object_del(sel->o_icon);
   evas_object_del(sel->o_main);

   if (list->visible && (sel == selector))
     _evry_view_clear(sel->state);

   while (sel->states)
     _evry_state_pop(sel);

   evry_plug_aggregator_free(sel->aggregator);

   if (sel->plugins) eina_list_free(sel->plugins);
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
	     s->view->clear(s->view);
	     _evry_view_hide(s->view);
	  }

	_evry_list_win_clear(evry_conf->hide_list);
     }

   selector = sel;
   s = selector->state;

   edje_object_signal_emit(sel->o_main, "e,state,selected", "e");

   if (s)
     {
	_evry_update_text_label(s);

	if (sel->state->sel_item)
	  edje_object_part_text_set(sel->o_main, "e.text.plugin",
				    s->sel_item->plugin->name);

	_evry_view_show(s->view);
	_evry_list_win_update(s);
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
	o = evry_icon_theme_get(s->plugin->icon, win->popup->evas);
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

   if (s)
     {
	it = s->sel_item;

	if (!s->plugin && it)
	  _evry_item_desel(s, NULL);
	else if (it && !eina_list_data_find_list(s->plugin->items, it))
	  _evry_item_desel(s, NULL);

	it = s->sel_item;

	if (s->plugin && (!it || s->item_auto_selected))
	  {
	     /* get first item */
	     if (s->plugin->items)
	       {
		  it = s->plugin->items->data;
		  s->item_auto_selected = EINA_TRUE;
		  _evry_item_sel(s, it);
	       }
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
	_evry_selector_actions_get(it);
	_evry_selector_update(selectors[1]);
     }
}

static void
_evry_list_win_update(Evry_State *s)
{
   if (s != selector->state) return;
   if (!list->visible) return;

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
   _evry_matches_update(sel);

   return 1;
}

static int
_evry_selector_actions_get(Evry_Item *it)
{
   Eina_List *l, *plugins = NULL;
   Evry_Plugin *p, *plugin;
   Evry_Selector *sel = selectors[1];
   const char *type_out;

   while (sel->state)
     _evry_state_pop(sel);

   if (!it) return 0;

   type_out = it->plugin->type_out;

   EINA_LIST_FOREACH(sel->plugins, l, plugin)
     {
	if ((plugin == action_selector) || (plugin == sel->aggregator) ||
	    (plugin->type_in &&  type_out && plugin->type_in == type_out))
	  {
	     if (plugin->begin)
	       {
		  if ((p = plugin->begin(plugin, it)))
		    plugins = eina_list_append(plugins, p);
	       }
	     else
	       plugins = eina_list_append(plugins, plugin);
	  }
     }

   if (!plugins) return 0;

   _evry_state_new(sel, plugins);
   _evry_matches_update(sel);

   return 1;
}

static int
_evry_selector_objects_get(Evry_Action *act)
{
   Eina_List *l, *plugins = NULL;
   Evry_Plugin *p, *plugin;
   Evry_Selector *sel = selectors[2];
   Evry_Item *it;
   const char *type_in = act->type_in2;

   while (sel->state)
     _evry_state_pop(sel);

   it = selectors[0]->state->sel_item;

   EINA_LIST_FOREACH(sel->plugins, l, plugin)
     {
	if ((plugin->type_out != type_in) &&
	    (plugin != sel->aggregator)) continue;

	if (plugin->begin)
	  {
	     if ((act->type_out) &&
		 (act->type_out == plugin->type_in) &&
		 (p = plugin->begin(plugin, it)))
	       plugins = eina_list_append(plugins, p);
	     else if (p = plugin->begin(plugin, NULL))
	       plugins = eina_list_append(plugins, p);
	  }
	else
	  plugins = eina_list_append(plugins, plugin);
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

   _evry_item_desel(s, NULL);

   free(s->input);

   EINA_LIST_FREE(s->plugins, p)
     p->cleanup(p);

   if (s->view)
     s->view->destroy(s->view);

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
   Evry_Plugin *p, *plugin;
   const char *type_out;

   it = s->sel_item;

   if (!it || !it->browseable)
     return;

   type_out = it->plugin->type_out;

   if (!type_out)
     return;

   EINA_LIST_FOREACH(sel->plugins, l, plugin)
     {
	if ((!plugin->begin || !plugin->type_in) ||
	    (plugin->type_in != type_out))
	  continue;

	if ((p = plugin->begin(plugin, it)))
	  plugins = eina_list_append(plugins, p);
     }

   if (!plugins) return;

   _evry_view_hide(s->view);

   _evry_state_new(sel, plugins);
   _evry_matches_update(sel);
   _evry_selector_update(sel);
   _evry_list_win_update(sel->state);
   _evry_update_text_label(sel->state);
}

static void
_evry_browse_back(Evry_Selector *sel)
{
   Evry_State *s = sel->state;

   if (!s || !sel->states->next)
     return;

   _evry_state_pop(sel);

   s = sel->state;

   sel->aggregator->fetch(sel->aggregator, s->input);
   _evry_selector_update(sel);
   _evry_update_text_label(s);
   
   _evry_view_show(s->view);
}

static void
_evry_selectors_switch(void)
{
   Evry_State *s = selector->state;

   if (update_timer)
     {
	if ((s && !s->plugin->async_fetch) &&
	    ((selector == selectors[0]) ||
	     (selector == selectors[1])))
	  {
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
	Evry_Action *act;

	if ((s->sel_item) &&
	    (s->sel_item->plugin == action_selector) &&
	    (act = s->sel_item->data[0]) &&
	    (act->type_in2))
	  {
	     _evry_selector_objects_get(act);
	     _evry_selector_update(selectors[2]);
	     edje_object_signal_emit(win->o_main,
				     "e,state,object_selector_show", "e");
	     next_selector = 2;
	  }
	_evry_selector_activate(selectors[next_selector]);
     }
   else if (selector == selectors[2])
     {
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
   const char *key = NULL, *old;

   win->request_selection = EINA_FALSE;

   ev = event;
   if (ev->event_window != input_window) return 1;

   old = ev->key;

   if (evry_conf->quick_nav && (ev->modifiers & ECORE_EVENT_MODIFIER_ALT))
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
   else
     {
	key = eina_stringshare_add(ev->key);
	ev->key = key;
     }

   if (!list->visible && (!strcmp(key, "Down")))
     _evry_list_win_show();
   else if (ev->modifiers & ECORE_EVENT_MODIFIER_CTRL)
     {
	if (!strcmp(key, "u"))
	  {
	     if (!_evry_clear(s))
	       _evry_browse_back(selector);
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
   else if (_evry_view_key_press(s, ev))
     goto end;
   else if (!strcmp(key, "Right"))
     _evry_browse_item(selector);
   else if (!strcmp(key, "Left"))
     _evry_browse_back(selector);
   else if (!strcmp(key, "Return"))
     {
	if (ev->modifiers & ECORE_EVENT_MODIFIER_SHIFT)
	  _evry_plugin_action(selector, 0);
	else
	  _evry_plugin_action(selector, 1);
     }
   else  if (!strcmp(key, "Escape"))
     evry_hide();
   else if (!strcmp(key, "Tab"))
     _evry_selectors_switch();
   else if (!strcmp(key, "BackSpace"))
     _evry_backspace(s);
   else if (!strcmp(key, "Delete"))
     _evry_backspace(s);
   else if (_evry_view_key_press(s, ev))
     goto end;
   else if ((ev->compose && !(ev->modifiers & ECORE_EVENT_MODIFIER_ALT)))
     {
	int len = strlen(s->input);
	if (len < (INPUTLEN - strlen(ev->compose)))
	  {
	     strcat(s->input, ev->compose);
	     /* if ((len == 0) && isspace(s->input[0]))
	      *   _evry_show_triggers(); */
	     if ((len == 1) &&
		 (isspace(s->input[0])) &&
		 (_evry_view_toggle(s, s->input + 1)))
	       _evry_update(s, 0);
	     else if (isspace(*ev->compose))
	       _evry_update(s, 0);
	     else
	       _evry_update(s, 1);
	  }
     }

 end:
   eina_stringshare_del(ev->key);
   ev->key = old;
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
	     val = *(s->input + pos);

	     s->input[pos] = 0;

	     if ((pos == 0) || !isspace(val))
	       _evry_update(s, 1);
	  }
     }
}

static void
_evry_update_text_label(Evry_State *s)
{
   if (evry_conf->hide_input)
     {
	if (strlen(s->input) > 0)
	  edje_object_signal_emit(list->o_main, "e,state,entry_show", "e");
	else
	  edje_object_signal_emit(list->o_main, "e,state,entry_hide", "e");
     }

   edje_object_part_text_set(win->o_main, "e.text.label", s->input);
   edje_object_part_text_set(list->o_main, "e.text.label", s->input);

}

static void
_evry_update(Evry_State *s, int fetch)
{
   _evry_update_text_label(s);

   if (fetch)
     {
	if (update_timer) ecore_timer_del(update_timer);
	update_timer = ecore_timer_add(MATCH_LAG, _evry_update_timer, s);

	if (s->plugin && !s->plugin->trigger)
	  edje_object_signal_emit(list->o_main, "e,signal,update", "e");
     }
}

static int
_evry_update_timer(void *data)
{
   _evry_matches_update(selector);
   _evry_selector_update(selector);
   _evry_list_win_update(selector->state);
   update_timer = NULL;

   return 0;
}

static int
_evry_clear(Evry_State *s)
{
   if ((s->plugin && s->plugin->trigger && s->input) &&
       (!strncmp(s->plugin->trigger, s->input, strlen(s->plugin->trigger))))
     {
	s->input[strlen(s->plugin->trigger)] = 0;
	_evry_update(s, 1);
	return 1;
     }
   else if (s->input[0] != 0)
     {
	s->input[0] = 0;
	_evry_update(s, 1);
	if (evry_conf->hide_input)
	  edje_object_signal_emit(list->o_main, "e,state,entry_hide", "e");
	return 1;
     }
   return 0;
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
	    (!selector->state->plugin->async_fetch))
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

	if (selectors[2] == selector)
	  it_object = selector->state->sel_item;

	if (act->type_in2 && !it_object) return;

	act->action(act, s_subject->sel_item, it_object, NULL);
     }
   else
     {
	Evry_Item *it = s_action->sel_item;
	s_action->plugin->action(s_action->plugin, it,
				 selector->state->input);
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
_evry_view_hide(Evry_View *v)
{
   if (!v) return;

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
     s->view->update(s->view);
}

static void
_evry_view_clear(Evry_State *s)
{
   if (!s || !s->view) return;

   s->view->clear(s->view);
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

   if (trigger)
     {
	EINA_LIST_FOREACH(evry_conf->views, ll, view)
	  {
	     if (view->trigger && !strncmp(trigger, view->trigger, 1) &&
		 (v = view->create(view, s, list->o_main)))
	       goto found;
	  }
     }
   else
     {
	l = eina_list_data_find_list(evry_conf->views, s->view->id);

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
	_evry_view_hide(s->view);
	s->view->destroy(s->view);
     }

   s->view = v;
   _evry_view_show(v);
   v->update(v);

   return 1;
}

static void
_evry_matches_update(Evry_Selector *sel)
{
   Evry_State *s = sel->state;
   Evry_Plugin *p;
   Eina_List *l;
   const char *input;

   EINA_LIST_FREE(s->cur_plugins, p);

   if (strlen(s->input) > 0)
     input = s->input;
   else
     input = NULL;

   if (input)
     {
	EINA_LIST_FOREACH(s->plugins, l, p)
	  {
	     /* input matches plugin trigger? */
	     if (!p->trigger) continue;

	     if ((strlen(s->input) >= strlen(p->trigger)) &&
		 (!strncmp(s->input, p->trigger, strlen(p->trigger))))
	       {
		  s->cur_plugins = eina_list_append(s->cur_plugins, p);
		  p->fetch(p, s->input + strlen(p->trigger));
		  break;
	       }
	  }
     }

   if (!s->cur_plugins)
     {
	EINA_LIST_FOREACH(s->plugins, l, p)
	  {
	     if (!win->plugin_dedicated && p->trigger) continue;
	     if (p == sel->aggregator) continue;

	     if (p->fetch(p, input)  ||
		 (sel->states->next) ||
		 (win->plugin_dedicated))
	       s->cur_plugins = eina_list_append(s->cur_plugins, p);
	  }

	if (eina_list_count(s->cur_plugins) > 1)
	  {
	     sel->aggregator->fetch(sel->aggregator, s->input);
	     s->cur_plugins = eina_list_prepend(s->cur_plugins, sel->aggregator);
	     if (s->plugin_auto_selected)
	       _evry_plugin_select(s, NULL);
	  }
	else
	  sel->aggregator->cleanup(sel->aggregator);
     }

   if (s->plugin && !eina_list_data_find_list(s->cur_plugins, s->plugin))
     s->plugin = NULL;

   _evry_plugin_select(s, s->plugin);
}

static void
_evry_item_desel(Evry_State *s, Evry_Item *it)
{
   if (s->sel_item)
     evry_item_free(s->sel_item);

   s->sel_item = NULL;
}

static void
_evry_item_sel(Evry_State *s, Evry_Item *it)
{
   if (s->sel_item == it) return;

   _evry_item_desel(s, NULL);

   evry_item_ref(it);
   s->sel_item = it;
}

static void
_evry_plugin_select(Evry_State *s, Evry_Plugin *p)
{
   if (!s || !s->cur_plugins) return;

   if (!p && s->cur_plugins)
     {
	p = s->cur_plugins->data;
	s->plugin_auto_selected = EINA_TRUE;
     }
   else s->plugin_auto_selected = EINA_FALSE;

   if ((p || !s->plugin) &&  s->plugin != p)
     {
	_evry_item_desel(s, NULL);
	s->plugin = p;
     }
}

void
evry_plugin_select(const Evry_State *s, Evry_Plugin *p)
{
   _evry_plugin_select((Evry_State *) s, p);

   _evry_selector_update(selector);
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
	     _evry_update(s, 1);
          }
     }

   return 1;
}

