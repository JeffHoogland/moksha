/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

static Ecore_Event_Handler *_e_screensaver_handler_config_mode = NULL;

static int
_e_screensaver_handler_config_mode_cb(void *data __UNUSED__, int type __UNUSED__, void *event __UNUSED__)
{
   e_screensaver_init();
   return 1;
}

EAPI int
e_screensaver_init(void)
{
   int timeout=0, interval=0, blanking=0, expose=0;

   if (!_e_screensaver_handler_config_mode)
     _e_screensaver_handler_config_mode = ecore_event_handler_add
       (E_EVENT_CONFIG_MODE_CHANGED, _e_screensaver_handler_config_mode_cb, NULL);

   if ((e_config->screensaver_enable) && (!e_config->mode.presentation))
     timeout = e_config->screensaver_timeout;
   
   interval = e_config->screensaver_interval;
   blanking = e_config->screensaver_blanking;
   expose = e_config->screensaver_expose;
  
   ecore_x_screensaver_set(timeout, interval, blanking, expose);
   return 1;
}
