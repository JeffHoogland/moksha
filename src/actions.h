#ifndef E_ACTIONS_H
#define E_ACTIONS_H

#include "object.h"

typedef struct _E_Action E_Action;
typedef struct _E_Action_Impl E_Action_Impl;
typedef struct _E_Active_Action_Timer E_Active_Action_Timer;

typedef enum e_action_type
{
   ACT_MOUSE_IN,
   ACT_MOUSE_OUT,
   ACT_MOUSE_CLICK,
   ACT_MOUSE_DOUBLE,
   ACT_MOUSE_TRIPLE,
   ACT_MOUSE_UP,
   ACT_MOUSE_CLICKED,
   ACT_MOUSE_MOVE,
   ACT_KEY_DOWN,
   ACT_KEY_UP
}
E_Action_Type;

struct _E_Active_Action_Timer
{
   void               *object;
   char               *name;
};

struct _E_Action
{
   E_Object            o;

   char               *name;
   char               *action;
   char               *params;
   E_Action_Type       event;
   int                 button;
   char               *key;
   int                 modifiers;
   E_Action_Impl      *action_impl;
   E_Object           *object;
   int                 started;
   int                 grabbed;
};

typedef void        (*E_Action_Start_Func) (E_Object * o, E_Action * a,
					    void *data, int x, int y, int rx,
					    int ry);
typedef void        (*E_Action_Cont_Func) (E_Object * o, E_Action * a,
					   void *data, int x, int y, int rx,
					   int ry, int dx, int dy);
typedef void        (*E_Action_Stop_Func) (E_Object * o, E_Action * a,
					   void *data, int x, int y, int rx,
					   int ry);

struct _E_Action_Impl
{
   E_Object            o;

   char               *action;

   E_Action_Start_Func func_start;
   E_Action_Cont_Func  func_cont;
   E_Action_Stop_Func  func_stop;
};

/**
 * e_action_init - Actions implementation initializer
 *
 * This function registers the various action implementations,
 * i.e. the way E performs actions.
 */
void                e_action_init(void);
void                e_action_cleanup(E_Action *a);

int                 e_action_start(char *action, E_Action_Type act, int button,
				   char *key, Ecore_Event_Key_Modifiers mods,
				   E_Object * o, void *data, int x, int y,
				   int rx, int ry);
void                e_action_stop(char *action, E_Action_Type act, int button,
				  char *key, Ecore_Event_Key_Modifiers mods,
				  E_Object * o, void *data, int x, int y,
				  int rx, int ry);
void                e_action_cont(char *action, E_Action_Type act, int button,
				  char *key, Ecore_Event_Key_Modifiers mods,
				  E_Object * o, void *data, int x, int y,
				  int rx, int ry, int dx, int dy);
void                e_action_stop_by_object(E_Object * o, void *data, int x,
					    int y, int rx, int ry);
void                e_action_stop_by_type(char *action);
void                e_action_add_impl(char *action,
				      E_Action_Start_Func func_start,
				      E_Action_Cont_Func func_cont,
				      E_Action_Stop_Func func_stop);
void                e_action_del_timer(E_Object * object, char *name);
void                e_action_add_timer(E_Object * object, char *name);
void                e_action_del_timer_object(E_Object * object);


void                e_act_exit_start(E_Object * object, E_Action * a, void *data, int x, int y,
				     int rx, int ry);
void                e_act_restart_start(E_Object * object, E_Action * a, void *data, int x, int y,
					int rx, int ry);

#endif
