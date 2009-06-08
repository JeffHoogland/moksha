#include "e.h"
#include "e_mod_main.h"
#include "evry.h"

#define WIDTH 400
#define HEIGHT 350
#define INPUTLEN 40
#define MATCH_LAG 0.33

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
static void _evry_matches_clear(void);
static void _evry_list_clear(void);
static void _evry_show_candidates(Evry_Plugin *plugin);
static int  _evry_update_timer(void *data);
static void _evry_matches_update(void);
static void _evry_clear(void);
static void _evry_item_next(void);
static void _evry_item_prev(void);
static void _evry_plugin_next(void);
static void _evry_plugin_prev(void);
static void _evry_scroll_to(int i);
static void _evry_item_desel(Evry_Item *it);
static void _evry_item_sel(Evry_Item *it);
static void _evry_item_remove(Evry_Item *it);
static void _evry_action(int finished);
static void _evry_cb_plugin_sel(void *data1, void *data2);

/* local subsystem globals */
static E_Popup     *popup = NULL;
static Ecore_X_Window input_window = 0;
static Evas_Object *o_list = NULL;
static Evas_Object *o_main = NULL;
static Evas_Object *icon_object = NULL;
static Evas_Object *o_toolbar = NULL;
static char        *cmd_buf = NULL;
static Eina_List   *handlers = NULL;
static Ecore_Timer *update_timer = NULL;
static Eina_List   *plugins = NULL;
static int          plugin_count;
static Evry_Plugin *cur_source;
static Eina_List   *cur_sources = NULL;

static Evry_Item   *item_selected = NULL;
static Evry_Item   *item_mouseover = NULL;

static int ev_last_is_mouse;
/* static Ecore_Animator *animator = NULL; */
static double       scroll_align_to;
static double       scroll_align;



/* externally accessible functions */
EAPI int
evry_init(void)
{
   return 1;
}

EAPI int
evry_shutdown(void)
{
   evry_hide();
   return 1;
}

EAPI void
evry_plugin_add(Evry_Plugin *plugin)
{
   plugins = eina_list_append(plugins, plugin);
   /* TODO sorting, initialization, etc */
}

EAPI void
evry_plugin_remove(Evry_Plugin *plugin)
{
   plugins = eina_list_remove(plugins, plugin);
   /* cleanup */
}

EAPI int
evry_show(E_Zone *zone)
{
   Evas_Object *o;
   int x, y, w, h;

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

   w = WIDTH;
   h = HEIGHT;
   x = zone->x + (zone->w / 2) - (w / 2);
   y = zone->y + (zone->h / 2) - (h / 2);

   popup = e_popup_new(zone, x, y, w, h);
   if (!popup) return 0;

   cmd_buf = malloc(INPUTLEN);
   if (!cmd_buf)
     {
	e_object_del(E_OBJECT(popup));
	return 0;
     }

   ecore_x_netwm_window_type_set(popup->evas_win, ECORE_X_WINDOW_TYPE_UTILITY);

   cmd_buf[0] = 0;
   
   e_popup_layer_set(popup, 255);
   evas_event_freeze(popup->evas);
   evas_event_feed_mouse_in(popup->evas, ecore_x_current_time_get(), NULL);
   evas_event_feed_mouse_move(popup->evas, -1000000, -1000000, ecore_x_current_time_get(), NULL);
   o = edje_object_add(popup->evas);
   o_main = o;
   e_theme_edje_object_set(o, "base/theme/everything",
			   "e/widgets/everything/main");
   edje_object_part_text_set(o, "e.text.label", cmd_buf);

   o = e_box_add(popup->evas);
   o_list = o;
   e_box_orientation_set(o, 0);
   e_box_homogenous_set(o, 1);
   edje_object_part_swallow(o_main, "e.swallow.list", o);
   evas_object_show(o);

   o = o_main;
   evas_object_move(o, 0, 0);
   evas_object_resize(o, w, h);
   evas_object_show(o);
   e_popup_edje_bg_object_set(popup, o);

   o = e_widget_toolbar_add(popup->evas, 48 * e_scale, 48 * e_scale);
   e_widget_toolbar_scrollable_set(o, 0);
   edje_object_part_swallow(o_main, "e.swallow.bar", o);
   evas_object_show(o);
   o_toolbar = o;

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

   _evry_matches_update();

   ev_last_is_mouse = 0;
   item_mouseover = NULL;
   item_selected = NULL;
   
   e_popup_show(popup);
   return 1;
}

EAPI void
evry_hide(void)
{
   Ecore_Event *ev;
   char *str;
   Evry_Plugin *plugin;
   Eina_List *l;

   if (!popup) return;

   if (update_timer)
     {
	ecore_timer_del(update_timer);
	update_timer = NULL;
     }
   
   evas_event_freeze(popup->evas);
   _evry_matches_clear();
   e_popup_hide(popup);

   e_box_freeze(o_list);
   EINA_LIST_FOREACH(plugins, l, plugin)
     {
	plugin->cleanup();
     }
   e_box_thaw(o_list);

   evas_object_del(o_list);
   o_list = NULL;

   evas_object_del(o_toolbar);
   o_toolbar = NULL;

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
   free(cmd_buf);
   cmd_buf = NULL;

   cur_source = NULL;
   item_selected = NULL;
   item_mouseover = NULL;
}

/* local subsystem functions */
static int
_evry_cb_key_down(void *data, int type, void *event)
{
   Ecore_Event_Key *ev;

   ev_last_is_mouse = 0;

   ev = event;
   if (ev->event_window != input_window) return 1;

   if      (!strcmp(ev->key, "Up"))
     _evry_item_prev();
   else if (!strcmp(ev->key, "Down"))
     _evry_item_next();
   else if (!strcmp(ev->key, "Right"))
     _evry_plugin_next();
   else if (!strcmp(ev->key, "Left"))
     _evry_plugin_prev();
   else if (!strcmp(ev->key, "Return") &&
	    (ev->modifiers & ECORE_EVENT_MODIFIER_CTRL))
     _evry_action(0);
   else if (!strcmp(ev->key, "Return"))
     _evry_action(1);
   /* else if (!strcmp(ev->key, "Tab"))
    *   _evry_complete(); */
   else if (!strcmp(ev->key, "u") &&
	    (ev->modifiers & ECORE_EVENT_MODIFIER_CTRL))
     _evry_clear();
   else  if (!strcmp(ev->key, "Escape"))
     evry_hide();
   else if (!strcmp(ev->key, "BackSpace"))
     _evry_backspace();
   else if (!strcmp(ev->key, "Delete"))
     _evry_backspace();
   else
     {
   	if (ev->compose)
   	  {
   	     if ((strlen(cmd_buf) < (INPUTLEN - strlen(ev->compose))))
   	       {
   		  strcat(cmd_buf, ev->compose);
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
   
   ev = event;
   if (ev->event_window != input_window) return 1;

   if (item_mouseover)
     {
	if (item_selected != item_mouseover)
	  {
	     if (item_selected) _evry_item_desel(item_selected);
	     item_selected = item_mouseover;
	     _evry_item_sel(item_selected); 
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
	  _evry_action(1);
	else if (ev->buttons == 3) 
	  _evry_action(0);
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

   ev = event;
   if (ev->event_window != input_window) return 1;

   if (!ev_last_is_mouse)
     {
        ev_last_is_mouse = 1;
        if (item_mouseover)
          {
             if (item_selected && (item_selected != item_mouseover))
               _evry_item_desel(item_selected);
             if (!item_selected || (item_selected != item_mouseover))
               {
                  item_selected = item_mouseover;
                  _evry_item_sel(item_selected); 
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
   item_mouseover = data;
   if (!ev_last_is_mouse) return;

   if (item_selected) _evry_item_desel(item_selected);
   if (!(item_selected = data)) return;
   _evry_item_sel(item_selected);
}

static void 
_evry_cb_item_mouse_out(void *data, Evas *evas, Evas_Object *obj, 
			void *event_info)
{
   item_mouseover = NULL;
}


static void
_evry_cb_plugin_sel(void *data1, void *data2)
{
   if (cur_source == data1) return;
   
   _evry_show_candidates(data1);
}

static void
_evry_backspace(void)
{
   int len, val, pos;

   len = strlen(cmd_buf);
   if (len > 0)
     {
	pos = evas_string_char_prev_get(cmd_buf, len, &val);
	if ((pos < len) && (pos >= 0))
	  {
	     cmd_buf[pos] = 0;
	     _evry_update();
	  }
     }
}

static void
_evry_update(void)
{
   Efreet_Desktop *desktop;
   Evas_Object *o;

   edje_object_part_text_set(o_main, "e.text.label", cmd_buf);

   if (icon_object) evas_object_del(icon_object);
   icon_object = NULL;

   if (update_timer) ecore_timer_del(update_timer);
   update_timer = ecore_timer_add(MATCH_LAG, _evry_update_timer, NULL);
}

static void
_evry_action(int finished)
{
   if (cur_source && item_selected)
     {
	cur_source->action(item_selected);
     }
   else
     e_exec(popup->zone, NULL, cmd_buf, NULL, "everything");

   if (finished)
     evry_hide();
}

static void
_evry_clear(void)
{
   if (cmd_buf[0] != 0)
     {
	cmd_buf[0] = 0;
	_evry_update();
	if (!update_timer)
	  update_timer = ecore_timer_add(MATCH_LAG, _evry_update_timer, NULL);
     }
}

static void
_evry_show_candidates(Evry_Plugin *plugin)
{
   Evry_Item *it;
   Eina_List *l;
   int mw, mh, h;
   Evas_Object *o;
   int i = 0;

   _evry_list_clear();
   cur_source = plugin;
   
   e_box_freeze(o_list);

   EINA_LIST_FOREACH(cur_source->candidates, l, it)
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

	cur_source->icon_get(it, popup->evas);
	if (edje_object_part_exists(o, "e.swallow.icons") && it->o_icon)
	  {
	     edje_object_part_swallow(o, "e.swallow.icons", it->o_icon);
	     evas_object_show(it->o_icon);
	  }
	edje_object_size_min_calc(o, &mw, &mh);
	e_box_pack_end(o_list, o);
	e_box_pack_options_set(o,
			       1, 1, /* fill */
			       1, 0, /* expand */
			       0.5, 0.5, /* align */
			       mw, mh, /* min */
			       9999, mh /* max */
			       );
     }
   e_box_thaw(o_list);

   e_box_min_size_get(o_list, NULL, &mh);
   evas_object_geometry_get(o_list, NULL, NULL, NULL, &h);
   if (mh <= h)
     e_box_align_set(o_list, 0.5, 0.0);
   else
     e_box_align_set(o_list, 0.5, 1.0);

   EINA_LIST_FOREACH(cur_sources, l, plugin)
     if (plugin == cur_source)
       break;
     else i++;

   e_widget_toolbar_item_select(o_toolbar, i);
   
   evas_event_thaw(popup->evas);
}

static void
_evry_matches_update()
{
   Evry_Plugin *plugin;
   Eina_List *l;
   char buf[64];
   int candidates;

   _evry_matches_clear();
   plugin_count = 0;
   eina_list_free(cur_sources);
   cur_sources = NULL;
   
   EINA_LIST_FOREACH(plugins, l, plugin)
     {
	if (strlen(cmd_buf) == 0)
	  {
	     candidates = !plugin->need_query ? plugin->fetch(NULL) : 0;
	  }
	else
	  {
	     candidates = plugin->fetch(cmd_buf);
	  }
	if (candidates)
	  {
	     snprintf(buf, 64, "%s (%d)", plugin->name,
		      eina_list_count(plugin->candidates));

	     e_widget_toolbar_item_append(o_toolbar,
					  NULL, buf,
					  _evry_cb_plugin_sel,
					  plugin, NULL);

	     cur_sources = eina_list_append(cur_sources, plugin);
	     plugin_count++;
	  }
     }

   if (!cur_source && (plugin_count > 0))
     {
	_evry_show_candidates(cur_sources->data);
     }

   else if (cur_source)
     {
	_evry_show_candidates(cur_source);
     }
}

static void
_evry_item_remove(Evry_Item *it)
{
   evas_object_del(it->o_bg);
   if (it->o_icon) evas_object_del(it->o_icon);
   it->o_icon = NULL;
}

static void
_evry_matches_clear(void)
{
   Evry_Plugin *plugin;
   Eina_List *l;

   // FIXME add toolbar item remove method or use sth different
   evas_object_del(o_toolbar);
   Evas_Object *o = e_widget_toolbar_add(popup->evas,
					 48 * e_scale,
					 48 * e_scale);
   e_widget_toolbar_scrollable_set(o, 0);
   edje_object_part_swallow(o_main, "e.swallow.bar", o);
   evas_object_show(o);
   o_toolbar = o;

   _evry_list_clear();

   EINA_LIST_FOREACH(plugins, l, plugin)
     plugin->cleanup();
}

static void
_evry_list_clear(void)
{
   Evry_Item *it;
   Eina_List *l;

   if (cur_source)
     {
	evas_event_freeze(popup->evas);
	e_box_freeze(o_list);
	EINA_LIST_FOREACH(cur_source->candidates, l, it)
	  _evry_item_remove(it);
	e_box_thaw(o_list);
	evas_event_thaw(popup->evas);
     }

   item_selected = NULL;
}

static void
_evry_scroll_to(int i)
{
   int n, h, mh;

   n = eina_list_count(cur_source->candidates);

   e_box_min_size_get(o_list, NULL, &mh);
   evas_object_geometry_get(o_list, NULL, NULL, NULL, &h);

   if (mh <= h) return;

   if (n > 1)
     {
	scroll_align_to = (double)i / (double)(n - 1);
	/* if (e_config->everything_scroll_animate)
	 *   {
	 *      eap_scroll_to = 1;
	 *      if (!eap_scroll_timer)
	 *        eap_scroll_timer = ecore_timer_add(0.01, _evry_eap_scroll_timer, NULL);
	 *      if (!animator)
	 *        animator = ecore_animator_add(_evry_animator, NULL);
	 *   }
	 * else */
	{
	   scroll_align = scroll_align_to;
	   e_box_align_set(o_list, 0.5, 1.0 - scroll_align);
	}
     }
   else
     e_box_align_set(o_list, 0.5, 1.0);
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

static int
_evry_update_timer(void *data)
{
   _evry_matches_update();
   update_timer = NULL;
   return 0;
}

static void
_evry_item_next(void)
{
   Eina_List *l;
   int i;
   
   if (item_selected)
     {
	for (i = 0, l = cur_source->candidates; l; l = l->next, i++)
	  {
	     if (l->data == item_selected)
	       {
		  if (l->next)
		    {
		       _evry_item_desel(item_selected);
		       item_selected = l->next->data;
		       _evry_item_sel(item_selected);
		       _evry_scroll_to(i + 1);
		    }
		  break;
	       }
	  }
     }
   else if (cur_source->candidates)
     {
	item_selected = cur_source->candidates->data;
	_evry_item_sel(item_selected);
	_evry_scroll_to(0);
     }

   /* if (item_selected)
    *   edje_object_part_text_set(o_main, "e.text.label", item_selected->label); */
}

static void
_evry_item_prev(void)
{
   Eina_List *l;
   int i;
   
   if (item_selected)
     {
	_evry_item_desel(item_selected);

	for (i = 0, l = cur_source->candidates; l; l = l->next, i++)
	  {
	     if (l->data == item_selected)
	       {
		  if (l->prev)
		    {
		       item_selected = l->prev->data;
		       _evry_item_sel(item_selected);
		       _evry_scroll_to(i - 1);
		    }
		  else
		    item_selected = NULL;
		  break;
	       }
	  }
     }
   /* if (item_selected)
    *   edje_object_part_text_set(o_main, "e.text.label", item_selected->label);
    * else
    *   edje_object_part_text_set(o_main, "e.text.label", cmd_buf); */
}

static void
_evry_plugin_next(void)
{
   Eina_List *l;
   Evry_Plugin *plugin;

   if (!cur_source) return;

   l = eina_list_data_find_list(cur_sources, cur_source);

   if (l && l->next)
     {
	_evry_show_candidates(l->next->data);
     }
   else if (cur_source != cur_sources->data)
     {
	_evry_show_candidates(cur_sources->data);
     }
}


static void
_evry_plugin_prev(void)
{
   Eina_List *l;
   Evry_Plugin *plugin;

   if (!cur_source) return;

   l = eina_list_data_find_list(plugins, cur_source);

   if (l && l->prev)
     {
	_evry_show_candidates(l->prev->data);
     }
   else
     {	
	l = eina_list_last(cur_sources);
	
	if (cur_source != l->data)
	  {
	     _evry_show_candidates(l->data);
	  }
     }
}

