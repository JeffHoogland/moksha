#include "debug.h"
#include "scrollbar.h"
#include "config.h"
#include "util.h"
#include "view.h"

static void         e_scrollbar_recalc(E_Scrollbar * sb);
static void         e_scrollbar_setup_bits(E_Scrollbar * sb);
static void         e_sb_base_down_cb(void *data, Ebits_Object o, char *class,
				      int bt, int x, int y, int ox, int oy,
				      int ow, int oh);
static void         e_sb_base_up_cb(void *data, Ebits_Object o, char *class,
				    int bt, int x, int y, int ox, int oy,
				    int ow, int oh);
static void         e_sb_bar_down_cb(void *data, Ebits_Object o, char *class,
				     int bt, int x, int y, int ox, int oy,
				     int ow, int oh);
static void         e_sb_bar_up_cb(void *data, Ebits_Object o, char *class,
				   int bt, int x, int y, int ox, int oy, int ow,
				   int oh);
static void         e_sb_bar_move_cb(void *data, Ebits_Object o, char *class,
				     int bt, int x, int y, int ox, int oy,
				     int ow, int oh);
static void         e_sb_scroll_timer(int val, void *data);
static void         e_scrollbar_cleanup(E_Scrollbar * sb);

static void
e_scrollbar_recalc(E_Scrollbar * sb)
{
   D_ENTER;

   if (sb->base)
     {
	double              x, y, w, h;

	ebits_get_named_bit_geometry(sb->base, "Trough_Area", &x, &y, &w, &h);
	sb->bar_area.x = x + sb->x;
	sb->bar_area.y = y + sb->y;
	sb->bar_area.w = w;
	sb->bar_area.h = h;
	if (sb->direction)
	  {
	     sb->bar_pos.w = sb->bar_area.w;
	     sb->bar_pos.h = (sb->bar_area.h * sb->range) / sb->max;
	     sb->bar_pos.x = sb->bar_area.x;
	     sb->bar_pos.y = sb->bar_area.y +
		((sb->bar_area.h * sb->val) / (sb->max - 1));
	  }
	else
	  {
	     sb->bar_pos.w = (sb->bar_area.w * sb->range) / sb->max;
	     sb->bar_pos.h = sb->bar_area.h;
	     sb->bar_pos.x = sb->bar_area.x +
		((sb->bar_area.w * sb->val) / (sb->max - 1));
	     sb->bar_pos.y = sb->bar_area.y;
	  }
     }
   else
     {
	sb->bar_area.x = sb->x;
	sb->bar_area.y = sb->y;
	sb->bar_area.w = sb->w;
	sb->bar_area.h = sb->h;
	sb->bar_pos.x = sb->bar_area.x;
	sb->bar_pos.y = sb->bar_area.y;
	sb->bar_pos.w = sb->bar_area.w;
	sb->bar_pos.h = sb->bar_area.h;
     }

   D_RETURN;
   UN(sb);
}

static void
e_scrollbar_setup_bits(E_Scrollbar * sb)
{
   char                buf[PATH_MAX];

   D_ENTER;

   if (sb->direction == 1)
     {
	/* load from the current dir's layout */
	snprintf(buf, PATH_MAX, "%s/.e_layout/scroll_base_v.bits.db",
		 sb->view->model->dir);
	sb->base = ebits_load(buf);

	/* if not loaded, load defaults */
	if (!sb->base)
	  {
	     snprintf(buf, PATH_MAX, "%s/scroll_base_v.bits.db",
		      e_config_get("scrollbars"));
	     sb->base = ebits_load(buf);
	  }

	/* load from current dir's layout */
	snprintf(buf, PATH_MAX, "%s/.e_layout/scroll_bar_v.bits.db",
		 sb->view->model->dir);
	sb->bar = ebits_load(buf);

	/* if not loaded, load defaults */
	if (!sb->bar)
	  {
	     snprintf(buf, PATH_MAX, "%s/scroll_bar_v.bits.db",
		      e_config_get("scrollbars"));
	     sb->bar = ebits_load(buf);
	  }
     }
   else
     {
	/* load from the current dir's layout */
	snprintf(buf, PATH_MAX, "%s/.e_layout/scroll_base_h.bits.db",
		 sb->view->model->dir);
	sb->base = ebits_load(buf);

	/* if not loaded, load defaults */
	if (!sb->base)
	  {
	     snprintf(buf, PATH_MAX, "%s/scroll_base_h.bits.db",
		      e_config_get("scrollbars"));
	     sb->base = ebits_load(buf);
	  }

	/* load from current dir's layout */
	snprintf(buf, PATH_MAX, "%s/.e_layout/scroll_bar_h.bits.db",
		 sb->view->model->dir);
	sb->bar = ebits_load(buf);

	/* if not loaded, load defaults */
	if (!sb->bar)
	  {
	     snprintf(buf, PATH_MAX, "%s/scroll_bar_h.bits.db",
		      e_config_get("scrollbars"));
	     sb->bar = ebits_load(buf);
	  }
     }
   if (sb->base)
     {
	int                 w, h;

	ebits_add_to_evas(sb->base, sb->evas);
	ebits_get_min_size(sb->base, &w, &h);
	sb->w = w;
	sb->h = h;

	ebits_set_classed_bit_callback(sb->base, "Scrollbar_Trough",
				       CALLBACK_MOUSE_DOWN, e_sb_base_down_cb,
				       sb);
	ebits_set_classed_bit_callback(sb->base, "Scrollbar_Trough",
				       CALLBACK_MOUSE_UP, e_sb_base_up_cb, sb);
	ebits_set_classed_bit_callback(sb->base, "Scrollbar_Arrow1",
				       CALLBACK_MOUSE_DOWN, e_sb_base_down_cb,
				       sb);
	ebits_set_classed_bit_callback(sb->base, "Scrollbar_Arrow1",
				       CALLBACK_MOUSE_UP, e_sb_base_up_cb, sb);
	ebits_set_classed_bit_callback(sb->base, "Scrollbar_Arrow2",
				       CALLBACK_MOUSE_DOWN, e_sb_base_down_cb,
				       sb);
	ebits_set_classed_bit_callback(sb->base, "Scrollbar_Arrow2",
				       CALLBACK_MOUSE_UP, e_sb_base_up_cb, sb);
     }
   if (sb->bar)
     {
	ebits_add_to_evas(sb->bar, sb->evas);
	ebits_set_classed_bit_callback(sb->bar, "Scrollbar_Bar",
				       CALLBACK_MOUSE_DOWN, e_sb_bar_down_cb,
				       sb);
	ebits_set_classed_bit_callback(sb->bar, "Scrollbar_Bar",
				       CALLBACK_MOUSE_UP, e_sb_bar_up_cb, sb);
	ebits_set_classed_bit_callback(sb->bar, "Scrollbar_Bar",
				       CALLBACK_MOUSE_MOVE, e_sb_bar_move_cb,
				       sb);
     }

   D_RETURN;
}

static void
e_sb_base_down_cb(void *data, Ebits_Object o, char *class, int bt, int x, int y,
		  int ox, int oy, int ow, int oh)
{
   E_Scrollbar        *sb;
   double              prev;
   char                name[PATH_MAX];
   int                 inc;

   D_ENTER;

   sb = data;
   if (sb->mouse_down)
      D_RETURN;
   sb->mouse_down = bt;
   if (!class)
      D_RETURN;
   prev = sb->val;

   D("x %d y %d bt %d\n", x, y, bt);
   D("val %f range %f max %f\n", sb->val, sb->range, sb->max);
   D("sb x %f y %f w %f h %f\n", sb->x, sb->y, sb->w, sb->h);
   D("sb area x %f y %f w %f h %f\n", sb->bar_area.x, sb->bar_area.y,
     sb->bar_area.w, sb->bar_area.h);
   D("sb pos x %f y %f w %f h %f\n", sb->bar_pos.x, sb->bar_pos.y,
     sb->bar_pos.w, sb->bar_pos.h);

   if (!strcmp(class, "Scrollbar_Arrow1"))
     {
	sb->scroll_step = -16;
	sb->scroll_speed = 0.01;

	snprintf(name, PATH_MAX, "scroll_up.%i.%s", sb->direction,
		 sb->view->name);
	ecore_add_event_timer(name, sb->scroll_speed, e_sb_scroll_timer, 0, sb);
     }

   else if (!strcmp(class, "Scrollbar_Arrow2"))
     {
	sb->scroll_step = 16;
	sb->scroll_speed = 0.01;

	snprintf(name, PATH_MAX, "scroll_down.%i.%s", sb->direction,
		 sb->view->name);
	ecore_add_event_timer(name, sb->scroll_speed, e_sb_scroll_timer, 0, sb);
     }

   else if (!strcmp(class, "Scrollbar_Trough"))
     {
	if (bt == 1)
	  {
	     if (sb->direction)	/* vertical */
	       {
		  sb->scroll_step = sb->bar_area.h;
		  inc = y - sb->bar_pos.y;
	       }
	     else		/* horizontal */
	       {
		  sb->scroll_step = sb->bar_area.w;
		  inc = x - sb->bar_pos.x;
	       }

	     if (inc < 0)	/* scroll up (or left) */
	       {
		  sb->scroll_step = -sb->scroll_step;
		  snprintf(name, PATH_MAX, "scroll_up.%i.%s", sb->direction,
			   sb->view->name);
	       }
	     else		/* scroll down (or right) */
	       {
		  snprintf(name, PATH_MAX, "scroll_down.%i.%s", sb->direction,
			   sb->view->name);
	       }

	     /* scroll once in the chosen direction */
	     sb->val += sb->scroll_step;
	     if (sb->val < 0)
		sb->val = 0;
	     if ((sb->val + sb->range) > sb->max)
		sb->val = sb->max - sb->range;

	     /* set a timer to keep on scrolling */
	     sb->scroll_speed = 0.1;
	     ecore_add_event_timer(name, sb->scroll_speed, e_sb_scroll_timer, 0,
				   sb);
	  }

	else
	  {
	     if (sb->direction)
		sb->val =
		   (y - sb->bar_area.y) * sb->max / sb->bar_area.h -
		   sb->bar_area.h / 2;
	     else
		sb->val =
		   (x - sb->bar_area.x) * sb->max / sb->bar_area.w -
		   sb->bar_area.w / 2;

	     if (sb->val < 0)
		sb->val = 0;
	     if ((sb->val + sb->range) > sb->max)
		sb->val = sb->max - sb->range;
	  }
     }

   e_scrollbar_recalc(sb);
   if (sb->bar)
      ebits_move(sb->bar, sb->bar_pos.x, sb->bar_pos.y);
   if (sb->bar)
      ebits_resize(sb->bar, sb->bar_pos.w, sb->bar_pos.h);
   if (prev != sb->val)
     {
	if (sb->func_change)
	   sb->func_change(sb->func_data, sb, sb->val);
     }

   D_RETURN;
   UN(o);
   UN(ox);
   UN(oy);
   UN(ow);
   UN(oh);
}

static void
e_sb_base_up_cb(void *data, Ebits_Object o, char *class, int bt, int x, int y,
		int ox, int oy, int ow, int oh)
{
   E_Scrollbar        *sb;
   char                name[PATH_MAX];

   D_ENTER;

   sb = data;
   if (bt == sb->mouse_down)
      sb->mouse_down = 0;
   else
      D_RETURN;
   if (!class)
      D_RETURN;

   if (!strcmp(class, "Scrollbar_Arrow1"))
     {
	sb->scroll_step = 0;

	snprintf(name, PATH_MAX, "scroll_up.%i.%s", sb->direction,
		 sb->view->name);
	ecore_del_event_timer(name);
     }

   else if (!strcmp(class, "Scrollbar_Arrow2"))
     {
	sb->scroll_step = 0;

	snprintf(name, PATH_MAX, "scroll_down.%i.%s", sb->direction,
		 sb->view->name);
	ecore_del_event_timer(name);
     }

   else if (!strcmp(class, "Scrollbar_Trough"))
     {
	if (sb->scroll_step < 0)
	  {
	     sb->scroll_step = 0;
	     snprintf(name, PATH_MAX, "scroll_up.%i.%s", sb->direction,
		      sb->view->name);
	     ecore_del_event_timer(name);
	  }
	else if (sb->scroll_step > 0)
	  {
	     sb->scroll_step = 0;
	     snprintf(name, PATH_MAX, "scroll_down.%i.%s", sb->direction,
		      sb->view->name);
	     ecore_del_event_timer(name);
	  }
     }

   D_RETURN;
   UN(o);
   UN(x);
   UN(y);
   UN(ox);
   UN(oy);
   UN(ow);
   UN(oh);
}

static void
e_sb_bar_down_cb(void *data, Ebits_Object o, char *class, int bt, int x, int y,
		 int ox, int oy, int ow, int oh)
{
   E_Scrollbar        *sb;

   D_ENTER;

   sb = data;
   if (sb->mouse_down)
      D_RETURN;
   sb->mouse_down = bt;
   sb->down_x = x;
   sb->down_y = y;
   sb->mouse_x = x;
   sb->mouse_y = y;
   sb->view->changed = 1;

   D_RETURN;
   UN(o);
   UN(class);
   UN(x);
   UN(y);
   UN(ox);
   UN(oy);
   UN(ow);
   UN(oh);
}

static void
e_sb_bar_up_cb(void *data, Ebits_Object o, char *class, int bt, int x, int y,
	       int ox, int oy, int ow, int oh)
{
   E_Scrollbar        *sb;

   D_ENTER;

   sb = data;
   if (bt == sb->mouse_down)
      sb->mouse_down = 0;
   else
      D_RETURN;

   sb->view->changed = 1;

   D_RETURN;
   UN(o);
   UN(class);
   UN(x);
   UN(y);
   UN(ox);
   UN(oy);
   UN(ow);
   UN(oh);
}

static void
e_sb_bar_move_cb(void *data, Ebits_Object o, char *class, int bt, int x, int y,
		 int ox, int oy, int ow, int oh)
{
   E_Scrollbar        *sb;
   int                 dx, dy;
   double              prev;

   D_ENTER;
   sb = data;
   if (!sb->mouse_down)
      D_RETURN;
   dx = x - sb->mouse_x;
   dy = y - sb->mouse_y;
   sb->mouse_x = x;
   sb->mouse_y = y;
   prev = sb->val;
   if (sb->direction)
     {
	if (sb->bar_area.h > sb->bar_pos.h)
	   sb->val += ((double)dy * sb->max) / sb->bar_area.h;
	else
	   sb->val = 0;
     }
   else
     {
	if (sb->bar_area.w > sb->bar_pos.w)
	   sb->val += ((double)dx * sb->max) / sb->bar_area.w;
	else
	   sb->val = 0;
     }
   if (sb->val < 0)
      sb->val = 0;
   if ((sb->val + sb->range) > sb->max)
      sb->val = sb->max - sb->range;
   if (prev != sb->val)
     {
	e_scrollbar_recalc(sb);
	if (sb->bar)
	   ebits_move(sb->bar, sb->bar_pos.x, sb->bar_pos.y);
	if (sb->bar)
	   ebits_resize(sb->bar, sb->bar_pos.w, sb->bar_pos.h);
	if (sb->func_change)
	   sb->func_change(sb->func_data, sb, sb->val);
     }

   D_RETURN;
   UN(o);
   UN(class);
   UN(bt);
   UN(x);
   UN(y);
   UN(ox);
   UN(oy);
   UN(ow);
   UN(oh);
}

static void
e_sb_scroll_timer(int val, void *data)
{
   E_Scrollbar        *sb;
   char                name[PATH_MAX];

   D_ENTER;

   sb = data;

   if (sb->scroll_step < 0)
     {
	sb->val += sb->scroll_step;
	if (sb->val < 0)
	   sb->val = 0;

	snprintf(name, PATH_MAX, "scroll_up.%i.%s", sb->direction,
		 sb->view->name);
	ecore_add_event_timer(name, sb->scroll_speed, e_sb_scroll_timer, 0, sb);
     }

   else if (sb->scroll_step > 0)
     {
	sb->val += sb->scroll_step;
	if ((sb->val + sb->range) > sb->max)
	   sb->val = sb->max - sb->range;

	snprintf(name, PATH_MAX, "scroll_down.%i.%s", sb->direction,
		 sb->view->name);
	ecore_add_event_timer(name, sb->scroll_speed, e_sb_scroll_timer, 0, sb);
     }

   e_scrollbar_recalc(sb);
   if (sb->bar)
      ebits_move(sb->bar, sb->bar_pos.x, sb->bar_pos.y);
   if (sb->bar)
      ebits_resize(sb->bar, sb->bar_pos.w, sb->bar_pos.h);
   if (sb->func_change)
      sb->func_change(sb->func_data, sb, sb->val);

   D_RETURN;
   UN(val);
}

E_Scrollbar        *
e_scrollbar_new(E_View * v)
{
   E_Scrollbar        *sb;

   D_ENTER;

   sb = NEW(E_Scrollbar, 1);
   ZERO(sb, E_Scrollbar, 1);

   e_object_init(E_OBJECT(sb), (E_Cleanup_Func) e_scrollbar_cleanup);

   sb->view = v;

   sb->range = 1.0;
   sb->max = 1.0;
   sb->w = 12;
   sb->h = 64;

   D_RETURN_(sb);
}

static void
e_scrollbar_cleanup(E_Scrollbar * sb)
{
   char                name[PATH_MAX];

   D_ENTER;

   if (sb->evas)
     {
	if (sb->base)
	   ebits_free(sb->base);
	if (sb->bar)
	   ebits_free(sb->bar);
     }

   snprintf(name, PATH_MAX, "scroll_up.%i.%s", sb->direction, sb->view->name);
   ecore_del_event_timer(name);
   snprintf(name, PATH_MAX, "scroll_down.%i.%s", sb->direction, sb->view->name);
   ecore_del_event_timer(name);

   e_object_cleanup(E_OBJECT(sb));

   D_RETURN;
}

void
e_scrollbar_add_to_evas(E_Scrollbar * sb, Evas evas)
{
   D_ENTER;

   if (sb->evas)
     {
	if (sb->base)
	   ebits_free(sb->base);
	if (sb->bar)
	   ebits_free(sb->bar);
     }
   sb->evas = evas;
   if (sb->evas)
     {
	e_scrollbar_setup_bits(sb);
	if (sb->base)
	   ebits_set_layer(sb->base, sb->layer);
	if (sb->bar)
	   ebits_set_layer(sb->bar, sb->layer);
	if (sb->base)
	   ebits_move(sb->base, sb->x, sb->y);
	if (sb->base)
	   ebits_resize(sb->base, sb->w, sb->h);
	e_scrollbar_recalc(sb);
	if (sb->bar)
	   ebits_move(sb->bar, sb->bar_pos.x, sb->bar_pos.y);
	if (sb->bar)
	   ebits_resize(sb->bar, sb->bar_pos.w, sb->bar_pos.h);
	if (sb->visible)
	  {
	     if (sb->base)
		ebits_show(sb->base);
	     if (sb->bar)
		ebits_show(sb->bar);
	  }
     }

   D_RETURN;
}

void
e_scrollbar_show(E_Scrollbar * sb)
{
   D_ENTER;

   if (sb->visible)
      D_RETURN;
   sb->visible = 1;
   if (sb->base)
      ebits_show(sb->base);
   if (sb->bar)
      ebits_show(sb->bar);

   D_RETURN;
}

void
e_scrollbar_hide(E_Scrollbar * sb)
{
   D_ENTER;

   if (!sb->visible)
      D_RETURN;
   sb->visible = 0;
   if (sb->base)
      ebits_hide(sb->base);
   if (sb->bar)
      ebits_hide(sb->bar);

   D_RETURN;
}

void
e_scrollbar_raise(E_Scrollbar * sb)
{
   D_ENTER;

   if (sb->base)
      ebits_raise(sb->base);
   if (sb->bar)
      ebits_raise(sb->bar);

   D_RETURN;
}

void
e_scrollbar_lower(E_Scrollbar * sb)
{
   D_ENTER;

   if (sb->bar)
      ebits_lower(sb->bar);
   if (sb->base)
      ebits_lower(sb->base);

   D_RETURN;
}

void
e_scrollbar_set_layer(E_Scrollbar * sb, int l)
{
   D_ENTER;

   if (l == sb->layer)
      D_RETURN;
   sb->layer = l;
   if (sb->base)
      ebits_set_layer(sb->base, sb->layer);
   if (sb->bar)
      ebits_set_layer(sb->bar, sb->layer);

   D_RETURN;
}

void
e_scrollbar_set_direction(E_Scrollbar * sb, int d)
{
   D_ENTER;

   if (d == sb->direction)
      D_RETURN;
   sb->direction = d;
   if (sb->evas)
     {
	Evas                evas;

	if (sb->base)
	   ebits_free(sb->base);
	if (sb->bar)
	   ebits_free(sb->bar);
	evas = sb->evas;
	sb->evas = NULL;
	e_scrollbar_add_to_evas(sb, evas);
     }

   D_RETURN;
}

void
e_scrollbar_move(E_Scrollbar * sb, double x, double y)
{
   D_ENTER;

   if ((x == sb->x) && (y == sb->y))
      D_RETURN;
   sb->x = x;
   sb->y = y;
   if (sb->base)
      ebits_move(sb->base, sb->x, sb->y);
   e_scrollbar_recalc(sb);
   if (sb->bar)
      ebits_move(sb->bar, sb->bar_pos.x, sb->bar_pos.y);

   D_RETURN;
}

void
e_scrollbar_resize(E_Scrollbar * sb, double w, double h)
{
   D_ENTER;

   if ((w == sb->w) && (h == sb->h))
      D_RETURN;
   sb->w = w;
   sb->h = h;
   if (sb->base)
      ebits_resize(sb->base, sb->w, sb->h);
   e_scrollbar_recalc(sb);
   if (sb->bar)
      ebits_move(sb->bar, sb->bar_pos.x, sb->bar_pos.y);
   if (sb->bar)
      ebits_resize(sb->bar, sb->bar_pos.w, sb->bar_pos.h);

   D_RETURN;
}

void
e_scrollbar_set_change_func(E_Scrollbar * sb,
			    void (*func_change) (void *_data, E_Scrollbar * sb,
						 double val), void *data)
{
   D_ENTER;

   sb->func_change = func_change;
   sb->func_data = data;

   D_RETURN;
}

void
e_scrollbar_set_value(E_Scrollbar * sb, double val)
{
   D_ENTER;

   if (val > sb->max - sb->range)
      val = sb->max - sb->range;
   if (val < 0)
      val = 0;
   if (sb->val == val)
      D_RETURN;
   sb->val = val;
   e_scrollbar_recalc(sb);
   if (sb->bar)
      ebits_move(sb->bar, sb->bar_pos.x, sb->bar_pos.y);
   if (sb->bar)
      ebits_resize(sb->bar, sb->bar_pos.w, sb->bar_pos.h);
   if (sb->func_change)
      sb->func_change(sb->func_data, sb, sb->val);

   D_RETURN;
}

void
e_scrollbar_set_range(E_Scrollbar * sb, double range)
{
   D_ENTER;

   if (sb->range == range)
      D_RETURN;
   sb->range = range;
   e_scrollbar_recalc(sb);
   if (sb->bar)
      ebits_move(sb->bar, sb->bar_pos.x, sb->bar_pos.y);
   if (sb->bar)
      ebits_resize(sb->bar, sb->bar_pos.w, sb->bar_pos.h);

   D_RETURN;
}

void
e_scrollbar_set_max(E_Scrollbar * sb, double max)
{
   D_ENTER;

   if (sb->max == max)
      D_RETURN;
   sb->max = max;
   e_scrollbar_recalc(sb);
   if (sb->bar)
      ebits_move(sb->bar, sb->bar_pos.x, sb->bar_pos.y);
   if (sb->bar)
      ebits_resize(sb->bar, sb->bar_pos.w, sb->bar_pos.h);

   D_RETURN;
}

double
e_scrollbar_get_value(E_Scrollbar * sb)
{
   D_ENTER;

   D_RETURN_(sb->val);
}

double
e_scrollbar_get_range(E_Scrollbar * sb)
{
   D_ENTER;

   D_RETURN_(sb->range);
}

double
e_scrollbar_get_max(E_Scrollbar * sb)
{
   D_ENTER;

   D_RETURN_(sb->max);
}

void
e_scrollbar_get_geometry(E_Scrollbar * sb, double *x, double *y, double *w,
			 double *h)
{
   D_ENTER;

   if (x)
      *x = sb->x;
   if (y)
      *y = sb->y;
   if (w)
      *w = sb->w;
   if (h)
      *h = sb->h;

   D_RETURN;
}
