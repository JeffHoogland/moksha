#include "e.h"

static void e_key_down(Eevent * ev);
static void e_key_up(Eevent * ev);

static void 
e_key_down(Eevent * ev)
{
   Ev_Key_Down          *e;
   
   e = ev->event;
   if (e->win == e_get_key_grab_win())
     {
	e_action_stop("Key_Binding", ACT_KEY_DOWN, 0, e->key, e->mods, 
		      NULL, NULL, 0, 0, 0, 0);
	e_action_start("Key_Binding", ACT_KEY_DOWN, 0, e->key, e->mods, 
		      NULL, NULL, 0, 0, 0, 0);
     }
}

static void 
e_key_up(Eevent * ev)
{
   Ev_Key_Up          *e;
   
   e = ev->event;
   if (e->win == e_get_key_grab_win())
     {
	e_action_stop("Key_Binding", ACT_KEY_UP, 0, e->key, e->mods, 
		      NULL, NULL, 0, 0, 0, 0);
	e_action_start("Key_Binding", ACT_KEY_UP, 0, e->key, e->mods, 
		      NULL, NULL, 0, 0, 0, 0);
     }
}

void
e_keys_init(void)
{
   /* load up our actions .... once to get some grabbed keys */
   e_action_start("", ACT_KEY_DOWN, 0, NULL, EV_KEY_MODIFIER_NONE,
		  NULL, NULL, 0, 0, 0, 0);   
   e_event_filter_handler_add(EV_KEY_DOWN,                 e_key_down);
   e_event_filter_handler_add(EV_KEY_UP,                   e_key_up);
}

void
e_keys_grab(char *key, Ev_Key_Modifiers mods, int anymod)
{
   e_key_grab(key, mods, anymod, 0);
}

void
e_keys_ungrab(char *key, Ev_Key_Modifiers mods, int anymod)
{
   e_key_ungrab(key, mods, anymod);
}
