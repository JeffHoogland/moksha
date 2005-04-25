/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* local subsystem functions */

/* local subsystem globals */

/* externally accessible functions */

int
e_bindings_init(void)
{
   return 1;
}

int
e_bindings_shutdown(void)
{
   return 1;
}

void
e_bindings_mouse_add(E_Binding_Context ctxt, int button, E_Binding_Modifier mod, int any_mod, char *action, char *param)
{
}

void
e_bindings_mouse_grab(Ecore_X_Window win)
{
}

void
e_bindings_mouse_ungrab(Ecore_X_Window win)
{
}

int
e_bindings_mouse_down_event_handle(E_Binding_Context ctxt, E_Object *obj, Ecore_X_Event_Mouse_Button_Down *ev)
{
   E_Binding_Modifier mod = 0;
   Evas_List *l;
   
   if (ev->modifiers & ECORE_X_MODIFIER_SHIFT) mod |= E_BINDING_MODIFIER_SHIFT;
   if (ev->modifiers & ECORE_X_MODIFIER_CTRL) mod |= E_BINDING_MODIFIER_CTRL;
   if (ev->modifiers & ECORE_X_MODIFIER_ALT) mod |= E_BINDING_MODIFIER_ALT;
   if (ev->modifiers & ECORE_X_MODIFIER_WIN) mod |= E_BINDING_MODIFIER_WIN;
   
   return 0;
}

int
e_bindings_key_down_event_handle(E_Binding_Context ctxt, E_Object *obj, Ecore_X_Event_Key_Down *ev)
{
   E_Binding_Modifier mod = 0;
   Evas_List *l;

   if (ev->modifiers & ECORE_X_MODIFIER_SHIFT) mod |= E_BINDING_MODIFIER_SHIFT;
   if (ev->modifiers & ECORE_X_MODIFIER_CTRL) mod |= E_BINDING_MODIFIER_CTRL;
   if (ev->modifiers & ECORE_X_MODIFIER_ALT) mod |= E_BINDING_MODIFIER_ALT;
   if (ev->modifiers & ECORE_X_MODIFIER_WIN) mod |= E_BINDING_MODIFIER_WIN;
   
   return 0;
}

/* local subsystem functions */
