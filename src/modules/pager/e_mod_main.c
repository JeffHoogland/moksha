/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */

#include "e.h"
#include "e_mod_main.h"

/* TODO List:
 *
 * should support proper resize and move handles in the edje.
 * 
 */

/* module private routines */
static Pager      *_pager_init(E_Module *m);
static void        _pager_shutdown(Pager *e);
static E_Menu     *_pager_config_menu_new(Pager *e);
static void        _pager_config_menu_del(Pager *e, E_Menu *m);

static Pager_Desk *_pager_desk_create(Pager *e, E_Desk *desk);
static void        _pager_desk_destroy(Pager_Desk *d);
static Pager_Win  *_pager_window_create(Pager *e, E_Border *border, Pager_Desk *owner);
static void        _pager_window_destroy(Pager_Win *w);
static void        _pager_window_move(Pager *e, Pager_Win *w);

static void        _pager_zone_set(Pager *e, E_Zone *zone);
static void        _pager_zone_leave(Pager *e);
static void        _pager_desk_set(Pager *e, E_Desk *desk);

static Pager_Desk *_pager_desk_find(Pager *e, E_Desk *desk);
static Pager_Win  *_pager_window_find(Pager *e, E_Border *border);

static void        _pager_cb_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void        _pager_cb_up(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void        _pager_cb_move(void *data, Evas *e, Evas_Object *obj, void *event_info);
static int         _pager_cb_event_container_resize(void *data, int type, void *event);

static int         _pager_cb_event_border_resize(void *data, int type, void *event);
static int         _pager_cb_event_border_move(void *data, int type, void *event);
static int         _pager_cb_event_border_add(void *data, int type, void *event);
static int         _pager_cb_event_border_remove(void *data, int type, void *event);
static int         _pager_cb_event_border_hide(void *data, int type, void *event);
static int         _pager_cb_event_border_show(void *data, int type, void *event);
static int         _pager_cb_event_border_desk_set(void *data, int type, void *event);

static void        _pager_reconfigure(Pager *e);

#define         PAGER_MIN_W 10
#define         PAGER_MIN_H 7

/* public module routines. all modules must have these */
void *
init(E_Module *m)
{
   Pager *e;
   
   /* check module api version */
   if (m->api->version < E_MODULE_API_VERSION)
     {
	e_error_dialog_show("Module API Error",
			    "Error initializing Module: Pager\n"
			    "It requires a minimum module API version of: %i.\n"
			    "The module API advertized by Enlightenment is: %i.\n"
			    "Aborting module.",
			    E_MODULE_API_VERSION,
			    m->api->version);
	return NULL;
     }
   /* actually init pager */
   e = _pager_init(m);
   m->config_menu = _pager_config_menu_new(e);
   return e;
}

int
shutdown(E_Module *m)
{
   Pager *e;
   
   e = m->data;
   if (e)
     {
	if (m->config_menu)
	  {
	     _pager_config_menu_del(e, m->config_menu);
	     m->config_menu = NULL;
	  }
	_pager_shutdown(e);
     }
   return 1;
}

int
save(E_Module *m)
{
   Pager *e;

   e = m->data;
   e_config_domain_save("module.pager", e->conf_edd, e->conf);
   return 1;
}

int
info(E_Module *m)
{
   char buf[4096];
   
   m->label = strdup("Pager");
   snprintf(buf, sizeof(buf), "%s/module_icon.png", e_module_dir_get(m));
   m->icon_file = strdup(buf);
   return 1;
}

int
about(E_Module *m)
{
   e_error_dialog_show("Enlightenment Pager Module",
		       "A pager module to navigate E17 desktops.");
   return 1;
}

/* module private routines */
static Pager *
_pager_init(E_Module *m)
{
   Pager       *e;
   Evas_List   *managers, *l, *l2;
   Evas_Object *o;
   
   e = calloc(1, sizeof(Pager));
   if (!e) return NULL;
   
   e->conf_edd = E_CONFIG_DD_NEW("Pager_Config", Config);
#undef T
#undef D
#define T Config
#define D e->conf_edd   
   E_CONFIG_VAL(D, T, width, INT);
   E_CONFIG_VAL(D, T, height, INT);
   E_CONFIG_VAL(D, T, x, DOUBLE);
   E_CONFIG_VAL(D, T, y, DOUBLE);

   e->conf = e_config_domain_load("module.pager", e->conf_edd);
   if (!e->conf)
     {
	e->conf = E_NEW(Config, 1);
	e->conf->width = 50;
	e->conf->height = 30;
	e->conf->x = 0.0;
	e->conf->y = 0.0;
     }
   E_CONFIG_LIMIT(e->conf->x, 0.0, 1.0);
   E_CONFIG_LIMIT(e->conf->y, 0.0, 1.0);
   E_CONFIG_LIMIT(e->conf->width, PAGER_MIN_W, 1000);
   E_CONFIG_LIMIT(e->conf->height, PAGER_MIN_H, 1000);

   managers = e_manager_list();
   for (l = managers; l; l = l->next)
     {
	E_Manager *man;
	
	man = l->data;
	for (l2 = man->containers; l2; l2 = l2->next)
	  {
	     E_Container *con;
	     
	     con = l2->data;
	     e->con = con;
	     e->evas = con->bg_evas;
	  }
     }

   o = evas_object_rectangle_add(e->evas);
   e->base = o;
   evas_object_color_set(o, 128, 128, 128, 0);
   evas_object_pass_events_set(o, 0);
   evas_object_repeat_events_set(o, 0);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _pager_cb_down, e);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_UP, _pager_cb_up, e);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_MOVE, _pager_cb_move, e);
   evas_object_show(o);

   o = edje_object_add(e->evas);
   e->screen = o;
   edje_object_file_set(o,
                        /* FIXME: "default.eet" needs to come from conf */
                        e_path_find(path_themes, "default.eet"),
                        "modules/pager/screen");
   evas_object_show(o);
   
   e->ev_handler_container_resize =
   ecore_event_handler_add(E_EVENT_CONTAINER_RESIZE,
                           _pager_cb_event_container_resize, e);

   e->ev_handler_border_resize =
   ecore_event_handler_add(E_EVENT_BORDER_RESIZE,
			   _pager_cb_event_border_resize, e);
   e->ev_handler_border_move =
   ecore_event_handler_add(E_EVENT_BORDER_MOVE,
			   _pager_cb_event_border_move, e);
   e->ev_handler_border_add =
   ecore_event_handler_add(E_EVENT_BORDER_ADD,
			   _pager_cb_event_border_add, e);
   e->ev_handler_border_remove =
   ecore_event_handler_add(E_EVENT_BORDER_REMOVE,
			   _pager_cb_event_border_remove, e);
   e->ev_handler_border_hide =
   ecore_event_handler_add(E_EVENT_BORDER_HIDE,
			   _pager_cb_event_border_hide, e);
   e->ev_handler_border_show =
   ecore_event_handler_add(E_EVENT_BORDER_SHOW,
			   _pager_cb_event_border_show, e);
   e->ev_handler_border_desk_set =
   ecore_event_handler_add(E_EVENT_BORDER_DESK_SET,
			   _pager_cb_event_border_desk_set, e);
 
   _pager_zone_set(e, e_zone_current_get(e->con));
   _pager_reconfigure(e);
   evas_object_resize(e->screen, e->fw, e->fh);
   evas_object_move(e->base, e->fx, e->fy);
   return e;
}

static void
_pager_shutdown(Pager *e)
{
   free(e->conf);
   E_CONFIG_DD_FREE(e->conf_edd);

   if (e->base)
     {
	evas_object_del(e->base);
	evas_object_free(e->base);
     }
   if (e->screen)
     {
	evas_object_del(e->screen);
	evas_object_free(e->screen);
     }

   _pager_zone_leave(e);
   ecore_event_handler_del(e->ev_handler_container_resize);

   ecore_event_handler_del(e->ev_handler_border_resize);
   ecore_event_handler_del(e->ev_handler_border_move);
   ecore_event_handler_del(e->ev_handler_border_add);
   ecore_event_handler_del(e->ev_handler_border_remove);
   ecore_event_handler_del(e->ev_handler_border_hide);
   ecore_event_handler_del(e->ev_handler_border_show);
   ecore_event_handler_del(e->ev_handler_border_desk_set);

   free(e);
}

static E_Menu *
_pager_config_menu_new(Pager *e)
{
   E_Menu *mn;
   E_Menu_Item *mi;

   mn = e_menu_new();
   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "(Unused)");
   e->config_menu = mn;
   
   return mn;
}

static void
_pager_config_menu_del(Pager *e, E_Menu *m)
{
   e_object_del(E_OBJECT(m));
}

static Pager_Desk *
_pager_desk_create(Pager *e, E_Desk *desk)
{
   Pager_Desk  *new;
   Evas_Object *o;
   E_Desk      *next;
   int          xpos, ypos, xcount, ycount, found;
   
   new = E_NEW(Pager_Desk, 1);
   e_zone_desk_count_get(desk->zone, &xcount, &ycount);
   for (xpos = 0; xpos < xcount; xpos++)
     for (ypos = 0; ypos < ycount; ypos++)
       {
	  next = e_desk_at_xy_get(desk->zone, xpos, ypos);
	  if (next == desk)
	    {
	       new->xpos = xpos;
	       new->ypos = ypos;
	       break;
	    }
       }
   if (desk == e_desk_current_get(desk->zone))
     {
	new->current = 1;
	evas_object_move(e->screen, e->fx, e->fy);
     }
   new->desk = desk;
   e_object_ref(E_OBJECT(desk));

   o = edje_object_add(e->evas);
   new->obj = o;
   edje_object_file_set(o,
			/* FIXME: "default.eet" needs to come from conf */
			e_path_find(path_themes, "default.eet"),
			"modules/pager/desk");
   evas_object_pass_events_set(o, 1);

   evas_object_resize(o, e->fw, e->fh);
   evas_object_move(o, e->fx + (new->xpos * e->fw), e->fy + (new->ypos * e->fh));
   evas_object_show(o);
  
   return new;
}

static void        
_pager_desk_destroy(Pager_Desk *d)
{
   if (d->obj)
     {
	evas_object_del(d->obj);
	evas_object_free(d->obj);
     }

   e_object_unref(E_OBJECT(d->desk));

   while (d->wins)
     {
	_pager_window_destroy((Pager_Win *) d->wins->data);
	d->wins = evas_list_remove_list(d->wins, d->wins);
     }
   E_FREE(d);
}

static void
_pager_window_move(Pager *e, Pager_Win *w)
{
   Evas_Coord   ww, hh;
   double       scalex, scaley;
   evas_output_viewport_get(e->evas, NULL, NULL, &ww, &hh);
   scalex = (double) e->fw / ww;
   scaley = (double) e->fh / hh;

   evas_object_resize(w->obj, w->border->w * scalex, w->border->h * scaley);
   evas_object_move(w->obj,
		    e->fx + (w->owner->xpos * e->fw) + (w->border->x * scalex),
		    e->fy + (w->owner->ypos * e->fh) + (w->border->y * scaley));
}

static Pager_Win  *
_pager_window_create(Pager *e, E_Border *border, Pager_Desk *owner)
{
   Pager_Win   *new;
   Evas_Object *o;
   E_App       *app;
   int          visible;
   Evas_Coord   ww, hh;
   double       scalex, scaley;
   
   evas_output_viewport_get(e->evas, NULL, NULL, &ww, &hh);
   scalex = e->fw / ww;
   scaley = e->fy / hh;
   
   new = E_NEW(Pager_Win, 1);
   new->border = border;
   e_object_ref(E_OBJECT(border));
   visible = !border->iconic;
   new->owner = owner;

   o = edje_object_add(e->evas);
   new->obj = o;
   edje_object_file_set(o,
			/* FIXME: "default.eet" needs to come from conf */
			e_path_find(path_themes, "default.eet"),
			"modules/pager/window");
   evas_object_pass_events_set(o, 1);
   if (visible)
     evas_object_show(o);
   app = e_app_window_name_class_find(border->client.icccm.name,
				      border->client.icccm.class);
   /* FIXME: here we do not get the info, the app has not populated the icccm */
   if (app)
     {
	o = edje_object_add(e->evas);
	new->icon = o;
	edje_object_file_set(o, app->path, "icon");
	if (visible)
	  evas_object_show(o);
	edje_object_part_swallow(new->obj, "WindowIcon", o);
     }

   _pager_window_move(e, new);
   return new;
}

static void        
_pager_window_destroy(Pager_Win *w)
{
   if (w->obj)
     {
	evas_object_del(w->obj);
	evas_object_free(w->obj);
     }
   if (w->icon)
     {
	evas_object_del(w->icon);
	evas_object_free(w->icon);
     }
   
   e_object_unref(E_OBJECT(w->border));
   E_FREE(w);
}

static void
_pager_zone_set(Pager *e, E_Zone *zone)
{
   int          desks_x, desks_y, x, y;
   Evas_Object *o;
   E_Desk      *current;
   Evas_List   *wins;
   E_App       *app;

   e_zone_desk_count_get(zone, &desks_x, &desks_y);
   current = e_desk_current_get(zone);

   for (x = 0; x < desks_x; x++)
     for (y = 0; y < desks_y; y++)
       {
	  Pager_Desk *sym;
	  E_Desk     *desk;

	  desk = e_desk_at_xy_get(zone, x, y);
	  sym = _pager_desk_create(e, desk);
	  e->desks = evas_list_append(e->desks, sym);

          wins = desk->clients;
	  while (wins)
	    {
	       Pager_Win *win;
	       E_Border  *bd;

	       bd = wins->data;
	       win = _pager_window_create(e, bd, sym);

	       sym->wins = evas_list_append(sym->wins, win);
	       wins = wins->next;
	    }
       }
   evas_object_resize(e->base, e->fw * desks_x, e->fh * desks_y);
}

static void
_pager_zone_leave(Pager *e)
{
   while (e->desks)
     {
	_pager_desk_destroy((Pager_Desk *) e->desks->data);
	e->desks = evas_list_remove_list(e->desks, e->desks);
     }
}

static void
_pager_desk_set(Pager *p, E_Desk *desk)
{
   Evas_List *desks;
   int        x, y, desks_x, desks_y;

   e_zone_desk_count_get(desk->zone, &desks_x, &desks_y);
   desks = p->desks;
   for (x = 0; x < desks_x; x++)
     for (y = 0; y < desks_y; y++)
       {
	  Pager_Desk *pd;
	  int left, right, top, bottom;

	  pd = desks->data;
	  left = p->fx + x * p->fw;
	  top = p->fy + y * p->fh;

	  if (pd->desk == desk)
	    {
	       pd->current = 1;
	       evas_object_move(p->screen, left, top);
	    }
	  else
	    pd->current = 0;
	  desks = desks->next;
       }
}

static void
_pager_reconfigure(Pager *e)
{
   Evas_Coord ww, hh;
   E_Zone    *zone;
   int        xcount, ycount;

   evas_output_viewport_get(e->evas, NULL, NULL, &ww, &hh);
   e->fx = e->conf->x * (ww - e->conf->width);
   e->fy = e->conf->y * (hh - e->conf->height);
   e->fw = e->conf->width;
   e->fh = e->conf->height;

   zone = e_zone_current_get(e->con);
   e_zone_desk_count_get(zone, &xcount, &ycount);
   e->tw = e->fw * xcount;
   e->th = e->fh * ycount;

   _pager_zone_leave(e);
   _pager_zone_set(e, zone);
}

static void
_pager_cb_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   Pager *p;
   
   ev = event_info;
   p = data;
   if (ev->button == 3)
     {
	e_menu_activate_mouse(p->config_menu, e_zone_current_get(p->con),
			      ev->output.x, ev->output.y, 1, 1,
			      E_MENU_POP_DIRECTION_DOWN);
	e_util_container_fake_mouse_up_all_later(p->con);
     }
   else if (ev->button == 2)
     {
	p->resize = 1;
     }
   else if (ev->button == 1)
     {
	p->move = 1;
     }
   evas_pointer_canvas_xy_get(p->evas, &p->xx, &p->yy);
   p->clickhackx = p->xx;
   p->clickhacky = p->yy;
}

static Pager_Desk *
_pager_desk_find(Pager *e, E_Desk *desk)
{
   Evas_List *desks;

   desks = e->desks;
   while (desks)
     {
	Pager_Desk *next;

	next = desks->data;
	if (next->desk == desk)
	  return next;
	desks = desks->next;
     }
   return NULL;
}

static Pager_Win  *
_pager_window_find(Pager *e, E_Border *border)
{
   Evas_List *desks, *wins;

   desks = e->desks;
   while (desks)
     {
	Pager_Desk *next;

	next = desks->data;
	wins = next->wins;
	while (wins)
	  {
	     Pager_Win *next2;

	     next2 = wins->data;
	     if (next2->border == border)
	       return next2;
	     wins = wins->next;
	  }
	desks = desks->next;
     }
   return NULL;
}

static void
_pager_cb_up(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Up *ev;
   Pager     *p;
   Evas_Coord xx, yy, ww, hh;
   
   ev = event_info;
   p = data;

   evas_output_viewport_get(p->evas, NULL, NULL, &ww, &hh);
   /* if we clicked, not moved - FIXME, this is a hack */
   if (p->move && (p->xx == p->clickhackx) && (p->yy == p->clickhacky))
     {
	int     x, y, w, h, xcount, ycount, cx, cy;
	E_Zone *zone;
	E_Desk *desk;

	zone = e_zone_current_get(p->con);
	e_zone_desk_count_get(zone, &xcount, &ycount);
	evas_pointer_canvas_xy_get(e, &cx, &cy);

	w = p->fw;
	h = p->fh;
	for (x = 0; x < xcount; x++)
	  for (y = 0; y < ycount; y++)
	    {
	       int left, right, top, bottom;
	       left = p->fx + x * w;
	       right = left + w;
	       top = p->fy + y * h;
	       bottom = top + h;

	       if (left <= cx && cx < right && top <= cy && cy < bottom)
		 {
		    desk = e_desk_at_xy_get(zone, x, y);
		    e_desk_show(desk);
		    _pager_desk_set(p, desk);
		 }
	    }
     }
   p->move = 0;
   p->resize = 0;

   p->conf->width = p->fw;
   p->conf->height = p->fh;
   p->conf->x = (double)p->fx / (double)(ww - p->fw);
   p->conf->y = (double)p->fy / (double)(hh - p->fh);
   e_config_save_queue();

}

static void
_pager_cb_move(void *data, Evas *e, Evas_Object *obj, void *event_info)
{ 
   Evas_Event_Mouse_Move *ev;
   Pager      *p;
   Evas_Coord  cx, cy, sw, sh, tw, th, dx, dy, xx, yy, ww, hh;
   E_Zone     *zone;
   int         xcount, ycount;
   Evas_List  *desks, *wins;
   Pager_Desk *desk;
   Pager_Win  *win;
   
   evas_pointer_canvas_xy_get(e, &cx, &cy);
   evas_output_viewport_get(e, NULL, NULL, &sw, &sh);

   ev = event_info;
   p = data;

   zone = e_zone_current_get(p->con);
   e_zone_desk_count_get(zone, &xcount, &ycount);
   /* note that these are not the same as p->tw, as that could be slightly
      larger (rounding etc) these will vie exactly the right result */
   tw = p->fw * xcount;
   th = p->fh * ycount;
   dx = cx - p->xx;
   dy = cy - p->yy;
   if (p->move)
     {
	if (p->fx + dx < 0) dx = 0 - p->fx;
	if (p->fx + dx + tw > sw) dx = sw - (p->fx + tw);
	if (p->fy + dy < 0) dy = 0 - p->fy;
	if (p->fy + dy + th > sh) dy = sh - (p->fy + th);
	p->fx += dx;
	p->fy += dy;

	evas_object_move(p->base, p->fx, p->fy);
	      
	desks = p->desks;
	while (desks)
	  {
	     desk = desks->data;

	     evas_object_geometry_get(desk->obj, &xx, &yy, &ww, &hh);
	     evas_object_move(desk->obj, xx + dx, yy + dy);

	     wins = desk->wins;
	     while (wins)
	       {
		  win = wins->data;

		  evas_object_geometry_get(win->obj, &xx, &yy, &ww, &hh);
		  evas_object_move(win->obj, xx + dx, yy + dy);

		  wins = wins->next;
	       }

	     desks = desks->next;
	  }
	
	evas_object_geometry_get(p->screen, &xx, &yy, &ww, &hh);
	evas_object_move(p->screen, xx + dx, yy + dy);
     }
   else if (p->resize)
     {
	p->tw += dx;
	p->th += dy;
	p->fw = p->tw / xcount;
	p->fh = p->th / ycount;
	if (p->fw < PAGER_MIN_W) p->fw = PAGER_MIN_W;
	if (p->fh < PAGER_MIN_H) p->fh = PAGER_MIN_H;
	if (p->fx + p->tw > sw)
	  {
	     p->tw = sw - p->fx;
	     p->fw = p->tw / xcount;
	  }
	if (p->fy + p->th > sh)
	  {
	     p->th = sh - p->fy;
	     p->fh = p->th / ycount;
	  }

	evas_object_resize(p->base, p->fw * xcount, p->fh * ycount);
	      
	evas_object_resize(p->screen, p->fw, p->fh);
	desks = p->desks;
	while (desks)
	  {
	     desk = desks->data;
	     evas_object_resize(desk->obj, p->fw, p->fh);
	     evas_object_move(desk->obj, p->fx + (desk->xpos * p->fw),
			      p->fy + (desk->ypos * p->fh));
	     
	     wins = desk->wins;
	     while (wins)
	       {
		  win = wins->data;
		  _pager_window_move(p, win);

		  wins = wins->next;
	       }

	     desks = desks->next;
	  }

   }
   p->xx = ev->cur.canvas.x;
   p->yy = ev->cur.canvas.y;
}  

static int
_pager_cb_event_container_resize(void *data, int type, void *event)
{
   Pager     *e;
   
   e = data;
   _pager_reconfigure(e);
   return 1;
}

static int        
_pager_cb_event_border_resize(void *data, int type, void *event)
{
   Pager *e;
   Pager_Win *win;
   E_Event_Border_Resize *ev;
   
   e = data;
   ev = event;
   win = _pager_window_find(e, ev->border);
   _pager_window_move(e, win);
   return 1;
}

static int        
_pager_cb_event_border_move(void *data, int type, void *event)
{
   Pager *e;
   Pager_Win *win;
   E_Event_Border_Resize *ev;

   e = data;
   ev = event;
   win = _pager_window_find(e, ev->border);
   _pager_window_move(e, win);
   return 1;
}

static int        
_pager_cb_event_border_add(void *data, int type, void *event)
{
   Pager              *e;
   E_Event_Border_Add *ev;
   Pager_Win          *new;
   Pager_Desk         *desk;
   
   e = data;
   ev = event;
   desk = _pager_desk_find(e, ((E_Border *) ev->border)->desk);
   if (_pager_window_find(e, ev->border)) return 1;
   if (desk)
     {
	new = _pager_window_create(data, ev->border, desk);

	desk->wins = evas_list_append(desk->wins, new);
     }
   return 1;
}

static int        
_pager_cb_event_border_remove(void *data, int type, void *event)
{
   Pager                 *e;
   E_Event_Border_Remove *ev;
   Pager_Win             *old;
   Pager_Desk            *desk;

   e = data;
   ev = event;
   old = _pager_window_find(e, ev->border);
   desk = _pager_desk_find(e, ((E_Border *) ev->border)->desk);
   if (old && desk)
     {
	desk->wins = evas_list_remove(desk->wins, old);
	_pager_window_destroy(old);
     }
   return 1;
}

static int        
_pager_cb_event_border_hide(void *data, int type, void *event)
{
   Pager_Win           *win;
   Pager               *e;
   E_Event_Border_Hide *ev;

   e = data;
   ev = event;
   win = _pager_window_find(e, ev->border);
   if (win && ev->border->desk->visible)
     {
	evas_object_hide(win->obj);
	evas_object_hide(win->icon);
     }
   return 1;
}

static int        
_pager_cb_event_border_show(void *data, int type, void *event)
{
   Pager_Win           *win;
   Pager               *e;
   E_Event_Border_Show *ev;

   e = data;
   ev = event;
   win = _pager_window_find(e, ev->border);
   if (win)
     {
	evas_object_show(win->obj);
	evas_object_show(win->icon);
     }
   return 1;
}

static int        
_pager_cb_event_border_desk_set(void *data, int type, void *event)
{
   Pager *e;
   E_Event_Border_Desk_Set *ev;

   /* FIXME do something */
   
   e = data;
   ev = event;
   return 1;
}

