#include <string.h>

#include "debug.h"
#include "observer.h"

static Evas_List    observers;

void
e_observer_init(E_Observer * obs, E_Event_Type event,
		E_Notify_Func notify_func, E_Cleanup_Func cleanup_func)
{
   D_ENTER;

   if (!obs)
      D_RETURN;

   memset(obs, 0, sizeof(E_Observer));

   obs->watched = NULL;
   obs->event = event;
   obs->notify_func = notify_func;

   e_object_init(E_OBJECT(obs), cleanup_func);

   observers = evas_list_append(observers, obs);

   D_RETURN;
}

void
e_observer_cleanup(E_Observer * obs)
{
   E_Observee         *o = NULL;

   D_ENTER;

   if (!obs)
      D_RETURN;

   while (obs->watched)
     {
	o = E_OBSERVEE(obs->watched->data);
	e_observer_unregister_observee(obs, o);
     }

   evas_list_remove(observers, obs);
   /* Call the destructor of the base class */
   e_object_cleanup(E_OBJECT(obs));

   D_RETURN;
}

void
e_observer_register_observee(E_Observer * observer, E_Observee * observee)
{
   D_ENTER;

   if (!observer || !observee)
      D_RETURN;

   observee->observers = evas_list_append(observee->observers, observer);
   observer->watched = evas_list_append(observer->watched, observee);
   D_RETURN;
}

void
e_observer_unregister_observee(E_Observer * observer, E_Observee * observee)
{
   D_ENTER;

   if (!observer || !observee)
      D_RETURN;

   observee->observers = evas_list_remove(observee->observers, observer);
   observer->watched = evas_list_remove(observer->watched, observee);

   D_RETURN;
}

/* --------------------- Observee code below */

void
e_observee_init(E_Observee * obs, E_Cleanup_Func cleanup_func)
{
   D_ENTER;

   if (!obs)
      D_RETURN;

   memset(obs, 0, sizeof(E_Observee));

   e_object_init(E_OBJECT(obs), cleanup_func);

   D_RETURN;
}

void
e_observee_notify_observers(E_Observee * o, E_Event_Type event)
{
   Evas_List           obs_list = NULL;
   E_Observer         *obs = NULL;

   D_ENTER;

   if (!o)
      D_RETURN;

   for (obs_list = o->observers; obs_list; obs_list = obs_list->next)
     {
	obs = E_OBSERVER(obs_list->data);

	/* check bit mask */
	if (obs->event & event)
	  {
	     obs->notify_func(obs, o, event);
	  }
     }

   D_RETURN;
}

void
e_observee_notify_all_observers(E_Observee * o, E_Event_Type event)
{
   Evas_List           obs_list = NULL;
   E_Observer         *obs = NULL;

   D_ENTER;

   if (!o)
      D_RETURN;

   for (obs_list = observers; obs_list; obs_list = obs_list->next)
     {
	obs = E_OBSERVER(obs_list->data);

	/* check bit mask */
	if (obs->event & event)
	  {
	     obs->notify_func(obs, o, event);
	  }
     }

   D_RETURN;
}

void
e_observee_cleanup(E_Observee * obs)
{
   E_Observer         *observer = NULL;

   D_ENTER;

   if (!obs)
      D_RETURN;

   if (e_object_get_usecount(E_OBJECT(obs)) == 1)
     {
	while (obs->observers)
	  {
	     observer = E_OBSERVER(obs->observers->data);
	     e_observer_unregister_observee(observer, obs);
	  }
     }

   /* Call the destructor of the base class */
   e_object_cleanup(E_OBJECT(obs));

   D_RETURN;
}
