#include <string.h>
#include "debug.h"
#include "delayed.h"

static void
e_delayed_action_cleanup(E_Delayed_Action *eda)
{
  D_ENTER;

  e_delayed_action_cancel(eda);
  e_observer_cleanup(E_OBSERVER(eda));

  D_RETURN;
}


E_Delayed_Action *
e_delayed_action_new(Ecore_Event_Type event,
		     double delay, E_Delay_Func delay_func)
{
  E_Delayed_Action *eda = NULL;

  D_ENTER;

  eda = NEW(E_Delayed_Action, 1);
  memset(eda, 0, sizeof(E_Delayed_Action));

  e_observer_init(E_OBSERVER(eda), event, e_delayed_action_start,
		  (E_Cleanup_Func) e_delayed_action_cleanup);

  eda->delay      = delay;
  eda->delay_func = delay_func;

  D_RETURN_(eda);
}


void
e_delayed_action_start(E_Observer *obs, E_Observee *obj)
{
   char event_name[1024];
   E_Delayed_Action *eda = (E_Delayed_Action*) obs;

   D_ENTER;

   snprintf(event_name, 1024, "_e_delayed_action_notify(%d)", obs->event);
   ecore_add_event_timer(event_name, eda->delay, eda->delay_func, 0, obj);

   D_RETURN;
}


void
e_delayed_action_cancel(E_Delayed_Action *eda)
{
   char event_name[1024];

   D_ENTER;

   snprintf(event_name, 1024, "_e_delayed_action_notify(%d)", E_OBSERVER(eda)->event);
   ecore_del_event_timer(event_name);

   D_RETURN;
}

