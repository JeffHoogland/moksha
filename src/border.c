#include "cursors.h"
#include "border.h"
#include "bordermenu.h"
#include "config.h"
#include "debug.h"
#include "actions.h"
#include "delayed.h"
#include "desktops.h"
#include "resist.h"
#include "icccm.h"
#include "file.h"
#include "util.h"
#include "place.h"
#include "match.h"
#include "focus.h"
#include "exec.h"
#include "menu.h"

/* Window border rendering, querying, setting  & modification code */

/* globals local to window borders */
static Evas_List *    evases = NULL;
static Evas_List *    borders = NULL;

static int          mouse_x, mouse_y, mouse_win_x, mouse_win_y;
static int          mouse_buttons = 0;

static int          border_mouse_x = 0;
static int          border_mouse_y = 0;
static int          border_mouse_buttons = 0;

static Ecore_Event *current_ev = NULL;

/* Global delayed window raise action */
E_Delayed_Action   *delayed_window_raise = NULL;

static void         e_idle(void *data);
static void         e_map_request(Ecore_Event * ev);
static void         e_configure_request(Ecore_Event * ev);
static void         e_property(Ecore_Event * ev);
static void         e_unmap(Ecore_Event * ev);
static void         e_destroy(Ecore_Event * ev);
static void         e_circulate_request(Ecore_Event * ev);
static void         e_reparent(Ecore_Event * ev);
static void         e_shape(Ecore_Event * ev);
static void         e_focus_in(Ecore_Event * ev);
static void         e_focus_out(Ecore_Event * ev);
static void         e_colormap(Ecore_Event * ev);
static void         e_mouse_down(Ecore_Event * ev);
static void         e_mouse_up(Ecore_Event * ev);
static void         e_mouse_in(Ecore_Event * ev);
static void         e_mouse_out(Ecore_Event * ev);
static void         e_window_expose(Ecore_Event * ev);
float               e_border_delayed_val();

static void         e_cb_mouse_in(void *data, Ebits_Object o, char *class,
				  int bt, int x, int y, int ox, int oy, int ow,
				  int oh);
static void         e_cb_mouse_out(void *data, Ebits_Object o, char *class,
				   int bt, int x, int y, int ox, int oy, int ow,
				   int oh);
static void         e_cb_mouse_down(void *data, Ebits_Object o, char *class,
				    int bt, int x, int y, int ox, int oy,
				    int ow, int oh);
static void         e_cb_mouse_up(void *data, Ebits_Object o, char *class,
				  int bt, int x, int y, int ox, int oy, int ow,
				  int oh);
static void         e_cb_mouse_move(void *data, Ebits_Object o, char *class,
				    int bt, int x, int y, int ox, int oy,
				    int ow, int oh);

static void         e_cb_border_mouse_in(E_Border * b, Ecore_Event * e);
static void         e_cb_border_mouse_out(E_Border * b, Ecore_Event * e);
static void         e_cb_border_mouse_down(E_Border * b, Ecore_Event * e);
static void         e_cb_border_mouse_up(E_Border * b, Ecore_Event * e);
static void         e_cb_border_mouse_move(E_Border * b, Ecore_Event * e);
static void         e_cb_border_move_resize(E_Border * b);
static void         e_cb_border_visibility(E_Border * b);

static void         e_border_poll(int val, void *data);
static void         e_border_cleanup(E_Border * b);
E_Border           *e_border_shuffle_last(E_Border *b);
E_Border           *e_border_current_select(void);

static int
e_border_replay_query(Ecore_Event_Mouse_Down * ev)
{
   E_Border           *b;

   D_ENTER;

   b = e_border_find_by_window(ev->win);
   if (b)
     {
	int                 focus_mode;

	focus_mode = config_data->window->focus_mode;
	if ((focus_mode == 2) && (ev->mods == ECORE_EVENT_KEY_MODIFIER_NONE))
	   /* FIXME: also if pass click always set */
	   D_RETURN_(1);
     }

   D_RETURN_(0);
}

/* what to dowhen we're idle */

void
e_border_update_borders(void)
{
   Evas_List *           l;

   D_ENTER;

   for (l = borders; l; l = l->next)
     {
	E_Border           *b;

	b = l->data;
	e_border_update(b);
     }
   for (l = borders; l; l = l->next)
     {
	E_Border           *b;

	b = l->data;

	if (b->shape_changed)
	  {
	     e_border_reshape(b);
	  }
     }

   e_db_runtime_flush();

   D_RETURN;
}

void
e_border_check_select( void )
{
   E_Border             *b;
   E_Desktop            *current_desk;

   current_desk = e_desktops_get(e_desktops_get_current());

   /* If no borders exist on present desktop */
   if (!current_desk || !current_desk->windows)
     {
       e_icccm_send_focus_to( e_desktop_window(), 1);
       D_RETURN_(NULL);
     }

   if((b = e_border_current_focused()))
       e_icccm_send_focus_to( b->win.client, 1);
   else
       e_border_shuffle_last(b);
}

static void
e_idle(void *data)
{
   D_ENTER;

   e_border_update_borders();

   D_RETURN;
   UN(data);
}

/* */
static void
e_map_request(Ecore_Event * ev)
{
   Ecore_Event_Window_Map_Request *e;

   D_ENTER;

   current_ev = ev;
   e = ev->event;
   {
      E_Border           *b;

      b = e_border_find_by_window(e->win);
      if (!b)
	{
	   if (ecore_window_exists(e->win))
	      b = e_border_adopt(e->win, 0);
	}
   }
   current_ev = NULL;

   D_RETURN;
}

/* */
static void
e_configure_request(Ecore_Event * ev)
{
   Ecore_Event_Window_Configure_Request *e;

   D_ENTER;

   current_ev = ev;
   e = ev->event;
   {
      E_Border           *b;

      b = e_border_find_by_window(e->win);
      if (b)
	{
	   int                 pl, pr, pt, pb;

	   pl = pr = pt = pb = 0;
	   if (b->bits.b)
	      ebits_get_insets(b->bits.b, &pl, &pr, &pt, &pb);
	   if (e->mask & ECORE_EVENT_VALUE_X)
	      b->current.requested.x = e->x;
	   if (e->mask & ECORE_EVENT_VALUE_Y)
	      b->current.requested.y = e->y;
	   if ((e->mask & ECORE_EVENT_VALUE_W)
	       || (e->mask & ECORE_EVENT_VALUE_H))
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
		b->current.requested.w = e->w + pl + pr;
		b->current.requested.h = e->h + pt + pb;
	     }
	   if ((e->mask & ECORE_EVENT_VALUE_SIBLING)
	       && (e->mask & ECORE_EVENT_VALUE_STACKING))
	     {
		E_Border           *b_rel;

		b_rel = e_border_find_by_window(e->stack_win);
		if (b_rel)
		  {
		     if (e->detail == ECORE_EVENT_STACK_ABOVE)
			e_border_raise_above(b, b_rel);
		     else if (e->detail == ECORE_EVENT_STACK_BELOW)
			e_border_lower_below(b, b_rel);
		     /* FIXME: need to handle  & fix
		      * ECORE_EVENT_STACK_TOP_IF 
		      * ECORE_EVENT_STACK_BOTTOM_IF 
		      * ECORE_EVENT_STACK_OPPOSITE 
		      */
		     else if (e->detail == ECORE_EVENT_STACK_TOP_IF)
			e_border_raise(b);
		     else if (e->detail == ECORE_EVENT_STACK_BOTTOM_IF)
			e_border_lower(b);
		  }
	     }
	   else if (e->mask & ECORE_EVENT_VALUE_STACKING)
	     {
		if (e->detail == ECORE_EVENT_STACK_ABOVE)
		   e_border_raise(b);
		else if (e->detail == ECORE_EVENT_STACK_BELOW)
		   e_border_lower(b);
		/* FIXME: need to handle  & fix
		 * ECORE_EVENT_STACK_TOP_IF 
		 * ECORE_EVENT_STACK_BOTTOM_IF 
		 * ECORE_EVENT_STACK_OPPOSITE 
		 */
		else if (e->detail == ECORE_EVENT_STACK_TOP_IF)
		   e_border_raise(b);
		else if (e->detail == ECORE_EVENT_STACK_BOTTOM_IF)
		   e_border_lower(b);
	     }
	   b->changed = 1;
	   e_border_adjust_limits(b);
	}
      else
	{
	   if ((e->mask & ECORE_EVENT_VALUE_X)
	       && (e->mask & ECORE_EVENT_VALUE_W))
	      ecore_window_move_resize(e->win, e->x, e->y, e->w, e->h);
	   else if ((e->mask & ECORE_EVENT_VALUE_W)
		    || (e->mask & ECORE_EVENT_VALUE_H))
	      ecore_window_resize(e->win, e->w, e->h);
	   else if ((e->mask & ECORE_EVENT_VALUE_X))
	      ecore_window_move(e->win, e->x, e->y);
	}
   }
   current_ev = NULL;

   D_RETURN;
}

/* */
static void
e_property(Ecore_Event * ev)
{
   Ecore_Event_Window_Property *e;

   D_ENTER;

   current_ev = ev;
   e = ev->event;
   {
      E_Border           *b;

      b = e_border_find_by_window(e->win);
      if (b)
	{
	   e_icccm_handle_property_change(e->atom, b);
	   e_border_apply_border(b);
	}
   }
   current_ev = NULL;

   D_RETURN;
}

/* */
static void
e_client_message(Ecore_Event * ev)
{
   Ecore_Event_Message *e;

   D_ENTER;

   current_ev = ev;
   e = ev->event;

   e_icccm_handle_client_message(e);

   current_ev = NULL;

   D_RETURN;
}

/* */
static void
e_unmap(Ecore_Event * ev)
{
   Ecore_Event_Window_Unmap *e;

   D_ENTER;

   current_ev = ev;
   e = ev->event;
   {
      E_Border           *b;

      b = e_border_find_by_window(e->win);
      if (b)
	{
	   if (b->win.client == e->win)
	     {
		if (b->ignore_unmap > 0)
		   b->ignore_unmap--;
		else
		  {
		     e_action_stop_by_object(E_OBJECT(b), NULL,
					     mouse_win_x, mouse_win_y,
					     border_mouse_x, border_mouse_y);

		     if (e_object_get_usecount(E_OBJECT(b)) == 1)
		       {
			 e_border_release(b);
			 e_border_shuffle_last(b);
		       }
		     e_object_unref(E_OBJECT(b));
		  }
	     }
	}
   }
   current_ev = NULL;

   D_RETURN;
}

/* */
static void
e_destroy(Ecore_Event * ev)
{
   Ecore_Event_Window_Destroy *e;

   D_ENTER;

   current_ev = ev;
   e = ev->event;
   {
      E_Border           *b;

      b = e_border_find_by_window(e->win);
      if (b)
	{
	   if (b->win.client == e->win)
	     {
		e_action_stop_by_object(E_OBJECT(b), NULL,
					mouse_win_x, mouse_win_y,
					border_mouse_x, border_mouse_y);

		if (e_object_get_usecount(E_OBJECT(b)) == 1)
		  {
		   e_border_release(b);
		   e_border_shuffle_last(b);
		  }
		e_object_unref(E_OBJECT(b));
	     }
	}
   }
   current_ev = NULL;

   D_RETURN;
}

/* */
static void
e_circulate_request(Ecore_Event * ev)
{
   Ecore_Event_Window_Circulate_Request *e;

   D_ENTER;

   current_ev = ev;
   e = ev->event;
   {
      E_Border           *b;

      b = e_border_find_by_window(e->win);
      if (b)
	{
	   if (e->lower)
	      e_border_lower(b);
	   else
	      e_border_raise(b);
	}
   }
   current_ev = NULL;

   D_RETURN;
}

/* */
static void
e_reparent(Ecore_Event * ev)
{
   Ecore_Event_Window_Reparent *e;

   D_ENTER;

   current_ev = ev;
   e = ev->event;
   current_ev = NULL;

   D_RETURN;
}

/* */
static void
e_shape(Ecore_Event * ev)
{
   Ecore_Event_Window_Shape *e;

   D_ENTER;

   current_ev = ev;
   e = ev->event;
   {
      E_Border           *b;

      b = e_border_find_by_window(e->win);
      if ((b) && (e->win == b->win.client))
	{
	   b->current.shaped_client = e_icccm_is_shaped(e->win);
	   b->changed = 1;
	   b->shape_changed = 1;
	}
   }
   current_ev = NULL;

   D_RETURN;
}

/* */
static void
e_focus_in(Ecore_Event * ev)
{
   Ecore_Event_Window_Focus_In *e;

   D_ENTER;

   current_ev = ev;
   e = ev->event;
   {
      E_Border           *b;

      b = e_border_find_by_window(e->win);
      if ((b) && (b->win.client == e->win))
	{
	   E_Grab             *g;

	   e_border_focus_grab_ended();
	   b->current.selected = 1;
	   b->changed = 1;
	   e_observee_notify_observers(E_OBSERVEE(b), E_EVENT_BORDER_FOCUS_IN,
				       NULL);
	   g = b->click_grab;
	   if (g)
	     {
		ecore_button_ungrab(b->win.container, g->button, g->mods,
				    g->any_mod);
		FREE(g);
		b->click_grab = NULL;
	     }
	}
   }
   current_ev = NULL;

   D_RETURN;
}

/* */
static void
e_focus_out(Ecore_Event * ev)
{
   Ecore_Event_Window_Focus_Out *e;

   D_ENTER;

   current_ev = ev;
   e = ev->event;
   {
      E_Border           *b;

      b = e_border_find_by_window(e->win);
      if ((b) && (b->win.client == e->win))
	{
	   int                 focus_mode;

	   focus_mode = config_data->window->focus_mode;
	   b->current.selected = 0;
	   if (e->key_grab)
	      b->current.select_lost_from_grab = 1;
	   /* settings - click to focus would affect grabs */
	   if ((!b->client.internal) && (focus_mode == 2))
	     {
		E_Grab             *g;

		g = NEW(E_Grab, 1);
		ZERO(g, E_Grab, 1);
		g->button = 1;
		g->mods = ECORE_EVENT_KEY_MODIFIER_NONE;
		g->any_mod = 0;
		g->remove_after = 0;
		ecore_button_grab(b->win.container, g->button, XEV_BUTTON_PRESS,
				  g->mods, g->any_mod);
		ecore_window_button_grab_auto_replay_set(b->win.container,
							 e_border_replay_query);
		b->click_grab = g;
	     }
	   b->changed = 1;
	}
      e_delayed_action_cancel(delayed_window_raise);
   }
   current_ev = NULL;

   D_RETURN;
}

/* */
static void
e_colormap(Ecore_Event * ev)
{
   Ecore_Event_Colormap *e;

   D_ENTER;

   current_ev = ev;
   e = ev->event;
   {
      E_Border           *b;

      b = e_border_find_by_window(e->win);
      if (b)
	{
	}
   }
   current_ev = NULL;

   D_RETURN;
}

/* handling mouse down events */
static void
e_mouse_down(Ecore_Event * ev)
{
   Ecore_Event_Mouse_Down *e;

   D_ENTER;
   current_ev = ev;
   e = ev->event;
   {
      E_Border           *b;

      mouse_win_x = e->x;
      mouse_win_y = e->y;
      mouse_x = e->rx;
      mouse_y = e->ry;
      mouse_buttons |= (1 << e->button);
      b = e_border_find_by_window(e->win);
      if (b)
	{
	   int                 focus_mode;

	   focus_mode = config_data->window->focus_mode;
	   if (focus_mode == 2)
	     {
		e_focus_set_focus(b);
		/* FIXME: if (raise on click to focus) ... */
		e_border_raise(b);
	     }
	   if (e->win == b->win.main)
	      e_cb_border_mouse_down(b, ev);
	   else
	     {
		Evas               *evas;
		int                 x, y;

		evas = b->evas;
		ecore_window_get_root_relative_location(b->win.b, &x, &y);
		x = e->rx - x;
		y = e->ry - y;
		evas_event_feed_mouse_down(evas, e->button);
	     }
	}
   }
   current_ev = NULL;

   D_RETURN;
}

/* handling mouse up events */
static void
e_mouse_up(Ecore_Event * ev)
{
   Ecore_Event_Mouse_Up *e;

   D_ENTER;

   current_ev = ev;
   e = ev->event;
   {
      E_Border           *b;

      mouse_win_x = e->x;
      mouse_win_y = e->y;
      mouse_x = e->rx;
      mouse_y = e->ry;
      mouse_buttons &= ~(1 << e->button);
      b = e_border_find_by_window(e->win);
      if (b)
	{
	   if (e->win == b->win.main)
	      e_cb_border_mouse_up(b, ev);
	   else
	     {
		Evas               *evas;
		int                 x, y;

		evas = b->evas;
		ecore_window_get_root_relative_location(b->win.b, &x, &y);
		x = e->rx - x;
		y = e->ry - y;
		evas_event_feed_mouse_up(evas, e->button);
	     }
	}
   }
   current_ev = NULL;

   D_RETURN;
}

/* handling mouse move events */
static void
e_mouse_move(Ecore_Event * ev)
{
   Ecore_Event_Mouse_Move *e;

   D_ENTER;

   current_ev = ev;
   e = ev->event;
   {
      E_Border           *b;

      mouse_win_x = e->x;
      mouse_win_y = e->y;
      mouse_x = e->rx;
      mouse_y = e->ry;
      b = e_border_find_by_window(e->win);
/*	D("motion... %3.8f\n", ecore_get_time());*/
      if (b)
	{
	   if (e->win == b->win.main)
	      e_cb_border_mouse_move(b, ev);
	   else
	     {
		Evas               *evas;
		int                 x, y;

		evas = b->evas;
		ecore_window_get_root_relative_location(b->win.b, &x, &y);
		x = e->rx - x;
		y = e->ry - y;
		evas_event_feed_mouse_move(evas, x, y);
	     }
	}
   }
   current_ev = NULL;

   D_RETURN;
}

/* handling mouse enter events */
static void
e_mouse_in(Ecore_Event * ev)
{
   Ecore_Event_Window_Enter *e;
   E_Border           *b;

   D_ENTER;
   current_ev = ev;
   e = ev->event;
   if ((b = e_border_find_by_window(e->win)))
     {
	if (e->win == b->win.main)
	   e_cb_border_mouse_in(b, ev);
	else if (e->win == b->win.input)
	  {
	     int                 x, y;
	     Evas               *evas;

	     evas = b->evas;
	     ecore_window_get_root_relative_location(b->win.b, &x, &y);
	     x = e->rx - x;
	     y = e->ry - y;
	     evas_event_feed_mouse_in(evas);
	     evas_event_feed_mouse_move(evas, x, y);
	  }
     }
   current_ev = NULL;
   D_RETURN;
}

/* handling mouse leave events */
static void
e_mouse_out(Ecore_Event * ev)
{
   Ecore_Event_Window_Leave *e;

   D_ENTER;

   current_ev = ev;
   e = ev->event;
   {
      E_Border           *b;

      b = e_border_find_by_window(e->win);
      if (b)
	{
	   if (e->win == b->win.main)
	      e_cb_border_mouse_out(b, ev);
	   if (e->win == b->win.input)
	     {
		evas_event_feed_mouse_out(b->evas);
	     }
	}
   }
   current_ev = NULL;

   D_RETURN;
}

/* handling expose events */
static void
e_window_expose(Ecore_Event * ev)
{
   Ecore_Event_Window_Expose *e;

   D_ENTER;

   current_ev = ev;
   e = ev->event;
   {
      E_Border           *b;

      b = e_border_find_by_window(e->win);
      if (b)
	{
	   e_border_redraw_region(b, e->x, e->y, e->w, e->h);
	}
   }
   current_ev = NULL;

   D_RETURN;
}

/* what to do with border events */

static void
e_cb_mouse_in(void *data, Ebits_Object o, char *class,
	      int bt, int x, int y, int ox, int oy, int ow, int oh)
{
   E_Border           *b;

   D_ENTER;

   b = data;
   if (border_mouse_buttons)
      D_RETURN;
   border_mouse_x = mouse_x;
   border_mouse_y = mouse_y;
   if (class)
      e_cursors_display_in_window(b->win.main, class);
   else
      e_cursors_display_in_window(b->win.main, "Default");
   if (!current_ev)
      D_RETURN;

   e_action_stop(class, ACT_MOUSE_IN, 0, NULL, ECORE_EVENT_KEY_MODIFIER_NONE,
		 E_OBJECT(b), NULL, x, y, border_mouse_x, border_mouse_y);
   e_action_start(class, ACT_MOUSE_IN, 0, NULL, ECORE_EVENT_KEY_MODIFIER_NONE,
		  E_OBJECT(b), NULL, x, y, border_mouse_x, border_mouse_y);

   D_RETURN;
   UN(o);
   UN(bt);
   UN(ox);
   UN(oy);
   UN(ow);
   UN(oh);
}

static void
e_cb_mouse_out(void *data, Ebits_Object o, char *class,
	       int bt, int x, int y, int ox, int oy, int ow, int oh)
{
   E_Border           *b;

   D_ENTER;

   b = data;
   if (border_mouse_buttons)
      D_RETURN;
   border_mouse_x = mouse_x;
   border_mouse_y = mouse_y;
   if (!current_ev)
      D_RETURN;
   e_cursors_display_in_window(b->win.main, "Default");
   e_action_stop(class, ACT_MOUSE_OUT, 0, NULL, ECORE_EVENT_KEY_MODIFIER_NONE,
		 E_OBJECT(b), NULL, x, y, border_mouse_x, border_mouse_y);
   e_action_start(class, ACT_MOUSE_OUT, 0, NULL, ECORE_EVENT_KEY_MODIFIER_NONE,
		  E_OBJECT(b), NULL, x, y, border_mouse_x, border_mouse_y);
   D_RETURN;
   UN(o);
   UN(bt);
   UN(ox);
   UN(oy);
   UN(ow);
   UN(oh);
}

static void
e_cb_mouse_down(void *data, Ebits_Object o, char *class,
		int bt, int x, int y, int ox, int oy, int ow, int oh)
{
   E_Border           *b;

   D_ENTER;

   b = data;
   border_mouse_x = mouse_x;
   border_mouse_y = mouse_y;
   border_mouse_buttons = mouse_buttons;
   if (!current_ev)
      D_RETURN;
   {
      E_Action_Type       act;
      Ecore_Event_Key_Modifiers mods;

      mods = ((Ecore_Event_Mouse_Down *) (current_ev->event))->mods;
      act = ACT_MOUSE_CLICK;

      if (((Ecore_Event_Mouse_Down *) (current_ev->event))->double_click)
	 act = ACT_MOUSE_DOUBLE;
      else if (((Ecore_Event_Mouse_Down *) (current_ev->event))->triple_click)
	 act = ACT_MOUSE_TRIPLE;

      e_action_stop(class, act, bt, NULL, mods, E_OBJECT(b),
		    NULL, x, y, border_mouse_x, border_mouse_y);
      e_action_start(class, act, bt, NULL, mods, E_OBJECT(b),
		     NULL, x, y, border_mouse_x, border_mouse_y);
   }
   D_RETURN;
   UN(o);
   UN(ox);
   UN(oy);
   UN(ow);
   UN(oh);
}

static void
e_cb_mouse_up(void *data, Ebits_Object o, char *class,
	      int bt, int x, int y, int ox, int oy, int ow, int oh)
{
   E_Border           *b;

   D_ENTER;

   b = data;
   border_mouse_x = mouse_x;
   border_mouse_y = mouse_y;
   border_mouse_buttons = mouse_buttons;
   if (!current_ev)
      D_RETURN;
   {
      E_Action_Type       act;
      Ecore_Event_Key_Modifiers mods;

      mods = ((Ecore_Event_Mouse_Up *) (current_ev->event))->mods;
      act = ACT_MOUSE_UP;

      if ((x >= ox) && (x < (ox + ow)) && (y >= oy) && (y < (oy + oh)))
	 act = ACT_MOUSE_CLICKED;

      e_action_stop(class, act, bt, NULL, mods, E_OBJECT(b),
		    NULL, x, y, border_mouse_x, border_mouse_y);
      e_action_start(class, act, bt, NULL, mods, E_OBJECT(b),
		     NULL, x, y, border_mouse_x, border_mouse_y);
   }
   D_RETURN;
   UN(o);
}

static void
e_cb_mouse_move(void *data, Ebits_Object o, char *class,
		int bt, int x, int y, int ox, int oy, int ow, int oh)
{
   E_Border           *b;
   int                 dx, dy;

   D_ENTER;

   b = data;
   dx = mouse_x - border_mouse_x;
   dy = mouse_y - border_mouse_y;
   border_mouse_x = mouse_x;
   border_mouse_y = mouse_y;

   if (!current_ev)
      D_RETURN;

   e_action_cont(class, ACT_MOUSE_MOVE, 0, NULL, ECORE_EVENT_KEY_MODIFIER_NONE,
		 E_OBJECT(b), NULL, x, y, border_mouse_x, border_mouse_y, dx,
		 dy);

   D_RETURN;
   UN(o);
   UN(bt);
   UN(ox);
   UN(oy);
   UN(ow);
   UN(oh);
}

/* callbacks for certain things */
static void
e_cb_border_mouse_in(E_Border * b, Ecore_Event * e)
{
   int                 x, y;
   char               *class = "Window_Grab";
   int                 focus_mode;

   D_ENTER;
   focus_mode = config_data->window->focus_mode;
   /* pointer focus stuff */
   if (focus_mode == 0)
      e_focus_set_focus(b);

   border_mouse_x = mouse_x;
   border_mouse_y = mouse_y;
   if (!current_ev)
      D_RETURN;

   x = ((Ecore_Event_Window_Enter *) (e->event))->x;
   y = ((Ecore_Event_Window_Enter *) (e->event))->y;

   e_action_stop(class, ACT_MOUSE_IN, 0, NULL, ECORE_EVENT_KEY_MODIFIER_NONE,
		 E_OBJECT(b), NULL, x, y, border_mouse_x, border_mouse_y);
   e_action_start(class, ACT_MOUSE_IN, 0, NULL, ECORE_EVENT_KEY_MODIFIER_NONE,
		  E_OBJECT(b), NULL, x, y, border_mouse_x, border_mouse_y);

   D_RETURN;
}

static void
e_cb_border_mouse_out(E_Border * b, Ecore_Event * e)
{
   int                 x, y;
   char               *class = "Window_Grab";

   D_ENTER;

   x = mouse_x;
   y = mouse_y;
   border_mouse_x = mouse_x;
   border_mouse_y = mouse_y;
   if (!current_ev)
      D_RETURN;

   e_action_stop(class, ACT_MOUSE_OUT, 0, NULL, ECORE_EVENT_KEY_MODIFIER_NONE,
		 E_OBJECT(b), NULL, x, y, border_mouse_x, border_mouse_y);
   e_action_start(class, ACT_MOUSE_OUT, 0, NULL, ECORE_EVENT_KEY_MODIFIER_NONE,
		  E_OBJECT(b), NULL, x, y, border_mouse_x, border_mouse_y);
   D_RETURN;
   UN(e);
}

static void
e_cb_border_mouse_down(E_Border * b, Ecore_Event * e)
{
   int                 x, y, bt;
   char               *class = "Window_Grab";
   int                 focus_mode;

   D_ENTER;

   focus_mode = config_data->window->focus_mode;
   ecore_pointer_grab(((Ecore_Event_Mouse_Down *) (e->event))->win,
		      CurrentTime);
   border_mouse_x = mouse_x;
   border_mouse_y = mouse_y;
   if (!current_ev)
      D_RETURN;

   x = ((Ecore_Event_Mouse_Down *) (e->event))->x;
   y = ((Ecore_Event_Mouse_Down *) (e->event))->y;
   bt = ((Ecore_Event_Mouse_Down *) (e->event))->button;
   {
      E_Action_Type       act;
      Ecore_Event_Key_Modifiers mods;

      mods = ((Ecore_Event_Mouse_Down *) (current_ev->event))->mods;
      act = ACT_MOUSE_CLICK;

      if (((Ecore_Event_Mouse_Down *) (current_ev->event))->double_click)
	 act = ACT_MOUSE_DOUBLE;
      else if (((Ecore_Event_Mouse_Down *) (current_ev->event))->triple_click)
	 act = ACT_MOUSE_TRIPLE;

      e_action_stop(class, act, bt, NULL, mods, E_OBJECT(b), NULL,
		    x, y, border_mouse_x, border_mouse_y);
      if (!e_action_start(class, act, bt, NULL, mods, E_OBJECT(b), NULL,
			  x, y, border_mouse_x, border_mouse_y))
	{
	   ecore_pointer_ungrab(((Ecore_Event_Mouse_Down *) (e->event))->time);
	}
      else
	{
	   ecore_pointer_grab(((Ecore_Event_Mouse_Down *) (e->event))->win,
			      ((Ecore_Event_Mouse_Down *) (e->event))->time);
	}
   }

   D_RETURN;
}

static void
e_cb_border_mouse_up(E_Border * b, Ecore_Event * e)
{
   int                 x, y, bt;
   char               *class = "Window_Grab";

   D_ENTER;

   border_mouse_x = mouse_x;
   border_mouse_y = mouse_y;

   if (!current_ev)
      D_RETURN;
   ecore_pointer_ungrab(((Ecore_Event_Mouse_Up *) (e->event))->time);

   x = ((Ecore_Event_Mouse_Up *) (e->event))->x;
   y = ((Ecore_Event_Mouse_Up *) (e->event))->y;
   bt = ((Ecore_Event_Mouse_Up *) (e->event))->button;
   {
      E_Action_Type       act;
      Ecore_Event_Key_Modifiers mods;

      mods = ((Ecore_Event_Mouse_Up *) (current_ev->event))->mods;
      act = ACT_MOUSE_UP;
      e_action_stop(class, act, bt, NULL, mods, E_OBJECT(b),
		    NULL, x, y, border_mouse_x, border_mouse_y);
      e_action_start(class, act, bt, NULL, mods, E_OBJECT(b),
		     NULL, x, y, border_mouse_x, border_mouse_y);
   }

   D_RETURN;
}

static void
e_cb_border_mouse_move(E_Border * b, Ecore_Event * e)
{
   int                 dx, dy;
   int                 x, y;
   char               *class = "Window_Grab";

   D_ENTER;

   dx = mouse_x - border_mouse_x;
   dy = mouse_y - border_mouse_y;
   border_mouse_x = mouse_x;
   border_mouse_y = mouse_y;
   if (!current_ev)
      D_RETURN;
   x = ((Ecore_Event_Mouse_Move *) (e->event))->x;
   y = ((Ecore_Event_Mouse_Move *) (e->event))->y;
   e_action_cont(class, ACT_MOUSE_MOVE, 0, NULL, ECORE_EVENT_KEY_MODIFIER_NONE,
		 E_OBJECT(b), NULL, x, y, border_mouse_x, border_mouse_y, dx,
		 dy);

   D_RETURN;
}

static void
e_cb_border_move_resize(E_Border * b)
{
   D_ENTER;

   D_RETURN;
   UN(b);
}

static void
e_cb_border_visibility(E_Border * b)
{
   D_ENTER;

   D_RETURN;
   UN(b);
}

static void
e_border_poll(int val, void *data)
{
   D_ENTER;

   ecore_add_event_timer("e_border_poll()", 1.00, e_border_poll, val + 1, NULL);
   D_RETURN;
   UN(data);
}

static void
e_border_cleanup_window_list(Evas_List *windows)
{
   Window temp;

   /* Free the old set of pixmaps */
   while (windows)
     {
	temp = (Window) windows->data;
	windows = evas_list_remove(windows, (void *)temp);
	ecore_window_destroy(temp);
     }
}

static void
e_border_cleanup(E_Border * b)
{
   D_ENTER;

   e_match_save_props(b);
   D("before notify\n");
   e_observee_notify_observers(E_OBSERVEE(b), E_EVENT_BORDER_DELETE, NULL);
   D("after notify\n");
   e_bordermenu_hide();
   e_desktops_del_border(b->desk, b);
   if (b->bits.b)
      ebits_free(b->bits.b);

   if (b->obj.title)
      e_text_free(b->obj.title);

   if (b->obj.title_clip)
      evas_object_del(b->obj.title_clip);

   e_border_cleanup_window_list(b->windows);
   evases = evas_list_remove(evases, b->evas);
   evas_free(b->evas);

   ecore_window_destroy(b->win.b);
   ecore_window_destroy(b->win.container);
   ecore_window_destroy(b->win.input);
   ecore_window_destroy(b->win.main);
   borders = evas_list_remove(borders, b);

   IF_FREE(b->client.title);
   IF_FREE(b->client.name);
   IF_FREE(b->client.class);
   IF_FREE(b->client.command);
   IF_FREE(b->client.machine);
   IF_FREE(b->client.icon_name);
   IF_FREE(b->border_style);
   IF_FREE(b->border_file);

   /* Cleanup superclass. */
   e_observee_cleanup(E_OBSERVEE(b));

   D_RETURN;
}

/* border creation, deletion, modification and general queries */

void
e_border_apply_border(E_Border * b)
{
   int                 pl, pr, pt, pb;
   char               *borders, buf[PATH_MAX], border[PATH_MAX], *style = NULL;
   int                 prop_selected = 0, prop_sticky = 0, prop_shaded = 0;

   D_ENTER;

   style = "default";
   if ((!b->client.titlebar) && (!b->client.border))
     {
	style = "borderless";
	b->current.has_shape = 0;
     }
   if (b->border_style)
      style = b->border_style;

   if (b->current.selected)
      prop_selected = 1;
   if ((b->current.shaded > 0) && (b->current.shaded == b->client.h))
      prop_shaded = 1;
   if (b->client.sticky)
     {
        prop_sticky = 1;
	e_desktops_add_sticky(b);
     }

   snprintf(border, PATH_MAX, "selected-%i.sticky-%i.shaded-%i.bits.db",
	    prop_selected, prop_sticky, prop_shaded);

   borders = e_config_get("borders");
   snprintf(buf, PATH_MAX, "%s%s/%s", borders, style, border);

   /* if it's not changed - abort and dont do anything */
   if ((b->border_file) && (!strcmp(buf, b->border_file)))
      D_RETURN;

   IF_FREE(b->border_file);
   e_strdup(b->border_file, buf);

   e_border_set_bits(b, buf);

   pl = pr = pt = pb = 0;
   if (b->bits.b)
      ebits_get_insets(b->bits.b, &pl, &pr, &pt, &pb);
   e_icccm_set_frame_size(b->win.client, pl, pr, pt, pb);

   D_RETURN;
}

void
e_border_reshape(E_Border * b)
{
   static Window       shape_win = 0;
   int                 pl, pr, pt, pb;

   D_ENTER;

   if ((b->current.shaped_client == b->previous.shaped_client) &&
       (b->current.shape_changes == b->previous.shape_changes) &&
       (b->current.has_shape == b->previous.has_shape) && (!b->shape_changed))
      D_RETURN;

   if (!shape_win)
      shape_win = ecore_window_override_new(0, 0, 0, 1, 1);
   pl = pr = pt = pb = 0;
   if (b->bits.b)
      ebits_get_insets(b->bits.b, &pl, &pr, &pt, &pb);
   b->shape_changed = 0;

   if ((!b->current.shaped_client) && (!b->current.has_shape))
     {
	ecore_window_set_shape_mask(b->win.main, 0);
	D_RETURN;
     }

   ecore_window_resize(shape_win, b->current.w, b->current.h);

   if ((b->current.shaped_client) && (!b->current.has_shape))
     {
	XRectangle          rects[4];

	rects[0].x = 0;
	rects[0].y = 0;
	rects[0].width = b->current.w;
	rects[0].height = pt;

	rects[1].x = 0;
	rects[1].y = pt;
	rects[1].width = pl;
	rects[1].height = b->current.h - pt - pb;

	rects[2].x = b->current.w - pr;
	rects[2].y = pt;
	rects[2].width = pr;
	rects[2].height = b->current.h - pt - pb;

	rects[3].x = 0;
	rects[3].y = b->current.h - pb;
	rects[3].width = b->current.w;
	rects[3].height = pb;

	ecore_window_set_shape_window(shape_win, b->win.client, pl,
				      pt - b->current.shaded);
	ecore_window_clip_shape_by_rectangle(shape_win, pl, pt,
					     b->current.w - pl - pr,
					     b->current.h - pt - pb);

	ecore_window_add_shape_rectangles(shape_win, rects, 4);

     }
   else
     {
	Display            *disp;
	Evas_List          *windows;

	if ((!b->current.shaped_client) && (b->current.has_shape))
	  {
	     ecore_window_set_shape_rectangle(shape_win, pl,
					      pt - b->current.shaded,
					      b->current.w - pl - pr,
					      b->current.h - pt - pb);
	  }
	else
	  {
	     ecore_window_set_shape_window(shape_win, b->win.client, pl,
					   pt - b->current.shaded);
	     ecore_window_clip_shape_by_rectangle(shape_win, pl, pt,
						  b->current.w - pl - pr,
						  b->current.h - pt - pb);
	  }

	D("SHAPE update for border %p bit %s\n", b, b->border_file);

	e_border_update_render(b);

	windows = b->windows;
	disp = ecore_display_get();

	while (windows)
	  {
	     int                 x, y, w, h;
	     Window              window;

	     window = (Window) windows->data;
	     ecore_window_get_geometry(window, &x, &y, &w, &h);
	     ecore_window_add_shape_window(shape_win, window, x, y);

	     windows = windows->next;
	  }

	ecore_window_clear(shape_win);
     }

   ecore_window_set_shape_window(b->win.main, shape_win, 0, 0);

   D_RETURN;
}

void
e_border_release(E_Border * b)
{
   int                 pl, pr, pt, pb;

   D_ENTER;

   pl = pr = pt = pb = 0;
   if (b->bits.b)
      ebits_get_insets(b->bits.b, &pl, &pr, &pt, &pb);
   ecore_window_reparent(b->win.client, 0, b->current.x + pl,
			 b->current.y + pt);
   e_icccm_release(b->win.client);
   e_icccm_send_focus_to( e_desktop_window(), 1);
   
   D_RETURN;
}

E_Border           *
e_border_adopt(Window win, int use_client_pos)
{
   E_Border           *b;
   int                 bw;
   int                 show = 1;

   D_ENTER;

   /* create the struct */
   b = e_border_new();
   /* set the right event on the client */
   ecore_window_set_events(win,
			   XEV_VISIBILITY |
			   ResizeRedirectMask |
			   XEV_CONFIGURE | XEV_MOUSE_MOVE | 
			   XEV_FOCUS | XEV_PROPERTY | XEV_COLORMAP);
   ecore_window_select_shape_events(win);
   /* parent of the client window listens for these */
   ecore_window_set_events(b->win.container,
			   XEV_CHILD_CHANGE | XEV_CHILD_REDIRECT);
   /* add to save set & border of 0 */
   e_icccm_adopt(win);
   bw = ecore_window_get_border_width(win);
   ecore_window_set_border_width(win, 0);
   b->win.client = win;
   b->current.requested.visible = 1;
   /* get hints */
   e_icccm_get_size_info(win, b);
   e_icccm_get_pos_info(win, b);
   {
      int                 x, y, w, h;

      ecore_window_get_geometry(win, &x, &y, &w, &h);
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
   e_icccm_get_e_hack_launch_id(win, b);
   b->current.shaped_client = e_icccm_is_shaped(win);
   /* we have now placed the bugger */
   b->placed = 1;
   /* desk area */
   if (use_client_pos)
     {
	int                 x, y;

	ecore_window_get_geometry(win, &x, &y, NULL, NULL);
	b->current.requested.x = x;
	b->current.requested.y = y;
	b->changed = 1;
     }
   /* reparent the window finally */
   ecore_window_reparent(win, b->win.container, 0, 0);
   e_match_set_props(b);
   e_icccm_set_desk_area(win, b->client.area.x, b->client.area.y);
   e_icccm_set_desk(win, b->client.desk);
   /* figure what border to use */
   e_border_apply_border(b);
   {
      int                 pl, pr, pt, pb;

      pl = pr = pt = pb = 0;
      if (b->bits.b)
	 ebits_get_insets(b->bits.b, &pl, &pr, &pt, &pb);
      b->current.requested.x += pl;
      b->current.requested.y += pt;
      b->changed = 1;
   }
   if (!use_client_pos)
     {
	int                 x, y;
	int                 pl, pr, pt, pb;

	pl = pr = pt = pb = 0;
	x = y = 0;
	if (b->bits.b)
	   ebits_get_insets(b->bits.b, &pl, &pr, &pt, &pb);
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
	     show = e_place_border(b, b->desk, &x, &y,
			           config_data->window->place_mode);
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
   e_border_update(b);
   e_border_reshape(b);
   ecore_window_show(win);

   if (b->client.e.launch_id)
      e_exec_broadcast_e_hack_found(b->win.client);

   D_RETURN_(b);
}

E_Border           *
e_border_new(void)
{
   /* FIXME: need to set an upper limit on the frame size */
   E_Border           *b;
   char               *font_dir;
   E_Desktop          *desk;

   D_ENTER;

   font_dir = e_config_get("fonts");
   b = NEW(E_Border, 1);
   ZERO(b, E_Border, 1);

   e_observee_init(E_OBSERVEE(b), (E_Cleanup_Func) e_border_cleanup);
   e_observer_register_observee(E_OBSERVER(delayed_window_raise),
				E_OBSERVEE(b));

   D("BORDER CREATED AT %p\n", b);

   b->current.has_shape = 1;

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

   desk = e_desktops_get(e_desktops_get_current());
   e_desktops_add_border(desk, b);
   /* b->win.main = ecore_window_override_new(desk->win.main, 0, 0, 1, 1); */
   b->win.main = ecore_window_override_new(0, 0, 0, 1, 1);
   b->win.input = ecore_window_input_new(b->win.main, 0, 0, 1, 1);
   b->win.container = ecore_window_override_new(b->win.main, 0, 0, 1, 1);
   e_cursors_display_in_window(b->win.container, "Application");
   ecore_window_set_events_propagate(b->win.input, 1);
   ecore_window_set_events(b->win.input, XEV_MOUSE_MOVE | XEV_BUTTON);
   ecore_window_set_events(b->win.main, XEV_IN_OUT);
   ecore_window_set_events(b->win.container, XEV_IN_OUT);
   ecore_window_show(b->win.input);
   ecore_window_show(b->win.container);

   b->evas = e_evas_new_all(ecore_display_get(),
			    b->win.main,
			    0, 0, 1, 1, font_dir);
   b->win.b = e_evas_get_window(b->evas);
   e_cursors_display_in_window(b->win.b, "Default");

   b->obj.title = e_text_new(b->evas, "", "title");
   b->obj.title_clip = evas_object_rectangle_add(b->evas);
   evas_object_color_set(b->obj.title_clip, 255, 255, 255, 255);
   e_text_show(b->obj.title);
   evas_object_show(b->obj.title_clip);
   e_text_set_clip(b->obj.title, b->obj.title_clip);

   ecore_window_raise(b->win.input);
   ecore_window_raise(b->win.container);

   evases = evas_list_append(evases, b->evas);

   ecore_window_set_events(b->win.b, XEV_EXPOSE);

   ecore_window_show(b->win.b);

   e_border_attach_mouse_grabs(b);

   /*   e_icccm_send_focus_to(b->win.client, 1);*/

   borders = evas_list_prepend(borders, b);

   e_observee_notify_all_observers(E_OBSERVEE(b), E_EVENT_BORDER_NEW, NULL);

   D_RETURN_(b);
}

void
e_border_iconify(E_Border * b)
{
   D_ENTER;
   b->client.iconified = 1;
   b->current.requested.visible = 0;
   e_icccm_state_iconified(b->win.client);
   b->changed = 1;
   e_border_update(b);
   e_observee_notify_observers(E_OBSERVEE(b), E_EVENT_BORDER_ICONIFY, NULL);

   D_RETURN;
}

void
e_border_uniconify(E_Border * b)
{
   b->client.iconified = 0;
   b->current.requested.visible = 1;
   b->client.desk = e_desktops_get_current();
   e_icccm_state_mapped(b->win.client);
   b->changed = 1;
   e_border_update(b);
   e_border_raise(b);

   e_observee_notify_observers(E_OBSERVEE(b), E_EVENT_BORDER_UNICONIFY, NULL);
}

void
e_border_remove_mouse_grabs(E_Border * b)
{
   Evas_List *           l;

   D_ENTER;

   if (config_data->grabs)
     {
	for (l = config_data->grabs; l; l = l->next)
	  {
	     E_Grab             *g;

	     g = l->data;
	     ecore_button_ungrab(b->win.main, g->button, g->mods, g->any_mod);
	  }
     }
   if (b->click_grab)
     {
	ecore_button_ungrab(b->win.main, b->click_grab->button,
			    b->click_grab->mods, b->click_grab->any_mod);
	FREE(b->click_grab);
     }
   b->click_grab = NULL;

   D_RETURN;
}

void
e_border_remove_click_grab(E_Border * b)
{
   D_ENTER;

   if (b->click_grab)
     {
	E_Grab             *g;

	g = b->click_grab;
	ecore_button_ungrab(b->win.container, g->button, g->mods, g->any_mod);
	ecore_window_button_grab_auto_replay_set(b->win.container, NULL);
	b->click_grab = NULL;
	FREE(g);
     }

   D_RETURN;
}

void
e_border_attach_mouse_grabs(E_Border * b)
{
   int                 focus_mode;
   Evas_List          *l;

   D_ENTER;

   focus_mode = config_data->window->focus_mode;

   /* settings - click to focus would affect grabs */
   if ((!b->current.selected) && (focus_mode == 2))
     {
	E_Grab             *g;

	g = NEW(E_Grab, 1);
	ZERO(g, E_Grab, 1);
	g->button = 1;
	g->mods = ECORE_EVENT_KEY_MODIFIER_NONE;
	g->any_mod = 0;
	g->remove_after = 0;
	ecore_button_grab(b->win.container, g->button, XEV_BUTTON_PRESS,
			  g->mods, g->any_mod);
	ecore_window_button_grab_auto_replay_set(b->win.container,
						 e_border_replay_query);
	b->click_grab = g;
     }

   for (l = config_data->grabs; l; l = l->next)
     {
	E_Grab             *g;

	g = l->data;
        ecore_button_grab(b->win.main, g->button, XEV_BUTTON_PRESS, g->mods, 0);
     }

   D_RETURN;
}

void
e_border_remove_all_mouse_grabs(void)
{
   Evas_List *           l;

   D_ENTER;

   for (l = borders; l; l = l->next)
      e_border_remove_mouse_grabs((E_Border *) l->data);

   D_RETURN;
}

void
e_border_attach_all_mouse_grabs(void)
{
   Evas_List *           l;

   D_ENTER;

   for (l = borders; l; l = l->next)
      e_border_attach_mouse_grabs((E_Border *) l->data);

   D_RETURN;
}

void
e_border_redo_grabs(void)
{
   char               *grabs_db;
   char               *settings_db;
   static time_t       mod_date_grabs = 0;
   static time_t       mod_date_settings = 0;
   time_t              mod;
   int                 changed = 0;
   Evas_List *           l;

   D_ENTER;

   grabs_db = e_config_get("grabs");
   settings_db = e_config_get("settings");
   mod = e_file_mod_time(grabs_db);
   if (mod != mod_date_grabs)
      changed = 1;
   mod_date_grabs = mod;
   if (!changed)
     {
	mod = e_file_mod_time(settings_db);
	if (mod != mod_date_settings)
	   changed = 1;
	mod_date_settings = mod;
     }
   if (!changed)
      D_RETURN;
   for (l = borders; l; l = l->next)
     {
	E_Border           *b;

	b = l->data;
	e_border_remove_mouse_grabs(b);
	e_border_attach_mouse_grabs(b);
     }

   D_RETURN;
}

E_Border           *
e_border_find_by_window(Window win)
{
   Window                pwin;
   Evas_List *           l;

   D_ENTER;

   pwin = ecore_window_get_parent(win);
   for (l = borders; l; l = l->next)
     {
	E_Border           *b;

	b = l->data;

	if ((win == b->win.main) ||
	    (win == b->win.client) ||
	    (win == b->win.container) ||
	    (win == b->win.input) ||
	    (win == b->win.b) ||
	    (pwin == b->win.main))
	   D_RETURN_(b);
     }

   D_RETURN_(NULL);
}

void
e_border_set_bits(E_Border * b, char *file)
{
   int                 pl, pr, pt, pb, ppl, ppr, ppt, ppb;

   D_ENTER;

   pl = pr = pt = pb = 0;
   ppl = ppr = ppt = ppb = 0;

   if (b->bits.b)
      ebits_get_insets(b->bits.b, &pl, &pr, &pt, &pb);

   if (b->bits.b)
      ebits_free(b->bits.b);

   b->bits.b = ebits_load(file);
   b->bits.new = 1;
   b->changed = 1;

   if (b->bits.b)
      ebits_get_insets(b->bits.b, &ppl, &ppr, &ppt, &ppb);
   b->current.requested.w -= (pl + pr) - (ppl + ppr);
   b->current.requested.h -= (pt + pb) - (ppt + ppb);
   b->current.requested.x += (pl - ppl);
   b->current.requested.y += (pt - ppt);

   if (b->bits.b)
     {

	ebits_add_to_evas(b->bits.b, b->evas);
	ebits_move(b->bits.b, 0, 0);
	ebits_show(b->bits.b);

	e_border_set_color_class(b, "Title BG", 100, 200, 255, 255);

#define HOOK_CB(_class)	\
ebits_set_classed_bit_callback(b->bits.b, _class, EVAS_CALLBACK_MOUSE_IN, e_cb_mouse_in, b); \
ebits_set_classed_bit_callback(b->bits.b, _class, EVAS_CALLBACK_MOUSE_OUT, e_cb_mouse_out, b); \
ebits_set_classed_bit_callback(b->bits.b, _class, EVAS_CALLBACK_MOUSE_DOWN, e_cb_mouse_down, b); \
ebits_set_classed_bit_callback(b->bits.b, _class, EVAS_CALLBACK_MOUSE_UP, e_cb_mouse_up, b); \
ebits_set_classed_bit_callback(b->bits.b, _class, EVAS_CALLBACK_MOUSE_MOVE, e_cb_mouse_move, b);
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

   D_RETURN;
}

void
e_border_set_color_class(E_Border * b, char *class, int rr, int gg, int bb,
			 int aa)
{
   D_ENTER;

   ebits_set_color_class(b->bits.b, class, rr, gg, bb, aa);

   D_RETURN;
}

void
e_border_adjust_limits(E_Border * b)
{
   int                 w, h, pl, pr, pt, pb, mx, my;

   D_ENTER;

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
	if (b->current.w < 1)
	   b->current.w = 1;
	if (b->current.h < 1)
	   b->current.h = 1;

	pl = pr = pt = pb = 0;
	if (b->bits.b)
	   ebits_get_insets(b->bits.b, &pl, &pr, &pt, &pb);

	if (b->current.w < (pl + pr + 1))
	   b->current.w = pl + pr + 1;
	if (b->current.h < (pt + pb + 1))
	   b->current.h = pt + pb + 1;

	w = b->current.w - pl - pr;
	h = b->current.h - pt - pb + b->current.shaded;

	mx = my = 1;
	if (b->bits.b)
	   ebits_get_min_size(b->bits.b, &mx, &my);
	if (b->current.w < mx)
	   b->current.w = mx;
	if (b->current.h < my)
	   b->current.h = my;
	mx = my = 999999;
	if (b->bits.b)
	   ebits_get_max_size(b->bits.b, &mx, &my);
	if (b->current.w > mx)
	   b->current.w = mx;
	if (b->current.h > my)
	   b->current.h = my;

	if (w < b->client.min.w)
	   w = b->client.min.w;
	if (h < b->client.min.h)
	   h = b->client.min.h;
	if (w > b->client.max.w)
	   w = b->client.max.w;
	if (h > b->client.max.h)
	   h = b->client.max.h;
	if ((w > 0) && (h > 0))
	  {
	     w -= b->client.base.w;
	     h -= b->client.base.h;
	     if ((w > 0) && (h > 0))
	       {
		  int                 i, j;
		  double              aspect;

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
	if ((b->mode.resize == 4) || (b->mode.resize == 6)
	    || (b->mode.resize == 8))
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

   D_RETURN;
}

void
e_border_redraw_region(E_Border * b, int x, int y, int w, int h)
{
   GC                  gc;
   Evas_List          *windows;

   gc = ecore_gc_new(b->win.b);

   windows = b->windows;

   while (windows)
     {
	int                 xx, yy, ww, hh;
	Window              window;

	window = (Window) windows->data;
	ecore_window_get_geometry(window, &xx, &yy, &ww, &hh);

	if (INTERSECTS(x, y, w, h, xx, yy, ww, hh))
	  {
	     int                 rw, rh;

	     rw = MIN(w, ww);
	     rh = MIN(h, hh);
	     ecore_window_clear_area(window, x, y, rw, rh);
	  }

	windows = windows->next;
     }

   ecore_gc_free(gc);
}

void
e_border_update_render(E_Border * b)
{
   GC                  gc1, gc2;
   Evas_List          *up, *hp, *owin, *nwin = NULL;
   Window              window;
   Pixmap              pmap, mask, temp;
   int                 pl, pr, pt, pb;
   Evas_Engine_Info_Software_X11 *info;

   pmap = ecore_pixmap_new(b->win.b, b->current.w, b->current.h, 0);
   mask = ecore_pixmap_new(b->win.b, b->current.w, b->current.h, 1);

   gc1 = ecore_gc_new(pmap);
   gc2 = ecore_gc_new(mask);

   info = (Evas_Engine_Info_Software_X11 *) evas_engine_info_get(b->evas);
   info->info.drawable = pmap;
   info->info.mask = mask;
   evas_engine_info_set(b->evas, (Evas_Engine_Info *) info);

   /*
    * Hide the bits and render to clear the old appearance from generating
    * damage rectangles.
    */
   if (b->bits.b)
     {
	ebits_hide(b->bits.b);
     }

   if (b->obj.title)
     {
	evas_object_hide(b->obj.title_clip);
     }

   evas_render(b->evas);

   /*
    * Position and then show the bits so we only get damage rectangles for the
    * area we want shown.
    */
   if (b->bits.b)
     {
        ebits_move(b->bits.b, 0, 0);
        ebits_resize(b->bits.b, b->current.w, b->current.h);
	ebits_get_insets(b->bits.b, &pl, &pr, &pt, &pb);
	ebits_show(b->bits.b);
     }

   if (b->obj.title)
     {
	double tx, ty, tw, th;

	ebits_get_named_bit_geometry(b->bits.b, "Title_Area", &tx, &ty, &tw,
				     &th);
        e_text_move(b->obj.title, tx, ty);
	e_text_set_layer(b->obj.title, 1);

	evas_object_move(b->obj.title_clip, tx, ty);
	evas_object_resize(b->obj.title_clip, tw, th);
	evas_object_show(b->obj.title_clip);
     }

   hp = up = evas_render_updates(b->evas);

   owin = b->windows;
   b->windows = NULL;

   D("Rendering %d rectangles for border %p { w = %d, h = %d }\n",
     (up ? up->count : 0), b, b->current.w, b->current.h);
   while (up)
     {
	Evas_Rectangle     *u;

	u = up->data;

	D("\tRectangle { x = %d, y = %d, w = %d, h = %d }\n",
			u->x, u->y, u->w, u->h);

	/* Copy the large pixmap to a series of small pixmaps. */
	temp = ecore_pixmap_new(b->win.b, u->w, u->h, 0);
	ecore_gc_set_fg(gc1, 0);
	ecore_fill_rectangle(temp, gc1, 0, 0, u->w, u->h);
	ecore_gc_set_fg(gc1, 1);
	ecore_area_copy(pmap, temp, gc1, u->x, u->y, u->w, u->h, 0, 0);

	/* Setup small windows for borders, with the pixmaps as backgrounds */
	window = ecore_window_override_new(b->win.main, u->x, u->y, u->w, u->h);
	ecore_window_set_events_propagate(window, 1);
	ecore_window_set_events(window, XEV_IN_OUT | XEV_MOUSE_MOVE |
			XEV_BUTTON);
	ecore_window_set_background_pixmap(window, temp);
	ecore_pixmap_free(temp);

	/* Copy the large mask to a series of small masks. */
	temp = ecore_pixmap_new(b->win.b, u->w, u->h, 1);
	ecore_gc_set_fg(gc2, 0);
	ecore_fill_rectangle(temp, gc2, 0, 0, u->w, u->h);
	ecore_gc_set_fg(gc2, 1);
	ecore_area_copy(mask, temp, gc2, u->x, u->y, u->w, u->h, 0, 0);

	ecore_window_set_shape_mask(window, temp);
	ecore_pixmap_free(temp);

	nwin = evas_list_append(nwin, (void *)window);
	up = up->next;
     }

   evas_render_updates_free(hp);

   ecore_gc_free(gc1);
   ecore_gc_free(gc2);

   ecore_pixmap_free(pmap);
   ecore_pixmap_free(mask);

   /* Update the display all at once. */
   b->windows = nwin;
   while (nwin)
     {
	window = (Window)nwin->data;
	ecore_window_raise(window);
	ecore_window_show(window);
	nwin = nwin->next;
     }

   /* Order is important here to have a smooth update */
   e_border_redraw_region(b, 0, 0, b->current.w, b->current.h);
   e_border_cleanup_window_list(owin);

   D("Finished rendering update\n");
}

void
e_border_update(E_Border * b)
{
   int                 location_changed = 0;
   int                 size_changed = 0;
   int                 border_changed = 0;
   int                 visibility_changed = 0;
   int                 state_changed = 0;

   D_ENTER;

   if (b->hold_changes)
      D_RETURN;
   if (!b->changed)
      D_RETURN;

   b->current.visible = b->current.requested.visible;

   if ((b->current.x != b->previous.x) || (b->current.y != b->previous.y))
      location_changed = 1;
   if ((b->current.w != b->previous.w) || (b->current.h != b->previous.h))
      size_changed = 1;
   if ((size_changed) && (b->current.has_shape))
      b->shape_changed = 1;
   if (b->current.selected != b->previous.selected)
      state_changed = 1;
   if (state_changed)
     {
	e_border_apply_border(b);
	if (!border_changed)
	  {
	     ecore_window_gravity_set(b->win.container, StaticGravity);
	     border_changed = 1;
	     size_changed = 1;
	  }
     }
   if (b->bits.new)
     {
	ecore_window_gravity_set(b->win.container, StaticGravity);
	border_changed = 1;
     }
   if ((border_changed) && (b->current.has_shape))
      b->shape_changed = 1;
   if (b->current.visible != b->previous.visible)
      visibility_changed = 1;

   if ((location_changed) && (!size_changed))
     {
	int                 pl, pr, pt, pb;

	ecore_window_move(b->win.main, b->current.x, b->current.y);
	pl = pr = pt = pb = 0;
	if (b->bits.b)
	   ebits_get_insets(b->bits.b, &pl, &pr, &pt, &pb);
	e_icccm_move_resize(b->win.client,
			    b->current.x + pl,
			    b->current.y + pt, b->client.w, b->client.h);
	e_cb_border_move_resize(b);
     }
   else if (size_changed)
     {
	int                 pl, pr, pt, pb, w, h;
	int                 smaller;

	if ((b->current.shaped_client) || (b->previous.shaped_client) ||
	    (b->current.shape_changes) || (b->previous.shape_changes) ||
	    (b->current.has_shape) || (b->previous.has_shape))
	   b->shape_changed = 1;
	smaller = 0;
	if ((b->current.w < b->previous.w) || (b->current.h < b->previous.h))
	   smaller = 1;
	pl = pr = pt = pb = 0;
	if (b->bits.b)
	   ebits_get_insets(b->bits.b, &pl, &pr, &pt, &pb);
	ecore_window_move_resize(b->win.input,
				 0, 0, b->current.w, b->current.h);
	ecore_window_move_resize(b->win.main,
				 b->current.x, b->current.y,
				 b->current.w, b->current.h);
	w = b->current.w, h = b->current.h;
	if ((w < 1) || (h < 1))
	   ecore_window_hide(b->win.b);
	else
	  {
	     ecore_window_show(b->win.b);
	     ecore_window_move_resize(b->win.b, 0, 0, w, h);
	     evas_output_size_set(b->evas, w, h);
	     evas_output_viewport_set(b->evas, 0, 0, w, h);
	  }

	if (b->current.shaded == b->client.h)
	  {
	     ecore_window_move_resize(b->win.container,
				      b->current.w + 1,
				      b->current.h + 1, 320, 320);
	  }
	else
	  {
	     ecore_window_move_resize(b->win.container,
				      pl,
				      pt,
				      b->current.w - pl - pr,
				      b->current.h - pt - pb);
	  }

	ecore_window_move_resize(b->win.client,
				 0, -b->current.shaded,
				 b->client.w, b->client.h);

	e_icccm_move_resize(b->win.client,
			    b->current.x + pl,
			    b->current.y + pt - b->current.shaded, b->client.w,
			    b->client.h);
	e_cb_border_move_resize(b);
     }
   if ((b->client.title) && (b->bits.b))
     {
	if (b->obj.title)
	  {
	     if (strcmp(b->client.title, b->obj.title->text))
	       {
	          e_text_set_text(b->obj.title, b->client.title);
		  b->shape_changed = 1;
	       }
	     if (b->current.selected)
		e_text_set_state(b->obj.title, "selected");
	     else
		e_text_set_state(b->obj.title, "normal");
	  }
     }
   if (visibility_changed)
     {
	if (b->current.visible)
	   ecore_window_show(b->win.main);
	else
	   ecore_window_hide(b->win.main);
	e_cb_border_visibility(b);
     }

   if (border_changed)
      ecore_window_gravity_set(b->win.container, NorthWestGravity);
   b->bits.new = 0;
   b->previous = b->current;
   b->changed = 0;

   D_RETURN;
}

void
e_border_set_layer(E_Border * b, int layer)
{
   int                 dl;

   D_ENTER;

   if (b->client.layer == layer)
      D_RETURN;
   dl = layer - b->client.layer;
   b->client.layer = layer;
   if (dl > 0)
      e_border_lower(b);
   else
      e_border_raise(b);

   D_RETURN;
}

static void
e_border_raise_delayed(int val, void *b)
{
   int                 auto_raise = 0;

   D_ENTER;

   auto_raise = config_data->window->auto_raise;
   if (auto_raise)
      e_border_raise((E_Border *) b);

   D_RETURN;
   UN(val);
}

float
e_border_delayed_val()
{
   return config_data->window->raise_delay;
}

void
e_border_raise(E_Border * b)
{
   Evas_List          *l;
   Evas_List         **windows;
   E_Border           *rel;

   D_ENTER;

   /* Sticky windows are not on a particular desktop, but we need the current
    * desktop window list to raise the window correctly. */
   if (b->client.sticky)
     {
	E_Desktop          *desk;

        desk = e_desktops_get(e_desktops_get_current());
	windows = &desk->windows;
     }
   else
      windows = &b->desk->windows;

   if (!(*windows))
     {
	*windows = evas_list_append(*windows, b);
	ecore_window_raise(b->win.main);
	D_RETURN;
     }
   for (l = *windows; l; l = l->next)
     {
	rel = l->data;
	if (rel->client.layer > b->client.layer)
	  {
	     if (!b->client.sticky)
	       {
	          *windows = evas_list_remove(*windows, b);
	          *windows = evas_list_prepend_relative(*windows, b, rel);
	       }

	     ecore_window_stack_below(b->win.main, rel->win.main);
	     D_RETURN;
	  }
	if ((!l->next) && (l->data != b))
	  {
	     if (!b->client.sticky)
	       {
	          *windows = evas_list_remove(*windows, b);
	          *windows = evas_list_append(*windows, b);
	       }
	     ecore_window_raise(b->win.main);
	     D_RETURN;
	  }
     }

   D_RETURN;
}

void
e_border_lower(E_Border * b)
{
   Evas_List          *l;
   Evas_List         **windows;
   E_Border           *rel;

   D_ENTER;

   /* Sticky windows are not on a particular desktop, but we need the current
    * desktop window list to raise the window correctly. */
   if (b->client.sticky)
     {
	E_Desktop          *desk;

        desk = e_desktops_get(e_desktops_get_current());
	windows = &desk->windows;
     }
   else
	windows = &b->desk->windows;

   if (!(*windows))
     {
	*windows = evas_list_append(*windows, b);
	ecore_window_raise(b->win.main);
	D_RETURN;
     }
   for (l = *windows; l; l = l->next)
     {
	rel = l->data;
	if (rel->client.layer == b->client.layer)
	  {
	     if (b == rel)
		D_RETURN;
	     *windows = evas_list_remove(*windows, b);
	     *windows = evas_list_prepend_relative(*windows, b, rel);
	     ecore_window_stack_below(b->win.main, rel->win.main);
	     D_RETURN;
	  }
     }

   D_RETURN;
}

void
e_border_raise_above(E_Border * b, E_Border * above)
{
   Evas_List         **windows;

   D_ENTER;

   /* Sticky windows are not on a particular desktop, but we need the current
    * desktop window list to raise the window correctly. */
   if (b->client.sticky)
     {
	E_Desktop          *desk;

        desk = e_desktops_get(e_desktops_get_current());
	windows = &desk->windows;
     }
   else
	windows = &b->desk->windows;

   if (!(*windows))
     {
	*windows = evas_list_append(*windows, b);
	ecore_window_raise(b->win.main);
	D_RETURN;
     }
   if (!evas_list_find(*windows, above))
      D_RETURN;
   if (b->client.layer < above->client.layer)
      b->client.layer = above->client.layer;
   *windows = evas_list_remove(*windows, b);
   *windows = evas_list_append_relative(*windows, b, above);
   ecore_window_stack_above(b->win.main, above->win.main);

   D_RETURN;
}

void
e_border_lower_below(E_Border * b, E_Border * below)
{
   Evas_List         **windows;

   D_ENTER;

   /* Sticky windows are not on a particular desktop, but we need the current
    * desktop window list to raise the window correctly. */
   if (b->client.sticky)
     {
	E_Desktop          *desk;

        desk = e_desktops_get(e_desktops_get_current());
	windows = &desk->windows;
     }
   else
	windows = &b->desk->windows;

   if (!(*windows))
     {
	*windows = evas_list_append(*windows, b);
	D_RETURN;
     }
   if (!evas_list_find(*windows, below))
      D_RETURN;
   if (b->client.layer > below->client.layer)
      b->client.layer = below->client.layer;
   *windows = evas_list_remove(*windows, b);
   *windows = evas_list_prepend_relative(*windows, b, below);
   ecore_window_stack_below(b->win.main, below->win.main);

   D_RETURN;
}

void
e_border_init(void)
{
   D_ENTER;

   ecore_event_filter_handler_add(ECORE_EVENT_MOUSE_DOWN, e_mouse_down);
   ecore_event_filter_handler_add(ECORE_EVENT_MOUSE_UP, e_mouse_up);
   ecore_event_filter_handler_add(ECORE_EVENT_MOUSE_MOVE, e_mouse_move);
   ecore_event_filter_handler_add(ECORE_EVENT_MOUSE_IN, e_mouse_in);
   ecore_event_filter_handler_add(ECORE_EVENT_MOUSE_OUT, e_mouse_out);
   ecore_event_filter_handler_add(ECORE_EVENT_WINDOW_EXPOSE, e_window_expose);
   ecore_event_filter_handler_add(ECORE_EVENT_WINDOW_MAP_REQUEST,
				  e_map_request);
   ecore_event_filter_handler_add(ECORE_EVENT_WINDOW_CONFIGURE_REQUEST,
				  e_configure_request);
   ecore_event_filter_handler_add(ECORE_EVENT_WINDOW_PROPERTY, e_property);
   ecore_event_filter_handler_add(ECORE_EVENT_WINDOW_UNMAP, e_unmap);
   ecore_event_filter_handler_add(ECORE_EVENT_WINDOW_DESTROY, e_destroy);
   ecore_event_filter_handler_add(ECORE_EVENT_WINDOW_CIRCULATE_REQUEST,
				  e_circulate_request);
   ecore_event_filter_handler_add(ECORE_EVENT_WINDOW_REPARENT, e_reparent);
   ecore_event_filter_handler_add(ECORE_EVENT_WINDOW_SHAPE, e_shape);
   ecore_event_filter_handler_add(ECORE_EVENT_WINDOW_FOCUS_IN, e_focus_in);
   ecore_event_filter_handler_add(ECORE_EVENT_WINDOW_FOCUS_OUT, e_focus_out);
   ecore_event_filter_handler_add(ECORE_EVENT_MESSAGE, e_client_message);
   ecore_event_filter_handler_add(ECORE_EVENT_COLORMAP, e_colormap);
   ecore_event_filter_idle_handler_add(e_idle, NULL);

   delayed_window_raise =
      e_delayed_action_new(E_EVENT_BORDER_FOCUS_IN,
			   e_border_delayed_val, e_border_raise_delayed);

   ecore_add_event_timer("e_border_poll()", 1.00, e_border_poll, 0, NULL);

   D_RETURN;
}

void
e_border_adopt_children(Window win)
{
   Window             *wins;
   int                 i, num;

   D_ENTER;

   wins = ecore_window_get_children(win, &num);
   if (wins)
     {
	for (i = 0; i < num; i++)
	  {
	     if (ecore_window_is_manageable(wins[i]))
	       {
		  E_Border           *b;

		  b = e_border_adopt(wins[i], 1);
		  {
		     int                 pl, pr, pt, pb;

		     pl = pr = pt = pb = 0;
		     if (b->bits.b)
			ebits_get_insets(b->bits.b, &pl, &pr, &pt, &pb);
		     b->current.requested.x -= pl;
		     b->current.requested.y -= pt;
		     b->changed = 1;
		     e_border_adjust_limits(b);
		  }
		  b->ignore_unmap = 2;
	       }
	  }
	FREE(wins);
     }

   D_RETURN;
}


E_Border           *
e_border_current_select(void)
{
   Evas_List *           l;
   E_Desktop *        desk;

   /* Only check for borders on the current desktop */
   desk = e_desktops_get(e_desktops_get_current());

   D_ENTER;
   for (l = borders; l; l = l->next)
     {
	E_Border           *b;

	b = l->data;
	if (b->current.selected && b->desk == desk)
	   D_RETURN_(b);
     }

   D_RETURN_(NULL);
}


E_Border           *
e_border_current_focused(void)
{
   Evas_List *           l;
   E_Desktop *        desk;

   /* Only check for borders on the current desktop */
   desk = e_desktops_get(e_desktops_get_current());

   D_ENTER;
   for (l = borders; l; l = l->next)
     {
	E_Border           *b;

	b = l->data;
	if (b->current.selected && b->desk == desk)
	   D_RETURN_(b);
     }
   for (l = borders; l; l = l->next)
     {
	E_Border           *b;

	b = l->data;
	if (b->current.select_lost_from_grab && b->desk == desk)
	   D_RETURN_(b);
     }

   D_RETURN_(NULL);
}

void
e_border_focus_grab_ended(void)
{
   Evas_List *           l;
   E_Desktop *desk;

   D_ENTER;

   desk = e_desktops_get(e_desktops_get_current());

   for (l = borders; l; l = l->next)
     {
	E_Border           *b;

	b = l->data;
	/* Only change selection of items on present desktop */
	if(b->desk == desk)
	  {
	    b->current.select_lost_from_grab = 0;
	    b->current.selected = 0;
	    b->changed = 1;
	  }
     }

   D_RETURN;
}

int
e_border_viewable(E_Border * b)
{
   D_ENTER;

   if (b->desk != e_desktops_get(e_desktops_get_current()))
      D_RETURN_(0);

   if (b->current.x + b->current.w <= 0)
      D_RETURN_(0);

   if (b->current.x >= b->desk->real.w)
      D_RETURN_(0);

   if (b->current.y + b->current.h <= 0)
      D_RETURN_(0);

   if (b->current.y >= b->desk->real.h)
      D_RETURN_(0);

   if (!b->current.visible)
      D_RETURN_(0);

   D_RETURN_(1);
}

void
e_border_send_pointer(E_Border * b)
{
   D_ENTER;

   ecore_pointer_warp_to(b->current.x + b->current.w / 2,
		         b->current.y + b->current.h / 2);

   D_RETURN;
}

void
e_border_print_pos(char *buf, E_Border * b)
{
   D_ENTER;

   snprintf(buf, PATH_MAX, "%i, %i", b->current.x, b->current.y);

   D_RETURN;
}

void
e_border_print_size(char *buf, E_Border * b)
{
   D_ENTER;

   if ((b->client.step.w > 1) || (b->client.step.h > 1))
     {
	snprintf(buf, PATH_MAX, "%i x %i",
		 (b->client.w - b->client.base.w) / b->client.step.w,
		 (b->client.h - b->client.base.h) / b->client.step.h);
     }
   else
     {
	snprintf(buf, PATH_MAX, "%i x %i", b->client.w, b->client.h);
     }

   D_RETURN;
}

void
e_border_set_gravity(E_Border * b, int gravity)
{
   D_ENTER;

   if (!b)
      D_RETURN;

   ecore_window_gravity_set(b->win.container, gravity);
   ecore_window_gravity_set(b->win.input, gravity);
   ecore_window_gravity_set(b->win.b, gravity);

   D_RETURN;
}

Evas_List *
e_border_get_borders_list()
{
   D_ENTER;
   D_RETURN_(borders);
}

void
e_borders_scroll_list(Evas_List *borders, int dx, int dy)
{
   Evas_List *l;

   for (l = borders; l; l = l->next)
     {
	E_Border           *b;

	b = l->data;
	ecore_window_gravity_reset(b->win.main);
	if ((!b->client.is_desktop) && (!b->mode.move))
	  {
	     b->previous.requested.x = b->current.requested.x;
	     b->previous.requested.y = b->current.requested.y;
	     b->previous.x = b->current.x;
	     b->previous.y = b->current.y;
	     b->current.requested.x += dx;
	     b->current.requested.y += dy;
	     b->current.x = b->current.requested.x;
	     b->current.y = b->current.requested.y;
	     b->changed = 1;
	  }
     }
}


E_Border *
e_border_shuffle_last(E_Border *b)
{
   Evas_List *           next;
   E_Border           *start;
   E_Border           *current = NULL;
   E_Desktop    *current_desk;

   D_ENTER;

   current_desk = e_desktops_get(e_desktops_get_current());

   if (!current_desk || !current_desk->windows)
     {
       e_icccm_send_focus_to( e_desktop_window(), 1);
       D_RETURN_(NULL);
     }

   if(b)
     current = b;
   else
     current = evas_list_last(current_desk->windows)->data;

   /* Find the current border on the list of borders */
   for (next = current_desk->windows; next && next->data != current; next = next->next);

   /* Step to the next border, wrap around the queue if the end is reached */
   if (next && next->prev)
      current = next->prev->data;
   else
      current = evas_list_last(next)->data;

   e_icccm_send_focus_to(current->win.client, current->client.takes_focus);

   D_RETURN_(current);
}
