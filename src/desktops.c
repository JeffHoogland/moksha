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
static int       current_desk = 0;
static int       current_desk_x = 0;

static void e_idle(void *data);

static void
e_idle(void *data)
{
   e_db_flush();
   return;
   UN(data);
}

void
e_desktops_init(void)
{
   E_Desktop *desk;
   
   e_window_get_geometry(0, NULL, NULL, &screen_w, &screen_h);
   e_base_win = e_window_override_new(0, 0, 0, screen_w, screen_h);
   e_window_show(e_base_win);
   desk = e_desktops_new();
   e_desktops_show(desk);
   e_event_filter_idle_handler_add(e_idle, NULL);
 
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
   grav_stick= SouthEastGravity;
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
	if (b->client.sticky)
	  e_window_gravity_set(b->win.main, StaticGravity);
	e_window_gravity_set(b->win.main, grav);	
     }
   grav_stick = StaticGravity;
   e_window_gravity_set(desk->win.desk, grav_stick);
   /* scroll */
   e_window_move_resize(desk->win.container, 
			xd, yd, 
			screen_w + wd, screen_h + hd);
   /* reset */
   for (l = desk->windows; l; l = l->next)
     {
	E_Border *b;
	
	b = l->data;
	e_window_gravity_set(b->win.main, grav_stick);
     }   
   e_window_move_resize(desk->win.container, 0, 0, screen_w, screen_h);
   e_window_gravity_reset(desk->win.desk);
   for (l = desk->windows; l; l = l->next)
     {
	E_Border *b;
	
	b = l->data;
	e_window_gravity_reset(b->win.main);
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
   e_sync();
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
	     e_window_reparent(b->win.client, 0, 0, 0);
	     e_icccm_release(b->win.client);
	     OBJ_FREE(b);
	  }
     }
   e_window_destroy(desk->win.main);
   IF_FREE(desk->name);
   IF_FREE(desk->dir);
   FREE(desk);
}

void
e_desktops_init_file_display(E_Desktop *desk)
{
   desk->view = e_view_new();
   desk->view->size.w = desk->real.w;
   desk->view->size.h = desk->real.h;
   desk->view->is_desktop = 1;

   /* FIXME: load bg here */
   {
     char buf[4096];
     
     sprintf(buf, "%s/default.bg.db", e_config_get("backgrounds"));
     desk->view->bg = e_background_load(buf);
     printf("**** load %s = %p\n", buf, desk->view->bg);
   }

   /* fixme: later */
   /* uncomment this and comment out the next line for some tress testing */
   /* desk->view->dir = strdup("/dev");  */
   desk->view->dir = strdup(e_file_home()); 
   e_view_realize(desk->view);

   if (desk->view->options.back_pixmap)
     e_view_update(desk->view);

   desk->win.desk = desk->view->win.base;
   e_window_reparent(desk->win.desk, desk->win.container, 0, 0);
   e_window_show(desk->win.desk);
}

E_Desktop *
e_desktops_new(void)
{
   E_Desktop *desk;
   
   desk = NEW(E_Desktop, 1);
   ZERO(desk, E_Desktop, 1);
   
   OBJ_INIT(desk, e_desktops_free);
   
   desk->win.main = e_window_override_new(e_base_win, 0, 0, screen_w, screen_h);
   desk->win.container = e_window_override_new(desk->win.main, 0, 0, screen_w, screen_h);
   
   e_window_show(desk->win.container);

   desk->x = 0;
   desk->y = 0;
   desk->real.w = screen_w;
   desk->real.h = screen_h;
   desk->virt.w = screen_w;
   desk->virt.h = screen_h;
   
   desktops = evas_list_append(desktops, desk);
   
   e_desktops_init_file_display(desk);
   
   return desk;
}

void
e_desktops_add_border(E_Desktop *d, E_Border *b)
{
   if ((!d) || (!b)) return;
   b->desk = d;
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
   e_window_show(d->win.main);
}

void
e_desktops_hide(E_Desktop *d)
{
   if (d->view->options.back_pixmap) e_view_update(d->view);
   e_window_hide(d->win.main);
}

int
e_desktops_get_num(void)
{
   Evas_List l;
   int i;
   
   for (i = 0, l = desktops; l; l = l->next, i++);
   return i;
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
   return current_desk;
}

void
e_desktops_goto(int d)
{
   int dx;
   E_Desktop *desk;
   
   dx = d - current_desk_x;
   desk = e_desktops_get(0);
   if (desk)
     {
	e_desktops_scroll(desk, -(dx * desk->real.w), 0);
     }
   current_desk_x = d;
}
