#include "debug.h"
#include "config.h"
#include "place.h"
#include "actions.h"
#include "guides.h"
#include "util.h"

static void         e_mouse_down(Ecore_Event * ev);
static void         e_mouse_up(Ecore_Event * ev);
static void         e_mouse_move(Ecore_Event * ev);

static int          prx, pry;
static Window       win_place = 0;

static void
e_mouse_down(Ecore_Event * ev)
{
   Ecore_Event_Mouse_Down *e;

   D_ENTER;

   e = ev->event;
   if (!win_place)
      D_RETURN;

   D_RETURN;
}

static void
e_mouse_up(Ecore_Event * ev)
{
   Ecore_Event_Mouse_Up *e;

   D_ENTER;

   e = ev->event;
   if (!win_place)
      D_RETURN;
   e_action_stop("Window_Place", ACT_MOUSE_UP, 1, NULL,
		 ECORE_EVENT_KEY_MODIFIER_NONE, NULL, NULL, e->x, e->y, e->rx,
		 e->ry);
   ecore_window_destroy(win_place);
   win_place = 0;

   D_RETURN;
}

static void
e_mouse_move(Ecore_Event * ev)
{
   Ecore_Event_Mouse_Move *e;

   D_ENTER;

   e = ev->event;
   if (!win_place)
      D_RETURN;
   e_action_cont("Window_Place", ACT_MOUSE_MOVE, 1, NULL,
		 ECORE_EVENT_KEY_MODIFIER_NONE, NULL, NULL, e->x, e->y, e->rx,
		 e->ry, e->rx - prx, e->ry - pry);
   prx = e->rx;
   pry = e->ry;

   D_RETURN;
}

static int
e_place_manual(E_Border * b, E_Desktop * desk, int *x, int *y)
{
   int                 w, h;
   int                 mx, my, rx, ry;
   int                 move_mode = E_GUIDES_BOX;

   E_CFG_INT(cfg_window_move_mode, "settings", "/window/move/mode",
	     E_GUIDES_BOX);

   D_ENTER;

   E_CONFIG_INT_GET(cfg_window_move_mode, move_mode);
   w = b->current.requested.w;
   h = b->current.requested.h;

   if (!win_place)
     {
	win_place = ecore_window_input_new(0, 0, 0, desk->real.w, desk->real.h);
	ecore_window_set_events(win_place, XEV_MOUSE_MOVE | XEV_BUTTON);
	ecore_window_raise(win_place);
	ecore_window_show(win_place);
     }
   else
     {
	e_action_stop_by_type("Window_Place");
     }

   /* get mouse coords */
   ecore_pointer_xy(desk->win.main, &mx, &my);
   rx = mx;
   ry = my;
   prx = rx;
   pry = ry;
   /* grab mouse to desktop */
   ecore_pointer_ungrab(CurrentTime);
   ecore_pointer_grab(win_place, CurrentTime);

   *x = mx - (w / 2);
   *y = my - (h / 2);

   /* start a move mode */
   e_action_stop_by_type("Window_Place");
   e_action_start("Window_Place", ACT_MOUSE_CLICK, 1, NULL,
		  ECORE_EVENT_KEY_MODIFIER_NONE, E_OBJECT(b), NULL, mx, my, rx,
		  ry);

   if (move_mode != E_GUIDES_OPAQUE)
      D_RETURN_(0);
   D_RETURN_(1);
}

static int
e_place_smart(E_Border * b, E_Desktop * desk, int *x, int *y)
{
   int                 w, h;
   int                 a_w = 0, a_h = 0;
   int                *a_x = NULL, *a_y = NULL;
   Evas_List           l;

   D_ENTER;

   w = b->current.requested.w;
   h = b->current.requested.h;
   a_w = 2;
   a_h = 2;
   a_x = NEW(int, 2);
   a_y = NEW(int, 2);

   a_x[0] = 0;
   a_x[1] = desk->real.w;
   a_y[0] = 0;
   a_y[1] = desk->real.h;

   for (l = desk->windows; l; l = l->next)
     {
	E_Border           *bd;

	bd = l->data;
	if ((bd != b) && (bd->current.visible))
	  {
	     if (INTERSECTS(bd->current.x, bd->current.y,
			    bd->current.w, bd->current.h,
			    0, 0, desk->real.w, desk->real.h))
	       {
		  int                 i, j;

		  for (i = 0; i < a_w; i++)
		    {
		       int                 ok = 1;

		       if (bd->current.x > 0)
			 {
			    if (a_x[i] == bd->current.x)
			       ok = 0;
			    else if (a_x[i] > bd->current.x)
			      {
				 a_w++;
				 REALLOC(a_x, int, a_w);

				 for (j = a_w - 1; j > i; j--)
				    a_x[j] = a_x[j - 1];
				 a_x[i] = bd->current.x;
				 ok = 0;
			      }
			 }
		       if (!ok)
			  break;
		    }
		  for (i = 0; i < a_w; i++)
		    {
		       int                 ok = 1;

		       if (bd->current.x + bd->current.w < desk->real.w)
			 {
			    if (a_x[i] == bd->current.x + bd->current.w)
			       ok = 0;
			    else if (a_x[i] > bd->current.x + bd->current.w)
			      {
				 a_w++;
				 REALLOC(a_x, int, a_w);

				 for (j = a_w - 1; j > i; j--)
				    a_x[j] = a_x[j - 1];
				 a_x[i] = bd->current.x + bd->current.w;
				 ok = 0;
			      }
			 }
		       if (!ok)
			  break;
		    }
		  for (i = 0; i < a_h; i++)
		    {
		       int                 ok = 1;

		       if (bd->current.y > 0)
			 {
			    if (a_y[i] == bd->current.y)
			       ok = 0;
			    else if (a_y[i] > bd->current.y)
			      {
				 a_h++;
				 REALLOC(a_y, int, a_h);

				 for (j = a_h - 1; j > i; j--)
				    a_y[j] = a_y[j - 1];
				 a_y[i] = bd->current.y;
				 ok = 0;
			      }
			 }
		       if (!ok)
			  break;
		    }
		  for (i = 0; i < a_h; i++)
		    {
		       int                 ok = 1;

		       if (bd->current.y + bd->current.h < desk->real.h)
			 {
			    if (a_y[i] == bd->current.y + bd->current.h)
			       ok = 0;
			    else if (a_y[i] > bd->current.y + bd->current.h)
			      {
				 a_h++;
				 REALLOC(a_y, int, a_h);

				 for (j = a_h - 1; j > i; j--)
				    a_y[j] = a_y[j - 1];
				 a_y[i] = bd->current.y + bd->current.h;
				 ok = 0;
			      }
			 }
		       if (!ok)
			  break;
		    }
	       }
	  }
     }
   {
      int                 i, j;
      int                 area = 0x7fffffff;

      for (j = 0; j < a_h - 1; j++)
	{
	   for (i = 0; i < a_w - 1; i++)
	     {
		if ((a_x[i] < (desk->real.w - w)) &&
		    (a_y[j] < (desk->real.h - h)))
		  {
		     int                 ar = 0;

		     for (l = desk->windows; l; l = l->next)
		       {
			  E_Border           *bd;
			  int                 x1, y1, w1, h1, x2, y2, w2, h2;

			  bd = l->data;
			  x1 = a_x[i];
			  y1 = a_y[j];
			  w1 = w;
			  h1 = h;
			  x2 = bd->current.x;
			  y2 = bd->current.y;
			  w2 = bd->current.w;
			  h2 = bd->current.h;
			  if ((bd != b) && (bd->current.visible) &&
			      INTERSECTS(x1, y1, w1, h1, x2, y2, w2, h2))
			    {
			       int                 iw, ih;
			       int                 x0, x00, y0, y00;

			       x0 = x1;
			       if (x1 < x2)
				  x0 = x2;
			       x00 = (x1 + w1);
			       if ((x2 + w2) < (x1 + w1))
				  x00 = (x2 + w2);

			       y0 = y1;
			       if (y1 < y2)
				  y0 = y2;
			       y00 = (y1 + h1);
			       if ((y2 + h2) < (y1 + h1))
				  y00 = (y2 + h2);

			       iw = x00 - x0;
			       ih = y00 - y0;
			       ar += (iw * ih);
			    }
		       }
		     if (ar < area)
		       {
			  area = ar;
			  *x = a_x[i];
			  *y = a_y[j];
			  if (ar == 0)
			     goto done;
		       }
		  }
		if ((a_x[i + 1] - w > 0) && (a_y[j] < (desk->real.h - h)))
		  {
		     int                 ar = 0;

		     for (l = desk->windows; l; l = l->next)
		       {
			  E_Border           *bd;
			  int                 x1, y1, w1, h1, x2, y2, w2, h2;

			  bd = l->data;
			  x1 = a_x[i + 1] - w;
			  y1 = a_y[j];
			  w1 = w;
			  h1 = h;
			  x2 = bd->current.x;
			  y2 = bd->current.y;
			  w2 = bd->current.w;
			  h2 = bd->current.h;
			  if ((bd != b) && (bd->current.visible) &&
			      INTERSECTS(x1, y1, w1, h1, x2, y2, w2, h2))
			    {
			       int                 iw, ih;
			       int                 x0, x00, y0, y00;

			       x0 = x1;
			       if (x1 < x2)
				  x0 = x2;
			       x00 = (x1 + w1);
			       if ((x2 + w2) < (x1 + w1))
				  x00 = (x2 + w2);

			       y0 = y1;
			       if (y1 < y2)
				  y0 = y2;
			       y00 = (y1 + h1);
			       if ((y2 + h2) < (y1 + h1))
				  y00 = (y2 + h2);

			       iw = x00 - x0;
			       ih = y00 - y0;
			       ar += (iw * ih);
			    }
		       }
		     if (ar < area)
		       {
			  area = ar;
			  *x = a_x[i + 1] - w;
			  *y = a_y[j];
			  if (ar == 0)
			     goto done;
		       }
		  }
		if ((a_x[i + 1] - w > 0) && (a_y[j + 1] - h > 0))
		  {
		     int                 ar = 0;

		     for (l = desk->windows; l; l = l->next)
		       {
			  E_Border           *bd;
			  int                 x1, y1, w1, h1, x2, y2, w2, h2;

			  bd = l->data;
			  x1 = a_x[i + 1] - w;
			  y1 = a_y[j + 1] - h;
			  w1 = w;
			  h1 = h;
			  x2 = bd->current.x;
			  y2 = bd->current.y;
			  w2 = bd->current.w;
			  h2 = bd->current.h;
			  if ((bd != b) && (bd->current.visible) &&
			      INTERSECTS(x1, y1, w1, h1, x2, y2, w2, h2))
			    {
			       int                 iw, ih;
			       int                 x0, x00, y0, y00;

			       x0 = x1;
			       if (x1 < x2)
				  x0 = x2;
			       x00 = (x1 + w1);
			       if ((x2 + w2) < (x1 + w1))
				  x00 = (x2 + w2);

			       y0 = y1;
			       if (y1 < y2)
				  y0 = y2;
			       y00 = (y1 + h1);
			       if ((y2 + h2) < (y1 + h1))
				  y00 = (y2 + h2);

			       iw = x00 - x0;
			       ih = y00 - y0;
			       ar += (iw * ih);
			    }
		       }
		     if (ar < area)
		       {
			  area = ar;
			  *x = a_x[i + 1] - w;
			  *y = a_y[j + 1] - h;
			  if (ar == 0)
			     goto done;
		       }
		  }
		if ((a_x[i] < (desk->real.w - w)) && (a_y[j + 1] - h > 0))
		  {
		     int                 ar = 0;

		     for (l = desk->windows; l; l = l->next)
		       {
			  E_Border           *bd;
			  int                 x1, y1, w1, h1, x2, y2, w2, h2;

			  bd = l->data;
			  x1 = a_x[i];
			  y1 = a_y[j + 1] - h;
			  w1 = w;
			  h1 = h;
			  x2 = bd->current.x;
			  y2 = bd->current.y;
			  w2 = bd->current.w;
			  h2 = bd->current.h;
			  if ((bd != b) && (bd->current.visible) &&
			      INTERSECTS(x1, y1, w1, h1, x2, y2, w2, h2))
			    {
			       int                 iw, ih;
			       int                 x0, x00, y0, y00;

			       x0 = x1;
			       if (x1 < x2)
				  x0 = x2;
			       x00 = (x1 + w1);
			       if ((x2 + w2) < (x1 + w1))
				  x00 = (x2 + w2);

			       y0 = y1;
			       if (y1 < y2)
				  y0 = y2;
			       y00 = (y1 + h1);
			       if ((y2 + h2) < (y1 + h1))
				  y00 = (y2 + h2);

			       iw = x00 - x0;
			       ih = y00 - y0;
			       ar += (iw * ih);
			    }
		       }
		     if (ar < area)
		       {
			  area = ar;
			  *x = a_x[i];
			  *y = a_y[j + 1] - h;
			  if (ar == 0)
			     goto done;
		       }
		  }
	     }
	}
   }
 done:
   FREE(a_x);
   FREE(a_y);

   D_RETURN_(1);
}

static int
e_place_middle(E_Border * b, E_Desktop * desk, int *x, int *y)
{
   int                 w, h;

   D_ENTER;

   w = b->current.requested.w;
   h = b->current.requested.h;
   *x = (desk->real.w - w) / 2;
   *y = (desk->real.h - h) / 2;

   D_RETURN_(1);
}

static int
e_place_cascade(E_Border * b, E_Desktop * desk, int *x, int *y)
{
   int                 w, h;
   static int          count_x = 0;
   static int          count_y = 0;
   int                 pl, pr, pt, pb;

   D_ENTER;

   pl = pr = pt = pb = 0;
   if (b->bits.l)
      ebits_get_insets(b->bits.l, &pl, &pr, &pt, &pb);
   w = b->current.requested.w;
   h = b->current.requested.h;
   if ((count_x + w) > desk->real.w)
      count_x = 0;
   if ((count_y + h) > desk->real.h)
      count_y = 0;
   *x = count_x;
   *y = count_y;
   count_x += pl;
   count_y += pt;

   D_RETURN_(1);
}

static int
e_place_random(E_Border * b, E_Desktop * desk, int *x, int *y)
{
   int                 w, h;

   D_ENTER;

   w = b->current.requested.w;
   h = b->current.requested.h;

   if (w < desk->real.w)
      *x = (rand() % (desk->real.w - w));
   else
      *x = 0;

   if (h < desk->real.h)
      *y = (rand() % (desk->real.h - h));
   else
      *y = 0;

   D_RETURN_(1);
}

int
e_place_border(E_Border * b, E_Desktop * desk, int *x, int *y,
	       E_Placement_Mode mode)
{
   int                 result = 1;

   D_ENTER;

   if (b->client.no_place)
      D_RETURN_(1);

   switch (mode)
     {
     case E_PLACE_MANUAL:
	result = e_place_manual(b, desk, x, y);
	break;
     case E_PLACE_SMART:
	result = e_place_smart(b, desk, x, y);
	break;
     case E_PLACE_MIDDLE:
	result = e_place_middle(b, desk, x, y);
	break;
     case E_PLACE_CASCADE:
	result = e_place_cascade(b, desk, x, y);
	break;
     case E_PLACE_RANDOM:
	result = e_place_random(b, desk, x, y);
	break;
     default:
	break;
     }

   D_RETURN_(result);
}

void
e_place_init(void)
{
   D_ENTER;

   ecore_event_filter_handler_add(ECORE_EVENT_MOUSE_DOWN, e_mouse_down);
   ecore_event_filter_handler_add(ECORE_EVENT_MOUSE_UP, e_mouse_up);
   ecore_event_filter_handler_add(ECORE_EVENT_MOUSE_MOVE, e_mouse_move);

   D_RETURN;
}
