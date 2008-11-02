/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* local subsystem functions */
static int _e_focus_raise_timer(void* data);

/* local subsystem globals */

/* externally accessible functions */
EAPI int
e_focus_init(void)
{
   return 1;
}

EAPI int
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
e_focus_event_mouse_in(E_Border* bd)
{
   if ((e_config->focus_policy == E_FOCUS_MOUSE) ||
       (e_config->focus_policy == E_FOCUS_SLOPPY))
     {
	if (!bd->lock_focus_out)
	  e_border_focus_set(bd, 1, 1);
     }
   if (bd->raise_timer) ecore_timer_del(bd->raise_timer);
   bd->raise_timer = NULL;
   if (e_config->use_auto_raise)
     {
	if (e_config->auto_raise_delay == 0.0)
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
e_focus_event_mouse_out(E_Border* bd)
{
   if (e_config->focus_policy == E_FOCUS_MOUSE)
     {
        /* FIXME: this is such a hack. its a big hack around x's async events
	 * as we dont know always exactly what action causes what event
	 * so by waiting more than 0.2 secs before reverting focus to nothing
	 * since we entered root, we are ignoring mouse in's on the root
	 * container for a bit after the mosue may have entered it
	 */
	if ((ecore_loop_time_get() - e_grabinput_last_focus_time_get()) > 0.2)
	  {
	     if (!bd->lock_focus_in)
	       e_border_focus_set(bd, 0, 1);
	  }
     }
   if (bd->raise_timer)
     {
	ecore_timer_del(bd->raise_timer);
	bd->raise_timer = NULL;
     }
}

EAPI void
e_focus_event_mouse_down(E_Border* bd)
{
   if (e_config->focus_policy == E_FOCUS_CLICK)
     {
	if (!bd->lock_focus_out)
	  e_border_focus_set(bd, 1, 1);
	if (!bd->lock_user_stacking)
	  {
	     if (e_config->border_raise_on_focus)
	       e_border_raise(bd);
	  }
     }
   else if (e_config->always_click_to_raise)
     {
	if (!bd->lock_user_stacking)
	  {
	     if (e_config->border_raise_on_focus)
	       e_border_raise(bd);
	  }
     }
   else if (e_config->always_click_to_focus)
     {
	if (!bd->lock_focus_out)
	  {
	     if (!bd->focused)
	       e_border_focus_set(bd, 1, 1);
	  }
     }
}

EAPI void
e_focus_event_mouse_up(E_Border* bd)
{
}

EAPI void
e_focus_event_focus_in(E_Border *bd)
{
   if ((e_config->focus_policy == E_FOCUS_CLICK) &&
       (!e_config->always_click_to_raise) &&
       (!e_config->always_click_to_focus))
     {
	if (!bd->button_grabbed) return;
	e_bindings_mouse_ungrab(E_BINDING_CONTEXT_BORDER, bd->win);
	e_bindings_wheel_ungrab(E_BINDING_CONTEXT_BORDER, bd->win);
	ecore_x_window_button_ungrab(bd->win, 1, 0, 1);
	ecore_x_window_button_ungrab(bd->win, 2, 0, 1);
	ecore_x_window_button_ungrab(bd->win, 3, 0, 1);
	e_bindings_mouse_grab(E_BINDING_CONTEXT_BORDER, bd->win);
	e_bindings_wheel_grab(E_BINDING_CONTEXT_BORDER, bd->win);
	bd->button_grabbed = 0;
     }
}

EAPI void
e_focus_event_focus_out(E_Border *bd)
{
   if ((e_config->focus_policy == E_FOCUS_CLICK) &&
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
   if ((e_config->focus_policy == E_FOCUS_CLICK) ||
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
   e_bindings_mouse_ungrab(E_BINDING_CONTEXT_BORDER, bd->win);
   e_bindings_wheel_ungrab(E_BINDING_CONTEXT_BORDER, bd->win);
   ecore_x_window_button_ungrab(bd->win, 1, 0, 1);
   ecore_x_window_button_ungrab(bd->win, 2, 0, 1);
   ecore_x_window_button_ungrab(bd->win, 3, 0, 1);
   e_bindings_mouse_grab(E_BINDING_CONTEXT_BORDER, bd->win);
   e_bindings_wheel_grab(E_BINDING_CONTEXT_BORDER, bd->win);
   bd->button_grabbed = 0;
}


/* local subsystem functions */
     
static int
_e_focus_raise_timer(void* data)
{
   E_Border *bd;
   
   bd = data;
   if (!bd->lock_user_stacking) e_border_raise(bd);
   bd->raise_timer = NULL;
   return 0;
}
