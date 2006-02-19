/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

#define INITS 
#define ACT_GO(name) \
   { \
      act = e_action_add(#name); \
      if (act) act->func.go = _e_actions_act_##name##_go; \
   }
#define ACT_FN_GO(act) \
   static void _e_actions_act_##act##_go(E_Object *obj, char *params)
#define ACT_GO_MOUSE(name) \
   { \
      act = e_action_add(#name); \
      if (act) act->func.go_mouse = _e_actions_act_##name##_go_mouse; \
   }
#define ACT_FN_GO_MOUSE(act) \
   static void _e_actions_act_##act##_go_mouse(E_Object *obj, char *params, Ecore_X_Event_Mouse_Button_Down *ev)
#define ACT_GO_WHEEL(name) \
   { \
      act = e_action_add(#name); \
      if (act) act->func.go_wheel = _e_actions_act_##name##_go_wheel; \
   }
#define ACT_FN_GO_WHEEL(act) \
   static void _e_actions_act_##act##_go_wheel(E_Object *obj, char *params, Ecore_X_Event_Mouse_Wheel *ev)
#define ACT_GO_SIGNAL(name) \
   { \
      act = e_action_add(#name); \
      if (act) act->func.go_signal = _e_actions_act_##name##_go_signal; \
   }
#define ACT_FN_GO_SIGNAL(act) \
   static void _e_actions_act_##act##_go_signal(E_Object *obj, char *params, char *sig, char *src)
#define ACT_GO_KEY(name) \
   { \
      act = e_action_add(#name); \
      if (act) act->func.go_key = _e_actions_act_##name##_go_key; \
   }
#define ACT_FN_GO_KEY(act) \
   static void _e_actions_act_##act##_go_key(E_Object *obj, char *params, Ecore_X_Event_Key_Down *ev)
#define ACT_END(name) \
   { \
      act = e_action_add(#name); \
      if (act) act->func.end = _e_actions_act_##name##_end; \
   }
#define ACT_FN_END(act) \
   static void _e_actions_act_##act##_end(E_Object *obj, char *params)
#define ACT_END_MOUSE(name) \
   { \
      act = e_action_add(#name); \
      if (act) act->func.end_mouse = _e_actions_act_##name##_end_mouse; \
   }
#define ACT_FN_END_MOUSE(act) \
   static void _e_actions_act_##act##_end_mouse(E_Object *obj, char *params, Ecore_X_Event_Mouse_Button_Up *ev)
#define ACT_END_KEY(name) \
   { \
      act = e_action_add(#name); \
      if (act) act->func.end_key = _e_actions_act_##name##_end_key; \
   }
#define ACT_FN_END_KEY(act) \
   static void _e_actions_act_##act##_end_key(E_Object *obj, char *params, Ecore_X_Event_Key_Up *ev)

/* local subsystem functions */
static void _e_action_free(E_Action *act);
static Evas_Bool _e_actions_cb_free(Evas_Hash *hash, const char *key, void *data, void *fdata);

static E_Dialog *exit_dialog = NULL;

/* to save writing this in N places - the sctions are defined here */
/***************************************************************************/
ACT_FN_GO(window_move)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE) return;
   if (!((E_Border *)obj)->lock_user_location)
     e_border_act_move_begin((E_Border *)obj, NULL);
}
ACT_FN_GO_MOUSE(window_move)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE) return;
   if (!((E_Border *)obj)->lock_user_location)
     e_border_act_move_begin((E_Border *)obj, ev);
}
ACT_FN_GO_SIGNAL(window_move)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE) return;
   if (!((E_Border *)obj)->lock_user_location)
     {
	if ((params) && (!strcmp(params, "end")))
	  {
	     e_border_signal_move_end((E_Border *)obj, sig, src);
	  }
	else
	  {
	     if (((E_Border *)obj)->moving)
	       e_border_signal_move_end((E_Border *)obj, sig, src);
	     else
	       e_border_signal_move_begin((E_Border *)obj, sig, src);
	  }
     }
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
   if (!((E_Border *)obj)->lock_user_size)
     e_border_act_resize_begin((E_Border *)obj, NULL);
}
ACT_FN_GO_MOUSE(window_resize)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE) return;
   if (!((E_Border *)obj)->lock_user_size)
     e_border_act_resize_begin((E_Border *)obj, ev);
}
ACT_FN_GO_SIGNAL(window_resize)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE) return;
   if (!((E_Border *)obj)->lock_user_size)
     {
	if ((params) && (!strcmp(params, "end")))
	  e_border_signal_resize_end((E_Border *)obj, params, sig, src);
	else
	  {
	     if (!params) params = "";
	     if (e_border_resizing_get((E_Border *)obj))
	       e_border_signal_resize_end((E_Border *)obj, params, sig, src);
	     else
	       e_border_signal_resize_begin((E_Border *)obj, params, sig, src);
	  }
     }
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
   if (obj->type != E_BORDER_TYPE)
     {
	obj = E_OBJECT(e_border_focused_get());
	if (!obj) return;
     }
   e_border_act_menu_begin((E_Border *)obj, NULL, 0);
}
ACT_FN_GO_MOUSE(window_menu)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE)
     {
	obj = E_OBJECT(e_border_focused_get());
	if (!obj) return;
     }
   e_border_act_menu_begin((E_Border *)obj, ev, 0);
}
ACT_FN_GO_KEY(window_menu)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE)
     {
	obj = E_OBJECT(e_border_focused_get());
	if (!obj) return;
     }
   e_border_act_menu_begin((E_Border *)obj, NULL, 1);
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
   if (!((E_Border *)obj)->lock_user_stacking)
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
   if (!((E_Border *)obj)->lock_user_stacking)
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
   if (!((E_Border *)obj)->lock_close)
     e_border_act_close_begin((E_Border *)obj);
}

/***************************************************************************/
ACT_FN_GO(window_kill)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE)
     {
	obj = E_OBJECT(e_border_focused_get());
	if (!obj) return;
     }
   if (!((E_Border *)obj)->lock_close)
     e_border_act_kill_begin((E_Border *)obj);
}

/***************************************************************************/
ACT_FN_GO(window_sticky_toggle)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE)
     {
	obj = E_OBJECT(e_border_focused_get());
	if (!obj) return;
     }
   if (!((E_Border *)obj)->lock_user_sticky)
     {
	E_Border *bd;
	
	bd = (E_Border *)obj;
	if (bd->sticky) e_border_unstick(bd);
	else e_border_stick(bd);
     }
}

/***************************************************************************/
ACT_FN_GO(window_sticky)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE)
     {
	obj = E_OBJECT(e_border_focused_get());
	if (!obj) return;
     }
   if (!((E_Border *)obj)->lock_user_sticky)
     {
	E_Border *bd;
	
	bd = (E_Border *)obj;
	if (params)
	  {
	     if (atoi(params) == 1)
	       e_border_stick(bd);  	
	     else if (atoi(params) == 0)
	       e_border_unstick(bd);
	  }
     }
}

/***************************************************************************/
ACT_FN_GO(window_iconic_toggle)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE)
     {
	obj = E_OBJECT(e_border_focused_get());
	if (!obj) return;
     }
   if (!((E_Border *)obj)->lock_user_iconify)
     {
	E_Border *bd;
	
	bd = (E_Border *)obj;
	if (bd->iconic) e_border_uniconify(bd);
	else e_border_iconify(bd);
     }
}

/***************************************************************************/
ACT_FN_GO(window_iconic)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE)
     {
	obj = E_OBJECT(e_border_focused_get());
	if (!obj) return;
     }
   if (!((E_Border *)obj)->lock_user_iconify)
     {
	E_Border *bd;
	
	bd = (E_Border *)obj;
	if (params)
	  {
	     if (atoi(params) == 1)
	       e_border_iconify(bd);
	     else if (atoi(params) == 0)
	       e_border_uniconify(bd);
	  }
     }
}

/***************************************************************************/
ACT_FN_GO(window_maximized_toggle)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE)
     {
	obj = E_OBJECT(e_border_focused_get());
	if (!obj) return;
     }
   if (!((E_Border *)obj)->lock_user_maximize)
     {
	E_Border *bd;
	
	bd = (E_Border *)obj;
	/*if (bd->maximized) e_border_unmaximize(bd);*/
	if( bd->maximized != E_MAXIMIZE_NONE &&
	    bd->maximized != E_MAXIMIZE_VERTICAL &&
	    bd->maximized != E_MAXIMIZE_HORIZONTAL )
	{
	  if( !params )
	    e_border_unmaximize(bd);
	  else
	  {
	    if( !strcmp( params, "vertical"))
	      e_border_unmaximize_vh(bd, E_MAXIMIZE_VERTICAL );
	    else if( !strcmp( params, "horizontal"))
	      e_border_unmaximize_vh(bd, E_MAXIMIZE_HORIZONTAL );
	    else
	      e_border_unmaximize(bd);
	  }
	}
	else
	{
	  if( !bd->maximized )
	  {
	    if (!params)
	      e_border_maximize(bd, e_config->maximize_policy);
	    else
	    {
	      if (!strcmp(params, "fullscreen")) e_border_maximize(bd, E_MAXIMIZE_FULLSCREEN);
	      else if (!strcmp(params, "smart")) e_border_maximize(bd, E_MAXIMIZE_SMART);
	      else if (!strcmp(params, "expand")) e_border_maximize(bd, E_MAXIMIZE_EXPAND);
	      else if (!strcmp(params, "fill")) e_border_maximize(bd, E_MAXIMIZE_FILL);
	      else if (!strcmp(params, "vertical")) e_border_maximize(bd, E_MAXIMIZE_VERTICAL);
	      else if (!strcmp(params, "horizontal")) e_border_maximize(bd, E_MAXIMIZE_HORIZONTAL);
	      else e_border_maximize(bd, e_config->maximize_policy);
	    }
	  }
	  else
	  {
	    if( !params )
	      e_border_maximize( bd, e_config->maximize_policy );
	    else
	    {
	      if( !strcmp(params, "vertical") )
	      {
		if( bd->maximized == E_MAXIMIZE_VERTICAL )
		  e_border_unmaximize_vh(bd, E_MAXIMIZE_VERTICAL );
		else
		  e_border_maximize(bd, E_MAXIMIZE_VERTICAL );
	      }
	      else if( !strcmp(params, "horizontal") )
	      {
		if( bd->maximized == E_MAXIMIZE_HORIZONTAL )
		  e_border_unmaximize_vh(bd, E_MAXIMIZE_HORIZONTAL );
		else
		  e_border_maximize(bd, E_MAXIMIZE_HORIZONTAL );
	      }
	      else
		e_border_maximize(bd, e_config->maximize_policy);
	    }
	  }
	}
     }
}
/***************************************************************************/
ACT_FN_GO(window_maximized)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE)
     {
	obj = E_OBJECT(e_border_focused_get());
	if (!obj) return;
     }
   if (!((E_Border *)obj)->lock_user_maximize)
     {
	E_Border *bd;
	
	bd = (E_Border *)obj;
	if (params)
	  {
	     int v;
	     char buf[32];
	     
	     if (sscanf(params, "%i %20s", &v, buf) == 2)
	       {
		  if (v == 1)
		    {
		       if (!strcmp(buf, "fullscreen")) e_border_maximize(bd, E_MAXIMIZE_FULLSCREEN);
		       else if (!strcmp(buf, "smart")) e_border_maximize(bd, E_MAXIMIZE_SMART);
		       else if (!strcmp(buf, "expand")) e_border_maximize(bd, E_MAXIMIZE_EXPAND);
		       else if (!strcmp(buf, "fill")) e_border_maximize(bd, E_MAXIMIZE_FILL);
		    }
		  else if (v == 0)
		    e_border_unmaximize(bd);
	       }
	  }
     }
}

/***************************************************************************/
ACT_FN_GO(window_shaded_toggle)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE)
     {
	obj = E_OBJECT(e_border_focused_get());
	if (!obj) return;
     }
   if (!((E_Border *)obj)->lock_user_shade)
     {
	E_Border *bd;
	
	bd = (E_Border *)obj;
	if (bd->shaded)
	  {
	     if (!params)
	       e_border_unshade(bd, E_DIRECTION_UP);
	     else
	       {
		  if (!strcmp(params, "up")) e_border_unshade(bd, E_DIRECTION_UP);
		  else if (!strcmp(params, "down")) e_border_unshade(bd, E_DIRECTION_DOWN);
		  else if (!strcmp(params, "left")) e_border_unshade(bd, E_DIRECTION_LEFT);
		  else if (!strcmp(params, "right")) e_border_unshade(bd, E_DIRECTION_RIGHT);
		  else e_border_unshade(bd, E_DIRECTION_UP);
	       }
	  }
	else
	  {
	     if (!params)
	       e_border_shade(bd, E_DIRECTION_UP);
	     else
	       {
		  if (!strcmp(params, "up")) e_border_shade(bd, E_DIRECTION_UP);
		  else if (!strcmp(params, "down")) e_border_shade(bd, E_DIRECTION_DOWN);
		  else if (!strcmp(params, "left")) e_border_shade(bd, E_DIRECTION_LEFT);
		  else if (!strcmp(params, "right")) e_border_shade(bd, E_DIRECTION_RIGHT);
		  else e_border_shade(bd, E_DIRECTION_UP);
	       }
	  }
     }
}

/***************************************************************************/
ACT_FN_GO(window_shaded)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE)
     {
	obj = E_OBJECT(e_border_focused_get());
	if (!obj) return;
     }
   if (!((E_Border *)obj)->lock_user_shade)
     {
	E_Border *bd;
	
	bd = (E_Border *)obj;
	if (params)
	  {
	     int v;
	     char buf[32];
	     
	     if (sscanf(params, "%i %20s", &v, buf) == 2)
	       {
		  if (v == 1)
		    {
		       if (!strcmp(buf, "up")) e_border_shade(bd, E_DIRECTION_UP);
		       else if (!strcmp(buf, "down")) e_border_shade(bd, E_DIRECTION_DOWN);
		       else if (!strcmp(buf, "left")) e_border_shade(bd, E_DIRECTION_LEFT);
		       else if (!strcmp(buf, "right")) e_border_shade(bd, E_DIRECTION_RIGHT);
		    }
		  else if (v == 0)
		    {
		       if (!strcmp(buf, "up")) e_border_unshade(bd, E_DIRECTION_UP);
		       else if (!strcmp(buf, "down")) e_border_unshade(bd, E_DIRECTION_DOWN);
		       else if (!strcmp(buf, "left")) e_border_unshade(bd, E_DIRECTION_LEFT);
		       else if (!strcmp(buf, "right")) e_border_unshade(bd, E_DIRECTION_RIGHT);
		    }
	       }
	  }
     }
}

/***************************************************************************/
ACT_FN_GO(window_move_by)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE)
     {
       obj = E_OBJECT(e_border_focused_get());
       if (!obj) return;
     }
   if (params)
     {
	int dx, dy;

	if (sscanf(params, "%i %i", &dx, &dy) == 2)
	  {
	     E_Border *bd;

	     bd = (E_Border *)obj;

	     e_border_move(bd, bd->x + dx, bd->y + dy);

	     if (e_config->focus_policy != E_FOCUS_CLICK)
	       ecore_x_pointer_warp(bd->zone->container->win,
				    bd->x + (bd->w / 2),
				    bd->y + (bd->h / 2));
	}
     }
}
/***************************************************************************/
ACT_FN_GO(window_move_to)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE)
     {
       obj = E_OBJECT(e_border_focused_get());
       if (!obj) return;
     }
   if (params)
     {
	E_Border *bd;
	int x, y;
	char cx, cy;

	bd = (E_Border *)obj;

	if (sscanf(params, "%c%i %c%i", &cx, &x, &cy, &y) == 4)
	  {
	     // Nothing, both x and y is updated.
	  }
	else if (sscanf(params, "* %c%i", &cy, &y) == 2)
	  {
	     // Updated y, reset x.
	     x = bd->x;
	  }
	else if (sscanf(params, "%c%i *", &cx, &x) == 2)
	  {
	     // Updated x, reset y.
	     y = bd->y;
	  }

	if (cx == '-') x = bd->zone->w - bd->w - x;
	if (cy == '-') y = bd->zone->h - bd->h - y;

	if ((x != bd->x) || (y != bd->y))
	  {
	     e_border_move(bd, x, y);

	     if (e_config->focus_policy != E_FOCUS_CLICK)
	       ecore_x_pointer_warp(bd->zone->container->win,
				    bd->x + (bd->w / 2),
				    bd->y + (bd->h / 2));
	  }
     }
}

/***************************************************************************/
ACT_FN_GO(window_resize_by)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE)
     {
       obj = E_OBJECT(e_border_focused_get());
       if (!obj) return;
     }
   
   if (params)
     {
	int dw, dh;
	
	if (sscanf(params, "%i %i", &dw, &dh) == 2)
	  {
	     E_Border *bd;
	     bd = (E_Border *)obj;
	     
	     dw += bd->w;
	     dh += bd->h;
	     e_border_resize_limit(bd, &dw, &dh);
	     e_border_resize(bd, dw, dh);
	     
	     if (e_config->focus_policy != E_FOCUS_CLICK)
	       ecore_x_pointer_warp(bd->zone->container->win,
				    bd->x + (dw / 2),
				    bd->y + (dh / 2));
	  }
     }
}

/***************************************************************************/
ACT_FN_GO(window_drag_icon)
{
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE)
     {
       obj = E_OBJECT(e_border_focused_get());
       if (!obj) return;
     }
     {
	E_Border *bd;
	
	bd = (E_Border *)obj;
	bd->drag.start = 1;
	bd->drag.x = -1;
	bd->drag.y = -1;
     }
}

/***************************************************************************/
ACT_FN_GO(window_desk_move_by)
{
   E_Border *bd;
   int x, y;

   if (!params) return;
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE)
     {
       obj = E_OBJECT(e_border_focused_get());
       if (!obj) return;
     }

   bd = (E_Border *)obj;
   if ((!bd->zone) || (!bd->desk)) return;
   if (sscanf(params, "%d %d", &x, &y) == 2)
     {
	E_Desk *desk;
	int dx, dy;
	int to_x = 0, to_y = 0;

	e_desk_xy_get(bd->desk, &dx, &dy);
	
	to_x = dx + x;
	to_y = dy + y;
	while ((desk = e_desk_at_xy_get(bd->zone, to_x, to_y )) == NULL)
	  {
	     // here we are out of our desktop range
	     while (to_x >= bd->zone->desk_x_count)
	       {
		  to_x -= bd->zone->desk_x_count;
		  to_y++;
	       }
	     while (to_x < 0)
	       {
		  to_x += bd->zone->desk_x_count;
		  to_y--;
	       }
	     
	     while (to_y >= bd->zone->desk_y_count)
	       {
		  to_y -= bd->zone->desk_y_count;
	       }
	     while (to_y < 0)
	       {
		  to_y += bd->zone->desk_y_count;
	       }
	  }
	
	if (desk)
	  {
	     // switch desktop. Quite usefull from the interface point of view.
	     e_zone_desk_flip_by(bd->zone, to_x - dx, to_y - dy);
	     // send the border to the required desktop.
	     e_border_desk_set(bd, desk);
	     e_border_focus_set(bd, 1, 1);
	  }
     }
}

/***************************************************************************/
ACT_FN_GO(window_desk_move_to)
{
   E_Border *bd;
   int x, y;

   if (!params) return;
   if (!obj) obj = E_OBJECT(e_border_focused_get());
   if (!obj) return;
   if (obj->type != E_BORDER_TYPE)
     {
       obj = E_OBJECT(e_border_focused_get());
       if (!obj) return;
     }

   bd = (E_Border *)obj;
   if ((!bd->zone) || (!bd->desk)) return;
   if (sscanf(params, "%d %d", &x, &y) == 2)
     {
	E_Desk *desk;

	desk = e_desk_at_xy_get(bd->zone, x, y);
	if (desk)
	  e_border_desk_set(bd, desk);
     }
}

/***************************************************************************/
static E_Zone *
_e_actions_zone_get(E_Object *obj)
{
   if (obj)
     {
	if (obj->type == E_MANAGER_TYPE)
	  return e_util_zone_current_get((E_Manager *)obj);
	else if (obj->type == E_CONTAINER_TYPE)
	  return e_util_zone_current_get(((E_Container *)obj)->manager);
	else if (obj->type == E_ZONE_TYPE)
	  return e_util_zone_current_get(((E_Zone *)obj)->container->manager);
	else
	  return e_util_zone_current_get(e_manager_current_get());
     }
   return e_util_zone_current_get(e_manager_current_get());
}
	  
ACT_FN_GO(desk_flip_by)
{
   E_Zone *zone;

   zone = _e_actions_zone_get(obj);
   if (zone)
     {
	if (params)
	  {
	     int dx = 0, dy = 0;
	
	     if (sscanf(params, "%i %i", &dx, &dy) == 2)
	       e_zone_desk_flip_by(zone, dx, dy);
	  }
     }
}

/***************************************************************************/
ACT_FN_GO(desk_flip_to)
{
   E_Zone *zone;

   zone = _e_actions_zone_get(obj);
   if (zone)
     {
	if (params)
	  {
	     int dx = 0, dy = 0;
	
	     if (sscanf(params, "%i %i", &dx, &dy) == 2)
	       e_zone_desk_flip_to(zone, dx, dy);
	  }
     }
}

/***************************************************************************/
ACT_FN_GO(desk_linear_flip_by)
{
   E_Zone *zone;

   zone = _e_actions_zone_get(obj);
   if (zone)
     {
	if (params)
	  {
	     int dx = 0;
	
	     if (sscanf(params, "%i", &dx) == 1)
	       e_zone_desk_linear_flip_by(zone, dx);
	  }
     }
}

/***************************************************************************/
ACT_FN_GO(desk_linear_flip_to)
{
   E_Zone *zone;

   zone = _e_actions_zone_get(obj);
   if (zone)
     {
	if (params)
	  {
	     int dx = 0;
	
	     if (sscanf(params, "%i", &dx) == 1)
	       e_zone_desk_linear_flip_to(zone, dx);
	  }
     }
}

#define ZONE_DESK_ACTION(con_num, zone_num, zone, act) \
E_Zone *zone; \
if ((con_num < 0) || (zone_num < 0)) { \
   Evas_List *l, *ll, *lll; \
   E_Container *con; \
   E_Manager *man; \
   if ((con_num >= 0) && (zone_num < 0)) /* con=1 zone=all */ { \
	con = e_util_container_number_get(con_num); \
	for (l = con->zones; l; l = l->next) { \
	   zone = l->data; \
	   act; \
	} } \
   else if ((con_num < 0) && (zone_num >= 0)) /* con=all zone=1 */ { \
	for (l = e_manager_list(); l; l = l->next) { \
	   man = l->data; \
	   for (ll = man->containers; ll; ll = ll->next) { \
	      con = ll->data; \
	      zone = e_container_zone_number_get(con, zone_num); \
	      if (zone) \
		act; \
	   } } } \
   else if ((con_num < 0) && (zone_num < 0)) /* con=all zone=all */ { \
      for (l = e_manager_list(); l; l = l->next) { \
	 man = l->data; \
	 for (ll = man->containers; ll; ll = ll->next) { \
	    con = ll->data; \
	    for (lll = con->zones; lll; lll = lll->next) { \
	       zone = lll->data; \
	       act; \
	    } } } } } \
else { \
   zone = e_util_container_zone_number_get(con_num, zone_num); \
   if (zone) act; \
}

/***************************************************************************/
ACT_FN_GO(zone_desk_flip_by)
{
   if (params)
     {
	int con_num = 0, zone_num = 0;
	int dx = 0, dy = 0;
	
	if (sscanf(params, "%i %i %i %i", &con_num, &zone_num, &dx, &dy) == 4)
	  {
	     ZONE_DESK_ACTION(con_num, zone_num, zone,
			      e_zone_desk_flip_by(zone, dx, dy));
	  }
     }
}

/***************************************************************************/
ACT_FN_GO(zone_desk_flip_to)
{
   if (params)
     {
	int con_num = 0, zone_num = 0;
	int dx = 0, dy = 0;
	
	if (sscanf(params, "%i %i %i %i", &con_num, &zone_num, &dx, &dy) == 4)
	  {
	     ZONE_DESK_ACTION(con_num, zone_num, zone,
			      e_zone_desk_flip_to(zone, dx, dy));
	  }
     }
}

/***************************************************************************/
ACT_FN_GO(zone_desk_linear_flip_by)
{
   if (params)
     {
	int con_num = 0, zone_num = 0;
	int dx = 0;
	
	if (sscanf(params, "%i %i %i", &con_num, &zone_num, &dx) == 3)
	  {
	     ZONE_DESK_ACTION(con_num, zone_num, zone,
			      e_zone_desk_linear_flip_by(zone, dx));
	  }
     }
}

/***************************************************************************/
ACT_FN_GO(zone_desk_linear_flip_to)
{
   if (params)
     {
	int con_num = 0, zone_num = 0;
	int dx = 0;
	
	if (sscanf(params, "%i %i %i", &con_num, &zone_num, &dx) == 3)
	  {
	     ZONE_DESK_ACTION(con_num, zone_num, zone,
			      e_zone_desk_linear_flip_to(zone, dx));
	  }
     }
}

/***************************************************************************/
static void
_e_actions_cb_menu_end(void *data, E_Menu *m)
{
   e_object_del(E_OBJECT(m));
}
static E_Menu *
_e_actions_menu_find(char *name)
{
   if (!strcmp(name, "main")) return e_int_menus_main_new();
   else if (!strcmp(name, "favorites")) return e_int_menus_favorite_apps_new();
   else if (!strcmp(name, "clients")) return e_int_menus_clients_new();
   return NULL;
}
ACT_FN_GO(menu_show)
{
   E_Zone *zone;

   /* menu is active - abort */
   if (e_menu_grab_window_get()) return;
   zone = _e_actions_zone_get(obj);
   if (zone)
     {
	if (params)
	  {
	     E_Menu *m = NULL;
	     
	     m = _e_actions_menu_find(params);	
	     if (m)
	       {
		  int x, y;
		  
		  /* FIXME: this is a bit of a hack... setting m->con - bad hack */
		  m->zone = zone;
		  ecore_x_pointer_xy_get(zone->container->win, &x, &y);
		  e_menu_post_deactivate_callback_set(m, _e_actions_cb_menu_end, NULL);
		  e_menu_activate_mouse(m, zone, x, y, 1, 1,
					E_MENU_POP_DIRECTION_DOWN, 
					ecore_x_current_time_get());
	       }
	  }
     }
}
ACT_FN_GO_MOUSE(menu_show)
{
   E_Zone *zone;

   /* menu is active - abort */
   if (e_menu_grab_window_get()) return;
   zone = _e_actions_zone_get(obj);
   if (zone)
     {
	if (params)
	  {
	     E_Menu *m = NULL;
	     
	     m = _e_actions_menu_find(params);	
	     if (m)
	       {
		  int x, y;
		  
		  /* FIXME: this is a bit of a hack... setting m->con - bad hack */
		  m->zone = zone;
		  x = ev->root.x;
		  y = ev->root.y;
		  x -= zone->container->x;
		  y -= zone->container->y;
		  e_menu_post_deactivate_callback_set(m, _e_actions_cb_menu_end, NULL);
		  e_menu_activate_mouse(m, zone, x, y, 1, 1,
					E_MENU_POP_DIRECTION_DOWN, ev->time);
		  e_util_container_fake_mouse_up_all_later(zone->container);
	       }
	  }
     }
}
ACT_FN_GO_KEY(menu_show)
{
   E_Zone *zone;

   /* menu is active - abort */
   if (e_menu_grab_window_get()) return;
   zone = _e_actions_zone_get(obj);
   if (zone)
     {
	if (params)
	  {
	     E_Menu *m = NULL;
	     
	     m = _e_actions_menu_find(params);	
	     if (m)
	       {
		  int x, y;
		  
		  /* FIXME: this is a bit of a hack... setting m->con - bad hack */
		  m->zone = zone;
		  ecore_x_pointer_xy_get(zone->container->win, &x, &y);
		  e_menu_post_deactivate_callback_set(m, _e_actions_cb_menu_end, NULL);
		  e_menu_activate_key(m, zone, x, y, 1, 1,
				      E_MENU_POP_DIRECTION_DOWN);
	       }
	  }
     }
}

/***************************************************************************/
ACT_FN_GO(exec)
{
   if (params)
     {
	Ecore_Exe *exe;
	
	exe = ecore_exe_run(params, NULL);
	e_exehist_add("action/exec", params);
	if (exe) ecore_exe_free(exe);
     }
}

/***************************************************************************/
ACT_FN_GO(app)
{
   E_Zone *zone;
   
   zone = _e_actions_zone_get(obj);
   if (zone)
     {
	if (params)
	  {
	     E_App *a = NULL;
	     char *p, *p2;
	     
	     p2 = alloca(strlen(params) + 1);
	     strcpy(p2, params);
	     p = strchr(p2, ' ');
	     if (p)
	       {
		  *p = 0;
		  if (!strcmp(p2, "file:"))
		    a = e_app_file_find(p + 1);
		  else if (!strcmp(p2, "name:"))
		    a = e_app_name_find(p + 1);
		  else if (!strcmp(p2, "generic:"))
		    a = e_app_generic_find(p + 1);
		  else if (!strcmp(p2, "exe:"))
		    a = e_app_exe_find(p + 1);
		  if (a)
		    {
		       e_zone_app_exec(zone, a);
		       e_exehist_add("action/app", a->exe);
		    }
	       }
	  }
     }
}

/***************************************************************************/
ACT_FN_GO(winlist)
{
   E_Zone *zone;
   
   zone = _e_actions_zone_get(obj);
   if (zone)
     {
	if (params)
	  {
	     if (!strcmp(params, "next"))
	       {
		  if (!e_winlist_show(zone))
		    e_winlist_next();
	       }
	     else if (!strcmp(params, "prev"))
	       {
		  if (!e_winlist_show(zone))
		    e_winlist_prev();
	       }
	  }
	else
	  {
	     if (!e_winlist_show(zone))
	       e_winlist_next();
	  }
     }
}
ACT_FN_GO_MOUSE(winlist)
{
   E_Zone *zone;
   
   zone = _e_actions_zone_get(obj);
   if (zone)
     {
	if (params)
	  {
	     if (!strcmp(params, "next"))
	       {
		  if (e_winlist_show(zone))
		    e_winlist_modifiers_set(ev->modifiers);
		  else
		    e_winlist_next();
	       }
	     else if (!strcmp(params, "prev"))
	       {
		  if (e_winlist_show(zone))
		    e_winlist_modifiers_set(ev->modifiers);
		  else
		    e_winlist_prev();
	       }
	  }
	else
	  {
	     if (e_winlist_show(zone))
	       e_winlist_modifiers_set(ev->modifiers);
	     else
	       e_winlist_next();
	  }
     }
}
ACT_FN_GO_KEY(winlist)
{
   E_Zone *zone;
   
   zone = _e_actions_zone_get(obj);
   if (zone)
     {
	if (params)
	  {
	     if (!strcmp(params, "next"))
	       {
		  if (e_winlist_show(zone))
		    e_winlist_modifiers_set(ev->modifiers);
		  else
		    e_winlist_next();
	       }
	     else if (!strcmp(params, "prev"))
	       {
		  if (e_winlist_show(zone))
		    e_winlist_modifiers_set(ev->modifiers);
		  else
		    e_winlist_prev();
	       }
	  }
	else
	  {
	     if (e_winlist_show(zone))
	       e_winlist_modifiers_set(ev->modifiers);
	     else
	       e_winlist_next();
	  }
     }
}

/***************************************************************************/
ACT_FN_GO(edit_mode)
{
   if (!obj) obj = E_OBJECT(e_container_current_get(e_manager_current_get()));
   if (!obj) return;
   if (obj->type != E_CONTAINER_TYPE)
     {
	obj = E_OBJECT(e_container_current_get(e_manager_current_get()));
	if (!obj) return;
     }
   e_gadman_mode_set(((E_Container *)obj)->gadman, E_GADMAN_MODE_EDIT);
   e_gadcon_all_edit_begin();
}
ACT_FN_END(edit_mode)
{
   if (!obj) obj = E_OBJECT(e_container_current_get(e_manager_current_get()));
   if (!obj) return;
   if (obj->type != E_CONTAINER_TYPE)
     {
	obj = E_OBJECT(e_container_current_get(e_manager_current_get()));
	if (!obj) return;
     }
   e_gadman_mode_set(((E_Container *)obj)->gadman, E_GADMAN_MODE_NORMAL);
   e_gadcon_all_edit_end();
}

/***************************************************************************/
ACT_FN_GO(edit_mode_toggle)
{
   if (!obj) obj = E_OBJECT(e_container_current_get(e_manager_current_get()));
   if (!obj) return;
   if (obj->type != E_CONTAINER_TYPE)
     {
	obj = E_OBJECT(e_container_current_get(e_manager_current_get()));
	if (!obj) return;
     }
   if (e_gadman_mode_get(((E_Container *)obj)->gadman) == E_GADMAN_MODE_NORMAL)
     e_gadman_mode_set(((E_Container *)obj)->gadman, E_GADMAN_MODE_EDIT);
   else
     e_gadman_mode_set(((E_Container *)obj)->gadman, E_GADMAN_MODE_NORMAL);
}

/***************************************************************************/

static void
_e_actions_cb_exit_dialog_ok(void *data, E_Dialog *dia)
{
   ecore_main_loop_quit();
   e_object_del(E_OBJECT(exit_dialog));
   exit_dialog = NULL;
}

static void
_e_actions_cb_exit_dialog_cancel(void *data, E_Dialog *dia)
{
   e_object_del(E_OBJECT(exit_dialog));
   exit_dialog = NULL;
}

static void
_e_actions_cb_exit_dialog_delete(E_Win *win)
{
   E_Dialog *dia;

   dia = win->data;
   _e_actions_cb_exit_dialog_cancel(NULL, dia);
}

ACT_FN_GO(exit)
{
   if (exit_dialog) e_object_del(E_OBJECT(exit_dialog));
   exit_dialog = e_dialog_new(e_container_current_get(e_manager_current_get()));
   if (!exit_dialog) return;
   e_win_delete_callback_set(exit_dialog->win, _e_actions_cb_exit_dialog_delete);
   e_dialog_title_set(exit_dialog, _("Are you sure you want to exit?"));
   e_dialog_text_set(exit_dialog,
		     _("You requested to exit Enlightenment.<br>"
		       "<br>"
		       "Are you sure you want to exit?"
		       ));
   e_dialog_icon_set(exit_dialog, "enlightenment/exit", 64);
   e_dialog_button_add(exit_dialog, _("Yes"), NULL,
		       _e_actions_cb_exit_dialog_ok, NULL);
   e_dialog_button_add(exit_dialog, _("No"), NULL,
		       _e_actions_cb_exit_dialog_cancel, NULL);
   e_dialog_button_focus_num(exit_dialog, 1);
   e_win_centered_set(exit_dialog->win, 1);
   e_dialog_show(exit_dialog);
}

/***************************************************************************/
ACT_FN_GO(restart)
{
   restart = 1;
   ecore_main_loop_quit();
}

ACT_FN_GO(pointer_resize_push)
{
   E_Manager *man = NULL;

   if (!obj) return;
   if (obj->type == E_BORDER_TYPE)
     {
	E_Border *bd;
	bd = (E_Border *)obj;
	if ((bd->lock_user_size) || (bd->shaded) || (bd->shading) ||
	    (bd->fullscreen) || (bd->maximized == E_MAXIMIZE_FULLSCREEN))
	  return;
	if (bd->zone)
	  man = bd->zone->container->manager;
     }
   if (!man) man = e_manager_current_get();
   if (!man) return;
   e_pointer_type_push(man->pointer, obj, params);
}

ACT_FN_GO(pointer_resize_pop)
{
   E_Manager *man = NULL;

   if (!obj) return;
   if (obj->type == E_BORDER_TYPE)
     {
	E_Border *bd;
	bd = (E_Border *)obj;
	if ((bd->lock_user_size) || (bd->shaded) || (bd->shading) ||
	    (bd->fullscreen) || (bd->maximized == E_MAXIMIZE_FULLSCREEN))
	  return;
	if (bd->zone)
	  man = (E_Manager *)bd->zone->container->manager;
     }
   if (!man) man = e_manager_current_get();
   if (!man) return;
   e_pointer_type_pop(man->pointer, obj, params);
}

/***************************************************************************/
ACT_FN_GO(exebuf)
{
   E_Zone *zone;
   
   zone = _e_actions_zone_get(obj);
   if (zone)
     e_exebuf_show(zone);
}

ACT_FN_GO(desk_lock)
{
/*  E_Zone *zone;

  zone = _e_actions_zone_get(obj);
  if (zone)*/
  e_desklock_show();
}

/* local subsystem globals */
static Evas_Hash *actions = NULL;
static Evas_List *action_names = NULL;

/* externally accessible functions */

EAPI int
e_actions_init(void)
{
   E_Action *act;

   ACT_GO(window_move);
   ACT_GO_MOUSE(window_move);
   ACT_GO_SIGNAL(window_move);
   ACT_END(window_move);
   ACT_END_MOUSE(window_move);
   
   ACT_GO(window_resize);
   ACT_GO_MOUSE(window_resize);
   ACT_GO_SIGNAL(window_resize);
   ACT_END(window_resize);
   ACT_END_MOUSE(window_resize);
  
   ACT_GO(window_menu);
   ACT_GO_MOUSE(window_menu);
   ACT_GO_KEY(window_menu);

   ACT_GO(window_raise);

   ACT_GO(window_lower);
   
   ACT_GO(window_close);

   ACT_GO(window_kill);
   
   ACT_GO(window_sticky_toggle);
   
   ACT_GO(window_sticky);
   
   ACT_GO(window_iconic_toggle);
   
   ACT_GO(window_iconic);
   
   ACT_GO(window_maximized_toggle);

   ACT_GO(window_maximized);
   
   ACT_GO(window_shaded_toggle);
   
   ACT_GO(window_shaded);
   
   ACT_GO(desk_flip_by);

   ACT_GO(desk_flip_to);

   ACT_GO(desk_linear_flip_by);
   
   ACT_GO(desk_linear_flip_to);

   ACT_GO(window_move_to);

   ACT_GO(window_move_by);

   ACT_GO(window_resize_by);
   
   ACT_GO(window_drag_icon);

   ACT_GO(window_desk_move_by);
   ACT_GO(window_desk_move_to);

   ACT_GO(menu_show);
   ACT_GO_MOUSE(menu_show);
   ACT_GO_KEY(menu_show);

   ACT_GO(exec);

   ACT_GO(app);
   
   ACT_GO(winlist);
   ACT_GO_MOUSE(winlist);
   ACT_GO_KEY(winlist);
   
   ACT_GO(edit_mode);
   ACT_END(edit_mode);
   
   ACT_GO(edit_mode_toggle);

   ACT_GO(restart);
   ACT_GO(exit);

   ACT_GO(pointer_resize_push);
   ACT_GO(pointer_resize_pop);
   
   ACT_GO(exebuf);
   ACT_GO(desk_lock);
   
   return 1;
}

EAPI int
e_actions_shutdown(void)
{
   action_names = evas_list_free(action_names);
   while (actions)
     {
	evas_hash_foreach(actions, _e_actions_cb_free, NULL);
     }
   return 1;
}

EAPI Evas_List *
e_action_name_list(void)
{
   return action_names;
}

EAPI E_Action *
e_action_add(const char *name)
{
   E_Action *act;
   
   act = e_action_find(name);
   if (!act)
     {
	act = E_OBJECT_ALLOC(E_Action, E_ACTION_TYPE, _e_action_free);
	if (!act) return NULL;
	act->name = name;
	actions = evas_hash_direct_add(actions, act->name, act);
	action_names = evas_list_append(action_names, name);
     }
   return act;
}

EAPI E_Action *
e_action_find(const char *name)
{
   E_Action *act;
   
   act = evas_hash_find(actions, name);
   return act;
}

/* local subsystem functions */

static void
_e_action_free(E_Action *act)
{
   actions = evas_hash_del(actions, act->name, act);
   action_names = evas_list_remove(action_names, act->name);
   free(act);
}

static Evas_Bool
_e_actions_cb_free(Evas_Hash *hash __UNUSED__, const char *key __UNUSED__,
		   void *data, void *fdata __UNUSED__)
{
   e_object_del(E_OBJECT(data));
   return 0;
}
