#include "e.h"

/* local function prototypes */
static Eina_Bool _e_alert_cb_exe_del(void *data __UNUSED__, int type __UNUSED__, void *event);

/* local variables */
static Ecore_Exe *alert_exe = NULL;
static Ecore_Event_Handler *alert_exe_hdl = NULL;

/* public variables */
EAPI unsigned long e_alert_composite_win = 0;

EINTERN int 
e_alert_init(void) 
{
   alert_exe_hdl = 
     ecore_event_handler_add(ECORE_EXE_EVENT_DEL, _e_alert_cb_exe_del, NULL);

   return 1;
}

EINTERN int 
e_alert_shutdown(void) 
{
   e_alert_hide();

   if (alert_exe_hdl) ecore_event_handler_del(alert_exe_hdl);
   alert_exe_hdl = NULL;

   return 1;
}

EAPI void 
e_alert_show(int sig) 
{
   char buf[8192];

   snprintf(buf, sizeof(buf), 
            "%s/enlightenment/utils/enlightenment_alert %d %d %d",
	    e_prefix_lib_get(), sig, getpid(), e_alert_composite_win);

   alert_exe = ecore_exe_run(buf, NULL);
   pause();
}

EAPI void 
e_alert_hide(void) 
{
   if (alert_exe) ecore_exe_terminate(alert_exe);
}

/* local functions */
static Eina_Bool 
_e_alert_cb_exe_del(void *data __UNUSED__, int type __UNUSED__, void *event) 
{
   Ecore_Exe_Event_Del *ev;

   ev = event;
   if (!alert_exe) return ECORE_CALLBACK_RENEW;
   if (ev->exe == alert_exe) alert_exe = NULL;

   return ECORE_CALLBACK_RENEW;
}
