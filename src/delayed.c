
#include "delayed.h"

void
e_delayed_action_start(void *obs, void *obj)
{
   char event_name[1024];
   E_Delayed_Action *eda = obs;

   snprintf(event_name, 1024, "_e_delayed_action_notify(%d)", eda->e_event);
   e_add_event_timer(event_name, eda->delay, eda->delay_func, 0, obj);
}

void
e_delayed_action_cancel(void *obs)
{
   E_Delayed_Action *eda = obs;
   char event_name[1024];
   snprintf(event_name, 1024, "_e_delayed_action_notify(%d)", eda->e_event);
   e_del_event_timer(event_name);
}

void
e_delayed_action_free(void *obs)
{
	e_delayed_action_cancel(obs);
	free(obs);
}

