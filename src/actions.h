#ifndef E_ACTIONS_H
#define E_ACTIONS_H

#include "e.h"

typedef struct _E_Action              E_Action;
typedef struct _E_Action_Impl         E_Action_Impl;
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
   void *object;
   char *name;
};

struct _E_Action
{
   OBJ_PROPERTIES;
   
   char           *name;
   char           *action;
   char           *params;
   E_Action_Type   event;
   int             button;
   char           *key;
   int             modifiers;
   E_Action_Impl  *action_impl;
   void           *object;
   int             started;
   int             grabbed;
};

struct _E_Action_Impl
{
   OBJ_PROPERTIES;
   
   char  *action;
   void (*func_start) (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);
   void (*func_cont)  (void *o, E_Action *a, void *data, int x, int y, int rx, int ry, int dx, int dy);
   void (*func_stop)  (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);
};


/**
 * e_action_init - Actions implementation initializer
 *
 * This function registers the various action implementations,
 * i.e. the way E performs actions.
 */
void e_action_init(void);

int  e_action_start(char *action, E_Action_Type act, int button, char *key,
		    Ecore_Event_Key_Modifiers mods, void *o, void *data,
		    int x, int y, int rx, int ry);
void e_action_stop(char *action, E_Action_Type act, int button, char *key,
		   Ecore_Event_Key_Modifiers mods, void *o, void *data,
		   int x, int y, int rx, int ry);
void e_action_cont(char *action, E_Action_Type act, int button, char *key,
		   Ecore_Event_Key_Modifiers mods, void *o, void *data,
		   int x, int y, int rx, int ry, int dx, int dy);
void e_action_stop_by_object(void *o, void *data, int x, int y, int rx, int ry);
void e_action_stop_by_type(char *action);
void e_action_add_proto(char *action, 
			void (*func_start) (void *o, E_Action *a, void *data, int x, int y, int rx, int ry),
			void (*func_cont)  (void *o, E_Action *a, void *data, int x, int y, int rx, int ry, int dx, int dy),
			void (*func_stop)  (void *o, E_Action *a, void *data, int x, int y, int rx, int ry));
void e_action_del_timer(void *o, char *name);
void e_action_add_timer(void *o, char *name);
void e_action_del_timer_object(void *o);

#endif
