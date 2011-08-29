#include "e_illume_private.h"
#include "e_mod_kbd.h"
#include "e_mod_kbd_device.h"

/* local function prototypes */
static Eina_Bool _e_mod_kbd_cb_client_message(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _e_mod_kbd_cb_border_remove(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _e_mod_kbd_cb_border_focus_in(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _e_mod_kbd_cb_border_focus_out(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _e_mod_kbd_cb_border_property(void *data __UNUSED__, int type __UNUSED__, void *event);
static void _e_mod_kbd_cb_border_pre_post_fetch(void *data __UNUSED__, void *data2);
static void _e_mod_kbd_cb_free(E_Illume_Keyboard *kbd);
static Eina_Bool _e_mod_kbd_cb_delay_hide(void *data __UNUSED__);
static void _e_mod_kbd_hide(void);
static void _e_mod_kbd_slide(int visible, double len);
static Eina_Bool _e_mod_kbd_cb_animate(void *data __UNUSED__);
static E_Illume_Keyboard *_e_mod_kbd_by_border_get(E_Border *bd);
static void _e_mod_kbd_border_adopt(E_Border *bd);
static void _e_mod_kbd_layout_send(void);
static void _e_mod_kbd_geometry_send(void);
static void _e_mod_kbd_changes_send(void);

/* local variables */
static Eina_List *_kbd_hdls = NULL;
static E_Border_Hook *_kbd_hook = NULL;
static Ecore_X_Atom _focused_state = 0;
static E_Border *_focused_border = NULL, *_prev_focused_border = NULL;

int 
e_mod_kbd_init(void) 
{
   /* add handlers for events we are interested in */
   _kbd_hdls = 
     eina_list_append(_kbd_hdls, 
                      ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE, 
                                              _e_mod_kbd_cb_client_message, 
                                              NULL));
   _kbd_hdls = 
     eina_list_append(_kbd_hdls, 
                      ecore_event_handler_add(E_EVENT_BORDER_REMOVE, 
                                              _e_mod_kbd_cb_border_remove, 
                                              NULL));
   _kbd_hdls = 
     eina_list_append(_kbd_hdls, 
                      ecore_event_handler_add(E_EVENT_BORDER_FOCUS_IN, 
                                              _e_mod_kbd_cb_border_focus_in, 
                                              NULL));
   _kbd_hdls = 
     eina_list_append(_kbd_hdls, 
                      ecore_event_handler_add(E_EVENT_BORDER_FOCUS_OUT, 
                                              _e_mod_kbd_cb_border_focus_out, 
                                              NULL));
   _kbd_hdls = 
     eina_list_append(_kbd_hdls, 
                      ecore_event_handler_add(ECORE_X_EVENT_WINDOW_PROPERTY, 
                                              _e_mod_kbd_cb_border_property, 
                                              NULL));

   /* add hooks for events we are interested in */
   _kbd_hook = e_border_hook_add(E_BORDER_HOOK_EVAL_PRE_POST_FETCH, 
                                 _e_mod_kbd_cb_border_pre_post_fetch, NULL);

   /* initialize the device subsystem */
   e_mod_kbd_device_init();

   return 1;
}

int 
e_mod_kbd_shutdown(void) 
{
   Ecore_Event_Handler *hdl;

   /* shutdown the device subsystem */
   e_mod_kbd_device_shutdown();

   /* destroy the hook */
   e_border_hook_del(_kbd_hook);

   /* destroy the handlers and free the list */
   EINA_LIST_FREE(_kbd_hdls, hdl)
     ecore_event_handler_del(hdl);

   return 1;
}

E_Illume_Keyboard *
e_mod_kbd_new(void) 
{
   E_Illume_Keyboard *kbd;

   /* try to allocate our new keyboard object */
   kbd = E_OBJECT_ALLOC(E_Illume_Keyboard, E_ILLUME_KBD_TYPE, 
                        _e_mod_kbd_cb_free);
   if (!kbd) return NULL;

   /* set default layout on new keyboard */
   kbd->layout = E_ILLUME_KEYBOARD_LAYOUT_DEFAULT;
   kbd->visible = 0;

   return kbd;
}

void 
e_mod_kbd_enable(void) 
{
   /* don't try to enable a keyboard that is already enabled */
   if (!_e_illume_kbd->disabled) return;

   /* set keyboard to enabled */
   _e_illume_kbd->disabled = 0;

   /* show it if we need to */
   if (!_e_illume_kbd->visible) e_mod_kbd_show();
}

void 
e_mod_kbd_disable(void) 
{
   /* don't try to disable a keyboard that is already disabled */
   if (_e_illume_kbd->disabled) return;

   /* hide it if we need to */
   if (_e_illume_kbd->visible) e_mod_kbd_hide();

   /* set keyboard to disabled */
   _e_illume_kbd->disabled = 1;
}

void 
e_mod_kbd_show(void) 
{
   /* destroy existing timer */
   if (_e_illume_kbd->timer) ecore_timer_del(_e_illume_kbd->timer);
   _e_illume_kbd->timer = NULL;

   /* destroy the animator if it exists */
   if (_e_illume_kbd->animator) ecore_animator_del(_e_illume_kbd->animator);
   _e_illume_kbd->animator = NULL;

   if ((_focused_border) && (_e_illume_kbd->border))
     {
        if (_e_illume_kbd->border->zone != _focused_border->zone) 
          e_border_zone_set(_e_illume_kbd->border, _focused_border->zone);
     }

   /* if it's disabled, get out */
   if (_e_illume_kbd->disabled) return;

//   _e_mod_kbd_layout_send();

   /* if we are not animating, just show it */
   if (_e_illume_cfg->animation.vkbd.duration <= 0) 
     {
        /* show the border */
        if (_e_illume_kbd->border) 
          {
             e_border_fx_offset(_e_illume_kbd->border, 0, 0);
             if (!_e_illume_kbd->border->visible) 
               e_border_show(_e_illume_kbd->border);
             e_border_raise(_e_illume_kbd->border);
          }
        _e_illume_kbd->visible = 1;

        _e_mod_kbd_geometry_send();

        _e_mod_kbd_changes_send();
     }
   else 
     {
        /* show the border */
        if (_e_illume_kbd->border) 
          {
             if (!_e_illume_kbd->border->visible) 
               e_border_show(_e_illume_kbd->border);
             e_border_raise(_e_illume_kbd->border);
          }

        /* animate it */
        _e_mod_kbd_slide(1, (double)_e_illume_cfg->animation.vkbd.duration / 1000.0);
     }
}

void 
e_mod_kbd_hide(void) 
{
   /* cannot hide keyboard that is not visible */
//   if (!_e_illume_kbd->visible) return;

   /* create new hide timer if it doesn't exist */
   if (_e_illume_kbd->disabled) return;
   
   _e_illume_kbd->visible = 0;

   if (!_e_illume_kbd->timer) 
     _e_illume_kbd->timer = ecore_timer_add(0.2, _e_mod_kbd_cb_delay_hide, NULL);
}

void 
e_mod_kbd_toggle(void) 
{
   if (_e_illume_kbd->visible) e_mod_kbd_hide();
   else e_mod_kbd_show();
}

void 
e_mod_kbd_fullscreen_set(E_Zone *zone, int fullscreen) 
{
   if (!_e_illume_kbd->border) return;
   if (_e_illume_kbd->border->zone != zone) return;
   if ((!!fullscreen) != _e_illume_kbd->fullscreen) 
     _e_illume_kbd->fullscreen = fullscreen;
}

void 
e_mod_kbd_layout_set(E_Illume_Keyboard_Layout layout) 
{
   if (!_e_illume_kbd->border) return;
   _e_illume_kbd->layout = layout;
   _e_mod_kbd_layout_send();
}

/* local functions */
static Eina_Bool
_e_mod_kbd_cb_client_message(void *data __UNUSED__, int type __UNUSED__, void *event) 
{
   Ecore_X_Event_Client_Message *ev;

   ev = event;
   if (ev->win != ecore_x_window_root_first_get()) 
     return ECORE_CALLBACK_PASS_ON;

   /* legacy illume 1 code */
   if ((ev->message_type == ecore_x_atom_get("_MB_IM_INVOKER_COMMAND")) || 
       (ev->message_type == ecore_x_atom_get("_MTP_IM_INVOKER_COMMAND"))) 
     {
        if (ev->data.l[0] == 1) e_mod_kbd_show();
        else if (ev->data.l[0] == 2) e_mod_kbd_hide();
        else if (ev->data.l[0] == 3) e_mod_kbd_toggle();
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_kbd_cb_border_remove(void *data __UNUSED__, int type __UNUSED__, void *event) 
{
   E_Event_Border_Remove *ev;
   E_Illume_Keyboard *kbd;

   ev = event;

   /* if we removed the focused border, reset some variables */
   if ((_prev_focused_border) && (_prev_focused_border == ev->border)) 
      _prev_focused_border = NULL;
   if ((_focused_border) && (_focused_border == ev->border)) 
     {
        e_mod_kbd_hide();
        _focused_border = NULL;
        _focused_state = 0;
        return ECORE_CALLBACK_PASS_ON;
     }

   /* try to find the keyboard for this border */
   if (!(kbd = _e_mod_kbd_by_border_get(ev->border))) 
     return ECORE_CALLBACK_PASS_ON;

   if ((kbd->border) && (kbd->border == ev->border)) 
     {
        kbd->border = NULL;
        if (kbd->waiting_borders) 
          {
             E_Border *bd;

             bd = kbd->waiting_borders->data;
             kbd->waiting_borders = 
               eina_list_remove_list(kbd->waiting_borders, kbd->waiting_borders);

             _e_mod_kbd_border_adopt(bd);
          }
        if (kbd->visible) 
          {
             e_border_hide(ev->border, 2);
             e_mod_kbd_hide();
          }
     }
   else if (!kbd->border) 
     kbd->waiting_borders = eina_list_remove(kbd->waiting_borders, ev->border);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_kbd_cb_border_focus_in(void *data __UNUSED__, int type __UNUSED__, void *event) 
{
   E_Event_Border_Focus_In *ev;

   ev = event;
   if (_e_mod_kbd_by_border_get(ev->border)) return ECORE_CALLBACK_PASS_ON;

   /* set focused border for kbd */
   _focused_border = ev->border;
   _focused_state = ev->border->client.vkbd.state;

   if (_focused_state <= ECORE_X_VIRTUAL_KEYBOARD_STATE_OFF) 
     e_mod_kbd_hide();
   else 
     e_mod_kbd_show();

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_kbd_cb_border_focus_out(void *data __UNUSED__, int type __UNUSED__, void *event) 
{
   E_Event_Border_Focus_Out *ev;

   ev = event;
   if (_e_mod_kbd_by_border_get(ev->border)) return ECORE_CALLBACK_PASS_ON;

   _prev_focused_border = _focused_border;

   /* hide the keyboard */
   e_mod_kbd_hide();

   /* tell the focused border it changed so layout gets udpated */
   if (_prev_focused_border) 
     {
        if (!e_illume_border_is_conformant(_prev_focused_border)) 
          {
             _prev_focused_border->changes.size = 1;
             _prev_focused_border->changed = 1;
          }
     }

   /* reset some variables */
   _focused_border = NULL;
   _focused_state = 0;

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_kbd_cb_border_property(void *data __UNUSED__, int type __UNUSED__, void *event) 
{
   Ecore_X_Event_Window_Property *ev;
   E_Border *bd;
   int fullscreen = 0;

   ev = event;

   /* only interested in vkbd state changes here */
   if (ev->atom != ECORE_X_ATOM_E_VIRTUAL_KEYBOARD_STATE) 
     return ECORE_CALLBACK_PASS_ON;

   /* make sure we have a border */
   if (!(bd = e_border_find_by_client_window(ev->win))) 
     return ECORE_CALLBACK_PASS_ON;

//   printf("Kbd Border Property Change: %s\n", bd->client.icccm.name);

   /* if it's not focused, we don't care */
   if ((!bd->focused) || (_e_mod_kbd_by_border_get(bd))) 
     return ECORE_CALLBACK_PASS_ON;

   /* NB: Not sure why, but we seem to need to fetch kbd state here. This could 
    * be a result of filtering the container_hook_layout. Not real happy 
    * with this because it is an X round-trip, but it is here because this 
    * needs more time to investigate. */
   e_hints_window_virtual_keyboard_state_get(bd);

   if ((_focused_border) && (_focused_border == bd)) 
     {
        /* if focused state is the same, get out */
        if (_focused_state == bd->client.vkbd.state) 
          return ECORE_CALLBACK_PASS_ON;
     }

   /* set our variables */
   _focused_border = bd;
   _focused_state = bd->client.vkbd.state;

   /* handle a border needing fullscreen keyboard */
   if ((bd->need_fullscreen) || (bd->fullscreen)) fullscreen = 1;
   if (_e_illume_kbd->fullscreen != fullscreen)
     e_mod_kbd_fullscreen_set(bd->zone, fullscreen);

   if (_focused_state <= ECORE_X_VIRTUAL_KEYBOARD_STATE_OFF) 
     e_mod_kbd_hide();
   else 
     e_mod_kbd_show();

   return ECORE_CALLBACK_PASS_ON;
}

static void 
_e_mod_kbd_cb_border_pre_post_fetch(void *data __UNUSED__, void *data2) 
{
   E_Border *bd;

   if (!(bd = data2)) return;
   if (!bd->new_client) return;
   if (_e_mod_kbd_by_border_get(bd)) return;
   if (e_illume_border_is_keyboard(bd)) 
     {
        if (!_e_illume_kbd->border) 
          _e_mod_kbd_border_adopt(bd);
        else 
          {
             _e_illume_kbd->waiting_borders = 
               eina_list_append(_e_illume_kbd->waiting_borders, bd);
          }
        bd->stolen = 1;
     }
}

static void 
_e_mod_kbd_cb_free(E_Illume_Keyboard *kbd) 
{
   E_Border *bd;

   /* destroy the animator if it exists */
   if (kbd->animator) ecore_animator_del(kbd->animator);
   kbd->animator = NULL;

   /* destroy the timer if it exists */
   if (kbd->timer) ecore_timer_del(kbd->timer);
   kbd->timer = NULL;

   /* free the list of waiting borders */
   EINA_LIST_FREE(kbd->waiting_borders, bd)
     bd->stolen = 0;

   /* free the keyboard structure */
   E_FREE(kbd);
}

static Eina_Bool
_e_mod_kbd_cb_delay_hide(void *data __UNUSED__) 
{
   _e_mod_kbd_hide();
   return ECORE_CALLBACK_CANCEL;
}

static void 
_e_mod_kbd_hide(void) 
{
   /* destroy existing timer */
   if (_e_illume_kbd->timer) ecore_timer_del(_e_illume_kbd->timer);
   _e_illume_kbd->timer = NULL;

   /* destroy the animator if it exists */
   if (_e_illume_kbd->animator) ecore_animator_del(_e_illume_kbd->animator);
   _e_illume_kbd->animator = NULL;

   /* can't hide keyboard if it's disabled */
   if (_e_illume_kbd->disabled) return;

//   _e_mod_kbd_layout_send();

   /* if we are not animating, just hide it */
   if (_e_illume_cfg->animation.vkbd.duration <= 0) 
     {
        if (_e_illume_kbd->border) 
          {
             e_border_fx_offset(_e_illume_kbd->border, 0, 
                                _e_illume_kbd->border->h);
             e_border_hide(_e_illume_kbd->border, 2);
          }
     }
   else  
     _e_mod_kbd_slide(0, (double)_e_illume_cfg->animation.vkbd.duration / 1000.0);

   if (_e_illume_cfg->animation.vkbd.resize_before)
     {
	_e_mod_kbd_geometry_send();
	_e_mod_kbd_changes_send();
     }
}

static void 
_e_mod_kbd_slide(int visible, double len) 
{
   _e_illume_kbd->start = ecore_loop_time_get();
   _e_illume_kbd->len = len;
   _e_illume_kbd->adjust_start = _e_illume_kbd->adjust;
   _e_illume_kbd->adjust_end = 0;
   if ((visible) && (_e_illume_kbd->border)) 
     _e_illume_kbd->adjust_end = _e_illume_kbd->border->h;
   if (!_e_illume_kbd->animator) 
     _e_illume_kbd->animator = ecore_animator_add(_e_mod_kbd_cb_animate, NULL);
}

static Eina_Bool
_e_mod_kbd_cb_animate(void *data __UNUSED__) 
{
   double t, v;

   t = (ecore_loop_time_get() - _e_illume_kbd->start);
   if (t > _e_illume_kbd->len) t = _e_illume_kbd->len;
   if (_e_illume_kbd->len > 0.0) 
     {
        v = (t / _e_illume_kbd->len);
        v = (1.0 - v);
        v = (v * v * v * v);
        v = (1.0 - v);
     }
   else 
     {
        t = _e_illume_kbd->len;
        v = 1.0;
     }
   _e_illume_kbd->adjust = ((_e_illume_kbd->adjust_end * v) + 
                            (_e_illume_kbd->adjust_start * (1.0 - v)));

   if (_e_illume_kbd->border) 
     e_border_fx_offset(_e_illume_kbd->border, 0, 
                        (_e_illume_kbd->border->h - _e_illume_kbd->adjust));

   if (t == _e_illume_kbd->len) 
     {
        _e_illume_kbd->animator = NULL;
        if (_focused_state <= ECORE_X_VIRTUAL_KEYBOARD_STATE_OFF)
          {
             if (_e_illume_kbd->border) 
               e_border_hide(_e_illume_kbd->border, 2);
             _e_illume_kbd->visible = 0;
          }
        else 
          _e_illume_kbd->visible = 1;

        _e_mod_kbd_geometry_send();
        _e_mod_kbd_changes_send();

        return ECORE_CALLBACK_CANCEL;
     }

   return ECORE_CALLBACK_RENEW;
}

static E_Illume_Keyboard *
_e_mod_kbd_by_border_get(E_Border *bd) 
{
   Eina_List *l;
   E_Border *over;

   if ((!bd) || (!bd->stolen)) return NULL;

   /* if this border is the one that vkbd is working with, return the kbd */
   if (_e_illume_kbd->border == bd) return _e_illume_kbd;

   /* loop the waiting borders */
   EINA_LIST_FOREACH(_e_illume_kbd->waiting_borders, l, over)
     if (over == bd) return _e_illume_kbd;

   return NULL;
}

static void 
_e_mod_kbd_border_adopt(E_Border *bd) 
{
   if ((!_e_illume_kbd) || (!bd)) return;

   _e_illume_kbd->border = bd;

   if (!_e_illume_kbd->visible) 
     {
        e_border_fx_offset(bd, 0, bd->h);
        _e_mod_kbd_layout_send();
     }
}

static void 
_e_mod_kbd_layout_send(void) 
{
   Ecore_X_Virtual_Keyboard_State type;

   type = ECORE_X_VIRTUAL_KEYBOARD_STATE_OFF;
   if ((!_e_illume_kbd->visible) && (!_e_illume_kbd->disabled)) 
     {
        type = ECORE_X_VIRTUAL_KEYBOARD_STATE_ON;
        if (_e_illume_kbd->layout == E_ILLUME_KEYBOARD_LAYOUT_DEFAULT) 
          type = ECORE_X_VIRTUAL_KEYBOARD_STATE_ON;
        else if (_e_illume_kbd->layout == E_ILLUME_KEYBOARD_LAYOUT_ALPHA) 
          type = ECORE_X_VIRTUAL_KEYBOARD_STATE_ALPHA;
        else if (_e_illume_kbd->layout == E_ILLUME_KEYBOARD_LAYOUT_NUMERIC) 
          type = ECORE_X_VIRTUAL_KEYBOARD_STATE_NUMERIC;
        else if (_e_illume_kbd->layout == E_ILLUME_KEYBOARD_LAYOUT_PIN) 
          type = ECORE_X_VIRTUAL_KEYBOARD_STATE_PIN;
        else if (_e_illume_kbd->layout == E_ILLUME_KEYBOARD_LAYOUT_PHONE_NUMBER) 
          type = ECORE_X_VIRTUAL_KEYBOARD_STATE_PHONE_NUMBER;
        else if (_e_illume_kbd->layout == E_ILLUME_KEYBOARD_LAYOUT_HEX) 
          type = ECORE_X_VIRTUAL_KEYBOARD_STATE_HEX;
        else if (_e_illume_kbd->layout == E_ILLUME_KEYBOARD_LAYOUT_TERMINAL) 
          type = ECORE_X_VIRTUAL_KEYBOARD_STATE_TERMINAL;
        else if (_e_illume_kbd->layout == E_ILLUME_KEYBOARD_LAYOUT_PASSWORD) 
          type = ECORE_X_VIRTUAL_KEYBOARD_STATE_PASSWORD;
        else if (_e_illume_kbd->layout == E_ILLUME_KEYBOARD_LAYOUT_IP) 
          type = ECORE_X_VIRTUAL_KEYBOARD_STATE_IP;
        else if (_e_illume_kbd->layout == E_ILLUME_KEYBOARD_LAYOUT_HOST) 
          type = ECORE_X_VIRTUAL_KEYBOARD_STATE_HOST;
        else if (_e_illume_kbd->layout == E_ILLUME_KEYBOARD_LAYOUT_FILE) 
          type = ECORE_X_VIRTUAL_KEYBOARD_STATE_FILE;
        else if (_e_illume_kbd->layout == E_ILLUME_KEYBOARD_LAYOUT_URL) 
          type = ECORE_X_VIRTUAL_KEYBOARD_STATE_URL;
        else if (_e_illume_kbd->layout == E_ILLUME_KEYBOARD_LAYOUT_KEYPAD) 
          type = ECORE_X_VIRTUAL_KEYBOARD_STATE_KEYPAD;
        else if (_e_illume_kbd->layout == E_ILLUME_KEYBOARD_LAYOUT_J2ME) 
          type = ECORE_X_VIRTUAL_KEYBOARD_STATE_J2ME;
        else if (_e_illume_kbd->layout == E_ILLUME_KEYBOARD_LAYOUT_NONE) 
          type = ECORE_X_VIRTUAL_KEYBOARD_STATE_OFF;
     }
   if (_e_illume_kbd->border) 
     ecore_x_e_virtual_keyboard_state_send(_e_illume_kbd->border->client.win, type);
}

static void 
_e_mod_kbd_geometry_send(void) 
{
   E_Zone *zone;
   int y = 0;

   /* make sure we have a keyboard border */
   if (!_e_illume_kbd->border) return;

   /* no need. we send geometry only when kbd is visible or invisible */
   /* adjust Y for keyboard visibility */
   //if (_e_illume_kbd->border->fx.y <= 0) 
   y = _e_illume_kbd->border->y;

   if (_focused_border) zone = _focused_border->zone;
   else zone = _e_illume_kbd->border->zone;

   if (_e_illume_kbd->visible)
     ecore_x_e_illume_keyboard_geometry_set(zone->black_win, 
					    _e_illume_kbd->border->x,
					    y, 
					    _e_illume_kbd->border->w, 
					    _e_illume_kbd->border->h);
   else
     ecore_x_e_illume_keyboard_geometry_set(zone->black_win, 
					    _e_illume_kbd->border->x,
					    _e_illume_kbd->border->h + y, 
					    _e_illume_kbd->border->w, 
					    _e_illume_kbd->border->h);     
}

static void 
_e_mod_kbd_changes_send(void) 
{
   if (((_prev_focused_border) && (_focused_border)) && 
       (_prev_focused_border != _focused_border))
     {
        /* tell previous focused border it changed so layout udpates */
        if (_prev_focused_border->client.vkbd.state > 
            ECORE_X_VIRTUAL_KEYBOARD_STATE_UNKNOWN)
          {
             if (!e_illume_border_is_conformant(_prev_focused_border)) 
               {
                  _prev_focused_border->changes.size = 1;
                  _prev_focused_border->changed = 1;
               }
          }
     }

   /* tell the focused border it changed so layout gets udpated */
   if ((_focused_border) && 
       (_focused_border->client.vkbd.state > 
           ECORE_X_VIRTUAL_KEYBOARD_STATE_UNKNOWN))
     {
        if (!e_illume_border_is_conformant(_focused_border)) 
          {
             _focused_border->changes.size = 1;
             _focused_border->changed = 1;
          }
     }
}
