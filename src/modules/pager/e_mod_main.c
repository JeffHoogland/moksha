/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

/* TODO
 * which options should be in main menu, and which in face menu?
 * check if a new desk is in the current zone
 */

/* module private routines */
static Pager      *_pager_new();
static void        _pager_free(Pager *pager);
static void        _pager_config_menu_new(Pager *pager);

static Pager_Face *_pager_face_new(E_Zone *zone);
static void        _pager_face_free(Pager_Face *face);
static void        _pager_face_menu_new(Pager_Face *face);
static void        _pager_face_enable(Pager_Face *face);
static void        _pager_face_disable(Pager_Face *face);
static void        _pager_face_zone_set(Pager_Face *face, E_Zone *zone);
static void        _pager_face_zone_leave(Pager_Face *face);

static Pager_Desk *_pager_desk_new(Pager_Face *face, E_Desk *desk, int x, int y);
static void        _pager_desk_free(Pager_Desk *desk);
static Pager_Win  *_pager_window_new(Pager_Desk *desk, E_Border *border);
static void        _pager_window_free(Pager_Win *win);

static void        _pager_face_draw(Pager_Face *face);
static void        _pager_window_move(Pager_Face *face, Pager_Win *win);

static Pager_Desk *_pager_desk_find(Pager_Face *face, E_Desk *desk);
static Pager_Win  *_pager_window_find(Pager_Face *face, E_Border *border);

static void        _pager_face_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void        _pager_face_cb_gmc_change(void *data, E_Gadman_Client *gmc, E_Gadman_Change change);
static int         _pager_face_cb_event_border_resize(void *data, int type, void *event);
static int         _pager_face_cb_event_border_move(void *data, int type, void *event);
static int         _pager_face_cb_event_border_add(void *data, int type, void *event);
static int         _pager_face_cb_event_border_remove(void *data, int type, void *event);
static int         _pager_face_cb_event_border_hide(void *data, int type, void *event);
static int         _pager_face_cb_event_border_show(void *data, int type, void *event);
static int         _pager_face_cb_event_border_stick(void *data, int type, void *event);
static int         _pager_face_cb_event_border_unstick(void *data, int type, void *event);
static int         _pager_face_cb_event_border_desk_set(void *data, int type, void *event);
static int         _pager_face_cb_event_zone_desk_count_set(void *data, int type, void *event);
static int         _pager_face_cb_event_desk_show(void *data, int type, void *event);
static void        _pager_face_cb_menu_enabled(void *data, E_Menu *m, E_Menu_Item *mi);
static void        _pager_face_cb_menu_scale(void *data, E_Menu *m, E_Menu_Item *mi);
static void        _pager_face_cb_menu_resize_none(void *data, E_Menu *m, E_Menu_Item *mi);
static void        _pager_face_cb_menu_resize_horz(void *data, E_Menu *m, E_Menu_Item *mi);
static void        _pager_face_cb_menu_resize_vert(void *data, E_Menu *m, E_Menu_Item *mi);
static void        _pager_face_cb_menu_resize_both(void *data, E_Menu *m, E_Menu_Item *mi);
static void        _pager_face_cb_menu_size_small(void *data, E_Menu *m, E_Menu_Item *mi);
static void        _pager_face_cb_menu_size_medium(void *data, E_Menu *m, E_Menu_Item *mi);
static void        _pager_face_cb_menu_size_large(void *data, E_Menu *m, E_Menu_Item *mi);
static void        _pager_face_cb_menu_edit(void *data, E_Menu *m, E_Menu_Item *mi);

static int         _pager_count;

static E_Config_DD *_conf_edd;
static E_Config_DD *_conf_face_edd;

/* public module routines. all modules must have these */
void *
init(E_Module *module)
{
   Pager *pager = NULL;

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
   pager = _pager_new(module);
   module->config_menu = pager->config_menu;

   return pager;
}

int
shutdown(E_Module *module)
{
   Pager *pager;

   if (module->config_menu)
     module->config_menu = NULL;

   pager = module->data;
   if (pager)
     _pager_free(pager);

   return 1;
}

int
save(E_Module *module)
{
   Pager *pager;

   pager = module->data;
   e_config_domain_save("module.pager", _conf_edd, pager->conf);

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
static Pager *
_pager_new()
{
   Pager       *pager;
   Pager_Face  *face;

   Evas_List   *managers, *l, *l2, *l3, *cl;
   E_Manager   *man;
   E_Container *con;
   E_Zone      *zone;
   E_Menu      *mn;
   E_Menu_Item *mi;

   _pager_count = 0;

   pager = E_NEW(Pager, 1);
   if (!pager) return NULL;

   _conf_face_edd = E_CONFIG_DD_NEW("Pager_Config_Face", Config_Face);
#undef T
#undef D
#define T Config_Face
#define D _conf_face_edd
   E_CONFIG_VAL(D, T, enabled, INT);
   E_CONFIG_VAL(D, T, scale, INT);
   E_CONFIG_VAL(D, T, resize, INT);

   _conf_edd = E_CONFIG_DD_NEW("Pager_Config", Config);
#undef T
#undef D
#define T Config
#define D _conf_edd
   E_CONFIG_LIST(D, T, faces, _conf_face_edd);

   pager->conf = e_config_domain_load("module.pager", _conf_edd);
   if (!pager->conf)
     {
	pager->conf = E_NEW(Config, 1);
     }

   _pager_config_menu_new(pager);

   managers = e_manager_list();
   cl = pager->conf->faces;
   for (l = managers; l; l = l->next)
     {
	man = l->data;

	for (l2 = man->containers; l2; l2 = l2->next)
	  {
	     con = l2->data;

	     mi = e_menu_item_new(pager->config_menu);
	     e_menu_item_label_set(mi, con->name);

	     mn = e_menu_new();
	     e_menu_item_submenu_set(mi, mn);
	     pager->menus = evas_list_append(pager->menus, mn);

	     for (l3 = con->zones; l3; l3 = l3->next)
	       {
		  zone = l3->data;

		  face = _pager_face_new(zone);
		  if (face)
		    {
		       pager->faces = evas_list_append(pager->faces, face);

		       /* Config */
		       if (!cl)
			 {
			    face->conf = E_NEW(Config_Face, 1);
			    face->conf->enabled = 1;
			    face->conf->resize = PAGER_RESIZE_BOTH;
			    face->conf->scale = 1;
			    pager->conf->faces = evas_list_append(pager->conf->faces, face->conf);
			 }
		       else
			 {
			    face->conf = cl->data;
			    cl = cl->next;
			 }

		       /* Menu */
		       /* This menu must be initialized after conf */
		       _pager_face_menu_new(face);

		       mi = e_menu_item_new(mn);
		       e_menu_item_label_set(mi, zone->name);
		       e_menu_item_submenu_set(mi, face->menu);

		       /* Setup */
		       if (!face->conf->enabled)
			 _pager_face_disable(face);
		    }
	       }
	  }
     }
   return pager;
}

static void
_pager_free(Pager *pager)
{
   Evas_List *list;

   E_CONFIG_DD_FREE(_conf_edd);
   E_CONFIG_DD_FREE(_conf_face_edd);

   for (list = pager->faces; list; list = list->next)
     _pager_face_free(list->data);
   evas_list_free(pager->faces);

   for (list = pager->menus; list; list = list->next)
     e_object_del(E_OBJECT(list->data));
   evas_list_free(pager->menus);
   e_object_del(E_OBJECT(pager->config_menu));

   evas_list_free(pager->conf->faces);
   free(pager->conf);
   free(pager);
}

static void
_pager_config_menu_new(Pager *pager)
{
   pager->config_menu = e_menu_new();
}

static Pager_Face *
_pager_face_new(E_Zone *zone)
{
   Pager_Face  *face;
   Evas_Object *o;

   face = E_NEW(Pager_Face, 1);
   if (!face) return NULL;

   face->evas = zone->container->bg_evas;

   o = evas_object_rectangle_add(face->evas);
   face->base = o;
   evas_object_color_set(o, 128, 128, 128, 0);
   evas_object_pass_events_set(o, 0);
   evas_object_repeat_events_set(o, 0);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _pager_face_cb_mouse_down, face);
   evas_object_show(o);

   o = edje_object_add(face->evas);
   face->screen = o;
   edje_object_file_set(o,
			/* FIXME: "default.eet" needs to come from conf */
			 e_path_find(path_themes, "default.eet"),
			 "modules/pager/screen");
   evas_object_show(o);

   face->ev_handler_border_resize =
      ecore_event_handler_add(E_EVENT_BORDER_RESIZE,
			      _pager_face_cb_event_border_resize, face);
   face->ev_handler_border_move =
      ecore_event_handler_add(E_EVENT_BORDER_MOVE,
			      _pager_face_cb_event_border_move, face);
   face->ev_handler_border_add =
      ecore_event_handler_add(E_EVENT_BORDER_ADD,
			      _pager_face_cb_event_border_add, face);
   face->ev_handler_border_remove =
      ecore_event_handler_add(E_EVENT_BORDER_REMOVE,
			      _pager_face_cb_event_border_remove, face);
   face->ev_handler_border_hide =
      ecore_event_handler_add(E_EVENT_BORDER_HIDE,
			      _pager_face_cb_event_border_hide, face);
   face->ev_handler_border_show =
      ecore_event_handler_add(E_EVENT_BORDER_SHOW,
			      _pager_face_cb_event_border_show, face);
   face->ev_handler_border_stick =
      ecore_event_handler_add(E_EVENT_BORDER_STICK,
			      _pager_face_cb_event_border_stick, face);
   face->ev_handler_border_unstick =
      ecore_event_handler_add(E_EVENT_BORDER_UNSTICK,
			      _pager_face_cb_event_border_unstick, face);
   face->ev_handler_border_desk_set =
      ecore_event_handler_add(E_EVENT_BORDER_DESK_SET,
			      _pager_face_cb_event_border_desk_set, face);
   face->ev_handler_zone_desk_count_set =
      ecore_event_handler_add(E_EVENT_ZONE_DESK_COUNT_SET,
			      _pager_face_cb_event_zone_desk_count_set, face);
   face->ev_handler_desk_show =
      ecore_event_handler_add(E_EVENT_DESK_SHOW,
			      _pager_face_cb_event_desk_show, face);

   _pager_face_zone_set(face, zone);

   face->gmc = e_gadman_client_new(face->zone->container->gadman);
   e_gadman_client_domain_set(face->gmc, "module.pager", _pager_count++);
   e_gadman_client_zone_set(face->gmc, face->zone);
   e_gadman_client_policy_set(face->gmc,
			      E_GADMAN_POLICY_ANYWHERE |
			      E_GADMAN_POLICY_HMOVE |
			      E_GADMAN_POLICY_VMOVE |
			      E_GADMAN_POLICY_HSIZE |
			      E_GADMAN_POLICY_VSIZE);
   e_gadman_client_min_size_set(face->gmc, 8, 8);
   e_gadman_client_max_size_set(face->gmc, 256, 256);
   e_gadman_client_auto_size_set(face->gmc, 64, 64);
   e_gadman_client_align_set(face->gmc, 0.0, 1.0);
   e_gadman_client_resize(face->gmc, 80, 60);
   e_gadman_client_change_func_set(face->gmc, _pager_face_cb_gmc_change, face);
   e_gadman_client_load(face->gmc);

   return face;
}

void
_pager_face_free(Pager_Face *face)
{
   if (face->base) evas_object_del(face->base);
   if (face->screen) evas_object_del(face->screen);
   e_gadman_client_save(face->gmc);
   e_object_del(E_OBJECT(face->gmc));

   _pager_face_zone_leave(face);
   ecore_event_handler_del(face->ev_handler_border_resize);
   ecore_event_handler_del(face->ev_handler_border_move);
   ecore_event_handler_del(face->ev_handler_border_add);
   ecore_event_handler_del(face->ev_handler_border_remove);
   ecore_event_handler_del(face->ev_handler_border_hide);
   ecore_event_handler_del(face->ev_handler_border_show);
   ecore_event_handler_del(face->ev_handler_border_stick);
   ecore_event_handler_del(face->ev_handler_border_unstick);
   ecore_event_handler_del(face->ev_handler_border_desk_set);
   ecore_event_handler_del(face->ev_handler_zone_desk_count_set);
   ecore_event_handler_del(face->ev_handler_desk_show);

   e_object_del(E_OBJECT(face->menu));

   free(face->conf);
   free(face);
   _pager_count--;
}

static void
_pager_face_menu_new(Pager_Face *face)
{
   E_Menu *mn;
   E_Menu_Item *mi;

   mn = e_menu_new();
   face->menu = mn;

   /* Enabled */
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Enabled");
   e_menu_item_check_set(mi, 1);
   if (face->conf->enabled) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _pager_face_cb_menu_enabled, face);

   /* Edit */
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Edit Mode");
   e_menu_item_callback_set(mi, _pager_face_cb_menu_edit, face);

   /* Scale */
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Keep Scale");
   e_menu_item_check_set(mi, 1);
   if (face->conf->scale) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _pager_face_cb_menu_scale, face);

   /* Resize */
   mi = e_menu_item_new(face->menu);
   e_menu_item_label_set(mi, "Auto Resize");
   mn = e_menu_new();
   e_menu_item_submenu_set(mi, mn);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "None");
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (face->conf->resize == PAGER_RESIZE_NONE) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _pager_face_cb_menu_resize_none, face);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Horizontal");
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (face->conf->resize == PAGER_RESIZE_HORZ) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _pager_face_cb_menu_resize_horz, face);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Vertical");
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (face->conf->resize == PAGER_RESIZE_VERT) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _pager_face_cb_menu_resize_vert, face);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Both");
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (face->conf->resize == PAGER_RESIZE_BOTH) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _pager_face_cb_menu_resize_both, face);

   /* Size */
   mi = e_menu_item_new(face->menu);
   e_menu_item_label_set(mi, "Size");
   mn = e_menu_new();
   e_menu_item_submenu_set(mi, mn);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Small");
   e_menu_item_callback_set(mi, _pager_face_cb_menu_size_small, face);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Medium");
   e_menu_item_callback_set(mi, _pager_face_cb_menu_size_medium, face);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Large");
   e_menu_item_callback_set(mi, _pager_face_cb_menu_size_large, face);
}

static void
_pager_face_enable(Pager_Face *face)
{
   Evas_List  *desks, *wins;
   Pager_Desk *desk;
   Pager_Win  *win;

   face->conf->enabled = 1;

   evas_object_show(face->base);
   evas_object_show(face->screen);
   for (desks = face->desks; desks; desks = desks->next)
     {
	desk = desks->data;
	evas_object_show(desk->obj);

	for (wins = desk->wins; wins; wins = wins->next)
	  {
	     win = wins->data;
	     if (!win->border->iconic)
	       {
		  evas_object_show(win->obj);
		  if (win->icon)
		    evas_object_show(win->icon);
	       }
	  }
     }
   e_config_save_queue();
}

static void
_pager_face_disable(Pager_Face *face)
{
   Evas_List  *desks, *wins;
   Pager_Desk *desk;
   Pager_Win  *win;

   face->conf->enabled = 0;

   evas_object_hide(face->base);
   evas_object_hide(face->screen);
   for (desks = face->desks; desks; desks = desks->next)
     {
	desk = desks->data;
	evas_object_hide(desk->obj);

	for (wins = desk->wins; wins; wins = wins->next)
	  {
	     win = wins->data;
	     if (!win->border->iconic)
	       {
		  evas_object_hide(win->obj);
		  if (win->icon)
		    evas_object_hide(win->icon);
	       }
	  }
     }
   e_config_save_queue();
}

static void
_pager_face_zone_set(Pager_Face *face, E_Zone *zone)
{
   int          desks_x, desks_y, x, y;
   E_Desk      *current;

   face->zone = zone;
   e_object_ref(E_OBJECT(zone));
   e_zone_desk_count_get(zone, &desks_x, &desks_y);
   current = e_desk_current_get(zone);
   face->xnum = desks_x;
   face->ynum = desks_y;

   for (x = 0; x < desks_x; x++)
     for (y = 0; y < desks_y; y++)
       {
	  Pager_Desk *pd;
	  E_Desk     *desk;

	  desk = e_desk_at_xy_get(zone, x, y);
	  pd = _pager_desk_new(face, desk, x, y);
	  if (pd)
	    face->desks = evas_list_append(face->desks, pd);
       }
   evas_object_resize(face->base, face->fw * face->xnum, face->fh * face->ynum);
}

static void
_pager_face_zone_leave(Pager_Face *face)
{
   Evas_List *list;
   e_object_unref(E_OBJECT(face->zone));

   for (list = face->desks; list; list = list->next)
     _pager_desk_free(list->data);

   evas_list_free(face->desks);
}

static Pager_Desk *
_pager_desk_new(Pager_Face *face, E_Desk *desk, int xpos, int ypos)
{
   Pager_Desk  *pd;
   Pager_Win   *pw;

   E_Border    *win;

   Evas_Object *o;
   Evas_List   *wins;

   pd = E_NEW(Pager_Desk, 1);
   if (!pd) return NULL;

   pd->xpos = xpos;
   pd->ypos = ypos;

   if (desk == e_desk_current_get(desk->zone))
     {
	pd->current = 1;
	evas_object_move(face->screen,
			 face->fx + (face->fw * pd->xpos),
			 face->fy + (face->fh * pd->ypos));
     }

   pd->desk = desk;
   e_object_ref(E_OBJECT(desk));
   pd->face = face;

   o = edje_object_add(face->evas);
   pd->obj = o;
   edje_object_file_set(o,
			/* FIXME: "default.eet" needs to come from conf */
			e_path_find(path_themes, "default.eet"),
			"modules/pager/desk");
   evas_object_pass_events_set(o, 1);

   evas_object_resize(o, face->fw, face->fh);
   evas_object_move(o, face->fx + (pd->xpos * face->fw), face->fy + (pd->ypos * face->fh));
   evas_object_show(o);
   evas_object_raise(face->screen);

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
	if (pw)
	  pd->wins = evas_list_append(pd->wins, pw);

	wins = wins->next;
     }
   return pd;
}

static void
_pager_desk_free(Pager_Desk *desk)
{
   Evas_List *list;

   if (desk->obj) evas_object_del(desk->obj);
   e_object_unref(E_OBJECT(desk->desk));

   for (list = desk->wins; list; list = list->next)
     _pager_window_free(list->data);
   evas_list_free(desk->wins);
   free(desk);
}

static Pager_Win  *
_pager_window_new(Pager_Desk *desk, E_Border *border)
{
   Pager_Win   *win;
   Evas_Object *o;
   E_App       *app;
   int          visible;
   double       scalex, scaley;

   scalex = (double)desk->face->fw / (double)desk->face->zone->w;
   scaley = (double)desk->face->fy / (double)desk->face->zone->h;

   win = E_NEW(Pager_Win, 1);
   if (!win) return NULL;

   win->border = border;
   e_object_ref(E_OBJECT(border));
   visible = !border->iconic;
   win->desk = desk;

   o = edje_object_add(desk->face->evas);
   win->obj = o;
   edje_object_file_set(o,
			/* FIXME: "default.eet" needs to come from conf */
			e_path_find(path_themes, "default.eet"),
			"modules/pager/window");
   evas_object_pass_events_set(o, 1);
   if (visible)
     evas_object_show(o);
   evas_object_raise(desk->face->screen);
   app = e_app_window_name_class_find(border->client.icccm.name,
				      border->client.icccm.class);
   if (app)
     {
	o = edje_object_add(desk->face->evas);
	win->icon = o;
	edje_object_file_set(o, app->path, "icon");
	if (visible)
	  evas_object_show(o);
	edje_object_part_swallow(win->obj, "WindowIcon", o);
     }

   _pager_window_move(desk->face, win);
   return win;
}

static void
_pager_window_free(Pager_Win *win)
{
   if (win->obj) evas_object_del(win->obj);
   if (win->icon) evas_object_del(win->icon);

   e_object_unref(E_OBJECT(win->border));
   free(win);
}

static void
_pager_face_draw(Pager_Face *face)
{
   Evas_List  *desks, *wins;
   Pager_Desk *desk;
   Pager_Win  *win;

   evas_object_move(face->base, face->fx, face->fy);
   evas_object_resize(face->base, face->fw * face->xnum, face->fh * face->ynum);
   evas_object_resize(face->screen, face->fw, face->fh);

   desks = face->desks;
   while (desks)
     {
	desk = desks->data;
	evas_object_resize(desk->obj, face->fw, face->fh);
	evas_object_move(desk->obj, face->fx + (face->fw * desk->xpos),
			 face->fy + (face->fh * desk->ypos));
	if (desk->current)
	  evas_object_move(face->screen, face->fx + (face->fw * desk->xpos),
			   face->fy + (face->fh * desk->ypos));

	wins = desk->wins;
	while (wins)
	  {
	     win = wins->data;
	     _pager_window_move(face, win);

	     wins = wins->next;
	  }
	desks = desks->next;
     }
}

static void
_pager_window_move(Pager_Face *face, Pager_Win *win)
{
   double       scalex, scaley;

   scalex = (double)face->fw / (double)face->zone->w;
   scaley = (double)face->fh / (double)face->zone->h;

   evas_object_resize(win->obj, win->border->w * scalex, win->border->h * scaley);
   evas_object_move(win->obj,
		    face->fx + (win->desk->xpos * face->fw) + ((win->border->x - face->zone->x) * scalex),
		    face->fy + (win->desk->ypos * face->fh) + ((win->border->y - face->zone->y) * scaley));
}

static void
_pager_face_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Pager_Face            *face;
   Evas_Event_Mouse_Down *ev;

   face = data;
   ev = event_info;
   if (ev->button == 3)
     {
	e_menu_activate_mouse(face->menu, face->zone,
			      ev->output.x, ev->output.y, 1, 1,
			      E_MENU_POP_DIRECTION_DOWN);
	e_util_container_fake_mouse_up_all_later(face->zone->container);
     }
   else if (ev->button == 1)
     {
	Evas_Coord  x, y;
	int         xpos, ypos;
	Evas_List  *list;
	Pager_Desk *desk;

	evas_pointer_canvas_xy_get(face->evas, &x, &y);

	xpos = (x - face->fx) / face->fw;
	ypos = (y - face->fy) / face->fh;

	for (list = face->desks; list; list = list->next)
	  {
	     desk = list->data;

	     if ((desk->xpos == xpos) && (desk->ypos == ypos))
	       {
		  desk->current = 1;
		  e_desk_show(desk->desk);
	       }
	     else
	       {
		  desk->current = 0;
	       }
	  }
     }
}

static Pager_Desk *
_pager_desk_find(Pager_Face *face, E_Desk *desk)
{
   Pager_Desk *pd;
   Evas_List  *desks;

   desks = face->desks;
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
_pager_window_find(Pager_Face *face, E_Border *border)
{
   Pager_Desk *desk;
   Pager_Win  *win;
   Evas_List  *desks, *wins;

   desks = face->desks;
   while (desks)
     {
	desk = desks->data;
	wins = desk->wins;
	while (wins)
	  {
	     win = wins->data;
	     /* We have to check the desk, wouldn't want
	      * a sticky copy */
	     if ((!win->border->sticky || (win->border->desk == desk->desk))
		 && (win->border == border))
	       return win;
	     wins = wins->next;
	  }
	desks = desks->next;
     }
   return NULL;
}

static void
_pager_face_cb_gmc_change(void *data, E_Gadman_Client *gmc, E_Gadman_Change change)
{
   Pager_Face *face;
   Pager_Win  *win;
   Pager_Desk *desk;
   Evas_Coord  x, y, w, h;
   Evas_Coord  deskw, deskh;
   int         xcount, ycount;
   Evas_List  *desks, *wins;

   face = data;
   e_zone_desk_count_get(face->zone, &xcount, &ycount);

   e_gadman_client_geometry_get(face->gmc, &x, &y, &w, &h);
   deskw = w / xcount;
   deskh = h / ycount;

   face->fx = x;
   face->fy = y;
   face->fw = deskw;
   face->fh = deskh;
   switch (change)
     {
      case E_GADMAN_CHANGE_MOVE_RESIZE:
	 _pager_face_draw(face);
	 break;
      case E_GADMAN_CHANGE_RAISE:
	 evas_object_raise(face->base);

	 desks = face->desks;
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

	 evas_object_raise(face->screen);
	 break;
      case E_GADMAN_CHANGE_EDGE:
      case E_GADMAN_CHANGE_ZONE:
	 /* FIXME
	  * Must we do something here?
	  */
	 break;
     }
}

static int
_pager_face_cb_event_border_resize(void *data, int type, void *event)
{
   Pager_Face            *face;
   Pager_Desk            *desk;
   Pager_Win             *win;
   E_Event_Border_Resize *ev;
   Evas_List             *desks, *wins;

   face = data;
   ev = event;
   /* Only care about windows in our zone */
   if (face->zone != ev->border->zone)
     return 1;

   for (desks = face->desks; desks; desks = desks->next)
     {
	desk = desks->data;
	for (wins = desk->wins; wins; wins = wins->next)
	  {
	     win = wins->data;
	     if (win->border == ev->border)
	       {
		  _pager_window_move(face, win);
		  break;
	       }
	  }
     }
   return 1;
}

static int
_pager_face_cb_event_border_move(void *data, int type, void *event)
{
   Pager_Face            *face;
   Pager_Desk            *desk;
   Pager_Win             *win;
   E_Event_Border_Move   *ev;
   Evas_List             *desks, *wins;

   face = data;
   ev = event;
   /* Only care about windows in our zone */
   if (face->zone != ev->border->zone)
     return 1;

   for (desks = face->desks; desks; desks = desks->next)
     {
	desk = desks->data;
	for (wins = desk->wins; wins; wins = wins->next)
	  {
	     win = wins->data;
	     if (win->border == ev->border)
	       {
		  _pager_window_move(face, win);
		  break;
	       }
	  }
     }
   return 1;
}

static int
_pager_face_cb_event_border_add(void *data, int type, void *event)
{
   Pager_Face         *face;
   Pager_Desk         *desk;
   Pager_Win          *win;
   E_Event_Border_Add *ev;

   face = data;
   ev = event;
   /* Only care about windows in our zone */
   if (face->zone != ev->border->zone)
     return 1;

#if 0
   if (_pager_window_find(face, ev->border))
     {
	printf("event_border_add, window found :'(\n");
	return 1;
     }
#endif
   if ((desk = _pager_desk_find(face, ev->border->desk)))
     {
	win = _pager_window_new(desk, ev->border);
	if (win)
	  desk->wins = evas_list_append(desk->wins, win);
     }
   else
     {
	printf("event_border_add, desk not found :'(\n");
     }
   return 1;
}

static int
_pager_face_cb_event_border_remove(void *data, int type, void *event)
{
   Pager_Face            *face;
   Pager_Desk            *desk;
   Pager_Win             *win;
   E_Event_Border_Remove *ev;
   Evas_List             *desks, *wins;

   face = data;
   ev = event;
   /* Only care about windows in our zone */
   if (face->zone != ev->border->zone)
     return 1;

   for (desks = face->desks; desks; desks = desks->next)
     {
	desk = desks->data;
	for (wins = desk->wins; wins; wins = wins->next)
	  {
	     win = wins->data;
	     if (win->border == ev->border)
	       {
		  desk->wins = evas_list_remove_list(desk->wins, wins);
		  _pager_window_free(win);
		  break;
	       }
	  }
     }
   return 1;
}

static int
_pager_face_cb_event_border_hide(void *data, int type, void *event)
{
   Pager_Face          *face;
   Pager_Desk          *desk;
   Pager_Win           *win;
   E_Event_Border_Hide *ev;
   Evas_List             *desks, *wins;

   face = data;
   ev = event;
   /* Only care about windows in our zone */
   if (face->zone != ev->border->zone)
     return 1;

   for (desks = face->desks; desks; desks = desks->next)
     {
	desk = desks->data;
	for (wins = desk->wins; wins; wins = wins->next)
	  {
	     win = wins->data;
	     if ((win->border == ev->border)
		 && (ev->border->desk->visible))
	       {
		  evas_object_hide(win->obj);
		  evas_object_hide(win->icon);
		  break;
	       }
	  }
     }
   return 1;
}

static int
_pager_face_cb_event_border_show(void *data, int type, void *event)
{
   Pager_Face          *face;
   Pager_Desk          *desk;
   Pager_Win           *win;
   E_Event_Border_Show *ev;
   Evas_List             *desks, *wins;

   face = data;
   ev = event;
   /* Only care about windows in our zone */
   if (face->zone != ev->border->zone)
     return 1;

   for (desks = face->desks; desks; desks = desks->next)
     {
	desk = desks->data;
	for (wins = desk->wins; wins; wins = wins->next)
	  {
	     win = wins->data;
	     if ((win->border == ev->border)
		 && (ev->border->desk->visible))
	       {
		  evas_object_show(win->obj);
		  evas_object_show(win->icon);
		  break;
	       }
	  }
     }
   evas_object_raise(face->screen);
   return 1;
}

static int
_pager_face_cb_event_border_stick(void *data, int type, void *event)
{
   Pager_Face           *face;
   Pager_Desk           *desk;
   Pager_Win            *win;
   E_Event_Border_Stick *ev;
   Evas_List            *desks;

   face = data;
   ev = event;
   /* Only care about windows in our zone */
   if (face->zone != ev->border->zone)
     return 1;

   for (desks = face->desks; desks; desks = desks->next)
     {
	desk = desks->data;
	/* On this desk there should already be a border */
	if (ev->border->desk == desk->desk)
	  continue;

	win = _pager_window_new(desk, ev->border);
	desk->wins = evas_list_append(desk->wins, win);
     }
   return 1;
}

static int
_pager_face_cb_event_border_unstick(void *data, int type, void *event)
{
   Pager_Face           *face;
   Pager_Desk           *desk;
   Pager_Win            *win;
   E_Event_Border_Unstick *ev;
   Evas_List            *desks, *wins;

   face = data;
   ev = event;
   /* Only care about windows in our zone */
   if (face->zone != ev->border->zone)
     return 1;


   for (desks = face->desks; desks; desks = desks->next)
     {
	desk = desks->data;
	/* On this desk there should be a border */
	if (desk->desk == ev->border->desk)
	  continue;

	for (wins = desk->wins; wins; wins = wins->next)
	  {
	     win = wins->data;
	     if (win->border == ev->border)
	       {
		  desk->wins = evas_list_remove_list(desk->wins, wins);
		  _pager_window_free(win);
		  break;
	       }
	  }
     }
   return 1;
}

static int
_pager_face_cb_event_border_desk_set(void *data, int type, void *event)
{
   Pager_Face              *face;
   Pager_Win               *win;
   Pager_Desk              *desk;
   E_Event_Border_Desk_Set *ev;

   face = data;
   ev = event;
   /* Only care about windows in our zone */
   if (face->zone != ev->border->zone)
     return 1;

   if (ev->border->sticky)
     return 1;

   win = _pager_window_find(face, ev->border);
   desk = _pager_desk_find(face, ev->border->desk);
   if (win && desk)
     {
	evas_list_remove(win->desk->wins, win);
	win->desk = desk;
	desk->wins = evas_list_append(desk->wins, win);
	_pager_window_move(face, win);
     }
   return 1;
}

static int
_pager_face_cb_event_zone_desk_count_set(void *data, int type, void *event)
{
   Pager_Face *face;
   Pager_Desk *pd, *new;

   E_Event_Zone_Desk_Count_Set *ev;
   E_Desk     *desk;

   Evas_List  *desks;

   int desks_x, desks_y;
   int max_x, max_y;
   int x, y;
   int w, h, nw, nh;
   double scale;

   face = data;
   ev = event;

   if (face->zone != ev->zone)
     return 1;

   e_zone_desk_count_get(ev->zone, &desks_x, &desks_y);

   if ((face->xnum == desks_x) && (face->ynum == desks_y))
     return 1;

   max_x = MAX(face->xnum, desks_x);
   max_y = MAX(face->ynum, desks_y);

   w = face->fw * face->xnum;
   h = face->fh * face->ynum;

   if ((face->conf->resize == PAGER_RESIZE_HORZ)
       && (face->ynum != desks_y))
     {
	/* fw is kept, so the pager face width will change
	 * if xnum changes */
	scale = (double)face->fw / (double)face->fh;
	face->fh = (face->fh * face->ynum) / desks_y;
	if (face->conf->scale)
	  face->fw = face->fh * scale;
     }
   else if ((face->conf->resize == PAGER_RESIZE_VERT)
	    && (face->xnum != desks_x))
     {
	/* fh is kept, so the pager face height will change 
	 * if ynum changes */
	scale = (double)face->fh / (double)face->fw;
	face->fw = (face->fw * face->xnum) / desks_x;
	if (face->conf->scale)
	  face->fh = face->fw * scale;
     }
   else if (face->conf->resize == PAGER_RESIZE_NONE)
     {
	face->fw = (face->fw * face->xnum) / desks_x;
	face->fh = (face->fh * face->ynum) / desks_y;
     }

   for (x = 0; x < max_x; x++)
     {
     for (y = 0; y < max_y; y++)
       {
	  if ((x >= face->xnum) || (y >= face->ynum))
	    {
	       /* Add desk */
	       desk = e_desk_at_xy_get(ev->zone, x, y);
	       pd = _pager_desk_new(face, desk, x, y);
	       if (pd)
		 face->desks = evas_list_append(face->desks, pd);
	    }
	  else if ((x >= desks_x) || (y >= desks_y))
	    {
	       /* Remove desk */
	       for (desks = face->desks; desks; desks = desks->next)
		 {
		    pd = desks->data;
		    if ((pd->xpos == x) && (pd->ypos == y))
		      break;
		 }
	       if (pd->current)
		 {
		    desk = e_desk_current_get(ev->zone);
		    new = _pager_desk_find(face, desk);
		    new->current = 1;
		 }
	       face->desks = evas_list_remove(face->desks, pd);
	       _pager_desk_free(pd);
	    }
       }
     }
   face->xnum = desks_x;
   face->ynum = desks_y;
   nw = face->fw * face->xnum;
   nh = face->fh * face->ynum;

   if ((nw != w) || (nh != h))
     {
	e_gadman_client_resize(face->gmc, face->fw * face->xnum, face->fh * face->ynum);
     }
   else
     _pager_face_draw(face);
   return 1;
}

static int
_pager_face_cb_event_desk_show(void *data, int type, void *event)
{
   Pager_Face        *face;
   Pager_Desk        *desk;
   E_Event_Desk_Show *ev;
   Evas_List         *desks;

   face = data;
   ev = event;
   /* Only care if this desk is in our zone */
   if (face->zone != ev->desk->zone)
     return 1;

   for (desks = face->desks; desks; desks = desks->next)
     {
	desk = desks->data;
	if (desk->desk == ev->desk)
	  {
	     desk->current = 1;
	     evas_object_move(face->screen,
			      face->fx + (face->fw * desk->xpos),
			      face->fy + (face->fh * desk->ypos));
	  }
	else
	  desk->current = 0;
     }
   return 1;
}

static void
_pager_face_cb_menu_enabled(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Pager_Face *face;
   unsigned char enabled;

   face = data;
   enabled = e_menu_item_toggle_get(mi);
   if ((face->conf->enabled) && (!enabled))
     {  
	_pager_face_disable(face);
     }
   else if ((!face->conf->enabled) && (enabled))
     { 
	_pager_face_enable(face);
     }
   e_menu_item_toggle_set(mi, face->conf->enabled);
}

static void
_pager_face_cb_menu_scale(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Pager_Face *face;

   face = data;

   face->conf->scale = e_menu_item_toggle_get(mi);
   e_menu_item_toggle_set(mi, face->conf->scale);
   e_config_save_queue();
}

static void
_pager_face_cb_menu_resize_none(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Pager_Face *face;

   face = data;
   face->conf->resize = PAGER_RESIZE_NONE;
   e_config_save_queue();
}

static void
_pager_face_cb_menu_resize_horz(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Pager_Face *face;

   face = data;
   face->conf->resize = PAGER_RESIZE_HORZ;
   e_config_save_queue();
}

static void
_pager_face_cb_menu_resize_vert(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Pager_Face *face;

   face = data;
   face->conf->resize = PAGER_RESIZE_VERT;
   e_config_save_queue();
}

static void
_pager_face_cb_menu_resize_both(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Pager_Face *face;

   face = data;
   face->conf->resize = PAGER_RESIZE_BOTH;
   e_config_save_queue();
}

static void
_pager_face_cb_menu_size_small(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Pager_Face *face;

   face = data;
   face->fw = 40;
   face->fh = 30;
   e_gadman_client_resize(face->gmc, face->fw * face->xnum, face->fh * face->ynum);
}

static void
_pager_face_cb_menu_size_medium(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Pager_Face *face;

   face = data;
   face->fw = 80;
   face->fh = 60;
   e_gadman_client_resize(face->gmc, face->fw * face->xnum, face->fh * face->ynum);
}

static void
_pager_face_cb_menu_size_large(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Pager_Face *face;

   face = data;
   face->fw = 120;
   face->fh = 90;
   e_gadman_client_resize(face->gmc, face->fw * face->xnum, face->fh * face->ynum);
}

static void
_pager_face_cb_menu_edit(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Pager_Face *face;

   face = data;
   e_gadman_mode_set(face->gmc->gadman, E_GADMAN_MODE_EDIT);
}
