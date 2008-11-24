/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

struct _E_Powersave_Deferred_Action
{
   void (*func) (void *data);
   const void *data;
   unsigned char delete_me : 1;
};

/* local subsystem functions */
static int _e_powersave_cb_deferred_timer(void *data);
static void _e_powersave_mode_eval(void);
static void _e_powersave_event_update_free(void *data __UNUSED__, void *event);

/* local subsystem globals */
EAPI int E_EVENT_POWERSAVE_UPDATE = 0;
static int walking_deferred_actions = 0;
static Eina_List *deferred_actions = NULL;
static Ecore_Timer *deferred_timer = NULL;
static E_Powersave_Mode powersave_mode_min = E_POWERSAVE_MODE_NONE;
static E_Powersave_Mode powersave_mode_max = E_POWERSAVE_MODE_EXTREME;
static E_Powersave_Mode powersave_mode = E_POWERSAVE_MODE_LOW;
static double defer_time = 5.0;

/* externally accessible functions */
EAPI int
e_powersave_init(void)
{
   _e_powersave_mode_eval();
   E_EVENT_POWERSAVE_UPDATE = ecore_event_type_new();
   return 1;
}

EAPI int
e_powersave_shutdown(void)
{
   return 1;
}

EAPI E_Powersave_Deferred_Action *
e_powersave_deferred_action_add(void (*func) (void *data), const void *data)
{
   E_Powersave_Deferred_Action *pa;
   
   pa = calloc(1, sizeof(E_Powersave_Deferred_Action));
   if (!pa) return NULL;
   if (deferred_timer) ecore_timer_del(deferred_timer);
   deferred_timer = ecore_timer_add(defer_time,
				    _e_powersave_cb_deferred_timer,
				    NULL);
   pa->func = func;
   pa->data = data;
   deferred_actions = eina_list_append(deferred_actions, pa);
   return pa;
}

EAPI void
e_powersave_deferred_action_del(E_Powersave_Deferred_Action *pa)
{
   if (walking_deferred_actions)
     {
	pa->delete_me = 1;
	return;
     }
   else
     {
	deferred_actions = eina_list_remove(deferred_actions, pa);
	free(pa);
	if (!deferred_actions)
	  {
	     if (deferred_timer)
	       {
		  ecore_timer_del(deferred_timer);
		  deferred_timer = NULL;
	       }
	  }
     }
}

EAPI void
e_powersave_mode_min_set(E_Powersave_Mode mode)
{
   powersave_mode_min = mode;
   e_powersave_mode_set(powersave_mode);
}

EAPI void
e_powersave_mode_max_set(E_Powersave_Mode mode)
{
   powersave_mode_max = mode;
   e_powersave_mode_set(powersave_mode);
}

EAPI void
e_powersave_mode_set(E_Powersave_Mode mode)
{
   E_Event_Powersave_Update *ev;

   if (mode < powersave_mode_min) mode = powersave_mode_min;
   else if (mode > powersave_mode_max) mode = powersave_mode_max;
   if (powersave_mode == mode) return;
   printf("CHANGE PW SAVE MODE TO %i / %i\n", (int)mode, E_POWERSAVE_MODE_EXTREME);
   powersave_mode = mode;

   ev = E_NEW(E_Event_Powersave_Update, 1);
   ev->mode = mode;
   ecore_event_add(E_EVENT_POWERSAVE_UPDATE, ev, _e_powersave_event_update_free, NULL);
   _e_powersave_mode_eval();
}

EAPI E_Powersave_Mode
e_powersave_mode_min_get(void)
{
   return powersave_mode_min;
}

EAPI E_Powersave_Mode
e_powersave_mode_max_get(void)
{
   return powersave_mode_max;
}

EAPI E_Powersave_Mode
e_powersave_mode_get(void)
{
   return powersave_mode;
}

/* local subsystem functions */

static int
_e_powersave_cb_deferred_timer(void *data)
{
   walking_deferred_actions++;
   while (deferred_actions)
     {
	E_Powersave_Deferred_Action *pa;
	
	pa = deferred_actions->data;
	deferred_actions = eina_list_remove_list(deferred_actions, deferred_actions);
	if (!pa->delete_me) pa->func((void *)pa->data);
	free(pa);
     }
   walking_deferred_actions--;
   if (!deferred_actions) deferred_timer = NULL;
   return 0;
}

static void
_e_powersave_mode_eval(void)
{
   double t = 0.0;
   
   switch (powersave_mode)
     {
	/* FIXME: these values are hardcoded - shoudl be configurable */
      case E_POWERSAVE_MODE_NONE:
	t = 0.25; /* time to defer "power expensive" activities */
	break;
      case E_POWERSAVE_MODE_LOW:
	t = 5.0;
	break;
      case E_POWERSAVE_MODE_MEDIUM:
	t = 60.0;
	break;
      case E_POWERSAVE_MODE_HIGH:
	t = 300.0;
	break;
      case E_POWERSAVE_MODE_EXTREME:
	t = 1200.0;
	break;
      default:
	return;
	break;
     }
   if (t != defer_time)
     {
	if (deferred_timer) ecore_timer_del(deferred_timer);
	deferred_timer = ecore_timer_add(defer_time,
					 _e_powersave_cb_deferred_timer,
					 NULL);
     }
}

static void
_e_powersave_event_update_free(void *data __UNUSED__, void *event)
{
   free(event);
}
