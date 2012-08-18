#include "e.h"

/* local subsystem functions */
static Eina_Bool _e_grabinput_focus_check(void *data);
static void _e_grabinput_focus_do(Ecore_X_Window win, E_Focus_Method method);
static void _e_grabinput_focus(Ecore_X_Window win, E_Focus_Method method);

/* local subsystem globals */
static Ecore_X_Window grab_mouse_win = 0;
static Ecore_X_Window grab_key_win = 0;
static Ecore_X_Window focus_win = 0;
static E_Focus_Method focus_method = E_FOCUS_METHOD_NO_INPUT;
static double last_focus_time = 0.0;

static Ecore_X_Window focus_fix_win = 0;
static Ecore_Timer *focus_fix_timer = NULL;
static E_Focus_Method focus_fix_method = E_FOCUS_METHOD_NO_INPUT;

/* externally accessible functions */
EINTERN int
e_grabinput_init(void)
{
   return 1;
}

EINTERN int
e_grabinput_shutdown(void)
{
   if (focus_fix_timer)
     {
        ecore_timer_del(focus_fix_timer);
        focus_fix_timer = NULL;
     }
   return 1;
}

EAPI int
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
        int ret = 0;

        if (confine_mouse)
          ret = ecore_x_pointer_confine_grab(mouse_win);
        else
          ret = ecore_x_pointer_grab(mouse_win);
        if (!ret) return 0;
        grab_mouse_win = mouse_win;
     }
   if (key_win)
     {
        int ret = 0;

        ret = ecore_x_keyboard_grab(key_win);
        if (!ret)
          {
             if (grab_mouse_win)
               {
                  ecore_x_pointer_ungrab();
                  grab_mouse_win = 0;
               }
             return 0;
          }
        grab_key_win = key_win;
     }
   return 1;
}

EAPI void
e_grabinput_release(Ecore_X_Window mouse_win, Ecore_X_Window key_win)
{
   if (mouse_win == grab_mouse_win)
     {
        ecore_x_pointer_ungrab();
        grab_mouse_win = 0;
     }
   if (key_win == grab_key_win)
     {
        ecore_x_keyboard_ungrab();
        grab_key_win = 0;
        if (focus_win != 0)
          {
             fprintf(stderr, "release focus to %x\n", focus_win);
             _e_grabinput_focus(focus_win, focus_method);
             focus_win = 0;
             focus_method = E_FOCUS_METHOD_NO_INPUT;
          }
     }
}

EAPI void
e_grabinput_focus(Ecore_X_Window win, E_Focus_Method method)
{
   if (grab_key_win != 0)
     {
        fprintf(stderr, "while grabbed focus changed to %x\n", win);
        focus_win = win;
        focus_method = method;
     }
   else
     {
        fprintf(stderr, "focus to %x\n", win);
        _e_grabinput_focus(win, method);
     }
}

EAPI double
e_grabinput_last_focus_time_get(void)
{
   return last_focus_time;
}

EAPI Ecore_X_Window
e_grabinput_last_focus_win_get(void)
{
   return focus_fix_win;
}

static Eina_Bool
_e_grabinput_focus_check(void *data __UNUSED__)
{
   if (ecore_x_window_focus_get() != focus_fix_win)
     {
        fprintf(stderr, "foc do 2\n");
        _e_grabinput_focus_do(focus_fix_win, focus_fix_method);
     }
   focus_fix_timer = NULL;
   return EINA_FALSE;
}

static void
_e_grabinput_focus_do(Ecore_X_Window win, E_Focus_Method method)
{
   fprintf(stderr, "focus to %x method %i\n", win, method);
   switch (method)
     {
      case E_FOCUS_METHOD_NO_INPUT:
        break;
      case E_FOCUS_METHOD_LOCALLY_ACTIVE:
        ecore_x_window_focus_at_time(win, ecore_x_current_time_get());
        ecore_x_icccm_take_focus_send(win, ecore_x_current_time_get());
        break;
      case E_FOCUS_METHOD_GLOBALLY_ACTIVE:
        ecore_x_icccm_take_focus_send(win, ecore_x_current_time_get());
        break;
      case E_FOCUS_METHOD_PASSIVE:
        ecore_x_window_focus_at_time(win, ecore_x_current_time_get());
        break;
      default:
        break;
     }
}

static void
_e_grabinput_focus(Ecore_X_Window win, E_Focus_Method method)
{
   focus_fix_win = win;
   focus_fix_method = method;
   fprintf(stderr, "foc do 1\n");
   _e_grabinput_focus_do(win, method);
   last_focus_time = ecore_loop_time_get();
   if (focus_fix_timer) ecore_timer_del(focus_fix_timer);
   focus_fix_timer = ecore_timer_add(0.2, _e_grabinput_focus_check, NULL);
}
