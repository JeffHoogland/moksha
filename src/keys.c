#include "keys.h"
#include "actions.h"

static void ecore_key_down(Ecore_Event * ev);
static void ecore_key_up(Ecore_Event * ev);

static void 
ecore_key_down(Ecore_Event * ev)
{
   Ecore_Event_Key_Down          *e;
   
   e = ev->event;
   if (e->win == ecore_get_key_grab_win())
     {
	e_action_stop("Key_Binding", ACT_KEY_DOWN, 0, e->key, e->mods, 
		      NULL, NULL, 0, 0, 0, 0);
	e_action_start("Key_Binding", ACT_KEY_DOWN, 0, e->key, e->mods, 
		      NULL, NULL, 0, 0, 0, 0);
     }
}

static void 
ecore_key_up(Ecore_Event * ev)
{
   Ecore_Event_Key_Up          *e;
   
   e = ev->event;
   if (e->win == ecore_get_key_grab_win())
     {
	e_action_stop("Key_Binding", ACT_KEY_UP, 0, e->key, e->mods, 
		      NULL, NULL, 0, 0, 0, 0);
	e_action_start("Key_Binding", ACT_KEY_UP, 0, e->key, e->mods, 
		      NULL, NULL, 0, 0, 0, 0);
     }
}

void
ecore_keys_init(void)
{
   /* load up our actions .... once to get some grabbed keys */
   e_action_start("", ACT_KEY_DOWN, 0, NULL, ECORE_EVENT_KEY_MODIFIER_NONE,
		  NULL, NULL, 0, 0, 0, 0);   
   ecore_event_filter_handler_add(ECORE_EVENT_KEY_DOWN,                 ecore_key_down);
   ecore_event_filter_handler_add(ECORE_EVENT_KEY_UP,                   ecore_key_up);
}

void
ecore_keys_grab(char *key, Ecore_Event_Key_Modifiers mods, int anymod)
{
   ecore_key_grab(key, mods, anymod, 0);
}

void
ecore_keys_ungrab(char *key, Ecore_Event_Key_Modifiers mods, int anymod)
{
   ecore_key_ungrab(key, mods, anymod);
}
