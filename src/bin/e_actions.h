/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef struct _E_Action E_Action;

struct _E_Action
{
   char *name;
   struct {
      void (*go) (E_Object *obj, char *params);
      void (*go_mouse) (E_Object *obj, char *params, Ecore_X_Event_Mouse_Button_Down *ev);
   } func;
};

#else
#ifndef E_ACTIONS_H
#define E_ACTIONS_H

EAPI int         e_actions_init(void);
EAPI int         e_actions_shutdown(void);

EAPI E_Action   *e_action_find(char *name);

#endif
#endif
