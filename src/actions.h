#ifndef E_ACTIONS_H
#define E_ACTIONS_H

#include "e.h"

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
   int             event;
   int             button;
   char           *key;
   int             modifiers;
   E_Action_Proto *action_proto;
   void           *object;
   int             started;
   int             grabbed;
};

struct _E_Action_Proto
{
   OBJ_PROPERTIES;
   
   char  *action;
   void (*func_start) (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);
   void (*func_stop)  (void *o, E_Action *a, void *data, int x, int y, int rx, int ry);
   void (*func_go)    (void *o, E_Action *a, void *data, int x, int y, int rx, int ry, int dx, int dy);
};


void e_action_start(char *action, int act, int button, char *key,
		    Ev_Key_Modifiers mods, void *o, void *data,
		    int x, int y, int rx, int ry);
void e_action_stop(char *action, int act, int button, char *key,
		   Ev_Key_Modifiers mods, void *o, void *data,
		   int x, int y, int rx, int ry);
void e_action_go(char *action, int act, int button, char *key,
		 Ev_Key_Modifiers mods, void *o, void *data,
		 int x, int y, int rx, int ry, int dx, int dy);
void e_action_stop_by_object(void *o, void *data, int x, int y, int rx, int ry);
void e_action_add_proto(char *action, 
			void (*func_start) (void *o, E_Action *a, void *data, int x, int y, int rx, int ry),
			void (*func_stop)  (void *o, E_Action *a, void *data, int x, int y, int rx, int ry),
			void (*func_go)    (void *o, E_Action *a, void *data, int x, int y, int rx, int ry, int dx, int dy));
void e_action_del_timer(void *o, char *name);
void e_action_add_timer(void *o, char *name);
void e_action_del_timer_object(void *o);
void e_action_init(void);

#endif
