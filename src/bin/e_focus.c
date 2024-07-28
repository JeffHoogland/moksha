#include "e.h"

/* local subsystem functions */
static Eina_Bool _e_focus_raise_timer(void *data);

/* local subsystem globals */

/* externally accessible functions */
EINTERN int
e_focus_init(void)
{
   return 1;
}

EINTERN int
e_focus_shutdown(void)
{
   return 1;
}

EAPI void
e_focus_idler_before(void)
{
   return;
}

EAPI void
e_focus_event_mouse_in(E_Border *bd)
{
   if ((e_config->focus_policy == E_FOCUS_MOUSE) ||
       (e_config->focus_policy == E_FOCUS_SLOPPY))
     {
         if (bd != e_border_focused_get())
          e_border_focus_set(bd, 1, 1);
     }
   if (bd->raise_timer) ecore_timer_del(bd->raise_timer);
   bd->raise_timer = NULL;
   if (e_config->use_auto_raise)
     {
        if (eina_dbl_exact(e_config->auto_raise_delay, 0.0))
          {
             if (!bd->lock_user_stacking)
               {
                  if (e_config->border_raise_on_focus)
                    e_border_raise(bd);
               }
          }
        else
          bd->raise_timer = ecore_timer_add(e_config->auto_raise_delay, _e_focus_raise_timer, bd);
     }
}

EAPI void
e_focus_event_mouse_out(E_Border *bd)
{
   if (e_config->focus_policy == E_FOCUS_MOUSE)
     {
        if (!bd->lock_focus_in)
           {
             if (bd->focused)
               e_border_focus_set(bd, 0, 1);
           }
      }
   E_FN_DEL(ecore_timer_del, bd->raise_timer);
}

EAPI void
e_focus_event_mouse_down(E_Border *bd)
{
   if (!bd->focused)
     {
        if (e_border_focus_policy_click(bd))
          e_border_focus_set(bd, 1, 1);
        else if (e_config->always_click_to_focus)
          e_border_focus_set(bd, 1, 1);
     }
   if (e_config->always_click_to_raise)
     {
        if (!bd->lock_user_stacking)
            e_border_raise(bd);
     }
}

EAPI void
e_focus_event_mouse_up(E_Border *bd __UNUSED__)
{
}

EAPI void
e_focus_event_focus_in(E_Border *bd)
{
   if ((e_border_focus_policy_click(bd)) && 
       (!e_config->always_click_to_raise) &&
       (!e_config->always_click_to_focus))
     {
        if (!bd->button_grabbed) return;
        e_bindings_mouse_ungrab(E_BINDING_CONTEXT_WINDOW, bd->win);
        e_bindings_wheel_ungrab(E_BINDING_CONTEXT_WINDOW, bd->win);
        ecore_x_window_button_ungrab(bd->win, 1, 0, 1);
        ecore_x_window_button_ungrab(bd->win, 2, 0, 1);
        ecore_x_window_button_ungrab(bd->win, 3, 0, 1);
        e_bindings_mouse_grab(E_BINDING_CONTEXT_WINDOW, bd->win);
        e_bindings_wheel_grab(E_BINDING_CONTEXT_WINDOW, bd->win);
        bd->button_grabbed = 0;
     }
    if (!bd->lock_user_stacking)
     {
        if (e_config->border_raise_on_focus)
          e_border_raise(bd);
     }    
}

EAPI void
e_focus_event_focus_out(E_Border *bd)
{
   if ((e_border_focus_policy_click(bd)) &&
       (!e_config->always_click_to_raise) &&
       (!e_config->always_click_to_focus))
     {
        if (bd->button_grabbed) return;
        ecore_x_window_button_grab(bd->win, 1,
                                   ECORE_X_EVENT_MASK_MOUSE_DOWN |
                                   ECORE_X_EVENT_MASK_MOUSE_UP |
                                   ECORE_X_EVENT_MASK_MOUSE_MOVE, 0, 1);
        ecore_x_window_button_grab(bd->win, 2,
                                   ECORE_X_EVENT_MASK_MOUSE_DOWN |
                                   ECORE_X_EVENT_MASK_MOUSE_UP |
                                   ECORE_X_EVENT_MASK_MOUSE_MOVE, 0, 1);
        ecore_x_window_button_grab(bd->win, 3,
                                   ECORE_X_EVENT_MASK_MOUSE_DOWN |
                                   ECORE_X_EVENT_MASK_MOUSE_UP |
                                   ECORE_X_EVENT_MASK_MOUSE_MOVE, 0, 1);
        bd->button_grabbed = 1;
     }
}

EAPI void
e_focus_setup(E_Border *bd)
{
   if ((e_border_focus_policy_click(bd)) ||
       (e_config->always_click_to_raise) ||
       (e_config->always_click_to_focus))
     {
        if (bd->button_grabbed) return;
        ecore_x_window_button_grab(bd->win, 1,
                                   ECORE_X_EVENT_MASK_MOUSE_DOWN |
                                   ECORE_X_EVENT_MASK_MOUSE_UP |
                                   ECORE_X_EVENT_MASK_MOUSE_MOVE, 0, 1);
        ecore_x_window_button_grab(bd->win, 2,
                                   ECORE_X_EVENT_MASK_MOUSE_DOWN |
                                   ECORE_X_EVENT_MASK_MOUSE_UP |
                                   ECORE_X_EVENT_MASK_MOUSE_MOVE, 0, 1);
        ecore_x_window_button_grab(bd->win, 3,
                                   ECORE_X_EVENT_MASK_MOUSE_DOWN |
                                   ECORE_X_EVENT_MASK_MOUSE_UP |
                                   ECORE_X_EVENT_MASK_MOUSE_MOVE, 0, 1);
        bd->button_grabbed = 1;
     }
}

EAPI void
e_focus_setdown(E_Border *bd)
{
   if (!bd->button_grabbed) return;
   e_bindings_mouse_ungrab(E_BINDING_CONTEXT_WINDOW, bd->win);
   e_bindings_wheel_ungrab(E_BINDING_CONTEXT_WINDOW, bd->win);
   ecore_x_window_button_ungrab(bd->win, 1, 0, 1);
   ecore_x_window_button_ungrab(bd->win, 2, 0, 1);
   ecore_x_window_button_ungrab(bd->win, 3, 0, 1);
   e_bindings_mouse_grab(E_BINDING_CONTEXT_WINDOW, bd->win);
   e_bindings_wheel_grab(E_BINDING_CONTEXT_WINDOW, bd->win);
   bd->button_grabbed = 0;
}

/* local subsystem functions */
static Eina_Bool
_e_focus_raise_timer(void *data)
{
   E_Border *bd;

   bd = data;
   if (!bd->lock_user_stacking) e_border_raise(bd);
   bd->raise_timer = NULL;
   return ECORE_CALLBACK_CANCEL;
}

