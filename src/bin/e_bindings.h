#ifdef E_TYPEDEFS

typedef enum _E_Binding_Context
{
   E_BINDING_CONTEXT_NONE,
   E_BINDING_CONTEXT_UNKNOWN,
   E_BINDING_CONTEXT_WINDOW,
   E_BINDING_CONTEXT_ZONE,
   E_BINDING_CONTEXT_CONTAINER,
   E_BINDING_CONTEXT_MANAGER,
   E_BINDING_CONTEXT_MENU,
   E_BINDING_CONTEXT_WINLIST,
   E_BINDING_CONTEXT_POPUP,
   E_BINDING_CONTEXT_ANY
} E_Binding_Context;

/* why do we do this? config stored bindings must be fixed. x's modifier masks
 * may change from time to time, xserver to xserver - so we can't do a
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
typedef struct _E_Binding_Acpi   E_Binding_Acpi;

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
   unsigned char drag_only : 1;
   const char *action;
   const char *params;
};

struct _E_Binding_Edge
{
   E_Binding_Context ctxt;
   E_Zone_Edge edge;
   E_Binding_Modifier mod;
   unsigned char any_mod : 1;
   unsigned char drag_only : 1;
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

struct _E_Binding_Acpi
{
   E_Binding_Context ctxt;
   int type, status;
   const char *action, *params;
};

EINTERN int         e_bindings_init(void);
EINTERN int         e_bindings_shutdown(void);

EAPI void        e_bindings_mouse_reset(void);
EAPI void        e_bindings_key_reset(void);
EAPI void        e_bindings_wheel_reset(void);
EAPI void        e_bindings_edge_reset(void);
EAPI void        e_bindings_signal_reset(void);
EAPI void        e_bindings_reset(void);

EAPI void        e_bindings_mouse_add(E_Binding_Context ctxt, int button, E_Binding_Modifier mod, int any_mod, const char *action, const char *params);
EAPI void        e_bindings_mouse_del(E_Binding_Context ctxt, int button, E_Binding_Modifier mod, int any_mod, const char *action, const char *params);
EAPI void        e_bindings_mouse_grab(E_Binding_Context ctxt, Ecore_X_Window win);
EAPI void        e_bindings_mouse_ungrab(E_Binding_Context ctxt, Ecore_X_Window win);
EAPI E_Action   *e_bindings_mouse_down_find(E_Binding_Context ctxt, E_Object *obj, Ecore_Event_Mouse_Button *ev, E_Binding_Mouse **bind_ret);
EAPI E_Action   *e_bindings_mouse_down_event_handle(E_Binding_Context ctxt, E_Object *obj, Ecore_Event_Mouse_Button *ev);
EAPI E_Action   *e_bindings_mouse_up_find(E_Binding_Context ctxt, E_Object *obj, Ecore_Event_Mouse_Button *ev, E_Binding_Mouse **bind_ret);
EAPI E_Action   *e_bindings_mouse_up_event_handle(E_Binding_Context ctxt, E_Object *obj, Ecore_Event_Mouse_Button *ev);

EAPI void        e_bindings_key_add(E_Binding_Context ctxt, const char *key, E_Binding_Modifier mod, int any_mod, const char *action, const char *params);
EAPI void        e_bindings_key_del(E_Binding_Context ctxt, const char *key, E_Binding_Modifier mod, int any_mod, const char *action, const char *params);
EAPI E_Binding_Key *e_bindings_key_get(const char *action);
EAPI E_Binding_Key *e_bindings_key_find(const char *key, E_Binding_Modifier mod, int any_mod);
EAPI void        e_bindings_key_grab(E_Binding_Context ctxt, Ecore_X_Window win);
EAPI void        e_bindings_key_ungrab(E_Binding_Context ctxt, Ecore_X_Window win);
EAPI E_Action   *e_bindings_key_down_event_handle(E_Binding_Context ctxt, E_Object *obj, Ecore_Event_Key *ev);
EAPI E_Action   *e_bindings_key_up_event_handle(E_Binding_Context ctxt, E_Object *obj, Ecore_Event_Key *ev);
EAPI E_Action   *e_bindings_key_down_event_find(E_Binding_Context ctxt, Ecore_Event_Key *ev);
EAPI E_Action   *e_bindings_key_up_event_find(E_Binding_Context ctxt, Ecore_Event_Key *ev);

EAPI void        e_bindings_edge_add(E_Binding_Context ctxt, E_Zone_Edge edge, Eina_Bool drag_only, E_Binding_Modifier mod, int any_mod, const char *action, const char *params, float delay);
EAPI Eina_Bool   e_bindings_edge_flippable_get(E_Zone_Edge edge);
EAPI Eina_Bool   e_bindings_edge_non_flippable_get(E_Zone_Edge edge);
EAPI E_Binding_Edge *e_bindings_edge_get(const char *action, E_Zone_Edge edge, int click);
EAPI void        e_bindings_edge_del(E_Binding_Context ctxt, E_Zone_Edge edge, Eina_Bool drag_only, E_Binding_Modifier mod, int any_mod, const char *action, const char *params, float delay);
EAPI E_Action   *e_bindings_edge_in_event_handle(E_Binding_Context ctxt, E_Object *obj, E_Event_Zone_Edge *ev);
EAPI E_Action   *e_bindings_edge_out_event_handle(E_Binding_Context ctxt, E_Object *obj, E_Event_Zone_Edge *ev);
EAPI E_Action   *e_bindings_edge_down_event_handle(E_Binding_Context ctxt, E_Object *obj, E_Event_Zone_Edge *ev);
EAPI E_Action   *e_bindings_edge_up_event_handle(E_Binding_Context ctxt, E_Object *obj, E_Event_Zone_Edge *ev);

EAPI void        e_bindings_signal_add(E_Binding_Context ctxt, const char *sig, const char *src, E_Binding_Modifier mod, int any_mod, const char *action, const char *params);
EAPI void        e_bindings_signal_del(E_Binding_Context ctxt, const char *sig, const char *src, E_Binding_Modifier mod, int any_mod, const char *action, const char *params);
EAPI E_Action   *e_bindings_signal_find(E_Binding_Context ctxt, E_Object *obj, const char *sig, const char *src, E_Binding_Signal **bind_ret);
EAPI E_Action   *e_bindings_signal_handle(E_Binding_Context ctxt, E_Object *obj, const char *sig, const char *src);

EAPI void        e_bindings_wheel_add(E_Binding_Context ctxt, int direction, int z, E_Binding_Modifier mod, int any_mod, const char *action, const char *params);
EAPI void        e_bindings_wheel_del(E_Binding_Context ctxt, int direction, int z, E_Binding_Modifier mod, int any_mod, const char *action, const char *params);
EAPI void        e_bindings_wheel_grab(E_Binding_Context ctxt, Ecore_X_Window win);
EAPI void        e_bindings_wheel_ungrab(E_Binding_Context ctxt, Ecore_X_Window win);
EAPI E_Action   *e_bindings_wheel_find(E_Binding_Context ctxt, E_Object *obj, Ecore_Event_Mouse_Wheel *ev, E_Binding_Wheel **bind_ret);
EAPI E_Action   *e_bindings_wheel_event_handle(E_Binding_Context ctxt, E_Object *obj, Ecore_Event_Mouse_Wheel *ev);

EAPI void e_bindings_acpi_add(E_Binding_Context ctxt, int type, int status, const char *action, const char *params);
EAPI void e_bindings_acpi_del(E_Binding_Context ctxt, int type, int status, const char *action, const char *params);
EAPI E_Action *e_bindings_acpi_find(E_Binding_Context ctxt, E_Object *obj, E_Event_Acpi *ev, E_Binding_Acpi **bind_ret);
EAPI E_Action *e_bindings_acpi_event_handle(E_Binding_Context ctxt, E_Object *obj, E_Event_Acpi *ev);
EAPI void e_bindings_mapping_change_enable(Eina_Bool enable);

#endif
#endif
