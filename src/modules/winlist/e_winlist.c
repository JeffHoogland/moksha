#include "e.h"
#include "e_mod_main.h"

/* local subsystem functions */
typedef struct _E_Winlist_Win E_Winlist_Win;

struct _E_Winlist_Win
{
   Evas_Object  *bg_object;
   Evas_Object  *icon_object;
   E_Border     *border;
   unsigned char was_iconified : 1;
   unsigned char was_shaded : 1;
};

static void      _e_winlist_size_adjust(void);
static void      _e_winlist_border_add(E_Border *bd, E_Zone *zone, E_Desk *desk);
static void      _e_winlist_border_del(E_Border *bd);
static void      _e_winlist_activate_nth(int n);
static void      _e_winlist_activate(void);
static void      _e_winlist_deactivate(void);
static void      _e_winlist_show_active(void);
static Eina_Bool _e_winlist_cb_event_border_add(void *data, int type, void *event);
static Eina_Bool _e_winlist_cb_event_border_remove(void *data, int type, void *event);
static Eina_Bool _e_winlist_cb_key_down(void *data, int type, void *event);
static Eina_Bool _e_winlist_cb_key_up(void *data, int type, void *event);
static Eina_Bool _e_winlist_cb_mouse_down(void *data, int type, void *event);
static Eina_Bool _e_winlist_cb_mouse_up(void *data, int type, void *event);
static Eina_Bool _e_winlist_cb_mouse_wheel(void *data, int type, void *event);
static Eina_Bool _e_winlist_cb_mouse_move(void *data, int type, void *event);
static Eina_Bool _e_winlist_scroll_timer(void *data);
static Eina_Bool _e_winlist_warp_timer(void *data);
static Eina_Bool _e_winlist_animator(void *data);
#if 0
static void      _e_winlist_cb_item_mouse_in(void *data, Evas *evas,
                                             Evas_Object *obj, void *event_info);
#endif

/* local subsystem globals */
static E_Popup *_winlist = NULL;
static Evas_Object *_bg_object = NULL;
static Evas_Object *_list_object = NULL;
static Evas_Object *_icon_object = NULL;
static Eina_List *_wins = NULL;
static Eina_List *_win_selected = NULL;
static E_Desk *_last_desk = NULL;
static int _last_pointer_x = 0;
static int _last_pointer_y = 0;
static E_Border *_last_border = NULL;
static int _hold_count = 0;
static int _hold_mod = 0;
static E_Winlist_Activate_Type _activate_type = 0;
static Eina_List *_handlers = NULL;
static Ecore_X_Window _input_window = 0;
static int _warp_to = 0;
static int _warp_to_x = 0;
static int _warp_to_y = 0;
static int _warp_x = 0;
static int _warp_y = 0;
static int _old_warp_x = 0;
static int _old_warp_y = 0;
static int _scroll_to = 0;
static double _scroll_align_to = 0.0;
static double _scroll_align = 0.0;
static Ecore_Timer *_warp_timer = NULL;
static Ecore_Timer *_scroll_timer = NULL;
static Ecore_Animator *_animator = NULL;
static const Ecore_X_Window *_win = NULL;
static E_Border *_bd_next = NULL;

static Eina_Bool
_wmclass_picked(const Eina_List *lst, const char *wmclass)
{
   const Eina_List *l;
   const char *s;

   if (!wmclass) return EINA_FALSE;

   EINA_LIST_FOREACH(lst, l, s)
     if (s == wmclass)
       return EINA_TRUE;

   return EINA_FALSE;
}

/* externally accessible functions */
int
e_winlist_init(void)
{
   return 1;
}

int
e_winlist_shutdown(void)
{
   e_winlist_hide();
   return 1;
}

int
e_winlist_show(E_Zone *zone, E_Winlist_Filter filter)
{
   int x, y, w, h;
   Evas_Object *o;
   Eina_List *l;
   E_Desk *desk;
   E_Border *bd;
   Eina_List *wmclasses = NULL;

   E_OBJECT_CHECK_RETURN(zone, 0);
   E_OBJECT_TYPE_CHECK_RETURN(zone, E_ZONE_TYPE, 0);

   if (_winlist) return 0;

   _input_window = ecore_x_window_input_new(zone->container->win, 0, 0, 1, 1);
   ecore_x_window_show(_input_window);
   if (!e_grabinput_get(_input_window, 0, _input_window))
     {
        ecore_x_window_free(_input_window);
        _input_window = 0;
        return 0;
     }

   w = (double)zone->w * e_config->winlist_pos_size_w;
   if (w > e_config->winlist_pos_max_w) w = e_config->winlist_pos_max_w;
   else if (w < e_config->winlist_pos_min_w)
     w = e_config->winlist_pos_min_w;
   if (w > zone->w) w = zone->w;
   x = (double)(zone->w - w) * e_config->winlist_pos_align_x;

   h = (double)zone->h * e_config->winlist_pos_size_h;
   if (h > e_config->winlist_pos_max_h) h = e_config->winlist_pos_max_h;
   else if (h < e_config->winlist_pos_min_h)
     h = e_config->winlist_pos_min_h;
   if (h > zone->h) h = zone->h;
   y = (double)(zone->h - h) * e_config->winlist_pos_align_y;

   _winlist = e_popup_new(zone, x, y, w, h);
   if (!_winlist)
     {
        ecore_x_window_free(_input_window);
        e_grabinput_release(_input_window, _input_window);
        _input_window = 0;
        return 0;
     }
   e_border_move_cancel();
   e_border_resize_cancel();
   e_border_focus_track_freeze();

   evas_event_feed_mouse_in(_winlist->evas, ecore_x_current_time_get(), NULL);
   evas_event_feed_mouse_move(_winlist->evas, -1000000, -1000000,
                              ecore_x_current_time_get(), NULL);

   e_popup_layer_set(_winlist, E_LAYER_POPUP);
   evas_event_freeze(_winlist->evas);
   o = edje_object_add(_winlist->evas);
   _bg_object = o;
   e_theme_edje_object_set(o, "base/theme/winlist",
                           "e/widgets/winlist/main");
   evas_object_move(o, 0, 0);
   evas_object_resize(o, w, h);
   evas_object_show(o);
   e_popup_edje_bg_object_set(_winlist, o);

   o = e_box_add(_winlist->evas);
   _list_object = o;
   e_box_align_set(o, 0.5, 0.0);
   e_box_orientation_set(o, 0);
   e_box_homogenous_set(o, 1);
   edje_object_part_swallow(_bg_object, "e.swallow.list", o);
   edje_object_part_text_set(_bg_object, "e.text.title", _("Select a window"));
   evas_object_show(o);

   _last_border = e_border_focused_get();

   desk = e_desk_current_get(_winlist->zone);
   e_box_freeze(_list_object);
   EINA_LIST_FOREACH(e_border_focus_stack_get(), l, bd)
     {
        Eina_Bool pick;
        switch (filter)
          {
           case E_WINLIST_FILTER_CLASS_WINDOWS:
             if (!_last_border)
               pick = EINA_FALSE;
             else
               pick = _last_border->client.icccm.class == bd->client.icccm.class;
             break;
           case E_WINLIST_FILTER_CLASSES:
             pick = (!_wmclass_picked(wmclasses, bd->client.icccm.class));
             if (pick)
               wmclasses = eina_list_append(wmclasses, bd->client.icccm.class);
             break;

           default:
             pick = EINA_TRUE;
          }
        if (pick) _e_winlist_border_add(bd, _winlist->zone, desk);
     }
   e_box_thaw(_list_object);
   eina_list_free(wmclasses);

   if (!_wins)
     {
        e_winlist_hide();
        return 1;
     }

   if (e_config->winlist_list_show_other_desk_windows ||
       e_config->winlist_list_show_other_screen_windows)
     _last_desk = e_desk_current_get(_winlist->zone);
   if (e_config->winlist_warp_while_selecting)
     ecore_x_pointer_xy_get(_winlist->zone->container->win,
                            &_last_pointer_x, &_last_pointer_y);
   if (_last_border)
     {
        if (!_last_border->lock_focus_out)
          e_border_focus_set(_last_border, 0, 0);
        else
          _last_border = NULL;
     }
   _e_winlist_activate_nth(1);
   evas_event_thaw(_winlist->evas);
   _e_winlist_size_adjust();

   E_LIST_HANDLER_APPEND(_handlers, E_EVENT_BORDER_ADD, _e_winlist_cb_event_border_add, NULL);
   E_LIST_HANDLER_APPEND(_handlers, E_EVENT_BORDER_REMOVE, _e_winlist_cb_event_border_remove, NULL);
   E_LIST_HANDLER_APPEND(_handlers, ECORE_EVENT_KEY_DOWN, _e_winlist_cb_key_down, NULL);
   E_LIST_HANDLER_APPEND(_handlers, ECORE_EVENT_KEY_UP, _e_winlist_cb_key_up, NULL);
   E_LIST_HANDLER_APPEND(_handlers, ECORE_EVENT_MOUSE_BUTTON_DOWN, _e_winlist_cb_mouse_down, NULL);
   E_LIST_HANDLER_APPEND(_handlers, ECORE_EVENT_MOUSE_BUTTON_UP, _e_winlist_cb_mouse_up, NULL);
   E_LIST_HANDLER_APPEND(_handlers, ECORE_EVENT_MOUSE_WHEEL, _e_winlist_cb_mouse_wheel, NULL);
   E_LIST_HANDLER_APPEND(_handlers, ECORE_EVENT_MOUSE_MOVE, _e_winlist_cb_mouse_move, NULL);

   e_popup_show(_winlist);
   return 1;
}

void
e_winlist_hide(void)
{
   E_Border *bd = NULL;
   E_Winlist_Win *ww;
   Ecore_Event_Handler *handler;

   if (!_winlist) return;
   if (_win_selected)
     {
        ww = _win_selected->data;
        bd = ww->border;
     }
   evas_event_freeze(_winlist->evas);
   e_popup_hide(_winlist);
   e_box_freeze(_list_object);
   while (_wins)
     {
        ww = _wins->data;
        evas_object_del(ww->bg_object);
        if (ww->icon_object) evas_object_del(ww->icon_object);
        _wins = eina_list_remove_list(_wins, _wins);
        if ((!bd) || (ww->border != bd))
          e_object_unref(E_OBJECT(ww->border));
        free(ww);
     }
   e_box_thaw(_list_object);
   _win_selected = NULL;
   if (_icon_object)
     {
        evas_object_del(_icon_object);
        _icon_object = NULL;
     }
   evas_object_del(_list_object);
   _list_object = NULL;
   evas_object_del(_bg_object);
   _bg_object = NULL;
   evas_event_thaw(_winlist->evas);
   e_object_del(E_OBJECT(_winlist));
   e_border_focus_track_thaw();
   _winlist = NULL;
   _hold_count = 0;
   _hold_mod = 0;
   _activate_type = 0;

   EINA_LIST_FREE(_handlers, handler)
     ecore_event_handler_del(handler);

   if (_warp_timer)
     {
        ecore_timer_del(_warp_timer);
        _warp_timer = NULL;
     }
   if (_scroll_timer)
     {
        ecore_timer_del(_scroll_timer);
        _scroll_timer = NULL;
     }
   if (_animator)
     {
        ecore_animator_del(_animator);
        _animator = NULL;
     }
   if (bd)
     {
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
             e_border_focus_set(bd, 1, 1);
          }
        if ((e_config->focus_policy != E_FOCUS_CLICK) ||
            (e_config->winlist_warp_at_end) ||
            (e_config->winlist_warp_while_selecting))
          {
             _warp_to_x = bd->x + (bd->w / 2);
             if (_warp_to_x < (bd->zone->x + 1))
               _warp_to_x = bd->zone->x +
                 ((bd->x + bd->w - bd->zone->x) / 2);
             else if (_warp_to_x >= (bd->zone->x + bd->zone->w - 1))
               _warp_to_x = (bd->zone->x + bd->zone->w + bd->x) / 2;

             _warp_to_y = bd->y + (bd->h / 2);
             if (_warp_to_y < (bd->zone->y + 1))
               _warp_to_y = bd->zone->y +
                 ((bd->y + bd->h - bd->zone->y) / 2);
             else if (_warp_to_y >= (bd->zone->y + bd->zone->h - 1))
               _warp_to_y = (bd->zone->y + bd->zone->h + bd->y) / 2;
             ecore_x_pointer_warp(bd->zone->container->win, _warp_to_x, _warp_to_y);
          }

        e_object_unref(E_OBJECT(bd));
     }
   
   e_border_idler_before();

   ecore_x_window_free(_input_window);
   e_grabinput_release(_input_window, _input_window);
   _input_window = 0;
}

void
e_winlist_next(void)
{
   if (!_winlist) return;
   if (eina_list_count(_wins) == 1)
     {
        if (!_win_selected)
          {
             _win_selected = _wins;
             _e_winlist_show_active();
             _e_winlist_activate();
          }
        return;
     }
   _e_winlist_deactivate();
   if (!_win_selected)
     _win_selected = _wins;
   else
     _win_selected = _win_selected->next;
   if (!_win_selected) _win_selected = _wins;
   _e_winlist_show_active();
   _e_winlist_activate();
}

void
e_winlist_prev(void)
{
   if (!_winlist) return;
   if (eina_list_count(_wins) == 1)
     {
        if (!_win_selected)
          {
             _win_selected = _wins;
             _e_winlist_show_active();
             _e_winlist_activate();
          }
        return;
     }
   _e_winlist_deactivate();
   if (!_win_selected)
     _win_selected = _wins;
   else
     _win_selected = _win_selected->prev;
   if (!_win_selected) _win_selected = eina_list_last(_wins);
   _e_winlist_show_active();
   _e_winlist_activate();
}

void
e_winlist_left(E_Zone *zone)
{
   E_Border *bd;
   Eina_List *l;
   E_Desk *desk;
   E_Border *bd_orig;
   int delta = INT_MAX;
   int delta2 = INT_MAX;
   int center;

   _bd_next = NULL;

   E_OBJECT_CHECK_RETURN(zone, 0);
   E_OBJECT_TYPE_CHECK_RETURN(zone, E_ZONE_TYPE, 0);

   bd_orig = e_border_focused_get();
   if (!bd_orig) return;

   center = bd_orig->x + bd_orig->w / 2;

   desk = e_desk_current_get(zone);
   EINA_LIST_FOREACH(e_border_focus_stack_get(), l, bd)
     {
        int center_next;
        int delta_next;
        int delta2_next;

        if (bd == bd_orig) continue;
        if ((!bd->client.icccm.accepts_focus) &&
            (!bd->client.icccm.take_focus)) continue;
        if (bd->client.netwm.state.skip_taskbar) continue;
        if (bd->user_skip_winlist) continue;
        if (bd->iconic)
          {
             if (!e_config->winlist_list_show_iconified) continue;
             if ((bd->zone != zone) &&
                 (!e_config->winlist_list_show_other_screen_iconified))
               continue;
             if ((bd->desk != desk) &&
                 (!e_config->winlist_list_show_other_desk_iconified)) continue;
          }
        else
          {
             if (bd->sticky)
               {
                  if ((bd->zone != zone) &&
                      (!e_config->winlist_list_show_other_screen_windows))
                    continue;
               }
             else
               {
                  if (bd->desk != desk)
                    {
                       if ((bd->zone) && (bd->zone != zone))
                         {
                            if (!e_config->winlist_list_show_other_screen_windows)
                              continue;
                         }
                       else if (!e_config->winlist_list_show_other_desk_windows)
                         continue;
                    }
               }
          }
        /* bd is suitable */
        center_next = bd->x + bd->w / 2;
        if (center_next >= center) continue;
        delta_next = bd_orig->x - (bd->x + bd->w);
        if (delta_next < 0) delta_next = center - center_next;
        delta2_next = abs(bd_orig->y - bd_orig->h / 2 - bd->y + bd->h/2);
        if (delta_next >= 0 && delta_next <= delta &&
            delta2_next >= 0 && delta2_next <= delta2)
          {
             _bd_next = bd;
             delta = delta_next;
             delta2 = delta2_next;
          }
     }

   if (_bd_next)
     {
        if (!bd_orig->lock_focus_out)
          e_border_focus_set(bd_orig, 0, 0);

        if ((e_config->focus_policy != E_FOCUS_CLICK) ||
            (e_config->winlist_warp_at_end) ||
            (e_config->winlist_warp_while_selecting))
          {
             _warp_to_x = _bd_next->x + (_bd_next->w / 2);
             if (_warp_to_x < (_bd_next->zone->x + 1))
               _warp_to_x = _bd_next->zone->x +
                 ((_bd_next->x + _bd_next->w - _bd_next->zone->x) / 2);
             else if (_warp_to_x >= (_bd_next->zone->x + _bd_next->zone->w - 1))
               _warp_to_x = (_bd_next->zone->x + _bd_next->zone->w + _bd_next->x) / 2;

             _warp_to_y = _bd_next->y + (_bd_next->h / 2);
             if (_warp_to_y < (_bd_next->zone->y + 1))
               _warp_to_y = _bd_next->zone->y +
                 ((_bd_next->y + _bd_next->h - _bd_next->zone->y) / 2);
             else if (_warp_to_y >= (_bd_next->zone->y + _bd_next->zone->h - 1))
               _warp_to_y = (_bd_next->zone->y + _bd_next->zone->h + _bd_next->y) / 2;

             _old_warp_x = _old_warp_y = INT_MAX;
          }

        ecore_x_pointer_xy_get(zone->container->win, &_warp_x, &_warp_y);
        _win = &zone->container->win;
        _warp_to = 1;
        if (!_warp_timer)
          _warp_timer = ecore_timer_add(0.01, _e_winlist_warp_timer, NULL);
        if (!_animator)
          _animator = ecore_animator_add(_e_winlist_animator, NULL);

        if ((!_bd_next->lock_user_stacking) &&
            (e_config->winlist_list_raise_while_selecting))
          e_border_raise(_bd_next);
        if ((!_bd_next->lock_focus_out) &&
            (e_config->winlist_list_focus_while_selecting))
          e_border_focus_set(_bd_next, 1, 1);
     }
}

void
e_winlist_down(E_Zone *zone)
{
   E_Border *bd;
   Eina_List *l;
   E_Desk *desk;
   E_Border *bd_orig;
   int delta = INT_MAX;
   int delta2 = INT_MAX;
   int center;

   _bd_next = NULL;

   E_OBJECT_CHECK_RETURN(zone, 0);
   E_OBJECT_TYPE_CHECK_RETURN(zone, E_ZONE_TYPE, 0);

   bd_orig = e_border_focused_get();
   if (!bd_orig) return;

   center = bd_orig->y + bd_orig->h / 2;

   desk = e_desk_current_get(zone);
   EINA_LIST_FOREACH(e_border_focus_stack_get(), l, bd)
     {
        int center_next;
        int delta_next;
        int delta2_next;

        if (bd == bd_orig) continue;
        if ((!bd->client.icccm.accepts_focus) &&
            (!bd->client.icccm.take_focus)) continue;
        if (bd->client.netwm.state.skip_taskbar) continue;
        if (bd->user_skip_winlist) continue;
        if (bd->iconic)
          {
             if (!e_config->winlist_list_show_iconified) continue;
             if ((bd->zone != zone) &&
                 (!e_config->winlist_list_show_other_screen_iconified))
               continue;
             if ((bd->desk != desk) &&
                 (!e_config->winlist_list_show_other_desk_iconified)) continue;
          }
        else
          {
             if (bd->sticky)
               {
                  if ((bd->zone != zone) &&
                      (!e_config->winlist_list_show_other_screen_windows))
                    continue;
               }
             else
               {
                  if (bd->desk != desk)
                    {
                       if ((bd->zone) && (bd->zone != zone))
                         {
                            if (!e_config->winlist_list_show_other_screen_windows)
                              continue;
                         }
                       else if (!e_config->winlist_list_show_other_desk_windows)
                         continue;
                    }
               }
          }
        /* bd is suitable */
        center_next = bd->y + bd->h / 2;
        if (center_next <= center) continue;
        delta_next = bd->y - (bd_orig->y + bd_orig->h);
        if (delta_next < 0) delta_next = center - center_next;
        delta2_next = abs(bd_orig->x - bd_orig->w / 2 - bd->x + bd->w/2);
        if (delta_next >= 0 && delta_next <= delta &&
            delta2_next >= 0 && delta2_next <= delta2)
          {
             _bd_next = bd;
             delta = delta_next;
             delta2 = delta2_next;
          }
     }

   if (_bd_next)
     {
        if (!bd_orig->lock_focus_out)
          e_border_focus_set(bd_orig, 0, 0);

        if ((e_config->focus_policy != E_FOCUS_CLICK) ||
            (e_config->winlist_warp_at_end) ||
            (e_config->winlist_warp_while_selecting))
          {
             _warp_to_x = _bd_next->x + (_bd_next->w / 2);
             if (_warp_to_x < (_bd_next->zone->x + 1))
               _warp_to_x = _bd_next->zone->x +
                 ((_bd_next->x + _bd_next->w - _bd_next->zone->x) / 2);
             else if (_warp_to_x >= (_bd_next->zone->x + _bd_next->zone->w - 1))
               _warp_to_x = (_bd_next->zone->x + _bd_next->zone->w + _bd_next->x) / 2;

             _warp_to_y = _bd_next->y + (_bd_next->h / 2);
             if (_warp_to_y < (_bd_next->zone->y + 1))
               _warp_to_y = _bd_next->zone->y +
                 ((_bd_next->y + _bd_next->h - _bd_next->zone->y) / 2);
             else if (_warp_to_y >= (_bd_next->zone->y + _bd_next->zone->h - 1))
               _warp_to_y = (_bd_next->zone->y + _bd_next->zone->h + _bd_next->y) / 2;

             _old_warp_x = _old_warp_y = INT_MAX;
          }

        ecore_x_pointer_xy_get(zone->container->win, &_warp_x, &_warp_y);
        _win = &zone->container->win;
        _warp_to = 1;
        if (!_warp_timer)
          _warp_timer = ecore_timer_add(0.01, _e_winlist_warp_timer, NULL);
        if (!_animator)
          _animator = ecore_animator_add(_e_winlist_animator, NULL);

        if ((!_bd_next->lock_user_stacking) &&
            (e_config->winlist_list_raise_while_selecting))
          e_border_raise(_bd_next);
        if ((!_bd_next->lock_focus_out) &&
            (e_config->winlist_list_focus_while_selecting))
          e_border_focus_set(_bd_next, 1, 1);
     }
}

void
e_winlist_up(E_Zone *zone)
{
   E_Border *bd;
   Eina_List *l;
   E_Desk *desk;
   E_Border *bd_orig;
   int delta = INT_MAX;
   int delta2 = INT_MAX;
   int center;

   _bd_next = NULL;

   E_OBJECT_CHECK_RETURN(zone, 0);
   E_OBJECT_TYPE_CHECK_RETURN(zone, E_ZONE_TYPE, 0);

   bd_orig = e_border_focused_get();
   if (!bd_orig) return;

   center = bd_orig->y + bd_orig->h / 2;

   desk = e_desk_current_get(zone);
   EINA_LIST_FOREACH(e_border_focus_stack_get(), l, bd)
     {
        int center_next;
        int delta_next;
        int delta2_next;

        if (bd == bd_orig) continue;
        if ((!bd->client.icccm.accepts_focus) &&
            (!bd->client.icccm.take_focus)) continue;
        if (bd->client.netwm.state.skip_taskbar) continue;
        if (bd->user_skip_winlist) continue;
        if (bd->iconic)
          {
             if (!e_config->winlist_list_show_iconified) continue;
             if ((bd->zone != zone) &&
                 (!e_config->winlist_list_show_other_screen_iconified))
               continue;
             if ((bd->desk != desk) &&
                 (!e_config->winlist_list_show_other_desk_iconified)) continue;
          }
        else
          {
             if (bd->sticky)
               {
                  if ((bd->zone != zone) &&
                      (!e_config->winlist_list_show_other_screen_windows))
                    continue;
               }
             else
               {
                  if (bd->desk != desk)
                    {
                       if ((bd->zone) && (bd->zone != zone))
                         {
                            if (!e_config->winlist_list_show_other_screen_windows)
                              continue;
                         }
                       else if (!e_config->winlist_list_show_other_desk_windows)
                         continue;
                    }
               }
          }
        /* bd is suitable */
        center_next = bd->y + bd->h / 2;
        if (center_next >= center) continue;
        delta_next = bd_orig->y - (bd->y + bd->h);
        if (delta_next < 0) delta_next = center - center_next;
        delta2_next = abs(bd_orig->x - bd_orig->w / 2 - bd->x + bd->w/2);
        if (delta_next >= 0 && delta_next <= delta &&
            delta2_next >= 0 && delta2_next <= delta2)
          {
             _bd_next = bd;
             delta = delta_next;
             delta2 = delta2_next;
          }
     }

   if (_bd_next)
     {
        if (!bd_orig->lock_focus_out)
          e_border_focus_set(bd_orig, 0, 0);

        if ((e_config->focus_policy != E_FOCUS_CLICK) ||
            (e_config->winlist_warp_at_end) ||
            (e_config->winlist_warp_while_selecting))
          {
             _warp_to_x = _bd_next->x + (_bd_next->w / 2);
             if (_warp_to_x < (_bd_next->zone->x + 1))
               _warp_to_x = _bd_next->zone->x +
                 ((_bd_next->x + _bd_next->w - _bd_next->zone->x) / 2);
             else if (_warp_to_x >= (_bd_next->zone->x + _bd_next->zone->w - 1))
               _warp_to_x = (_bd_next->zone->x + _bd_next->zone->w + _bd_next->x) / 2;

             _warp_to_y = _bd_next->y + (_bd_next->h / 2);
             if (_warp_to_y < (_bd_next->zone->y + 1))
               _warp_to_y = _bd_next->zone->y +
                 ((_bd_next->y + _bd_next->h - _bd_next->zone->y) / 2);
             else if (_warp_to_y >= (_bd_next->zone->y + _bd_next->zone->h - 1))
               _warp_to_y = (_bd_next->zone->y + _bd_next->zone->h + _bd_next->y) / 2;

             _old_warp_x = _old_warp_y = INT_MAX;
          }

        ecore_x_pointer_xy_get(zone->container->win, &_warp_x, &_warp_y);
        _win = &zone->container->win;
        _warp_to = 1;
        if (!_warp_timer)
          _warp_timer = ecore_timer_add(0.01, _e_winlist_warp_timer, NULL);
        if (!_animator)
          _animator = ecore_animator_add(_e_winlist_animator, NULL);

        if ((!_bd_next->lock_user_stacking) &&
            (e_config->winlist_list_raise_while_selecting))
          e_border_raise(_bd_next);
        if ((!_bd_next->lock_focus_out) &&
            (e_config->winlist_list_focus_while_selecting))
          e_border_focus_set(_bd_next, 1, 1);
     }
}

void
e_winlist_right(E_Zone *zone)
{
   E_Border *bd;
   Eina_List *l;
   E_Desk *desk;
   E_Border *bd_orig;
   int delta = INT_MAX;
   int delta2 = INT_MAX;
   int center;

   _bd_next = NULL;

   E_OBJECT_CHECK_RETURN(zone, 0);
   E_OBJECT_TYPE_CHECK_RETURN(zone, E_ZONE_TYPE, 0);

   bd_orig = e_border_focused_get();
   if (!bd_orig) return;

   center = bd_orig->x + bd_orig->w / 2;

   desk = e_desk_current_get(zone);
   EINA_LIST_FOREACH(e_border_focus_stack_get(), l, bd)
     {
        int center_next;
        int delta_next;
        int delta2_next;

        if (bd == bd_orig) continue;
        if ((!bd->client.icccm.accepts_focus) &&
            (!bd->client.icccm.take_focus)) continue;
        if (bd->client.netwm.state.skip_taskbar) continue;
        if (bd->user_skip_winlist) continue;
        if (bd->iconic)
          {
             if (!e_config->winlist_list_show_iconified) continue;
             if ((bd->zone != zone) &&
                 (!e_config->winlist_list_show_other_screen_iconified))
               continue;
             if ((bd->desk != desk) &&
                 (!e_config->winlist_list_show_other_desk_iconified)) continue;
          }
        else
          {
             if (bd->sticky)
               {
                  if ((bd->zone != zone) &&
                      (!e_config->winlist_list_show_other_screen_windows))
                    continue;
               }
             else
               {
                  if (bd->desk != desk)
                    {
                       if ((bd->zone) && (bd->zone != zone))
                         {
                            if (!e_config->winlist_list_show_other_screen_windows)
                              continue;
                         }
                       else if (!e_config->winlist_list_show_other_desk_windows)
                         continue;
                    }
               }
          }
        /* bd is suitable */
        center_next = bd->x + bd->w / 2;
        if (center_next <= center) continue;
        delta_next = bd->x - (bd_orig->x + bd_orig->w);
        if (delta_next < 0) delta = center_next - center;
        delta2_next = abs(bd_orig->y - bd_orig->h / 2 - bd->y + bd->h/2);
        if (delta_next >= 0 && delta_next <= delta &&
            delta2_next >= 0 && delta2_next <= delta2)
          {
             _bd_next = bd;
             delta = delta_next;
             delta2 = delta2_next;
          }
     }

   if (_bd_next)
     {
        if (!bd_orig->lock_focus_out)
          e_border_focus_set(bd_orig, 0, 0);

        if ((e_config->focus_policy != E_FOCUS_CLICK) ||
            (e_config->winlist_warp_at_end) ||
            (e_config->winlist_warp_while_selecting))
          {
             _warp_to_x = _bd_next->x + (_bd_next->w / 2);
             if (_warp_to_x < (_bd_next->zone->x + 1))
               _warp_to_x = _bd_next->zone->x +
                 ((_bd_next->x + _bd_next->w - _bd_next->zone->x) / 2);
             else if (_warp_to_x >= (_bd_next->zone->x + _bd_next->zone->w - 1))
               _warp_to_x = (_bd_next->zone->x + _bd_next->zone->w + _bd_next->x) / 2;

             _warp_to_y = _bd_next->y + (_bd_next->h / 2);
             if (_warp_to_y < (_bd_next->zone->y + 1))
               _warp_to_y = _bd_next->zone->y +
                 ((_bd_next->y + _bd_next->h - _bd_next->zone->y) / 2);
             else if (_warp_to_y >= (_bd_next->zone->y + _bd_next->zone->h - 1))
               _warp_to_y = (_bd_next->zone->y + _bd_next->zone->h + _bd_next->y) / 2;

             _old_warp_x = _old_warp_y = INT_MAX;
          }

        ecore_x_pointer_xy_get(zone->container->win, &_warp_x, &_warp_y);
        _win = &zone->container->win;
        _warp_to = 1;
        if (!_warp_timer)
          _warp_timer = ecore_timer_add(0.01, _e_winlist_warp_timer, NULL);
        if (!_animator)
          _animator = ecore_animator_add(_e_winlist_animator, NULL);

        if ((!_bd_next->lock_user_stacking) &&
            (e_config->winlist_list_raise_while_selecting))
          e_border_raise(_bd_next);
        if ((!_bd_next->lock_focus_out) &&
            (e_config->winlist_list_focus_while_selecting))
          e_border_focus_set(_bd_next, 1, 1);
     }
}

void
e_winlist_modifiers_set(int mod, E_Winlist_Activate_Type type)
{
   if (!_winlist) return;
   _hold_mod = mod;
   _hold_count = 0;
   _activate_type = type;
   if (type == E_WINLIST_ACTIVATE_TYPE_MOUSE) _hold_count++;
   if (_hold_mod & ECORE_EVENT_MODIFIER_SHIFT) _hold_count++;
   if (_hold_mod & ECORE_EVENT_MODIFIER_CTRL) _hold_count++;
   if (_hold_mod & ECORE_EVENT_MODIFIER_ALT) _hold_count++;
   if (_hold_mod & ECORE_EVENT_MODIFIER_WIN) _hold_count++;
}

/* local subsystem functions */
static void
_e_winlist_size_adjust(void)
{
   Evas_Coord mw, mh;
   E_Zone *zone;
   int x, y, w, h;

   e_box_freeze(_list_object);
   e_box_size_min_get(_list_object, &mw, &mh);
   evas_object_size_hint_min_set(_list_object, mw, mh);
   edje_object_part_swallow(_bg_object, "e.swallow.list", _list_object);
   edje_object_size_min_calc(_bg_object, &mw, &mh);
   evas_object_size_hint_min_set(_list_object, -1, -1);
   edje_object_part_swallow(_bg_object, "e.swallow.list", _list_object);
   e_box_thaw(_list_object);

   zone = _winlist->zone;
   w = (double)zone->w * e_config->winlist_pos_size_w;
   if (w < mw) w = mw;
   if (w > e_config->winlist_pos_max_w) w = e_config->winlist_pos_max_w;
   else if (w < e_config->winlist_pos_min_w)
     w = e_config->winlist_pos_min_w;
   if (w > zone->w) w = zone->w;
   x = (double)(zone->w - w) * e_config->winlist_pos_align_x;

   h = mh;
   if (h > e_config->winlist_pos_max_h) h = e_config->winlist_pos_max_h;
   else if (h < e_config->winlist_pos_min_h)
     h = e_config->winlist_pos_min_h;
   if (h > zone->h) h = zone->h;
   y = (double)(zone->h - h) * e_config->winlist_pos_align_y;

   evas_object_resize(_bg_object, w, h);
   e_popup_move_resize(_winlist, x, y, w, h);
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
                       if (!e_config->winlist_list_show_other_screen_windows)
                         return;
                       if (bd->zone && bd->desk && (bd->desk != e_desk_current_get(bd->zone)))
                         {
                            if (!e_config->winlist_list_show_other_desk_windows)
                              return;
                         }
                    }
                  else if (!e_config->winlist_list_show_other_desk_windows)
                    return;
               }
          }
     }

   ww = E_NEW(E_Winlist_Win, 1);
   if (!ww) return;
   ww->border = bd;
   _wins = eina_list_append(_wins, ww);
   o = edje_object_add(_winlist->evas);
   ww->bg_object = o;
   e_theme_edje_object_set(o, "base/theme/winlist",
                           "e/widgets/winlist/item");
   edje_object_part_text_set(o, "e.text.label", e_border_name_get(ww->border));
   evas_object_show(o);
   if (edje_object_part_exists(ww->bg_object, "e.swallow.icon"))
     {
        o = e_border_icon_add(bd, _winlist->evas);
        ww->icon_object = o;
        edje_object_part_swallow(ww->bg_object, "e.swallow.icon", o);
        evas_object_show(o);
     }
   if (bd->shaded)
     edje_object_signal_emit(ww->bg_object, "e,state,shaded", "e");
   else if (bd->iconic)
     edje_object_signal_emit(ww->bg_object, "e,state,iconified", "e");
   else if (bd->desk != desk)
     {
        if (!((bd->sticky) && (bd->zone == zone)))
          edje_object_signal_emit(ww->bg_object, "e,state,invisible", "e");
     }

   edje_object_size_min_calc(ww->bg_object, &mw, &mh);
   e_box_pack_end(_list_object, ww->bg_object);
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
   E_Winlist_Win *ww;
   Eina_List *l;

   if (bd == _last_border) _last_border = NULL;
   EINA_LIST_FOREACH(_wins, l, ww)
     {
        if (ww->border == bd)
          {
             e_object_unref(E_OBJECT(ww->border));
             if (l == _win_selected)
               {
                  _win_selected = l->next;
                  if (!_win_selected) _win_selected = l->prev;
                  _e_winlist_show_active();
                  _e_winlist_activate();
               }
             evas_object_del(ww->bg_object);
             if (ww->icon_object) evas_object_del(ww->icon_object);
             E_FREE(ww);
             _wins = eina_list_remove_list(_wins, l);
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
   cnt = eina_list_count(_wins);
   if (n >= cnt) n = cnt - 1;
   l = eina_list_nth_list(_wins, n);
   if (l)
     {
        _win_selected = l;
        _e_winlist_show_active();
        _e_winlist_activate();
     }
}

static void
_e_winlist_activate(void)
{
   E_Winlist_Win *ww;
   Evas_Object *o;
   int ok = 0;

   if (!_win_selected) return;
   ww = _win_selected->data;
   edje_object_signal_emit(ww->bg_object, "e,state,selected", "e");
   if (ww->icon_object)
     edje_object_signal_emit(ww->icon_object,
                             "e,state,selected", "e");

   if ((ww->border->iconic) &&
       (e_config->winlist_list_uncover_while_selecting))
     {
        if (!ww->border->lock_user_iconify)
          e_border_uniconify(ww->border);
        ww->was_iconified = 1;
        ok = 1;
     }
   if ((!ww->border->sticky) &&
       (ww->border->desk != e_desk_current_get(_winlist->zone)) &&
       (e_config->winlist_list_jump_desk_while_selecting))
     {
        if (ww->border->desk) e_desk_show(ww->border->desk);
        ok = 1;
     }
   if (((ww->border->shaded) ||
        ((ww->border->changes.shaded) &&
         (ww->border->shade.val != ww->border->shaded) &&
         (ww->border->shade.val))) &&
       (ww->border->desk == e_desk_current_get(_winlist->zone)) &&
       (e_config->winlist_list_uncover_while_selecting))
     {
        if (!ww->border->lock_user_shade)
          e_border_unshade(ww->border, ww->border->shade.dir);
        ww->was_shaded = 1;
        ok = 1;
     }
   if ((!ww->border->iconic) &&
       ((ww->border->desk == e_desk_current_get(_winlist->zone)) ||
        (ww->border->sticky)))
     ok = 1;
   if (ok)
     {
        if ((e_config->focus_policy != E_FOCUS_CLICK) ||
            (e_config->winlist_warp_at_end) ||
            (e_config->winlist_warp_while_selecting))
          {
             _warp_to_x = ww->border->x + (ww->border->w / 2);
             if (_warp_to_x < (ww->border->zone->x + 1))
               _warp_to_x = ww->border->zone->x +
                 ((ww->border->x + ww->border->w - ww->border->zone->x) / 2);
             else if (_warp_to_x >= (ww->border->zone->x + ww->border->zone->w - 1))
               _warp_to_x = (ww->border->zone->x +
                             ww->border->zone->w + ww->border->x) / 2;

             _warp_to_y = ww->border->y + (ww->border->h / 2);
             if (_warp_to_y < (ww->border->zone->y + 1))
               _warp_to_y = ww->border->zone->y +
                 ((ww->border->y + ww->border->h - ww->border->zone->y) / 2);
             else if (_warp_to_y >= (ww->border->zone->y + ww->border->zone->h - 1))
               _warp_to_y = (ww->border->zone->y +
                             ww->border->zone->h + ww->border->y) / 2;
          }
        if (e_config->winlist_warp_while_selecting)
          {
             ecore_x_pointer_xy_get(_winlist->zone->container->win,
                                    &_warp_x, &_warp_y);
             _win = &_winlist->zone->container->win;
             e_border_focus_latest_set(ww->border);
             _warp_to = 1;
             if (!_warp_timer)
               _warp_timer = ecore_timer_add(0.01, _e_winlist_warp_timer, NULL);
             if (!_animator)
               _animator = ecore_animator_add(_e_winlist_animator, NULL);
          }
        else
          {
             _warp_to = 0;
             if (_warp_timer)
               {
                  ecore_timer_del(_warp_timer);
                  _warp_timer = NULL;
               }
             if (_animator)
               {
                  ecore_animator_del(_animator);
                  _animator = NULL;
               }
          }

        if ((!ww->border->lock_user_stacking) &&
            (e_config->winlist_list_raise_while_selecting))
          e_border_raise(ww->border);
        if ((!ww->border->lock_focus_out) &&
            (e_config->winlist_list_focus_while_selecting))
          e_border_focus_set(ww->border, 1, 0);
     }
   edje_object_part_text_set(_bg_object, "e.text.label",
                             e_border_name_get(ww->border));
   if (_icon_object)
     {
        evas_object_del(_icon_object);
        _icon_object = NULL;
     }
   if (edje_object_part_exists(_bg_object, "e.swallow.icon"))
     {
        o = e_border_icon_add(ww->border, _winlist->evas);
        _icon_object = o;
        edje_object_part_swallow(_bg_object, "e.swallow.icon", o);
        evas_object_show(o);
     }

   edje_object_signal_emit(_bg_object, "e,state,selected", "e");
}

static void
_e_winlist_deactivate(void)
{
   E_Winlist_Win *ww;

   if (!_win_selected) return;
   ww = _win_selected->data;
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
   if (_icon_object)
     {
        evas_object_del(_icon_object);
        _icon_object = NULL;
     }
   edje_object_part_text_set(_bg_object, "e.text.label", "");
   edje_object_signal_emit(ww->bg_object, "e,state,unselected", "e");
   if (ww->icon_object)
     edje_object_signal_emit(ww->icon_object,
                             "e,state,unselected", "e");
   if (!ww->border->lock_focus_in)
     e_border_focus_set(ww->border, 0, 0);
}

static void
_e_winlist_show_active(void)
{
   Eina_List *l;
   int i, n;

   if (!_wins) return;

   for (i = 0, l = _wins; l; l = l->next, i++)
     if (l == _win_selected) break;

   n = eina_list_count(_wins);
   if (n <= 1) return;
   _scroll_align_to = (double)i / (double)(n - 1);
   if (e_config->winlist_scroll_animate)
     {
        _scroll_to = 1;
        if (!_scroll_timer)
          _scroll_timer = ecore_timer_add(0.01, _e_winlist_scroll_timer, NULL);
        if (!_animator)
          _animator = ecore_animator_add(_e_winlist_animator, NULL);
     }
   else
     {
        _scroll_align = _scroll_align_to;
        e_box_align_set(_list_object, 0.5, _scroll_align);
     }
}

static void
_e_winlist_restore_desktop(void)
{
   if (_last_desk &&
       (e_config->winlist_list_show_other_desk_windows ||
        e_config->winlist_list_show_other_screen_windows))
     e_desk_show(_last_desk);
   if (e_config->winlist_warp_while_selecting)
     ecore_x_pointer_warp(_winlist->zone->container->win,
                          _last_pointer_x, _last_pointer_y);
   _e_winlist_deactivate();
   _win_selected = NULL;
   e_winlist_hide();
   if (_last_border)
     {
        e_border_focus_set(_last_border, 1, 1);
        _last_border = NULL;
     }
}

static Eina_Bool
_e_winlist_cb_event_border_add(void *data __UNUSED__, int type __UNUSED__,
                               void *event)
{
   E_Event_Border_Add *ev;

   ev = event;
   _e_winlist_border_add(ev->border, _winlist->zone,
                         e_desk_current_get(_winlist->zone));
   _e_winlist_size_adjust();
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_winlist_cb_event_border_remove(void *data __UNUSED__, int type __UNUSED__,
                                  void *event)
{
   E_Event_Border_Remove *ev;

   ev = event;
   _e_winlist_border_del(ev->border);
   _e_winlist_size_adjust();
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_winlist_cb_key_down(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_Event_Key *ev;

   ev = event;
   if (ev->window != _input_window) return ECORE_CALLBACK_PASS_ON;
   if (!strcmp(ev->key, "Up"))
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
        Eina_List *l;
        E_Config_Binding_Key *binding;
        E_Binding_Modifier mod;

        EINA_LIST_FOREACH(e_config->key_bindings, l, binding)
          {
             if (binding->action != _winlist_act) continue;

             mod = 0;

             if (ev->modifiers & ECORE_EVENT_MODIFIER_SHIFT)
               mod |= E_BINDING_MODIFIER_SHIFT;
             if (ev->modifiers & ECORE_EVENT_MODIFIER_CTRL)
               mod |= E_BINDING_MODIFIER_CTRL;
             if (ev->modifiers & ECORE_EVENT_MODIFIER_ALT)
               mod |= E_BINDING_MODIFIER_ALT;
             if (ev->modifiers & ECORE_EVENT_MODIFIER_WIN)
               mod |= E_BINDING_MODIFIER_WIN;

             if (binding->key && (!strcmp(binding->key, ev->keyname)) &&
                 ((binding->modifiers == mod) || (binding->any_mod)))
               {
                  if (!_act_winlist) continue;
                  if (_act_winlist->func.go_key)
                    _act_winlist->func.go_key(E_OBJECT(_winlist->zone), binding->params, ev);
                  else if (_act_winlist->func.go)
                    _act_winlist->func.go(E_OBJECT(_winlist->zone), binding->params);
               }
          }
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_winlist_cb_key_up(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_Event_Key *ev;
   Eina_List *l;
   E_Config_Binding_Key *binding;
   E_Binding_Modifier mod;

   ev = event;
   if (!_winlist) return ECORE_CALLBACK_PASS_ON;
   if (_hold_mod)
     {
#define KEY_CHECK(MOD, NAME) \
        if ((_hold_mod & ECORE_EVENT_MODIFIER_##MOD) && (!strcmp(ev->key, NAME))) \
          _hold_count--, _hold_mod &= ~ECORE_EVENT_MODIFIER_##MOD
        KEY_CHECK(SHIFT, "Shift_L");
        else KEY_CHECK(SHIFT, "Shift_R");
        else KEY_CHECK(CTRL, "Control_L");
        else KEY_CHECK(CTRL, "Control_R");
        else KEY_CHECK(ALT, "Alt_L");
        else KEY_CHECK(ALT, "Alt_R");
        else KEY_CHECK(ALT, "Meta_L");
        else KEY_CHECK(ALT, "Meta_R");
        else KEY_CHECK(WIN, "Meta_L");
        else KEY_CHECK(WIN, "Meta_R");
        else KEY_CHECK(ALT, "Super_L");
        else KEY_CHECK(ALT, "Super_R");
        else KEY_CHECK(WIN, "Super_L");
        else KEY_CHECK(WIN, "Super_R");
        else KEY_CHECK(WIN, "Mode_switch");

        if ((_hold_count <= 0) || ((!_hold_mod) && (_activate_type == E_WINLIST_ACTIVATE_TYPE_KEY)))
          {
             e_winlist_hide();
             return 1;
          }
     }

   EINA_LIST_FOREACH(e_config->key_bindings, l, binding)
     {
        if (binding->action != _winlist_act) continue;
        mod = 0;

        if (ev->modifiers & ECORE_EVENT_MODIFIER_SHIFT)
          mod |= E_BINDING_MODIFIER_SHIFT;
        if (ev->modifiers & ECORE_EVENT_MODIFIER_CTRL)
          mod |= E_BINDING_MODIFIER_CTRL;
        if (ev->modifiers & ECORE_EVENT_MODIFIER_ALT)
          mod |= E_BINDING_MODIFIER_ALT;
        if (ev->modifiers & ECORE_EVENT_MODIFIER_WIN)
          mod |= E_BINDING_MODIFIER_WIN;

        if (binding->key && (!strcmp(binding->key, ev->keyname)) &&
            ((binding->modifiers == mod) || (binding->any_mod)))
          {
             if (!_act_winlist) continue;
             if (_act_winlist->func.end_key)
               _act_winlist->func.end_key(E_OBJECT(_winlist->zone), binding->params, ev);
             else if (_act_winlist->func.end)
               _act_winlist->func.end(E_OBJECT(_winlist->zone), binding->params);
          }
     }

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_winlist_cb_mouse_down(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_Event_Mouse_Button *ev;

   ev = event;
   if (ev->window != _input_window) return ECORE_CALLBACK_PASS_ON;
   e_bindings_mouse_down_event_handle(E_BINDING_CONTEXT_WINLIST,
                                      E_OBJECT(_winlist->zone), ev);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_winlist_cb_mouse_up(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_Event_Mouse_Button *ev;

   ev = event;
   if (ev->window != _input_window) return ECORE_CALLBACK_PASS_ON;
   if (e_bindings_mouse_up_event_handle(E_BINDING_CONTEXT_WINLIST, E_OBJECT(_winlist->zone), ev))
     return ECORE_CALLBACK_RENEW;
   if (_activate_type != E_WINLIST_ACTIVATE_TYPE_MOUSE) return ECORE_CALLBACK_RENEW;
   if (!--_hold_count) e_winlist_hide();
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_winlist_cb_mouse_wheel(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_Event_Mouse_Wheel *ev;
   int i;

   ev = event;
   if (ev->window != _input_window) return ECORE_CALLBACK_PASS_ON;
   e_bindings_wheel_event_handle(E_BINDING_CONTEXT_WINLIST,
                                 E_OBJECT(_winlist->zone), ev);
   if (ev->z < 0) /* up */
     {
        for (i = ev->z; i < 0; i++)
          e_winlist_prev();
     }
   else if (ev->z > 0) /* down */
     {
        for (i = ev->z; i > 0; i--)
          e_winlist_next();
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_winlist_cb_mouse_move(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_Event_Mouse_Move *ev;

   ev = event;
   if (ev->window != _input_window) return ECORE_CALLBACK_PASS_ON;

   evas_event_feed_mouse_move(_winlist->evas, ev->x - _winlist->x +
                              _winlist->zone->x, ev->y - _winlist->y +
                              _winlist->zone->y, ev->timestamp, NULL);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_winlist_scroll_timer(void *data __UNUSED__)
{
   if (_scroll_to)
     {
        double spd;

        spd = e_config->winlist_scroll_speed;
        _scroll_align = (_scroll_align * (1.0 - spd)) +
          (_scroll_align_to * spd);
        return 1;
     }
   _scroll_timer = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_e_winlist_warp_timer(void *data __UNUSED__)
{
   if (_warp_to && _warp_timer)
     {
        double spd = e_config->winlist_warp_speed;

        _old_warp_x = _warp_x;
        _old_warp_y = _warp_y;
        _warp_x = (_warp_x * (1.0 - spd)) + (_warp_to_x * spd);
        _warp_y = (_warp_y * (1.0 - spd)) + (_warp_to_y * spd);
        return ECORE_CALLBACK_RENEW;
     }
   _warp_timer = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_e_winlist_animator(void *data __UNUSED__)
{
   if (_warp_to)
     {
        if (_warp_x == _old_warp_x && _warp_y == _old_warp_y)
          {
             _warp_x = _warp_to_x;
             _warp_y = _warp_to_y;
             _warp_to = 0;
          }
        if (_win) ecore_x_pointer_warp(*_win, _warp_x, _warp_y);
     }
   if (_scroll_to)
     {
        double da;

        da = _scroll_align - _scroll_align_to;
        if (da < 0.0) da = -da;
        if (da < 0.01)
          {
             _scroll_align = _scroll_align_to;
             _scroll_to = 0;
          }
        e_box_align_set(_list_object, 0.5, 1.0 - _scroll_align);
     }
   if ((_warp_to) || (_scroll_to)) return ECORE_CALLBACK_RENEW;
   if (_bd_next)
     {
        if (_bd_next->iconic)
          {
             if (!_bd_next->lock_user_iconify)
               e_border_uniconify(_bd_next);
          }
        if (_bd_next->shaded)
          {
             if (!_bd_next->lock_user_shade)
               e_border_unshade(_bd_next, _bd_next->shade.dir);
          }
        else if (_bd_next->desk)
          {
             if (!_bd_next->sticky) e_desk_show(_bd_next->desk);
          }
        if (!_bd_next->lock_user_stacking)
          e_border_raise(_bd_next);

        if (!_bd_next->lock_focus_out)
          {
             e_border_focus_set(_bd_next, 1, 1);
             e_border_focus_latest_set(_bd_next);
          }
        if ((e_config->focus_policy != E_FOCUS_CLICK) ||
            (e_config->winlist_warp_at_end) ||
            (e_config->winlist_warp_while_selecting))
          ecore_x_pointer_warp(_bd_next->zone->container->win,
                               _warp_to_x, _warp_to_y);
        _bd_next = NULL;
     }
   _animator = NULL;
   _win = NULL;
   return ECORE_CALLBACK_CANCEL;
}

#if 0
static void
_e_winlist_cb_item_mouse_in(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
   E_Winlist_Win *ww;
   E_Winlist_Win *lww;
   Eina_List *l;

   if (!(ww = data)) return;
   if (!_wins) return;
   EINA_LIST_FOREACH(_wins, l, lww)
     if (lww == ww) break;
   _e_winlist_deactivate();
   _win_selected = l;
   _e_winlist_show_active();
   _e_winlist_activate();
}

#endif
