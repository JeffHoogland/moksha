/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

/* TODO List:
 *
 * * Get E_EVENT_BORDER_ADD when a window is moved from one desk to another by the wm
 */

/* module private routines */
static Evas_List   *_pager_init(E_Module *module);
static void        _pager_shutdown(Evas_List *pagers);
static E_Menu     *_pager_config_menu_new();
static void        _pager_config_menu_del(E_Menu *menu);

static Pager      *_pager_new(E_Zone *zone);
static void        _pager_destroy(Pager *pager);
static void        _pager_menu_new(Pager *pager);
static void        _pager_menu_del(E_Menu *menu);
static void        _pager_enable(Pager *pager);
static void        _pager_disable(Pager *pager);
static void        _pager_zone_set(Pager *pager, E_Zone *zone);
static void        _pager_zone_leave(Pager *pager);

static Pager_Desk *_pager_desk_new(Pager *pager, E_Desk *desk, int x, int y);
static void        _pager_desk_destroy(Pager_Desk *d);
static Pager_Win  *_pager_window_new(Pager_Desk *owner, E_Border *border);
static void        _pager_window_destroy(Pager_Win *win);

static void        _pager_draw(Pager *pager);
static void        _pager_window_move(Pager *pager, Pager_Win *win);

static Pager_Desk *_pager_desk_find(Pager *pager, E_Desk *desk);
static Pager_Win  *_pager_window_find(Pager *pager, E_Border *border);

static void        _pager_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void        _pager_cb_gmc_change(void *data, E_Gadman_Client *gmc, E_Gadman_Change change);
static int         _pager_cb_event_border_resize(void *data, int type, void *event);
static int         _pager_cb_event_border_move(void *data, int type, void *event);
static int         _pager_cb_event_border_add(void *data, int type, void *event);
static int         _pager_cb_event_border_remove(void *data, int type, void *event);
static int         _pager_cb_event_border_hide(void *data, int type, void *event);
static int         _pager_cb_event_border_show(void *data, int type, void *event);
static int         _pager_cb_event_border_desk_set(void *data, int type, void *event);
static int         _pager_cb_event_zone_desk_count_set(void *data, int type, void *event);
static void         _pager_cb_menu_enabled(void *data, E_Menu *m, E_Menu_Item *mi);

static int         _pager_count;
/* public module routines. all modules must have these */
void *
init(E_Module *module)
{
   Evas_List *pagers = NULL;

   /* check module api version */
   if (module->api->version < E_MODULE_API_VERSION)
     {
	e_error_dialog_show("Module API Error",
			    "Error initializing Module: Pager\n"
			    "It requires a minimum module API version of: %i.\n"
			    "The module API advertized by Enlightenment is: %i.\n"
			    "Aborting module.",
			    E_MODULE_API_VERSION,
			    module->api->version);
	return NULL;
     }
   /* actually init pager */
   module->config_menu = _pager_config_menu_new();
   pagers = _pager_init(module);

   return pagers;
}

int
shutdown(E_Module *module)
{
   Evas_List *pagers;

   if (module->config_menu)
     {
	_pager_config_menu_del(module->config_menu);
	module->config_menu = NULL;
     }

   pagers = module->data;
   if (pagers)
     {
	_pager_shutdown(pagers);
     }
   return 1;
}

int
save(E_Module *module)
{
   Evas_List *pagers;

   pagers = module->data;
/*   e_config_domain_save("module.pager", e->conf_edd, e->conf); */
   return 1;
}

int
info(E_Module *module)
{
   char buf[4096];

   module->label = strdup("Pager");
   snprintf(buf, sizeof(buf), "%s/module_icon.png", e_module_dir_get(module));
   module->icon_file = strdup(buf);
   return 1;
}

int
about(E_Module *module)
{
   e_error_dialog_show("Enlightenment Pager Module",
		       "A pager module to navigate E17 desktops.");
   return 1;
}

/* module private routines */
static Evas_List *
_pager_init(E_Module *module)
{
   Evas_List   *pagers = NULL;
   Evas_List   *managers, *l, *l2, *l3;
   E_Manager   *man;
   E_Container *con;
   E_Zone      *zone;
   Pager       *pager;
   E_Menu      *mn;
   E_Menu_Item *mi;

   _pager_count = 0;

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
   for (l = managers; l; l = l->next)
     {
	man = l->data;

	for (l2 = man->containers; l2; l2 = l2->next)
	  {
	     con = l2->data;

	     mi = e_menu_item_new(module->config_menu);
	     e_menu_item_label_set(mi, con->name);

	     mn = e_menu_new();
	     e_menu_item_submenu_set(mi, mn);

	     for (l3 = con->zones; l3; l3 = l3->next)
	       {
		  zone = l3->data;

		  pager = _pager_new(zone);
		  if (pager)
		    {
		       /* FIXME : config */
		       pager->enabled = 1;
		       pagers = evas_list_append(pagers, pager);

		       _pager_menu_new(pager);

		       mi = e_menu_item_new(mn);
		       e_menu_item_label_set(mi, zone->name);
		       e_menu_item_submenu_set(mi, pager->menu);
		    }
	       }
	  }
     }
   return pagers;
}

static void
_pager_shutdown(Evas_List *pagers)
{
   Evas_List *list;
/*   free(e->conf); */
/*   E_CONFIG_DD_FREE(e->conf_edd);*/

   for (list = pagers; list; list = list->next)
     _pager_destroy(list->data);
   evas_list_free(pagers);
}

static E_Menu *
_pager_config_menu_new()
{
   E_Menu *mn;

   mn = e_menu_new();

   return mn;
}

static void
_pager_config_menu_del(E_Menu *menu)
{
   e_object_del(E_OBJECT(menu));
}

static Pager *
_pager_new(E_Zone *zone)
{
   Pager       *pager;
   Evas_Object *o;

   pager = E_NEW(Pager, 1);
   if (!pager) return NULL;

   pager->evas = zone->container->bg_evas;

   o = evas_object_rectangle_add(pager->evas);
   pager->base = o;
   evas_object_color_set(o, 128, 128, 128, 0);
   evas_object_pass_events_set(o, 0);
   evas_object_repeat_events_set(o, 0);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _pager_cb_mouse_down, pager);
   evas_object_show(o);

   o = edje_object_add(pager->evas);
   pager->screen = o;
   edje_object_file_set(o,
			/* FIXME: "default.eet" needs to come from conf */
			 e_path_find(path_themes, "default.eet"),
			 "modules/pager/screen");
   evas_object_show(o);

   pager->ev_handler_border_resize =
      ecore_event_handler_add(E_EVENT_BORDER_RESIZE,
			      _pager_cb_event_border_resize, pager);
   pager->ev_handler_border_move =
      ecore_event_handler_add(E_EVENT_BORDER_MOVE,
			      _pager_cb_event_border_move, pager);
   pager->ev_handler_border_add =
      ecore_event_handler_add(E_EVENT_BORDER_ADD,
			      _pager_cb_event_border_add, pager);
   pager->ev_handler_border_remove =
      ecore_event_handler_add(E_EVENT_BORDER_REMOVE,
			      _pager_cb_event_border_remove, pager);
   pager->ev_handler_border_hide =
      ecore_event_handler_add(E_EVENT_BORDER_HIDE,
			      _pager_cb_event_border_hide, pager);
   pager->ev_handler_border_show =
      ecore_event_handler_add(E_EVENT_BORDER_SHOW,
			      _pager_cb_event_border_show, pager);
   pager->ev_handler_border_desk_set =
      ecore_event_handler_add(E_EVENT_BORDER_DESK_SET,
			      _pager_cb_event_border_desk_set, pager);
   pager->ev_handler_zone_desk_count_set =
      ecore_event_handler_add(E_EVENT_ZONE_DESK_COUNT_SET,
			      _pager_cb_event_zone_desk_count_set, pager);

   _pager_zone_set(pager, zone);

   pager->gmc = e_gadman_client_new(pager->zone->container->gadman);
   e_gadman_client_domain_set(pager->gmc, "module.pager", _pager_count++);
   e_gadman_client_zone_set(pager->gmc, pager->zone);
   e_gadman_client_policy_set(pager->gmc,
	 E_GADMAN_POLICY_ANYWHERE |
	 E_GADMAN_POLICY_HMOVE |
	 E_GADMAN_POLICY_VMOVE |
	 E_GADMAN_POLICY_HSIZE |
	 E_GADMAN_POLICY_VSIZE);
   e_gadman_client_min_size_set(pager->gmc, 8, 8);
   e_gadman_client_max_size_set(pager->gmc, 256, 256);
   e_gadman_client_auto_size_set(pager->gmc, 64, 64);
   e_gadman_client_align_set(pager->gmc, 0.0, 1.0);
   e_gadman_client_resize(pager->gmc, 80, 60);
   e_gadman_client_change_func_set(pager->gmc, _pager_cb_gmc_change, pager);
   e_gadman_client_load(pager->gmc);

   return pager;
}

void
_pager_destroy(Pager *pager)
{
   if (pager->base) evas_object_del(pager->base);
   if (pager->screen) evas_object_del(pager->screen);
   e_object_del(E_OBJECT(pager->gmc));

   _pager_zone_leave(pager);
   ecore_event_handler_del(pager->ev_handler_border_resize);
   ecore_event_handler_del(pager->ev_handler_border_move);
   ecore_event_handler_del(pager->ev_handler_border_add);
   ecore_event_handler_del(pager->ev_handler_border_remove);
   ecore_event_handler_del(pager->ev_handler_border_hide);
   ecore_event_handler_del(pager->ev_handler_border_show);
   ecore_event_handler_del(pager->ev_handler_border_desk_set);
   ecore_event_handler_del(pager->ev_handler_zone_desk_count_set);

   _pager_menu_del(pager->menu);

   E_FREE(pager);
   _pager_count--;
}

static void
_pager_menu_new(Pager *pager)
{
   E_Menu *mn;
   E_Menu_Item *mi;

   mn = e_menu_new();

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Enabled");
   e_menu_item_check_set(mi, 1);
   if (pager->enabled) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _pager_cb_menu_enabled, pager);

   pager->menu = mn;
}

static void
_pager_menu_del(E_Menu *menu)
{
   e_object_del(E_OBJECT(menu));
}

static void
_pager_enable(Pager *pager)
{
   Evas_List  *desks, *wins;
   Pager_Desk *pd;
   Pager_Win  *pw;

   pager->enabled = 1;

   evas_object_show(pager->base);
   evas_object_show(pager->screen);
   for (desks = pager->desks; desks; desks = desks->next)
     {
	pd = desks->data;
	evas_object_show(pd->obj);

	for (wins = pd->wins; wins; wins = wins->next)
	  {
	     pw = wins->data;
	     if (!pw->border->iconic)
	       {
		  evas_object_show(pw->obj);
		  if (pw->icon)
		    evas_object_show(pw->icon);
	       }
	  }
     }
}

static void
_pager_disable(Pager *pager)
{
   Evas_List  *desks, *wins;
   Pager_Desk *pd;
   Pager_Win  *pw;

   pager->enabled = 0;

   evas_object_hide(pager->base);
   evas_object_hide(pager->screen);
   for (desks = pager->desks; desks; desks = desks->next)
     {
	pd = desks->data;
	evas_object_hide(pd->obj);

	for (wins = pd->wins; wins; wins = wins->next)
	  {
	     pw = wins->data;
	     if (!pw->border->iconic)
	       {
		  evas_object_hide(pw->obj);
		  if (pw->icon)
		    evas_object_hide(pw->icon);
	       }
	  }
     }
}

static void
_pager_zone_set(Pager *pager, E_Zone *zone)
{
   int          desks_x, desks_y, x, y;
   E_Desk      *current;

   pager->zone = zone;
   e_object_ref(E_OBJECT(zone));
   e_zone_desk_count_get(zone, &desks_x, &desks_y);
   current = e_desk_current_get(zone);
   pager->xnum = desks_x;
   pager->ynum = desks_y;

   for (x = 0; x < desks_x; x++)
     for (y = 0; y < desks_y; y++)
       {
	  Pager_Desk *pd;
	  E_Desk     *desk;

	  desk = e_desk_at_xy_get(zone, x, y);
	  pd = _pager_desk_new(pager, desk, x, y);
	  pager->desks = evas_list_append(pager->desks, pd);
       }
   evas_object_resize(pager->base, pager->fw * desks_x, pager->fh * desks_y);
}

static void
_pager_zone_leave(Pager *pager)
{
   Evas_List *list;
   e_object_unref(E_OBJECT(pager->zone));

   for (list = pager->desks; list; list = list->next)
     _pager_desk_destroy(list->data);
   evas_list_free(pager->desks);
}

static Pager_Desk *
_pager_desk_new(Pager *pager, E_Desk *desk, int xpos, int ypos)
{
   Pager_Desk  *pd;
   Pager_Win   *pw;

   E_Border    *win;

   Evas_Object *o;
   Evas_List   *wins;

   pd = E_NEW(Pager_Desk, 1);
   pd->xpos = xpos;
   pd->ypos = ypos;
   if (desk == e_desk_current_get(desk->zone))
     {
	pd->current = 1;
	evas_object_move(pager->screen, pager->fx + (pager->fw * pd->xpos),
			 pager->fy + (pager->fh * pd->ypos));
     }
   pd->desk = desk;
   e_object_ref(E_OBJECT(desk));
   pd->owner = pager;

   o = edje_object_add(pager->evas);
   pd->obj = o;
   edje_object_file_set(o,
			/* FIXME: "default.eet" needs to come from conf */
			e_path_find(path_themes, "default.eet"),
			"modules/pager/desk");
   evas_object_pass_events_set(o, 1);

   evas_object_resize(o, pager->fw, pager->fh);
   evas_object_move(o, pager->fx + (pd->xpos * pager->fw), pager->fy + (pd->ypos * pager->fh));
   evas_object_show(o);
   evas_object_raise(pager->screen);

   /* Add windows to the desk */
   wins = desk->clients;
   while (wins)
     {
	win = wins->data;
	if (win->new_client)
	  {
	     wins = wins->next;
	     continue;
	  }
	pw = _pager_window_new(pd, win);

	pd->wins = evas_list_append(pd->wins, pw);
	wins = wins->next;
     }
   return pd;
}

static void
_pager_desk_destroy(Pager_Desk *desk)
{
   Evas_List *list;

   if (desk->obj) evas_object_del(desk->obj);
   e_object_unref(E_OBJECT(desk->desk));

   for (list = desk->wins; list; list = list->next)
     _pager_window_destroy(list->data);
   evas_list_free(desk->wins);
   E_FREE(desk);
}

static Pager_Win  *
_pager_window_new(Pager_Desk *owner, E_Border *border)
{
   Pager_Win   *new;
   Evas_Object *o;
   E_App       *app;
   int          visible;
   double       scalex, scaley;

   scalex = owner->owner->fw / owner->owner->zone->w;
   scaley = owner->owner->fy / owner->owner->zone->h;

   new = E_NEW(Pager_Win, 1);
   new->border = border;
   e_object_ref(E_OBJECT(border));
   visible = !border->iconic;
   new->owner = owner;

   o = edje_object_add(owner->owner->evas);
   new->obj = o;
   edje_object_file_set(o,
			/* FIXME: "default.eet" needs to come from conf */
			e_path_find(path_themes, "default.eet"),
			"modules/pager/window");
   evas_object_pass_events_set(o, 1);
   if (visible)
     evas_object_show(o);
   evas_object_raise(owner->owner->screen);
   app = e_app_window_name_class_find(border->client.icccm.name,
				      border->client.icccm.class);
   if (app)
     {
	o = edje_object_add(owner->owner->evas);
	new->icon = o;
	edje_object_file_set(o, app->path, "icon");
	if (visible)
	  evas_object_show(o);
	edje_object_part_swallow(new->obj, "WindowIcon", o);
     }

   _pager_window_move(owner->owner, new);
   return new;
}

static void
_pager_window_destroy(Pager_Win *win)
{
   if (win->obj) evas_object_del(win->obj);
   if (win->icon) evas_object_del(win->icon);

   e_object_unref(E_OBJECT(win->border));
   E_FREE(win);
}

static void
_pager_draw(Pager *pager)
{
   Evas_List  *desks;
   Pager_Desk *desk;
   Evas_List  *wins;
   Pager_Win  *win;

   evas_object_move(pager->base, pager->fx, pager->fy);
   evas_object_resize(pager->base, pager->fw * pager->xnum, pager->fh * pager->ynum);
   evas_object_resize(pager->screen, pager->fw, pager->fh);

   desks = pager->desks;
   while (desks)
     {
	desk = desks->data;
	evas_object_resize(desk->obj, pager->fw, pager->fh);
	evas_object_move(desk->obj, pager->fx + (pager->fw * desk->xpos),
	      pager->fy + (pager->fh * desk->ypos));
	if (desk->current)
	  evas_object_move(pager->screen, pager->fx + (pager->fw * desk->xpos),
		pager->fy + (pager->fh * desk->ypos));

	wins = desk->wins;
	while (wins)
	  {
	     win = wins->data;
	     _pager_window_move(pager, win);

	     wins = wins->next;
	  }
	desks = desks->next;
     }
}

static void
_pager_window_move(Pager *pager, Pager_Win *win)
{
   double       scalex, scaley;
   scalex = (double) pager->fw / pager->zone->w;
   scaley = (double) pager->fh / pager->zone->h;

   evas_object_resize(win->obj, win->border->w * scalex, win->border->h * scaley);
   evas_object_move(win->obj,
		    pager->fx + (win->owner->xpos * pager->fw) + ((win->border->x - pager->zone->x) * scalex),
		    pager->fy + (win->owner->ypos * pager->fh) + ((win->border->y - pager->zone->y) * scaley));
}

static void
_pager_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   Pager *pager;

   ev = event_info;
   pager = data;
   if (ev->button == 3)
     {
	e_menu_activate_mouse(pager->menu, pager->zone,
			      ev->output.x, ev->output.y, 1, 1,
			      E_MENU_POP_DIRECTION_DOWN);
	e_util_container_fake_mouse_up_all_later(pager->zone->container);
     }
   else if (ev->button == 1)
     {
	Evas_Coord  x, y;
	int         xpos, ypos;
	Evas_List  *list;
	Pager_Desk *desk;

	evas_pointer_canvas_xy_get(pager->evas, &x, &y);

	xpos = (x - pager->fx) / pager->fw;
	ypos = (y - pager->fy) / pager->fh;

	for (list = pager->desks; list; list = list->next)
	  {
	     desk = list->data;

	     if ((desk->xpos == xpos) && (desk->ypos == ypos))
	       {
		  desk->current = 1;
		  e_desk_show(desk->desk);
		  evas_object_move(pager->screen,
				   pager->fx + xpos * pager->fw,
				   pager->fy + ypos * pager->fh);
	       }
	     else
	       {
		  desk->current = 0;
	       }
	  }
     }
}

static Pager_Desk *
_pager_desk_find(Pager *pager, E_Desk *desk)
{
   Pager_Desk *pd;
   Evas_List *desks;

   desks = pager->desks;
   while (desks)
     {
	pd = desks->data;
	if (pd->desk == desk)
	  return pd;
	desks = desks->next;
     }
   return NULL;
}

static Pager_Win *
_pager_window_find(Pager *pager, E_Border *border)
{
   Evas_List *desks, *wins;
   Pager_Desk *pd;
   Pager_Win *win;

   desks = pager->desks;
   while (desks)
     {
	pd = desks->data;
	wins = pd->wins;
	while (wins)
	  {
	     win = wins->data;
	     if (win->border == border)
	       return win;
	     wins = wins->next;
	  }
	desks = desks->next;
     }
   return NULL;
}

static void
_pager_cb_gmc_change(void *data, E_Gadman_Client *gmc, E_Gadman_Change change)
{
   Pager      *pager;
   Evas_Coord  x, y, w, h;
   Evas_Coord  deskw, deskh;
   int         xcount, ycount;
   Evas_List  *desks, *wins;
   Pager_Desk *desk;
   Pager_Win  *win;

   pager = data;
   e_zone_desk_count_get(pager->zone, &xcount, &ycount);

   e_gadman_client_geometry_get(pager->gmc, &x, &y, &w, &h);
   deskw = w / xcount;
   deskh = h / ycount;

   pager->fx = x;
   pager->fy = y;
   pager->fw = deskw;
   pager->fh = deskh;
   if (change == E_GADMAN_CHANGE_MOVE_RESIZE)
     {
	_pager_draw(pager);
     }
   else if (change == E_GADMAN_CHANGE_RAISE)
     {
	evas_object_raise(pager->base);

	desks = pager->desks;
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

	evas_object_raise(pager->screen);
     }
}

static int
_pager_cb_event_border_resize(void *data, int type, void *event)
{
   Pager *pager;
   Pager_Win *win;
   E_Event_Border_Resize *ev;

   pager = data;
   ev = event;
   if ((win = _pager_window_find(pager, ev->border)))
     {
	_pager_window_move(pager, win);
     }
   else
     {
	printf("ERROR: event_border_resize %p:%p\n", event, ev->border);
     }
   return 1;
}

static int
_pager_cb_event_border_move(void *data, int type, void *event)
{
   Pager *pager;
   Pager_Win *win;
   E_Event_Border_Resize *ev;

   pager = data;
   ev = event;
   if((win = _pager_window_find(pager, ev->border)))
     {
	_pager_window_move(pager, win);
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
   Pager              *pager;
   E_Event_Border_Add *ev;
   E_Border           *border;
   Pager_Win          *win;
   Pager_Desk         *desk;

   pager = data;
   ev = event;
   border = ev->border;
   if (_pager_window_find(pager, border))
     {
	/* FIXME; may have switched desk! */
	printf("event_border_add, window found :'(\n");
	return 1;
     }
   if ((desk = _pager_desk_find(pager, border->desk)))
     {
	win = _pager_window_new(desk, border);
	desk->wins = evas_list_append(desk->wins, win);
     }
   else
     {
	printf("event_border_add, desk not found :'(\n");
     }
   return 1;
}

static int
_pager_cb_event_border_remove(void *data, int type, void *event)
{
   Pager                 *pager;
   E_Event_Border_Remove *ev;
   E_Border              *border;
   Pager_Win             *old;
   Pager_Desk            *desk;

   pager = data;
   ev = event;
   border = ev->border;

   old = _pager_window_find(pager, border);
   desk = _pager_desk_find(pager, border->desk);
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

	evas_object_raise(e->screen);
     }
   return 1;
}

static int
_pager_cb_event_border_desk_set(void *data, int type, void *event)
{
   Pager      *pager;
   Pager_Win  *win;
   Pager_Desk *desk;
   E_Event_Border_Desk_Set *ev;

   pager = data;
   ev = event;
   win = _pager_window_find(pager, ev->border);
   desk = _pager_desk_find(pager, ev->border->desk);
   if (win && desk)
     {
	win->owner = desk;
	_pager_window_move(pager, win);
     }
   return 1;
}

static int
_pager_cb_event_zone_desk_count_set(void *data, int type, void *event)
{
   Pager      *pager;
   Pager_Desk *pd, *new;

   E_Event_Zone_Desk_Count_Set *ev;
   E_Desk     *desk;

   Evas_List  *desks;

   int desks_x, desks_y;
   int max_x, max_y;
   int x, y;

   pager = data;
   ev = event;
   e_zone_desk_count_get(ev->zone, &desks_x, &desks_y);

   max_x = MAX(pager->xnum, desks_x);
   max_y = MAX(pager->ynum, desks_y);

   if ((pager->xnum == desks_x) && (pager->ynum == desks_y))
     return 1;

   pager->fw = (pager->fw * pager->xnum) / desks_x;
   pager->fh = (pager->fh * pager->ynum) / desks_y;
   for (x = 0; x < max_x; x++)
     {
     for (y = 0; y < max_y; y++)
       {
	  if ((x >= pager->xnum) || (y >= pager->ynum))
	    {
	       /* Add desk */
	       desk = e_desk_at_xy_get(ev->zone, x, y);
	       pd = _pager_desk_new(pager, desk, x, y);
	       pager->desks = evas_list_append(pager->desks, pd);
	    }
	  else if ((x >= desks_x) || (y >= desks_y))
	    {
	       /* Remove desk */
	       for (desks = pager->desks; desks; desks = desks->next)
		 {
		    pd = desks->data;
		    if ((pd->xpos == x) && (pd->ypos == y))
		      break;
		 }
	       if (pd->current)
		 {
		    desk = e_desk_current_get(ev->zone);
		    new = _pager_desk_find(pager, desk);
		    new->current = 1;
		 }
	       pager->desks = evas_list_remove(pager->desks, pd);
	       _pager_desk_destroy(pd);
	    }
       }
     }
   pager->xnum = desks_x;
   pager->ynum = desks_y;

   _pager_draw(pager);
   return 1;
}

static void
_pager_cb_menu_enabled(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Pager *pager;
   unsigned char enabled;

   pager = data;
   enabled = e_menu_item_toggle_get(mi);
   if ((pager->enabled) && (!enabled))
     {  
	_pager_disable(pager);
     }
   else if ((!pager->enabled) && (enabled))
     { 
	_pager_enable(pager);
     }
   e_menu_item_toggle_set(mi, pager->enabled);
}
