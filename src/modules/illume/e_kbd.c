#include "e.h"
#include "e_kbd.h"
#include "e_mod_layout.h"
#include "e_cfg.h"

static void _e_kbd_layout_send(E_Kbd *kbd);

static Eina_List *handlers = NULL;
static Eina_List *kbds = NULL;
static Ecore_X_Atom atom_mb_im_invoker_command = 0;
static Ecore_X_Atom atom_mtp_im_invoker_command = 0;
static Eina_List *border_hooks = NULL;
static E_Border *focused_border = NULL;
static Ecore_X_Atom focused_vkbd_state = 0;
static E_Module *mod = NULL;

//#define ICONIFY_TO_HIDE

static Ecore_Job *_e_kbd_apply_all_job = NULL;
static void
_e_kbd_cb_apply_all_job(void *data)
{
   _e_mod_layout_apply_all();
   _e_kbd_apply_all_job = NULL;
}

static void
_e_kbd_apply_all_job_queue(void)
{
   if (_e_kbd_apply_all_job) ecore_job_del(_e_kbd_apply_all_job);
   _e_kbd_apply_all_job = ecore_job_add(_e_kbd_cb_apply_all_job, NULL);
}

static void
_e_kbd_apply_all_job_queue_end(void)
{
   if (_e_kbd_apply_all_job) ecore_job_del(_e_kbd_apply_all_job);
   _e_kbd_apply_all_job = NULL;
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
_e_kbd_border_show(E_Kbd *kbd, E_Border *bd)
{
   if (!bd) return;
   e_border_uniconify(bd);
   if (kbd->fullscreen)
     e_border_layer_set(kbd->border, 250);
   else
     e_border_layer_set(kbd->border, 100);
   e_border_show(bd);
   e_border_raise(bd);
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
     {
	e_border_fx_offset(kbd->border, 0, kbd->border->h - kbd->adjust);
     }
   if (t == kbd->len)
     {
	kbd->animator = NULL;
	if (!kbd->visible)
	  {
	     _e_kbd_border_hide(kbd->border);
	     kbd->actually_visible = kbd->visible;
	  }
	_e_kbd_apply_all_job_queue();
	_e_kbd_layout_send(kbd);
	return 0;
     }
   return 1;
}

static void
_e_kbd_free(E_Kbd *kbd)
{
   E_Border *bd;

   kbds = eina_list_remove(kbds, kbd);
   if (kbd->animator) ecore_animator_del(kbd->animator);
   if (kbd->delay_hide) ecore_timer_del(kbd->delay_hide);
// FIXME: thought right - on shutdoiwn, this might point to freed data
//   if (kbd->border) kbd->border->stolen = 0;
   EINA_LIST_FREE(kbd->waiting_borders, bd)
	bd->stolen = 0;
   free(kbd);
}

static void
_e_kbd_slide(E_Kbd *kbd, int visible, double len)
{
   _e_kbd_apply_all_job_queue();
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
_e_kbd_hide(E_Kbd *kbd)
{
   if (kbd->visible) return;
   if (illume_cfg->sliding.kbd.duration <= 0)
     {
	_e_kbd_border_hide(kbd->border);
	kbd->actually_visible = kbd->visible;
	_e_kbd_apply_all_job_queue();
	_e_kbd_layout_send(kbd);
     }
   else
     _e_kbd_slide(kbd, 0, (double)illume_cfg->sliding.kbd.duration / 1000.0);
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
	 && (bd->client.netwm.state.skip_pager)
	 )
       )
     return 1;

   return 0;
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
   if (!kbd->actually_visible)
     {
	e_border_fx_offset(kbd->border, 0, kbd->border->h);
	_e_kbd_layout_send(kbd);
     }
   kbd->h = kbd->border->h;
}

static E_Kbd * 
_e_kbd_by_border_get(E_Border *bd)
{
   Eina_List *l, *l2;
   E_Border *over;
   E_Kbd *kbd;
   
   if (!bd->stolen) return NULL;
   EINA_LIST_FOREACH(kbds, l, kbd)
     {
	if (kbd->border == bd) return kbd;
	EINA_LIST_FOREACH(kbd->waiting_borders, l2, over)
	  if (over == bd) return kbd;
     }
   return NULL;
}

static int
_e_kbd_cb_delayed_hide(void *data)
{
   E_Kbd *kbd;
   
   kbd = data;
   _e_kbd_hide(kbd);
   kbd->delay_hide = NULL;
   return 0;
}

static void
_e_kbd_all_enable(void)
{
   Eina_List *l;
	E_Kbd *kbd;
	
   EINA_LIST_FOREACH(kbds, l, kbd)
	e_kbd_enable(kbd);
}

static void
_e_kbd_all_disable(void)
{
   Eina_List *l;
	E_Kbd *kbd;
	
   EINA_LIST_FOREACH(kbds, l, kbd)
	e_kbd_disable(kbd);
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
_e_kbd_all_layout_set(E_Kbd_Layout layout)
{
   Eina_List *l;
	E_Kbd *kbd;
	
   EINA_LIST_FOREACH(kbds, l, kbd)
	e_kbd_layout_set(kbd, layout);
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
	if (kbd->visible) e_kbd_hide(kbd);
	else e_kbd_show(kbd);
}

static int
_e_kbd_cb_client_message(void *data, int type, void *event)
{
   Ecore_X_Event_Client_Message *ev;
   
   ev = event;
   if ((ev->win == ecore_x_window_root_first_get()) &&
       ((ev->message_type == atom_mb_im_invoker_command) ||
	(ev->message_type == atom_mtp_im_invoker_command)))
     {
	if      (ev->data.l[0] == 1) _e_kbd_all_show();
	else if (ev->data.l[0] == 2) _e_kbd_all_hide();
	else if (ev->data.l[0] == 3) _e_kbd_all_toggle();
     }
   return 1;
}

static int
_e_kbd_cb_border_add(void *data, int type, void *event)
{
   E_Event_Border_Add *ev;
   
   ev = event;
   // nothing - border hooks do this
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
   // if border is in a created kbd - unstore
   kbd = _e_kbd_by_border_get(ev->border);
   if (!kbd) return 1;
   if (kbd->border == ev->border)
     {
	kbd->border = NULL;
	if (kbd->waiting_borders)
	  {
	     E_Border *bd;
	     
	     bd = kbd->waiting_borders->data;
	     kbd->waiting_borders = eina_list_remove_list(kbd->waiting_borders, kbd->waiting_borders);
	     _e_kbd_border_adopt(kbd, bd);
	  }
	if (kbd->visible)
	  {
	     kbd->visible = 0;
	     _e_kbd_border_hide(kbd->border);
	     kbd->actually_visible = kbd->visible;
	     e_kbd_show(kbd);
	  }
	_e_kbd_apply_all_job_queue();
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
   // FIXME: if ev->border->client.vkbd.state == 0 then this app doesnt know
   // how to request for a virtual keyboard and so we should have a manual
   // override
   if ((ev->border->need_fullscreen) || (ev->border->fullscreen))
     e_kbd_fullscreen_set(ev->border->zone, 1);
   else e_kbd_fullscreen_set(ev->border->zone, 0);
   focused_border = ev->border;
   focused_vkbd_state = ev->border->client.vkbd.state;
   if (ev->border->client.vkbd.state == 0)
     return 1;
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
   if ((ev->border->need_fullscreen) || (ev->border->fullscreen))
     e_kbd_fullscreen_set(ev->border->zone, 1);
   else e_kbd_fullscreen_set(ev->border->zone, 0);
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
   /* nothing happened to vkbd prop - leave everything alone */
   if ((ev->border == focused_border) &&
       (ev->border->client.vkbd.state == focused_vkbd_state))
     return 1;
   focused_vkbd_state = ev->border->client.vkbd.state;
   /* app doesn't know what to do - just leave everything as-is */
   if ((ev->border->need_fullscreen) || (ev->border->fullscreen))
     e_kbd_fullscreen_set(ev->border->zone, 1);
   else e_kbd_fullscreen_set(ev->border->zone, 0);
   if (ev->border->client.vkbd.state == 0)
     return 1;
   /* app wats kbd off - then kbd off it is */
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

static void
_e_kbd_cb_border_hook_pre_post_fetch(void *data, E_Border *bd)
{
   // check if bd has special kbd properites - if so, store in created kbd
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
			    evas_stringshare_del(bd->bordername);
			    bd->bordername = NULL;
			    bd->client.border.changed = 1;
			 }
		       e_remember_unuse(bd->remember);
		       bd->remember = NULL;
		    }
		  if (bd->bordername) evas_stringshare_del(bd->bordername);
		  bd->bordername = evas_stringshare_add("borderless");
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
			    evas_stringshare_del(bd->bordername);
			    bd->bordername = NULL;
			    bd->client.border.changed = 1;
			 }
		       e_remember_unuse(bd->remember);
		       bd->remember = NULL;
		    }
		  if (bd->bordername) evas_stringshare_del(bd->bordername);
		  bd->bordername = evas_stringshare_add("borderless");
		  bd->client.border.changed = 1;
		  return;
	       }
	  }
     }
}

static void
_e_kbd_cb_border_hook_post_fetch(void *data, E_Border *bd)
{
   // nothing - all done in _e_kbd_cb_border_hook_pre_post_fetch()
   if (!_e_kbd_by_border_get(bd)) return;
}

static void
_e_kbd_cb_border_hook_post_border_assign(void *data, E_Border *bd)
{
   E_Kbd *kbd;
   int pbx, pby, pbw, pbh;
   
   kbd = _e_kbd_by_border_get(bd);
   if (!kbd) return;
   
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
				       bd->client.w,
				       bd->client.h);
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
	     else if (!kbd->actually_visible)
	       e_border_fx_offset(kbd->border, 0, kbd->border->h);
	     kbd->h = bd->h;
	  }
     }
}

static void
_e_kbd_cb_border_hook_end(void *data, E_Border *bd)
{
   E_Kbd *kbd;
   
   kbd = _e_kbd_by_border_get(bd);
   if (!kbd) return;
   if (kbd->border == bd)
     {
	if (!kbd->actually_visible)
	  _e_kbd_border_hide(kbd->border);
     }
   else
     _e_kbd_border_hide(bd);
}

static void
_e_kbd_layout_send(E_Kbd *kbd)
{
   Ecore_X_Virtual_Keyboard_State type;

   if ((kbd->actually_visible) && (!kbd->disabled))
     {
	type = ECORE_X_VIRTUAL_KEYBOARD_STATE_ON;
	if (kbd->layout == E_KBD_LAYOUT_DEFAULT) type = ECORE_X_VIRTUAL_KEYBOARD_STATE_ON;
	else if (kbd->layout == E_KBD_LAYOUT_ALPHA) type = ECORE_X_VIRTUAL_KEYBOARD_STATE_ALPHA;
	else if (kbd->layout == E_KBD_LAYOUT_NUMERIC) type = ECORE_X_VIRTUAL_KEYBOARD_STATE_NUMERIC;
	else if (kbd->layout == E_KBD_LAYOUT_PIN) type = ECORE_X_VIRTUAL_KEYBOARD_STATE_PIN;
	else if (kbd->layout == E_KBD_LAYOUT_PHONE_NUMBER) type = ECORE_X_VIRTUAL_KEYBOARD_STATE_PHONE_NUMBER;
	else if (kbd->layout == E_KBD_LAYOUT_HEX) type = ECORE_X_VIRTUAL_KEYBOARD_STATE_HEX;
	else if (kbd->layout == E_KBD_LAYOUT_TERMINAL) type = ECORE_X_VIRTUAL_KEYBOARD_STATE_TERMINAL;
	else if (kbd->layout == E_KBD_LAYOUT_PASSWORD) type = ECORE_X_VIRTUAL_KEYBOARD_STATE_PASSWORD;
	else if (kbd->layout == E_KBD_LAYOUT_NONE) type = ECORE_X_VIRTUAL_KEYBOARD_STATE_OFF;
     }
   else
     type = ECORE_X_VIRTUAL_KEYBOARD_STATE_OFF;
   if (kbd->border)
     ecore_x_e_virtual_keyboard_state_send(kbd->border->client.win, type);
}

// NOTES:
// 
// on freerunner these are always there:
// 
// /org/freedesktop/Hal/devices/platform_s3c2440_i2c_logicaldev_input
// /org/freedesktop/Hal/devices/platform_neo1973_button_0_logicaldev_input
// 
// on desktop (built-in/ps2):
// 
// /org/freedesktop/Hal/devices/platform_i8042_i8042_KBD_port_logicaldev_input
// 
// on desktop (usb):
// 
// /org/freedesktop/Hal/devices/usb_device_46d_c517_noserial_if0_logicaldev_input
// 
// this:
// 
// /org/freedesktop/Hal/devices/usb_device_a12_1_noserial_if0_bluetooth_hci_bluetooth_hci_logicaldev_input
// 
// gets added for a bt kbd - unknown for usb.
// 
// chances are i either need a way for a platform config to list devices
// known to be ebuilt-in keyboards but to be ignored when determining
// if a vkbd should be disabled or not... or... i need some other way

static E_DBus_Connection *_e_kbd_dbus_conn = NULL;
static E_DBus_Signal_Handler *_e_kbd_dbus_handler_dev_add = NULL;
static E_DBus_Signal_Handler *_e_kbd_dbus_handler_dev_del = NULL;
static E_DBus_Signal_Handler *_e_kbd_dbus_handler_dev_chg = NULL;

static Eina_List *_e_kbd_dbus_keyboards = NULL;
static int _e_kbd_dbus_have_real_keyboard = 0;
static Eina_List *_e_kbd_dbus_real_ignore = NULL;

static void
_e_kbd_dbus_keyboard_add(const char *udi)
{
   const char *str;
   Eina_List *l;
   
   EINA_LIST_FOREACH(_e_kbd_dbus_keyboards, l, str)
     if (!strcmp(str, udi)) return;

   _e_kbd_dbus_keyboards = eina_list_append
     (_e_kbd_dbus_keyboards, evas_stringshare_add(udi));
}

static void
_e_kbd_dbus_keyboard_del(const char *udi)
{
   Eina_List *l;
   char *str;
   
   EINA_LIST_FOREACH(_e_kbd_dbus_keyboards, l, str)
     if (!strcmp(str, udi))
     {
	  evas_stringshare_del(str);
	     _e_kbd_dbus_keyboards = eina_list_remove_list(_e_kbd_dbus_keyboards, l);
	     return;
	  }
}

static void
_e_kbd_dbus_keyboard_eval(void)
{
   int have_real = 0;
   Eina_List *l, *ll;
   const char *g, *gg;
   
   have_real = eina_list_count(_e_kbd_dbus_keyboards);
   EINA_LIST_FOREACH(_e_kbd_dbus_keyboards, l, g)
     EINA_LIST_FOREACH(_e_kbd_dbus_real_ignore, ll, gg)
       if (e_util_glob_match(g, gg))
	       {
		  have_real--;
		  break;
	       }

   if (have_real != _e_kbd_dbus_have_real_keyboard)
     {
	_e_kbd_dbus_have_real_keyboard = have_real;
	if (_e_kbd_dbus_have_real_keyboard)
	  _e_kbd_all_disable();
	else
	  _e_kbd_all_enable();
     }
}

static void
_e_kbd_dbus_cb_dev_input_keyboard(void *user_data, void *reply_data, DBusError *error)
{
   E_Hal_Manager_Find_Device_By_Capability_Return *ret = reply_data;
   Eina_List *l;
   char *device;
                                     
   if (!ret || !ret->strings) return;
   
   if (dbus_error_is_set(error))
     {
	dbus_error_free(error);
	return;
     }
   
   EINA_LIST_FOREACH(ret->strings, l, device)
     {
	_e_kbd_dbus_keyboard_add(device);
	_e_kbd_dbus_keyboard_eval();
     }
}

static void
_e_kbd_dbus_cb_input_keyboard_is(void *user_data, void *reply_data, DBusError *error)
{
   char *udi = user_data;
   E_Hal_Device_Query_Capability_Return *ret = reply_data;
   
   if (dbus_error_is_set(error))
     {
	dbus_error_free(error);
	goto error;
     }
   
   if (ret && ret->boolean)
     {
	_e_kbd_dbus_keyboard_add(udi);
	_e_kbd_dbus_keyboard_eval();
     }
   
   error:
   free(udi);
}

static void
_e_kbd_dbus_cb_dev_add(void *data, DBusMessage *msg)
{
   DBusError err;
   char *udi;
   int ret;
        
   dbus_error_init(&err);
   dbus_message_get_args(msg, &err, DBUS_TYPE_STRING, &udi, DBUS_TYPE_INVALID);
   udi = strdup(udi);
   ret = e_hal_device_query_capability(_e_kbd_dbus_conn, udi, "input.keyboard",
                                       _e_kbd_dbus_cb_input_keyboard_is, 
				       strdup(udi));
}
     
static void
_e_kbd_dbus_cb_dev_del(void *data, DBusMessage *msg)
{
   DBusError err;
   char *udi;
   
   dbus_error_init(&err);
   dbus_message_get_args(msg, &err, DBUS_TYPE_STRING, &udi, DBUS_TYPE_INVALID);
   _e_kbd_dbus_keyboard_del(udi);
   _e_kbd_dbus_keyboard_eval();
}

static void
_e_kbd_dbus_cb_cap_add(void *data, DBusMessage *msg)
{
   DBusError err;
   char *udi, *capability;
   
   dbus_error_init(&err);
   dbus_message_get_args(msg, &err, DBUS_TYPE_STRING, &udi, DBUS_TYPE_STRING,
                         &capability, DBUS_TYPE_INVALID);
   if (!strcmp(capability, "input.keyboard"))
     {
	_e_kbd_dbus_keyboard_add(udi);
	_e_kbd_dbus_keyboard_eval();
     }
}

static void
_e_kbd_dbus_ignore_keyboards_file_load(const char *file)
{
   char buf[4096];
   FILE *f;
   
   f = fopen(file, "r");
   if (!f) return;
   while (fgets(buf, sizeof(buf), f))
     {
	char *p;
	int len;
	
	if (buf[0] == '#') continue;
        len = strlen(buf);
	if (len > 0)
	  {
	     if (buf[len - 1] == '\n') buf[len - 1] = 0;
	  }
	p = buf;
	while (isspace(*p)) p++;
	if (*p)
	  _e_kbd_dbus_real_ignore = eina_list_append
	  (_e_kbd_dbus_real_ignore, evas_stringshare_add(p));
     }
   fclose(f);
}

static void
_e_kbd_dbus_ignore_keyboards_load(void)
{
   const char *homedir;
   char buf[PATH_MAX];
   
   homedir = e_user_homedir_get();
   snprintf(buf, sizeof(buf), "%s/.e/e/keyboards/ignore_built_in_keyboards", homedir);
   _e_kbd_dbus_ignore_keyboards_file_load(buf);
   snprintf(buf, sizeof(buf), "%s/keyboards/ignore_built_in_keyboards", e_module_dir_get(mod));
   _e_kbd_dbus_ignore_keyboards_file_load(buf);
}

static void
_e_kbd_dbus_real_kbd_init(void)
{
   _e_kbd_dbus_have_real_keyboard = 0;
   
   _e_kbd_dbus_ignore_keyboards_load();
      
   _e_kbd_dbus_conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
   if (_e_kbd_dbus_conn)
     {
	 
	e_hal_manager_find_device_by_capability(_e_kbd_dbus_conn, "input.keyboard",
						_e_kbd_dbus_cb_dev_input_keyboard, NULL);

	_e_kbd_dbus_handler_dev_add =
	  e_dbus_signal_handler_add(_e_kbd_dbus_conn, "org.freedesktop.Hal",
				    "/org/freedesktop/Hal/Manager",
				    "org.freedesktop.Hal.Manager",
				    "DeviceAdded", _e_kbd_dbus_cb_dev_add, NULL);
	_e_kbd_dbus_handler_dev_del =
	  e_dbus_signal_handler_add(_e_kbd_dbus_conn, "org.freedesktop.Hal",
				    "/org/freedesktop/Hal/Manager",
				    "org.freedesktop.Hal.Manager",
				    "DeviceRemoved", _e_kbd_dbus_cb_dev_del, NULL);
	_e_kbd_dbus_handler_dev_chg =
	  e_dbus_signal_handler_add(_e_kbd_dbus_conn, "org.freedesktop.Hal",
				    "/org/freedesktop/Hal/Manager",
				    "org.freedesktop.Hal.Manager",
				    "NewCapability", _e_kbd_dbus_cb_cap_add, NULL);
     }
}

static void
_e_kbd_dbus_real_kbd_shutdown(void)
{
   char *str;

   if (_e_kbd_dbus_conn)
     {
	e_dbus_signal_handler_del(_e_kbd_dbus_conn, _e_kbd_dbus_handler_dev_add);
	e_dbus_signal_handler_del(_e_kbd_dbus_conn, _e_kbd_dbus_handler_dev_del);
	e_dbus_signal_handler_del(_e_kbd_dbus_conn, _e_kbd_dbus_handler_dev_chg);
     }
   EINA_LIST_FREE(_e_kbd_dbus_real_ignore, str)
     evas_stringshare_del(str);
   EINA_LIST_FREE(_e_kbd_dbus_keyboards, str)
     evas_stringshare_del(str);
   _e_kbd_dbus_have_real_keyboard = 0;
}

EAPI int
e_kbd_init(E_Module *m)
{
   mod = m;
   focused_border = NULL;
   focused_vkbd_state = 0;
   atom_mb_im_invoker_command = ecore_x_atom_get("_MB_IM_INVOKER_COMMAND");
   atom_mtp_im_invoker_command = ecore_x_atom_get("_MTP_IM_INVOKER_COMMAND");
   handlers = eina_list_append(handlers, 
			       ecore_event_handler_add
			       (ECORE_X_EVENT_CLIENT_MESSAGE,
				_e_kbd_cb_client_message, NULL));
   handlers = eina_list_append(handlers, 
			       ecore_event_handler_add
			       (E_EVENT_BORDER_ADD,
				_e_kbd_cb_border_add, NULL));
   handlers = eina_list_append(handlers, 
			       ecore_event_handler_add
			       (E_EVENT_BORDER_REMOVE,
				_e_kbd_cb_border_remove, NULL));
   handlers = eina_list_append(handlers, 
			       ecore_event_handler_add
			       (E_EVENT_BORDER_FOCUS_IN,
				_e_kbd_cb_border_focus_in, NULL));
   handlers = eina_list_append(handlers, 
			       ecore_event_handler_add
			       (E_EVENT_BORDER_FOCUS_OUT,
				_e_kbd_cb_border_focus_out, NULL));
   handlers = eina_list_append(handlers, 
			       ecore_event_handler_add
			       (E_EVENT_BORDER_PROPERTY,
				_e_kbd_cb_border_property, NULL));
   border_hooks = eina_list_append(border_hooks,
				   e_border_hook_add
				   (E_BORDER_HOOK_EVAL_PRE_POST_FETCH,
				    _e_kbd_cb_border_hook_pre_post_fetch,
				    NULL));
   border_hooks = eina_list_append(border_hooks,
				   e_border_hook_add
				   (E_BORDER_HOOK_EVAL_POST_FETCH,
				    _e_kbd_cb_border_hook_post_fetch,
				    NULL));
   border_hooks = eina_list_append(border_hooks,
				   e_border_hook_add
				   (E_BORDER_HOOK_EVAL_POST_BORDER_ASSIGN,
				    _e_kbd_cb_border_hook_post_border_assign,
				    NULL));
   border_hooks = eina_list_append(border_hooks,
				   e_border_hook_add
				   (E_BORDER_HOOK_EVAL_END,
				    _e_kbd_cb_border_hook_end,
				    NULL));
   _e_kbd_dbus_real_kbd_init();
   return 1;
}

EAPI int
e_kbd_shutdown(void)
{
   E_Border_Hook *bh;
   Ecore_Event_Handler *handle;

   _e_kbd_apply_all_job_queue_end();
   _e_kbd_dbus_real_kbd_shutdown();
   EINA_LIST_FREE(border_hooks, bh)
     e_border_hook_del(bh);
   EINA_LIST_FREE(handlers, handle)
     ecore_event_handler_del(handle);
   focused_border = NULL;
   focused_vkbd_state = 0;
   mod = NULL;
   return 1;
}

EAPI E_Kbd *
e_kbd_new(E_Zone *zone, const char *themedir, const char *syskbds, const char *sysdicts)
{
   E_Kbd *kbd;
   
   kbd = E_OBJECT_ALLOC(E_Kbd, E_KBD_TYPE, _e_kbd_free);
   if (!kbd) return NULL;
   kbds = eina_list_append(kbds, kbd);
   kbd->layout = ECORE_X_VIRTUAL_KEYBOARD_STATE_ON;
   return kbd;
}

EAPI void
e_kbd_enable(E_Kbd *kbd)
{
   if (!kbd->disabled) return;
   kbd->disabled = 0;
   if (kbd->visible)
     {
	kbd->visible  = 0;
	e_kbd_show(kbd);
     }
}

EAPI void
e_kbd_disable(E_Kbd *kbd)
{
   if (kbd->disabled) return;
   if (kbd->visible) e_kbd_hide(kbd);
   kbd->disabled = 1;
}

EAPI void
e_kbd_show(E_Kbd *kbd)
{
   if (kbd->delay_hide)
     {
	ecore_timer_del(kbd->delay_hide);
	kbd->delay_hide = NULL;
     }
   if (kbd->visible) return;
   kbd->visible = 1;
   if (kbd->disabled) return;
   kbd->actually_visible = kbd->visible;
   _e_kbd_layout_send(kbd);
   if (illume_cfg->sliding.kbd.duration <= 0)
     {
	if (kbd->border)
	  {
	     e_border_fx_offset(kbd->border, 0, 0);
	     _e_kbd_border_show(kbd, kbd->border);
	  }
	kbd->actually_visible = kbd->visible;
	_e_kbd_apply_all_job_queue();
     }
   else
     {
        if (kbd->border)
	  {
	     e_border_fx_offset(kbd->border, 0, kbd->border->h - kbd->adjust);
	     _e_kbd_border_show(kbd, kbd->border);
	  }
	_e_kbd_slide(kbd, 1, (double)illume_cfg->sliding.kbd.duration / 1000.0);
     }
}

EAPI void
e_kbd_layout_set(E_Kbd *kbd, E_Kbd_Layout layout)
{
   kbd->layout = layout;
   _e_kbd_layout_send(kbd);
}

EAPI void
e_kbd_hide(E_Kbd *kbd)
{
   if (!kbd->visible) return;
   kbd->visible = 0;
   if (!kbd->delay_hide)
     kbd->delay_hide = ecore_timer_add(0.2, _e_kbd_cb_delayed_hide, kbd);
}

EAPI void
e_kbd_safe_app_region_get(E_Zone *zone, int *x, int *y, int *w, int *h)
{
   Eina_List *l;

   if (x) *x = zone->x;
   if (y) *y = zone->y;
   if (w) *w = zone->w;
   if (h) *h = zone->h;
   for (l = kbds; l; l = l->next)
     {
	E_Kbd *kbd;
	
	kbd = l->data;
	if ((kbd->border) && (kbd->border->zone == zone))
	  {
	     if ((kbd->visible) && (!kbd->animator) && (!kbd->disabled)) 
	       /* out finished, not disabled */
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
}

EAPI void
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
