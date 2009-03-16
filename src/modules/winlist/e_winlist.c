/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_winlist.h"

/* local subsystem functions */
typedef struct _E_Winlist_Win E_Winlist_Win;

struct _E_Winlist_Win
{
   Evas_Object *bg_object;
   Evas_Object *icon_object;
   E_Border *border;
   unsigned char was_iconified : 1;
   unsigned char was_shaded : 1;
};

static void _e_winlist_size_adjust(void);
static void _e_winlist_border_add(E_Border *bd, E_Zone *zone, E_Desk *desk);
static void _e_winlist_border_del(E_Border *bd);
static void _e_winlist_activate_nth(int n);
static void _e_winlist_activate(void);
static void _e_winlist_deactivate(void);
static void _e_winlist_show_active(void);
static int _e_winlist_cb_event_border_add(void *data, int type,  void *event);
static int _e_winlist_cb_event_border_remove(void *data, int type,  void *event);
static int _e_winlist_cb_key_down(void *data, int type, void *event);
static int _e_winlist_cb_key_up(void *data, int type, void *event);
static int _e_winlist_cb_mouse_down(void *data, int type, void *event);
static int _e_winlist_cb_mouse_up(void *data, int type, void *event);
static int _e_winlist_cb_mouse_wheel(void *data, int type, void *event);
static int _e_winlist_cb_mouse_move(void *data, int type, void *event);
static int _e_winlist_scroll_timer(void *data);
static int _e_winlist_warp_timer(void *data);
static int _e_winlist_animator(void *data);
#if 0
static void _e_winlist_cb_item_mouse_in(void *data, Evas *evas,
      Evas_Object *obj, void *event_info);
#endif

/* local subsystem globals */
static E_Popup *winlist = NULL;
static Evas_Object *bg_object = NULL;
static Evas_Object *list_object = NULL;
static Evas_Object *icon_object = NULL;
static Eina_List *wins = NULL;
static Eina_List *win_selected = NULL;
static E_Desk *last_desk = NULL;
static int last_pointer_x = 0;
static int last_pointer_y = 0;
static E_Border *last_border = NULL;
static int hold_count = 0;
static int hold_mod = 0;
static Eina_List *handlers = NULL;
static Ecore_X_Window input_window = 0;
static int warp_to = 0;
static int warp_to_x = 0;
static int warp_to_y = 0;
static int warp_x = 0;
static int warp_y = 0;
static int scroll_to = 0;
static double scroll_align_to = 0.0;
static double scroll_align = 0.0;
static Ecore_Timer *warp_timer = NULL;
static Ecore_Timer *scroll_timer = NULL;
static Ecore_Timer *animator = NULL;

/* externally accessible functions */
EAPI int
e_winlist_init(void)
{
   return 1;
}

EAPI int
e_winlist_shutdown(void)
{
   e_winlist_hide();
   return 1;
}

EAPI int
e_winlist_show(E_Zone *zone)
{
   int x, y, w, h;
   Evas_Object *o;
   Eina_List *l;
   E_Desk *desk;
   
   E_OBJECT_CHECK_RETURN(zone, 0);
   E_OBJECT_TYPE_CHECK_RETURN(zone, E_ZONE_TYPE, 0);
   
   if (winlist) return 0;

   input_window = ecore_x_window_input_new(zone->container->win, 0, 0, 1, 1);
   ecore_x_window_show(input_window);
   if (!e_grabinput_get(input_window, 0, input_window))
     {
	ecore_x_window_del(input_window);
	input_window = 0;
	return 0;
     }

   w = (double)zone->w * e_config->winlist_pos_size_w;
   if (w > e_config->winlist_pos_max_w) w = e_config->winlist_pos_max_w;
   else if (w < e_config->winlist_pos_min_w) w = e_config->winlist_pos_min_w;
   if (w > zone->w) w = zone->w;
   x = (double)(zone->w - w) * e_config->winlist_pos_align_x;
   
   h = (double)zone->h * e_config->winlist_pos_size_h;
   if (h > e_config->winlist_pos_max_h) h = e_config->winlist_pos_max_h;
   else if (h < e_config->winlist_pos_min_h) h = e_config->winlist_pos_min_h;
   if (h > zone->h) h = zone->h;
   y = (double)(zone->h - h) * e_config->winlist_pos_align_y;
   
   winlist = e_popup_new(zone, x, y, w, h); 
   if (!winlist) return 0;
   e_border_focus_track_freeze();
   
   evas_event_feed_mouse_in(winlist->evas, ecore_x_current_time_get(), NULL);
   evas_event_feed_mouse_move(winlist->evas, -1000000, -1000000, ecore_x_current_time_get(), NULL);
   
   e_popup_layer_set(winlist, 255);
   evas_event_freeze(winlist->evas);
   o = edje_object_add(winlist->evas);
   bg_object = o;
   e_theme_edje_object_set(o, "base/theme/winlist",
			   "e/widgets/winlist/main");
   evas_object_move(o, 0, 0);
   evas_object_resize(o, w, h);
   evas_object_show(o);
   e_popup_edje_bg_object_set(winlist, o);

   o = e_box_add(winlist->evas);
   list_object = o;
   e_box_align_set(o, 0.5, 0.0);
   e_box_orientation_set(o, 0);
   e_box_homogenous_set(o, 1);
   edje_object_part_swallow(bg_object, "e.swallow.list", o);
   edje_object_part_text_set(bg_object, "e.text.title", _("Select a window"));
   evas_object_show(o);

   desk = e_desk_current_get(winlist->zone);
   e_box_freeze(list_object);
   for (l = e_border_focus_stack_get(); l; l = l->next)
     {
	E_Border *bd;
	
	bd = l->data;
        
	_e_winlist_border_add(bd, winlist->zone, desk);
     }
   e_box_thaw(list_object);
   
   if (!wins)
     {
	e_winlist_hide();
	return 1;
     }

   if (e_config->winlist_list_show_other_desk_windows ||
       e_config->winlist_list_show_other_screen_windows)
     last_desk = e_desk_current_get(winlist->zone);
   if (e_config->winlist_warp_while_selecting)
     ecore_x_pointer_xy_get(winlist->zone->container->win,
                            &last_pointer_x, &last_pointer_y);
   last_border = e_border_focused_get();
   if (last_border)
     {
        if (!last_border->lock_focus_out)
	   e_border_focus_set(last_border, 0, 0);
        else
          last_border = NULL;
     }
   _e_winlist_activate_nth(1);
   evas_event_thaw(winlist->evas);
   _e_winlist_size_adjust();

   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (E_EVENT_BORDER_ADD, _e_winlist_cb_event_border_add, NULL));
   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (E_EVENT_BORDER_REMOVE, _e_winlist_cb_event_border_remove, NULL));
   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (ECORE_EVENT_KEY_DOWN, _e_winlist_cb_key_down, NULL));
   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (ECORE_EVENT_KEY_UP, _e_winlist_cb_key_up, NULL));
   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (ECORE_EVENT_MOUSE_BUTTON_DOWN, _e_winlist_cb_mouse_down, NULL));
   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (ECORE_EVENT_MOUSE_BUTTON_UP, _e_winlist_cb_mouse_up, NULL));
   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (ECORE_EVENT_MOUSE_WHEEL, _e_winlist_cb_mouse_wheel, NULL));
   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (ECORE_EVENT_MOUSE_MOVE, _e_winlist_cb_mouse_move, NULL));
   
   e_popup_show(winlist);
   return 1;
}

EAPI void
e_winlist_hide(void)
{
   E_Border *bd = NULL;
   E_Winlist_Win *ww;
   
   if (!winlist) return;
   
   if (win_selected)
     {
	ww = win_selected->data;
	bd = ww->border;
     }
   evas_event_freeze(winlist->evas);
   e_popup_hide(winlist);
   e_box_freeze(list_object);
   while (wins)
     {
	ww = wins->data;
	evas_object_del(ww->bg_object);
	if (ww->icon_object) evas_object_del(ww->icon_object);
	wins = eina_list_remove_list(wins, wins);
	if ((!bd) || (ww->border != bd))
	  e_object_unref(E_OBJECT(ww->border));	
	free(ww);
     }
   e_box_thaw(list_object);
   win_selected = NULL;
   if (icon_object)
     {
	evas_object_del(icon_object);
	icon_object = NULL;
     }
   evas_object_del(list_object);
   list_object = NULL;
   evas_object_del(bg_object);
   bg_object = NULL;
   evas_event_thaw(winlist->evas);
   e_object_del(E_OBJECT(winlist));
   e_border_focus_track_thaw();
   winlist = NULL;
   hold_count = 0;
   hold_mod = 0;
   while (handlers)
     {
	ecore_event_handler_del(handlers->data);
	handlers = eina_list_remove_list(handlers, handlers);
     }
   ecore_x_window_del(input_window);
   e_grabinput_release(input_window, input_window);
   input_window = 0;
   if (warp_timer)
     {
	ecore_timer_del(warp_timer);
	warp_timer = NULL;
     }
   if (scroll_timer)
     {
	ecore_timer_del(scroll_timer);
	scroll_timer = NULL;
     }
   if (animator)
     {
	ecore_animator_del(animator);
	animator = NULL;
     }
   if (bd)
     {
	if (bd->iconic)
	  {
	     if (!bd->lock_user_iconify)
	       e_border_uniconify(bd);
	  }
	if (bd->shaded)
	  {
	     if (!bd->lock_user_shade)
	       e_border_unshade(bd, bd->shade.dir);
	  }
	else if (bd->desk)
	  {
	     if (!bd->sticky) e_desk_show(bd->desk);
	  }
	if (!bd->lock_user_stacking)
	  e_border_raise(bd);
	if (!bd->lock_focus_out)
	  {
	     e_border_focus_set(bd, 1, 1);
	     e_border_focus_latest_set(bd);
	  }
	if ((e_config->focus_policy != E_FOCUS_CLICK) ||
	    (e_config->winlist_warp_at_end) ||
	    (e_config->winlist_warp_while_selecting))
	  ecore_x_pointer_warp(bd->zone->container->win,
			       warp_to_x, 
			       warp_to_y);
	e_object_unref(E_OBJECT(bd));
     }
}

EAPI void
e_winlist_next(void)
{
   if (!winlist) return;
   if (eina_list_count(wins) == 1)
     {
	if (!win_selected)
	  {
	     win_selected = wins;
	     _e_winlist_show_active();
	     _e_winlist_activate();
	  }
	return;
     }
   _e_winlist_deactivate();
   if (!win_selected)
     win_selected = wins;
   else
     win_selected = win_selected->next;
   if (!win_selected) win_selected = wins;
   _e_winlist_show_active();
   _e_winlist_activate();
}

EAPI void
e_winlist_prev(void)
{
   if (!winlist) return;
   if (eina_list_count(wins) == 1)
     {
	if (!win_selected)
	  {
	     win_selected = wins;
	     _e_winlist_show_active();
	     _e_winlist_activate();
	  }
	return;
     }
   _e_winlist_deactivate();
   if (!win_selected)
     win_selected = wins;
   else
     win_selected = win_selected->prev;
   if (!win_selected) win_selected = eina_list_last(wins);
   _e_winlist_show_active();
   _e_winlist_activate();
}

EAPI void
e_winlist_modifiers_set(int mod)
{
   if (!winlist) return;
   hold_mod = mod;
   hold_count = 0;
   if (hold_mod & ECORE_EVENT_MODIFIER_SHIFT) hold_count++;
   if (hold_mod & ECORE_EVENT_MODIFIER_CTRL) hold_count++;
   if (hold_mod & ECORE_EVENT_MODIFIER_ALT) hold_count++;
   if (hold_mod & ECORE_EVENT_MODIFIER_WIN) hold_count++;
}

/* local subsystem functions */
static void
_e_winlist_size_adjust(void)
{
   Evas_Coord mw, mh;
   E_Zone *zone;
   int x, y, w, h;   

   e_box_freeze(list_object);
   e_box_min_size_get(list_object, &mw, &mh);
   edje_extern_object_min_size_set(list_object, mw, mh);
   edje_object_part_swallow(bg_object, "e.swallow.list", list_object);
   edje_object_size_min_calc(bg_object, &mw, &mh);
   edje_extern_object_min_size_set(list_object, -1, -1);
   edje_object_part_swallow(bg_object, "e.swallow.list", list_object);
   e_box_thaw(list_object);
   
   zone = winlist->zone;
   w = (double)zone->w * e_config->winlist_pos_size_w;
   if (w < mw) w = mw;
   if (w > e_config->winlist_pos_max_w) w = e_config->winlist_pos_max_w;
   else if (w < e_config->winlist_pos_min_w) w = e_config->winlist_pos_min_w;
   if (w > zone->w) w = zone->w;
   x = (double)(zone->w - w) * e_config->winlist_pos_align_x;
   
   h = mh;
   if (h > e_config->winlist_pos_max_h) h = e_config->winlist_pos_max_h;
   else if (h < e_config->winlist_pos_min_h) h = e_config->winlist_pos_min_h;
   if (h > zone->h) h = zone->h;
   y = (double)(zone->h - h) * e_config->winlist_pos_align_y;
   
   evas_object_resize(bg_object, w, h);
   e_popup_move_resize(winlist, x, y, w, h);
}

static void
_e_winlist_border_add(E_Border *bd, E_Zone *zone, E_Desk *desk)
{
   E_Winlist_Win *ww;
   Evas_Coord mw, mh;
   Evas_Object *o;
	
   if ((!bd->client.icccm.accepts_focus) &&
       (!bd->client.icccm.take_focus)) return;
   if (bd->client.netwm.state.skip_taskbar) return;
   if (bd->user_skip_winlist) return;
   if (bd->iconic)
     {
	if (!e_config->winlist_list_show_iconified) return;
	if ((bd->zone != zone) &&
	    (!e_config->winlist_list_show_other_screen_iconified)) return;	
	if ((bd->desk != desk) &&
	    (!e_config->winlist_list_show_other_desk_iconified)) return;
     }
   else
     {
	if (bd->sticky)
	  {
	     if ((bd->zone != zone) &&
		 (!e_config->winlist_list_show_other_screen_windows)) return;
	  }
	else
	  {
	     if (bd->desk != desk)
	       {
		  if ((bd->zone) && (bd->zone != zone))
		    {
		       if (!e_config->winlist_list_show_other_screen_windows) return;
		    }   
		  else if (!e_config->winlist_list_show_other_desk_windows) return;
	       }
	  }
     }

   ww = calloc(1, sizeof(E_Winlist_Win));
   if (!ww) return;
   ww->border = bd;
   wins = eina_list_append(wins, ww);
   o = edje_object_add(winlist->evas);
   ww->bg_object = o;
   e_theme_edje_object_set(o, "base/theme/winlist",
			   "e/widgets/winlist/item");
   edje_object_part_text_set(o, "e.text.label", e_border_name_get(ww->border));
   evas_object_show(o);
   if (edje_object_part_exists(ww->bg_object, "e.swallow.icon"))
     {
	o = e_border_icon_add(bd, winlist->evas);
	ww->icon_object = o;
	edje_object_part_swallow(ww->bg_object, "e.swallow.icon", o);
	evas_object_show(o);
     }
   if (bd->shaded)
     {
	edje_object_signal_emit(ww->bg_object, "e,state,shaded", "e");
     }
   else if (bd->iconic)
     {
	edje_object_signal_emit(ww->bg_object, "e,state,iconified", "e");
     }
   else if (bd->desk != desk)
     {
	if (!((bd->sticky) && (bd->zone == zone)))
	  edje_object_signal_emit(ww->bg_object, "e,state,invisible", "e");
     }

   edje_object_size_min_calc(ww->bg_object, &mw, &mh);
   e_box_pack_end(list_object, ww->bg_object);
   e_box_pack_options_set(ww->bg_object, 
			  1, 1, /* fill */
			  1, 0, /* expand */
			  0.5, 0.5, /* align */
			  mw, mh, /* min */
			  9999, mh /* max */
			 );
   e_object_ref(E_OBJECT(ww->border));
}

static void
_e_winlist_border_del(E_Border *bd)
{
   Eina_List *l;
   
   if (bd == last_border) last_border = NULL;
   for (l = wins; l; l = l->next)
     {
	E_Winlist_Win *ww;
	
	ww = l->data;
	if (ww->border == bd)
	  {
             e_object_unref(E_OBJECT(ww->border));
	     if (l == win_selected)
	       {
		  win_selected = l->next;
		  if (!win_selected) win_selected = l->prev;
		  _e_winlist_show_active();
		  _e_winlist_activate();
	       }
	     evas_object_del(ww->bg_object);
	     if (ww->icon_object) evas_object_del(ww->icon_object);
	     free(ww);
	     wins = eina_list_remove_list(wins, l);
	     return;
	  }
     }
}

static void
_e_winlist_activate_nth(int n)
{
   Eina_List *l;
   int cnt;
   
   _e_winlist_deactivate();
   cnt = eina_list_count(wins);
   if (n >= cnt) n = cnt - 1;
   l = eina_list_nth_list(wins, n);
   if (l)
     {
	win_selected = l;
	_e_winlist_show_active();
	_e_winlist_activate();
     }
}

static void
_e_winlist_activate(void)
{
   E_Winlist_Win *ww;
   Evas_Object *o;
   int ok;
   
   if (!win_selected) return;
   ww = win_selected->data;
   edje_object_signal_emit(ww->bg_object, "e,state,selected", "e");
   if (ww->icon_object) edje_object_signal_emit(ww->icon_object, "e,state,selected", "e");
   ok = 0;

   if ((ww->border->iconic) &&
       (e_config->winlist_list_uncover_while_selecting))
     {
	if (!ww->border->lock_user_iconify)
	  e_border_uniconify(ww->border);
	ww->was_iconified = 1;
	ok = 1;
     }
   if ((!ww->border->sticky) &&
       (ww->border->desk != e_desk_current_get(winlist->zone)) &&
       (e_config->winlist_list_jump_desk_while_selecting))
     {
	if (ww->border->desk) e_desk_show(ww->border->desk);
	ok = 1;
     }
   if (((ww->border->shaded) ||
	((ww->border->changes.shaded) &&
	 (ww->border->shade.val != ww->border->shaded) &&
	 (ww->border->shade.val))) &&
       (ww->border->desk == e_desk_current_get(winlist->zone)) &&
       (e_config->winlist_list_uncover_while_selecting))
     {
	if (!ww->border->lock_user_shade)
	  e_border_unshade(ww->border, ww->border->shade.dir);
	ww->was_shaded = 1;
	ok = 1;
     }
   if ((!ww->border->iconic) &&
       ((ww->border->desk == e_desk_current_get(winlist->zone)) ||
	(ww->border->sticky)))
     ok = 1;
   if (ok)
     {
	if ((e_config->focus_policy != E_FOCUS_CLICK) ||
	    (e_config->winlist_warp_at_end) ||
	    (e_config->winlist_warp_while_selecting))
	  { 
	     warp_to_x = ww->border->x + (ww->border->w / 2); 
	     if (warp_to_x < (ww->border->zone->x + 1))
	       warp_to_x = ww->border->zone->x + ((ww->border->x + ww->border->w - ww->border->zone->x) / 2);
	     else if (warp_to_x >= (ww->border->zone->x + ww->border->zone->w - 1))
	       warp_to_x = (ww->border->zone->x + ww->border->zone->w + ww->border->x) / 2; 
     
	     warp_to_y = ww->border->y + (ww->border->h / 2);
	     if (warp_to_y < (ww->border->zone->y + 1))
	       warp_to_y = ww->border->zone->y + ((ww->border->y + ww->border->h - ww->border->zone->y) / 2);
	     else if (warp_to_y >= (ww->border->zone->y + ww->border->zone->h - 1))
	       warp_to_y = (ww->border->zone->y + ww->border->zone->h + ww->border->y) / 2; 
	  }
	if (e_config->winlist_warp_while_selecting)
	  {
	     ecore_x_pointer_xy_get(winlist->zone->container->win, &warp_x, &warp_y);
	     e_border_focus_latest_set(ww->border);
	     warp_to = 1;
	     if (!warp_timer)
	       warp_timer = ecore_timer_add(0.01, _e_winlist_warp_timer, NULL);
	     if (!animator)
	       animator = ecore_animator_add(_e_winlist_animator, NULL);
	  }
	else 
	  {
	     warp_to = 0;
	     if (warp_timer)
	       {
		  ecore_timer_del(warp_timer);
		  warp_timer = NULL;
	       }
	     if (animator) 
	       {
		  ecore_animator_del(animator);
		  animator = NULL;
	       }
	  }
	
	if ((!ww->border->lock_user_stacking) && (e_config->winlist_list_raise_while_selecting))
	  e_border_raise(ww->border);
	if ((!ww->border->lock_focus_out) && (e_config->winlist_list_focus_while_selecting))
	  e_border_focus_set(ww->border, 1, 1);
     }
   edje_object_part_text_set(bg_object, "e.text.label", e_border_name_get(ww->border));
   if (icon_object)
     {
	evas_object_del(icon_object);
	icon_object = NULL;
     }
   if (edje_object_part_exists(bg_object, "e.swallow.icon"))
     {
	o = e_border_icon_add(ww->border, winlist->evas);
	icon_object = o;
	edje_object_part_swallow(bg_object, "e.swallow.icon", o);
	evas_object_show(o);
     }
   
   edje_object_signal_emit(bg_object, "e,state,selected", "e");
}

static void
_e_winlist_deactivate(void)
{
   E_Winlist_Win *ww;

   if (!win_selected) return;
   ww = win_selected->data;
   if (ww->was_shaded)
     {
	if (!ww->border->lock_user_shade)
	  e_border_shade(ww->border, ww->border->shade.dir);
     }
   if (ww->was_iconified)
     {
	if (!ww->border->lock_user_iconify)
	  e_border_iconify(ww->border);
     }
   ww->was_shaded = 0;
   ww->was_iconified = 0;
   if (icon_object)
     {
	evas_object_del(icon_object);
	icon_object = NULL;
     }
   edje_object_part_text_set(bg_object, "e.text.label", "");
   edje_object_signal_emit(ww->bg_object, "e,state,unselected", "e");
   if (ww->icon_object) edje_object_signal_emit(ww->icon_object, "e,state,unselected", "e");
   if (!ww->border->lock_focus_in)
     e_border_focus_set(ww->border, 0, 0);
}

static void
_e_winlist_show_active(void)
{
   Eina_List *l;
   int i, n;
   
   if (!wins) return;
   for (i = 0, l = wins; l; l = l->next, i++)
     {
	if (l == win_selected) break;
     }
   n = eina_list_count(wins);
   if (n <= 1) return;
   scroll_align_to = (double)i / (double)(n - 1);
   if (e_config->winlist_scroll_animate)
     {
	scroll_to = 1;
	if (!scroll_timer)
	  scroll_timer = ecore_timer_add(0.01, _e_winlist_scroll_timer, NULL);
	if (!animator)
	  animator = ecore_animator_add(_e_winlist_animator, NULL);
     }
   else
     {
	scroll_align = scroll_align_to;
	e_box_align_set(list_object, 0.5, scroll_align);
     }
}

static void
_e_winlist_restore_desktop(void)
{
   if (last_desk &&
       (e_config->winlist_list_show_other_desk_windows ||
        e_config->winlist_list_show_other_screen_windows))
     e_desk_show(last_desk);
   if (e_config->winlist_warp_while_selecting)
     ecore_x_pointer_warp(winlist->zone->container->win,
                          last_pointer_x, last_pointer_y);
   _e_winlist_deactivate();
   win_selected = NULL;
   e_winlist_hide();
   if (last_border)
     {
        e_border_focus_set(last_border, 1, 1);
        last_border = NULL;
     }
}

static int
_e_winlist_cb_event_border_add(void *data, int type,  void *event)
{
   E_Event_Border_Add *ev;

   ev = event;
   _e_winlist_border_add(ev->border, winlist->zone,
			 e_desk_current_get(winlist->zone));
   _e_winlist_size_adjust();
   return 1;
}

static int
_e_winlist_cb_event_border_remove(void *data, int type,  void *event)
{
   E_Event_Border_Remove *ev;

   ev = event;
   _e_winlist_border_del(ev->border);
   _e_winlist_size_adjust();
   return 1;
}

static int
_e_winlist_cb_key_down(void *data, int type, void *event)
{
   Ecore_Event_Key *ev;
   
   ev = event;
   if (ev->window != input_window) return 1;
   if      (!strcmp(ev->key, "Up"))
     e_winlist_prev();
   else if (!strcmp(ev->key, "Down"))
     e_winlist_next();
   else if (!strcmp(ev->key, "Left"))
     e_winlist_prev();
   else if (!strcmp(ev->key, "Right"))
     e_winlist_next();
   else if (!strcmp(ev->key, "Return"))
     e_winlist_hide();
   else if (!strcmp(ev->key, "space"))
     e_winlist_hide();
   else if (!strcmp(ev->key, "Escape"))
     _e_winlist_restore_desktop();
   else if (!strcmp(ev->key, "1"))
     _e_winlist_activate_nth(0);
   else if (!strcmp(ev->key, "2"))
     _e_winlist_activate_nth(1);
   else if (!strcmp(ev->key, "3"))
     _e_winlist_activate_nth(2);
   else if (!strcmp(ev->key, "4"))
     _e_winlist_activate_nth(3);
   else if (!strcmp(ev->key, "5"))
     _e_winlist_activate_nth(4);
   else if (!strcmp(ev->key, "6"))
     _e_winlist_activate_nth(5);
   else if (!strcmp(ev->key, "7"))
     _e_winlist_activate_nth(6);
   else if (!strcmp(ev->key, "8"))
     _e_winlist_activate_nth(7);
   else if (!strcmp(ev->key, "9"))
     _e_winlist_activate_nth(8);
   else if (!strcmp(ev->key, "0"))
     _e_winlist_activate_nth(9);
   else
     {
	E_Action *act;
	Eina_List *l;
	E_Config_Binding_Key *bind;
	E_Binding_Modifier mod;

	for (l = e_config->key_bindings; l; l = l->next)
	  {
	     bind = l->data;

	     if (bind->action && strcmp(bind->action,"winlist")) continue;

	     mod = 0;

	     if (ev->modifiers & ECORE_EVENT_MODIFIER_SHIFT) mod |= E_BINDING_MODIFIER_SHIFT;
	     if (ev->modifiers & ECORE_EVENT_MODIFIER_CTRL) mod |= E_BINDING_MODIFIER_CTRL;
	     if (ev->modifiers & ECORE_EVENT_MODIFIER_ALT) mod |= E_BINDING_MODIFIER_ALT;
	     if (ev->modifiers & ECORE_EVENT_MODIFIER_WIN) mod |= E_BINDING_MODIFIER_WIN;

	     if (bind->key && (!strcmp(bind->key, ev->keyname)) &&
		 ((bind->modifiers == mod) || (bind->any_mod))) 
	       {	
		  act = e_action_find(bind->action);
		  
		  if(!act) continue;

		  if (act->func.go_key)
		    act->func.go_key(E_OBJECT(winlist->zone), bind->params, ev); 
		  else if (act->func.go)
		    act->func.go(E_OBJECT(winlist->zone), bind->params); 

	       }
	  }
     }
   return 1;
}

static int
_e_winlist_cb_key_up(void *data, int type, void *event)
{
   Ecore_Event_Key *ev;
   E_Action *act;
   Eina_List *l;
   E_Config_Binding_Key *bind;
   E_Binding_Modifier mod;
   
   ev = event;
   if (!winlist) return 1;
   if (hold_mod)
     {
	if      ((hold_mod & ECORE_EVENT_MODIFIER_SHIFT) && (!strcmp(ev->key, "Shift_L"))) hold_count--;
	else if ((hold_mod & ECORE_EVENT_MODIFIER_SHIFT) && (!strcmp(ev->key, "Shift_R"))) hold_count--;
	else if ((hold_mod & ECORE_EVENT_MODIFIER_CTRL) && (!strcmp(ev->key, "Control_L"))) hold_count--;
	else if ((hold_mod & ECORE_EVENT_MODIFIER_CTRL) && (!strcmp(ev->key, "Control_R"))) hold_count--;
	else if ((hold_mod & ECORE_EVENT_MODIFIER_ALT) && (!strcmp(ev->key, "Alt_L"))) hold_count--;
	else if ((hold_mod & ECORE_EVENT_MODIFIER_ALT) && (!strcmp(ev->key, "Alt_R"))) hold_count--;
	else if ((hold_mod & ECORE_EVENT_MODIFIER_ALT) && (!strcmp(ev->key, "Meta_L"))) hold_count--;
	else if ((hold_mod & ECORE_EVENT_MODIFIER_ALT) && (!strcmp(ev->key, "Meta_R"))) hold_count--;
	else if ((hold_mod & ECORE_EVENT_MODIFIER_ALT) && (!strcmp(ev->key, "Super_L"))) hold_count--;
	else if ((hold_mod & ECORE_EVENT_MODIFIER_ALT) && (!strcmp(ev->key, "Super_R"))) hold_count--;
	else if ((hold_mod & ECORE_EVENT_MODIFIER_WIN) && (!strcmp(ev->key, "Super_L"))) hold_count--;
	else if ((hold_mod & ECORE_EVENT_MODIFIER_WIN) && (!strcmp(ev->key, "Super_R"))) hold_count--;
	else if ((hold_mod & ECORE_EVENT_MODIFIER_WIN) && (!strcmp(ev->key, "Mode_switch"))) hold_count--;
	else if ((hold_mod & ECORE_EVENT_MODIFIER_WIN) && (!strcmp(ev->key, "Meta_L"))) hold_count--;
	else if ((hold_mod & ECORE_EVENT_MODIFIER_WIN) && (!strcmp(ev->key, "Meta_R"))) hold_count--;
	if (hold_count <= 0)
	  {
	     e_winlist_hide();
	     return 1;
	  }
     }

   for (l = e_config->key_bindings; l; l = l->next)
     {
	bind = l->data;

	if (bind->action && strcmp(bind->action,"winlist")) continue;

	mod = 0;

	if (ev->modifiers & ECORE_EVENT_MODIFIER_SHIFT) mod |= E_BINDING_MODIFIER_SHIFT;
	if (ev->modifiers & ECORE_EVENT_MODIFIER_CTRL) mod |= E_BINDING_MODIFIER_CTRL;
	if (ev->modifiers & ECORE_EVENT_MODIFIER_ALT) mod |= E_BINDING_MODIFIER_ALT;
	if (ev->modifiers & ECORE_EVENT_MODIFIER_WIN) mod |= E_BINDING_MODIFIER_WIN;

	if (bind->key && (!strcmp(bind->key, ev->keyname)) &&
	    ((bind->modifiers == mod) || (bind->any_mod))) 
	  {	
	     act = e_action_find(bind->action);

	     if(!act) continue;

	     if (act->func.end_key)
	       act->func.end_key(E_OBJECT(winlist->zone), bind->params, ev);
	     else if (act->func.end)
	       act->func.end(E_OBJECT(winlist->zone), bind->params);
	  }
     }

   return 1;
}

static int
_e_winlist_cb_mouse_down(void *data, int type, void *event)
{
   Ecore_Event_Mouse_Button *ev;
   
   ev = event;
   if (ev->window != input_window) return 1;
   e_bindings_mouse_down_event_handle(E_BINDING_CONTEXT_WINLIST,
				      E_OBJECT(winlist->zone), ev);
   return 1;
}

static int
_e_winlist_cb_mouse_up(void *data, int type, void *event)
{
   Ecore_Event_Mouse_Button *ev;
   
   ev = event;
   if (ev->window != input_window) return 1;
   e_bindings_mouse_up_event_handle(E_BINDING_CONTEXT_WINLIST,
				    E_OBJECT(winlist->zone), ev);
   return 1;
}

static int
_e_winlist_cb_mouse_wheel(void *data, int type, void *event)
{
   Ecore_Event_Mouse_Wheel *ev;
   
   ev = event;
   if (ev->window != input_window) return 1;
   e_bindings_wheel_event_handle(E_BINDING_CONTEXT_WINLIST,
				 E_OBJECT(winlist->zone), ev);
   if (ev->z < 0) /* up */
     {
	int i;
	
	for (i = ev->z; i < 0; i++) e_winlist_prev();
     }
   else if (ev->z > 0) /* down */
     {
	int i;
	
	for (i = ev->z; i > 0; i--) e_winlist_next();
     }
   return 1;
}

static int 
_e_winlist_cb_mouse_move(void *data, int type, void *event)
{
   Ecore_Event_Mouse_Move *ev;

   ev = event;
   if (ev->window != input_window) return 1;

   evas_event_feed_mouse_move(winlist->evas, ev->x - winlist->x +
	 winlist->zone->x, ev->y - winlist->y + winlist->zone->y, ev->timestamp, NULL);

   return 1;
}

static int
_e_winlist_scroll_timer(void *data)
{
   if (scroll_to)
     {
	double spd;

	spd = e_config->winlist_scroll_speed;
	scroll_align = (scroll_align * (1.0 - spd)) + (scroll_align_to * spd);
	return 1;
     }
   scroll_timer = NULL;
   return 0;
}

static int
_e_winlist_warp_timer(void *data)
{
   if (warp_to)
     {
	int x, y;
	double spd;
	
	spd = e_config->winlist_warp_speed;
	x = warp_x;
	y = warp_y;
	warp_x = (x * (1.0 - spd)) + (warp_to_x * spd);
	warp_y = (y * (1.0 - spd)) + (warp_to_y * spd);
	return 1;
     }
   warp_timer = NULL;
   return 0;
}

static int
_e_winlist_animator(void *data)
{
   if (warp_to)
     {
	int dx, dy;
	
	dx = warp_x - warp_to_x;
	dy = warp_y - warp_to_y;
	dx = dx * dx;
	dy = dy * dy;
	if ((dx <= 1) && (dy <= 1))
	  {
	     warp_x = warp_to_x;
	     warp_y = warp_to_y;
	     warp_to = 0;
	  }
	ecore_x_pointer_warp(winlist->zone->container->win,
			     warp_x, warp_y);
     }
   if (scroll_to)
     {
	double da;
	
	da = scroll_align - scroll_align_to;
	if (da < 0.0) da = -da;
	if (da < 0.01)
	  {
	     scroll_align = scroll_align_to;
	     scroll_to = 0;
	  }
        e_box_align_set(list_object, 0.5, 1.0 - scroll_align);
     }
   if ((warp_to) || (scroll_to)) return 1;
   animator = NULL;
   return 0;
}

#if 0
static void 
_e_winlist_cb_item_mouse_in(void *data, Evas *evas, Evas_Object *obj, 
      void *event_info)
{
   E_Winlist_Win *ww;
   Eina_List *l;

   if (!(ww = data)) return;
   if (!wins) return;
   for (l = wins; l; l = l->next)
     {
	if (l->data == ww) break;
     }
   _e_winlist_deactivate();
   win_selected = l;
   _e_winlist_show_active();
   _e_winlist_activate();
}
#endif
