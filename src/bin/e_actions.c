/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

#define INITS 
#define ACT_GO(name) \
   { \
      act = e_action_set(#name); \
      if (act) act->func.go = _e_actions_act_##name##_go; \
   }
#define ACT_FN_GO(act) \
   static void _e_actions_act_##act##_go(E_Object *obj, char *params)
#define ACT_GO_MOUSE(name) \
   { \
      act = e_action_set(#name); \
      if (act) act->func.go_mouse = _e_actions_act_##name##_go_mouse; \
   }
#define ACT_FN_GO_MOUSE(act) \
   static void _e_actions_act_##act##_go_mouse(E_Object *obj, char *params, Ecore_X_Event_Mouse_Button_Down *ev)
#define ACT_END(name) \
   { \
      act = e_action_set(#name); \
      if (act) act->func.end = _e_actions_act_##name##_end; \
   }
#define ACT_FN_END(act) \
   static void _e_actions_act_##act##_end(E_Object *obj, char *params)
#define ACT_END_MOUSE(name) \
   { \
      act = e_action_set(#name); \
      if (act) act->func.end_mouse = _e_actions_act_##name##_end_mouse; \
   }
#define ACT_FN_END_MOUSE(act) \
   static void _e_actions_act_##act##_end_mouse(E_Object *obj, char *params, Ecore_X_Event_Mouse_Button_Up *ev)

/* local subsystem functions */

/* to save writing this in N places - the sctions are defined here */
/***************************************************************************/
ACT_FN_GO(window_move)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (obj->type != E_BORDER_TYPE) return;
   e_border_act_move_begin((E_Border *)obj, NULL);
}
ACT_FN_GO_MOUSE(window_move)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (obj->type != E_BORDER_TYPE) return;
   e_border_act_move_begin((E_Border *)obj, ev);
}
ACT_FN_END(window_move)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (obj->type != E_BORDER_TYPE) return;
   e_border_act_move_end((E_Border *)obj, NULL);
}
ACT_FN_END_MOUSE(window_move)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (obj->type != E_BORDER_TYPE) return;
   e_border_act_move_end((E_Border *)obj, ev);
}

/***************************************************************************/
ACT_FN_GO(window_resize)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (obj->type != E_BORDER_TYPE) return;
   e_border_act_resize_begin((E_Border *)obj, NULL);
}
ACT_FN_GO_MOUSE(window_resize)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (obj->type != E_BORDER_TYPE) return;
   e_border_act_resize_begin((E_Border *)obj, ev);
}
ACT_FN_END(window_resize)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (obj->type != E_BORDER_TYPE) return;
   e_border_act_resize_end((E_Border *)obj, NULL);
}
ACT_FN_END_MOUSE(window_resize)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (obj->type != E_BORDER_TYPE) return;
   e_border_act_resize_end((E_Border *)obj, ev);
}

/***************************************************************************/
ACT_FN_GO(window_menu)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (obj->type != E_BORDER_TYPE) return;
   e_border_act_menu_begin((E_Border *)obj, NULL);
}
ACT_FN_GO_MOUSE(window_menu)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (obj->type != E_BORDER_TYPE) return;
   e_border_act_menu_begin((E_Border *)obj, ev);
}

/* local subsystem globals */
static Evas_Hash *actions = NULL;

/* externally accessible functions */

int
e_actions_init(void)
{
   E_Action *act;

   ACT_GO(window_move);
   ACT_GO_MOUSE(window_move);
   ACT_END(window_move);
   ACT_END_MOUSE(window_move);
   
   ACT_GO(window_resize);
   ACT_GO_MOUSE(window_resize);
   ACT_END(window_resize);
   ACT_END_MOUSE(window_resize);
  
   ACT_GO(window_menu);
   ACT_GO_MOUSE(window_menu);

   return 1;
}

int
e_actions_shutdown(void)
{
   /* FIXME: free actions */
   return 1;
}

E_Action *
e_action_find(char *name)
{
   E_Action *act;
   
   act = evas_hash_find(actions, name);
   return act;
}

E_Action *
e_action_set(char *name)
{
   E_Action *act;
   
   act = e_action_find(name);
   if (!act)
     {
	act = calloc(1, sizeof(E_Action));
	if (!act) return NULL;
	act->name = strdup(name);
	actions = evas_hash_add(actions, name, act);
     }
   return act;
}

void
e_action_del(char *name)
{
   E_Action *act;
   
   act = e_action_find(name);
   if (act)
     {
	actions = evas_hash_del(actions, name, act);
	IF_FREE(act->name);
	free(act);
     }
}

/* local subsystem functions */
