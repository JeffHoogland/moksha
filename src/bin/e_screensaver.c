#include "e.h"

static Ecore_Event_Handler *_e_screensaver_handler_on = NULL;
static Ecore_Event_Handler *_e_screensaver_handler_off = NULL;
static Ecore_Event_Handler *_e_screensaver_handler_config_mode = NULL;
static Ecore_Event_Handler *_e_screensaver_handler_screensaver_notify = NULL;
static Ecore_Event_Handler *_e_screensaver_handler_border_fullscreen = NULL;
static Ecore_Event_Handler *_e_screensaver_handler_border_unfullscreen = NULL;
static Ecore_Event_Handler *_e_screensaver_handler_border_remove = NULL;
static Ecore_Event_Handler *_e_screensaver_handler_border_iconify = NULL;
static Ecore_Event_Handler *_e_screensaver_handler_border_uniconify = NULL;
static Ecore_Event_Handler *_e_screensaver_handler_border_desk_set = NULL;
static Ecore_Event_Handler *_e_screensaver_handler_desk_show = NULL;
static Ecore_Event_Handler *_e_screensaver_handler_powersave = NULL;
static E_Dialog *_e_screensaver_ask_presentation_dia = NULL;
static int _e_screensaver_ask_presentation_count = 0;

static int _e_screensaver_timeout = 0;
//static int _e_screensaver_interval = 0;
static int _e_screensaver_blanking = 0;
static int _e_screensaver_expose = 0;

static Ecore_Timer *_e_screensaver_suspend_timer = NULL;
static Eina_Bool _e_screensaver_on = EINA_FALSE;

EAPI int E_EVENT_SCREENSAVER_ON = -1;
EAPI int E_EVENT_SCREENSAVER_OFF = -1;

EAPI int
e_screensaver_timeout_get(Eina_Bool use_idle)
{
   int timeout = 0, count = (1 + _e_screensaver_ask_presentation_count);
   
   if ((e_config->screensaver_enable) && (!e_config->mode.presentation) &&
       (!e_util_fullscreen_current_any()))
     timeout = e_config->screensaver_timeout * count;
   
   if (use_idle)
     {
        if (e_config->backlight.idle_dim)
          {
             if (timeout > 0)
               {
                  if (e_config->backlight.timer < timeout)
                    timeout = e_config->backlight.timer;
               }
             else
               timeout = e_config->backlight.timer;
          }
     }
   return timeout;
}

EAPI void
e_screensaver_update(void)
{
   int timeout = 0, interval = 0, blanking = 0, expose = 0;
   Eina_Bool changed = EINA_FALSE;

   timeout = e_screensaver_timeout_get(EINA_TRUE);
   
   interval = e_config->screensaver_interval;
   blanking = e_config->screensaver_blanking;
   expose = e_config->screensaver_expose;

   if (_e_screensaver_timeout != timeout)
     {
        _e_screensaver_timeout = timeout;
        changed = EINA_TRUE;
     }
//   if (_e_screensaver_interval != interval)
//     {
//        _e_screensaver_interval = interval;
//        changed = EINA_TRUE;
//     }
   if (_e_screensaver_blanking != blanking)
     {
        _e_screensaver_blanking = blanking;
        changed = EINA_TRUE;
     }
   if (_e_screensaver_expose != expose)
     {
        _e_screensaver_expose = expose;
        changed = EINA_TRUE;
     }

   if (changed) ecore_x_screensaver_set(timeout, interval, blanking, expose);
}

EAPI void
e_screensaver_force_update(void)
{
   int timeout = e_screensaver_timeout_get(EINA_TRUE);

   ecore_x_screensaver_set(timeout + 10,
                           0,
//                           e_config->screensaver_interval,
                           !e_config->screensaver_blanking,
                           !e_config->screensaver_expose);
   ecore_x_screensaver_set(timeout,
                           0,
//                           e_config->screensaver_interval,
                           e_config->screensaver_blanking,
                           e_config->screensaver_expose);
}

static Eina_Bool
_e_screensaver_handler_config_mode_cb(void *data __UNUSED__, int type __UNUSED__, void *event __UNUSED__)
{
   e_screensaver_update();
   return ECORE_CALLBACK_PASS_ON;
}

static void
_e_screensaver_ask_presentation_del(void *data)
{
   if (_e_screensaver_ask_presentation_dia == data)
     _e_screensaver_ask_presentation_dia = NULL;
}

static void
_e_screensaver_ask_presentation_yes(void *data __UNUSED__, E_Dialog *dia)
{
   e_config->mode.presentation = 1;
   e_config_mode_changed();
   e_config_save_queue();
   e_object_del(E_OBJECT(dia));
   _e_screensaver_ask_presentation_count = 0;
}

static void
_e_screensaver_ask_presentation_no(void *data __UNUSED__, E_Dialog *dia)
{
   e_object_del(E_OBJECT(dia));
   _e_screensaver_ask_presentation_count = 0;
}

static void
_e_screensaver_ask_presentation_no_increase(void *data __UNUSED__, E_Dialog *dia)
{
   _e_screensaver_ask_presentation_count++;
   e_screensaver_update();
   e_object_del(E_OBJECT(dia));
}

static void
_e_screensaver_ask_presentation_no_forever(void *data __UNUSED__, E_Dialog *dia)
{
   e_config->screensaver_ask_presentation = 0;
   e_config_save_queue();
   e_object_del(E_OBJECT(dia));
   _e_screensaver_ask_presentation_count = 0;
}

static void
_e_screensaver_ask_presentation_key_down(void *data, Evas *e __UNUSED__, Evas_Object *o __UNUSED__, void *event)
{
   Evas_Event_Key_Down *ev = event;
   E_Dialog *dia = data;

   if (strcmp(ev->key, "Return") == 0)
     _e_screensaver_ask_presentation_yes(NULL, dia);
   else if (strcmp(ev->key, "Escape") == 0)
     _e_screensaver_ask_presentation_no(NULL, dia);
}

static void
_e_screensaver_ask_presentation_mode(void)
{
   E_Manager *man;
   E_Container *con;
   E_Dialog *dia;

   if (_e_screensaver_ask_presentation_dia) return;

   if (!(man = e_manager_current_get())) return;
   if (!(con = e_container_current_get(man))) return;
   if (!(dia = e_dialog_new(con, "E", "_screensaver_ask_presentation"))) return;

   e_dialog_title_set(dia, _("Activate Presentation Mode?"));
   e_dialog_icon_set(dia, "dialog-ask", 64);
   e_dialog_text_set(dia,
		     _("You disabled the screensaver too fast.<br><br>"
		       "Would you like to enable <b>presentation</b> mode and "
		       "temporarily disable screen saver, lock and power saving?"));

   e_object_del_attach_func_set(E_OBJECT(dia),
				_e_screensaver_ask_presentation_del);
   e_dialog_button_add(dia, _("Yes"), NULL,
		       _e_screensaver_ask_presentation_yes, NULL);
   e_dialog_button_add(dia, _("No"), NULL,
		       _e_screensaver_ask_presentation_no, NULL);
   e_dialog_button_add(dia, _("No, but increase timeout"), NULL,
		       _e_screensaver_ask_presentation_no_increase, NULL);
   e_dialog_button_add(dia, _("No, and stop asking"), NULL,
		       _e_screensaver_ask_presentation_no_forever, NULL);

   e_dialog_button_focus_num(dia, 0);
   e_widget_list_homogeneous_set(dia->box_object, 0);
   e_win_centered_set(dia->win, 1);
   e_dialog_show(dia);

   evas_object_event_callback_add(dia->bg_object, EVAS_CALLBACK_KEY_DOWN,
				  _e_screensaver_ask_presentation_key_down, dia);

   _e_screensaver_ask_presentation_dia = dia;
}

static Eina_Bool
_e_screensaver_suspend_cb(void *data __UNUSED__)
{
   _e_screensaver_suspend_timer = NULL;
   if (e_config->screensaver_suspend)
     {
        if ((e_config->screensaver_suspend_on_ac) ||
            (e_powersave_mode_get() > E_POWERSAVE_MODE_LOW))
           e_sys_action_do(E_SYS_SUSPEND, NULL);
     }
   return EINA_FALSE;
}

static Eina_Bool
_e_screensaver_handler_powersave_cb(void *data __UNUSED__, int type __UNUSED__, void *event __UNUSED__)
{
   if ((e_config->screensaver_suspend) && (_e_screensaver_on))
     {
        if (_e_screensaver_suspend_timer)
           ecore_timer_del(_e_screensaver_suspend_timer);
        _e_screensaver_suspend_timer =
           ecore_timer_add(e_config->screensaver_suspend_delay,
                           _e_screensaver_suspend_cb, NULL);
     }
   return ECORE_CALLBACK_PASS_ON;
}

static double last_start = 0.0;

static Eina_Bool
_e_screensaver_handler_screensaver_on_cb(void *data __UNUSED__, int type __UNUSED__, void *event __UNUSED__)
{
   _e_screensaver_on = EINA_TRUE;
   if (_e_screensaver_suspend_timer)
     {
        ecore_timer_del(_e_screensaver_suspend_timer);
        _e_screensaver_suspend_timer = NULL;
     }
   if (e_config->screensaver_suspend)
     _e_screensaver_suspend_timer =
     ecore_timer_add(e_config->screensaver_suspend_delay,
                     _e_screensaver_suspend_cb, NULL);
   last_start = ecore_loop_time_get();
   _e_screensaver_ask_presentation_count = 0;
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_screensaver_handler_screensaver_off_cb(void *data __UNUSED__, int type __UNUSED__, void *event __UNUSED__)
{
//   e_screensaver_force_update();
//   e_dpms_force_update();
        
   _e_screensaver_on = EINA_FALSE;
   if (_e_screensaver_suspend_timer)
     {
        ecore_timer_del(_e_screensaver_suspend_timer);
        _e_screensaver_suspend_timer = NULL;
     }
   if ((last_start > 0.0) && (e_config->screensaver_ask_presentation))
     {
	double current = ecore_loop_time_get();
        
	if ((last_start + e_config->screensaver_ask_presentation_timeout)
            >= current)
	  _e_screensaver_ask_presentation_mode();
	last_start = 0.0;
     }
   else if (_e_screensaver_ask_presentation_count)
     _e_screensaver_ask_presentation_count = 0;
   return ECORE_CALLBACK_PASS_ON;
}

static Ecore_Timer *idle_timer = NULL;
static Eina_Bool saver_on = EINA_FALSE;
static Eina_Bool dimmed = EINA_FALSE;

static Eina_Bool
_e_screensaver_idle_timer_cb(void *d __UNUSED__)
{
   ecore_event_add(E_EVENT_SCREENSAVER_ON, NULL, NULL, NULL);
   idle_timer = NULL;
   return EINA_FALSE;
}

static Eina_Bool
_e_screensaver_handler_screensaver_notify_cb(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_X_Event_Screensaver_Notify *e = event;

   if ((e->on) && (!saver_on))
     {
        saver_on = EINA_TRUE;
        if (e_config->backlight.idle_dim)
          {
             double t = e_config->screensaver_timeout - 
               e_config->backlight.timer;
             
             if (t < 1.0) t = 1.0;
             if (idle_timer)
               {
                  ecore_timer_del(idle_timer);
                  idle_timer = NULL;
               }
             if (e_config->screensaver_enable)
               idle_timer = ecore_timer_add
               (t, _e_screensaver_idle_timer_cb, NULL);
             if (e_backlight_mode_get(NULL) != E_BACKLIGHT_MODE_DIM)
               {
                  e_backlight_mode_set(NULL, E_BACKLIGHT_MODE_DIM);
                  dimmed = EINA_TRUE;
               }
          }
        else
          {
             if (!_e_screensaver_on)
               ecore_event_add(E_EVENT_SCREENSAVER_ON, NULL, NULL, NULL);
          }
     }
   else if ((!e->on) && (saver_on))
     {
        saver_on = EINA_FALSE;
        if (idle_timer)
          {
             ecore_timer_del(idle_timer);
             idle_timer = NULL;
             if (e_config->backlight.idle_dim)
               {
                  if (e_backlight_mode_get(NULL) != E_BACKLIGHT_MODE_NORMAL)
                    e_backlight_mode_set(NULL, E_BACKLIGHT_MODE_NORMAL);
               }
          }
        else
          {
             if (dimmed)
               {
                  if (e_backlight_mode_get(NULL) != E_BACKLIGHT_MODE_NORMAL)
                    e_backlight_mode_set(NULL, E_BACKLIGHT_MODE_NORMAL);
                  dimmed = EINA_FALSE;
               }
             if (_e_screensaver_on)
               ecore_event_add(E_EVENT_SCREENSAVER_OFF, NULL, NULL, NULL);
          }
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_screensaver_handler_border_fullscreen_check_cb(void *data __UNUSED__, int type __UNUSED__, void *event __UNUSED__)
{
   e_screensaver_update();
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_screensaver_handler_border_desk_set_cb(void *data __UNUSED__, int type __UNUSED__, void *event __UNUSED__)
{
   e_screensaver_update();
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_screensaver_handler_desk_show_cb(void *data __UNUSED__, int type __UNUSED__, void *event __UNUSED__)
{
   e_screensaver_update();
   return ECORE_CALLBACK_PASS_ON;
}

EINTERN void
e_screensaver_preinit(void)
{
   E_EVENT_SCREENSAVER_ON = ecore_event_type_new();
   E_EVENT_SCREENSAVER_OFF = ecore_event_type_new();
}

EINTERN int
e_screensaver_init(void)
{
   ecore_x_screensaver_custom_blanking_enable();

   _e_screensaver_handler_on = ecore_event_handler_add
     (E_EVENT_SCREENSAVER_ON, _e_screensaver_handler_screensaver_on_cb, NULL);
   _e_screensaver_handler_off = ecore_event_handler_add
     (E_EVENT_SCREENSAVER_OFF, _e_screensaver_handler_screensaver_off_cb, NULL);
   
   _e_screensaver_handler_screensaver_notify = ecore_event_handler_add
     (ECORE_X_EVENT_SCREENSAVER_NOTIFY, _e_screensaver_handler_screensaver_notify_cb, NULL);
   
   _e_screensaver_handler_config_mode = ecore_event_handler_add
     (E_EVENT_CONFIG_MODE_CHANGED, _e_screensaver_handler_config_mode_cb, NULL);
   
   _e_screensaver_handler_border_fullscreen = ecore_event_handler_add
     (E_EVENT_BORDER_FULLSCREEN, _e_screensaver_handler_border_fullscreen_check_cb, NULL);
   _e_screensaver_handler_border_unfullscreen = ecore_event_handler_add
     (E_EVENT_BORDER_UNFULLSCREEN, _e_screensaver_handler_border_fullscreen_check_cb, NULL);
   _e_screensaver_handler_border_remove = ecore_event_handler_add
     (E_EVENT_BORDER_REMOVE, _e_screensaver_handler_border_fullscreen_check_cb, NULL);
   _e_screensaver_handler_border_iconify = ecore_event_handler_add
     (E_EVENT_BORDER_ICONIFY, _e_screensaver_handler_border_fullscreen_check_cb, NULL);
   _e_screensaver_handler_border_uniconify = ecore_event_handler_add
     (E_EVENT_BORDER_UNICONIFY, _e_screensaver_handler_border_fullscreen_check_cb, NULL);
   _e_screensaver_handler_border_desk_set = ecore_event_handler_add
     (E_EVENT_BORDER_DESK_SET, _e_screensaver_handler_border_desk_set_cb, NULL);
   _e_screensaver_handler_desk_show = ecore_event_handler_add
     (E_EVENT_DESK_SHOW, _e_screensaver_handler_desk_show_cb, NULL);

   _e_screensaver_handler_powersave = ecore_event_handler_add
     (E_EVENT_POWERSAVE_UPDATE, _e_screensaver_handler_powersave_cb, NULL);
   
   _e_screensaver_timeout = ecore_x_screensaver_timeout_get();
//   _e_screensaver_interval = ecore_x_screensaver_interval_get();
   _e_screensaver_blanking = ecore_x_screensaver_blank_get();
   _e_screensaver_expose = ecore_x_screensaver_expose_get();

   e_screensaver_force_update();

   return 1;
}

EINTERN int
e_screensaver_shutdown(void)
{
   if (_e_screensaver_handler_on)
     {
        ecore_event_handler_del(_e_screensaver_handler_on);
        _e_screensaver_handler_on = NULL;
     }

   if (_e_screensaver_handler_off)
     {
        ecore_event_handler_del(_e_screensaver_handler_off);
        _e_screensaver_handler_off = NULL;
     }
   
   if (_e_screensaver_suspend_timer)
     {
        ecore_timer_del(_e_screensaver_suspend_timer);
        _e_screensaver_suspend_timer = NULL;
     }
   
   ecore_x_screensaver_custom_blanking_disable();

   if (_e_screensaver_handler_powersave)
     {
        ecore_event_handler_del(_e_screensaver_handler_powersave);
        _e_screensaver_handler_powersave = NULL;
     }

   if (_e_screensaver_handler_config_mode)
     {
        ecore_event_handler_del(_e_screensaver_handler_config_mode);
        _e_screensaver_handler_config_mode = NULL;
     }

   if (_e_screensaver_handler_screensaver_notify)
     {
        ecore_event_handler_del(_e_screensaver_handler_screensaver_notify);
        _e_screensaver_handler_screensaver_notify = NULL;
     }

   if (_e_screensaver_handler_border_fullscreen)
     {
        ecore_event_handler_del(_e_screensaver_handler_border_fullscreen);
        _e_screensaver_handler_border_fullscreen = NULL;
     }

   if (_e_screensaver_handler_border_unfullscreen)
     {
        ecore_event_handler_del(_e_screensaver_handler_border_unfullscreen);
        _e_screensaver_handler_border_unfullscreen = NULL;
     }

   if (_e_screensaver_handler_border_remove)
     {
        ecore_event_handler_del(_e_screensaver_handler_border_remove);
        _e_screensaver_handler_border_remove = NULL;
     }

   if (_e_screensaver_handler_border_iconify)
     {
        ecore_event_handler_del(_e_screensaver_handler_border_iconify);
        _e_screensaver_handler_border_iconify = NULL;
     }

   if (_e_screensaver_handler_border_uniconify)
     {
        ecore_event_handler_del(_e_screensaver_handler_border_uniconify);
        _e_screensaver_handler_border_uniconify = NULL;
     }

   if (_e_screensaver_handler_border_desk_set)
     {
        ecore_event_handler_del(_e_screensaver_handler_border_desk_set);
        _e_screensaver_handler_border_desk_set = NULL;
     }

   if (_e_screensaver_handler_desk_show)
     {
        ecore_event_handler_del(_e_screensaver_handler_desk_show);
        _e_screensaver_handler_desk_show = NULL;
     }

   return 1;
}
