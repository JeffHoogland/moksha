#include "debug.h"
#include "config.h"
#include "actions.h"
#include "border.h"
#include "view.h"
#include "icccm.h"
#include "util.h"
#include "object.h"
#include "e_view_look.h"
#include "e_view_machine.h"
#include "menubuild.h"
#include "globals.h"
#include "desktops.h"

static E_Desktop   *current_desk = NULL;
static Evas_List   *sticky_list = NULL;
static Evas_List   *desktops = NULL;
static Window       e_base_win = 0;
static int          screen_w, screen_h;
static int          mouse_x, mouse_y;

static void         e_idle(void *data);
static void         e_mouse_move(Ecore_Event * ev);
static void         e_mouse_down(Ecore_Event * ev);
static void         e_mouse_up(Ecore_Event * ev);
static void         e_bg_up_cb(void *_data, Evas * _e, Evas_Object * _o,
			       void *event_info);
static void         e_window_expose(Ecore_Event * ev);
void                e_desktop_layout_reload(E_Desktop * d);

static void
e_scroller_timer(int val, void *data)
{
   int                 ok = 0;
   int                 resist;
   int                 scroll_speed;
   static double       last_time = 0.0;
   double              t;
   E_Desktop          *desk;

   D_ENTER;

   resist = config_data->desktops->resist;
   scroll_speed = config_data->desktops->speed;

   t = ecore_get_time();
   if (val != 0)
      scroll_speed = (int)(((t - last_time) / 0.02) * (double)scroll_speed);
   last_time = t;

   ok = 0;
   desk = current_desk;
   if (!desk)
      D_RETURN;
   if (mouse_x >= (screen_w - resist))
     {
	int                 scroll = 0;

	if ((desk->desk.area.x + desk->virt.w) > screen_w)
	   scroll = desk->desk.area.x + desk->virt.w - screen_w;
	if (scroll)
	  {
	     if (scroll > scroll_speed)
		scroll = scroll_speed;
	     e_desktops_scroll(desk, -scroll, 0);
	     ok = 1;
	  }
     }
   else if (mouse_x < resist)
     {
	int                 scroll = 0;

	if (desk->desk.area.x < 0)
	   scroll = -desk->desk.area.x;
	if (scroll)
	  {
	     if (scroll > scroll_speed)
		scroll = scroll_speed;
	     e_desktops_scroll(desk, scroll, 0);
	     ok = 1;
	  }
     }
   if (mouse_y >= (screen_h - resist))
     {
	int                 scroll = 0;

	if ((desk->desk.area.y + desk->virt.h) > screen_h)
	   scroll = desk->desk.area.y + desk->virt.h - screen_h;
	if (scroll)
	  {
	     if (scroll > scroll_speed)
		scroll = scroll_speed;
	     e_desktops_scroll(desk, 0, -scroll);
	     ok = 1;
	  }
     }
   else if (mouse_y < resist)
     {
	int                 scroll = 0;

	if (desk->desk.area.y < 0)
	   scroll = -desk->desk.area.y;
	if (scroll)
	  {
	     if (scroll > scroll_speed)
		scroll = scroll_speed;
	     e_desktops_scroll(desk, 0, scroll);
	     ok = 1;
	  }
     }
   if ((ok))
      ecore_add_event_timer("desktop_scroller", 0.02, e_scroller_timer, val + 1,
			    NULL);

   D_RETURN;
   UN(data);
}

Window
e_desktop_window()
{
   return e_base_win;
}

void
e_desktops_init(void)
{
   int                 i;
   E_Desktop          *desk;

   D_ENTER;

   ecore_window_get_geometry(0, NULL, NULL, &screen_w, &screen_h);
   e_base_win = ecore_window_input_new(0, 0, 0, screen_w, screen_h);
   ecore_window_set_events(e_base_win, XEV_CHILD_REDIRECT | XEV_PROPERTY |
		           XEV_COLORMAP | XEV_FOCUS | XEV_KEY |
			   XEV_MOUSE_MOVE | XEV_BUTTON | XEV_IN_OUT);
   ecore_window_set_events_propagate(e_base_win, True);
   ecore_window_show(e_base_win);

   D("Creating %d desktops\n", config_data->desktops->count);
   for (i = 0; i < config_data->desktops->count; i++)
      desk = e_desktops_new(i);

   current_desk = desktops->data;

   e_icccm_advertise_e_compat();
   /* todo 
   e_icccm_advertise_mwm_compat();
   e_icccm_advertise_gnome_compat();
   e_icccm_advertise_kde_compat();
   e_icccm_advertise_net_compat();
   */

   e_icccm_set_desk_area_size(0, 1, 1);
   e_icccm_set_desk_area(0, 0, 0);
   e_icccm_set_desk(0, 0);

   ecore_event_filter_handler_add(ECORE_EVENT_MOUSE_MOVE, e_mouse_move);
   ecore_event_filter_handler_add(ECORE_EVENT_MOUSE_UP, e_mouse_up);
   ecore_event_filter_handler_add(ECORE_EVENT_MOUSE_DOWN, e_mouse_down);
   ecore_event_filter_handler_add(ECORE_EVENT_WINDOW_EXPOSE, e_window_expose);

   ecore_event_filter_idle_handler_add(e_idle, NULL);

   D_RETURN;
}

void
e_desktops_scroll(E_Desktop * desk, int dx, int dy)
{
   /* Evas_List *           l;
   int                 xd, yd, wd, hd;
   int                 grav, grav_stick; */
   int                 scroll_sticky;

   D_ENTER;

   scroll_sticky = config_data->desktops->scroll_sticky;

   if ((dx == 0) && (dy == 0))
      D_RETURN;
   desk->x -= dx;
   desk->y -= dy;

   e_borders_scroll_list(desk->windows, dx, dy);
   if (scroll_sticky)
      e_borders_scroll_list(sticky_list, dx, dy);

   desk->desk.area.x += dx;
   desk->desk.area.y += dy;

   e_bg_set_scroll(desk->bg, desk->desk.area.x, desk->desk.area.y);

   D_RETURN;
}

void
e_desktops_cleanup(E_Desktop * desk)
{
   D_ENTER;

   while (desk->windows)
     {
	E_Border           *b;

	b = desk->windows->data;
	e_action_stop_by_object(E_OBJECT(b), NULL, 0, 0, 0, 0);

	if (e_object_get_usecount(E_OBJECT(b)) == 1)
	   e_border_release(b);
	e_object_unref(E_OBJECT(b));
     }

   if (desk->iconbar)
     {
	/* e_iconbar_save_out_final(desk->iconbar); */
	e_object_unref(E_OBJECT(desk->iconbar));
     }

   if (desk->look)
      e_object_unref(E_OBJECT(desk->look));

   if (desk->bg)
      e_bg_free(desk->bg);

   if (desk->evas)
      evas_free(desk->evas);

   IF_FREE(desk->name);
   IF_FREE(desk->dir);

   e_object_cleanup(E_OBJECT(desk));

   D_RETURN;
}

void
e_desktop_adaptor_cleanup(void *adaptor)
{
   e_object_cleanup(E_OBJECT(adaptor));
}

void
e_desktop_file_event_handler(E_Observer *obs, E_Observee *o, E_Event_Type event, void *data)
{
   E_Desktop_Adaptor *a = (E_Desktop_Adaptor *) obs;

   D_ENTER;

   if(a&&a->desktop) {
     char *mn=a->desktop->name?a->desktop->name:"";
     if (event & E_EVENT_BG_CHANGED) {
       D("background_reload: %s\n",mn);
       e_desktop_bg_reload(a->desktop); }
     else if (event & E_EVENT_ICB_CHANGED) {
       D("iconbar_reload: %s\n",mn);
       e_desktop_ib_reload(a->desktop); }
     else if (event & E_EVENT_LAYOUT_CHANGED) {
       D("layout_reload: %s\n",mn);
       e_desktop_layout_reload(a->desktop); }}
#ifdef DEBUG
   else {  /* add'l debug foo. technically, a, a->desktop should always be
              set, it's only a->desktop->name that we really worry about.
              Azundris 2003/01/11 */
     if(a) {
       D("e_desktop_file_event_handler: E_Desktop_Adaptor->desktop not set, "); }
     else {
       D("e_desktop_file_event_handler: E_Desktop_Adaptor not set, "); }

     if (event & E_EVENT_BG_CHANGED) {
       D("BG_CHANGED\n"); }
     else if (event & E_EVENT_ICB_CHANGED) {
       D("ICB_CHANGED\n"); }
     else if (event & E_EVENT_LAYOUT_CHANGED) {
       D("LAYOUT_CHANGED\n"); }
     else { D(" (unknown event-type)\n"); }}
#endif

   D_RETURN;
   UN(o);
   UN(data);
}

/* 2002/04/23  Azundris  <hacks@azundris.com>  Transparency for legacy apps
 *
 * Since we have new fancy ways of drawing stuff, we technically don't
 * use the root window any more, in the X meaning of the word.  This
 * results in legacy apps (xchat, xemacs, konsole, ...) failing to find
 * a root pixmap, thereby breaking pseudo-transparency.  Since we're
 * using a pixmap below anyway, we may as well export it to unbreak
 * the legacy thingies.  This code reuses parts of Esetroot (written
 * by Nat Friedman <ndf@mit.edu> with modifications by Gerald Britton
 * <gbritton@mit.edu> and Michael Jennings <mej@eterm.org>).  raster
 * intensely dislikes the pseudo-transparency hacks, so don't go to him
 * if you need to discuss them. : )
 * */

static void
e_desktops_set_fake_root(Pixmap p)
{
   Display            *Xdisplay;
   Screen             *scr;
   Window              Xroot;
   int                 screen;
   Atom                prop_root, prop_esetroot, type;
   int                 format;
   unsigned long       length, after;
   unsigned char      *data_root, *data_esetroot;

   D_ENTER;

   Xdisplay = ecore_display_get();

   screen = DefaultScreen(Xdisplay);
   Xroot = RootWindow(Xdisplay, screen);
   scr = ScreenOfDisplay(Xdisplay, screen);

   ecore_grab();

   prop_root = XInternAtom(Xdisplay, "_XROOTPMAP_ID", True);
   prop_esetroot = XInternAtom(Xdisplay, "ESETROOT_PMAP_ID", True);

   if (prop_root != None && prop_esetroot != None)
     {
	XGetWindowProperty(Xdisplay, Xroot, prop_root, 0L, 1L, False,
			   AnyPropertyType, &type, &format, &length, &after,
			   &data_root);
	if (type == XA_PIXMAP)
	  {
	     XGetWindowProperty(Xdisplay, Xroot, prop_esetroot, 0L, 1L, False,
				AnyPropertyType, &type, &format, &length,
				&after, &data_esetroot);
	     if (data_root && data_esetroot)
	       {
		  if (type == XA_PIXMAP
		      && *((Pixmap *) data_root) == *((Pixmap *) data_esetroot))
		    {
		       XKillClient(Xdisplay, *((Pixmap *) data_root));
		    }
	       }
	  }
     }

   /* This will locate the property, creating it if it doesn't exist */
   prop_root = XInternAtom(Xdisplay, "_XROOTPMAP_ID", False);
   prop_esetroot = XInternAtom(Xdisplay, "ESETROOT_PMAP_ID", False);

   /* The call above should have created it.  If that failed, we can't 
    * continue. */
   if (prop_root == None || prop_esetroot == None)
     {
	fprintf(stderr,
		"E17:  Creation of pixmap property failed in internal Esetroot.\n");
	D_RETURN;
     }

   XChangeProperty(Xdisplay, Xroot, prop_root, XA_PIXMAP, 32, PropModeReplace,
		   (unsigned char *)&p, 1);
   XChangeProperty(Xdisplay, Xroot, prop_esetroot, XA_PIXMAP, 32,
		   PropModeReplace, (unsigned char *)&p, 1);
   XSetCloseDownMode(Xdisplay, RetainPermanent);
   ecore_flush();

   ecore_window_set_background_pixmap(0, p);
   ecore_window_clear(0);
   ecore_ungrab();
   ecore_flush();

   D_RETURN;
}

void
e_desktop_bg_reload(E_Desktop * d)
{
   E_Background        bg = NULL;

   /* This should only be called if the background did really
    * change in the underlying dir. We dont check again
    * here. */
   D_ENTER;

   if (!d || !d->look)
      D_RETURN;
  
   /* nuke the old one */
   if (d->bg)
     {
	int                 size;

	e_bg_free(d->bg);
	d->bg = NULL;
	/*
	 * FIXME: Do we need to do all this flushing? Doesn't evas do some
	 * time stamp comparisons?
	 */
	if (d->evas)
	{
	   size = evas_image_cache_get(d->evas);
	   evas_image_cache_flush(d->evas);
	   evas_image_cache_set(d->evas, size);
	}
	e_db_flush();
     }

   if (d->look->obj->bg)
   {
      bg = e_bg_load(d->look->obj->bg);
   }
   else
   {
      /* Our look doesnt provide a bg, falls back */
      char buf[PATH_MAX];
      snprintf(buf, PATH_MAX, "%s/default.bg.db", e_config_get("backgrounds"));
      bg = e_bg_load(buf);
   }

   if (bg)
   {
      d->bg = bg;
      if (d->evas)
      {
	 e_bg_add_to_evas(d->bg, d->evas);
	 e_bg_set_scroll(d->bg, d->desk.area.x, d->desk.area.y);
	 e_bg_set_layer(d->bg, 100);
	 e_bg_resize(d->bg, d->real.w, d->real.h);

	 e_bg_callback_add(d->bg, EVAS_CALLBACK_MOUSE_UP, e_bg_up_cb, d);

	 e_bg_show(d->bg);
      }
   }

   evas_damage_rectangle_add(d->evas, 0, 0, d->real.w, d->real.h);
   e_desktop_update(d);

   D_RETURN;
}

void
e_desktop_layout_reload(E_Desktop * d)
{   
   D_ENTER;
   if (!d || !d->look)
      D_RETURN;

   if (e_object_unref(E_OBJECT(d->layout)) == 0)
      d->layout = NULL;

   /* try load a new layout */   
   d->layout = e_view_layout_new(d);

   /* if the layout loaded and theres an evas - we're realized */
   /* so realize the layout */
   if ((d->layout) && (d->evas))
      e_view_layout_realize(d->layout);

   e_view_layout_update(d->layout);
   
   D_RETURN;
}

void
e_desktop_ib_reload(E_Desktop * d)
{
   D_ENTER;
  
   /* if we have an iconbar.. well nuke it */
   if (e_object_unref(E_OBJECT(d->iconbar)) == 0)
      d->iconbar = NULL;
     
   /* no iconbar in our look */
   if(!d->look->obj->icb || !d->look->obj->icb_bits)
      D_RETURN;

   /* try load a new iconbar */
   if (!d->iconbar)
      d->iconbar = e_iconbar_new(d);

   /* if the iconbar loaded and theres an evas - we're realized */
   /* so realize the iconbar */
   if ((d->iconbar) && (d->evas))
      e_iconbar_realize(d->iconbar);

   D_RETURN;
}

E_Desktop          *
e_desktops_new(int i)
{
   char                buf[PATH_MAX];
   /* E_Border           *b; */
   E_Desktop          *desk;
   E_View_Look        *l;
   static Pixmap       background = 0;
   Evas_Engine_Info_Software_X11 *einfo;

   D_ENTER;

   desk = NEW(E_Desktop, 1);
   ZERO(desk, E_Desktop, 1);

   e_observee_init(E_OBSERVEE(desk), (E_Cleanup_Func) e_desktops_cleanup);

   desk->x = 0;
   desk->y = 0;
   desk->real.w = screen_w;
   desk->real.h = screen_h;
   desk->virt.w = config_data->desktops->width * screen_w;
   desk->virt.h = config_data->desktops->height * screen_h;
   desk->desk.desk = i;

   if (!background)
     {
        background = ecore_pixmap_new(0, screen_w, screen_h, 0);
        e_desktops_set_fake_root(background);
     }

   desk->evas = evas_new();
   evas_output_method_set(desk->evas, evas_render_method_lookup("software_x11"));
   evas_output_size_set(desk->evas, screen_w, screen_h);
   evas_output_viewport_set(desk->evas, 0, 0, screen_w, screen_h);

   einfo = (Evas_Engine_Info_Software_X11 *) evas_engine_info_get(desk->evas);
   einfo->info.display = ecore_display_get();
   einfo->info.visual = DefaultVisual(einfo->info.display,
		                      DefaultScreen(einfo->info.display));
   einfo->info.colormap = DefaultColormap(einfo->info.display,
		                      DefaultScreen(einfo->info.display));

   einfo->info.drawable = background;
   einfo->info.depth = DefaultDepth(einfo->info.display,
		                    DefaultScreen(einfo->info.display));
   einfo->info.rotation = 0;
   einfo->info.debug = 0;
   evas_engine_info_set(desk->evas, (Evas_Engine_Info *) einfo);

   snprintf(buf, PATH_MAX, "%sdesktop/%d/.e_layout", e_config_user_dir(), i);
   if (!(l = e_view_machine_look_lookup(buf)))
      desk->look = e_view_look_new();
   else
     {
	desk->look = l;
	e_object_ref(E_OBJECT(desk->look));
     }

   e_view_look_set_dir(desk->look, buf);

   /* experimental, Azundris 2003/01/11 */
   if(!desk->name) {
     size_t l=0;
     int j=10,k=1;
     while(i>=j) {
       j*=10;
       k++; }
#define DESK_DFLT_NAME "Desk "
     if((desk->name=malloc(l=sizeof(DESK_DFLT_NAME)+k)))
       snprintf(desk->name,l,"%s%d",DESK_DFLT_NAME,i); }

   /* The adaptor allows us to watch the look for events, while keeping the
    * desktop an observable object */
   desk->adaptor = NEW(E_Desktop_Adaptor, 1);
   e_observer_init(E_OBSERVER(desk->adaptor),
		   E_EVENT_BG_CHANGED | E_EVENT_ICB_CHANGED | E_EVENT_LAYOUT_CHANGED,
		   e_desktop_file_event_handler, e_desktop_adaptor_cleanup);
   desk->adaptor->desktop = desk;
   e_observer_register_observee(E_OBSERVER(desk->adaptor), E_OBSERVEE(desk->look->obj));

   e_desktop_bg_reload(desk);

   ecore_window_set_background_pixmap(0, background);
   ecore_window_clear(0);
   desktops = evas_list_append(desktops, desk);

   D_RETURN_(desk);
}

void
e_desktops_add_border(E_Desktop * d, E_Border * b)
{
   D_ENTER;

   if ((!d) || (!b))
      D_RETURN;

   if (!evas_list_find(d->windows, b))
     {
	 b->desk = d;
	 b->client.desk = d->desk.desk;
	 b->client.area.x = d->desk.area.x;
	 b->client.area.y = d->desk.area.y;
     }
   e_border_raise(b);

   D_RETURN;
}

void
e_desktops_add_sticky(E_Border *b)
{
   D_ENTER;

   if (b->desk)
      e_desktops_del_border(b->desk, b);
   if (!evas_list_find(sticky_list, b))
      sticky_list = evas_list_append(sticky_list, b);

   D_RETURN;
}


void
e_desktops_rm_sticky(E_Border *b)
{
   D_ENTER;

   e_desktops_add_border(current_desk, b);

   if (evas_list_find(sticky_list, b))
      sticky_list = evas_list_remove(sticky_list, b);

   D_RETURN;
}


void
e_desktops_del_border(E_Desktop * d, E_Border * b)
{
   D_ENTER;

   if ((!d) || (!b))
      D_RETURN;
   d->windows = evas_list_remove(d->windows, b);
   b->desk = NULL;

   D_RETURN;
}

E_Border *
e_desktop_raise_next_border(void)
{
   Evas_List *           next;
   E_Border           *start;
   E_Border           *current;

   D_ENTER;

   if (!current_desk || !current_desk->windows)
      D_RETURN_(NULL);

   start = current = e_border_current_focused();
   if (!start)
      start = current = current_desk->windows->data;

   /* Find the current border on the list of borders */
   for (next = current_desk->windows; next && next->data != current; next = next->next);

   /* Step to the next border, wrap around the queue if the end is reached */
   if (next && next->next)
      next = next->next;
   else
      next = current_desk->windows;

   /* Now find the next viewable border on the same desktop */
   current = (E_Border *) next->data;
   while ((current != start) && (!e_border_viewable(current)))
     {
	next = next->next;
	if (!next)
	   next = current_desk->windows;

	current = (E_Border *) next->data;
     }

   e_border_raise(current);
   e_icccm_send_focus_to(current->win.client, current->client.takes_focus);

   D_RETURN_(current);
}

void
e_desktops_delete(E_Desktop * d)
{
   D_ENTER;

   e_object_unref(E_OBJECT(d));

   D_RETURN;
}

void
e_desktops_show(E_Desktop * d)
{
   D_ENTER;

   e_bg_show(d->bg);
   evas_damage_rectangle_add(d->evas, 0, 0, d->real.w, d->real.h);
   e_desktop_update(d);

   D_RETURN;
}

void
e_desktops_hide(E_Desktop * d)
{
   D_ENTER;

   e_bg_hide(d->bg);

   D_RETURN;
   UN(d);
}

int
e_desktops_get_num(void)
{
   D_ENTER;
   D_RETURN_(config_data->desktops->count);
}

E_Desktop          *
e_desktops_get(int d)
{
   Evas_List *           l;
   int                 i;

   D_ENTER;

   for (i = 0, l = desktops; l; l = l->next, i++)
     {
	if (i == d)
	   D_RETURN_((E_Desktop *) l->data);
     }

   D_RETURN_(NULL);
}

int
e_desktops_get_current(void)
{
   D_ENTER;

   if (current_desk)
      D_RETURN_(current_desk->desk.desk);

   D_RETURN_(0);
}

void
e_desktops_goto_desk(int d)
{
   D_ENTER;

   e_desktops_goto(d, 0, 0);

   D_RETURN;
}

void
e_desktops_goto(int d, int ax, int ay)
{
   E_Desktop          *desk;

   D_ENTER;

   D("Switching to desktop %d at %d, %d\n", d, ax, ay);
   desk = e_desktops_get(d);
   if (desk)
     {
	int                 dx, dy;
	Evas_List          *l;
	E_Border           *b;

	if ((d == current_desk->desk.desk))
	   D_RETURN;

	dx = ax - desk->desk.area.x;
	dy = ay - desk->desk.area.y;

	for (l = current_desk->windows; l; l = l->next)
	  {
	     b = l->data;
	     if ((!b->client.iconified) && (!b->mode.move))
	       {
	          if (b->current.requested.visible)
		    {
		       b->current.requested.visible = 0;
		       b->changed = 1;
		    }
	       }
	     else if ((!b->client.iconified) && (b->mode.move))
	       {
		 e_desktops_del_border(current_desk, b);
		 b->client.desk = desk;
		 e_desktops_add_border(desk, b);
		 b->current.requested.visible = 1;
		 b->changed = 1;
	       }
	  }

	for (l = desk->windows; l; l = l->next)
	  {
	     b = l->data;
	     if ((!b->mode.move) && (!b->client.iconified))
	       {
	          if (!b->current.requested.visible)
		    {
		       b->current.requested.visible = 1;
		       b->changed = 1;
		    }
	       }
	  }

	e_border_update_borders();

	e_desktops_scroll(desk, dx, dy);
	dx = current_desk->desk.area.x - desk->desk.area.x;
	dy = current_desk->desk.area.y - desk->desk.area.y;
	e_borders_scroll_list(sticky_list, dx, dy);

	e_desktops_hide(current_desk);
	e_desktops_show(desk);
	current_desk = desk;

	e_border_check_select();

	e_icccm_set_desk_area(0, desk->desk.area.x, desk->desk.area.y);
	e_icccm_set_desk(0, desk->desk.desk);
	e_observee_notify_observers(E_OBSERVEE(desk), E_EVENT_DESKTOP_SWITCH, NULL);
     }

   D_RETURN;
}

Evas_List *
e_desktops_get_desktops_list()
{
   D_ENTER;
   D_RETURN_(desktops);
}

void
e_desktop_update(E_Desktop *d)
{
   Evas_Rectangle      *u;
   Evas_List           *up, *fp;

   D_ENTER;

   fp = up = evas_render_updates(d->evas);
   /* special code to handle if we are double buffering to a pixmap */
   /* and clear sections of the window if they got updated */
   while (up)
     {
        u = up->data;
	ecore_window_clear_area(0, u->x, u->y, u->w, u->h);
	up = evas_list_next(up);
     }

   if (fp)
      evas_render_updates_free(fp);

   D_RETURN;
}

/* handling expose events */
static void
e_window_expose(Ecore_Event * ev)
{
   Ecore_Event_Window_Expose *e;

   D_ENTER;

   e = ev->event;
/*   if (e->win == DefaultRootWindow(ecore_display_get())) */
   if (e->win == e_base_win)
      e_desktop_update(current_desk);

   D_RETURN;
}

/**
 * e_mouse_down - Handle mouse down events
 *
 * @ev: Pointer to event.
 */
static void
e_mouse_down(Ecore_Event * ev)
{
   Ecore_Event_Mouse_Up *e;

   D_ENTER;

   e = ev->event;
/*   if (e->win == DefaultRootWindow(ecore_display_get())) */
   if (e->win == e_base_win)
     {
        evas_event_feed_mouse_down(current_desk->evas, e->button);
     }

   D_RETURN;
}

/**
 * e_mouse_up - Handle mouse up events
 *
 * @ev: Pointer to event.
 */
static void
e_mouse_up(Ecore_Event * ev)
{
   Ecore_Event_Mouse_Up *e;

   D_ENTER;

   e = ev->event;
/*   if (e->win == DefaultRootWindow(ecore_display_get())) */
   if (e->win == e_base_win)
     {
        evas_event_feed_mouse_up(current_desk->evas, e->button);
     }

   D_RETURN;
}

/**
 * e_mouse_move - Handle mouse move events
 *
 * @ev: Pointer to event.
 */
static void
e_mouse_move(Ecore_Event * ev)
{
   Ecore_Event_Mouse_Move *e;

   D_ENTER;

   e = ev->event;
   mouse_x = e->rx;
   mouse_y = e->ry;
   if (config_data->desktops->scroll)
      e_scroller_timer(0, NULL);
/*   if (e->win == DefaultRootWindow(ecore_display_get())) */
   if (e->win == e_base_win)
     {
        evas_event_feed_mouse_move(current_desk->evas, e->x, e->y);
     }

   D_RETURN;
}

static void
e_idle(void *data)
{
   D_ENTER;

   e_desktop_update(current_desk);

   D_RETURN;
   UN(data);
}

static void
e_bg_up_cb(void *_data, Evas * _e, Evas_Object * _o, void *event_info)
{
   E_Desktop          *d;
   int                 dx, dy;
   Evas_Event_Mouse_Up *ev_info = event_info;

   D_ENTER;

   d = _data;
   dx = 0;
   dy = 0;
   ecore_pointer_xy_get(&dx, &dy);
   if (ev_info->button == 1)
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
		  e_menu_show_at_mouse(menu, dx, dy, CurrentTime);
	       }
	  }
     }
   else if (ev_info->button == 2)
     {
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
		  e_menu_show_at_mouse(menu, dx, dy, CurrentTime);
	       }
	  }
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
	        e_menu_show_at_mouse(menu, dx, dy, CurrentTime);
	  }
     }

   D_RETURN;
   UN(_e);
   UN(_o);
}
