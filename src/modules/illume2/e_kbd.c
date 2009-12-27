#include "e.h"
#include "e_kbd.h"
#include "e_kbd_dbus.h"
#include "e_mod_config.h"

#define ICONIFY_TO_HIDE 0

/* local function prototypes */
static void _e_kbd_cb_free(E_Kbd *kbd);
static int _e_kbd_cb_delayed_hide(void *data);
static int _e_kbd_cb_animate(void *data);
static E_Kbd *_e_kbd_by_border_get(E_Border *bd);
static void _e_kbd_layout_send(E_Kbd *kbd);
static void _e_kbd_border_show(E_Border *bd);
static void _e_kbd_border_hide(E_Border *bd);
static void _e_kbd_border_adopt(E_Kbd *kbd, E_Border *bd);
static int _e_kbd_border_is_keyboard(E_Border *bd);
static void _e_kbd_hide(E_Kbd *kbd);
static void _e_kbd_slide(E_Kbd *kbd, int visible, double len);
static void _e_kbd_all_show(void);
static void _e_kbd_all_hide(void);
static void _e_kbd_all_toggle(void);
static void _e_kbd_all_layout_set(E_Kbd_Layout layout);

/* handlers */
static int _e_kbd_cb_client_message(void *data, int type, void *event);
static int _e_kbd_cb_border_remove(void *data, int type, void *event);
static int _e_kbd_cb_border_focus_in(void *data, int type, void *event);
static int _e_kbd_cb_border_focus_out(void *data, int type, void *event);
static int _e_kbd_cb_border_property(void *data, int type, void *event);

/* hooks */
static void _e_kbd_cb_border_pre_post_fetch(void *data, void *data2);
static void _e_kbd_cb_border_post_border_assign(void *data, void *data2);
static void _e_kbd_cb_border_eval_end(void *data, void *data2);

/* local variables */
static Eina_List *kbds = NULL, *handlers = NULL, *hooks = NULL;
static E_Border *focused_border = NULL;
static Ecore_X_Atom focused_vkbd_state = 0;
const char *mod_dir = NULL;

/* public functions */
int 
e_kbd_init(E_Module *m) 
{
   mod_dir = eina_stringshare_add(m->dir);

   handlers = 
     eina_list_append(handlers, 
                      ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE, 
                                              _e_kbd_cb_client_message, NULL));
   handlers = 
     eina_list_append(handlers, 
                      ecore_event_handler_add(E_EVENT_BORDER_REMOVE, 
                                              _e_kbd_cb_border_remove, NULL));
   handlers = 
     eina_list_append(handlers, 
                      ecore_event_handler_add(E_EVENT_BORDER_FOCUS_IN, 
                                              _e_kbd_cb_border_focus_in, NULL));
   handlers = 
     eina_list_append(handlers, 
                      ecore_event_handler_add(E_EVENT_BORDER_FOCUS_OUT, 
                                              _e_kbd_cb_border_focus_out, NULL));
   handlers = 
     eina_list_append(handlers, 
                      ecore_event_handler_add(E_EVENT_BORDER_PROPERTY, 
                                              _e_kbd_cb_border_property, NULL));

   hooks = 
     eina_list_append(hooks, 
                      e_border_hook_add(E_BORDER_HOOK_EVAL_PRE_POST_FETCH, 
                                        _e_kbd_cb_border_pre_post_fetch, NULL));

   e_kbd_dbus_init();

   return 1;
}

int 
e_kbd_shutdown(void) 
{
   E_Kbd *kbd;
   Ecore_Event_Handler *handler;
   E_Border_Hook *hook;

   e_kbd_dbus_shutdown();

   EINA_LIST_FREE(handlers, handler)
     ecore_event_handler_del(handler);

   EINA_LIST_FREE(hooks, hook)
     e_border_hook_del(hook);

   EINA_LIST_FREE(kbds, kbd)
     e_object_del(E_OBJECT(kbd));

   if (mod_dir) eina_stringshare_del(mod_dir);
   mod_dir = NULL;

   return 1;
}

E_Kbd *
e_kbd_new(void) 
{
   E_Kbd *kbd;

   kbd = E_OBJECT_ALLOC(E_Kbd, E_KBD_TYPE, _e_kbd_cb_free);
   if (!kbd) return NULL;
   kbds = eina_list_append(kbds, kbd);
   kbd->layout = E_KBD_LAYOUT_ALPHA;
   return kbd;
}

void 
e_kbd_all_enable(void) 
{
   Eina_List *l;
   E_Kbd *kbd;

   EINA_LIST_FOREACH(kbds, l, kbd)
     e_kbd_enable(kbd);
}

void 
e_kbd_all_disable(void) 
{
   Eina_List *l;
   E_Kbd *kbd;

   EINA_LIST_FOREACH(kbds, l, kbd)
     e_kbd_disable(kbd);
}

void 
e_kbd_show(E_Kbd *kbd) 
{
   if (kbd->timer) ecore_timer_del(kbd->timer);
   kbd->timer = NULL;
   if ((kbd->visible) || (kbd->disabled)) return;
   _e_kbd_layout_send(kbd);
   if (il_cfg->sliding.kbd.duration <= 0) 
     {
        if (kbd->border) 
          {
             e_border_fx_offset(kbd->border, 0, 0);
             _e_kbd_border_show(kbd->border);
          }
        kbd->visible = 1;
     }
   else 
     {
        if (kbd->border) _e_kbd_border_show(kbd->border);
        _e_kbd_slide(kbd, 1, (double)il_cfg->sliding.kbd.duration / 1000.0);
     }
}

void 
e_kbd_hide(E_Kbd *kbd) 
{
   if (!kbd->visible) return;
   if (!kbd->timer)
     kbd->timer = ecore_timer_add(0.2, _e_kbd_cb_delayed_hide, kbd);
}

void 
e_kbd_enable(E_Kbd *kbd) 
{
   if (!kbd->disabled) return;
   kbd->disabled = 0;
   if (!kbd->visible) e_kbd_show(kbd);
}

void 
e_kbd_disable(E_Kbd *kbd) 
{
   if (kbd->disabled) return;
   if (kbd->visible) e_kbd_hide(kbd);
   kbd->disabled = 1;
}

void 
e_kbd_layout_set(E_Kbd *kbd, E_Kbd_Layout layout) 
{
   kbd->layout = layout;
   _e_kbd_layout_send(kbd);
}

void 
e_kbd_fullscreen_set(E_Zone *zone, int fullscreen) 
{
   Eina_List *l;
   E_Kbd *kbd;

   EINA_LIST_FOREACH(kbds, l, kbd)
     if ((!!fullscreen) != kbd->fullscreen) 
       {
          kbd->fullscreen = fullscreen;
          if (kbd->fullscreen)
            e_border_layer_set(kbd->border, 250);
          else
            e_border_layer_set(kbd->border, 100);
       }
}

void 
e_kbd_safe_app_region_get(E_Zone *zone, int *x, int *y, int *w, int *h) 
{
   Eina_List *l;
   E_Kbd *kbd;

   if (x) *x = zone->x;
   if (y) *y = zone->y;
   if (w) *w = zone->w;
   if (h) *h = zone->h;
   EINA_LIST_FOREACH(kbds, l, kbd) 
     {
        if (!kbd->border) continue;
        if (kbd->border->zone != zone) continue;
        if ((kbd->visible) && (!kbd->animator) && (!kbd->disabled)) 
          {
             if (h) 
               {
                  *h -= kbd->border->h;
                  if (*h < 0) *h = 0;
               }
          }
        return;
     }
}

/* local functions */
static void 
_e_kbd_cb_free(E_Kbd *kbd) 
{
   E_Border *bd;

   if (kbd->animator) ecore_animator_del(kbd->animator);
   kbd->animator = NULL;
   if (kbd->timer) ecore_timer_del(kbd->timer);
   kbd->timer = NULL;
   EINA_LIST_FREE(kbd->waiting_borders, bd)
     bd->stolen = 0;
   E_FREE(kbd);
}

static int 
_e_kbd_cb_delayed_hide(void *data) 
{
   E_Kbd *kbd;

   if (!(kbd = data)) return 0;
   _e_kbd_hide(kbd);
   kbd->timer = NULL;
   return 0;
}

static int 
_e_kbd_cb_animate(void *data) 
{
   E_Kbd *kbd;
   double t, v;

   kbd = data;
   t = ecore_loop_time_get() - kbd->start;
   if (t > kbd->len) t = kbd->len;
   if (kbd->len > 0.0)
     {
	v = t / kbd->len;
	v = 1.0 - v;
	v = v * v * v * v;
	v = 1.0 - v;
     }
   else
     {
	t = kbd->len;
	v = 1.0;
     }
   kbd->adjust = (kbd->adjust_end * v) + (kbd->adjust_start  * (1.0 - v));
   if (kbd->border)
     e_border_fx_offset(kbd->border, 0, kbd->border->h - kbd->adjust);
   if (t == kbd->len)
     {
	kbd->animator = NULL;
	if (kbd->visible) 
	  {
	     _e_kbd_border_hide(kbd->border);
	     kbd->visible = 0;
	  }
        else
          kbd->visible = 1;
	_e_kbd_layout_send(kbd);
	return 0;
     }
   return 1;
}

static E_Kbd *
_e_kbd_by_border_get(E_Border *bd) 
{
   Eina_List *l, *ll;
   E_Border *over;
   E_Kbd *kbd;

   if (!bd->stolen) return NULL;
   EINA_LIST_FOREACH(kbds, l, kbd) 
     {
        if (kbd->border == bd) return kbd;
        EINA_LIST_FOREACH(kbd->waiting_borders, ll, over)
          if (over == bd) return kbd;
     }
   return NULL;
}

static void
_e_kbd_layout_send(E_Kbd *kbd)
{
   Ecore_X_Virtual_Keyboard_State type;

   if ((kbd->visible) && (!kbd->disabled))
     {
	type = ECORE_X_VIRTUAL_KEYBOARD_STATE_ON;
	if (kbd->layout == E_KBD_LAYOUT_DEFAULT) 
          type = ECORE_X_VIRTUAL_KEYBOARD_STATE_ON;
	else if (kbd->layout == E_KBD_LAYOUT_ALPHA) 
          type = ECORE_X_VIRTUAL_KEYBOARD_STATE_ALPHA;
	else if (kbd->layout == E_KBD_LAYOUT_NUMERIC) 
          type = ECORE_X_VIRTUAL_KEYBOARD_STATE_NUMERIC;
	else if (kbd->layout == E_KBD_LAYOUT_PIN) 
          type = ECORE_X_VIRTUAL_KEYBOARD_STATE_PIN;
	else if (kbd->layout == E_KBD_LAYOUT_PHONE_NUMBER) 
          type = ECORE_X_VIRTUAL_KEYBOARD_STATE_PHONE_NUMBER;
	else if (kbd->layout == E_KBD_LAYOUT_HEX) 
          type = ECORE_X_VIRTUAL_KEYBOARD_STATE_HEX;
	else if (kbd->layout == E_KBD_LAYOUT_TERMINAL) 
          type = ECORE_X_VIRTUAL_KEYBOARD_STATE_TERMINAL;
	else if (kbd->layout == E_KBD_LAYOUT_PASSWORD) 
          type = ECORE_X_VIRTUAL_KEYBOARD_STATE_PASSWORD;
	else if (kbd->layout == E_KBD_LAYOUT_NONE) 
          type = ECORE_X_VIRTUAL_KEYBOARD_STATE_OFF;
     }
   else
     type = ECORE_X_VIRTUAL_KEYBOARD_STATE_OFF;
   if (kbd->border)
     ecore_x_e_virtual_keyboard_state_send(kbd->border->client.win, type);
}

static void 
_e_kbd_border_show(E_Border *bd) 
{
   if (!bd) return;
   e_border_uniconify(bd);
   e_border_show(bd);
   e_border_raise(bd);
}

static void 
_e_kbd_border_hide(E_Border *bd) 
{
   if (!bd) return;
#ifdef ICONIFY_TO_HIDE
   e_border_iconify(bd);
#else
   e_border_hide(bd, 2);
#endif
}

static void 
_e_kbd_border_adopt(E_Kbd *kbd, E_Border *bd) 
{
   kbd->border = bd;
   bd->sticky = 1;
   if (kbd->fullscreen)
     e_border_layer_set(kbd->border, 250);
   else
     e_border_layer_set(kbd->border, 100);
   if (!kbd->visible) 
     {
        e_border_fx_offset(kbd->border, 0, kbd->border->h);
        _e_kbd_layout_send(kbd);
     }
   kbd->h = kbd->border->h;
}

static int
_e_kbd_border_is_keyboard(E_Border *bd)
{
   if ((bd->client.vkbd.vkbd) || /* explicit hint that its a virtual keyboard */
       /* legacy */
       ( /* trap the matchbox qwerty and multitap kbd's */
	 (((bd->client.icccm.title) && (!strcmp(bd->client.icccm.title, "Keyboard"))) ||
	  ((bd->client.icccm.name) && ((!strcmp(bd->client.icccm.name, "multitap-pad")))))
	 && (bd->client.netwm.state.skip_taskbar)
	 && (bd->client.netwm.state.skip_pager)))
     return 1;
   return 0;
}

static void 
_e_kbd_hide(E_Kbd *kbd) 
{
   if (kbd->timer) ecore_timer_del(kbd->timer);
   kbd->timer = NULL;
   if ((!kbd->visible) || (kbd->disabled)) return;
   _e_kbd_layout_send(kbd);
   if (il_cfg->sliding.kbd.duration <= 0) 
     {
        if (kbd->border) 
          {
             e_border_fx_offset(kbd->border, 0, kbd->border->h);
             _e_kbd_border_hide(kbd->border);
          }
        kbd->visible = 0;
     }
   else 
     _e_kbd_slide(kbd, 0, (double)il_cfg->sliding.kbd.duration / 1000.0);
}

static void 
_e_kbd_slide(E_Kbd *kbd, int visible, double len) 
{
   kbd->start = ecore_loop_time_get();
   kbd->len = len;
   kbd->adjust_start = kbd->adjust;
   kbd->adjust_end = 0;
   if ((visible) && (kbd->border))
     kbd->adjust_end = kbd->border->h;
   if (!kbd->animator)
     kbd->animator = ecore_animator_add(_e_kbd_cb_animate, kbd);
}

static void 
_e_kbd_all_show(void) 
{
   Eina_List *l;
   E_Kbd *kbd;

   EINA_LIST_FOREACH(kbds, l, kbd)
     e_kbd_show(kbd);
}

static void 
_e_kbd_all_hide(void) 
{
   Eina_List *l;
   E_Kbd *kbd;

   EINA_LIST_FOREACH(kbds, l, kbd)
     e_kbd_hide(kbd);
}

static void 
_e_kbd_all_toggle(void) 
{
   Eina_List *l;
   E_Kbd *kbd;

   EINA_LIST_FOREACH(kbds, l, kbd) 
     {
        if (kbd->visible) e_kbd_hide(kbd);
        else e_kbd_show(kbd);
     }
}

static void 
_e_kbd_all_layout_set(E_Kbd_Layout layout) 
{
   Eina_List *l;
   E_Kbd *kbd;

   EINA_LIST_FOREACH(kbds, l, kbd) 
     e_kbd_layout_set(kbd, layout);
}

/* handlers */
static int 
_e_kbd_cb_client_message(void *data, int type, void *event) 
{
   Ecore_X_Event_Client_Message *ev;

   ev = event;
   if (ev->win != ecore_x_window_root_first_get()) return 1;
   if ((ev->message_type == ecore_x_atom_get("_MB_IM_INVOKER_COMMAND")) || 
       (ev->message_type == ecore_x_atom_get("_MTP_IM_INVOKER_COMMAND")))
     {
        if (ev->data.l[0] == 1) _e_kbd_all_show();
        else if (ev->data.l[0] == 2) _e_kbd_all_hide();
        else if (ev->data.l[0] == 3) _e_kbd_all_toggle();
     }
   return 1;
}

static int 
_e_kbd_cb_border_remove(void *data, int type, void *event) 
{
   E_Event_Border_Remove *ev;
   E_Kbd *kbd;

   ev = event;
   if (ev->border == focused_border) 
     {
        focused_border = NULL;
        focused_vkbd_state = 0;
        return 1;
     }
   if (!(kbd = _e_kbd_by_border_get(ev->border))) return 1;
   if (kbd->border == ev->border) 
     {
        kbd->border = NULL;
        if (kbd->waiting_borders) 
          {
             E_Border *bd;

             bd = kbd->waiting_borders->data;
             kbd->waiting_borders = 
               eina_list_remove_list(kbd->waiting_borders, kbd->waiting_borders);
             _e_kbd_border_adopt(kbd, bd);
          }
        if (kbd->visible) 
          {
             _e_kbd_border_hide(ev->border);
             e_kbd_hide(kbd);
          }
     }
   else
     kbd->waiting_borders = eina_list_remove(kbd->waiting_borders, ev->border);
   return 1;
}

static int 
_e_kbd_cb_border_focus_in(void *data, int type, void *event) 
{
   E_Event_Border_Focus_In *ev;

   ev = event;
   if (_e_kbd_by_border_get(ev->border)) return 1;
   focused_border = ev->border;
   focused_vkbd_state = ev->border->client.vkbd.state;
   if (focused_vkbd_state == 0) return 1;
   if (ev->border->client.vkbd.state == ECORE_X_VIRTUAL_KEYBOARD_STATE_OFF)
     {
	_e_kbd_all_layout_set(E_KBD_LAYOUT_NONE);
	_e_kbd_all_hide();
	return 1;
     }
   else if (ev->border->client.vkbd.state == ECORE_X_VIRTUAL_KEYBOARD_STATE_ALPHA)
     _e_kbd_all_layout_set(E_KBD_LAYOUT_ALPHA);
   else if (ev->border->client.vkbd.state == ECORE_X_VIRTUAL_KEYBOARD_STATE_NUMERIC)
     _e_kbd_all_layout_set(E_KBD_LAYOUT_NUMERIC);
   else if (ev->border->client.vkbd.state == ECORE_X_VIRTUAL_KEYBOARD_STATE_PIN)
     _e_kbd_all_layout_set(E_KBD_LAYOUT_PIN);
   else if (ev->border->client.vkbd.state == ECORE_X_VIRTUAL_KEYBOARD_STATE_PHONE_NUMBER)
     _e_kbd_all_layout_set(E_KBD_LAYOUT_PHONE_NUMBER);
   else if (ev->border->client.vkbd.state == ECORE_X_VIRTUAL_KEYBOARD_STATE_HEX)
     _e_kbd_all_layout_set(E_KBD_LAYOUT_HEX);
   else if (ev->border->client.vkbd.state == ECORE_X_VIRTUAL_KEYBOARD_STATE_TERMINAL)
     _e_kbd_all_layout_set(E_KBD_LAYOUT_TERMINAL);
   else if (ev->border->client.vkbd.state == ECORE_X_VIRTUAL_KEYBOARD_STATE_PASSWORD)
     _e_kbd_all_layout_set(E_KBD_LAYOUT_PASSWORD);
   else
     _e_kbd_all_layout_set(E_KBD_LAYOUT_DEFAULT);
   _e_kbd_all_show();
   return 1;
}

static int 
_e_kbd_cb_border_focus_out(void *data, int type, void *event) 
{
   E_Event_Border_Focus_Out *ev;

   ev = event;
   if (_e_kbd_by_border_get(ev->border)) return 1;
   _e_kbd_all_layout_set(E_KBD_LAYOUT_NONE);
   _e_kbd_all_hide();
   focused_border = NULL;
   focused_vkbd_state = 0;
   return 1;
}

static int 
_e_kbd_cb_border_property(void *data, int type, void *event) 
{
   E_Event_Border_Property *ev;

   ev = event;
   if (_e_kbd_by_border_get(ev->border)) return 1;
   if (!ev->border->focused) return 1;
   if ((ev->border == focused_border) && 
       (ev->border->client.vkbd.state == focused_vkbd_state)) return 1;
   focused_vkbd_state = ev->border->client.vkbd.state;
   if ((ev->border->need_fullscreen) || (ev->border->fullscreen))
     e_kbd_fullscreen_set(ev->border->zone, 1);
   else
     e_kbd_fullscreen_set(ev->border->zone, 0);
   if (ev->border->client.vkbd.state == 0) return 1;
   /* app wants kbd off - then kbd off it is */
   else if (ev->border->client.vkbd.state == ECORE_X_VIRTUAL_KEYBOARD_STATE_OFF)
     _e_kbd_all_hide();
   /* app wants something else than off... */
   else
     {
	if (ev->border->client.vkbd.state == ECORE_X_VIRTUAL_KEYBOARD_STATE_ALPHA)
	  _e_kbd_all_layout_set(E_KBD_LAYOUT_ALPHA);
	else if (ev->border->client.vkbd.state == ECORE_X_VIRTUAL_KEYBOARD_STATE_NUMERIC)
	  _e_kbd_all_layout_set(E_KBD_LAYOUT_NUMERIC);
	else if (ev->border->client.vkbd.state == ECORE_X_VIRTUAL_KEYBOARD_STATE_PIN)
	  _e_kbd_all_layout_set(E_KBD_LAYOUT_PIN);
	else if (ev->border->client.vkbd.state == ECORE_X_VIRTUAL_KEYBOARD_STATE_PHONE_NUMBER)
	  _e_kbd_all_layout_set(E_KBD_LAYOUT_PHONE_NUMBER);
	else if (ev->border->client.vkbd.state == ECORE_X_VIRTUAL_KEYBOARD_STATE_HEX)
	  _e_kbd_all_layout_set(E_KBD_LAYOUT_HEX);
	else if (ev->border->client.vkbd.state == ECORE_X_VIRTUAL_KEYBOARD_STATE_TERMINAL)
	  _e_kbd_all_layout_set(E_KBD_LAYOUT_TERMINAL);
	else if (ev->border->client.vkbd.state == ECORE_X_VIRTUAL_KEYBOARD_STATE_PASSWORD)
	  _e_kbd_all_layout_set(E_KBD_LAYOUT_PASSWORD);
	else
	  _e_kbd_all_layout_set(E_KBD_LAYOUT_DEFAULT);
	_e_kbd_all_show();
     }
   return 1;
}

/* hooks */
static void 
_e_kbd_cb_border_pre_post_fetch(void *data, void *data2) 
{
   E_Border *bd;

   if (!(bd = data2)) return;
   if (!bd->new_client) return;
   if (_e_kbd_by_border_get(bd)) return;
   if (_e_kbd_border_is_keyboard(bd))
     {
	Eina_List *l;
        E_Kbd *kbd;

	EINA_LIST_FOREACH(kbds, l, kbd)
          {
	     if (!kbd->border)
	       {
		  _e_kbd_border_adopt(kbd, bd);
		  bd->stolen = 1;
		  if (bd->remember)
		    {
		       if (bd->bordername)
			 {
			    eina_stringshare_del(bd->bordername);
			    bd->bordername = NULL;
			    bd->client.border.changed = 1;
			 }
		       e_remember_unuse(bd->remember);
		       bd->remember = NULL;
		    }
		  eina_stringshare_replace(&bd->bordername, "borderless");
		  bd->client.border.changed = 1;
		  return;
	       }
	     else
	       {
		  kbd->waiting_borders = eina_list_append(kbd->waiting_borders, bd);
		  bd->stolen = 1;
		  if (bd->remember)
		    {
		       if (bd->bordername)
			 {
			    eina_stringshare_del(bd->bordername);
			    bd->bordername = NULL;
			    bd->client.border.changed = 1;
			 }
		       e_remember_unuse(bd->remember);
		       bd->remember = NULL;
		    }
		  eina_stringshare_replace(&bd->bordername, "borderless");
		  bd->client.border.changed = 1;
		  return;
	       }
	  }
     }
}

static void 
_e_kbd_cb_border_post_border_assign(void *data, void *data2) 
{
   E_Border *bd;
   E_Kbd *kbd;
   int pbx, pby, pbw, pbh;

   if (!(bd = data2)) return;
   if (!(kbd = _e_kbd_by_border_get(bd))) return;
   pbx = bd->x;
   pby = bd->y;
   pbw = bd->w;
   pbh = bd->h;

   bd->lock_border = 1;

   bd->lock_client_location = 1;
   bd->lock_client_size = 1;
   bd->lock_client_desk = 1;
   bd->lock_client_sticky = 1;
   bd->lock_client_shade = 1;
   bd->lock_client_maximize = 1;

   bd->lock_user_location = 1;
   bd->lock_user_size = 1;
   bd->lock_user_desk = 1;
   bd->lock_user_sticky = 1;
   bd->lock_user_shade = 1;
   bd->lock_user_maximize = 1;

   bd->client.icccm.accepts_focus  = 0;
   bd->client.icccm.take_focus  = 0;

   bd->w = bd->zone->w;
   bd->h = bd->h; 
   bd->x = bd->zone->x;
   bd->y = bd->zone->y + bd->zone->h - bd->h;

   bd->client.w = bd->w - bd->client_inset.l - bd->client_inset.r;
   bd->client.h = bd->h - bd->client_inset.t - bd->client_inset.b;

   bd->changes.size = 1;
   bd->placed = 1;

   if ((pbx != bd->x) || (pby != bd->y)  ||
       (pbw != bd->w) || (pbh != bd->h))
     {
	if (bd->internal_ecore_evas)
	  {
	     ecore_evas_managed_move(bd->internal_ecore_evas,
				     bd->x + bd->fx.x + bd->client_inset.l,
				     bd->y + bd->fx.y + bd->client_inset.t);
	  }
	ecore_x_icccm_move_resize_send(bd->client.win,
				       bd->x + bd->fx.x + bd->client_inset.l,
				       bd->y + bd->fx.y + bd->client_inset.t,
				       bd->client.w, bd->client.h);
	bd->changed = 1;
	bd->changes.pos = 1;
	bd->changes.size = 1;
     }
   if (bd == kbd->border)
     {
	if (kbd->h != bd->h)
	  {
	     if (kbd->animator)
	       {
		  if (kbd->adjust_end > kbd->adjust_start)
		    {
		       kbd->adjust_start -= (bd->h - kbd->h);
		       kbd->adjust_end -= (bd->h - kbd->h);
		    }
	       }
	     else if (!kbd->visible)
	       e_border_fx_offset(kbd->border, 0, kbd->border->h);
	     kbd->h = bd->h;
	  }
     }
}

static void 
_e_kbd_cb_border_eval_end(void *data, void *data2) 
{
   E_Border *bd;
   E_Kbd *kbd;

   if (!(bd = data2)) return;
   if (!(kbd = _e_kbd_by_border_get(bd))) return;
   if (kbd->border == bd) 
     {
        if (!kbd->visible) _e_kbd_border_hide(kbd->border);
     }
   else
     _e_kbd_border_hide(bd);
}
