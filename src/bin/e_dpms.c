/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

static Ecore_Event_Handler *_e_dpms_handler_config_mode = NULL;

static int
_e_dpms_handler_config_mode_cb(void *data __UNUSED__, int type __UNUSED__, void *event __UNUSED__)
{
   e_dpms_init();
   return 1;
}

EAPI int
e_dpms_init(void)
{
   int standby=0, suspend=0, off=0;
   int enabled = ((e_config->dpms_enable) && (!e_config->mode.presentation));

   if (!_e_dpms_handler_config_mode)
     _e_dpms_handler_config_mode = ecore_event_handler_add
       (E_EVENT_CONFIG_MODE_CHANGED, _e_dpms_handler_config_mode_cb, NULL);

   ecore_x_dpms_enabled_set(enabled);
   if (!enabled)
     return 1;

   if (e_config->dpms_standby_enable)
     standby = e_config->dpms_standby_timeout;
   
   if (e_config->dpms_suspend_enable)
     suspend = e_config->dpms_suspend_timeout;
   
   if (e_config->dpms_off_enable)
     off = e_config->dpms_off_timeout;

   ecore_x_dpms_timeouts_set(standby, suspend, off);
   
   return 1;
}
