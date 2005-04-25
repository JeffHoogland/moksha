/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef enum _E_Binding_Context
{
   E_BINDING_CONTEXT_NONE,
   E_BINDING_CONTEXT_UNKOWN,
   E_BINDING_CONTEXT_BORDER,
   E_BINDING_CONTEXT_ZONE
} E_Binding_Context;

/* why do we do this? config stored bindings must be fixed. x's modifier masks
 * may change from time to time, xserver to xserver - so we cant do a 
 * simple match without translating to fixed values
 */
typedef enum _E_Binding_Modifier
{
   E_BINDING_MODIFIER_SHIFT = (1 << 0),
   E_BINDING_MODIFIER_CTRL = (1 << 1),
   E_BINDING_MODIFIER_ALT = (1 << 2),
   E_BINDING_MODIFIER_WIN = (1 << 3)
} E_Binding_Modifier;

#else
#ifndef E_BINDINGS_H
#define E_BINDINGS_H

EAPI int         e_bindings_init(void);
EAPI int         e_bindings_shutdown(void);

EAPI int         e_bindings_mouse_down_event_handle(E_Binding_Context ctxt, E_Object *obj, Ecore_X_Event_Mouse_Button_Down *ev);
EAPI int         e_bindings_key_down_event_handle(E_Binding_Context ctxt, E_Object *obj, Ecore_X_Event_Key_Down *ev);
					 
#endif
#endif
