#include "cursors.h"
#include "border.h"
#include "config.h"
#include "actions.h"
#include "delayed.h"
#include "desktops.h"
#include "resist.h"
#include "icccm.h"
#include "util.h"
#include "place.h"

/* Window border rendering, querying, setting  & modification code */

/* globals local to window borders */
static Evas_List evases = NULL;
static Evas_List borders = NULL;

static int mouse_x, mouse_y, mouse_win_x, mouse_win_y;
static int mouse_buttons = 0;

static int border_mouse_x = 0;
static int border_mouse_y = 0;
static int border_mouse_buttons = 0;

static Eevent *current_ev = NULL;

/* Global delayed window raise action */
E_Delayed_Action *delayed_window_raise = NULL;

static void e_idle(void *data);
static void e_map_request(Eevent * ev);
static void e_configure_request(Eevent * ev);
static void e_property(Eevent * ev);
static void e_unmap(Eevent * ev);
static void e_destroy(Eevent * ev);
static void e_circulate_request(Eevent * ev);
static void e_reparent(Eevent * ev);
static void e_shape(Eevent * ev);
static void e_focus_in(Eevent * ev);
static void e_focus_out(Eevent * ev);
static void e_colormap(Eevent * ev);
static void e_mouse_down(Eevent * ev);
static void e_mouse_up(Eevent * ev);
static void e_mouse_in(Eevent * ev);
static void e_mouse_out(Eevent * ev);
static void e_window_expose(Eevent * ev);

static void e_cb_mouse_in(void *data, Ebits_Object o, char *class, int bt, int x, int y, int ox, int oy, int ow, int oh);
static void e_cb_mouse_out(void *data, Ebits_Object o, char *class, int bt, int x, int y, int ox, int oy, int ow, int oh);
static void e_cb_mouse_down(void *data, Ebits_Object o, char *class, int bt, int x, int y, int ox, int oy, int ow, int oh);
static void e_cb_mouse_up(void *data, Ebits_Object o, char *class, int bt, int x, int y, int ox, int oy, int ow, int oh);
static void e_cb_mouse_move(void *data, Ebits_Object o, char *class, int bt, int x, int y, int ox, int oy, int ow, int oh);

static void e_cb_border_mouse_in(E_Border *b, Eevent *e);
static void e_cb_border_mouse_out(E_Border *b, Eevent *e);
static void e_cb_border_mouse_down(E_Border *b, Eevent *e);
static void e_cb_border_mouse_up(E_Border *b, Eevent *e);
static void e_cb_border_mouse_move(E_Border *b, Eevent *e);
static void e_cb_border_move_resize(E_Border *b);
static void e_cb_border_visibility(E_Border *b);

static void e_border_poll(int val, void *data);

/* what to dowhen we're idle */

void
e_border_update_borders(void)
{
   Evas_List l;

   for (l = borders; l; l = l->next)
     {
	E_Border *b;
	
	b = l->data;
	e_border_update(b);
     }
   for (l = borders; l; l = l->next)
     {
	E_Border *b;
	
	b = l->data;
	if (b->first_expose)
	  {
	     evas_render(b->evas.l);
	     evas_render(b->evas.r);
	     evas_render(b->evas.t);
	     evas_render(b->evas.b);
	  }
     }
   e_db_flush();
}

static void
e_idle(void *data)
{
   e_border_update_borders();
   return;
   UN(data);
}

/* */
static void
e_map_request(Eevent * ev)
{
   Ev_Window_Map_Request      *e;
   
   current_ev = ev;
   e = ev->event;
     {
	E_Border *b;
	
	b = e_border_find_by_window(e->win);
	if (!b)
	  {
	     if (e_window_exists(e->win))
	       b = e_border_adopt(e->win, 0);
	  }
     }
   current_ev = NULL;
}

/* */
static void
e_configure_request(Eevent * ev)
{
   Ev_Window_Configure_Request      *e;
   
   current_ev = ev;
   e = ev->event;
     {
	E_Border *b;
	
	b = e_border_find_by_window(e->win);
	if (b)
	  {
	     int pl, pr, pt, pb;
      
	     pl = pr = pt = pb = 0;
	     if (b->bits.t) ebits_get_insets(b->bits.t, &pl, &pr, &pt, &pb);
	     if (e->mask & EV_VALUE_X)
		b->current.requested.x = e->x;
	     if (e->mask & EV_VALUE_Y)
		b->current.requested.y = e->y;
	     if ((e->mask & EV_VALUE_W) || (e->mask & EV_VALUE_H))
	       {
		  if (b->current.shaded == b->client.h)
		    {
		       b->current.shaded = e->h;
		    }
		  else if (b->current.shaded != 0)
		    {
		       b->current.shaded += b->client.h - e->h;
		       if (b->current.shaded > b->client.h)
			 b->current.shaded = b->client.h;
		       if (b->current.shaded < 1)
			 b->current.shaded = 1;
		    }
/*		  b->client.w = e->w;
		  b->client.h = e->h;
*/		  
		  b->current.requested.w = e->w + pl + pr;
		  b->current.requested.h = e->h + pt + pb;
	       }
	     if ((e->mask & EV_VALUE_SIBLING) && (e->mask & EV_VALUE_STACKING))
	       {
		  E_Border *b_rel;
		  
		  b_rel = e_border_find_by_window(e->stack_win);
		  if (b_rel)
		    {
		       if (e->detail == EV_STACK_ABOVE) e_border_raise_above(b, b_rel);
		       else if (e->detail == EV_STACK_BELOW) e_border_lower_below(b, b_rel);
		       /* FIXME: need to handle  & fix
			* EV_STACK_TOP_IF 
			* EV_STACK_BOTTOM_IF 
			* EV_STACK_OPPOSITE 
			*/
		       else if (e->detail == EV_STACK_TOP_IF) e_border_raise(b);
		       else if (e->detail == EV_STACK_BOTTOM_IF) e_border_lower(b);
		    }
	       }
	     else if (e->mask & EV_VALUE_STACKING)
	       {
		  if (e->detail == EV_STACK_ABOVE) e_border_raise(b);
		  else if (e->detail == EV_STACK_BELOW) e_border_lower(b);
		       /* FIXME: need to handle  & fix
			* EV_STACK_TOP_IF 
			* EV_STACK_BOTTOM_IF 
			* EV_STACK_OPPOSITE 
			*/
		  else if (e->detail == EV_STACK_TOP_IF) e_border_raise(b);
		  else if (e->detail == EV_STACK_BOTTOM_IF) e_border_lower(b);
	       }
	     b->changed = 1;
	     e_border_adjust_limits(b);
	  }
	else
	  {
	     if ((e->mask & EV_VALUE_X) && (e->mask & EV_VALUE_W))
		e_window_move_resize(e->win, e->x, e->y, e->w, e->h);
	     else if ((e->mask & EV_VALUE_W) || (e->mask & EV_VALUE_H))
		e_window_resize(e->win, e->w, e->h);
	     else if ((e->mask & EV_VALUE_X))
		e_window_move(e->win, e->x, e->y);
	  }
     }
   current_ev = NULL;
}

/* */
static void
e_property(Eevent * ev)
{
   Ev_Window_Property      *e;
   
   current_ev = ev;
   e = ev->event;
     {
	E_Border *b;
	
	b = e_border_find_by_window(e->win);
	if (b)
	  {
	     e_icccm_handle_property_change(e->atom, b);
	  }
     }
   current_ev = NULL;
}

/* */
static void
e_client_message(Eevent * ev)
{
   Ev_Message      *e;
   
   current_ev = ev;
   e = ev->event;
   
   e_icccm_handle_client_message(e);

   current_ev = NULL;
}

/* */
static void
e_unmap(Eevent * ev)
{
   Ev_Window_Unmap      *e;
   
   current_ev = ev;
   e = ev->event;
     {
	E_Border *b;
	
	b = e_border_find_by_window(e->win);
	if (b)
	  {
	     if (b->win.client == e->win)
	       {
		  if (b->ignore_unmap > 0) b->ignore_unmap--;
		  else
		    {
		       e_action_stop_by_object(b, NULL, 
					       mouse_win_x, mouse_win_y, 
					       border_mouse_x, border_mouse_y);
		       OBJ_UNREF(b);
		       OBJ_IF_FREE(b)
			 {
			    e_window_reparent(e->win, 0, 0, 0);
			    e_icccm_release(e->win);
			    OBJ_FREE(b);
			 }
		    }
	       }
	  }
     }
   current_ev = NULL;
}

/* */
static void
e_destroy(Eevent * ev)
{
   Ev_Window_Destroy      *e;
   
   current_ev = ev;
   e = ev->event;
     {
	E_Border *b;
	
	b = e_border_find_by_window(e->win);
	if (b)
	  {
	     if (b->win.client == e->win)
	       {
		  e_action_stop_by_object(b, NULL, 
					  mouse_win_x, mouse_win_y, 
					  border_mouse_x, border_mouse_y);
		  OBJ_UNREF(b);
		  OBJ_IF_FREE(b)
		    {
		       e_window_reparent(e->win, 0, 0, 0);
		       e_icccm_release(e->win);
		       OBJ_FREE(b);
		    }
	       }
	  }
     }
   current_ev = NULL;
}

/* */
static void
e_circulate_request(Eevent * ev)
{
   Ev_Window_Circulate_Request      *e;
   
   current_ev = ev;
   e = ev->event;
     {
	E_Border *b;
	
	b = e_border_find_by_window(e->win);
	if (b)
	  {
	     if (e->lower) e_border_lower(b);
	     else e_border_raise(b);
	  }
     }
   current_ev = NULL;
}

/* */
static void
e_reparent(Eevent * ev)
{
   Ev_Window_Reparent      *e;
   
   current_ev = ev;
   e = ev->event;
#if 0   
     {
	E_Border *b;

	b = e_border_find_by_window(e->win);
	if ((b) && (e->parent_from == b->win.container))
	  {
	     if (b)
	       {
		  e_action_stop_by_object(b, NULL, 
					  mouse_win_x, mouse_win_y, 
					  border_mouse_x, border_mouse_y);
		  OBJ_UNREF(b);
		  OBJ_IF_FREE(b)
		    {
		       e_window_reparent(e->win, 0, 0, 0);
		       e_icccm_release(e->win);
		       OBJ_FREE(b);
		    }
	       }
	  }
     }
#endif   
   current_ev = NULL;
}

/* */
static void
e_shape(Eevent * ev)
{
   Ev_Window_Shape      *e;
   
   current_ev = ev;
   e = ev->event;
     {
	E_Border *b;
	
	b = e_border_find_by_window(e->win);
	if ((b) && (e->win == b->win.client))
	  {
	     b->current.shaped_client = e_icccm_is_shaped(e->win);
	     b->changed = 1;
	     b->shape_changed = 1;
	  }
     }
   current_ev = NULL;
}

/* */
static void
e_focus_in(Eevent * ev)
{
   Ev_Window_Focus_In   *e;
   
   current_ev = ev;
   e = ev->event;
     {
	E_Border *b;
	
	b = e_border_find_by_window(e->win);
	if (b)
	  {
	     b->current.selected = 1;
	     e_border_focus_grab_ended();
	     b->changed = 1;
	     OBS_NOTIFY(b, EV_WINDOW_FOCUS_IN);
	  }
     }
   current_ev = NULL;
}

/* */
static void
e_focus_out(Eevent * ev)
{
   Ev_Window_Focus_Out      *e;
   
   current_ev = ev;
   e = ev->event;
     {
	E_Border *b;
	
	b = e_border_find_by_window(e->win);
	if (b)
	  {	
	     /* char *settings_db; */
	     /* E_DB_File *db; */
	     int focus_mode;
	     /* char buf[4096]; */
	     E_CFG_INT(cfg_focus_mode, "settings", "/focus/mode", 0);
	     
	     E_CONFIG_INT_GET(cfg_focus_mode, focus_mode);
	     b->current.selected = 0;
	     if (e->key_grab) b->current.select_lost_from_grab = 1;
	     /* settings - click to focus would affect grabs */
	     if (!b->current.selected)
	       {
		  if (focus_mode == 2) /* click to focus */
		    {
		       E_Grab *g;
		       
		       g = NEW(E_Grab, 1);
		       ZERO(g, E_Grab, 1);
		       g->button = 0;
		       g->mods = 0;
		       g->any_mod = 1;
		       g->remove_after = 1;
		       b->grabs = evas_list_append(b->grabs, g);
		       e_button_grab(b->win.main, 0, XEV_BUTTON | XEV_MOUSE_MOVE, EV_KEY_MODIFIER_NONE, 1);
		    }
	       }
	     b->changed = 1;
	  }
	e_delayed_action_cancel(delayed_window_raise);
     }
   current_ev = NULL;
}

/* */
static void
e_colormap(Eevent * ev)
{
   Ev_Colormap      *e;
   
   current_ev = ev;
   e = ev->event;
     {
	E_Border *b;

	b = e_border_find_by_window(e->win);
	if (b)
	  {
	  }
     }
   current_ev = NULL;
}

/* handling mouse down events */
static void
e_mouse_down(Eevent * ev)
{
   Ev_Mouse_Down      *e;
   
   current_ev = ev;
   e = ev->event;
     {
	E_Border *b;
	
	mouse_win_x = e->x;
	mouse_win_y = e->y;
	mouse_x = e->rx;
	mouse_y = e->ry;
	mouse_buttons |= (1 << e->button);
	b = e_border_find_by_window(e->win);
	if (b)
	  {
	     if (e->win == b->win.main) e_cb_border_mouse_down(b, ev);
	     else
	       {
		  Evas evas;
		  int x, y;
		  
		  evas = b->evas.l;
		  e_window_get_root_relative_location(evas_get_window(evas), 
						      &x, &y);
		  x = e->rx - x;
		  y = e->ry - y;
		  evas_event_button_down(evas, x, y, e->button);
		  evas = b->evas.r;
		  e_window_get_root_relative_location(evas_get_window(evas), 
						      &x, &y);
		  x = e->rx - x;
		  y = e->ry - y;
		  evas_event_button_down(evas, x, y, e->button);
		  evas = b->evas.t;
		  e_window_get_root_relative_location(evas_get_window(evas), 
						      &x, &y);
		  x = e->rx - x;
		  y = e->ry - y;
		  evas_event_button_down(evas, x, y, e->button);
		  evas = b->evas.b;
		  e_window_get_root_relative_location(evas_get_window(evas), 
						      &x, &y);
		  x = e->rx - x;
		  y = e->ry - y;
		  evas_event_button_down(evas, x, y, e->button);
	       }
	  }
     }
   current_ev = NULL;
}

/* handling mouse up events */
static void
e_mouse_up(Eevent * ev)
{
   Ev_Mouse_Up      *e;
   
   current_ev = ev;
   e = ev->event;
     {
	E_Border *b;
	
	mouse_win_x = e->x;
	mouse_win_y = e->y;
	mouse_x = e->rx;
	mouse_y = e->ry;
	mouse_buttons &= ~(1 << e->button);
	b = e_border_find_by_window(e->win);
	if (b)
	  {	
	     if (e->win == b->win.main) e_cb_border_mouse_up(b, ev);
	     else
	       {
		  Evas evas;
		  int x, y;
		       
		  evas = b->evas.l;
		  e_window_get_root_relative_location(evas_get_window(evas), 
						      &x, &y);
		  x = e->rx - x;
		  y = e->ry - y;
		  evas_event_button_up(evas, x, y, e->button);
		  evas = b->evas.r;
		  e_window_get_root_relative_location(evas_get_window(evas), 
						      &x, &y);
		  x = e->rx - x;
		  y = e->ry - y;
		  evas_event_button_up(evas, x, y, e->button);
		  evas = b->evas.t;
		  e_window_get_root_relative_location(evas_get_window(evas), 
						      &x, &y);
		  x = e->rx - x;
		  y = e->ry - y;
		  evas_event_button_up(evas, x, y, e->button);
		  evas = b->evas.b;
		  e_window_get_root_relative_location(evas_get_window(evas), 
						      &x, &y);
		  x = e->rx - x;
		  y = e->ry - y;
		  evas_event_button_up(evas, x, y, e->button);
	       }
	  }
     }
   current_ev = NULL;
}

/* handling mouse move events */
static void
e_mouse_move(Eevent * ev)
{
   Ev_Mouse_Move      *e;

   current_ev = ev;
   e = ev->event;
     {
	E_Border *b;
	
	mouse_win_x = e->x;
	mouse_win_y = e->y;
	mouse_x = e->rx;
	mouse_y = e->ry;
	b = e_border_find_by_window(e->win);
	if (b)
	  {
	     if (e->win == b->win.main) e_cb_border_mouse_move(b, ev);
	     else
	       {
		  Evas evas;
		  int x, y;
		  
		  evas = b->evas.l;
		  e_window_get_root_relative_location(evas_get_window(evas), 
						      &x, &y);
		  x = e->rx - x;
		  y = e->ry - y;
		  evas_event_move(evas, x, y);
		  evas = b->evas.r;
		  e_window_get_root_relative_location(evas_get_window(evas), 
						      &x, &y);
		  x = e->rx - x;
		  y = e->ry - y;
		  evas_event_move(evas, x, y);
		  evas = b->evas.t;
		  e_window_get_root_relative_location(evas_get_window(evas), 
						      &x, &y);
		  x = e->rx - x;
		  y = e->ry - y;
		  evas_event_move(evas, x, y);
		  evas = b->evas.b;
		  e_window_get_root_relative_location(evas_get_window(evas), 
						      &x, &y);
		  x = e->rx - x;
		  y = e->ry - y;
		  evas_event_move(evas, x, y);
	       }
	  }
     }
   current_ev = NULL;
}

/* handling mouse enter events */
static void
e_mouse_in(Eevent * ev)
{
   Ev_Window_Enter      *e;
   
   current_ev = ev;
   e = ev->event;
     {
	E_Border *b;
	
	b = e_border_find_by_window(e->win);
	if (b)
	  {
	     if (e->win == b->win.main) e_cb_border_mouse_in(b, ev);
	     if (e->win == b->win.input)
	       {
		  int x, y;
		  Evas evas;
		  
		  evas = b->evas.l;
		  e_window_get_root_relative_location(evas_get_window(evas), &x, &y);
		  x = e->rx - x;
		  y = e->ry - y;
		  evas_event_move(evas, x, y);
		  evas_event_enter(evas);
		  
		  evas = b->evas.r;
		  e_window_get_root_relative_location(evas_get_window(evas), &x, &y);
		  x = e->rx - x;
		  y = e->ry - y;
		  evas_event_move(evas, x, y);
		  evas_event_enter(evas);
		  
		  evas = b->evas.t;
		  e_window_get_root_relative_location(evas_get_window(evas), &x, &y);
		  x = e->rx - x;
		  y = e->ry - y;
		  evas_event_move(evas, x, y);
		  evas_event_enter(evas);
		  
		  evas = b->evas.b;
		  e_window_get_root_relative_location(evas_get_window(evas), &x, &y);
		  x = e->rx - x;
		  y = e->ry - y;
		  evas_event_move(evas, x, y);
		  evas_event_enter(evas);
	       }
	  }
     }
   current_ev = NULL;
}

/* handling mouse leave events */
static void
e_mouse_out(Eevent * ev)
{
   Ev_Window_Leave      *e;
   
   current_ev = ev;
   e = ev->event;
     {
	E_Border *b;
	
	b = e_border_find_by_window(e->win);
	if (b)
	  {
	     if (e->win == b->win.main) e_cb_border_mouse_out(b, ev);
	     if (e->win == b->win.input)
	       {
		  evas_event_leave(b->evas.l);
		  evas_event_leave(b->evas.r);
		  evas_event_leave(b->evas.t);
		  evas_event_leave(b->evas.b);
	       }
	  }
     }
   current_ev = NULL;
   current_ev = NULL;
}

/* handling expose events */
static void
e_window_expose(Eevent * ev)
{
   Ev_Window_Expose      *e;
   
   current_ev = ev;
   e = ev->event;
     {
	Evas_List l;
	E_Border *b;
	
	for (l = evases; l; l = l->next)
	  {
	     Evas evas;

	     evas = l->data;
	     if (evas_get_window(evas) == e->win)
	       evas_update_rect(evas, e->x, e->y, e->w, e->h);
	  }
	b = e_border_find_by_window(e->win);
	if (b) b->first_expose = 1;
     }
   current_ev = NULL;
}

/* what to do with border events */

static void
e_cb_mouse_in(void *data, Ebits_Object o, char *class, int bt, int x, int y, int ox, int oy, int ow, int oh)
{
   E_Border *b;
   
   b = data;
   if (border_mouse_buttons) return;
   border_mouse_x = mouse_x;
   border_mouse_y = mouse_y;
   if (class) e_cursors_display_in_window(b->win.main, class);
   else e_cursors_display_in_window(b->win.main, "Default");     
   if (!current_ev) return;
   e_action_stop(class, ACT_MOUSE_IN, 0, NULL, EV_KEY_MODIFIER_NONE, b, NULL, x, y, border_mouse_x, border_mouse_y);
   e_action_start(class, ACT_MOUSE_IN, 0, NULL, EV_KEY_MODIFIER_NONE, b, NULL, x, y, border_mouse_x, border_mouse_y);
   return;
   UN(o);
   UN(bt);
   UN(ox);
   UN(oy);
   UN(ow);
   UN(oh);
}

static void
e_cb_mouse_out(void *data, Ebits_Object o, char *class, int bt, int x, int y, int ox, int oy, int ow, int oh)
{
   E_Border *b;
   
   b = data;
   if (border_mouse_buttons) return;
   border_mouse_x = mouse_x;
   border_mouse_y = mouse_y;
   if (!current_ev) return;
   e_cursors_display_in_window(b->win.main, "Default");
   e_action_stop(class, ACT_MOUSE_OUT, 0, NULL, EV_KEY_MODIFIER_NONE, b, NULL, x, y, border_mouse_x, border_mouse_y);
   e_action_start(class, ACT_MOUSE_OUT, 0, NULL, EV_KEY_MODIFIER_NONE, b, NULL, x, y, border_mouse_x, border_mouse_y);
   return;
   UN(o);
   UN(bt);
   UN(ox);
   UN(oy);
   UN(ow);
   UN(oh);
}

static void
e_cb_mouse_down(void *data, Ebits_Object o, char *class, int bt, int x, int y, int ox, int oy, int ow, int oh)
{
   E_Border *b;
   
   b = data;
   border_mouse_x = mouse_x;
   border_mouse_y = mouse_y;
   border_mouse_buttons = mouse_buttons;
   if (!current_ev) return;
     {
	E_Action_Type act;
	Ev_Key_Modifiers mods;
	
	mods = ((Ev_Mouse_Down *)(current_ev->event))->mods;
	act = ACT_MOUSE_CLICK;
	if (((Ev_Mouse_Down *)(current_ev->event))->double_click) act = ACT_MOUSE_DOUBLE;
	else if (((Ev_Mouse_Down *)(current_ev->event))->triple_click) act = ACT_MOUSE_TRIPLE;
	e_action_stop(class, act, bt, NULL, mods, b, NULL, x, y, border_mouse_x, border_mouse_y);
	e_action_start(class, act, bt, NULL, mods, b, NULL, x, y, border_mouse_x, border_mouse_y);
     }
   return;
   UN(o);
   UN(ox);
   UN(oy);
   UN(ow);
   UN(oh);
}

static void
e_cb_mouse_up(void *data, Ebits_Object o, char *class, int bt, int x, int y, int ox, int oy, int ow, int oh)
{
   E_Border *b;
   
   b = data;
   border_mouse_x = mouse_x;
   border_mouse_y = mouse_y;
   border_mouse_buttons = mouse_buttons;
   if (!current_ev) return;
     {
	E_Action_Type act;
	Ev_Key_Modifiers mods;
	
	mods = ((Ev_Mouse_Up *)(current_ev->event))->mods;
	act = ACT_MOUSE_UP;
	if ((x >= ox) && (x < (ox + ow)) && (y >= oy) && (y < (oy + oh)))
	  act = ACT_MOUSE_CLICKED;
	e_action_stop(class, act, bt, NULL, mods, b, NULL, x, y, border_mouse_x, border_mouse_y);
	e_action_start(class, act, bt, NULL, mods, b, NULL, x, y, border_mouse_x, border_mouse_y);
     }
   return;
   UN(o);
}

static void
e_cb_mouse_move(void *data, Ebits_Object o, char *class, int bt, int x, int y, int ox, int oy, int ow, int oh)
{
   E_Border *b;
   int dx, dy;
   
   b = data;
   dx = mouse_x - border_mouse_x;
   dy = mouse_y - border_mouse_y;
   border_mouse_x = mouse_x;
   border_mouse_y = mouse_y;
   if (!current_ev) return;
   e_action_go(class, ACT_MOUSE_MOVE, 0, NULL, EV_KEY_MODIFIER_NONE, b, NULL, x, y, border_mouse_x, border_mouse_y, dx, dy);
   return;
   UN(o);
   UN(bt);
   UN(ox);
   UN(oy);
   UN(ow);
   UN(oh);
}

/* callbacks for certain things */
static void
e_cb_border_mouse_in(E_Border *b, Eevent *e)
{
   int x, y;
   char *class = "Window_Grab";
   
   if (border_mouse_buttons) return;
   /* pointer focus stuff */
   if (b->client.takes_focus) e_focus_to_window(b->win.client);

   border_mouse_x = mouse_x;
   border_mouse_y = mouse_y;
   border_mouse_buttons = mouse_buttons;
   if (!current_ev) return;
   x = ((Ev_Window_Enter *)(e->event))->x;
   y = ((Ev_Window_Enter *)(e->event))->y;
   e_action_stop(class, ACT_MOUSE_IN, 0, NULL, EV_KEY_MODIFIER_NONE, b, NULL, x, y, border_mouse_x, border_mouse_y);
   e_action_start(class, ACT_MOUSE_IN, 0, NULL, EV_KEY_MODIFIER_NONE, b, NULL, x, y, border_mouse_x, border_mouse_y);
}

static void
e_cb_border_mouse_out(E_Border *b, Eevent *e)
{
   int x, y;
   char *class = "Window_Grab";

   if (border_mouse_buttons) return;
   /* pointer focus stuff */
/*   e_focus_to_window(0);*/

   x = mouse_x;
   y = mouse_y;
   border_mouse_x = mouse_x;
   border_mouse_y = mouse_y;
   border_mouse_buttons = mouse_buttons;
   if (!current_ev) return;
   e_action_stop(class, ACT_MOUSE_OUT, 0, NULL, EV_KEY_MODIFIER_NONE, b, NULL, x, y, border_mouse_x, border_mouse_y);
   e_action_start(class, ACT_MOUSE_OUT, 0, NULL, EV_KEY_MODIFIER_NONE, b, NULL, x, y, border_mouse_x, border_mouse_y);
   return;
   UN(e);
}

static void
e_cb_border_mouse_down(E_Border *b, Eevent *e)
{
   int x, y, bt;
   char *class = "Window_Grab";
   
   e_pointer_grab(b->win.main, CurrentTime);
   border_mouse_x = mouse_x;
   border_mouse_y = mouse_y;
   if (border_mouse_buttons) return;
   border_mouse_buttons = mouse_buttons;
   if (!current_ev) return;
   x = ((Ev_Mouse_Down *)(e->event))->x;
   y = ((Ev_Mouse_Down *)(e->event))->y;
   bt = ((Ev_Mouse_Down *)(e->event))->button;
     {
	Evas_List l;
	
	again:
	for (l = b->grabs; l; l = l->next)
	  {
	     E_Grab *g;
	     
	     g = l->data;
	     /* find a grab that triggered this */
	     if ((((Ev_Mouse_Down *)(e->event))->button == g->button) &&
		 ((g->any_mod) ||
		  (((Ev_Mouse_Down *)(e->event))->mods == g->mods)))
	       {
		  if (g->allow)
		    e_pointer_replay(((Ev_Mouse_Down *)(e->event))->time);
		  if (g->remove_after)
		    {
		       e_button_ungrab(b->win.main, g->button, g->mods, g->any_mod);
		       free(g);
		       b->grabs = evas_list_remove(b->grabs, g);
		       goto again;
		    }
	       }
	  }
     }
     {
	E_Action_Type act;
	Ev_Key_Modifiers mods;
	
	mods = ((Ev_Mouse_Down *)(current_ev->event))->mods;
	act = ACT_MOUSE_CLICK;
	if (((Ev_Mouse_Down *)(current_ev->event))->double_click) act = ACT_MOUSE_DOUBLE;
	else if (((Ev_Mouse_Down *)(current_ev->event))->triple_click) act = ACT_MOUSE_TRIPLE;
	e_action_stop(class, act, bt, NULL, mods, b, NULL, x, y, border_mouse_x, border_mouse_y);
	e_action_start(class, act, bt, NULL, mods, b, NULL, x, y, border_mouse_x, border_mouse_y);
     }
/*
 * e_pointer_ungrab(((Ev_Mouse_Up *)(e->event))->time);
 * e_pointer_replay(((Ev_Mouse_Up *)(e->event))->time);
 */
}

static void
e_cb_border_mouse_up(E_Border *b, Eevent *e)
{
   int x, y, bt;
   char *class = "Window_Grab";
   
   e_pointer_ungrab(CurrentTime);   
   border_mouse_x = mouse_x;
   border_mouse_y = mouse_y;
   border_mouse_buttons = mouse_buttons;
   if (!current_ev) return;
   x = ((Ev_Mouse_Up *)(e->event))->x;
   y = ((Ev_Mouse_Up *)(e->event))->y;
   bt = ((Ev_Mouse_Up *)(e->event))->button;
     {
	E_Action_Type act;
	Ev_Key_Modifiers mods;
	
	mods = ((Ev_Mouse_Up *)(current_ev->event))->mods;
	act = ACT_MOUSE_UP;
	e_action_stop(class, act, bt, NULL, mods, b, NULL, x, y, border_mouse_x, border_mouse_y);
	e_action_start(class, act, bt, NULL, mods, b, NULL, x, y, border_mouse_x, border_mouse_y);
     }
}

static void
e_cb_border_mouse_move(E_Border *b, Eevent *e)
{
   int dx, dy;
   int x, y;
   char *class = "Window_Grab";
   
   dx = mouse_x - border_mouse_x;
   dy = mouse_y - border_mouse_y;
   border_mouse_x = mouse_x;
   border_mouse_y = mouse_y;
   if (!current_ev) return;
   x = ((Ev_Mouse_Move *)(e->event))->x;
   y = ((Ev_Mouse_Move *)(e->event))->y;
   e_action_go(class, ACT_MOUSE_MOVE, 0, NULL, EV_KEY_MODIFIER_NONE, b, NULL, x, y, border_mouse_x, border_mouse_y, dx, dy);
}

static void
e_cb_border_move_resize(E_Border *b)
{
   return;
   UN(b);
}

static void
e_cb_border_visibility(E_Border *b)
{
   return;
   UN(b);
}

static void e_border_poll(int val, void *data)
{
   e_add_event_timer("e_border_poll()", 1.00, e_border_poll, val + 1, NULL);
   return;
   UN(data);
}

/* border creation, deletion, modification and general queries */

void
e_border_apply_border(E_Border *b)
{
   int pl, pr, pt, pb;
   char *borders, buf[4096], border[4096], *style = NULL;
   int prop_selected = 0, prop_sticky = 0, prop_shaded = 0;
   
   style = "default"; 
   if ((!b->client.titlebar) && (!b->client.border)) style = "borderless";
   
   if (b->current.selected)              prop_selected = 1;
   if (b->current.shaded == b->client.h) prop_shaded = 1;
   if (b->client.sticky)                 prop_sticky = 1;
   
   sprintf(border, "selected-%i.sticky-%i.shaded-%i.bits.db", 
	   prop_selected, prop_sticky, prop_shaded);

   borders = e_config_get("borders");
   sprintf(buf, "%s%s/%s", borders, style, border);
   /* if it's not changed - abort and dont do anything */
   if ((b->border_file) && (!strcmp(buf, b->border_file))) return;
   IF_FREE(b->border_file);
   e_strdup(b->border_file, buf);
   
   e_border_set_bits(b, buf);
   
   pl = pr = pt = pb = 0;
   if (b->bits.t) ebits_get_insets(b->bits.t, &pl, &pr, &pt, &pb);   
   e_icccm_set_frame_size(b->win.client, pl, pr, pt, pb);
}

void
e_border_reshape(E_Border *b)
{
   static Window shape_win = 0;
   int pl, pr, pt, pb;
   
   if ((b->current.shaped_client == b->previous.shaped_client) &&
       (b->current.shape_changes == b->previous.shape_changes) &&
       (b->current.has_shape == b->previous.has_shape) && 
       (!b->shape_changed))
     return;
   if (!shape_win) shape_win = e_window_override_new(0, 0, 0, 1, 1);
   pl = pr = pt = pb = 0;
   if (b->bits.t) ebits_get_insets(b->bits.t, &pl, &pr, &pt, &pb);
   b->shape_changed = 0;
   if ((!b->current.shaped_client) && (!b->current.has_shape))
     {
	e_window_set_shape_mask(b->win.main, 0);
	return;
     }
   if ((b->current.shaped_client) && (!b->current.has_shape))
     {
	XRectangle rects[4];
	
	rects[0].x = 0; rects[0].y = 0; 
	rects[0].width = b->current.w; rects[0].height = pt;
	rects[1].x = 0; rects[1].y = pt; 
	rects[1].width = pl; rects[1].height = b->current.h - pt - pb;
	rects[2].x = b->current.w - pr; rects[2].y = pt; 
	rects[2].width = pr; rects[2].height = b->current.h - pt - pb;
	rects[3].x = 0; rects[3].y = b->current.h - pb; 
	rects[3].width = b->current.w; rects[3].height = pb;
	
        e_window_resize(shape_win, b->current.w, b->current.h);
	e_window_set_shape_window(shape_win, b->win.client, pl, pt - b->current.shaded);
	e_window_clip_shape_by_rectangle(shape_win, pl, pt, b->current.w - pl - pr, b->current.h - pt - pb);
	e_window_add_shape_rectangles(shape_win, rects, 4);
	e_window_set_shape_window(b->win.main, shape_win, 0, 0);
	return;
     }
   if ((!b->current.shaped_client) && (b->current.has_shape))
     {
        e_window_resize(shape_win, b->current.w, b->current.h);
	e_window_set_shape_rectangle(shape_win, pl, pt - b->current.shaded, b->current.w - pl - pr, b->current.h - pt - pb);
	/* FIXME: later when i actually generate shape masks for borders */
	  {
	     XRectangle rects[4];
	     
	     rects[0].x = 0; rects[0].y = 0; 
	     rects[0].width = b->current.w; rects[0].height = pt;
	     rects[1].x = 0; rects[1].y = pt; 
	     rects[1].width = pl; rects[1].height = b->current.h - pt - pb;
	     rects[2].x = b->current.w - pr; rects[2].y = pt; 
	     rects[2].width = pr; rects[2].height = b->current.h - pt - pb;
	     rects[3].x = 0; rects[3].y = b->current.h - pb; 
	     rects[3].width = b->current.w; rects[3].height = pb;
	     e_window_add_shape_rectangles(shape_win, rects, 4);
	  }
	e_window_set_shape_window(b->win.main, shape_win, 0, 0);
	return;
     }
   if ((b->current.shaped_client) && (b->current.has_shape))
     {
        e_window_resize(shape_win, b->current.w, b->current.h);
	e_window_set_shape_window(shape_win, b->win.client, pl, pt - b->current.shaded);
	e_window_clip_shape_by_rectangle(shape_win, pl, pt, b->current.w - pl - pr, b->current.h - pt - pb);
	/* FIXME: later when i actually generate shape masks for borders */
	  {
	     XRectangle rects[4];
	     
	     rects[0].x = 0; rects[0].y = 0; 
	     rects[0].width = b->current.w; rects[0].height = pt;
	     rects[1].x = 0; rects[1].y = pt; 
	     rects[1].width = pl; rects[1].height = b->current.h - pt - pb;
	     rects[2].x = b->current.w - pr; rects[2].y = pt; 
	     rects[2].width = pr; rects[2].height = b->current.h - pt - pb;
	     rects[3].x = 0; rects[3].y = b->current.h - pb; 
	     rects[3].width = b->current.w; rects[3].height = pb;
	     e_window_add_shape_rectangles(shape_win, rects, 4);
	  }
	e_window_set_shape_window(b->win.main, shape_win, 0, 0);
	return;
     }
}

E_Border * 
e_border_adopt(Window win, int use_client_pos)
{
   E_Border *b;
   int bw;
   int show = 1;
   
   /* create the struct */
   b = e_border_new();
   /* set the right event on the client */
   e_window_set_events(win, 
		       XEV_VISIBILITY | 
		       ResizeRedirectMask | 
		       XEV_CONFIGURE | 
		       XEV_FOCUS | 
		       XEV_PROPERTY | 
		       XEV_COLORMAP);
   e_window_select_shape_events(win);
   /* parent of the client window listens for these */
   e_window_set_events(b->win.container, XEV_CHILD_CHANGE | XEV_CHILD_REDIRECT);
   /* add to save set & border of 0 */
   e_icccm_adopt(win);
   bw = e_window_get_border_width(win);
   e_window_set_border_width(win, 0);
   b->win.client = win;
   b->current.requested.visible = 1;
   /* get hints */
   e_icccm_get_size_info(win, b);
   e_icccm_get_pos_info(win, b);
     {
	int x, y, w, h;
	
	e_window_get_geometry(win, &x, &y, &w, &h);
        b->current.requested.x = x;
	b->current.requested.y = y;	
	b->current.requested.w = w;
	b->current.requested.h = h;
     }
	
   e_icccm_get_mwm_hints(win, b);
   e_icccm_get_layer(win, b);
   e_icccm_get_title(win, b);
   e_icccm_get_class(win, b);
   e_icccm_get_hints(win, b);
   e_icccm_get_machine(win, b);
   e_icccm_get_command(win, b);
   e_icccm_get_icon_name(win, b);
   b->current.shaped_client = e_icccm_is_shaped(win);
   /* we have now placed the bugger */
   b->placed = 1;
   /* desk area */
   e_icccm_set_desk_area(win, 0, 0);
   e_icccm_set_desk(win, e_desktops_get_current());
   if (use_client_pos)
     {
	int x, y;
	
	e_window_get_geometry(win, &x, &y, NULL, NULL);
	b->current.requested.x = x;
	b->current.requested.y = y;
	b->changed = 1;
     }
   /* reparent the window finally */
   e_window_reparent(win, b->win.container, 0, 0);
   /* figure what border to use */
   e_border_apply_border(b);
     {
	int pl, pr, pt, pb;
        
	pl = pr = pt = pb = 0;
	if (b->bits.l) ebits_get_insets(b->bits.l, &pl, &pr, &pt, &pb);
	b->current.requested.x += pl;
	b->current.requested.y += pt;
	b->changed = 1;
     }
   if (!use_client_pos)
     {
	int x, y;
        int pl, pr, pt, pb;
	
	pl = pr = pt = pb = 0;
	x = y = 0;
	if (b->bits.t) ebits_get_insets(b->bits.t, &pl, &pr, &pt, &pb);
	bw *= 2;
	if (b->client.pos.requested)
	  {
	     switch (b->client.pos.gravity)
	       {
		case NorthWestGravity:
		  x = b->client.pos.x + pl;
		  y = b->client.pos.y + pt;
		  break;
		case NorthGravity:
		  x = b->client.pos.x + (bw / 2);
		  y = b->client.pos.y + pt;
		  break;
		case NorthEastGravity:
		  x = b->client.pos.x - pr + bw;
		  y = b->client.pos.y + pt;
		  break;
		case EastGravity:
		  x = b->client.pos.x - pr + bw;
		  y = b->client.pos.y + (bw / 2);
		  break;
		case SouthEastGravity:
		  x = b->client.pos.x - pr + bw;
		  y = b->client.pos.y - pb + bw;
		  break;
		case SouthGravity:
		  x = b->client.pos.x + (bw / 2);
		  y = b->client.pos.y - pb;
		  break;
		case SouthWestGravity:
		  x = b->client.pos.x + pl;
		  y = b->client.pos.y - pb + bw;
		  break;
		case WestGravity:
		  x = b->client.pos.x + pl;
		  y = b->client.pos.y + (bw / 2);
		  break;
		case CenterGravity:
		  x = b->client.pos.x;
		  y = b->client.pos.y;
		  break;
		case StaticGravity:
		  x = b->client.pos.x;
		  y = b->client.pos.y;
		  break;
		case ForgetGravity:
		  x = b->client.pos.x;
		  y = b->client.pos.y;
		  break;
		default:
		  x = b->client.pos.x + pl;
		  y = b->client.pos.y + pt;
		  break;
	       }
	  }
	else
	  {
	     show = e_place_border(b, b->desk, &x, &y, E_PLACE_SMART);
	     x += pl;
	     y += pt;
	  }
	b->current.requested.x = x - pl;
	b->current.requested.y = y - pt;
	b->changed = 1;
     }
   /* show the client */
   e_icccm_state_mapped(win);
   /* fix size so it matches the hints a client asks for */
   b->changed = 1;
   e_border_adjust_limits(b);
   b->current.requested.h = b->current.h;
   b->current.requested.w = b->current.w;
   e_border_raise(b);
   e_window_show(win);
   return b;
}

E_Border *
e_border_new(void)
{
   /* FIXME: need to set an upper limit on the frame size */
   E_Border *b;
   int max_colors = 216;
   int font_cache = 1024 * 1024;
   int image_cache = 8192 * 1024;
   char *font_dir;
   E_Desktop *desk;
   
   font_dir = e_config_get("fonts");
   b = NEW(E_Border, 1);
   ZERO(b, E_Border, 1);
   
   OBJ_INIT(b, e_border_free);
   OBS_REGISTER(delayed_window_raise, b);
   
   b->current.requested.w = 1;
   b->current.requested.h = 1;
   b->client.min.w = 1;
   b->client.min.h = 1;
   b->client.max.w = 1000000;
   b->client.max.h = 1000000;
   b->client.min.aspect = -1000000;
   b->client.max.aspect = 1000000;
   b->client.step.w = 1;
   b->client.step.h = 1;
   b->client.layer = 4;
   b->client.border = 1;
   b->client.handles = 1;
   b->client.titlebar = 1;
   b->client.takes_focus = 1;
   
   desk = e_desktops_get(0);
   e_desktops_add_border(desk, b);
   b->win.main = e_window_override_new(desk->win.container, 0, 0, 1, 1);
   b->win.input = e_window_input_new(b->win.main, 0, 0, 1, 1);
   b->win.container = e_window_override_new(b->win.main, 0, 0, 1, 1);
   e_cursors_display_in_window(b->win.container, "Application");
   e_window_set_events_propagate(b->win.input, 1);
   e_window_set_events(b->win.input, XEV_MOUSE_MOVE | XEV_BUTTON);
   e_window_set_events(b->win.main, XEV_IN_OUT);
   e_window_set_events(b->win.container, XEV_IN_OUT);
   e_window_show(b->win.input);
   e_window_show(b->win.container);

   b->evas.l = evas_new_all(e_display_get(),
			    b->win.main,
			    0, 0, 1, 1,
			    RENDER_METHOD_ALPHA_SOFTWARE,
			    max_colors,
			    font_cache, 
			    image_cache,
			    font_dir);
   b->win.l = evas_get_window(b->evas.l);
   b->evas.r = evas_new_all(e_display_get(),
			    b->win.main,
			    0, 0, 1, 1,
			    RENDER_METHOD_ALPHA_SOFTWARE,
			    max_colors,
			    font_cache, 
			    image_cache,
			    font_dir);
   b->win.r = evas_get_window(b->evas.r);
   b->evas.t = evas_new_all(e_display_get(),
			    b->win.main,
			    0, 0, 1, 1,
			    RENDER_METHOD_ALPHA_SOFTWARE,
			    max_colors,
			    font_cache, 
			    image_cache,
			    font_dir);
   b->win.t = evas_get_window(b->evas.t);
   b->evas.b = evas_new_all(e_display_get(),
			    b->win.main,
			    0, 0, 1, 1,
			    RENDER_METHOD_ALPHA_SOFTWARE,
			    max_colors,
			    font_cache, 
			    image_cache,
			    font_dir);
   b->win.b = evas_get_window(b->evas.b); 
   e_cursors_display_in_window(b->win.l, "Default");
   e_cursors_display_in_window(b->win.r, "Default");
   e_cursors_display_in_window(b->win.t, "Default");
   e_cursors_display_in_window(b->win.b, "Default");

   b->obj.title.l = evas_add_text(b->evas.l, "borzoib", 8, "");
   b->obj.title.r = evas_add_text(b->evas.r, "borzoib", 8, "");
   b->obj.title.t = evas_add_text(b->evas.t, "borzoib", 8, "");
   b->obj.title.b = evas_add_text(b->evas.b, "borzoib", 8, "");
   
   b->obj.title_clip.l = evas_add_rectangle(b->evas.l);
   b->obj.title_clip.r = evas_add_rectangle(b->evas.r);
   b->obj.title_clip.t = evas_add_rectangle(b->evas.t);
   b->obj.title_clip.b = evas_add_rectangle(b->evas.b);

   evas_set_color(b->evas.l, b->obj.title_clip.l, 255, 255, 255, 255);
   evas_set_color(b->evas.r, b->obj.title_clip.r, 255, 255, 255, 255);
   evas_set_color(b->evas.t, b->obj.title_clip.t, 255, 255, 255, 255);
   evas_set_color(b->evas.b, b->obj.title_clip.b, 255, 255, 255, 255);
   
   evas_set_pass_events(b->evas.l, b->obj.title.l, 1);
   evas_set_pass_events(b->evas.r, b->obj.title.r, 1);
   evas_set_pass_events(b->evas.t, b->obj.title.t, 1);
   evas_set_pass_events(b->evas.b, b->obj.title.b, 1);
   
   evas_set_color(b->evas.l, b->obj.title.l, 0, 0, 0, 255);
   evas_set_color(b->evas.r, b->obj.title.r, 0, 0, 0, 255);
   evas_set_color(b->evas.t, b->obj.title.t, 0, 0, 0, 255);
   evas_set_color(b->evas.b, b->obj.title.b, 0, 0, 0, 255);

   evas_show(b->evas.l, b->obj.title.l);
   evas_show(b->evas.r, b->obj.title.r);
   evas_show(b->evas.t, b->obj.title.t);
   evas_show(b->evas.b, b->obj.title.b);

   evas_show(b->evas.l, b->obj.title_clip.l);
   evas_show(b->evas.r, b->obj.title_clip.r);
   evas_show(b->evas.t, b->obj.title_clip.t);
   evas_show(b->evas.b, b->obj.title_clip.b);
   
   evas_set_clip(b->evas.l, b->obj.title.l, b->obj.title_clip.l);
   evas_set_clip(b->evas.l, b->obj.title.r, b->obj.title_clip.r);
   evas_set_clip(b->evas.l, b->obj.title.t, b->obj.title_clip.t);
   evas_set_clip(b->evas.l, b->obj.title.b, b->obj.title_clip.b);
   
   e_window_raise(b->win.input);
   e_window_raise(b->win.container);
/*   
   e_window_raise(b->win.l);
   e_window_raise(b->win.r);
   e_window_raise(b->win.t);
   e_window_raise(b->win.b);
*/   

   evases = evas_list_append(evases, b->evas.l);
   evases = evas_list_append(evases, b->evas.r);
   evases = evas_list_append(evases, b->evas.t);
   evases = evas_list_append(evases, b->evas.b);
   
   e_window_set_events(b->win.l, XEV_EXPOSE);
   e_window_set_events(b->win.r, XEV_EXPOSE);
   e_window_set_events(b->win.t, XEV_EXPOSE);
   e_window_set_events(b->win.b, XEV_EXPOSE);
   
   e_window_show(b->win.l);
   e_window_show(b->win.r);
   e_window_show(b->win.t);
   e_window_show(b->win.b);
   
   e_border_attach_mouse_grabs(b);

   borders = evas_list_prepend(borders, b);
   
   return b;
}

void
e_border_free(E_Border *b)
{
   Evas_List l;

   e_desktops_del_border(b->desk, b);
   if (b->bits.l) ebits_free(b->bits.l);
   if (b->bits.r) ebits_free(b->bits.r);
   if (b->bits.t) ebits_free(b->bits.t);
   if (b->bits.b) ebits_free(b->bits.b);
   evases = evas_list_remove(evases, b->evas.l);
   evases = evas_list_remove(evases, b->evas.r);
   evases = evas_list_remove(evases, b->evas.t);
   evases = evas_list_remove(evases, b->evas.b);
   evas_free(b->evas.l);
   evas_free(b->evas.r);
   evas_free(b->evas.t);
   evas_free(b->evas.b);
   e_window_destroy(b->win.container);
   e_window_destroy(b->win.input);
   e_window_destroy(b->win.main);
   borders = evas_list_remove(borders, b);
   
   IF_FREE(b->client.title);
   IF_FREE(b->client.name);
   IF_FREE(b->client.class);
   IF_FREE(b->border_file);
   
   if (b->grabs)
     {
	for (l = b->grabs; l; l = l->next)
	  {
	     FREE(l->data);
	  }
	evas_list_free(b->grabs);
     }
   
   FREE(b);
}

void
e_border_remove_mouse_grabs(E_Border *b)
{
   Evas_List l;
   
   if (b->grabs)
     {
	for (l = b->grabs; l; l = l->next)
	  {
	     E_Grab *g;
	     
	     g = l->data;
	     e_button_ungrab(b->win.main, g->button, g->mods, g->any_mod);
	     FREE(g);
	  }
	evas_list_free(b->grabs);
	b->grabs = NULL;
     }
}

void
e_border_attach_mouse_grabs(E_Border *b)
{
   char *grabs_db;
   E_DB_File *db;
   int focus_mode;
   char buf[4096];
   E_CFG_INT(cfg_focus_mode, "settings", "/focus/mode", 0);
   
   E_CONFIG_INT_GET(cfg_focus_mode, focus_mode);
   
   grabs_db = e_config_get("grabs");
   /* settings - click to focus would affect grabs */
   if ((!b->current.selected))
     {
	if (focus_mode == 2) /* click to focus */
	  {
	     E_Grab *g;
	     
	     g = NEW(E_Grab, 1);
	     ZERO(g, E_Grab, 1);
	     g->button = 0;
	     g->mods = 0;
	     g->any_mod = 1;
	     g->remove_after = 1;
	     b->grabs = evas_list_append(b->grabs, g);
	     e_button_grab(b->win.main, 0, XEV_BUTTON | XEV_MOUSE_MOVE, EV_KEY_MODIFIER_NONE, 1);
	  }
     }
   
   /* other grabs - liek alt+left to move */
   db = e_db_open_read(grabs_db);
   if (db)
     {
	int i, num;
	
	sprintf(buf, "/grabs/count");
	if (!e_db_int_get(db, buf, &num))
	  {
	     e_db_close(db);
	     return;
	  }
	for (i = 0; i < num; i++)
	  {
	     int button, any_mod, mod;
	     Ev_Key_Modifiers mods;

	     button = -1;
	     mods = EV_KEY_MODIFIER_NONE;
	     any_mod = 0;
	     sprintf(buf, "/grabs/%i/button", i);
	     if (!e_db_int_get(db, buf, &button)) continue;
	     sprintf(buf, "/grabs/%i/modifiers", i);
	     if (!e_db_int_get(db, buf, &mod)) continue;
	     if (mod == -1) any_mod = 1;
	     mods = (Ev_Key_Modifiers)mod;
	     
	     if (button >= 0)
	       {
		  E_Grab *g;
		  
		  g = NEW(E_Grab, 1);
		  ZERO(g, E_Grab, 1);
		  g->button = button;
		  g->mods = mods;
		  g->any_mod = any_mod;
		  g->remove_after = 0;
		  b->grabs = evas_list_append(b->grabs, g);
		  e_button_grab(b->win.main, button, XEV_BUTTON | XEV_MOUSE_MOVE, mods, 0);
	       }
	  }
	e_db_close(db);
     }
}

void
e_border_remove_all_mouse_grabs(void)
{
   Evas_List l;
   
   for (l = borders; l; l = l->next)
      e_border_remove_mouse_grabs((E_Border *)l->data);
}

void
e_border_attach_all_mouse_grabs(void)
{
   Evas_List l;
   
   for (l = borders; l; l = l->next)
      e_border_attach_mouse_grabs((E_Border *)l->data);
}

void
e_border_redo_grabs(void)
{   
   char *grabs_db;
   char *settings_db;
   static time_t mod_date_grabs = 0;
   static time_t mod_date_settings = 0;
   time_t mod;
   int changed = 0;
   Evas_List l;
   
   grabs_db = e_config_get("grabs");
   settings_db = e_config_get("settings");
   mod = e_file_modified_time(grabs_db);
   if (mod != mod_date_grabs) changed = 1;
   mod_date_grabs = mod;
   if (!changed)
     {
	mod = e_file_modified_time(settings_db);
	if (mod != mod_date_settings) changed = 1;
	mod_date_settings = mod;
     }
   if (!changed) return;
   for (l = borders; l; l = l->next)
     {
	E_Border *b;
	
	b = l->data;
	e_border_remove_mouse_grabs(b);
	e_border_attach_mouse_grabs(b);
     }
}

E_Border *
e_border_find_by_window(Window win)
{
   Evas_List l;
   
   for (l = borders; l; l = l->next)
     {
	E_Border *b;
	
	b = l->data;
	
	if ((win == b->win.main) || 
	    (win == b->win.client) ||
	    (win == b->win.container) ||
	    (win == b->win.input) ||
	    (win == b->win.l) ||
	    (win == b->win.r) ||
	    (win == b->win.t) ||
	    (win == b->win.b))
	  return b;
     }
   return NULL;
}

void
e_border_set_bits(E_Border *b, char *file)
{
   int pl, pr, pt, pb, ppl, ppr, ppt, ppb;
      
   pl = pr = pt = pb = 0;
   ppl = ppr = ppt = ppb = 0;
   
   if (b->bits.t) ebits_get_insets(b->bits.t, &pl, &pr, &pt, &pb);
   
   if (b->bits.l) ebits_free(b->bits.l);
   if (b->bits.r) ebits_free(b->bits.r);
   if (b->bits.t) ebits_free(b->bits.t);
   if (b->bits.b) ebits_free(b->bits.b);

   b->bits.l = ebits_load(file);
   b->bits.r = ebits_load(file);
   b->bits.t = ebits_load(file);
   b->bits.b = ebits_load(file);
   b->bits.new = 1;
   b->changed = 1;

   if (b->bits.t) ebits_get_insets(b->bits.t, &ppl, &ppr, &ppt, &ppb);
   b->current.requested.w -= (pl + pr) - (ppl + ppr);
   b->current.requested.h -= (pt + pb) - (ppt + ppb);
   b->current.requested.x += (pl - ppl);
   b->current.requested.y += (pt - ppt);
   
   if (b->bits.t)
     {
	ebits_add_to_evas(b->bits.l, b->evas.l);
	ebits_move(b->bits.l, 0, 0);
	ebits_show(b->bits.l);

	ebits_add_to_evas(b->bits.r, b->evas.r);
	ebits_move(b->bits.r, 0, 0);
	ebits_show(b->bits.r);

	ebits_add_to_evas(b->bits.t, b->evas.t);
	ebits_move(b->bits.t, 0, 0);
	ebits_show(b->bits.t);

	ebits_add_to_evas(b->bits.b, b->evas.b);
	ebits_move(b->bits.b, 0, 0);
	ebits_show(b->bits.b);
	
	e_border_set_color_class(b, "Title BG", 100, 200, 255, 255);

#define HOOK_CB(_class)	\
ebits_set_classed_bit_callback(b->bits.t, _class, CALLBACK_MOUSE_IN, e_cb_mouse_in, b); \
ebits_set_classed_bit_callback(b->bits.t, _class, CALLBACK_MOUSE_OUT, e_cb_mouse_out, b); \
ebits_set_classed_bit_callback(b->bits.t, _class, CALLBACK_MOUSE_DOWN, e_cb_mouse_down, b); \
ebits_set_classed_bit_callback(b->bits.t, _class, CALLBACK_MOUSE_UP, e_cb_mouse_up, b); \
ebits_set_classed_bit_callback(b->bits.t, _class, CALLBACK_MOUSE_MOVE, e_cb_mouse_move, b);
	HOOK_CB("Title_Bar");
	HOOK_CB("Resize");
	HOOK_CB("Resize_Horizontal");
	HOOK_CB("Resize_Vertical");
	HOOK_CB("Close");
	HOOK_CB("Iconify");
	HOOK_CB("Max_Size");
	HOOK_CB("Menu");
	
	e_border_adjust_limits(b);
     }
}

void
e_border_set_color_class(E_Border *b, char *class, int rr, int gg, int bb, int aa)
{
   ebits_set_color_class(b->bits.l, class, rr, gg, bb, aa);
   ebits_set_color_class(b->bits.r, class, rr, gg, bb, aa);
   ebits_set_color_class(b->bits.t, class, rr, gg, bb, aa);
   ebits_set_color_class(b->bits.b, class, rr, gg, bb, aa);
}

void
e_border_adjust_limits(E_Border *b)
{
   int w, h, pl, pr, pt, pb, mx, my;

   if (b->mode.move) 
     {
	e_resist_border(b);
     }
   else
     {
        b->current.x = b->current.requested.x;
	b->current.y = b->current.requested.y;
     }
   
   b->current.w = b->current.requested.w;
   b->current.h = b->current.requested.h - b->current.shaded;
   
   if ((!b->current.shaded) && (!b->mode.move))
     { 
	if (b->current.w < 1) b->current.w = 1;
	if (b->current.h < 1) b->current.h = 1;
   
	pl = pr = pt = pb = 0;
	if (b->bits.t) ebits_get_insets(b->bits.t, &pl, &pr, &pt, &pb);
	
	if (b->current.w < (pl + pr + 1)) b->current.w = pl + pr + 1;
	if (b->current.h < (pt + pb + 1)) b->current.h = pt + pb + 1;
	
	w = b->current.w - pl - pr;
	h = b->current.h - pt - pb + b->current.shaded;

	mx = my = 1;
	if (b->bits.t) ebits_get_min_size(b->bits.t, &mx, &my);	
	if (b->current.w < mx) b->current.w = mx;
	if (b->current.h < my) b->current.h = my;
	mx = my = 999999;
	if (b->bits.t) ebits_get_max_size(b->bits.t, &mx, &my);	
	if (b->current.w > mx) b->current.w = mx;
	if (b->current.h > my) b->current.h = my;
	
	if (w < b->client.min.w) w = b->client.min.w;
	if (h < b->client.min.h) h = b->client.min.h;
	if (w > b->client.max.w) w = b->client.max.w;
	if (h > b->client.max.h) h = b->client.max.h;
	if ((w > 0) && (h > 0))
	  {
	     w -= b->client.base.w;
	     h -= b->client.base.h;
	     if ((w > 0) && (h > 0))
	       {
		  int i, j;
		  double aspect;
		  
		  aspect = ((double)w) / ((double)h);
		  if ((b->mode.resize == 4) || (b->mode.resize == 5))
		    {
		       if (aspect < b->client.min.aspect)
			 h = (int)((double)w / b->client.min.aspect);
		       if (aspect > b->client.max.aspect)
			 h = (int)((double)w / b->client.max.aspect);
		    }
		  else if ((b->mode.resize == 6) || (b->mode.resize == 7))
		    {
		       if (aspect < b->client.min.aspect)
			 w = (int)((double)h * b->client.min.aspect);
		       if (aspect > b->client.max.aspect)
			 w = (int)((double)h * b->client.max.aspect);
		    }
		  else
		    {
		       if (aspect < b->client.min.aspect)
			 w = (int)((double)h * b->client.min.aspect);
		       if (aspect > b->client.max.aspect)
			 h = (int)((double)w / b->client.max.aspect);
		    }
		  i = w / b->client.step.w;
		  j = h / b->client.step.h;
		  w = i * b->client.step.w;
		  h = j * b->client.step.h;
	       }
	     w += b->client.base.w;
	     h += b->client.base.h;
	  }
	b->client.w = w;
	b->client.h = h;
	b->current.w = w + pl + pr;
	b->current.h = h + pt + pb;
     }

   if (b->current.shaded == 0)
     {
	if ((b->mode.resize == 4) || (b->mode.resize == 6) || (b->mode.resize == 8))
	  {
	  }
	else if ((b->mode.resize == 1) || (b->mode.resize == 5))
	  {
	     b->current.x += (b->current.requested.w - b->current.w);
	     b->current.y += (b->current.requested.h - b->current.h);
	  }
	else if ((b->mode.resize == 2) || (b->mode.resize == 7))
	  {
	     b->current.y += (b->current.requested.h - b->current.h);
	  }
	else if ((b->mode.resize == 3))
	  {
	     b->current.x += (b->current.requested.w - b->current.w);
	  }
     }
}

void
e_border_update(E_Border *b)
{
   int location_changed = 0;
   int size_changed = 0;
   int shape_changed = 0;
   int border_changed = 0;
   int visibility_changed = 0;
   int state_changed = 0;
   
   if (b->hold_changes) return;
   if (!b->changed) return;
   
   b->current.visible = b->current.requested.visible;
   
   if ((b->current.x != b->previous.x) || (b->current.y != b->previous.y))
     location_changed = 1;
   if ((b->current.w != b->previous.w) || (b->current.h != b->previous.h))
     size_changed = 1;
   if ((size_changed) && (b->current.has_shape))
     shape_changed = 1;
   if (b->bits.new)
     {
	e_window_gravity_set(b->win.container, StaticGravity);
	border_changed = 1;
     }
   if ((border_changed) && (b->current.has_shape))
     shape_changed = 1;
   if (b->current.visible != b->previous.visible)
     visibility_changed = 1;
   if (b->current.selected != b->previous.selected)
      state_changed = 1;
   
   if (state_changed)
     {
	e_border_apply_border(b);
	if (!border_changed)
	  {
	     e_window_gravity_set(b->win.container, StaticGravity);
	     border_changed = 1;
	     size_changed = 1;
	  }
     }
   if ((location_changed) && (!size_changed))
     {
	int pl, pr, pt, pb;
	
	e_window_move(b->win.main, b->current.x, b->current.y);
	pl = pr = pt = pb = 0;
	if (b->bits.t) ebits_get_insets(b->bits.t, &pl, &pr, &pt, &pb);
	e_icccm_move_resize(b->win.client, 
			    b->current.x + pl, 
			    b->current.y + pt, 
			    b->client.w,
			    b->client.h);
	e_cb_border_move_resize(b);
     }
   else if (size_changed)
     {
	int pl, pr, pt, pb, x, y, w, h;
	int smaller;
	
	if ((b->current.shaped_client) || (b->previous.shaped_client) ||
	    (b->current.shape_changes) || (b->previous.shape_changes) ||
	    (b->current.has_shape) || (b->previous.has_shape))
	  b->shape_changed = 1;
	smaller = 0;
	if ((b->current.w < b->previous.w) || (b->current.h < b->previous.h))
	   smaller = 1;
	pl = pr = pt = pb = 0;
	if (b->bits.t) ebits_get_insets(b->bits.t, &pl, &pr, &pt, &pb);
	e_window_move_resize(b->win.input, 
			     0, 0, b->current.w, b->current.h);
	if (smaller)
	  {
	     if (b->current.shaded == b->client.h)
	       {
		  e_window_move_resize(b->win.client, 
				       0, - b->current.shaded,
				       b->client.w,
				       b->client.h);
		  e_window_move_resize(b->win.container,
				       b->current.w + 1, 
				       b->current.h + 1,
				       320, 
				       320);
	       }
	     else
	       {
		  e_window_move_resize(b->win.client, 
				       0, - b->current.shaded,
				       b->client.w,
				       b->client.h);
		  e_window_move_resize(b->win.container, 
				       pl, 
				       pt,
				       b->current.w - pl - pr, 
				       b->current.h - pt - pb);
	       }
	     e_window_move_resize(b->win.main, 
				  b->current.x, b->current.y,
				  b->current.w, b->current.h);
	     x = 0, y = pt, w = pl, h = (b->current.h - pt - pb);
	     if ((w <1) || (h < 1)) e_window_hide(b->win.l);
	     else 
	       {
		  e_window_show(b->win.l);
		  e_window_move_resize(b->win.l, x, y, w, h);
		  evas_set_output_size(b->evas.l, w, h);
		  evas_set_output_viewport(b->evas.l, x, y, w, h);
	       }
	     
	     x = 0, y = 0, w = b->current.w, h = pt;
	     if ((w <1) || (h < 1)) e_window_hide(b->win.t);
	     else 
	       {
		  e_window_show(b->win.t);
		  e_window_move_resize(b->win.t, x, y, w, h);
		  evas_set_output_size(b->evas.t, w, h);
		  evas_set_output_viewport(b->evas.t, x, y, w, h);
	       }
	     
	     x = b->current.w - pr, y = pt, w = pr, h = (b->current.h - pt - pb);
	     if ((w <1) || (h < 1)) e_window_hide(b->win.r);
	     else 
	       {
		  e_window_show(b->win.r);
		  e_window_move_resize(b->win.r, x, y, w, h);
		  evas_set_output_size(b->evas.r, w, h);
		  evas_set_output_viewport(b->evas.r, x, y, w, h);
	       }
	     
	     x = 0, y = b->current.h - pb, w = b->current.w, h = pb;
	     if ((w <1) || (h < 1)) e_window_hide(b->win.b);
	     else 
	       {
		  e_window_show(b->win.b);
		  e_window_move_resize(b->win.b, x, y, w, h);
		  evas_set_output_size(b->evas.b, w, h);
		  evas_set_output_viewport(b->evas.b, x, y, w, h);
	       }
	  }
	else
	  {
	     e_window_move_resize(b->win.main, 
				  b->current.x, b->current.y,
				  b->current.w, b->current.h);
	     x = 0, y = pt, w = pl, h = (b->current.h - pt - pb);
	     if ((w <1) || (h < 1)) e_window_hide(b->win.l);
	     else 
	       {
		  e_window_show(b->win.l);
		  e_window_move_resize(b->win.l, x, y, w, h);
		  evas_set_output_size(b->evas.l, w, h);
		  evas_set_output_viewport(b->evas.l, x, y, w, h);
	       }
	     
	     x = 0, y = 0, w = b->current.w, h = pt;
	     if ((w <1) || (h < 1)) e_window_hide(b->win.t);
	     else 
	       {
		  e_window_show(b->win.t);
		  e_window_move_resize(b->win.t, x, y, w, h);
		  evas_set_output_size(b->evas.t, w, h);
		  evas_set_output_viewport(b->evas.t, x, y, w, h);
	       }
	     
	     x = b->current.w - pr, y = pt, w = pr, h = (b->current.h - pt - pb);
	     if ((w <1) || (h < 1)) e_window_hide(b->win.r);
	     else 
	       {
		  e_window_show(b->win.r);
		  e_window_move_resize(b->win.r, x, y, w, h);
		  evas_set_output_size(b->evas.r, w, h);
		  evas_set_output_viewport(b->evas.r, x, y, w, h);
	       }
	     
	     x = 0, y = b->current.h - pb, w = b->current.w, h = pb;
	     if ((w <1) || (h < 1)) e_window_hide(b->win.b);
	     else 
	       {
		  e_window_show(b->win.b);
		  e_window_move_resize(b->win.b, x, y, w, h);
		  evas_set_output_size(b->evas.b, w, h);
		  evas_set_output_viewport(b->evas.b, x, y, w, h);
	       }
	     if (b->current.shaded == b->client.h)
	       {
		  e_window_move_resize(b->win.container,
				       b->current.w + 1, 
				       b->current.h + 1,
				       320, 
				       320);
		  e_window_move_resize(b->win.client, 
				       0, - b->current.shaded,
				       b->client.w,
				       b->client.h);
	       }
	     else
	       {
		  e_window_move_resize(b->win.container, 
				       pl, 
				       pt,
				       b->current.w - pl - pr, 
				       b->current.h - pt - pb);
		  e_window_move_resize(b->win.client, 
				       0, - b->current.shaded,
				       b->client.w,
				       b->client.h);
	       }
	  }
	     
	if (b->bits.l) ebits_resize(b->bits.l, b->current.w, b->current.h);
	if (b->bits.r) ebits_resize(b->bits.r, b->current.w, b->current.h);
	if (b->bits.t) ebits_resize(b->bits.t, b->current.w, b->current.h);
	if (b->bits.b) ebits_resize(b->bits.b, b->current.w, b->current.h);

	e_icccm_move_resize(b->win.client, 
			    b->current.x + pl, b->current.y + pt - b->current.shaded, 
			    b->client.w, b->client.h);
	e_cb_border_move_resize(b);
     }
   if ((b->client.title) && (b->bits.t))
     {
	double tx, ty, tw, th;
	
	ebits_get_named_bit_geometry(b->bits.l, "Title_Area", &tx, &ty, &tw, &th);
	evas_set_text(b->evas.l, b->obj.title.l, b->client.title);
	evas_move(b->evas.l, b->obj.title.l, tx, ty);
	evas_move(b->evas.l, b->obj.title_clip.l, tx, ty);
	evas_resize(b->evas.l, b->obj.title_clip.l, tw, th);
	
	ebits_get_named_bit_geometry(b->bits.r, "Title_Area", &tx, &ty, &tw, &th);
	evas_set_text(b->evas.r, b->obj.title.r, b->client.title);
	evas_move(b->evas.r, b->obj.title.r, tx, ty);
	evas_move(b->evas.r, b->obj.title_clip.r, tx, ty);
	evas_resize(b->evas.r, b->obj.title_clip.r, tw, th);
	
	ebits_get_named_bit_geometry(b->bits.t, "Title_Area", &tx, &ty, &tw, &th);
	evas_set_text(b->evas.t, b->obj.title.t, b->client.title);
	evas_move(b->evas.t, b->obj.title.t, tx, ty);
	evas_move(b->evas.t, b->obj.title_clip.t, tx, ty);
	evas_resize(b->evas.t, b->obj.title_clip.t, tw, th);
	
	ebits_get_named_bit_geometry(b->bits.b, "Title_Area", &tx, &ty, &tw, &th);
	evas_set_text(b->evas.b, b->obj.title.b, b->client.title);
	evas_move(b->evas.b, b->obj.title.b, tx, ty);
	evas_move(b->evas.b, b->obj.title_clip.b, tx, ty);
	evas_resize(b->evas.b, b->obj.title_clip.b, tw, th);

	evas_set_layer(b->evas.l, b->obj.title.l, 1);
	evas_set_layer(b->evas.r, b->obj.title.r, 1);
	evas_set_layer(b->evas.t, b->obj.title.t, 1);
	evas_set_layer(b->evas.b, b->obj.title.b, 1);   
     }
   e_border_reshape(b);
   if (visibility_changed)
     {
	if (b->current.visible)
	  e_window_show(b->win.main);
	else
	  e_window_hide(b->win.main);
	e_cb_border_visibility(b);
     }

   if (border_changed)
     e_window_gravity_set(b->win.container, NorthWestGravity);
   b->bits.new = 0;   
   b->previous = b->current;   
   b->changed = 0;
}

void
e_border_set_layer(E_Border *b, int layer)
{
   int dl;
   
   if (b->client.layer == layer) return;
   dl = layer - b->client.layer;
   b->client.layer = layer;
   if (dl > 0) e_border_lower(b);
   else  e_border_raise(b);
}

static void
e_border_raise_delayed(int val, void *b)
{
   int auto_raise = 0;
   E_CFG_INT(cfg_auto_raise, "settings", "/window/raise/auto", 0);
   
   E_CONFIG_INT_GET(cfg_auto_raise, auto_raise);
   if (auto_raise)
     e_border_raise((E_Border *)b);

   return;
   UN(val);
}

void
e_border_raise(E_Border *b)
{
   Evas_List l;
   E_Border *rel;
   
   if (!b->desk->windows)
     {
	b->desk->windows = evas_list_append(b->desk->windows, b);
	b->desk->changed = 1;
	e_window_raise(b->win.main);
	return;
     }
   for (l = b->desk->windows; l; l = l->next)
     {
	rel = l->data;
	if (rel->client.layer > b->client.layer)
	  {
	     b->desk->windows = evas_list_remove(b->desk->windows, b);
	     b->desk->windows = evas_list_prepend_relative(b->desk->windows, b, rel);
	     b->desk->changed = 1;
	     e_window_stack_below(b->win.main, rel->win.main);
	     return;
	  }
	if ((!l->next) && (l->data != b))
	  {
	     b->desk->windows = evas_list_remove(b->desk->windows, b);
	     b->desk->windows = evas_list_append(b->desk->windows, b);
	     b->desk->changed = 1;
	     e_window_raise(b->win.main);
	     return;
	  }
     }
}

void
e_border_lower(E_Border *b)
{
   Evas_List l;
   E_Border *rel;
   
   if (!b->desk->windows)
     {
	b->desk->windows = evas_list_append(b->desk->windows, b);
	b->desk->changed = 1;
	e_window_raise(b->win.main);
	return;
     }
   for (l = b->desk->windows; l; l = l->next)
     {
	rel = l->data;
	if (rel->client.layer == b->client.layer)
	  {
	     if (b == rel) return;
	     b->desk->windows = evas_list_remove(b->desk->windows, b);
	     b->desk->windows = evas_list_prepend_relative(b->desk->windows, b, rel);
	     b->desk->changed = 1;
	     e_window_stack_below(b->win.main, rel->win.main);
	     return;
	  }
     }
}

void
e_border_raise_above(E_Border *b, E_Border *above)
{
   if (!b->desk->windows)
     {
	b->desk->windows = evas_list_append(b->desk->windows, b);
	b->desk->changed = 1;
	e_window_raise(b->win.main);
	return;
     }
   if (!evas_list_find(b->desk->windows, above)) return;
   if (b->client.layer < above->client.layer) b->client.layer = above->client.layer;
   b->desk->windows = evas_list_remove(b->desk->windows, b);
   b->desk->windows = evas_list_append_relative(b->desk->windows, b, above);
   b->desk->changed = 1;
   e_window_stack_above(b->win.main, above->win.main);
}

void
e_border_lower_below(E_Border *b, E_Border *below)
{
   if (!b->desk->windows)
     {
	b->desk->windows = evas_list_append(b->desk->windows, b);
	b->desk->changed = 1;
	return;
     }
   if (!evas_list_find(b->desk->windows, below)) return;
   if (b->client.layer > below->client.layer) b->client.layer = below->client.layer;
   b->desk->windows = evas_list_remove(b->desk->windows, b);
   b->desk->windows = evas_list_prepend_relative(b->desk->windows, b, below);
   b->desk->changed = 1;
   e_window_stack_below(b->win.main, below->win.main);
}

void
e_border_init(void)
{
   double raise_delay = 0.5;
   E_CFG_FLOAT(cfg_raise_delay, "settings", "/window/raise/delay", 0.5);
   
   E_CONFIG_FLOAT_GET(cfg_raise_delay, raise_delay);
   
   e_event_filter_handler_add(EV_MOUSE_DOWN,               e_mouse_down);
   e_event_filter_handler_add(EV_MOUSE_UP,                 e_mouse_up);
   e_event_filter_handler_add(EV_MOUSE_MOVE,               e_mouse_move);
   e_event_filter_handler_add(EV_MOUSE_IN,                 e_mouse_in);
   e_event_filter_handler_add(EV_MOUSE_OUT,                e_mouse_out);
   e_event_filter_handler_add(EV_WINDOW_EXPOSE,            e_window_expose);
   e_event_filter_handler_add(EV_WINDOW_MAP_REQUEST,       e_map_request);
   e_event_filter_handler_add(EV_WINDOW_CONFIGURE_REQUEST, e_configure_request);
   e_event_filter_handler_add(EV_WINDOW_PROPERTY,          e_property);
   e_event_filter_handler_add(EV_WINDOW_UNMAP,             e_unmap);
   e_event_filter_handler_add(EV_WINDOW_DESTROY,           e_destroy);
   e_event_filter_handler_add(EV_WINDOW_CIRCULATE_REQUEST, e_circulate_request);
   e_event_filter_handler_add(EV_WINDOW_REPARENT,          e_reparent);
   e_event_filter_handler_add(EV_WINDOW_SHAPE,             e_shape);
   e_event_filter_handler_add(EV_WINDOW_FOCUS_IN,          e_focus_in);
   e_event_filter_handler_add(EV_WINDOW_FOCUS_OUT,         e_focus_out);
   e_event_filter_handler_add(EV_MESSAGE,                  e_client_message);
   e_event_filter_handler_add(EV_COLORMAP,                 e_colormap);
   e_event_filter_idle_handler_add(e_idle, NULL);

   delayed_window_raise = NEW(E_Delayed_Action, 1);
   E_DELAYED_ACT_INIT(delayed_window_raise, EV_WINDOW_FOCUS_IN, raise_delay, e_border_raise_delayed);
   
   e_add_event_timer("e_border_poll()", 1.00, e_border_poll, 0, NULL);
}

void
e_border_adopt_children(Window win)
{
   Window *wins;
   int i, num;

   wins = e_window_get_children(win, &num);
   if (wins)
     {
	for (i = 0; i < num; i++)
	  {
	     if (e_window_is_manageable(wins[i])) 
	       {
		  E_Border *b;
		  
		  b = e_border_adopt(wins[i], 1);
		    {
		       int pl, pr, pt, pb;
		       
		       pl = pr = pt = pb = 0;
		       if (b->bits.l) ebits_get_insets(b->bits.l, &pl, &pr, &pt, &pb);
		       b->current.requested.x -= pl;
		       b->current.requested.y -= pt;
		       b->changed = 1;
		       e_border_adjust_limits(b);
		    }
		  b->ignore_unmap = 2;
	       }
	  }
	free(wins);
     }
}

E_Border *
e_border_current_focused(void)
{
   Evas_List l;
   
   for (l = borders; l; l = l->next)
     {
	E_Border *b;
	
	b = l->data;
	if (b->current.selected) return b;
     }
   for (l = borders; l; l = l->next)
     {
	E_Border *b;
	
	b = l->data;
	if (b->current.select_lost_from_grab) return b;
     }
   return NULL;
}

void
e_border_focus_grab_ended(void)
{
   Evas_List l;
   for (l = borders; l; l = l->next)
     {
	E_Border *b;
	
	b = l->data;
	b->current.select_lost_from_grab = 0;
     }
}

int
e_border_viewable(E_Border *b)
{
   if (b->desk != e_desktops_get(0))
     return 0;

   if (b->current.x + b->current.w <= 0)
     return 0;

   if (b->current.x >= b->desk->real.w)
     return 0;

   if (b->current.y + b->current.h <= 0)
     return 0;

   if (b->current.y >= b->desk->real.h)
     return 0;
   
   if (!b->current.visible)
     return 0;

   return 1;
}

void
e_border_send_pointer(E_Border *b)
{
   XWarpPointer(e_display_get(), None, b->win.main, 0, 0, 0, 0, b->current.w / 2, b->current.h / 2);
}

void
e_border_raise_next(void)
{
   Evas_List next;
   E_Border *current;

   if (!borders)
	return;

   current = e_border_current_focused();

   /* Find the current border on the list of borders */
   for (next = borders; next && next->data != current; next = next->next);

   /* Step to the next border, wrap around the queue if the end is reached */
   if (next && next->next)
	next = next->next;
   else
	next = borders;

   /* Now find the next viewable border on the same desktop */
   current = (E_Border *)next->data;
   while (next && !e_border_viewable(current))
     {
	next = next->next;
	if (!next)
	  next = borders;

	current = (E_Border *)next->data;
     }

   printf("current desk coords %d, %d, real dim %d, %d\n", current->desk->x,
		   current->desk->y, current->desk->real.w, current->desk->real.h);
   printf("current coords %d, %d\n", current->current.x,
		   current->current.y);

   e_border_raise(current);
   e_border_send_pointer(current);
}

void
e_border_print_pos(char *buf, E_Border *b)
{
   sprintf(buf, "%i, %i",
	   b->current.x, b->current.y);
}

void
e_border_print_size(char *buf, E_Border *b)
{
   if ((b->client.step.w > 1) || (b->client.step.h > 1))
     {
	sprintf(buf, "%i x %i",
		(b->client.w - b->client.base.w) / b->client.step.w,
		(b->client.h - b->client.base.h) / b->client.step.h);
     }
   else
     {
	sprintf(buf, "%i x %i", 
		b->client.w, 
		b->client.h);
     }
}


void      
e_border_set_gravity(E_Border *b, int gravity)
{
  if (!b)
    return;

  e_window_gravity_set(b->win.container, gravity);
  e_window_gravity_set(b->win.input, gravity);
  e_window_gravity_set(b->win.l, gravity);
  e_window_gravity_set(b->win.r, gravity);
  e_window_gravity_set(b->win.t, gravity);
  e_window_gravity_set(b->win.b, gravity);
}

