/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* local subsystem functions */

/* local subsystem globals */
Ecore_X_Window grab_mouse_win = 0;
Ecore_X_Window grab_key_win = 0;

/* externally accessible functions */
int
e_grabinput_init(void)
{
   return 1;
}

int
e_grabinput_shutdown(void)
{
   return 1;
}

void
e_grabinput_get(Ecore_X_Window mouse_win, int confine_mouse, Ecore_X_Window key_win)
{
   if (grab_mouse_win)
     {
	ecore_x_pointer_ungrab();
	grab_mouse_win = 0;
     }
   if (grab_key_win)
     {
	ecore_x_keyboard_ungrab();
	grab_key_win = 0;
     }
   if (mouse_win)
     {
	if (confine_mouse)
	  ecore_x_pointer_confine_grab(mouse_win);
	else
	  ecore_x_pointer_grab(mouse_win);
	grab_mouse_win = mouse_win;
     }
   if (key_win)
     {
	ecore_x_keyboard_grab(key_win);
	grab_key_win = key_win;
     }
}

void
e_grabinput_release(Ecore_X_Window mouse_win, Ecore_X_Window key_win)
{
   if (mouse_win == grab_mouse_win)
     {
	ecore_x_pointer_ungrab();
	mouse_win = 0;
     }
   if (key_win == grab_key_win)
     {
	ecore_x_keyboard_ungrab();
	grab_key_win = 0;
     }
}

/* local subsystem functions */
