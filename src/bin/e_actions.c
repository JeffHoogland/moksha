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
#define ACT_GO_KEY(name) \
   { \
      act = e_action_set(#name); \
      if (act) act->func.go_key = _e_actions_act_##name##_go_key; \
   }
#define ACT_FN_GO_KEY(act) \
   static void _e_actions_act_##act##_go_key(E_Object *obj, char *params, Ecore_X_Event_Key_Down *ev)
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
#define ACT_END_KEY(name) \
   { \
      act = e_action_set(#name); \
      if (act) act->func.end_key = _e_actions_act_##name##_end_key; \
   }
#define ACT_FN_END_KEY(act) \
   static void _e_actions_act_##act##_end_key(E_Object *obj, char *params, Ecore_X_Event_Key_Up *ev)

/* local subsystem functions */

/* to save writing this in N places - the sctions are defined here */
/***************************************************************************/
ACT_FN_GO(window_move)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE) return;
   e_border_act_move_begin((E_Border *)obj, NULL);
}
ACT_FN_GO_MOUSE(window_move)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE) return;
   e_border_act_move_begin((E_Border *)obj, ev);
}
ACT_FN_END(window_move)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE) return;
   e_border_act_move_end((E_Border *)obj, NULL);
}
ACT_FN_END_MOUSE(window_move)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE) return;
   e_border_act_move_end((E_Border *)obj, ev);
}

/***************************************************************************/
ACT_FN_GO(window_resize)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE) return;
   e_border_act_resize_begin((E_Border *)obj, NULL);
}
ACT_FN_GO_MOUSE(window_resize)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE) return;
   e_border_act_resize_begin((E_Border *)obj, ev);
}
ACT_FN_END(window_resize)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE) return;
   e_border_act_resize_end((E_Border *)obj, NULL);
}
ACT_FN_END_MOUSE(window_resize)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE) return;
   e_border_act_resize_end((E_Border *)obj, ev);
}

/***************************************************************************/
ACT_FN_GO(window_menu)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE) return;
   e_border_act_menu_begin((E_Border *)obj, NULL);
}
ACT_FN_GO_MOUSE(window_menu)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE) return;
   e_border_act_menu_begin((E_Border *)obj, ev);
}

/***************************************************************************/
ACT_FN_GO(window_raise)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE)
     {
	obj = E_OBJECT(e_border_focused_get());
	if (!obj) return;
     }
   e_border_raise((E_Border *)obj);
}

/***************************************************************************/
ACT_FN_GO(window_lower)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE)
     {
	obj = E_OBJECT(e_border_focused_get());
	if (!obj) return;
     }
   e_border_lower((E_Border *)obj);
}

/***************************************************************************/
ACT_FN_GO(window_close)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE)
     {
	obj = E_OBJECT(e_border_focused_get());
	if (!obj) return;
     }
   e_border_act_close_begin((E_Border *)obj);
}

/***************************************************************************/
ACT_FN_GO(desk_flip_by)
{
   E_Container *con;
   
   if (!obj) return;
   if (obj->type != E_MANAGER_TYPE) return;
   con = e_manager_container_current_get((E_Manager *)obj);
   /* FIXME: this shoudl really go into desk logic and zone... */
   if (con)
     {
	E_Zone *zone;
	
	zone = e_zone_current_get(con);
	if (zone)
	  {
	     E_Desk *desk;
	     int dx = 0, dy = 0;
	     
	     if (params)
	       {
		  if (sscanf(params, "%i %i", &dx, &dy) != 2)
		    {
		       dx = 0;
		       dy = 0;
		    }
	       }
	     dx = zone->desk_x_current + dx;
	     if (dx < 0) dx = 0;
	     else if (dx >= zone->desk_x_count) dx = zone->desk_x_count  - 1;
	     dy = zone->desk_x_current + dy;
	     if (dy < 0) dy = 0;
	     else if (dy >= zone->desk_y_count) dy = zone->desk_y_count  - 1;
	     desk = e_desk_at_xy_get(zone, dx, dy);
	     if (desk)
	       {  
		  ecore_x_window_focus(con->manager->root);
		  e_desk_show(desk);
		  e_zone_update_flip(zone);
	       }
	  }
     }
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

   ACT_GO(window_raise);

   ACT_GO(window_lower);
   ACT_GO(window_close);

   ACT_GO(desk_flip_by);
   
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
