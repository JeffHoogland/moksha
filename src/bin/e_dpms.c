#include "e.h"

static Ecore_Event_Handler *_e_dpms_handler_config_mode = NULL;
static Ecore_Event_Handler *_e_dpms_handler_border_fullscreen = NULL;
static Ecore_Event_Handler *_e_dpms_handler_border_unfullscreen = NULL;
static Ecore_Event_Handler *_e_dpms_handler_border_remove = NULL;
static Ecore_Event_Handler *_e_dpms_handler_border_iconify = NULL;
static Ecore_Event_Handler *_e_dpms_handler_border_uniconify = NULL;
static Ecore_Event_Handler *_e_dpms_handler_border_desk_set = NULL;
static Ecore_Event_Handler *_e_dpms_handler_desk_show = NULL;

static Eina_Bool
_e_dpms_handler_config_mode_cb(void *data __UNUSED__, int type __UNUSED__, void *event __UNUSED__)
{
   e_dpms_init();
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_dpms_handler_border_fullscreen_check_cb(void *data __UNUSED__, int type __UNUSED__, void *event __UNUSED__)
{
   e_dpms_init();
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_dpms_handler_border_desk_set_cb(void *data __UNUSED__, int type __UNUSED__, void *event __UNUSED__)
{
   e_dpms_init();
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_dpms_handler_desk_show_cb(void *data __UNUSED__, int type __UNUSED__, void *event __UNUSED__)
{
   e_dpms_init();
   return ECORE_CALLBACK_PASS_ON;
}

EAPI int
e_dpms_init(void)
{
   int standby = 0, suspend = 0, off = 0;
   int enabled;

   enabled = ((e_config->dpms_enable) && (!e_config->mode.presentation) &&
	      (!e_util_fullscreen_curreny_any()));

   if (!_e_dpms_handler_config_mode)
     _e_dpms_handler_config_mode = ecore_event_handler_add
       (E_EVENT_CONFIG_MODE_CHANGED, _e_dpms_handler_config_mode_cb, NULL);

   if (!_e_dpms_handler_border_fullscreen)
     _e_dpms_handler_border_fullscreen = ecore_event_handler_add
       (E_EVENT_BORDER_FULLSCREEN, _e_dpms_handler_border_fullscreen_check_cb, NULL);

   if (!_e_dpms_handler_border_unfullscreen)
     _e_dpms_handler_border_unfullscreen = ecore_event_handler_add
       (E_EVENT_BORDER_UNFULLSCREEN, _e_dpms_handler_border_fullscreen_check_cb, NULL);

   if (!_e_dpms_handler_border_remove)
     _e_dpms_handler_border_remove = ecore_event_handler_add
       (E_EVENT_BORDER_REMOVE, _e_dpms_handler_border_fullscreen_check_cb, NULL);

   if (!_e_dpms_handler_border_iconify)
     _e_dpms_handler_border_iconify = ecore_event_handler_add
       (E_EVENT_BORDER_ICONIFY, _e_dpms_handler_border_fullscreen_check_cb, NULL);

   if (!_e_dpms_handler_border_uniconify)
     _e_dpms_handler_border_uniconify = ecore_event_handler_add
       (E_EVENT_BORDER_UNICONIFY, _e_dpms_handler_border_fullscreen_check_cb, NULL);

   if (!_e_dpms_handler_border_desk_set)
     _e_dpms_handler_border_desk_set = ecore_event_handler_add
       (E_EVENT_BORDER_DESK_SET, _e_dpms_handler_border_desk_set_cb, NULL);

   if (!_e_dpms_handler_desk_show)
     _e_dpms_handler_desk_show = ecore_event_handler_add
       (E_EVENT_DESK_SHOW, _e_dpms_handler_desk_show_cb, NULL);

   ecore_x_dpms_enabled_set(enabled);
   if (!enabled) return 1;

   if (e_config->dpms_standby_enable)
     standby = e_config->dpms_standby_timeout;

   if (e_config->dpms_suspend_enable)
     suspend = e_config->dpms_suspend_timeout;

   if (e_config->dpms_off_enable)
     off = e_config->dpms_off_timeout;

   ecore_x_dpms_timeouts_set(standby, suspend, off);

   return 1;
}
