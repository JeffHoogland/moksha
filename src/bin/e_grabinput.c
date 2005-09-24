/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* local subsystem functions */
static void _e_grabinput_focus(Ecore_X_Window win, E_Focus_Method method);

/* local subsystem globals */
static Ecore_X_Window grab_mouse_win = 0;
static Ecore_X_Window grab_key_win = 0;
static Ecore_X_Window focus_win = 0;
static E_Focus_Method focus_method = E_FOCUS_METHOD_NO_INPUT;
static double last_focus_time = 0.0;

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
	focus_win = 0;
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
	if (focus_win != 0)
	  {
	     _e_grabinput_focus(focus_win, focus_method);
	     focus_win = 0;
	     focus_method = E_FOCUS_METHOD_NO_INPUT;
	  }
     }
}

void
e_grabinput_focus(Ecore_X_Window win, E_Focus_Method method)
{
   if (grab_key_win != 0)
     {
	focus_win = win;
	focus_method = method;
     }
   else
     _e_grabinput_focus(win, method);
}

double
e_grabinput_last_focus_time_get(void)
{
   return last_focus_time;
}

/* local subsystem functions */
static void
_e_grabinput_focus(Ecore_X_Window win, E_Focus_Method method)
{
   switch (method)
     {
      case E_FOCUS_METHOD_NO_INPUT:
	break;
      case E_FOCUS_METHOD_LOCALLY_ACTIVE:
	ecore_x_window_focus(win);
	ecore_x_icccm_take_focus_send(win, ecore_x_current_time_get());
	break;
      case E_FOCUS_METHOD_GLOBALLY_ACTIVE:
	ecore_x_icccm_take_focus_send(win, ecore_x_current_time_get());
	break;
      case E_FOCUS_METHOD_PASSIVE:
	ecore_x_window_focus(win);
	break;
      default:
	break;
     }
   last_focus_time = ecore_time_get();
}
