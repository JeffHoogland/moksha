#include <libefsd.h>

#include "debug.h"
#include "view.h"
#include "cursors.h"
#include "config.h"
#include "border.h"
#include "menu.h"
#include "menubuild.h"
#include "fs.h"
#include "file.h"
#include "util.h"
#include "icons.h"
#include "e_dir.h"
#include "e_view_machine.h"
#include "e_file.h"
#include "globals.h"
#include <Ebg.h>

static Ecore_Event *current_ev = NULL;

static char       **dnd_files = NULL;
static int          dnd_num_files = 0;
static E_dnd_enum   dnd_pending_mode;
static E_View      *v_dnd_source;

static void         e_bg_down_cb(void *_data, Evas * _e, Evas_Object * _o,
				 void *event_info);
static void         e_bg_up_cb(void *_data, Evas * _e, Evas_Object * _o,
			       void *event_info);
static void         e_bg_move_cb(void *_data, Evas * _e, Evas_Object * _o,
				 void *event_info);
static void         e_idle(void *data);
static void         e_wheel(Ecore_Event * ev);
static void         e_key_down(Ecore_Event * ev);
static void         e_key_up(Ecore_Event * ev);
static void         e_mouse_down(Ecore_Event * ev);
static void         e_mouse_up(Ecore_Event * ev);
static void         e_mouse_move(Ecore_Event * ev);
static void         e_mouse_in(Ecore_Event * ev);
static void         e_mouse_out(Ecore_Event * ev);
static void         e_window_expose(Ecore_Event * ev);
static void         e_configure(Ecore_Event * ev);
static void         e_property(Ecore_Event * ev);
static void         e_unmap(Ecore_Event * ev);
static void         e_visibility(Ecore_Event * ev);
static void         e_focus_in(Ecore_Event * ev);
static void         e_focus_out(Ecore_Event * ev);
static void         e_delete(Ecore_Event * ev);
static void         e_dnd_status(Ecore_Event * ev);
static void         e_dnd_data_request(Ecore_Event * ev);
static void         e_dnd_drop_end(Ecore_Event * ev);
static void         e_dnd_drop_position(Ecore_Event * ev);
static void         e_dnd_drop(Ecore_Event * ev);
static void         e_dnd_drop_request(Ecore_Event * ev);
static void         e_dnd_drop_request_free(void);
static void         e_dnd_handle_drop(E_View * v);
static void         e_view_resort_timeout(int val, void *data);
static int          e_view_restart_alphabetical_qsort_cb(const void *data1,
							 const void *data2);
static void         e_view_geometry_record_timeout(int val, void *data);
static void         e_view_scrollbar_v_change_cb(void *_data, E_Scrollbar * sb,
						 double val);
static void         e_view_scrollbar_h_change_cb(void *_data, E_Scrollbar * sb,
						 double val);
static void         e_view_write_icon_xy_timeout(int val, void *data);

static void
e_view_write_icon_xy_timeout(int val, void *data)
{
   E_View             *v;
   Evas_List *           l;
   E_Icon             *ic;
   double              t_in;

   D_ENTER;

   v = data;
   /* FIXME: this is a problem if we have 1000's of icons */
   t_in = ecore_get_time();
   for (l = v->icons; l; l = l->next)
     {
	ic = l->data;
	if (ic->q.write_xy)
	  {
	     char                buf[PATH_MAX];

	     ic->q.write_xy = 0;
	     /* FIXME */
	     snprintf(buf, PATH_MAX, "%s/%s", ic->view->dir->dir,
		      ic->file->file);

	     D("write meta xy for icon for file %s\n", ic->file->file);
	     efsd_set_metadata_int(e_fs_get_connection(),
				   "/pos/x", buf, ic->geom.x);
	     efsd_set_metadata_int(e_fs_get_connection(),
				   "/pos/y", buf, ic->geom.y);
	  }
	if ((ecore_get_time() - t_in) > 0.10)
	  {
	     char                name[PATH_MAX];

	     snprintf(name, PATH_MAX, "icon_xy_record.%s", v->dir->dir);
	     ecore_add_event_timer(name, 0.01, e_view_write_icon_xy_timeout, 0,
				   v);
	     D_RETURN;
	  }
     }

   D_RETURN;
   UN(val);
}

void
e_view_selection_update(E_View * v)
{
   D_ENTER;

   if ((v->select.on) && (!v->select.obj.middle))
     {
	/* create select objects */
	v->select.obj.middle = evas_object_rectangle_add(v->evas);
	evas_object_color_set(v->select.obj.middle,
		       v->select.config.middle.r,
		       v->select.config.middle.g,
		       v->select.config.middle.b, v->select.config.middle.a);
	evas_object_layer_set(v->select.obj.middle, 300);
	v->select.obj.edge_l = evas_object_rectangle_add(v->evas);
	evas_object_color_set(v->select.obj.edge_l,
		       v->select.config.edge_l.r,
		       v->select.config.edge_l.g,
		       v->select.config.edge_l.b, v->select.config.edge_l.a);
	evas_object_layer_set(v->select.obj.edge_l, 300);
	v->select.obj.edge_r = evas_object_rectangle_add(v->evas);
	evas_object_color_set(v->select.obj.edge_r,
		       v->select.config.edge_r.r,
		       v->select.config.edge_r.g,
		       v->select.config.edge_r.b, v->select.config.edge_r.a);
	evas_object_layer_set(v->select.obj.edge_r, 300);
	v->select.obj.edge_t = evas_object_rectangle_add(v->evas);
	evas_object_color_set(v->select.obj.edge_t,
		       v->select.config.edge_t.r,
		       v->select.config.edge_t.g,
		       v->select.config.edge_t.b, v->select.config.edge_t.a);
	evas_object_layer_set(v->select.obj.edge_t, 300);
	v->select.obj.edge_b = evas_object_rectangle_add(v->evas);
	evas_object_color_set(v->select.obj.edge_b,
		       v->select.config.edge_b.r,
		       v->select.config.edge_b.g,
		       v->select.config.edge_b.b, v->select.config.edge_b.a);
	evas_object_layer_set(v->select.obj.edge_b, 300);

	v->select.obj.grad_l = evas_object_gradient_add(v->evas);
	evas_object_gradient_angle_set(v->select.obj.grad_l, 270);
	evas_object_gradient_color_add(v->select.obj.grad_l,
				v->select.config.grad_l.r,
				v->select.config.grad_l.g,
				v->select.config.grad_l.b,
				v->select.config.grad_l.a, 8);
	evas_object_gradient_color_add(v->select.obj.grad_l,
				v->select.config.grad_l.r,
				v->select.config.grad_l.g,
				v->select.config.grad_l.b, 0, 8);
	evas_object_layer_set(v->select.obj.grad_l, 300);
	v->select.obj.grad_r = evas_object_gradient_add(v->evas);
	evas_object_gradient_angle_set(v->select.obj.grad_r, 90);
	evas_object_gradient_color_add(v->select.obj.grad_r,
				v->select.config.grad_r.r,
				v->select.config.grad_r.g,
				v->select.config.grad_r.b,
				v->select.config.grad_r.a, 8);
	evas_object_gradient_color_add(v->select.obj.grad_r,
				v->select.config.grad_r.r,
				v->select.config.grad_r.g,
				v->select.config.grad_r.b, 0, 8);
	evas_object_layer_set(v->select.obj.grad_r, 300);
	v->select.obj.grad_t = evas_object_gradient_add(v->evas);
	evas_object_gradient_angle_set(v->select.obj.grad_t, 0);
	evas_object_gradient_color_add(v->select.obj.grad_t,
				v->select.config.grad_t.r,
				v->select.config.grad_t.g,
				v->select.config.grad_t.b,
				v->select.config.grad_t.a, 8);
	evas_object_gradient_color_add(v->select.obj.grad_t,
				v->select.config.grad_t.r,
				v->select.config.grad_t.g,
				v->select.config.grad_t.b, 0, 8);
	evas_object_layer_set(v->select.obj.grad_t, 300);
	v->select.obj.grad_b = evas_object_gradient_add(v->evas);
	evas_object_gradient_angle_set(v->select.obj.grad_b, 180);
	evas_object_gradient_color_add(v->select.obj.grad_b,
				v->select.config.grad_b.r,
				v->select.config.grad_b.g,
				v->select.config.grad_b.b,
				v->select.config.grad_b.a, 8);
	evas_object_gradient_color_add(v->select.obj.grad_b,
				v->select.config.grad_b.r,
				v->select.config.grad_b.g,
				v->select.config.grad_b.b, 0, 8);
	evas_object_layer_set(v->select.obj.grad_b, 300);
	v->select.obj.clip = evas_object_rectangle_add(v->evas);
	evas_object_color_set(v->select.obj.clip, 255, 255, 255, 255);
	evas_object_clip_set(v->select.obj.grad_l, v->select.obj.clip);
	evas_object_clip_set(v->select.obj.grad_r, v->select.obj.clip);
	evas_object_clip_set(v->select.obj.grad_t, v->select.obj.clip);
	evas_object_clip_set(v->select.obj.grad_b, v->select.obj.clip);
     }
   if ((!v->select.on) && (v->select.obj.middle))
     {
	/* destroy select objects */
	evas_object_del(v->select.obj.middle);
	evas_object_del(v->select.obj.edge_l);
	evas_object_del(v->select.obj.edge_r);
	evas_object_del(v->select.obj.edge_t);
	evas_object_del(v->select.obj.edge_b);
	evas_object_del(v->select.obj.grad_l);
	evas_object_del(v->select.obj.grad_r);
	evas_object_del(v->select.obj.grad_t);
	evas_object_del(v->select.obj.grad_b);
	evas_object_del(v->select.obj.clip);
	v->select.obj.middle = NULL;
	v->changed = 1;
	D_RETURN;
     }
   if (!v->select.on)
      D_RETURN;
   /* move & resize select objects */
   {
      evas_object_move(v->select.obj.edge_l, v->select.x, v->select.y + 1);
      evas_object_resize(v->select.obj.edge_l, 1, v->select.h - 1);
      evas_object_move(v->select.obj.edge_r, v->select.x + v->select.w - 1,
		v->select.y);
      evas_object_resize(v->select.obj.edge_r, 1, v->select.h - 1);
      evas_object_move(v->select.obj.edge_t, v->select.x, v->select.y);
      evas_object_resize(v->select.obj.edge_t, v->select.w - 1, 1);
      evas_object_move(v->select.obj.edge_b, v->select.x + 1,
		v->select.y + v->select.h - 1);
      evas_object_resize(v->select.obj.edge_b, v->select.w - 1, 1);
      evas_object_move(v->select.obj.middle, v->select.x + 1,
		v->select.y + 1);
      evas_object_resize(v->select.obj.middle, v->select.w - 1 - 1,
		  v->select.h - 1 - 1);
      evas_object_move(v->select.obj.grad_l, v->select.x + 1,
		v->select.y + 1);
      evas_object_resize(v->select.obj.grad_l, v->select.config.grad_size.l,
		  v->select.h - 1 - 1);
      evas_object_move(v->select.obj.grad_r,
		v->select.x + v->select.w - 1 - v->select.config.grad_size.r,
		v->select.y + 1);
      evas_object_resize(v->select.obj.grad_r, v->select.config.grad_size.r,
		  v->select.h - 1 - 1);
      evas_object_move(v->select.obj.grad_t, v->select.x + 1,
		v->select.y + 1);
      evas_object_resize(v->select.obj.grad_t, v->select.w - 1 - 1,
		  v->select.config.grad_size.t);
      evas_object_move(v->select.obj.grad_b, v->select.x + 1,
		v->select.y + v->select.h - 1 - v->select.config.grad_size.b);
      evas_object_resize(v->select.obj.grad_b, v->select.w - 1 - 1,
		  v->select.config.grad_size.b);
      evas_object_move(v->select.obj.clip, v->select.x + 1, v->select.y + 1);
      evas_object_resize(v->select.obj.clip, v->select.w - 1 - 1,
		  v->select.h - 1 - 1);
   }

   evas_object_show(v->select.obj.middle);
   evas_object_show(v->select.obj.edge_l);
   evas_object_show(v->select.obj.edge_r);
   evas_object_show(v->select.obj.edge_t);
   evas_object_show(v->select.obj.edge_b);
   evas_object_show(v->select.obj.grad_l);
   evas_object_show(v->select.obj.grad_r);
   evas_object_show(v->select.obj.grad_t);
   evas_object_show(v->select.obj.grad_b);
   evas_object_show(v->select.obj.clip);
   v->changed = 1;
   D_RETURN;
}

static void
e_view_scrollbar_v_change_cb(void *_data, E_Scrollbar * sb, double val)
{
   E_View             *v;

   D_ENTER;

   v = _data;
   e_view_scroll_to(v, v->scroll.x, v->spacing.window.t - sb->val);

   D_RETURN;
   UN(val);
}

static void
e_view_scrollbar_h_change_cb(void *_data, E_Scrollbar * sb, double val)
{
   E_View             *v;

   D_ENTER;

   v = _data;
   e_view_scroll_to(v, v->spacing.window.l - sb->val, v->scroll.y);

   D_RETURN;
   UN(val);
}

static void
e_bg_down_cb(void *_data, Evas * _e, Evas_Object * _o, void *event_info)
{
   Ecore_Event_Mouse_Down *ev;
   E_View             *v;
   Evas_Event_Mouse_Down *ev_info = event_info;

   D_ENTER;

   if (!current_ev)
      D_RETURN;
   ev = current_ev->event;
   v = _data;

   if (!(ev->mods & (multi_select_mod | range_select_mod)))
     {
	v->select.last_count = v->select.count;
	e_view_deselect_all(v);
     }

   if (ev_info->button == 1)
     {
	v->select.down.x = ev_info->output.x;
	v->select.down.y = ev_info->output.y;
	v->select.on = 1;
	if (ev_info->output.x < v->select.down.x)
	  {
	     v->select.x = ev_info->output.x;
	     v->select.w = v->select.down.x - v->select.x + 1;
	  }
	else
	  {
	     v->select.x = v->select.down.x;
	     v->select.w = ev_info->output.x - v->select.down.x + 1;
	  }
	if (ev_info->output.y < v->select.down.y)
	  {
	     v->select.y = ev_info->output.y;
	     v->select.h = v->select.down.y - v->select.y + 1;
	  }
	else
	  {
	     v->select.y = v->select.down.y;
	     v->select.h = ev_info->output.y - v->select.down.y + 1;
	  }
	e_view_selection_update(v);
     }

   if (ev_info->button == 2 && ev->double_click)
      ecore_event_loop_quit();

   D_RETURN;
   UN(_e);
   UN(_o);
}

static void
e_bg_up_cb(void *_data, Evas * _e, Evas_Object * _o, void *event_info)
{
   Ecore_Event_Mouse_Up *ev;
   E_View             *v;
   int                 dx, dy;
   Evas_Event_Mouse_Up *ev_info = event_info;

   D_ENTER;

   if (!current_ev)
      D_RETURN;
   ev = current_ev->event;
   v = _data;
   dx = 0;
   dy = 0;

   if (v->select.on)
     {
	dx = v->select.down.x - ev_info->output.x;
	dy = v->select.down.y - ev_info->output.y;
	if (dx < 0)
	   dx = -dx;
	if (dy < 0)
	   dy = -dy;
	if (ev_info->button == 1)
	   v->select.on = 0;
	e_view_selection_update(v);
     }

   if ((ev_info->button == 1) && ((dx > 3) || (dy > 3)))
     {
	Evas_List *           l;

	for (l = v->icons; l; l = l->next)
	  {
	     E_Icon             *ic;

	     ic = l->data;
	     if (INTERSECTS(v->select.x, v->select.y, v->select.w, v->select.h,
			    v->scroll.x + ic->geom.x,
			    v->scroll.y + ic->geom.y, ic->geom.w, ic->geom.h))
	       {
		  if (ic->state.visible)
		    {
		       e_icon_invert_selection(ic);
		    }
	       }
	  }
     }
   else if (v->select.last_count == 0)
     {
	if (ev_info->button == 1)
	  {
	     if (!(ev->mods & (multi_select_mod | range_select_mod)))
	       {
		  static E_Build_Menu *buildmenu = NULL;

		  if (!buildmenu)
		    {
		       char               *apps_menu_db;

		       apps_menu_db = e_config_get("apps_menu");
		       if (apps_menu_db)
			  buildmenu = e_build_menu_new_from_db(apps_menu_db);
		    }
		  if (buildmenu)
		    {
		       static E_Menu      *menu = NULL;

		       menu = buildmenu->menu;
		       if (menu)
			 {
			    e_menu_show_at_mouse(menu, ev->rx, ev->ry,
						 ev->time);
			 }
		    }
	       }
	  }
	else if (ev_info->button == 2)
	  {
#if 1
	     static E_Build_Menu *buildmenu = NULL;

	     if (!buildmenu)
	       {
		  D("building iconified windows menu\n");
		  buildmenu = e_build_menu_new_from_iconified_borders();
	       }
	     if (buildmenu && buildmenu->changed)
	       {
		  D("buildmenu changed! rebuild!\n");
		  e_build_menu_iconified_borders_rebuild(buildmenu);
	       }
	     if (buildmenu)
	       {
		  static E_Menu      *menu = NULL;

		  menu = buildmenu->menu;

		  if (menu)
		    {
		       D("showing iconified window menu\n");
		       e_menu_show_at_mouse(menu, ev->rx, ev->ry, ev->time);
		    }
	       }
#endif
	  }
	else if (ev_info->button == 3)
	  {
	     static E_Build_Menu *buildmenu = NULL;

	     if (!buildmenu)
	       {
		  buildmenu =
		     e_build_menu_new_from_gnome_apps("/usr/share/gnome/apps");
	       }
	     if (buildmenu)
	       {
		  static E_Menu      *menu = NULL;

		  menu = buildmenu->menu;
		  if (menu)
		     e_menu_show_at_mouse(menu, ev->rx, ev->ry, ev->time);
	       }
	  }
     }
   if (ev_info->button == 1)
     {
	v->select.x = ev_info->output.x;
	v->select.y = ev_info->output.y;
     }

   D_RETURN;
   UN(_e);
   UN(_o);
}

static void
e_bg_move_cb(void *_data, Evas * _e, Evas_Object * _o, void *event_info)
{
   Ecore_Event_Mouse_Down *ev;
   E_View             *v;
   Evas_Event_Mouse_Move *ev_info = event_info;

   D_ENTER;

   if (!current_ev)
      D_RETURN;
   ev = current_ev->event;
   v = _data;
   if (v->select.on)
     {
	if (ev_info->cur.output.x < v->select.down.x)
	  {
	     v->select.x = ev_info->cur.output.x;
	     v->select.w = v->select.down.x - v->select.x + 1;
	  }
	else
	  {
	     v->select.x = v->select.down.x;
	     v->select.w = ev_info->cur.output.x - v->select.down.x + 1;
	  }
	if (ev_info->cur.output.y < v->select.down.y)
	  {
	     v->select.y = ev_info->cur.output.y;
	     v->select.h = v->select.down.y - v->select.y + 1;
	  }
	else
	  {
	     v->select.y = v->select.down.y;
	     v->select.h = ev_info->cur.output.y - v->select.down.y + 1;
	  }
	e_view_selection_update(v);

     }

   D_RETURN;
   UN(_e);
   UN(_o);
}

void
e_view_deselect_all(E_View * v)
{
   Evas_List *           l;

   D_ENTER;

   for (l = v->icons; l; l = l->next)
     {
	E_Icon             *ic;

	ic = l->data;
	e_icon_deselect(ic);
     }

   D_RETURN;
}

void
e_view_deselect_all_except(E_Icon * not_ic)
{
   Evas_List *           l;

   D_ENTER;

   for (l = not_ic->view->icons; l; l = l->next)
     {
	E_Icon             *ic;

	ic = l->data;
	if (ic != not_ic)
	   e_icon_deselect(ic);
     }

   D_RETURN;
}

void
e_view_icons_get_extents(E_View * v, int *min_x, int *min_y, int *max_x,
			 int *max_y)
{
   Evas_List *           l;
   int                 x1, x2, y1, y2;

   D_ENTER;

   x1 = v->extents.x1;
   x2 = v->extents.x2;
   y1 = v->extents.y1;
   y2 = v->extents.y2;
   if (!v->extents.valid)
     {
	x1 = 999999999;
	x2 = -999999999;
	y1 = 999999999;
	y2 = -999999999;
	if (!v->icons)
	  {
	     if (min_x)
		*min_x = 0;
	     if (min_y)
		*min_y = 0;
	     if (max_x)
		*max_x = 1;
	     if (max_y)
		*max_y = 1;
	     D_RETURN;
	  }
	for (l = v->icons; l; l = l->next)
	  {
	     E_Icon             *ic;

	     ic = l->data;
	     if (ic->geom.x < x1)
		x1 = ic->geom.x;
	     if (ic->geom.y < y1)
		y1 = ic->geom.y;
	     if (ic->geom.x + ic->geom.w > x2)
		x2 = ic->geom.x + ic->geom.w;
	     if (ic->geom.y + ic->geom.h > y2)
		y2 = ic->geom.y + ic->geom.h;
	  }
	v->extents.x1 = x1;
	v->extents.y1 = y1;
	v->extents.x2 = x2;
	v->extents.y2 = y2;
     }
   v->extents.valid = 1;
   if (x1 > 0)
      x1 = 0;
   if (y1 > 0)
      y1 = 0;
   if (min_x)
      *min_x = x1;
   if (min_y)
      *min_y = y1;
   if (max_x)
      *max_x = x2 - 1;
   if (max_y)
      *max_y = y2 - 1;

   D_RETURN;
}

void
e_view_icons_apply_xy(E_View * v)
{
   Evas_List *           l;

   D_ENTER;

   for (l = v->icons; l; l = l->next)
     {
	E_Icon             *ic;

	ic = l->data;
	e_icon_apply_xy(ic);
     }

   v->changed = 1;

   D_RETURN;
}

void
e_view_scroll_to(E_View * v, int sx, int sy)
{
   int                 min_x, min_y, max_x, max_y;

   D_ENTER;

   e_view_icons_get_extents(v, &min_x, &min_y, &max_x, &max_y);

   if (sx < v->size.w - v->spacing.window.r - max_x)
      sx = v->size.w - v->spacing.window.r - max_x;
   if (sx > v->spacing.window.l - min_x)
      sx = v->spacing.window.l - min_x;
   if (sy < v->size.h - v->spacing.window.b - max_y)
      sy = v->size.h - v->spacing.window.b - max_y;
   if (sy > v->spacing.window.t - min_y)
      sy = v->spacing.window.t - min_y;

   if ((sx == v->scroll.x) && (v->scroll.y == sy))
      D_RETURN;
   v->scroll.x = sx;
   v->scroll.y = sy;
   e_view_icons_apply_xy(v);
   if (v->bg)
      e_bg_set_scroll(v->bg, v->scroll.x, v->scroll.y);
   v->changed = 1;

   D_RETURN;
}

void
e_view_scroll_by(E_View * v, int sx, int sy)
{
   D_ENTER;

   e_view_scroll_to(v, v->scroll.x + sx, v->scroll.y + sy);

   D_RETURN;
}

void
e_view_scroll_to_percent(E_View * v, double psx, double psy)
{
   int                 min_x, min_y, max_x, max_y;
   int                 sx, sy;

   D_ENTER;

   e_view_icons_get_extents(v, &min_x, &min_y, &max_x, &max_y);
   sx = (psx * ((double)max_x - (double)min_x)) - min_x;
   sy = (psy * ((double)max_y - (double)min_y)) - min_y;

   if (sx < v->size.w - v->spacing.window.r - max_x)
      sx = v->size.w - v->spacing.window.r - max_x;
   if (sx > v->spacing.window.l - min_x)
      sx = v->spacing.window.l - min_x;
   if (sy < v->size.h - v->spacing.window.b - max_y)
      sy = v->size.h - v->spacing.window.b - max_y;
   if (sy > v->spacing.window.t - min_y)
      sy = v->spacing.window.t - min_y;

   if ((sx == v->scroll.x) && (v->scroll.y == sy))
      D_RETURN;
   v->scroll.x = sx;
   v->scroll.y = sy;
   e_view_icons_apply_xy(v);
   if (v->bg)
      e_bg_set_scroll(v->bg, v->scroll.x, v->scroll.y);

   D_RETURN;
}

void
e_view_get_viewable_percentage(E_View * v, double *vw, double *vh)
{
   int                 min_x, min_y, max_x, max_y;
   double              p;

   D_ENTER;

   e_view_icons_get_extents(v, &min_x, &min_y, &max_x, &max_y);
   if (min_x == max_x)
     {
	if (vw)
	   *vw = 0;
     }
   else
     {
	p = ((double)(v->size.w - v->spacing.window.l - v->spacing.window.r)) /
	   ((double)(max_x - min_x));
	if (vw)
	   *vw = p;
     }
   if (min_y == max_y)
     {
	if (vh)
	   *vh = 0;
     }
   else
     {
	p = ((double)(v->size.h - v->spacing.window.t - v->spacing.window.b)) /
	   ((double)(max_y - min_y));
	if (vh)
	   *vh = p;
     }

   D_RETURN;
}

void
e_view_get_position_percentage(E_View * v, double *vx, double *vy)
{
   int                 min_x, min_y, max_x, max_y;
   double              p;

   D_ENTER;

   e_view_icons_get_extents(v, &min_x, &min_y, &max_x, &max_y);
   if (min_x == max_x)
     {
	if (vx)
	   *vx = 0;
     }
   else
     {
	p = ((double)(v->scroll.x - min_x)) / ((double)(max_x - min_x));
	if (vx)
	   *vx = p;
     }
   if (min_y == max_y)
     {
	if (vy)
	   *vy = 0;
     }
   else
     {
	p = ((double)(v->scroll.y - min_y)) / ((double)(max_y - min_y));
	if (vy)
	   *vy = p;
     }

   D_RETURN;
}

static void
e_idle(void *data)
{
   Evas_List *           l;

   D_ENTER;

   for (l = VM->views; l; l = l->next)
     {
	E_View             *v;

	v = l->data;
	e_view_update(v);
     }

   D_RETURN;
   UN(data);
}

void
e_view_geometry_record(E_View * v)
{
   D_ENTER;

   if (e_fs_get_connection())
     {
	int                 left, top;

	D("Record geom for view\n");
	ecore_window_get_frame_size(v->win.base, &left, NULL, &top, NULL);
	efsd_set_metadata_int(e_fs_get_connection(),
			      "/view/x", v->dir->dir, v->location.x - left);
	efsd_set_metadata_int(e_fs_get_connection(),
			      "/view/y", v->dir->dir, v->location.y - top);
	efsd_set_metadata_int(e_fs_get_connection(),
			      "/view/w", v->dir->dir, v->size.w);
	efsd_set_metadata_int(e_fs_get_connection(),
			      "/view/h", v->dir->dir, v->size.h);
     }

   D_RETURN;
}

static void
e_view_geometry_record_timeout(int val, void *data)
{
   E_View             *v;

   D_ENTER;

   v = data;
   e_view_geometry_record(v);

   D_RETURN;
   UN(val);
}

void
e_view_queue_geometry_record(E_View * v)
{
   char                name[PATH_MAX];

   D_ENTER;

   snprintf(name, PATH_MAX, "geometry_record.%s", v->dir->dir);
   ecore_add_event_timer(name, 0.10, e_view_geometry_record_timeout, 0, v);

   D_RETURN;
}

void
e_view_queue_icon_xy_record(E_View * v)
{
   char                name[PATH_MAX];

   D_ENTER;

   snprintf(name, PATH_MAX, "icon_xy_record.%s", v->dir->dir);
   ecore_add_event_timer(name, 0.10, e_view_write_icon_xy_timeout, 0, v);

   D_RETURN;
}

static void
e_configure(Ecore_Event * ev)
{
   Ecore_Event_Window_Configure *e;
   E_View             *v;

   D_ENTER;

   e = ev->event;
   if (!e || !e->win)
      D_RETURN;
   v = e_view_machine_get_view_by_base_window(e->win);
   if (!v)
      D_RETURN;
   /* win, root, x, y, w, h, wm_generated */
   D("Configure for view: %s\n", v->name);
   if (e->wm_generated)
   {
      D("wm generated %i %i, %ix%i\n", e->x, e->y, e->w, e->h);
      if ((e->x != v->location.x) || (e->y != v->location.y))
      {
	 D("new spot!\n");
	 v->location.x = e->x;
	 v->location.y = e->y;   
      }
   }
   D("size %ix%i\n", e->w, e->h);
   if ((e->w != v->size.w) || (e->h != v->size.h) || (v->size.force))
   {
      v->size.force = 0;
      D("... a new size!\n");
      v->size.w = e->w;
      v->size.h = e->h;
      if (v->pmap)
	 ecore_pixmap_free(v->pmap);
      v->pmap = 0;
      ecore_window_resize(v->win.main, v->size.w, v->size.h);
      if (v->options.back_pixmap)
      {
	 Evas_Engine_Info_Software_X11 *info;

	 v->pmap =
	    ecore_pixmap_new(v->win.main, v->size.w, v->size.h,
		  0);
	 info = (Evas_Engine_Info_Software_X11 *)evas_engine_info_get(v->evas);
	 info->info.drawable = v->pmap;
	 evas_engine_info_set(v->evas, (Evas_Engine_Info *)info);
	 ecore_window_set_background_pixmap(v->win.main, v->pmap);
	 ecore_window_clear(v->win.main);
      }
      if (v->bg)
	 e_bg_resize(v->bg, v->size.w, v->size.h);
      D("evas_set_output_viewpor(%p)\n", v->evas);
      evas_output_viewport_set(v->evas, 0, 0, v->size.w, v->size.h);
      evas_output_size_set(v->evas, v->size.w, v->size.h);
      e_view_scroll_to(v, v->scroll.x, v->scroll.y);
      e_view_arrange(v);	 
      e_view_layout_update(v->layout);
   }
   D_RETURN;
}

static void
e_property(Ecore_Event * ev)
{
   Ecore_Event_Window_Configure *e;
   E_View             *v;

   D_ENTER;

   e = ev->event;
   if (e && e->win)
     {
	if ((v = e_view_machine_get_view_by_base_window(e->win)))
	  {
	  }
     }

   D_RETURN;
}

static void
e_unmap(Ecore_Event * ev)
{
   Ecore_Event_Window_Unmap *e;
   E_View             *v;

   D_ENTER;

   e = ev->event;
   if (e && e->win)
     {
	if ((v = e_view_machine_get_view_by_base_window(e->win)))
	  {
	  }
     }

   D_RETURN;
}

static void
e_visibility(Ecore_Event * ev)
{
   Ecore_Event_Window_Unmap *e;
   E_View             *v;

   D_ENTER;

   e = ev->event;
   if (e && e->win)
     {
	if ((v = e_view_machine_get_view_by_base_window(e->win)))
	  {
	  }
     }

   D_RETURN;
}

static void
e_focus_in(Ecore_Event * ev)
{
   Ecore_Event_Window_Focus_In *e;
   E_View             *v;

   D_ENTER;

   e = ev->event;
   if (e && e->win)
     {
	if ((v = e_view_machine_get_view_by_base_window(e->win)))
	  {
	  }
     }

   D_RETURN;
}

static void
e_focus_out(Ecore_Event * ev)
{
   Ecore_Event_Window_Focus_Out *e;
   E_View             *v;

   D_ENTER;

   e = ev->event;
   if (e && e->win)
     {
	if ((v = e_view_machine_get_view_by_base_window(e->win)))
	  {
	  }
     }

   D_RETURN;
}

static void
e_delete(Ecore_Event * ev)
{
   Ecore_Event_Window_Delete *e;
   E_View             *v;

   D_ENTER;

   e = ev->event;
   if (e && e->win)
     {
	if ((v = e_view_machine_get_view_by_base_window(e->win)))
	  {
	     e_object_unref(E_OBJECT(v));
	     D_RETURN;
	  }
     }

   D_RETURN;
}

/*
 * dnd status handler 
 * 
 */
static void
e_dnd_status(Ecore_Event * ev)
{
   Ecore_Event_Dnd_Drop_Status *e;

   /*
    *  typedef struct _ecore_event_dnd_drop_status
    *  {
    *    Window              win, root, source_win;
    *    int                 x, y, w, h;
    *    int                 copy, link, move, private;
    *    int                 all_position_msgs;
    *    int                 ok;
    *  } Ecore_Event_Dnd_Drop_Status;
    */
   E_View             *v;

   D_ENTER;

   e = ev->event;
   if (e && e->win)
     {
	if ((v = e_view_machine_get_view_by_base_window(e->win)))
	  {
	     if (dnd_pending_mode != E_DND_DELETED &&
		 dnd_pending_mode != E_DND_COPIED)
	       {
		  if (e->copy)
		     dnd_pending_mode = E_DND_COPY;
		  else if (e->move)
		     dnd_pending_mode = E_DND_MOVE;
		  else if (e->link)
		     dnd_pending_mode = E_DND_LINK;
		  else
		     dnd_pending_mode = E_DND_ASK;
	       }

	     ecore_window_dnd_ok(e->ok);

	     v->changed = 1;
	     v->drag.icon_hide = 1;
	  }
     }

   D_RETURN;
}

static void
e_wheel(Ecore_Event * ev)
{
   Ecore_Event_Wheel  *e;
   E_View             *v;

   D_ENTER;

   e = ev->event;
   if (e && e->win)
     {
	if ((v = e_view_machine_get_view_by_main_window(e->win)))
	  {
	  }
     }

   D_RETURN;
}

static void
e_key_down(Ecore_Event * ev)
{
   Ecore_Event_Key_Down *e;
   E_View             *v;

   D_ENTER;

   e = ev->event;
   if (e && e->win)
     {
	if ((v = e_view_machine_get_view_by_main_window(e->win))
	    || (v = e_view_machine_get_view_by_base_window(e->win)))
	  {
	     if (!strcmp(e->key, "Up"))
	       {
		  e_scrollbar_set_value(v->scrollbar.v,
					v->scrollbar.v->val - 8);
	       }
	     else if (!strcmp(e->key, "Down"))
	       {
		  e_scrollbar_set_value(v->scrollbar.v,
					v->scrollbar.v->val + 8);
	       }
	     else if (!strcmp(e->key, "Left"))
	       {
		  e_scrollbar_set_value(v->scrollbar.h,
					v->scrollbar.h->val - 8);
	       }
	     else if (!strcmp(e->key, "Right"))
	       {
		  e_scrollbar_set_value(v->scrollbar.h,
					v->scrollbar.h->val + 8);
	       }
	     else if (!strcmp(e->key, "Escape"))
	       {
	       }
	     else
	       {
		  char               *type;

		  type = ecore_keypress_translate_into_typeable(e);
		  if (type)
		    {
		    }
	       }
	     D_RETURN;
	  }
     }

   D_RETURN;
}

static void
e_key_up(Ecore_Event * ev)
{
   Ecore_Event_Key_Up *e;
   E_View             *v;

   D_ENTER;

   e = ev->event;
   if (e && e->win)
     {
	if ((v = e_view_machine_get_view_by_base_window(e->win)))
	  {
	  }
     }

   D_RETURN;
}

static void
e_mouse_down(Ecore_Event * ev)
{
   Ecore_Event_Mouse_Down *e;
   E_View             *v;

   D_ENTER;

   e = ev->event;
   current_ev = ev;
   if (e && e->win)
     {
	if ((v = e_view_machine_get_view_by_main_window(e->win)))
	  {
	     int                 focus_mode;

	     focus_mode = config_data->window->focus_mode;
	     if (focus_mode == 2)
		ecore_focus_to_window(v->win.base);
	     evas_event_feed_mouse_down(v->evas, e->button);
	     current_ev = NULL;
	     D_RETURN;
	  }
     }
   current_ev = NULL;

   D_RETURN;
}

static void
e_mouse_up(Ecore_Event * ev)
{
   Ecore_Event_Mouse_Up *e;
   E_View             *v;

   D_ENTER;

   e = ev->event;
   current_ev = ev;
   if (e && e->win)
     {
	if ((v = e_view_machine_get_view_by_main_window(e->win)))
	  {
	     evas_event_feed_mouse_up(v->evas, e->button);
	     current_ev = NULL;
	     D_RETURN;
	  }
     }
   current_ev = NULL;

   D_RETURN;
}

static void
e_mouse_move(Ecore_Event * ev)
{
   Ecore_Event_Mouse_Move *e;
   E_View             *v;

   D_ENTER;

   e = ev->event;
   current_ev = ev;
   if (e && e->win)
     {
	if ((v = e_view_machine_get_view_by_main_window(e->win)))
	  {
	     evas_event_feed_mouse_move(v->evas, e->x, e->y);
	     current_ev = NULL;
	     D_RETURN;
	  }
     }
   current_ev = NULL;

   D_RETURN;
}

static void
e_mouse_in(Ecore_Event * ev)
{
   Ecore_Event_Window_Enter *e;
   E_View             *v;

   D_ENTER;

   e = ev->event;
   current_ev = ev;
   if (e && e->win)
     {
	if ((v = e_view_machine_get_view_by_main_window(e->win)))
	  {
	     if (v->is_desktop)
	       {
		  evas_event_feed_mouse_in(v->evas);
	       }
	     current_ev = NULL;
	     D_RETURN;
	  }
     }
   current_ev = NULL;

   D_RETURN;
}

static void
e_mouse_out(Ecore_Event * ev)
{
   Ecore_Event_Window_Leave *e;
   E_View             *v;

   D_ENTER;

   e = ev->event;
   current_ev = ev;
   if (e && e->win)
     {
	if ((v = e_view_machine_get_view_by_main_window(e->win)))
	  {
	     evas_event_feed_mouse_out(v->evas);
	     current_ev = NULL;
	     D_RETURN;
	  }
     }
   current_ev = NULL;

   D_RETURN;
}

static void
e_window_expose(Ecore_Event * ev)
{
   Ecore_Event_Window_Expose *e;
   E_View             *v;

   D_ENTER;

   e = ev->event;
   current_ev = ev;
   if (e && e->win)
     {
	if ((v = e_view_machine_get_view_by_main_window(e->win)))
	  {
	     if (!(v->pmap))
		evas_damage_rectangle_add(v->evas, e->x, e->y, e->w, e->h);
	     v->changed = 1;
	     D_RETURN;
	  }
     }

   D_RETURN;
}

Ecore_Event        *
e_view_get_current_event(void)
{
   D_ENTER;

   D_RETURN_(current_ev);
}

int
e_view_filter_file(E_View * v, char *file)
{
   D_ENTER;

   if (file[0] == '.')
      D_RETURN_(0);

   D_RETURN_(1);
   UN(v);
}

static int
e_view_restart_alphabetical_qsort_cb(const void *data1, const void *data2)
{
   E_Icon             *ic, *ic2;

   D_ENTER;

   ic = *((E_Icon **) data1);
   ic2 = *((E_Icon **) data2);

   D_RETURN_(strcmp(ic->file->file, ic2->file->file));
}

void
e_view_resort_alphabetical(E_View * v)
{
   Evas_List          *icons = NULL, *l;
   E_Icon            **array;
   int                 i, count;

   D_ENTER;

   if (!v->icons)
      D_RETURN;
   for (count = 0, l = v->icons; l; l = l->next)
      count++;
   array = malloc(sizeof(E_Icon *) * count);
   for (i = 0, l = v->icons; l; l = l->next)
      array[i++] = l->data;
   D("qsort %i elements...\n", count);
   qsort(array, count, sizeof(E_Icon *), e_view_restart_alphabetical_qsort_cb);
   for (i = 0; i < count; i++)
      icons = evas_list_append(icons, array[i]);
   FREE(array);

   evas_list_free(v->icons);
   v->icons = icons;

   D("done...\n");

   D_RETURN;
}

void
e_view_arrange(E_View * v)
{
   Evas_List *           l;
   int                 x, y;
   int                 x1, x2, y1, y2;
   double              sv, sr, sm;

   D_ENTER;

   x = v->spacing.window.l;
   y = v->spacing.window.t;

   for (l = v->icons; l; l = l->next)
     {
	E_Icon             *ic;

	ic = l->data;
	if ((x != v->spacing.window.l) &&
	    ((x + ic->geom.w) > v->size.w - v->spacing.window.r))
	  {
	     x = v->spacing.window.l;
	     y += ic->geom.h + v->spacing.icon.b;
	  }
	ic->geom.x = x;
	ic->geom.y = y;
	e_icon_apply_xy(ic);
	x += ic->geom.w + v->spacing.icon.s;
     }

   e_view_icons_get_extents(v, &x1, &y1, &x2, &y2);

   sv = -(v->scroll.y - v->spacing.window.t);
   sr = v->size.h - v->spacing.window.t - v->spacing.window.b;
   sm = y2 - y1;
   if (sr > sm)
      sr = sm;
   e_scrollbar_set_range(v->scrollbar.v, sr);
   e_scrollbar_set_max(v->scrollbar.v, sm);
   e_scrollbar_set_value(v->scrollbar.v, sv);
   if (sr < sm)
      e_scrollbar_show(v->scrollbar.v);
   else
      e_scrollbar_hide(v->scrollbar.v);

   sv = -(v->scroll.x - v->spacing.window.l);
   sr = v->size.w - v->spacing.window.l - v->spacing.window.r;
   sm = x2 - x1;
   if (sr > sm)
      sr = sm;
   e_scrollbar_set_range(v->scrollbar.h, sr);
   e_scrollbar_set_max(v->scrollbar.h, sm);
   e_scrollbar_set_value(v->scrollbar.h, sv);
   if (sr < sm)
      e_scrollbar_show(v->scrollbar.h);
   else
      e_scrollbar_hide(v->scrollbar.h);

   v->changed = 1;
   D_RETURN;
}

void
e_view_resort(E_View * v)
{
   D_ENTER;

   e_view_resort_alphabetical(v);
   e_view_arrange(v);

   D_RETURN;
}

static void
e_view_resort_timeout(int val, void *data)
{
   E_View             *v;

   D_ENTER;

   v = data;
   e_view_resort(v);
   v->have_resort_queued = 0;

   D_RETURN;
   UN(val);
}

static void
e_view_layout_reload_timeout(int val, void *data)
{
   E_View             *v;
   D_ENTER;
   v = data;
   e_view_layout_reload(v);
   D_RETURN;
   UN(val);
}

static void
e_view_bg_reload_timeout(int val, void *data)
{
   E_View             *v;
   D_ENTER;
   v = data;
   e_view_bg_reload(v);
   D_RETURN;
   UN(val);
}

static void
e_view_ib_reload_timeout(int val, void *data)
{
   E_View             *v;
   D_ENTER;
   v = data;
   e_view_ib_reload(v);
   D_RETURN;
   UN(val);
}

void
e_view_queue_resort(E_View * v)
{
   char                name[PATH_MAX];

   D_ENTER;

   if (v->have_resort_queued)
      D_RETURN;
   v->have_resort_queued = 1;
   snprintf(name, PATH_MAX, "resort_timer.%s", v->name);
   ecore_add_event_timer(name, 1.0, e_view_resort_timeout, 0, v);

   D_RETURN;
}

static void
e_view_cleanup(E_View * v)
{
   char                name[PATH_MAX];

   D_ENTER;
   /* write geometry to metadata. This is done directly and 
    * not via a timeout, because we will destroy the object after this.*/
   e_view_geometry_record(v);

   if (v->bg)
      e_bg_free(v->bg);
   if (v->scrollbar.h)
      e_object_unref(E_OBJECT(v->scrollbar.h));
   if (v->scrollbar.v)
      e_object_unref(E_OBJECT(v->scrollbar.v));
   if (v->layout)
      e_object_unref(E_OBJECT(v->layout));

   ecore_window_destroy(v->win.base);

   snprintf(name, PATH_MAX, "resort_timer.%s", v->name);
   ecore_del_event_timer(name);

   /* unregister with the underlying dir and the global list of views */
   e_observer_unregister_observee(E_OBSERVER(v), E_OBSERVEE(v->dir));
   e_object_unref(E_OBJECT(v->dir));
   v->dir = NULL;
   e_view_machine_unregister_view(v);
   /* FIXME: clean up the rest!!! this leaks ... */

   /* Call the destructor of the base class */
   e_observer_cleanup(E_OBSERVER(v));
   D_RETURN;
}

void
e_view_file_event_handler(E_Observer *obs, E_Observee *o, E_Event_Type event, void *data)
{
   E_View *v = (E_View *) obs;
   E_File *f = (E_File *) data;
   char buf[PATH_MAX];	

   D_ENTER;

   if (event & E_EVENT_FILE_ADD)
      e_view_file_add(v, f);
   else if (event & E_EVENT_FILE_DELETE)
      e_view_file_delete(v, f);
   else if (event & E_EVENT_FILE_CHANGE)
      e_view_file_changed(v, f);
   else if (event & E_EVENT_FILE_INFO)
      e_view_file_try_to_show(v, f);
   else if (event & E_EVENT_BG_CHANGED)
   {
      snprintf(buf, PATH_MAX, "background_reload:%s", v->name);
      e_view_bg_reload(v);
      //ecore_add_event_timer(buf, 0.5, e_view_bg_reload_timeout, 0, v);
   }
   else if (event & E_EVENT_ICB_CHANGED)
   {
      snprintf(buf, PATH_MAX, "iconbar_reload:%s", v->name);
      ecore_add_event_timer(buf, 0.5, e_view_ib_reload_timeout, 0, v);
   }
   else if (event & E_EVENT_LAYOUT_CHANGED)
   {
      snprintf(buf, PATH_MAX, "layout_reload:%s", v->name);
      ecore_add_event_timer(buf, 0.5, e_view_layout_reload_timeout, 0, v);
   }

   D_RETURN;
   UN(o);
}

E_View             *
e_view_new(void)
{
   E_View             *v;

   D_ENTER;

   v = NEW(E_View, 1);
   ZERO(v, E_View, 1);

   e_observer_init(E_OBSERVER(v),	           
	 E_EVENT_FILE_ADD | E_EVENT_FILE_DELETE | E_EVENT_FILE_CHANGE 
	 | E_EVENT_FILE_INFO | E_EVENT_BG_CHANGED | E_EVENT_ICB_CHANGED 
	 | E_EVENT_LAYOUT_CHANGED ,
	 (E_Notify_Func) e_view_file_event_handler,
	 (E_Cleanup_Func) e_view_cleanup);

  
#define SOFT_DESK
/* #define X_DESK */
/* #define GL_DESK */

#ifdef SOFT_DESK
   /* ONLY alpha software can be "backing stored" */
   /*v->options.render_method = RENDER_METHOD_ALPHA_SOFTWARE;*/
   v->options.back_pixmap = 0;
#else
#ifdef X_DESK
   /*v->options.render_method = RENDER_METHOD_BASIC_HARDWARE;*/
   v->options.back_pixmap = 0;
#else
   /*v->options.render_method = RENDER_METHOD_3D_HARDWARE;*/
   v->options.back_pixmap = 0;
#endif
#endif
   v->select.config.grad_size.l = 8;
   v->select.config.grad_size.r = 8;
   v->select.config.grad_size.t = 8;
   v->select.config.grad_size.b = 8;
#define SETCOL(_member, _r, _g, _b, _a) \
_member.r = _r; _member.g = _g; _member.b = _b; _member.a = _a;
   SETCOL(v->select.config.edge_l, 0, 0, 0, 255);
   SETCOL(v->select.config.edge_r, 0, 0, 0, 255);
   SETCOL(v->select.config.edge_t, 0, 0, 0, 255);
   SETCOL(v->select.config.edge_b, 0, 0, 0, 255);
   SETCOL(v->select.config.middle, 255, 255, 255, 100);
   SETCOL(v->select.config.grad_l, 255, 255, 255, 100);
   SETCOL(v->select.config.grad_r, 255, 255, 255, 100);
   SETCOL(v->select.config.grad_t, 255, 255, 255, 100);
   SETCOL(v->select.config.grad_b, 255, 255, 255, 100);

   v->spacing.window.l = 3;
   v->spacing.window.r = 15;
   v->spacing.window.t = 3;
   v->spacing.window.b = 15;
   v->spacing.icon.s = 7;
   v->spacing.icon.g = 7;
   v->spacing.icon.b = 7;

   e_view_machine_register_view(v);

   
   D_RETURN_(v);
}

void
e_view_set_look(E_View * v, char *path)
{
   E_View_Look *l = NULL;
   char buf[PATH_MAX];
   D_ENTER;

   if (v->look)
      e_object_unref(E_OBJECT(v->look));
   
   if(!path)
   {
      /*
       * no path specified, lets look in the view's dir. If
       * there is a e_layout dir there, use whats in there.
       * Otherwise use the default dir.
       */
      snprintf(buf, PATH_MAX, "%s/.e_layout", v->dir->dir);
      if (e_file_exists(buf) && e_file_is_dir(buf))
	 path = buf;
      else
      {
	 snprintf(buf, PATH_MAX, "%s/appearance", e_config_user_dir());
	 path = buf;
      }
   }

   if ( !(l=e_view_machine_look_lookup(path)) )	 
   {
      v->look = e_view_look_new();
      e_view_look_set_dir (v->look, path);
   }
   else 
   {
      v->look = l;
      e_object_ref(E_OBJECT(v->look));
   }
   if(v->look)
   {
      e_observer_register_observee(E_OBSERVER(v), E_OBSERVEE(v->look->obj));

      e_view_bg_reload(v);
      e_view_layout_reload(v);
      e_view_ib_reload(v);
   }
   D_RETURN;
}

void
e_view_set_dir(E_View * v, char *path)
{
   E_Dir              *d = NULL;
   char                buf[PATH_MAX];

   D_ENTER;

   if (!v || !path || *path == 0)
      D_RETURN;

   if (!(d = e_view_machine_dir_lookup(path)))
     {
	D("Model for this dir doesn't exist, make a new one\n");
	d = e_dir_new();
	e_dir_set_dir(d, path);
     }
   else
      e_object_ref(E_OBJECT(d));

   if (d)
     {
	v->dir = d;
	e_observer_register_observee(E_OBSERVER(v), E_OBSERVEE(d));   
	/* FIXME do a real naming scheme here */
	snprintf(buf, PATH_MAX, "%s:%d", v->dir->dir, e_object_get_usecount(E_OBJECT(v->dir)));
	e_strdup(v->name, buf);
	D("assigned name to view: %s\n", v->name);

	/* Request metadata via efsd */
	v->geom_get.x = efsd_get_metadata(e_fs_get_connection(),
					  "/view/x", v->dir->dir, EFSD_INT);
	v->geom_get.y = efsd_get_metadata(e_fs_get_connection(),
					  "/view/y", v->dir->dir, EFSD_INT);
	v->geom_get.w = efsd_get_metadata(e_fs_get_connection(),
					  "/view/w", v->dir->dir, EFSD_INT);
	v->geom_get.h = efsd_get_metadata(e_fs_get_connection(),
					  "/view/h", v->dir->dir, EFSD_INT);
	v->geom_get.busy = 1;
     }
   else
     {
	D("Couldnt set dir for view! Bad!");
     }
   D_RETURN;
}

void
e_view_realize(E_View * v)
{
   char               *font_dir;

   D_ENTER;
   if (v->evas)
      D_RETURN;
   v->win.base = ecore_window_new(0,
				  v->location.x, v->location.y,
				  v->size.w, v->size.h);
   ecore_window_set_delete_inform(v->win.base);
   font_dir = e_config_get("fonts");

   v->evas = e_evas_new_all(ecore_display_get(),
			  v->win.base,
			  0, 0, v->size.w, v->size.h,
			  font_dir);
   v->win.main = e_evas_get_window(v->evas);
   e_cursors_display_in_window(v->win.main, "View");
   evas_event_feed_mouse_move(v->evas, -999999, -999999);
   ecore_window_set_events(v->win.base,
			   XEV_VISIBILITY | XEV_CONFIGURE |
			   XEV_PROPERTY | XEV_FOCUS);
   ecore_window_set_events(v->win.main,
			   XEV_EXPOSE | XEV_MOUSE_MOVE |
			   XEV_BUTTON | XEV_IN_OUT | XEV_KEY);
   if (v->options.back_pixmap)
     {
	Evas_Engine_Info_Software_X11 *info;

	v->pmap = ecore_pixmap_new(v->win.main, v->size.w, v->size.h, 0);
	info = (Evas_Engine_Info_Software_X11 *)evas_engine_info_get(v->evas);
	info->info.drawable = v->pmap;
	evas_engine_info_set(v->evas, (Evas_Engine_Info *)info);
	ecore_window_set_background_pixmap(v->win.main, v->pmap);
	ecore_window_clear(v->win.main);
     }
   v->scrollbar.v = e_scrollbar_new(v);
   e_scrollbar_set_change_func(v->scrollbar.v, e_view_scrollbar_v_change_cb, v);
   e_scrollbar_set_direction(v->scrollbar.v, 1);
   e_scrollbar_add_to_evas(v->scrollbar.v, v->evas);
   e_scrollbar_set_layer(v->scrollbar.v, 2000);
   e_scrollbar_set_value(v->scrollbar.v, 0.0);
   e_scrollbar_set_range(v->scrollbar.v, 1.0);
   e_scrollbar_set_max(v->scrollbar.v, 1.0);

   v->scrollbar.h = e_scrollbar_new(v);
   e_scrollbar_set_change_func(v->scrollbar.h, e_view_scrollbar_h_change_cb, v);
   e_scrollbar_set_direction(v->scrollbar.h, 0);
   e_scrollbar_add_to_evas(v->scrollbar.h, v->evas);
   e_scrollbar_set_layer(v->scrollbar.h, 2000);
   e_scrollbar_set_value(v->scrollbar.h, 0.0);
   e_scrollbar_set_range(v->scrollbar.h, 1.0);
   e_scrollbar_set_max(v->scrollbar.h, 1.0);

   /* I support dnd */
   ecore_window_dnd_advertise(v->win.base);
   ecore_window_show(v->win.main);

   v->changed = 1;
   D_RETURN;
}

void
e_view_populate(E_View * v)
{
   Evas_List *           l;

   /* populate with icons for all files in the dir we are monitoring. 
    * This has to be called _after_ view_realize because
    * view_add_file needs the evas to be intialized */
   for (l = v->dir->files; l; l = l->next)
     {
	E_File             *f = (E_File *) l->data;

	e_view_file_add(v, f);
	/* try to show the icons for the file. If this is not the first for
	 * the dir this will succeed because filetype and stat info have
	 * already been received. If not, it'll be shown when those arrive. */
	e_view_file_try_to_show(v, f);	     
     }
}

void
e_view_update(E_View * v)
{
   Evas_List *           l;

   D_ENTER;

   if (!v->changed)
      D_RETURN;

   if (v->drag.icon_hide)
     {
	for (l = v->icons; l; l = l->next)
	  {
	     E_Icon             *ic;

	     ic = l->data;
	     e_icon_hide_delete_pending(ic);
	  }
	v->drag.icon_hide = 0;
	v_dnd_source = v;
     }
   if (v->drag.icon_show)
     {
	for (l = v->icons; l; l = l->next)
	  {
	     E_Icon             *ic;

	     ic = l->data;
	     e_icon_show_delete_end(ic, dnd_pending_mode);
	  }
	dnd_pending_mode = E_DND_NONE;
	v->drag.icon_show = 0;
     }
   if (v->drag.update)
     {
	ecore_window_move(v->drag.win, v->drag.x, v->drag.y);
	v->drag.update = 0;
     }
   if (v->options.back_pixmap)
     {
	Evas_List           *up, *fp;
        Evas_Rectangle      *u;

	fp = up = evas_render_updates(v->evas);
	/* special code to handle if we are double buffering to a pixmap */
	/* and clear sections of the window if they got updated */
	while (up)
	  {
	     u = up->data;
	     ecore_window_clear_area(v->win.main, u->x, u->y, u->w, u->h);
	     up = evas_list_next(up);
	  }
	evas_render_updates_free(fp);
     }
   else
      evas_render(v->evas);

   v->changed = 0;

   D_RETURN;
}

void
e_view_file_add(E_View * v, E_File * f)
{
   D_ENTER;

   if (!e_icon_find_by_file(v, f->file))
     {
	E_Icon             *ic;

	ic = e_icon_new();
	ic->view = v;
	ic->file = f;
	ic->changed = 1;
	/* this basically allocates the obj.icon struct. Its image will be
	 * set later in icon_update_state */
	ic->obj.icon = evas_object_image_add(ic->view->evas);
	ic->obj.text = e_text_new(ic->view->evas, f->file, "filename");
	v->icons = evas_list_append(v->icons, ic);
	v->extents.valid = 0;
     }
   e_view_queue_resort(v);
   v->changed = 1;

   D_RETURN;
}

void
e_view_file_changed(E_View * v, E_File * f)
{
   E_Icon             *ic;

   D_ENTER;

   ic = e_icon_find_by_file(v, f->file);
   if (ic)
     {
	e_icon_update_state(ic);
     }
   v->changed = 1;
   D_RETURN;
}

void
e_view_file_try_to_show(E_View * v, E_File * f)
{
   E_Icon             *ic;

   D_ENTER;
   ic = e_icon_find_by_file(v, f->file);
   if (ic)
     {
	e_icon_update_state(ic);
	e_icon_initial_show(ic);
     }
   v->changed = 1;
   D_RETURN;
}

void
e_view_file_delete(E_View * v, E_File * f)
{
   E_Icon             *ic;

   D_ENTER;

   ic = e_icon_find_by_file(v, f->file);
   if (ic)
     {
	e_icon_hide(ic);
	v->icons = evas_list_remove(v->icons, ic);
	e_object_unref(E_OBJECT(ic));
	v->changed = 1;
	v->extents.valid = 0;
	e_view_queue_resort(v);
     }
   D_RETURN;
}

void
e_view_layout_reload(E_View * v)
{   
   D_ENTER;
   if (!v || !v->look)
      D_RETURN;

   if (e_object_unref(E_OBJECT(v->layout)) == 0)
      v->layout = NULL;

   /* try load a new layout */   
   v->layout = e_view_layout_new(v);

   /* if the layout loaded and theres an evas - we're realized */
   /* so realize the layout */
   if ((v->layout) && (v->evas))
      e_view_layout_realize(v->layout);

   e_view_layout_update(v->layout);
   
   v->changed = 1;
   D_RETURN;
}


void
e_view_ib_reload(E_View * v)
{
   D_ENTER;
   
  
   /* if we have an iconbar.. well nuke it */
   if (e_object_unref(E_OBJECT(v->iconbar)) == 0)
      v->iconbar = NULL;
     
   /* no iconbar in our look */
   if(!v->look->obj->icb || !v->look->obj->icb_bits)
      D_RETURN;

   /* try load a new iconbar */
   if (!v->iconbar)
      v->iconbar = e_iconbar_new(v);

   /* if the iconbar loaded and theres an evas - we're realized */
   /* so realize the iconbar */
   if ((v->iconbar) && (v->evas))
      e_iconbar_realize(v->iconbar);

   D_RETURN;
}

void
e_view_bg_reload(E_View * v)
{
   E_Background        bg = NULL;

   /* This should only be called if the background did really
    * change in the underlying dir. We dont check again
    * here. */
   D_ENTER;

   if (!v || !v->look)
      D_RETURN;
  
   /* nuke the old one */
   if (v->bg)
     {
	int                 size;

	e_bg_free(v->bg);
	v->bg = NULL;
	if (v->evas)
	{
	   size = evas_object_image_cache_get(v->evas);
	   evas_object_image_cache_flush(v->evas);
	   evas_object_image_cache_set(v->evas, size);
	}
	e_db_flush();
     }
   if(v->look->obj->bg)
   {
      bg = e_bg_load(v->look->obj->bg);
   }
   else
   {
      /* Our look doesnt provide a bg, falls back */
      char buf[PATH_MAX];
      if(v->is_desktop)
	 snprintf(buf, PATH_MAX, "%s/default.bg.db", e_config_get("backgrounds"));
      else
	 snprintf(buf, PATH_MAX, "%s/view.bg.db", e_config_get("backgrounds"));

      bg = e_bg_load(buf);
   }
   if (bg)
   {
      v->bg = bg;
      if (v->evas)
      {
	 e_bg_add_to_evas(v->bg, v->evas);
	 e_bg_set_scroll(v->bg, v->scroll.x, v->scroll.y);
	 e_bg_set_layer(v->bg, 100);
	 e_bg_resize(v->bg, v->size.w, v->size.h);

	 e_bg_callback_add(v->bg, EVAS_CALLBACK_MOUSE_UP, e_bg_up_cb, v);
	 e_bg_callback_add(v->bg, EVAS_CALLBACK_MOUSE_DOWN, e_bg_down_cb, v);
	 e_bg_callback_add(v->bg, EVAS_CALLBACK_MOUSE_MOVE, e_bg_move_cb, v);

	 e_bg_show(v->bg);
      }
   }
   v->changed = 1;
   D_RETURN;
}

void
e_view_init(void)
{
   D_ENTER;

   ecore_event_filter_handler_add(ECORE_EVENT_MOUSE_DOWN, e_mouse_down);
   ecore_event_filter_handler_add(ECORE_EVENT_MOUSE_UP, e_mouse_up);
   ecore_event_filter_handler_add(ECORE_EVENT_MOUSE_MOVE, e_mouse_move);
   ecore_event_filter_handler_add(ECORE_EVENT_MOUSE_IN, e_mouse_in);
   ecore_event_filter_handler_add(ECORE_EVENT_MOUSE_OUT, e_mouse_out);
   ecore_event_filter_handler_add(ECORE_EVENT_WINDOW_EXPOSE, e_window_expose);
   ecore_event_filter_handler_add(ECORE_EVENT_KEY_DOWN, e_key_down);
   ecore_event_filter_handler_add(ECORE_EVENT_KEY_UP, e_key_up);
   ecore_event_filter_handler_add(ECORE_EVENT_MOUSE_WHEEL, e_wheel);
   ecore_event_filter_handler_add(ECORE_EVENT_WINDOW_CONFIGURE, e_configure);
   ecore_event_filter_handler_add(ECORE_EVENT_WINDOW_PROPERTY, e_property);
   ecore_event_filter_handler_add(ECORE_EVENT_WINDOW_UNMAP, e_unmap);
   ecore_event_filter_handler_add(ECORE_EVENT_WINDOW_VISIBILITY, e_visibility);
   ecore_event_filter_handler_add(ECORE_EVENT_WINDOW_FOCUS_IN, e_focus_in);
   ecore_event_filter_handler_add(ECORE_EVENT_WINDOW_FOCUS_OUT, e_focus_out);
   ecore_event_filter_handler_add(ECORE_EVENT_WINDOW_DELETE, e_delete);
   /* dnd source handlers */
   ecore_event_filter_handler_add(ECORE_EVENT_DND_DROP_STATUS, e_dnd_status);
   ecore_event_filter_handler_add(ECORE_EVENT_DND_DATA_REQUEST,
				  e_dnd_data_request);
   ecore_event_filter_handler_add(ECORE_EVENT_DND_DROP_END, e_dnd_drop_end);
   /* dnd target handlers */
   ecore_event_filter_handler_add(ECORE_EVENT_DND_DROP_POSITION,
				  e_dnd_drop_position);
   ecore_event_filter_handler_add(ECORE_EVENT_DND_DROP, e_dnd_drop);
   ecore_event_filter_handler_add(ECORE_EVENT_DND_DROP_REQUEST,
				  e_dnd_drop_request);

   ecore_event_filter_idle_handler_add(e_idle, NULL);

   D_RETURN;
}

/*
 * send the dnd data to the target app
 *
 * uri-list (http://www.faqs.org/rfcs/rfc2483.html)
 * URL formatting per RFC 1738
 * (or not. Looks like a lot of apps ignore this. So do we! )
 */
static void
e_dnd_data_request(Ecore_Event * ev)
{
   Ecore_Event_Dnd_Data_Request *e;

   /*
    *  typedef struct _ecore_event_dnd_data_request
    *  {
    *    Window              win, root, source_win;
    *    int                 plain_text;
    *    Atom                destination_atom;
    *  } Ecore_Event_Dnd_Data_Request;
    */
   E_View             *v;
   Evas_List *           ll;
   char               *data = NULL;

   D_ENTER;
   /* Me, my null, and an extra for the end '/r/n'... */
   e_strdup(data, "");

   e = ev->event;
   if (!(v = e_view_machine_get_view_by_base_window(e->win)))
      D_RETURN;

   if (e->uri_list)
     {
	int                 first = 1;

	for (ll = v->icons; ll; ll = ll->next)
	  {
	     E_Icon             *ic;

	     ic = ll->data;
	     if (ic->state.selected)
	       {
		  int                 size;
		  char                buf[PATH_MAX];

		  if (first)
		    {
		       /*FIXME */
		       snprintf(buf, PATH_MAX, "file:%s/%s", v->dir->dir,
				ic->file->file);
		       first = 0;
		    }
		  else
		     /* FIXME */
		     snprintf(buf, PATH_MAX, "\r\nfile:%s/%s", v->dir->dir,
			      ic->file->file);
		  size = strlen(data) + strlen(buf) + 1;
		  REALLOC(data, char, size);

		  strncat(data, buf, size);
	       }
	  }
	ecore_dnd_send_data(e->source_win, e->win,
			    data, strlen(data) + 1,
			    e->destination_atom, DND_TYPE_URI_LIST);
     }
   else if (e->plain_text)
     {
	int                 first = 1;

	for (ll = v->icons; ll; ll = ll->next)
	  {
	     E_Icon             *ic;

	     ic = ll->data;
	     if (ic->state.selected)
	       {
		  int                 size;
		  char                buf[PATH_MAX];

		  if (first)
		    {
		       /*FIXME */
		       snprintf(buf, PATH_MAX, "%s/%s\n", v->dir->dir,
				ic->file->file);
		       first = 0;
		    }
		  else
		     /*FIXME */
		     snprintf(buf, PATH_MAX, "\n%s/%s", v->dir->dir,
			      ic->file->file);
		  size = strlen(data) + strlen(buf) + 1;
		  REALLOC(data, char, size);

		  strncat(data, buf, size);
	       }
	  }
	ecore_dnd_send_data(e->source_win, e->win,
			    data, strlen(data) + 1,
			    e->destination_atom, DND_TYPE_PLAIN_TEXT);
     }
   else				/* if (e->moz_url) */
     {
	FREE(data);
	data = NULL;

	for (ll = v->icons; ll; ll = ll->next)
	  {
	     E_Icon             *ic;

	     ic = ll->data;
	     if (ic->state.selected)
	       {
		  char                buf[16384];

		  /* FIXME */
		  snprintf(buf, PATH_MAX, "file:%s/%s", v->dir->dir,
			   ic->file->file);
		  data = strdup(buf);
		  break;
	       }
	  }
	if (data)
	  {
	     ecore_dnd_send_data(e->source_win, e->win,
				 data, strlen(data) + 1,
				 e->destination_atom, DND_TYPE_NETSCAPE_URL);
	  }
     }
   IF_FREE(data);
   D_RETURN;
}

static void
e_dnd_drop_end(Ecore_Event * ev)
{
   Ecore_Event_Dnd_Drop_End *e;

   /*
    * *  typedef struct _ecore_event_dnd_drop_end
    * *  {
    * *    Window              win, root, source_win;
    * *  } Ecore_Event_Dnd_Drop_End;
    */
   E_View             *v;

   D_ENTER;

   e = ev->event;
   if (e && e->win)
     {
	if ((v = e_view_machine_get_view_by_base_window(e->win)))
	  {
	     if (v_dnd_source)
	       {
		  if (dnd_pending_mode != E_DND_DELETED &&
		      dnd_pending_mode != E_DND_COPIED)
		    {
		       dnd_pending_mode = E_DND_COPIED;
		    }
		  if (v_dnd_source->drag.matching_drop_attempt)
		    {
		       v_dnd_source->drag.matching_drop_attempt = 0;
		       dnd_pending_mode = E_DND_COPIED;
		    }
		  v_dnd_source->changed = 1;
		  v_dnd_source->drag.icon_show = 1;
	       }

	     e_dnd_drop_request_free();
	  }
     }

   D_RETURN;
}

static void
e_dnd_drop_position(Ecore_Event * ev)
{
   Ecore_Event_Dnd_Drop_Position *e;

   /*
    *  typedef struct _ecore_event_dnd_drop_position
    *  {
    *    Window              win, root, source_win;
    *    int                 x, y;
    *  } Ecore_Event_Dnd_Drop_Position;
    */
   E_View             *v;

   D_ENTER;

   e = ev->event;
   if (e && e->win)
     {
	if ((v = e_view_machine_get_view_by_base_window(e->win)))
	  {
	     if (v->iconbar)
	       {
		  if (e->x >= v->iconbar->icon_area.x &&
		      e->x <= v->iconbar->icon_area.x + v->iconbar->icon_area.w
		      && e->y >= v->iconbar->icon_area.y
		      && e->y <=
		      v->iconbar->icon_area.y + v->iconbar->icon_area.h)
		    {
		       v->iconbar->dnd.x = e->x;
		       v->iconbar->dnd.y = e->y;
		       /* for iconbar drops, allow from same view */
		       v->drag.matching_drop_attempt = 0;
		       dnd_pending_mode = E_DND_ICONBAR_ADD;
		    }
	       }

	     /* send XdndStatus (even to same view, we'll */
	     /* ignore actions within the same view later */
	     /* during the drop action.) */
	     ecore_window_dnd_send_status_ok(v->win.base, e->source_win,
					     v->location.x, v->location.y,
					     v->size.w, v->size.h);

	     /* todo - cache window extents, don't send again within these extents. */
	  }
     }

   D_RETURN;
}

static void
e_dnd_drop(Ecore_Event * ev)
{
   Ecore_Event_Dnd_Drop *e;

   /*
    *  typedef struct _ecore_event_dnd_drop
    *  {
    *    Window              win, root, source_win;
    *  } Ecore_Event_Dnd_Drop;
    */
   E_View             *v;

   D_ENTER;

   e = ev->event;
   if (e && e->win)
     {
	if ((v = e_view_machine_get_view_by_base_window(e->win)))
	  {
	     /* Dropped!  Handle data */
	     /* Same view or same underlying dir?  Mark to skip action */
	     if (e->win == e->source_win || v->dir == v_dnd_source->dir)
		v_dnd_source->drag.matching_drop_attempt = 1;
	     /* Perform the action... */
	     e_dnd_handle_drop(v);

	     ecore_window_dnd_send_finished(v->win.base, e->source_win);
	     e_dnd_drop_request_free();
	  }
     }

   D_RETURN;
}

static void
e_dnd_drop_request(Ecore_Event * ev)
{
   Ecore_Event_Dnd_Drop_Request *e;

   /*
    *  typedef struct _ecore_event_dnd_drop_request
    *  {
    *    Window              win, root, source_win;
    *    int                 num_files;
    *    char              **files;
    *    int                 copy, link, move;
    *  } Ecore_Event_Dnd_Drop_Request;
    */
   E_View             *v;

   D_ENTER;

   e = ev->event;
   if (e && e->win)
     {
	if ((v = e_view_machine_get_view_by_base_window(e->win)))
	  {
	     /* if it exists, we already have the data... */
	     if ((!dnd_files) && (e->num_files > 0))
	       {
		  int                 i;

		  dnd_files = NEW_PTR(e->num_files);

		  /* copy the file list locally, for use in a dnd_drop */
		  for (i = 0; i < e->num_files; i++)
		     dnd_files[i] = strdup(e->files[i]);

		  dnd_num_files = e->num_files;

		  /* if the dnd source is e itself then dont use the event mode */
		  if ((v ==
		       e_view_machine_get_view_by_base_window(e->source_win)))
		    {
		       dnd_pending_mode = v->drag.drop_mode;
		    }
		  else
		    {
		       if (e->copy)
			  dnd_pending_mode = E_DND_COPY;
		       else if (e->move)
			  dnd_pending_mode = E_DND_MOVE;
		       else if (e->link)
			  dnd_pending_mode = E_DND_LINK;
		       else
			  dnd_pending_mode = E_DND_ASK;
		    }
	       }
	  }
     }

   D_RETURN;
}

static void
e_dnd_drop_request_free(void)
{
   D_ENTER;

   if (dnd_files)
     {
	int                 i;

	for (i = 0; i < dnd_num_files; i++)
	   FREE(dnd_files[i]);

	FREE(dnd_files);

	dnd_num_files = 0;
     }
   D_RETURN;
}

static void
e_dnd_handle_drop(E_View * v)
{
   int                 in, out;
   char               *filename;

   D_ENTER;

   /* Make space for destination in file list */
   dnd_num_files++;
   REALLOC_PTR(dnd_files, dnd_num_files);
   dnd_files[dnd_num_files - 1] = NULL;

   /* Verify files are local, convert to non-URL */
   for (in = 0, out = 0; in < dnd_num_files - 1; in++)
     {
	filename = e_util_de_url_and_verify(dnd_files[in]);
	/* Need a overlap safe copy here, like memmove() */
	if (filename)
	   memmove(dnd_files[out++], filename, strlen(filename) + 1);
     }

   /* Append destination for efsd */
   if (dnd_files[out])
      FREE(dnd_files[out]);

   dnd_files[out++] = strdup(v->dir->dir);

   switch (dnd_pending_mode)
     {
     case E_DND_COPY:
	/* Copy files */
	efsd_copy(e_fs_get_connection(), out, dnd_files, efsd_ops(0));
	dnd_pending_mode = E_DND_COPIED;
	break;
     case E_DND_MOVE:
	efsd_move(e_fs_get_connection(), out, dnd_files, efsd_ops(0));
	dnd_pending_mode = E_DND_DELETED;
	break;
     case E_DND_ICONBAR_ADD:
	e_iconbar_dnd_add_files(v, v_dnd_source, out, dnd_files);
	/*FIXME: should this be ICONBAR_ADDED? */
	dnd_pending_mode = E_DND_NONE;
     default:
	/* nothing yet */
	break;
     }

   D_RETURN;
}
