#include "desktops.h"
#include "config.h"
#include "actions.h"
#include "border.h"
#include "background.h"
#include "view.h"
#include "icccm.h"
#include "util.h"

static Evas_List desktops = NULL;
static Window    e_base_win = 0;
static int       screen_w, screen_h;

static void ecore_idle(void *data);

static void
ecore_idle(void *data)
{
   e_db_flush();
   return;
   UN(data);
}

void
e_desktops_init(void)
{
   E_Desktop *desk;
   
   ecore_window_get_geometry(0, NULL, NULL, &screen_w, &screen_h);
   e_base_win = ecore_window_override_new(0, 0, 0, screen_w, screen_h);
   ecore_window_show(e_base_win);
   desk = e_desktops_new();
   e_desktops_show(desk);
   ecore_event_filter_idle_handler_add(ecore_idle, NULL);
 
   e_icccm_advertise_e_compat();
   e_icccm_advertise_mwm_compat();
   e_icccm_advertise_gnome_compat();
   e_icccm_advertise_kde_compat();
   e_icccm_advertise_net_compat();
   
   e_icccm_set_desk_area_size(0, 1, 1);
   e_icccm_set_desk_area(0, 0, 0);
   e_icccm_set_desk(0, 0);
}

void
e_desktops_scroll(E_Desktop *desk, int dx, int dy)
{
   Evas_List l;
   int xd, yd, wd, hd;
   int grav, grav_stick;   
   
   /* set grav */
   if ((dx ==0) && (dy == 0)) return;
   desk->x -= dx;
   desk->y -= dy;
   xd = yd = wd = hd = 0;
   grav = NorthWestGravity;
   grav_stick = SouthEastGravity;
   if ((dx <= 0) && (dy <= 0))
     {
	grav = NorthWestGravity;
	grav_stick = SouthEastGravity;
	xd = dx;
	yd = dy;
	wd = -dx;
	hd = -dy;
     }
   else if ((dx >= 0) && (dy <= 0))
     {
	grav = NorthEastGravity;
	grav_stick = SouthWestGravity;
	xd = 0;
	yd = dy;
	wd = dx;
	hd = -dy;
     }
   else if ((dx >= 0) && (dy >= 0))
     {
	grav = SouthEastGravity;
	grav_stick = NorthWestGravity;
	xd = 0;
	yd = 0;
	wd = dx;
	hd = dy;
     }
   else if ((dx <= 0) && (dy >= 0))
     {
	grav = SouthWestGravity;
	grav_stick = NorthEastGravity;
	xd = dx;
	yd = 0;
	wd = -dx;
	hd = dy;
     }
   for (l = desk->windows; l; l = l->next)
     {
	E_Border *b;
	
	b = l->data;
	/* if sticky */
	if ((b->client.sticky) && (!b->mode.move))	  
	  ecore_window_gravity_set(b->win.main, StaticGravity);
	else
	  ecore_window_gravity_set(b->win.main, grav);	
     }
   grav_stick = StaticGravity;
   /* scroll */
   ecore_window_move_resize(desk->win.container, 
			xd, yd, 
			screen_w + wd, screen_h + hd);
   /* reset */
   for (l = desk->windows; l; l = l->next)
     {
	E_Border *b;
	
	b = l->data;
	/* if sticky */
	if (b->client.sticky)
	  ecore_window_gravity_set(b->win.main, StaticGravity);
	else
	  ecore_window_gravity_set(b->win.main, grav_stick);	
/*	ecore_window_gravity_set(b->win.main, grav_stick);*/
     }   
   ecore_window_move_resize(desk->win.container, 0, 0, screen_w, screen_h);
   for (l = desk->windows; l; l = l->next)
     {
	E_Border *b;
	
	b = l->data;
	ecore_window_gravity_reset(b->win.main);
	if ((!b->client.sticky) && (!b->mode.move))	  
	  {
	     b->current.requested.x += dx;
	     b->current.requested.y += dy;
	     b->current.x = b->current.requested.x;
	     b->current.y = b->current.requested.y;
	     b->previous.requested.x = b->current.requested.x;
	     b->previous.requested.y = b->current.requested.y;
	     b->previous.x = b->current.x;
	     b->previous.y = b->current.y;
	     b->changed = 1;
	  }
     }
}

void
e_desktops_free(E_Desktop *desk)
{
   while (desk->windows)
     {
	E_Border *b;
	
	b = desk->windows->data;
	e_action_stop_by_object(b, NULL, 0, 0, 0, 0);
	OBJ_UNREF(b);
	OBJ_IF_FREE(b)
	  {
	     ecore_window_reparent(b->win.client, 0, 0, 0);
	     e_icccm_release(b->win.client);
	     OBJ_FREE(b);
	  }
     }
   ecore_window_destroy(desk->win.main);
   IF_FREE(desk->name);
   IF_FREE(desk->dir);
   FREE(desk);
}

void
e_desktops_init_file_display(E_Desktop *desk)
{
   E_View *v;
   E_Border *b;
   char buf[PATH_MAX];
   
   v = e_view_new();
   v->size.w = desk->real.w;
   v->size.h = desk->real.h;
   v->options.back_pixmap = 1;
   v->is_desktop = 1;

   /* fixme: later */
   /* uncomment this and comment out the next line for some tress testing */
   /* e_strdup(v->dir, "/dev"); */
   /* e_strdup(v->dir, e_file_home()); */
   sprintf(buf, "%s/desktop/default", e_config_user_dir());
   e_strdup(v->dir, buf);
   sprintf(buf, "%s/.e_background.bg.db", v->dir);
   v->bg = e_background_load(buf);
   if (!v->bg)
     {
	sprintf(buf, "%s/default.bg.db", e_config_get("backgrounds"));
	v->bg = e_background_load(buf);
     }
   e_view_realize(v);

   ecore_window_hint_set_borderless(v->win.base);
   ecore_window_hint_set_sticky(v->win.base, 1);
   ecore_window_hint_set_layer(v->win.base, 1);
   ecore_window_set_title(v->win.base, "Desktop");
   ecore_window_set_name_class(v->win.base, "FileView", "Desktop");
   ecore_window_set_min_size(v->win.base, desk->real.w, desk->real.h);
   ecore_window_set_max_size(v->win.base, desk->real.w, desk->real.h);
   b = e_border_adopt(v->win.base, 1);
   b->client.sticky = 1;
   b->client.fixed = 1;
   b->client.is_desktop = 1;

   if (v->options.back_pixmap) e_view_update(v);
}

E_Desktop *
e_desktops_new(void)
{
   E_Desktop *desk;
   
   desk = NEW(E_Desktop, 1);
   ZERO(desk, E_Desktop, 1);
   
   OBJ_INIT(desk, e_desktops_free);
   
   desk->win.main = ecore_window_override_new(e_base_win, 0, 0, screen_w, screen_h);
   desk->win.container = ecore_window_override_new(desk->win.main, 0, 0, screen_w, screen_h);
   ecore_window_lower(desk->win.container);
   
   ecore_window_show(desk->win.container);

   desk->x = 0;
   desk->y = 0;
   desk->real.w = screen_w;
   desk->real.h = screen_h;
   desk->virt.w = screen_w;
   desk->virt.h = screen_h;
   
   desktops = evas_list_append(desktops, desk);
   
   return desk;
}

void
e_desktops_add_border(E_Desktop *d, E_Border *b)
{
   if ((!d) || (!b)) return;
   b->desk = d;
   b->client.desk = d->desk.desk;
   b->client.area.x = d->desk.area.x;
   b->client.area.y = d->desk.area.y;
   e_border_raise(b);
}

void
e_desktops_del_border(E_Desktop *d, E_Border *b)
{
   if ((!d) || (!b)) return;
   d->windows = evas_list_remove(d->windows, b);
   b->desk = NULL;
}

void
e_desktops_delete(E_Desktop *d)
{
   OBJ_DO_FREE(d);
}

void
e_desktops_show(E_Desktop *d)
{
   ecore_window_show(d->win.main);
}

void
e_desktops_hide(E_Desktop *d)
{
   ecore_window_hide(d->win.main);
}

int
e_desktops_get_num(void)
{
   return 8;
}

E_Desktop *
e_desktops_get(int d)
{
   Evas_List l;
   int i;
   
   for (i = 0, l = desktops; l; l = l->next, i++)
     {
	if (i == d) return (E_Desktop *)l->data;
     }
   return NULL;
}

int
e_desktops_get_current(void)
{
   E_Desktop *desk;
   
   desk = e_desktops_get(0);
   if (desk)
     return desk->desk.desk;
   return 0;
}

void
e_desktops_goto_desk(int d)
{
   e_desktops_goto(d, 0, 0);
}

void
e_desktops_goto(int d, int ax, int ay)
{
   E_Desktop *desk;
   
   desk = e_desktops_get(0);
   if (desk)
     {
	int dx, dy;
	Evas_List l;
	
	if ((d == desk->desk.desk) &&
	    (ax == desk->desk.area.x) &&
	    (ay == desk->desk.area.y)) return;
	
	dx = ax - desk->desk.area.x;
	dy = ay - desk->desk.area.y;
	
	for (l = desk->windows; l; l = l->next)
	  {
	     E_Border *b;
	     
	     b = l->data;
	     if ((!b->client.sticky) && (!b->mode.move))
	       {
		  if (b->client.desk != d)
		    {
		       if (b->current.requested.visible)
			 {
			    b->current.requested.visible = 0;
			    b->changed = 1;
			 }
		    }
		  else
		    {
		       if (!b->current.requested.visible)
			 {
			    b->current.requested.visible = 1;
			    b->changed = 1;
			 }
		    }
	       }
	  }
	e_border_update_borders();
	
	/* if no scrolling... */
	e_desktops_scroll(desk, -(dx * desk->real.w), -(dy * desk->real.h));
	/* if scrolling.. need to setup a timeout etc. */
	
	desk->desk.desk = d;
	desk->desk.area.x = ax;
	desk->desk.area.y = ay;
	e_icccm_set_desk_area(0, desk->desk.area.x, desk->desk.area.y);
	e_icccm_set_desk(0, desk->desk.desk);
    }
}
