/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef struct _E_Action E_Action;

struct _E_Action
{
   char *name;
   struct {
      void (*go)        (E_Object *obj, char *params);
      void (*go_mouse)  (E_Object *obj, char *params, Ecore_X_Event_Mouse_Button_Down *ev);
      void (*go_key)    (E_Object *obj, char *params, Ecore_X_Event_Key_Down *ev);
      void (*end)       (E_Object *obj, char *params);
      void (*end_mouse) (E_Object *obj, char *params, Ecore_X_Event_Mouse_Button_Up *ev);
      void (*end_key)   (E_Object *obj, char *params, Ecore_X_Event_Key_Up *ev);
   } func;
};

#else
#ifndef E_ACTIONS_H
#define E_ACTIONS_H

EAPI int         e_actions_init(void);
EAPI int         e_actions_shutdown(void);

EAPI E_Action   *e_action_find(char *name);
EAPI E_Action   *e_action_set(char *name);
EAPI void        e_action_del(char *name);
    
#endif
#endif
