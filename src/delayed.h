#ifndef E_DELAYED_H
#define E_DELAYED_H

#include "e.h"
#include "observer.h"

typedef void        (*E_Delay_Func) (int val, void *obj);
typedef float       (*E_Delay_Val) ();

typedef struct _e_delayed_action
{
   E_Observer          obs;

   E_Delay_Val         delay;
   E_Delay_Func        delay_func;

}
E_Delayed_Action;

E_Delayed_Action   *e_delayed_action_new(E_Event_Type event,
					 E_Delay_Val delay,
					 E_Delay_Func delay_func);

void                e_delayed_action_start(E_Observer * obs, E_Observee * obj,
					   E_Event_Type event, void *data);
void                e_delayed_action_cancel(E_Delayed_Action * eda);

#endif
