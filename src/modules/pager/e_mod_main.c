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

static void        _pager_container_set(Pager *e);
static void        _pager_container_leave(Pager *e);

static Pager_Desk *_pager_desk_find(Pager *e, E_Desk *desk);
static Pager_Win  *_pager_window_find(Pager *e, E_Border *border);

static E_Manager  *_pager_manager_current_get(Pager *e);

static void        _pager_cb_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void        _pager_cb_gmc_change(void *data, E_Gadman_Client *gmc, E_Gadman_Change change);
static int         _pager_cb_event_border_resize(void *data, int type, void *event);
static int         _pager_cb_event_border_move(void *data, int type, void *event);
static int         _pager_cb_event_border_add(void *data, int type, void *event);
static int         _pager_cb_event_border_remove(void *data, int type, void *event);
static int         _pager_cb_event_border_hide(void *data, int type, void *event);
static int         _pager_cb_event_border_show(void *data, int type, void *event);
static int         _pager_cb_event_border_desk_set(void *data, int type, void *event);
static int         _pager_cb_event_zone_desk_count_set(void *data, int type, void *event);

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
/*   e_config_domain_save("module.pager", e->conf_edd, e->conf); */
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
   
/*   e->conf_edd = E_CONFIG_DD_NEW("Pager_Config", Config);
#undef T
#undef D
#define T Config
#define D e->conf_edd   */

/*   e->conf = e_config_domain_load("module.pager", e->conf_edd);
   if (!e->conf)
     {
	e->conf = E_NEW(Config, 1);
     } */

   managers = e_manager_list();
   e->managers = managers;
   for (l = managers; l; l = l->next)
     {
	E_Manager *man;
	
	man = l->data;
	for (l2 = man->containers; l2; l2 = l2->next)
	  {
	     E_Container *con;
	     
	     con = l2->data;
	     e->evas = con->bg_evas;
	     e->con = con;
	  }
     }

   o = evas_object_rectangle_add(e->evas);
   e->base = o;
   evas_object_color_set(o, 128, 128, 128, 0);
   evas_object_pass_events_set(o, 0);
   evas_object_repeat_events_set(o, 0);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _pager_cb_down, e);
   evas_object_show(o);

   o = edje_object_add(e->evas);
   e->screen = o;
   edje_object_file_set(o,
                        /* FIXME: "default.eet" needs to come from conf */
                        e_path_find(path_themes, "default.eet"),
                        "modules/pager/screen");
   evas_object_show(o);
   
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
   e->ev_handler_zone_desk_count_set =
   ecore_event_handler_add(E_EVENT_ZONE_DESK_COUNT_SET,
			   _pager_cb_event_zone_desk_count_set, e);

   _pager_container_set(e);

   e->gmc = e_gadman_client_new(e->con->gadman);
   e_gadman_client_domain_set(e->gmc, "module.pager", 0);
   e_gadman_client_policy_set(e->gmc,
			      E_GADMAN_POLICY_ANYWHERE |
			      E_GADMAN_POLICY_HMOVE |
			      E_GADMAN_POLICY_VMOVE |
			      E_GADMAN_POLICY_HSIZE |
			      E_GADMAN_POLICY_VSIZE);
   e_gadman_client_min_size_set(e->gmc, 8, 8);
   e_gadman_client_max_size_set(e->gmc, 256, 256);
   e_gadman_client_auto_size_set(e->gmc, 64, 64);
   e_gadman_client_align_set(e->gmc, 0.0, 0.5);
   e_gadman_client_resize(e->gmc, 80, 60);
   e_gadman_client_change_func_set(e->gmc, _pager_cb_gmc_change, e);
   e_gadman_client_load(e->gmc);
   return e;
}

static void
_pager_shutdown(Pager *e)
{
   free(e->conf);
/*   E_CONFIG_DD_FREE(e->conf_edd);*/

   if (e->base) evas_object_del(e->base);
   if (e->screen) evas_object_del(e->screen);
   e_object_del(E_OBJECT(e->gmc));

   _pager_zone_leave(e);
   ecore_event_handler_del(e->ev_handler_border_resize);
   ecore_event_handler_del(e->ev_handler_border_move);
   ecore_event_handler_del(e->ev_handler_border_add);
   ecore_event_handler_del(e->ev_handler_border_remove);
   ecore_event_handler_del(e->ev_handler_border_hide);
   ecore_event_handler_del(e->ev_handler_border_show);
   ecore_event_handler_del(e->ev_handler_border_desk_set);
   ecore_event_handler_del(e->ev_handler_zone_desk_count_set);

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
   if (d->obj) evas_object_del(d->obj);
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
   if (w->obj) evas_object_del(w->obj);
   if (w->icon) evas_object_del(w->icon);
   
   e_object_unref(E_OBJECT(w->border));
   E_FREE(w);
}

static void
_pager_container_set(Pager *e)
{
   int          desks_x, desks_y, x, y;
   Evas_Object *o;
   E_Desk      *current;
   Evas_List   *wins;
   E_App       *app;
   E_Zone      *zone;
   Evas_List   *zones;
   E_Container *con;

   con = e_container_current_get(_pager_manager_current_get(e));
   for(zones = con->zones; zones; zones = zones->next)
     {
	zone = zones->data;

	e_zone_desk_count_get(zone, &desks_x, &desks_y);

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
		    if (bd->new_client)
		      {
			 wins = wins->next;
			 continue;
		      }
		    win = _pager_window_create(e, bd, sym);

		    sym->wins = evas_list_append(sym->wins, win);
		    wins = wins->next;
		 }
	    }
	 evas_object_resize(e->base, e->fw * desks_x, e->fh * desks_y);
     }
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
	       if (bd->new_client)
		 {
		    wins = wins->next;
		    continue;
		 }
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
_pager_container_leave(Pager *e)
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
_pager_cb_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   Pager *p;
   E_Container *con = NULL;

   ev = event_info;
   p = data;
   con = e_container_current_get(_pager_manager_current_get(p));
   if (ev->button == 3)
     {
	e_menu_activate_mouse(p->config_menu, e_zone_current_get(con),
			      ev->output.x, ev->output.y, 1, 1,
			      E_MENU_POP_DIRECTION_DOWN);
	e_util_container_fake_mouse_up_all_later(con);
     }
   else if (ev->button == 1)
     {
	int          xcount, ycount;
	Evas_Coord   xx, yy, x, y;
	E_Zone      *zone;
	E_Desk      *desk;
	E_Container *con;

	con = e_container_current_get(_pager_manager_current_get(p));
	zone = e_zone_current_get(con);
	e_zone_desk_count_get(zone, &xcount, &ycount);
	evas_pointer_canvas_xy_get(p->evas, &xx, &yy);

	for (x = 0; x < xcount; x++)
	  for (y = 0; y < ycount; y++)
	    {
	       int left, right, top, bottom;
	       left = p->fx + x * p->fw;
	       right = left + p->fw;
	       top = p->fy + y * p->fh;
	       bottom = top + p->fh;

	       if (left <= xx && xx < right && top <= yy && yy < bottom)
		 {
		    desk = e_desk_at_xy_get(zone, x, y);
		    if (desk)
		      {
			 e_desk_show(desk);
			 _pager_desk_set(p, desk);
		      }
		    else
		      {
			 printf("PAGER ERROR - %d, %d seems to be out of bounds\n", x, y);
		      }
		 }
	    }

     }
}

static Pager_Desk *
_pager_desk_find(Pager *e, E_Desk *desk)
{
   Pager_Desk *pd;
   Evas_List *desks;

   desks = e->desks;
   while (desks)
     {
	pd = desks->data;
#if 0
	printf("Desk Find: (search,current)%p:%p\n", desk, pd->desk);
#endif
	if (pd->desk == desk)
	  return pd;
#if 0
	printf("Zone Find: (search,current)%p:%p\n", desk->zone, pd->desk->zone);
#endif
	desks = desks->next;
     }
   return NULL;
}

static Pager_Desk *
_pager_border_find(Pager *e, E_Border *border)
{
   int x, y;
   int desks_x, desks_y;
   E_Desk *desk = NULL;
   Pager_Desk *result = NULL;
   E_Zone *zone = NULL;
   
   if(border)
     {
	zone = border->zone;
	e_zone_desk_count_get(zone, &desks_x, &desks_y);
        printf("Desks_X, Desks_Y, DeskFind -> %i,%i:%p\n", desks_x, desks_y, border->desk);
	for (x = 0; x < desks_x; x++)
	  for (y = 0; y < desks_y; y++)
	    {
	       desk = e_desk_at_xy_get(zone, x, y);
	       printf("Desk %i,%i:%p\n", x, y, desk);
	       if(desk == border->desk) 
		 {
		    if((result = _pager_desk_find(e, desk)))
		      return(result);
		 }
	    }
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
_pager_cb_gmc_change(void *data, E_Gadman_Client *gmc, E_Gadman_Change change)
{ 
   Pager      *p;
   Evas_Coord  x, y, w, h;
   Evas_Coord  xx, yy, ww, hh, deskw, deskh;
   E_Zone     *zone;
   int         xcount, ycount;
   Evas_List  *desks, *wins;
   Pager_Desk *desk;
   Pager_Win  *win;
   E_Container *con;

   p = data;
   con = e_container_current_get(_pager_manager_current_get(p));
   zone = e_zone_current_get(con);
   e_zone_desk_count_get(zone, &xcount, &ycount);

   e_gadman_client_geometry_get(p->gmc, &x, &y, &w, &h);
   deskw = w / xcount;
   deskh = h / ycount;

   p->fx = x;
   p->fy = y;
   p->fw = deskw;
   p->fh = deskh;
   if (change == E_GADMAN_CHANGE_MOVE_RESIZE)
     {
	evas_object_move(p->base, x, y);
	evas_object_resize(p->base, w, h);
	evas_object_resize(p->screen, deskw, deskh);
	      
	desks = p->desks;
	while (desks)
	  {
	     desk = desks->data;
	     evas_object_resize(desk->obj, deskw, deskh);
	     evas_object_move(desk->obj, x + (deskw * desk->xpos),
			      y + (deskh * desk->ypos));
	     if (desk->current)
	       evas_object_move(p->screen, x + (deskw * desk->xpos),
				 y + (deskh * desk->ypos));
	     
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
   else if (change == E_GADMAN_CHANGE_RAISE)
     {
	evas_object_raise(p->base);
	      
	desks = p->desks;
	while (desks)
	  {
	     desk = desks->data;
	     evas_object_raise(desk->obj);
	     
	     wins = desk->wins;
	     while (wins)
	       {
		  win = wins->data;
		  evas_object_raise(win->obj);

		  wins = wins->next;
	       }
	     desks = desks->next;
	  }

	evas_object_raise(p->screen);
     }
}  

static int        
_pager_cb_event_border_resize(void *data, int type, void *event)
{
   Pager *e;
   Pager_Win *win;
   E_Event_Border_Resize *ev;
   
   e = data;
   ev = event;
   if((win = _pager_window_find(e, ev->border)))
     {
	_pager_window_move(e, win);
     }
   else
     {
	printf("ERROR: event_border_resize %p:%p\n",event, ev->border);
     }
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
   if((win = _pager_window_find(e, ev->border)))
     {
	_pager_window_move(e, win);
     }
   else
     {
	printf("ERROR: event_border_move %p:%p\n",event, ev->border);
     }
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
   if (_pager_window_find(e, ev->border)) 
     {
	printf("event_border_add, window found :'(\n");
	return 1;
     }
   if ((desk = _pager_desk_find(e, ((E_Border *) ev->border)->desk)))
     {
	new = _pager_window_create(data, ev->border, desk);
	desk->wins = evas_list_append(desk->wins, new);
#if 0
	printf("event_border_add, window created :) :) :)\n");
#endif
     }
/*
 * Correct me if I am wrong, but will the desk not previously been found above,
 * as _pager_desk find iterates all available, so if it has not matched we are
 * "out of zone" - if I am right we can remove _pager_border_find
 *
 * FIXME decide
   else if ((desk = _pager_border_find(e, ((E_Border *) ev->border))))
     {
	new = _pager_window_create(data, ev->border, desk);
	desk->wins = evas_list_append(desk->wins, new);
#if 0
	printf("event_border_add, window created from zone :) :) :)\n");
#endif
     }*/
   else
     {
	printf("event_border_add, desk not found :'(\n");
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
   Pager      *e;
   Pager_Win  *win;
   Pager_Desk *desk;
   E_Event_Border_Desk_Set *ev;

   e = data;
   ev = event;
   win = _pager_window_find(e, ev->border);
   desk = _pager_desk_find(e, ev->border->desk);
   if (win && desk)
     {
	win->owner = desk;
	_pager_window_move(e, win);
     }
   return 1;
}

static int
_pager_cb_event_zone_desk_count_set(void *data, int type, void *event)
{
   Pager *e;
   E_Event_Zone_Desk_Count_Set *ev;

   e = data;
   // FIXME need to update display with sizes etc
   return 1;
}

static E_Manager*
_pager_manager_current_get(Pager *e)
{
   Evas_List *l;
   E_Manager *result;
   for(l = e->managers; l; l = l->next)
     {
	result = l->data;
	if(result->visible)
	  return(result);
     }
   return(NULL);
}
