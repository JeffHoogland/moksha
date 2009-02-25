/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef enum _E_Binding_Context
{
   E_BINDING_CONTEXT_NONE,
   E_BINDING_CONTEXT_UNKNOWN,
   E_BINDING_CONTEXT_BORDER,
   E_BINDING_CONTEXT_ZONE,
   E_BINDING_CONTEXT_CONTAINER,
   E_BINDING_CONTEXT_MANAGER,
   E_BINDING_CONTEXT_MENU,
   E_BINDING_CONTEXT_WINLIST,
   E_BINDING_CONTEXT_POPUP,
   E_BINDING_CONTEXT_ANY
} E_Binding_Context;

/* why do we do this? config stored bindings must be fixed. x's modifier masks
 * may change from time to time, xserver to xserver - so we cant do a 
 * simple match without translating to fixed values
 */
typedef enum _E_Binding_Modifier
{
   E_BINDING_MODIFIER_NONE = 0,
   E_BINDING_MODIFIER_SHIFT = (1 << 0),
   E_BINDING_MODIFIER_CTRL = (1 << 1),
   E_BINDING_MODIFIER_ALT = (1 << 2),
   E_BINDING_MODIFIER_WIN = (1 << 3)
} E_Binding_Modifier;

typedef struct _E_Binding_Mouse  E_Binding_Mouse;
typedef struct _E_Binding_Key    E_Binding_Key;
typedef struct _E_Binding_Edge   E_Binding_Edge;
typedef struct _E_Binding_Signal E_Binding_Signal;
typedef struct _E_Binding_Wheel  E_Binding_Wheel;

#else
#ifndef E_BINDINGS_H
#define E_BINDINGS_H

struct _E_Binding_Mouse
{
   E_Binding_Context ctxt;
   int button;
   E_Binding_Modifier mod;
   unsigned char any_mod : 1;
   const char *action;
   const char *params;
};

struct _E_Binding_Key
{
   E_Binding_Context ctxt;
   const char *key;
   E_Binding_Modifier mod;
   unsigned char any_mod : 1;
   const char *action;
   const char *params;
};

struct _E_Binding_Edge
{
   E_Binding_Context ctxt;
   E_Zone_Edge edge;
   E_Binding_Modifier mod;
   unsigned char any_mod : 1;
   const char *action;
   const char *params;

   float delay;
   Ecore_Timer *timer;
};

struct _E_Binding_Signal
{
   E_Binding_Context ctxt;
   const char *sig;
   const char *src;
   E_Binding_Modifier mod;
   unsigned char any_mod : 1;
   const char *action;
   const char *params;
};

struct _E_Binding_Wheel
{
   E_Binding_Context ctxt;
   int direction;
   int z;
   E_Binding_Modifier mod;
   unsigned char any_mod : 1;
   const char *action;
   const char *params;
};

EAPI int         e_bindings_init(void);
EAPI int         e_bindings_shutdown(void);

EAPI void        e_bindings_mouse_add(E_Binding_Context ctxt, int button, E_Binding_Modifier mod, int any_mod, const char *action, const char *params);
EAPI void        e_bindings_mouse_del(E_Binding_Context ctxt, int button, E_Binding_Modifier mod, int any_mod, const char *action, const char *params);
EAPI void        e_bindings_mouse_grab(E_Binding_Context ctxt, Ecore_X_Window win);
EAPI void        e_bindings_mouse_ungrab(E_Binding_Context ctxt, Ecore_X_Window win);
EAPI E_Action   *e_bindings_mouse_down_find(E_Binding_Context ctxt, E_Object *obj, Ecore_X_Event_Mouse_Button_Down *ev, E_Binding_Mouse **bind_ret);
EAPI E_Action   *e_bindings_mouse_down_event_handle(E_Binding_Context ctxt, E_Object *obj, Ecore_X_Event_Mouse_Button_Down *ev);
EAPI E_Action   *e_bindings_mouse_up_find(E_Binding_Context ctxt, E_Object *obj, Ecore_X_Event_Mouse_Button_Up *ev, E_Binding_Mouse **bind_ret);
EAPI E_Action   *e_bindings_mouse_up_event_handle(E_Binding_Context ctxt, E_Object *obj, Ecore_X_Event_Mouse_Button_Up *ev);

EAPI void        e_bindings_key_add(E_Binding_Context ctxt, const char *key, E_Binding_Modifier mod, int any_mod, const char *action, const char *params);
EAPI void        e_bindings_key_del(E_Binding_Context ctxt, const char *key, E_Binding_Modifier mod, int any_mod, const char *action, const char *params);
EAPI E_Binding_Key *e_bindings_key_get(const char *action);
EAPI void        e_bindings_key_grab(E_Binding_Context ctxt, Ecore_X_Window win);
EAPI void        e_bindings_key_ungrab(E_Binding_Context ctxt, Ecore_X_Window win);
EAPI E_Action   *e_bindings_key_down_event_handle(E_Binding_Context ctxt, E_Object *obj, Ecore_X_Event_Key_Down *ev);
EAPI E_Action   *e_bindings_key_up_event_handle(E_Binding_Context ctxt, E_Object *obj, Ecore_X_Event_Key_Up *ev);

EAPI void        e_bindings_edge_add(E_Binding_Context ctxt, E_Zone_Edge edge, E_Binding_Modifier mod, int any_mod, const char *action, const char *params, float delay);
EAPI E_Binding_Edge *e_bindings_edge_get(const char *action, E_Zone_Edge edge);
EAPI void        e_bindings_edge_del(E_Binding_Context ctxt, E_Zone_Edge edge, E_Binding_Modifier mod, int any_mod, const char *action, const char *params, float delay);
EAPI E_Action   *e_bindings_edge_in_event_handle(E_Binding_Context ctxt, E_Object *obj, E_Event_Zone_Edge *ev);
EAPI E_Action   *e_bindings_edge_out_event_handle(E_Binding_Context ctxt, E_Object *obj, E_Event_Zone_Edge *ev);

EAPI void        e_bindings_signal_add(E_Binding_Context ctxt, const char *sig, const char *src, E_Binding_Modifier mod, int any_mod, const char *action, const char *params);
EAPI void        e_bindings_signal_del(E_Binding_Context ctxt, const char *sig, const char *src, E_Binding_Modifier mod, int any_mod, const char *action, const char *params);
EAPI E_Action   *e_bindings_signal_find(E_Binding_Context ctxt, E_Object *obj, const char *sig, const char *src, E_Binding_Signal **bind_ret);
EAPI E_Action   *e_bindings_signal_handle(E_Binding_Context ctxt, E_Object *obj, const char *sig, const char *src);
					 
EAPI void        e_bindings_wheel_add(E_Binding_Context ctxt, int direction, int z, E_Binding_Modifier mod, int any_mod, const char *action, const char *params);
EAPI void        e_bindings_wheel_del(E_Binding_Context ctxt, int direction, int z, E_Binding_Modifier mod, int any_mod, const char *action, const char *params);
EAPI void        e_bindings_wheel_grab(E_Binding_Context ctxt, Ecore_X_Window win);
EAPI void        e_bindings_wheel_ungrab(E_Binding_Context ctxt, Ecore_X_Window win);
EAPI E_Action   *e_bindings_wheel_find(E_Binding_Context ctxt, E_Object *obj, Ecore_X_Event_Mouse_Wheel *ev, E_Binding_Wheel **bind_ret);
EAPI E_Action   *e_bindings_wheel_event_handle(E_Binding_Context ctxt, E_Object *obj, Ecore_X_Event_Mouse_Wheel *ev);

#endif
#endif
