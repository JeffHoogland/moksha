#include "e.h"
#include "e_mod_main.h"

#define INPUTLEN 40
#define MATCH_LAG 0.33


typedef struct _Evry_State Evry_State;

struct _Evry_State
{
  Eina_List *cur_items;
  Eina_List *cur_plugins;
  Eina_List *plugins;
  Evry_Plugin *cur_plugin;
  Evry_Item *sel_item;

  Eina_List *cur_actions;
  Evry_Action *cur_action;

  /* Eina_List *sel_items; */
  char *input;
  const char *request_type;

  Eina_Bool initial;

  /* this is for the case when the current plugin was not selected
     manually and a higher priority (async) plugin retrieves
     candidates, the higher priority plugin is made current */
  Eina_Bool plugin_auto_selected;

};

static int  _evry_cb_key_down(void *data, int type, void *event);
static int  _evry_cb_key_down(void *data, int type, void *event);
static int  _evry_cb_mouse_down(void *data, int type, void *event);
static int  _evry_cb_mouse_up(void *data, int type, void *event);
static int  _evry_cb_mouse_move(void *data, int type, void *event);
static int  _evry_cb_mouse_wheel(void *data, int type, void *event);
static void _evry_cb_item_mouse_in(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void _evry_cb_item_mouse_out(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void _evry_backspace(void);
static void _evry_update(void);
static void _evry_list_clear(void);
static void _evry_show_items(Evry_Plugin *plugin);
static int  _evry_update_timer(void *data);
static void _evry_matches_update(Evry_Plugin *cur_plugin);
static void _evry_clear(void);
static void _evry_item_next(void);
static void _evry_item_prev(void);
static void _evry_plugin_next(void);
static void _evry_plugin_prev(void);
static void _evry_scroll_to(int i);
static void _evry_tab_scroll_to(Evry_Plugin *p);
static void _evry_item_desel(Evry_Item *it);
static void _evry_item_sel(Evry_Item *it);
static void _evry_plugin_action(int finished);
static void _evry_action_select(void);
static void _evry_cb_plugin_sel(void *data1, void *data2);
static int  _evry_animator(void *data);
static int  _evry_scroll_timer(void *data);
static int  _evry_push_state(void);
static int  _evry_pop_state(void);
static void _evry_plugin_selector_show(Evry_Plugin *p);
static int  _evry_cb_plugin_sort(const void *data1, const void *data2);

static int  _evry_plug_act_select_init(void);
static int  _evry_plug_act_select_actions_set(void);
static int  _evry_plug_act_select_fetch(Evry_Plugin *p, const char *input);
static int  _evry_plug_act_select_action(Evry_Plugin *p, Evry_Item *item, const char *input);
static void _evry_plug_act_select_cleanup(Evry_Plugin *p);
static void _evry_plug_act_select_item_icon_get(Evry_Plugin *p, Evry_Item *it, Evas *e);

/* local subsystem globals */
static E_Popup     *popup = NULL;
static Ecore_X_Window input_window = 0;
static Eina_List   *handlers = NULL;
static Ecore_Timer *update_timer = NULL;
static Evas_Object *o_list = NULL;
static Evas_Object *o_main = NULL;
static Evas_Object *o_selector = NULL;
static Evas_Object *o_selector_frame = NULL;
static int          ev_last_is_mouse;
static Evry_Item   *item_mouseover = NULL;
static Ecore_Animator *scroll_animator = NULL;
static Ecore_Timer *scroll_timer = NULL;
static double       scroll_align_to;
static double       scroll_align;
/* static Eina_List   *plugins = NULL; */
static Eina_List   *actions = NULL;
static Evry_State  *cur_state = NULL;
static Eina_List   *stack = NULL;
static Evry_Plugin *action_selector = NULL;

/* externally accessible functions */
EAPI int
evry_init(void)
{
   _evry_plug_act_select_init();

   return 1;
}

EAPI int
evry_shutdown(void)
{
   evry_hide();
   return 1;
}

EAPI void
evry_plugin_register(Evry_Plugin *plugin)
{
   Eina_List *l;
   Plugin_Config *pc;
   Eina_Bool found = 0;
   
   evry_conf->plugins = eina_list_append(evry_conf->plugins, plugin);

   EINA_LIST_FOREACH(evry_conf->plugins_conf, l, pc)
     {
	if (pc->name && plugin->name && !strcmp(pc->name, plugin->name))
	  {
	     found = 1;
	     break;
	  }
     }

   if (!found)
     {
	pc = E_NEW(Plugin_Config, 1);
	pc->name = eina_stringshare_add(plugin->name);
	pc->enabled = 1;
	pc->priority = eina_list_count(evry_conf->plugins);

	evry_conf->plugins_conf = eina_list_append(evry_conf->plugins_conf, pc);	
     }

   if (plugin->trigger && !pc->trigger)
     pc->trigger = eina_stringshare_add(plugin->trigger);
   
   plugin->config = pc;

   evry_conf->plugins = eina_list_sort(evry_conf->plugins,
				       eina_list_count(evry_conf->plugins),
				       _evry_cb_plugin_sort);
   
   /* TODO sorting, initialization, etc */
}

EAPI void
evry_plugin_unregister(Evry_Plugin *plugin)
{
   evry_conf->plugins = eina_list_remove(evry_conf->plugins, plugin);
   /* cleanup */
}

EAPI void
evry_action_register(Evry_Action *action)
{
   actions = eina_list_append(actions, action);
   /* TODO sorting, initialization, etc */
}

EAPI void
evry_action_unregister(Evry_Action *action)
{
   actions = eina_list_remove(actions, action);
   /* cleanup */
}

EAPI int
evry_show(E_Zone *zone)
{
   Evas_Object *o;
   int x, y;

   E_OBJECT_CHECK_RETURN(zone, 0);
   E_OBJECT_TYPE_CHECK_RETURN(zone, E_ZONE_TYPE, 0);

   if (popup) return 0;

   input_window = ecore_x_window_input_new(zone->container->win, zone->x,
					   zone->y, zone->w, zone->h);
   ecore_x_window_show(input_window);
   if (!e_grabinput_get(input_window, 1, input_window))
     {
        ecore_x_window_free(input_window);
	input_window = 0;
	return 0;
     }

   x = (zone->w / 2) - (evry_conf->width / 2);
   y = (zone->h / 2) - (evry_conf->height / 2);

   popup = e_popup_new(zone, x, y, evry_conf->width, evry_conf->height);
   if (!popup) return 0;

   ecore_x_netwm_window_type_set(popup->evas_win, ECORE_X_WINDOW_TYPE_UTILITY);

   e_popup_layer_set(popup, 255);
   evas_event_freeze(popup->evas);
   evas_event_feed_mouse_in(popup->evas, ecore_x_current_time_get(), NULL);
   evas_event_feed_mouse_move(popup->evas, -1000000, -1000000, ecore_x_current_time_get(), NULL);
   o = edje_object_add(popup->evas);
   o_main = o;
   e_theme_edje_object_set(o, "base/theme/everything",
			   "e/widgets/everything/main");

   o = e_box_add(popup->evas);
   o_list = o;
   e_box_orientation_set(o, 0);
   e_box_homogenous_set(o, 1);
   edje_object_part_swallow(o_main, "e.swallow.list", o);
   evas_object_show(o);

   o = o_main;
   evas_object_move(o, 0, 0);
   evas_object_resize(o, evry_conf->width, evry_conf->height);
   evas_object_show(o);
   e_popup_edje_bg_object_set(popup, o);

   o = e_box_add(popup->evas);
   e_box_orientation_set(o, 1);
   e_box_homogenous_set(o, 1);
   edje_object_part_swallow(o_main, "e.swallow.bar", o);
   evas_object_show(o);
   o_selector = o;

   evas_event_thaw(popup->evas);

   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (ECORE_EVENT_KEY_DOWN, _evry_cb_key_down, NULL));
   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (ECORE_EVENT_MOUSE_BUTTON_DOWN, _evry_cb_mouse_down, NULL));
   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (ECORE_EVENT_MOUSE_BUTTON_UP, _evry_cb_mouse_up, NULL));
   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (ECORE_EVENT_MOUSE_MOVE, _evry_cb_mouse_move, NULL));
   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (ECORE_EVENT_MOUSE_WHEEL, _evry_cb_mouse_wheel, NULL));

   _evry_push_state();

   e_popup_show(popup);

   e_box_align_set(o_selector, 0.0, 0.5);
   return 1;
}

EAPI void
evry_hide(void)
{
   Ecore_Event *ev;
   char *str;
   Evry_Plugin *plugin;
   Eina_List *l;
   Evry_State *s;

   if (!popup) return;

   evas_event_freeze(popup->evas);

   _evry_list_clear();

   EINA_LIST_FREE(stack, s)
     {
	Evry_Plugin *p;

	free(s->input);

	EINA_LIST_FREE(s->plugins, p)
	  {
	     p->cleanup(p);

	     if (p->tab)
	       {
		  evas_object_del(p->tab);
		  p->tab = NULL;
	       }
	  }
     }
   stack = NULL;
   cur_state = NULL;

   if (update_timer)
     {
	ecore_timer_del(update_timer);
	update_timer = NULL;
     }
   if (scroll_timer)
     {
	ecore_timer_del(scroll_timer);
	scroll_timer = NULL;
     }
   if (scroll_animator)
     {
	ecore_animator_del(scroll_animator);
	scroll_animator = NULL;
     }

   e_popup_hide(popup);

   evas_object_del(o_list);
   o_list = NULL;

   evas_object_del(o_selector);
   o_selector = NULL;

   evas_object_del(o_main);
   o_main = NULL;

   evas_event_thaw(popup->evas);
   e_object_del(E_OBJECT(popup));
   popup = NULL;

   EINA_LIST_FREE(handlers, ev)
     ecore_event_handler_del(ev);

   ecore_x_window_free(input_window);
   e_grabinput_release(input_window, input_window);
   input_window = 0;
}

EAPI void
evry_plugin_async_update(Evry_Plugin *p, int action)
{
   Evry_State *s;

   if (!popup) return;

   s = cur_state;

   if (action == EVRY_ASYNC_UPDATE_ADD)
     {
	if (!eina_list_data_find(s->cur_plugins, p))
	  {
	     s->cur_plugins = eina_list_append(s->cur_plugins, p);
	     _evry_plugin_selector_show(p);
	  }
	else
	  {
	     char buf[64];
	     snprintf(buf, 64, "%s (%d)", p->name, eina_list_count(p->items));
	     edje_object_part_text_set(p->tab, "e.text.label", buf);
	  }

	if ((!s->cur_plugin && s->cur_plugins) || (s->cur_plugin == p) ||
	    (s->plugin_auto_selected && (s->cur_plugin->config->priority > p->config->priority)))
	  {
	     if (!s->cur_plugin)
	       s->plugin_auto_selected = 1;

	     _evry_list_clear();
	     _evry_show_items(s->cur_plugins->data);
	  }
	else if (s->cur_plugin)
	  _evry_tab_scroll_to(s->cur_plugin); 
     }
   else if ((action == EVRY_ASYNC_UPDATE_CLEAR) && (s->cur_plugin == p))
     {
	_evry_list_clear();
	/*XXX  move this into list clear?*/
	s->cur_items = NULL;
     }
}

/* local subsystem functions */
static int
_evry_cb_plugin_sort(const void *data1, const void *data2)
{
   const Evry_Plugin *p1 = data1;
   const Evry_Plugin *p2 = data2;
   return p1->config->priority - p2->config->priority;
}

static int
_evry_push_state(void)
{
   Evry_State *s;
   Eina_List *l, *list = NULL;
   const char *cur_type = "NONE";
   Evry_Plugin *p;

   s = cur_state;

   if (s)
     {
	if (!s->cur_plugin || !s->sel_item)
	  return 0;

	cur_type = s->cur_plugin->type_out;
	/* current plugin provides no candidates to act on by others */
	if (!s->cur_action && !strcmp(cur_type, "NONE"))
	  return 0;

	EINA_LIST_FOREACH(s->plugins, l, p)
	  {
	     if (p != s->cur_plugin)
	       p->cleanup(p);

	     if (p->tab)
	       {
		  evas_object_del(p->tab);
		  p->tab = NULL;
	       }
	  }
     }

   EINA_LIST_FOREACH(evry_conf->plugins, l, p)
     {
	if (!p->config->enabled) continue;
	
	if (strstr(p->type_in, cur_type))
	  {
	     /* printf("%s- in:%s out:%s\n", p->name, p->type_in, p->type_out); */

	     if (s && s->cur_action)
	       {
		  /* filter out plugins that dont provide the type
		     requested by current action */
		  if (strcmp(p->type_out, s->request_type))
		    continue;
	       }

	     if (p->begin)
	       {
		  Evry_Item *it = NULL;

		  if (s && (s->cur_plugin != action_selector))
		    it = s->sel_item;

		  if (p->begin(p, it))
		    list = eina_list_append(list, p);
	       }
	     else
	       list = eina_list_append(list, p);
	  }
     }

   if (s && !s->cur_action)
     {
	if (_evry_plug_act_select_actions_set())
	  {
	     list = eina_list_append(list, action_selector);
	  }
     }

   if (!list) return 0;

   /* list = eina_list_sort(list, eina_list_count(list), _evry_cb_plugin_sort); */

   _evry_list_clear();

   s = E_NEW(Evry_State, 1);
   s->input = malloc(INPUTLEN);
   s->input[0] = 0;
   s->plugins = list;
   s->cur_plugins = NULL;
   s->sel_item = NULL;
   s->plugin_auto_selected = 1;

   if (cur_state)
     {
	s->request_type = cur_state->request_type;
	s->cur_action = cur_state->cur_action;
	s->initial = 0;
     }
   else
     {
	s->initial = 1;
     }

   cur_state = s;
   stack = eina_list_prepend(stack, s);

   edje_object_part_text_set(o_main, "e.text.label", s->input);

   ev_last_is_mouse = 0;
   item_mouseover = NULL;

   _evry_matches_update(s->cur_plugin);

   return 1;
}

static int
_evry_pop_state(void)
{
   Evry_State *s, *last_state;
   Evry_Plugin *p;
   Eina_List *l;

   if (!stack || !stack->next) return 0;

   s = cur_state;

   stack = eina_list_remove_list(stack, stack);
   last_state = stack->data;
   
   _evry_list_clear();

   free(s->input);

   EINA_LIST_FOREACH(s->plugins, l, p)
     p->cleanup(p);

   E_FREE(s);

   if (stack)
     {
	Evry_Item *it = NULL;
	int i = 0;

	s = last_state;
	cur_state = s;

	edje_object_part_text_set(o_main, "e.text.label", s->input);

	if (s->sel_item)
	  {
	     Evry_Item *it;
	     EINA_LIST_FOREACH(s->cur_plugin->items, l, it)
	       if (it == s->sel_item)
		 break;
	       else i++;
	  }

	if (stack->next)
	  {
	     Evry_State *prev = stack->next->data;
	     it = prev->sel_item;
	  }

	EINA_LIST_FOREACH(s->plugins, l, p)
	  if (p != s->cur_plugin && p->begin)
	    p->begin(p, it);

	p = s->cur_plugin;
	s->cur_plugin = NULL;
	_evry_matches_update(p);

	it = eina_list_nth(s->cur_plugin->items, i);

	if (it)
	  {
	     if (s->sel_item) _evry_item_desel(s->sel_item);
	     s->sel_item = it;
	     _evry_item_sel(s->sel_item);
	     _evry_scroll_to(i);
	  }
     }

   /* else
    *   {
    * 	cur_state = NULL;
    * 	evry_hide();
    *   } */

   ev_last_is_mouse = 0;
   item_mouseover = NULL;

   return 1;
}

/* TODO config options for users preferred keys */
static int
_evry_cb_key_down(void *data, int type, void *event)
{
   Ecore_Event_Key *ev;
   Evry_State *s = cur_state;
   ev_last_is_mouse = 0;
   item_mouseover = NULL;

   ev = event;
   if (ev->event_window != input_window) return 1;

   if      (!strcmp(ev->key, "Up"))
     _evry_item_prev();
   else if (!strcmp(ev->key, "Down"))
     _evry_item_next();
   else if (!strcmp(ev->key, "Right") &&
	    ((ev->modifiers & ECORE_EVENT_MODIFIER_CTRL) ||
	     (ev->modifiers & ECORE_EVENT_MODIFIER_SHIFT)))
     {
	if (s->sel_item)
	  {
	     if (!_evry_push_state())
	       _evry_plugin_action(0);
	  }
     }
   /* _evry_plugin_next(); */
   else if (!strcmp(ev->key, "Left") &&
	    ((ev->modifiers & ECORE_EVENT_MODIFIER_CTRL) ||
	     (ev->modifiers & ECORE_EVENT_MODIFIER_SHIFT)))
     _evry_pop_state();
   /* _evry_plugin_prev(); */
   else if (!strcmp(ev->key, "Right"))
     _evry_plugin_next();
   /* {
    * 	if (s->sel_item)
    * 	  {
    * 	     if (!_evry_push_state())
    * 	       _evry_plugin_action(0);
    * 	  }
    * } */
   else if (!strcmp(ev->key, "Left"))
     _evry_plugin_prev();
   /* _evry_pop_state();      */
   else if (!strcmp(ev->key, "Return") &&
	    (ev->modifiers & ECORE_EVENT_MODIFIER_CTRL))
     _evry_plugin_action(0);
   else if (!strcmp(ev->key, "Return"))
     _evry_plugin_action(1);
   else if (!strcmp(ev->key, "Tab") &&
	    ((ev->modifiers & ECORE_EVENT_MODIFIER_CTRL) ||
	     (ev->modifiers & ECORE_EVENT_MODIFIER_SHIFT)))
     _evry_plugin_prev();
   else if (!strcmp(ev->key, "Tab"))
     {
     	if (s->sel_item)
     	  {
     	     if (!_evry_push_state())
     	       _evry_plugin_action(0);
     	  }
     }
   /* _evry_plugin_next(); */
   else if (!strcmp(ev->key, "u") &&
	    (ev->modifiers & ECORE_EVENT_MODIFIER_CTRL))
     _evry_clear();
   else  if ((!strcmp(ev->key, "Escape")) ||
	     (!strcmp(ev->key, "e") &&
	      (ev->modifiers & ECORE_EVENT_MODIFIER_CTRL)))
     {
	if (!_evry_pop_state())
	  evry_hide();
     }

   else if (!strcmp(ev->key, "BackSpace"))
     _evry_backspace();
   else if (!strcmp(ev->key, "Delete"))
     _evry_backspace();
   else
     {
   	if (ev->compose)
   	  {
   	     if ((strlen(s->input) < (INPUTLEN - strlen(ev->compose))))
   	       {
   		  strcat(s->input, ev->compose);
   		  _evry_update();
   	       }
   	  }
     }
   return 1;
}

static int
_evry_cb_mouse_down(void *data, int type, void *event)
{
   Ecore_Event_Mouse_Button *ev;
   Evry_State *s =cur_state;

   ev = event;
   if (ev->event_window != input_window) return 1;

   if (item_mouseover)
     {
	if (s->sel_item != item_mouseover)
	  {
	     if (s->sel_item) _evry_item_desel(s->sel_item);
	     s->sel_item = item_mouseover;
	     _evry_item_sel(s->sel_item);
	  }
     }
   else
     {
	evas_event_feed_mouse_up(popup->evas, ev->buttons, 0, ev->timestamp, NULL);
     }

   return 1;
}

static int
_evry_cb_mouse_up(void *data, int type, void *event)
{
   Ecore_Event_Mouse_Button *ev;

   ev = event;
   if (ev->event_window != input_window) return 1;

   if (item_mouseover)
     {
	if (ev->buttons == 1)
	  _evry_plugin_action(1);
	else if (ev->buttons == 3)
	  _evry_plugin_action(0);
     }
   else
     {
	evas_event_feed_mouse_up(popup->evas, ev->buttons, 0, ev->timestamp, NULL);
     }

   return 1;
}

static int
_evry_cb_mouse_move(void *data, int type, void *event)
{
   Ecore_Event_Mouse_Move *ev;
   Evry_State *s =cur_state;

   ev = event;
   if (ev->event_window != input_window) return 1;

   if (!ev_last_is_mouse)
     {
        ev_last_is_mouse = 1;
        if (item_mouseover)
          {
             if (s->sel_item && (s->sel_item != item_mouseover))
               _evry_item_desel(s->sel_item);
             if (!s->sel_item || (s->sel_item != item_mouseover))
               {
                  s->sel_item = item_mouseover;
                  _evry_item_sel(s->sel_item);
               }
          }
     }

   evas_event_feed_mouse_move(popup->evas, ev->x - popup->x,
			      ev->y - popup->y, ev->timestamp, NULL);

   return 1;
}

static int
_evry_cb_mouse_wheel(void *data, int type, void *event)
{
   Ecore_Event_Mouse_Wheel *ev;

   ev = event;
   if (ev->event_window != input_window) return 1;

   ev_last_is_mouse = 0;

   if (ev->z < 0) /* up */
     {
	int i;

	for (i = ev->z; i < 0; i++) _evry_item_prev();
     }
   else if (ev->z > 0) /* down */
     {
	int i;

	for (i = ev->z; i > 0; i--) _evry_item_next();
     }
   return 1;
}

static void
_evry_cb_item_mouse_in(void *data, Evas *evas, Evas_Object *obj,
		       void *event_info)
{
   Evry_State *s =cur_state;

   if (!ev_last_is_mouse) return;

   item_mouseover = data;

   if (s->sel_item) _evry_item_desel(s->sel_item);
   if (!(s->sel_item = data)) return;
   _evry_item_sel(s->sel_item);
}

static void
_evry_cb_item_mouse_out(void *data, Evas *evas, Evas_Object *obj,
			void *event_info)
{
   item_mouseover = NULL;
}

static void
_evry_backspace(void)
{
   int len, val, pos;
   Evry_State *s = cur_state;

   len = strlen(s->input);
   if (len > 0)
     {
	pos = evas_string_char_prev_get(s->input, len, &val);
	if ((pos < len) && (pos >= 0))
	  {
	     s->input[pos] = 0;
	     _evry_update();
	  }
     }
}

static void
_evry_update(void)
{
   Efreet_Desktop *desktop;
   Evas_Object *o;

   edje_object_part_text_set(o_main, "e.text.label", cur_state->input);

   if (update_timer) ecore_timer_del(update_timer);
   update_timer = ecore_timer_add(MATCH_LAG, _evry_update_timer, NULL);
}

static int
_evry_update_timer(void *data)
{
   _evry_matches_update(cur_state->cur_plugin);
   update_timer = NULL;
   return 0;
}

static void
_evry_clear(void)
{
   if (cur_state->input[0] != 0)
     {
	cur_state->input[0] = 0;
	_evry_update();
     }
}

static void
_evry_plugin_action(int finished)
{
   Evry_State *s = cur_state;

   if (s->cur_plugin == action_selector)
     {
	/* set cur_action and start plugins for second parameter (if required)*/
	if (s->sel_item) _evry_plug_act_select_action(s->cur_plugin, s->sel_item, s->input);

	finished = 0;
     }
   else if (s->cur_action)
     {
	/* */
	if (s->sel_item && (!strcmp(s->cur_plugin->type_out, s->request_type)))
	  {
	     s->cur_action->thing2 = s->sel_item;
	     s->cur_action->action();
	  }
     }
   else if (s->cur_plugin && (s->sel_item || s->input))
     {
	int ret = EVRY_ACTION_OTHER;

	if (s->cur_plugin->action)
	  {
	     ret = s->cur_plugin->action(s->cur_plugin, s->sel_item, s->input);
	  }
	if (ret == EVRY_ACTION_OTHER)
	  {
	     _evry_push_state();
 	     finished = 0;
	  }
	else if (ret == EVRY_ACTION_CONTINUE)
	  finished = 0;
     }
   else if (s->initial)
     e_exec(popup->zone, NULL, s->input, NULL, NULL /* "everything" */);

   if (finished)
     evry_hide();
}

static void
_evry_show_items(Evry_Plugin *p)
{
   Evry_Item *it;
   Eina_List *l;
   int mw, mh, h;
   Evas_Object *o;
   Evry_State *s = cur_state;

   if (s->cur_plugin)
     {
	if (s->cur_plugin != p)
	  {
	     edje_object_signal_emit(s->cur_plugin->tab, "e,state,unselected", "e");
	     edje_object_signal_emit(p->tab, "e,state,selected", "e");
	  }
     }
   else
     edje_object_signal_emit(p->tab, "e,state,selected", "e");

   /* XXX move this to push/pop as plugin could be the same for
      different states!*/
   if (s->cur_plugin != p)
     s->sel_item = NULL;

   if (p->realize_items) p->realize_items(p, popup->evas);
   
   s->cur_plugin = p;
   s->cur_items = p->items;

   if (scroll_timer)
     {
	ecore_timer_del(scroll_timer);
	scroll_timer = NULL;
     }
   if (scroll_animator)
     {
	ecore_animator_del(scroll_animator);
	scroll_animator = NULL;
     }

   scroll_align = 0;
   
   evas_event_freeze(popup->evas);
   e_box_freeze(o_list);

   EINA_LIST_FOREACH(p->items, l, it)
     {
	o = edje_object_add(popup->evas);
	it->o_bg = o;
	e_theme_edje_object_set(o, "base/theme/everything",
				"e/widgets/everything/item");

	edje_object_part_text_set(o, "e.text.title", it->label);

	evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_IN,
				       _evry_cb_item_mouse_in, it);
	evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_OUT,
				       _evry_cb_item_mouse_out, it);
	evas_object_show(o);

	if (!it->o_icon) p->icon_get(p, it, popup->evas);

	if (edje_object_part_exists(o, "e.swallow.icons") && it->o_icon)
	  {
	     edje_object_part_swallow(o, "e.swallow.icons", it->o_icon);
	     evas_object_show(it->o_icon);
	  }
	edje_object_size_min_calc(o, &mw, &mh);
	e_box_pack_end(o_list, o);
	e_box_pack_options_set(o, 1, 1, 1, 0, 0.5, 0.5, mw, mh, 9999, mh);
     }
   e_box_thaw(o_list);

   e_box_min_size_get(o_list, NULL, &mh);
   evas_object_geometry_get(o_list, NULL, NULL, NULL, &h);
   if (mh <= h)
     e_box_align_set(o_list, 0.5, 0.0);
   else
     e_box_align_set(o_list, 0.5, 1.0);

   /* TODO add option */

   if (!s->initial && !s->sel_item && p->items)
     {
   	s->sel_item = p->items->data;
   	_evry_item_sel(s->sel_item);
   	_evry_scroll_to(0);
     }

   evas_event_thaw(popup->evas);

   _evry_tab_scroll_to(p);
}

static void
_evry_matches_update(Evry_Plugin *cur_plugin)
{
   Evry_Plugin *p;
   Eina_List *l;
   int items;
   Evry_State *s = cur_state;

   _evry_list_clear();
   eina_list_free(s->cur_plugins);
   s->cur_plugins = NULL;
   s->sel_item = NULL;

   EINA_LIST_FOREACH(s->plugins, l, p)
     {
	/* input matches trigger? */
	if (p->trigger)
	  {
	     const char *trigger = p->trigger;
	     
	     if ((strlen(s->input) < strlen(trigger)) ||
		 (strncmp(s->input, trigger, strlen(trigger))))
	       continue;
	  }
   
	if (strlen(s->input) == 0)
	  items = !p->need_query ? p->fetch(p, NULL) : 0;
	else
	  items = p->fetch(p, s->input);

	if (!s->initial || (items && eina_list_count(p->items) > 0))
	  s->cur_plugins = eina_list_append(s->cur_plugins, p);
     }

   /* remove tabs for not active plugins */
   EINA_LIST_FOREACH(evry_conf->plugins, l, p)
     {
	if (!p->config->enabled) continue;
	
	if (p->tab && !eina_list_data_find(s->cur_plugins, p))
	  {
	     evas_object_del(p->tab);
	     p->tab = NULL;
	  }
     }
   /* show/update tabs of active plugins */
   EINA_LIST_FOREACH(s->cur_plugins, l, p)
     _evry_plugin_selector_show(p);

   if (s->cur_plugins)
     {
	if (cur_plugin && eina_list_data_find(s->cur_plugins, cur_plugin))
	  _evry_show_items(cur_plugin);
	else
	  _evry_show_items(s->cur_plugins->data);
     }
   else
     {
	s->cur_plugin = NULL;
	s->cur_items = NULL;
     }
}

static void
_evry_list_clear(void)
{
   Evry_State *s = cur_state;
   Evry_Item *it;
   Eina_List *l;

   if (s && s->cur_items)
     {
	evas_event_freeze(popup->evas);
	e_box_freeze(o_list);
	EINA_LIST_FOREACH(s->cur_items, l, it)
	  {
	     evas_object_del(it->o_bg);
	     if (it->o_icon) evas_object_del(it->o_icon);
	     it->o_icon = NULL;
	  }
	e_box_thaw(o_list);
	evas_event_thaw(popup->evas);
     }
}

static void
_evry_scroll_to(int i)
{
   int n, h, mh;

   n = eina_list_count(cur_state->cur_plugin->items);

   e_box_min_size_get(o_list, NULL, &mh);
   evas_object_geometry_get(o_list, NULL, NULL, NULL, &h);

   if (mh <= h) return;

   if (n > 1)
     {
	scroll_align_to = (double)i / (double)(n - 1);
	if (evry_conf->scroll_animate)
	  {
	     if (!scroll_timer)
	       scroll_timer = ecore_timer_add(0.01, _evry_scroll_timer, NULL);
	     if (!scroll_animator)
	       scroll_animator = ecore_animator_add(_evry_animator, NULL);
	  }
	else
	  {
	     scroll_align = scroll_align_to;
	     e_box_align_set(o_list, 0.5, 1.0 - scroll_align);
	  }
     }
   else
     e_box_align_set(o_list, 0.5, 1.0);
}

static void
_evry_tab_scroll_to(Evry_Plugin *p)
{
   int n, w, mw, i;
   double align;

   Eina_List *l;

   for(i = 0, l = cur_state->cur_plugins; l; l = l->next, i++)
     if (l->data == p) break;

   n = eina_list_count(cur_state->cur_plugins);

   e_box_min_size_get(o_selector, &mw, NULL);
   evas_object_geometry_get(o_selector, NULL, NULL, &w, NULL);

   if (mw <= w)
     {
	e_box_align_set(o_selector, 0.0, 0.5);
	return;
     }

   if (n > 1)
     {
	align = (double)i / (double)(n - 1);
	/* if (evry_conf->scroll_animate)
	 *   {
	 *      if (!scroll_timer)
	 *        scroll_timer = ecore_timer_add(0.01, _evry_scroll_timer, NULL);
	 *      if (!scroll_animator)
	 *        scroll_animator = ecore_animator_add(_evry_animator, NULL);
	 *   }
	 * else */
	{
	   e_box_align_set(o_selector, 1.0 - align, 0.5);
	}
     }
   else
     e_box_align_set(o_selector, 1.0, 0.5);
}

static void
_evry_item_desel(Evry_Item *it)
{
   edje_object_signal_emit(it->o_bg, "e,state,unselected", "e");
   if (it->o_icon)
     edje_object_signal_emit(it->o_icon, "e,state,unselected", "e");
}

static void
_evry_item_sel(Evry_Item *it)
{
   edje_object_signal_emit(it->o_bg, "e,state,selected", "e");
   if (it->o_icon)
     edje_object_signal_emit(it->o_icon, "e,state,selected", "e");
}

static void
_evry_item_next(void)
{
   Eina_List *l;
   int i;
   Evry_State *s = cur_state;

   s->plugin_auto_selected = 0;

   if (s->sel_item)
     {
	for (i = 0, l = s->cur_plugin->items; l; l = l->next, i++)
	  {
	     if (l->data == s->sel_item)
	       {
		  if (l->next)
		    {
		       _evry_item_desel(s->sel_item);
		       s->sel_item = l->next->data;
		       _evry_item_sel(s->sel_item);
		       _evry_scroll_to(i + 1);
		    }
		  break;
	       }
	  }
     }
   else if (s->cur_plugin && s->cur_plugin->items)
     {
	s->sel_item = s->cur_plugin->items->data;
	_evry_item_sel(s->sel_item);
	_evry_scroll_to(0);
     }

   /* if (s->sel_item)
    *   edje_object_part_text_set(o_main, "e.text.label", s->sel_item->label); */
}

static void
_evry_item_prev(void)
{
   Eina_List *l;
   int i;
   Evry_State *s = cur_state;

   s->plugin_auto_selected = 0;

   if (s->sel_item)
     {
	_evry_item_desel(s->sel_item);

	for (i = 0, l = s->cur_plugin->items; l; l = l->next, i++)
	  {
	     if (l->data == s->sel_item)
	       {
		  if (l->prev)
		    {
		       s->sel_item = l->prev->data;
		       _evry_item_sel(s->sel_item);
		       _evry_scroll_to(i - 1);
		    }
		  else
		    s->sel_item = NULL;
		  break;
	       }
	  }
     }
   /* if (s->sel_item)
    *   edje_object_part_text_set(o_main, "e.text.label", s->sel_item->label);
    * else
    *   edje_object_part_text_set(o_main, "e.text.label", input); */
}

static void
_evry_plugin_next(void)
{
   Eina_List *l;
   Evry_State *s = cur_state;

   if (!s->cur_plugin) return;

   l = eina_list_data_find_list(s->cur_plugins, s->cur_plugin);

   if (l && l->next)
     {
	s->plugin_auto_selected = 0;
	_evry_list_clear();
	_evry_show_items(l->next->data);
     }
   else if (s->cur_plugin != s->cur_plugins->data)
     {
	s->plugin_auto_selected = 0;
	_evry_list_clear();
	_evry_show_items(s->cur_plugins->data);
     }
}

static void
_evry_plugin_prev(void)
{
   Eina_List *l;
   Evry_Plugin *plugin;
   Evry_State *s = cur_state;

   if (!s->cur_plugin) return;

   l = eina_list_data_find_list(s->cur_plugins, s->cur_plugin);

   if (l && l->prev)
     {
	s->plugin_auto_selected = 0;
	_evry_list_clear();
	_evry_show_items(l->prev->data);
     }
   else
     {
	l = eina_list_last(s->cur_plugins);

	if (s->cur_plugin != l->data)
	  {
	     s->plugin_auto_selected = 0;
	     _evry_list_clear();
	     _evry_show_items(l->data);
	  }
     }
}

static int
_evry_scroll_timer(void *data)
{
   if (scroll_animator)
     {
	double spd;
	spd = evry_conf->scroll_speed;
	scroll_align = (scroll_align * (1.0 - spd)) + (scroll_align_to * spd);
	return 1;
     }
   scroll_timer = NULL;
   return 0;
}

static int
_evry_animator(void *data)
{
   double da;
   Eina_Bool scroll_to = 1;

   da = scroll_align - scroll_align_to;
   if (da < 0.0) da = -da;
   if (da < 0.01)
     {
	scroll_align = scroll_align_to;
	scroll_to = 0;
     }
   e_box_align_set(o_list, 0.5, 1.0 - scroll_align);
   if (scroll_to) return 1;
   scroll_animator = NULL;
   return 0;
}

static void
_evry_plugin_selector_show(Evry_Plugin *p)
{
   Evas_Object *o;
   Evas_Coord mw = 0, mh = 0;
   char buf[64];
   Eina_List *l;
   Evry_State *s = cur_state;

   e_box_freeze(o_selector);

   if (p->tab)
     {
	o = p->tab;
	e_box_unpack(p->tab);
     }
   else
     {
	o = edje_object_add(popup->evas);
	/* TODO move this to everything theme group !*/
	e_theme_edje_object_set(o, "base/theme/widgets",
				"e/widgets/toolbar/item");
     }

   snprintf(buf, 64, "%s (%d)", p->name, eina_list_count(p->items));

   edje_object_part_text_set(o, "e.text.label", buf);

   edje_object_size_min_calc(o, &mw, &mh);

   l = eina_list_data_find_list(s->cur_plugins, p);
   if (l && l->prev)
     {
	Evry_Plugin *p2 = l->prev->data;
	e_box_pack_after(o_selector, o, p2->tab);
     }
   else
     e_box_pack_end(o_selector, o);

   evas_object_show(o);
   e_box_pack_options_set(o, 1, 1, 0, 0, 0.5, 0.5, mw, mh, 9999, 9999);
   e_box_thaw(o_selector);
   p->tab = o;
}

static int
_evry_plug_act_select_init(void)
{
   Evry_Plugin *p = E_NEW(Evry_Plugin, 1);
   p->name = "Select Action";
   p->type_in  = "ANY";
   p->type_out = "NONE";
   p->cleanup  = &_evry_plug_act_select_cleanup;
   p->fetch    = &_evry_plug_act_select_fetch;
   p->action   = &_evry_plug_act_select_action;
   p->icon_get = &_evry_plug_act_select_item_icon_get;
   action_selector = p;
}

static int
_evry_plug_act_select_actions_set(void)
{
   Evry_Action *act;
   Eina_List *l;
   Evry_State *s = cur_state;
   Evry_Plugin *p = action_selector;

   _evry_plug_act_select_cleanup(p);

   const char *type = s->cur_plugin->type_out;

   EINA_LIST_FOREACH(actions, l, act)
     {
	if (strstr(act->type_in1, type))
	  {
	     act->thing1 = s->sel_item;
	     s->cur_actions = eina_list_append(s->cur_actions, act);
	  }
     }

   if (s->cur_actions) return 1;

   return 0;
}

static int
_evry_plug_act_select_fetch(Evry_Plugin *p, const char *input)
{
   Evry_Action *act;
   Eina_List *l;

   if (p->items) return 1;
   
   EINA_LIST_FOREACH(actions, l, act)
     {
	Evry_Item *it;

	it = E_NEW(Evry_Item, 1);
	it->label = eina_stringshare_add(act->name);
	it->data[0] = act;

	p->items = eina_list_append(p->items, it);
     }

   if (p->items) return 1;

   return 0;
}

static int
_evry_plug_act_select_action(Evry_Plugin *p, Evry_Item *it, const char *input)
{
   Evry_Action *act = it->data[0];
   cur_state->cur_action = act;

   cur_state->request_type = act->type_in2;

   return _evry_push_state();
}

static void
_evry_plug_act_select_cleanup(Evry_Plugin *p)
{
   Evry_Item *it;

   EINA_LIST_FREE(p->items, it)
     {
	eina_stringshare_del(it->label);
	E_FREE(it);
     }

   eina_list_free(cur_state->cur_actions);
   cur_state->cur_actions = NULL;

   p->items = NULL;

   if (p->tab)
     {
	evas_object_del(p->tab);
	p->tab = NULL;
     }
}

static void
_evry_plug_act_select_item_icon_get(Evry_Plugin *p, Evry_Item *it, Evas *e)
{
   Evry_Action *act = it->data[0];

   /* if (act->icon_get)
    *   it->o_icon = act->icon_get(act, e); */
}

