#include "debug.h"
#include "keys.h"
#include "actions.h"

static void         e_key_down(Ecore_Event * ev);
static void         e_key_up(Ecore_Event * ev);

static void
e_key_down(Ecore_Event * ev)
{
   Ecore_Event_Key_Down *e;

   D_ENTER;

   e = ev->event;
   if (e->win == ecore_get_key_grab_win())
     {
	e_action_stop("Key_Binding", ACT_KEY_DOWN, 0, e->key, e->mods,
		      NULL, NULL, 0, 0, 0, 0);
	e_action_start("Key_Binding", ACT_KEY_DOWN, 0, e->key, e->mods,
		       NULL, NULL, 0, 0, 0, 0);
     }

   D_RETURN;
}

static void
e_key_up(Ecore_Event * ev)
{
   Ecore_Event_Key_Up *e;

   D_ENTER;

   e = ev->event;
   if (e->win == ecore_get_key_grab_win())
     {
	e_action_stop("Key_Binding", ACT_KEY_UP, 0, e->key, e->mods,
		      NULL, NULL, 0, 0, 0, 0);
	e_action_start("Key_Binding", ACT_KEY_UP, 0, e->key, e->mods,
		       NULL, NULL, 0, 0, 0, 0);
     }

   D_RETURN;
}

void
e_keys_init(void)
{
   D_ENTER;

   /* load up our actions .... once to get some grabbed keys */
   e_action_start("", ACT_KEY_DOWN, 0, NULL, ECORE_EVENT_KEY_MODIFIER_NONE,
		  NULL, NULL, 0, 0, 0, 0);
   ecore_event_filter_handler_add(ECORE_EVENT_KEY_DOWN, e_key_down);
   ecore_event_filter_handler_add(ECORE_EVENT_KEY_UP, e_key_up);

   D_RETURN;
}

void
e_keys_grab(char *key, Ecore_Event_Key_Modifiers mods, int anymod)
{
   D_ENTER;

   ecore_key_grab(key, mods, anymod, 0);

   D_RETURN;
}

void
e_keys_ungrab(char *key, Ecore_Event_Key_Modifiers mods, int anymod)
{
   D_ENTER;

   ecore_key_ungrab(key, mods, anymod);

   D_RETURN;
}
