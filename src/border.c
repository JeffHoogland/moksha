#include "cursors.h"
#include "border.h"
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
#include "menu.h"
#include "exec.h"

/* Window border rendering, querying, setting  & modification code */

/* globals local to window borders */
static Evas_List    evases = NULL;
static Evas_List    borders = NULL;

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

static int
e_border_replay_query(Ecore_Event_Mouse_Down * ev)
{
   E_Border           *b;

   D_ENTER;

   b = e_border_find_by_window(ev->win);
   if (b)
     {
	int                 focus_mode;

	E_CFG_INT(cfg_focus_mode, "settings", "/focus/mode", 0);

	E_CONFIG_INT_GET(cfg_focus_mode, focus_mode);
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
   Evas_List           l;

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
	if (b->first_expose)
	  {
	     evas_render(b->evas);
	  }
     }
   e_db_runtime_flush();

   D_RETURN;
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
			e_border_release(b);
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
		   e_border_release(b);
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

	   E_CFG_INT(cfg_focus_mode, "settings", "/focus/mode", 0);

	   E_CONFIG_INT_GET(cfg_focus_mode, focus_mode);
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

	   E_CFG_INT(cfg_focus_mode, "settings", "/focus/mode", 0);

	   E_CONFIG_INT_GET(cfg_focus_mode, focus_mode);
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
		Evas                evas;
		int                 x, y;

		evas = b->evas;
		ecore_window_get_root_relative_location(evas_get_window(evas),
							&x, &y);
		x = e->rx - x;
		y = e->ry - y;
		evas_event_button_down(evas, x, y, e->button);
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
		Evas                evas;
		int                 x, y;

		evas = b->evas;
		ecore_window_get_root_relative_location(evas_get_window(evas),
							&x, &y);
		x = e->rx - x;
		y = e->ry - y;
		evas_event_button_up(evas, x, y, e->button);
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
		Evas                evas;
		int                 x, y;

		evas = b->evas;
		ecore_window_get_root_relative_location(evas_get_window(evas),
							&x, &y);
		x = e->rx - x;
		y = e->ry - y;
		evas_event_move(evas, x, y);
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
	     Evas                evas;

	     evas = b->evas;
	     ecore_window_get_root_relative_location(evas_get_window(evas), &x,
						     &y);
	     x = e->rx - x;
	     y = e->ry - y;
	     evas_event_move(evas, x, y);
	     evas_event_enter(evas);
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
		evas_event_leave(b->evas);
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
      Evas_List           l;
      E_Border           *b;

      for (l = evases; l; l = l->next)
	{
	   Evas                evas;

	   evas = l->data;
	   if (evas_get_window(evas) == e->win)
	      evas_update_rect(evas, e->x, e->y, e->w, e->h);
	}
      b = e_border_find_by_window(e->win);
      if (b)
	 b->first_expose = 1;
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

   E_CFG_INT(cfg_focus_mode, "settings", "/focus/mode", 0);

   D_ENTER;
   E_CONFIG_INT_GET(cfg_focus_mode, focus_mode);
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

   E_CFG_INT(cfg_focus_mode, "settings", "/focus/mode", 0);

   D_ENTER;

   E_CONFIG_INT_GET(cfg_focus_mode, focus_mode);
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
      Evas_List           l;

    again:
      for (l = b->grabs; l; l = l->next)
	{
	   E_Grab             *g;

	   g = l->data;
	   /* find a grab that triggered this */
	   if (((((Ecore_Event_Mouse_Down *) (e->event))->button == g->button)
		|| (g->button == 0)) && ((g->any_mod)
					 ||
					 (((Ecore_Event_Mouse_Down *) (e->
								       event))->
					  mods == g->mods)))
	     {
		if (g->remove_after)
		  {
		     ecore_button_ungrab(b->win.main, g->button, g->mods,
					 g->any_mod);
		     ecore_window_button_grab_auto_replay_set(b->win.main,
							      NULL);
		     FREE(g);
		     b->grabs = evas_list_remove(b->grabs, g);
		     goto again;
		  }
	     }
	}
   }
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
e_border_cleanup(E_Border * b)
{
   Evas_List           l;

   D_ENTER;

   e_match_save_props(b);
   D("before notify\n");
   e_observee_notify_observers(E_OBSERVEE(b), E_EVENT_BORDER_DELETE, NULL);
   D("after notify\n");
   while (b->menus)
     {
	E_Menu             *m;

	m = b->menus->data;
	e_menu_hide(m);
	e_object_unref(E_OBJECT(m));
	b->menus = evas_list_remove(b->menus, m);
     }
   e_desktops_del_border(b->desk, b);
   if (b->bits.b)
      ebits_free(b->bits.b);

   if (b->obj.title)
      e_text_free(b->obj.title);

   evases = evas_list_remove(evases, b->evas);
   evas_free(b->evas);
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

   if (b->grabs)
     {
	for (l = b->grabs; l; l = l->next)
	  {
	     FREE(l->data);
	  }
	evas_list_free(b->grabs);
     }

   /* Cleanup superclass. */
   e_object_cleanup(E_OBJECT(b));

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
      prop_sticky = 1;

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
   static Evas         e = NULL;
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

   if (!e)
     {
	e = evas_new();
	evas_set_output_method(e, RENDER_METHOD_IMAGE);
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

	if (b->bits.file)
	  {
	     Imlib_Image         im;
	     Ebits_Object        bit;
	     Pixmap              pmap, mask;

	     printf("SHAPE update for border %s\n", b->bits.file);
	     pmap = ecore_pixmap_new(shape_win, b->current.w, b->current.h, 0);
	     mask = ecore_pixmap_new(shape_win, b->current.w, b->current.h, 1);

	     im = imlib_create_image(b->current.w, b->current.h);
	     imlib_context_set_image(im);
	     imlib_image_set_has_alpha(1);
	     imlib_image_clear();

	     evas_set_output_image(e, im);
	     evas_set_output_size(e, b->current.w, b->current.h);
	     evas_set_output_viewport(e, 0, 0, b->current.w, b->current.h);

	     bit = ebits_load(b->bits.file);
	     ebits_add_to_evas(bit, e);
	     ebits_move(bit, 0, 0);
	     ebits_resize(bit, b->current.w, b->current.h);
	     ebits_show(bit);

	     evas_update_rect(e, 0, 0, b->current.w, b->current.h);
	     evas_render(e);

	     ebits_hide(bit);
	     ebits_free(bit);

	     imlib_context_set_image(im);
	     imlib_context_set_dither_mask(1);
	     imlib_context_set_dither(1);
	     imlib_context_set_drawable(pmap);
	     imlib_context_set_mask(mask);
	     imlib_context_set_blend(0);
	     imlib_context_set_color_modifier(NULL);
	     imlib_render_image_on_drawable(0, 0);
	     imlib_free_image();

	     ecore_window_set_background_pixmap(shape_win, pmap);
	     ecore_window_add_shape_mask(shape_win, mask);
	     ecore_window_clear(shape_win);

	     ecore_pixmap_free(pmap);
	     ecore_pixmap_free(mask);
	  }

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
			   XEV_CONFIGURE |
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
   int                 max_colors = 216;
   int                 font_cache = 1024 * 1024;
   int                 image_cache = 8192 * 1024;
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

   desk = e_desktops_get(0);
   e_desktops_add_border(desk, b);
   b->win.main = ecore_window_override_new(desk->win.container, 0, 0, 1, 1);
   b->win.input = ecore_window_input_new(b->win.main, 0, 0, 1, 1);
   b->win.container = ecore_window_override_new(b->win.main, 0, 0, 1, 1);
   e_cursors_display_in_window(b->win.container, "Application");
   ecore_window_set_events_propagate(b->win.input, 1);
   ecore_window_set_events(b->win.input, XEV_MOUSE_MOVE | XEV_BUTTON);
   ecore_window_set_events(b->win.main, XEV_IN_OUT);
   ecore_window_set_events(b->win.container, XEV_IN_OUT);
   ecore_window_show(b->win.input);
   ecore_window_show(b->win.container);

   b->evas = evas_new_all(ecore_display_get(),
			  b->win.main,
			  0, 0, 1, 1,
			  RENDER_METHOD_ALPHA_SOFTWARE,
			  max_colors, font_cache, image_cache, font_dir);
   b->win.b = evas_get_window(b->evas);
   e_cursors_display_in_window(b->win.b, "Default");

   b->obj.title = e_text_new(b->evas, "", "title");
   b->obj.title_clip = evas_add_rectangle(b->evas);
   evas_set_color(b->evas, b->obj.title_clip, 255, 255, 255, 255);
   e_text_show(b->obj.title);
   evas_show(b->evas, b->obj.title_clip);
   e_text_set_clip(b->obj.title, b->obj.title_clip);

   ecore_window_raise(b->win.input);
   ecore_window_raise(b->win.container);

   evases = evas_list_append(evases, b->evas);

   ecore_window_set_events(b->win.b, XEV_EXPOSE);

   ecore_window_show(b->win.b);

   e_border_attach_mouse_grabs(b);

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
   Evas_List           l;

   D_ENTER;

   if (b->grabs)
     {
	for (l = b->grabs; l; l = l->next)
	  {
	     E_Grab             *g;

	     g = l->data;
	     ecore_button_ungrab(b->win.main, g->button, g->mods, g->any_mod);
	     FREE(g);
	  }
	evas_list_free(b->grabs);
	b->grabs = NULL;
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
   char               *grabs_db;
   E_DB_File          *db;
   int                 focus_mode;
   char                buf[PATH_MAX];

   E_CFG_INT(cfg_focus_mode, "settings", "/focus/mode", 0);

   D_ENTER;

   E_CONFIG_INT_GET(cfg_focus_mode, focus_mode);

   grabs_db = e_config_get("grabs");
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

   /* other grabs - liek alt+left to move */
   db = e_db_open_read(grabs_db);
   if (db)
     {
	int                 i, num;

	snprintf(buf, PATH_MAX, "/grabs/count");
	if (!e_db_int_get(db, buf, &num))
	  {
	     e_db_close(db);
	     D_RETURN;
	  }
	for (i = 0; i < num; i++)
	  {
	     int                 button, any_mod, mod;
	     Ecore_Event_Key_Modifiers mods;

	     button = -1;
	     mods = ECORE_EVENT_KEY_MODIFIER_NONE;
	     any_mod = 0;
	     snprintf(buf, PATH_MAX, "/grabs/%i/button", i);
	     if (!e_db_int_get(db, buf, &button))
		continue;
	     snprintf(buf, PATH_MAX, "/grabs/%i/modifiers", i);
	     if (!e_db_int_get(db, buf, &mod))
		continue;
	     if (mod == -1)
		any_mod = 1;
	     mods = (Ecore_Event_Key_Modifiers) mod;

	     if (button >= 0)
	       {
		  E_Grab             *g;

		  g = NEW(E_Grab, 1);
		  ZERO(g, E_Grab, 1);
		  g->button = button;
		  g->mods = mods;
		  g->any_mod = any_mod;
		  g->remove_after = 0;
		  b->grabs = evas_list_append(b->grabs, g);
		  ecore_button_grab(b->win.main, button, XEV_BUTTON_PRESS, mods,
				    0);
	       }
	  }
	e_db_close(db);
     }

   D_RETURN;
}

void
e_border_remove_all_mouse_grabs(void)
{
   Evas_List           l;

   D_ENTER;

   for (l = borders; l; l = l->next)
      e_border_remove_mouse_grabs((E_Border *) l->data);

   D_RETURN;
}

void
e_border_attach_all_mouse_grabs(void)
{
   Evas_List           l;

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
   Evas_List           l;

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
   Evas_List           l;

   D_ENTER;

   for (l = borders; l; l = l->next)
     {
	E_Border           *b;

	b = l->data;

	if ((win == b->win.main) ||
	    (win == b->win.client) ||
	    (win == b->win.container) ||
	    (win == b->win.input) || (win == b->win.b))
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
   if (b->bits.file)
      free(b->bits.file);

   b->bits.b = ebits_load(file);
   if (b->bits.b)
      b->bits.file = strdup(file);
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
ebits_set_classed_bit_callback(b->bits.b, _class, CALLBACK_MOUSE_IN, e_cb_mouse_in, b); \
ebits_set_classed_bit_callback(b->bits.b, _class, CALLBACK_MOUSE_OUT, e_cb_mouse_out, b); \
ebits_set_classed_bit_callback(b->bits.b, _class, CALLBACK_MOUSE_DOWN, e_cb_mouse_down, b); \
ebits_set_classed_bit_callback(b->bits.b, _class, CALLBACK_MOUSE_UP, e_cb_mouse_up, b); \
ebits_set_classed_bit_callback(b->bits.b, _class, CALLBACK_MOUSE_MOVE, e_cb_mouse_move, b);
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
e_border_update(E_Border * b)
{
   int                 location_changed = 0;
   int                 size_changed = 0;
   int                 shape_changed = 0;
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
      shape_changed = 1;
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
      shape_changed = 1;
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
	int                 pl, pr, pt, pb, x, y, w, h;
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
	if (smaller)
	  {
	     if (b->current.shaded == b->client.h)
	       {
		  ecore_window_move_resize(b->win.client,
					   0, -b->current.shaded,
					   b->client.w, b->client.h);
		  ecore_window_move_resize(b->win.container,
					   b->current.w + 1,
					   b->current.h + 1, 320, 320);
	       }
	     else
	       {
		  ecore_window_move_resize(b->win.client,
					   0, -b->current.shaded,
					   b->client.w, b->client.h);
		  ecore_window_move_resize(b->win.container,
					   pl,
					   pt,
					   b->current.w - pl - pr,
					   b->current.h - pt - pb);
	       }
	     ecore_window_move_resize(b->win.main,
				      b->current.x, b->current.y,
				      b->current.w, b->current.h);

	     x = 0, y = 0, w = b->current.w, h = b->current.h;
	     if ((w < 1) || (h < 1))
		ecore_window_hide(b->win.b);
	     else
	       {
		  ecore_window_show(b->win.b);
		  ecore_window_move_resize(b->win.b, x, y, w, h);
		  evas_set_output_size(b->evas, w, h);
		  evas_set_output_viewport(b->evas, x, y, w, h);
	       }
	  }
	else
	  {
	     ecore_window_move_resize(b->win.main,
				      b->current.x, b->current.y,
				      b->current.w, b->current.h);
	     x = 0, y = 0, w = b->current.w, h = b->current.h;
	     if ((w < 1) || (h < 1))
		ecore_window_hide(b->win.b);
	     else
	       {
		  ecore_window_show(b->win.b);
		  ecore_window_move_resize(b->win.b, x, y, w, h);
		  evas_set_output_size(b->evas, w, h);
		  evas_set_output_viewport(b->evas, x, y, w, h);
	       }

	     if (b->current.shaded == b->client.h)
	       {
		  ecore_window_move_resize(b->win.container,
					   b->current.w + 1,
					   b->current.h + 1, 320, 320);
		  ecore_window_move_resize(b->win.client,
					   0, -b->current.shaded,
					   b->client.w, b->client.h);
	       }
	     else
	       {
		  ecore_window_move_resize(b->win.container,
					   pl,
					   pt,
					   b->current.w - pl - pr,
					   b->current.h - pt - pb);
		  ecore_window_move_resize(b->win.client,
					   0, -b->current.shaded,
					   b->client.w, b->client.h);
	       }
	  }

	if (b->bits.b)
	  {
	     ebits_resize(b->bits.b, b->current.w, b->current.h);
	     evas_clear_obscured_rects(b->evas);
	     evas_add_obscured_rect(b->evas, pl, pt, b->current.w - pl - pr,
				    b->current.h - pt - pb);
	  }

	e_icccm_move_resize(b->win.client,
			    b->current.x + pl,
			    b->current.y + pt - b->current.shaded, b->client.w,
			    b->client.h);
	e_cb_border_move_resize(b);
     }
   if ((b->client.title) && (b->bits.b))
     {
	double              tx, ty, tw, th;

	ebits_get_named_bit_geometry(b->bits.b, "Title_Area", &tx, &ty, &tw,
				     &th);

	if (b->obj.title)
	  {
	     e_text_set_text(b->obj.title, b->client.title);
	     e_text_move(b->obj.title, tx, ty);
	     if (b->current.selected)
		e_text_set_state(b->obj.title, "selected");
	     else
		e_text_set_state(b->obj.title, "normal");
	  }
	evas_move(b->evas, b->obj.title_clip, tx, ty);
	evas_resize(b->evas, b->obj.title_clip, tw, th);

	if (b->obj.title)
	   e_text_set_layer(b->obj.title, 1);
     }
   e_border_reshape(b);
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

   E_CFG_INT(cfg_auto_raise, "settings", "/window/raise/auto", 0);

   D_ENTER;

   E_CONFIG_INT_GET(cfg_auto_raise, auto_raise);
   if (auto_raise)
      e_border_raise((E_Border *) b);

   D_RETURN;
   UN(val);
}

void
e_border_raise(E_Border * b)
{
   Evas_List           l;
   E_Border           *rel;

   D_ENTER;

   if (!b->desk->windows)
     {
	b->desk->windows = evas_list_append(b->desk->windows, b);
	b->desk->changed = 1;
	ecore_window_raise(b->win.main);
	D_RETURN;
     }
   for (l = b->desk->windows; l; l = l->next)
     {
	rel = l->data;
	if (rel->client.layer > b->client.layer)
	  {
	     b->desk->windows = evas_list_remove(b->desk->windows, b);
	     b->desk->windows =
		evas_list_prepend_relative(b->desk->windows, b, rel);
	     b->desk->changed = 1;
	     ecore_window_stack_below(b->win.main, rel->win.main);
	     D_RETURN;
	  }
	if ((!l->next) && (l->data != b))
	  {
	     b->desk->windows = evas_list_remove(b->desk->windows, b);
	     b->desk->windows = evas_list_append(b->desk->windows, b);
	     b->desk->changed = 1;
	     ecore_window_raise(b->win.main);
	     D_RETURN;
	  }
     }

   D_RETURN;
}

void
e_border_lower(E_Border * b)
{
   Evas_List           l;
   E_Border           *rel;

   D_ENTER;

   if (!b->desk->windows)
     {
	b->desk->windows = evas_list_append(b->desk->windows, b);
	b->desk->changed = 1;
	ecore_window_raise(b->win.main);
	D_RETURN;
     }
   for (l = b->desk->windows; l; l = l->next)
     {
	rel = l->data;
	if (rel->client.layer == b->client.layer)
	  {
	     if (b == rel)
		D_RETURN;
	     b->desk->windows = evas_list_remove(b->desk->windows, b);
	     b->desk->windows =
		evas_list_prepend_relative(b->desk->windows, b, rel);
	     b->desk->changed = 1;
	     ecore_window_stack_below(b->win.main, rel->win.main);
	     D_RETURN;
	  }
     }

   D_RETURN;
}

void
e_border_raise_above(E_Border * b, E_Border * above)
{
   D_ENTER;

   if (!b->desk->windows)
     {
	b->desk->windows = evas_list_append(b->desk->windows, b);
	b->desk->changed = 1;
	ecore_window_raise(b->win.main);
	D_RETURN;
     }
   if (!evas_list_find(b->desk->windows, above))
      D_RETURN;
   if (b->client.layer < above->client.layer)
      b->client.layer = above->client.layer;
   b->desk->windows = evas_list_remove(b->desk->windows, b);
   b->desk->windows = evas_list_append_relative(b->desk->windows, b, above);
   b->desk->changed = 1;
   ecore_window_stack_above(b->win.main, above->win.main);

   D_RETURN;
}

void
e_border_lower_below(E_Border * b, E_Border * below)
{
   D_ENTER;

   if (!b->desk->windows)
     {
	b->desk->windows = evas_list_append(b->desk->windows, b);
	b->desk->changed = 1;
	D_RETURN;
     }
   if (!evas_list_find(b->desk->windows, below))
      D_RETURN;
   if (b->client.layer > below->client.layer)
      b->client.layer = below->client.layer;
   b->desk->windows = evas_list_remove(b->desk->windows, b);
   b->desk->windows = evas_list_prepend_relative(b->desk->windows, b, below);
   b->desk->changed = 1;
   ecore_window_stack_below(b->win.main, below->win.main);

   D_RETURN;
}

void
e_border_init(void)
{
   double              raise_delay = 0.5;

   E_CFG_FLOAT(cfg_raise_delay, "settings", "/window/raise/delay", 0.5);

   D_ENTER;

   E_CONFIG_FLOAT_GET(cfg_raise_delay, raise_delay);

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
			   raise_delay, e_border_raise_delayed);

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
e_border_current_focused(void)
{
   Evas_List           l;

   D_ENTER;
   for (l = borders; l; l = l->next)
     {
	E_Border           *b;

	b = l->data;
	if (b->current.selected)
	   D_RETURN_(b);
     }
   for (l = borders; l; l = l->next)
     {
	E_Border           *b;

	b = l->data;
	if (b->current.select_lost_from_grab)
	   D_RETURN_(b);
     }

   D_RETURN_(NULL);
}

void
e_border_focus_grab_ended(void)
{
   Evas_List           l;

   D_ENTER;

   for (l = borders; l; l = l->next)
     {
	E_Border           *b;

	b = l->data;
	b->current.select_lost_from_grab = 0;
	b->current.selected = 0;
	b->changed = 1;
     }

   D_RETURN;
}

int
e_border_viewable(E_Border * b)
{
   D_ENTER;

   if (b->desk != e_desktops_get(0))
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

   XWarpPointer(ecore_display_get(), None, b->win.main, 0, 0, 0, 0,
		b->current.w / 2, b->current.h / 2);

   D_RETURN;
}

void
e_border_raise_next(void)
{
   Evas_List           next;
   E_Border           *current;

   D_ENTER;

   if (!borders)
      D_RETURN;

   current = e_border_current_focused();

   /* Find the current border on the list of borders */
   for (next = borders; next && next->data != current; next = next->next);

   /* Step to the next border, wrap around the queue if the end is reached */
   if (next && next->next)
      next = next->next;
   else
      next = borders;

   /* Now find the next viewable border on the same desktop */
   current = (E_Border *) next->data;
   while (next && (!e_border_viewable(current) || current->client.is_desktop))
     {
	next = next->next;
	if (!next)
	   next = borders;

	current = (E_Border *) next->data;
     }

   e_border_raise(current);
   e_border_send_pointer(current);

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

Evas_List
e_border_get_borders_list()
{
   D_ENTER;
   D_RETURN_(borders);
}
