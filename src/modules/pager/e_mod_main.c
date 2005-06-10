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
static void        _pager_face_zone_unset(Pager_Face *face);

static Pager_Desk *_pager_desk_new(Pager_Face *face, E_Desk *desk, int x, int y);
static void        _pager_desk_free(Pager_Desk *pd);

static Pager_Win  *_pager_window_new(Pager_Desk *pd, E_Border *border);
static void        _pager_window_free(Pager_Win *pw);
static void        _pager_window_move(Pager_Face *face, Pager_Win *pw);

static Pager_Win  *_pager_face_border_find(Pager_Face *face, E_Border *border);
static Pager_Win  *_pager_desk_border_find(Pager_Desk *pd, E_Border *border);
static Pager_Desk *_pager_face_desk_find(Pager_Face *face, E_Desk *desk);
static void        _pager_face_desk_select(Pager_Desk *pd);

static void        _pager_face_cb_gmc_change(void *data, E_Gadman_Client *gmc, E_Gadman_Change change);
static int         _pager_face_cb_event_border_resize(void *data, int type, void *event);
static int         _pager_face_cb_event_border_move(void *data, int type, void *event);
static int         _pager_face_cb_event_border_add(void *data, int type, void *event);
static int         _pager_face_cb_event_border_remove(void *data, int type, void *event);
static int         _pager_face_cb_event_border_iconify(void *data, int type, void *event);
static int         _pager_face_cb_event_border_uniconify(void *data, int type, void *event);
static int         _pager_face_cb_event_border_stick(void *data, int type, void *event);
static int         _pager_face_cb_event_border_unstick(void *data, int type, void *event);
static int         _pager_face_cb_event_border_desk_set(void *data, int type, void *event);
static int         _pager_face_cb_event_border_raise(void *data, int type, void *event);
static int         _pager_face_cb_event_border_lower(void *data, int type, void *event);
static int         _pager_face_cb_event_border_icon_change(void *data, int type, void *event);
static int         _pager_face_cb_event_zone_desk_count_set(void *data, int type, void *event);
static int         _pager_face_cb_event_desk_show(void *data, int type, void *event);
static int         _pager_face_cb_event_container_resize(void *data, int type, void *event);
static void        _pager_face_cb_menu_enabled(void *data, E_Menu *m, E_Menu_Item *mi);
static void        _pager_face_cb_menu_edit(void *data, E_Menu *m, E_Menu_Item *mi);

static void        _pager_desk_cb_mouse_in(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void        _pager_desk_cb_mouse_out(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void        _pager_desk_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void        _pager_desk_cb_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void        _pager_desk_cb_mouse_move(void *data, Evas *e, Evas_Object *obj, void *event_info);

static void        _pager_desk_cb_intercept_move(void *data, Evas_Object *o, Evas_Coord x, Evas_Coord y);
static void        _pager_desk_cb_intercept_resize(void *data, Evas_Object *o, Evas_Coord w, Evas_Coord h);

static void        _pager_face_cb_enter(void *data, const char *type, void *drop);
static void        _pager_face_cb_move(void *data, const char *type, void *drop);
static void        _pager_face_cb_leave(void *data, const char *type, void *drop);
static void        _pager_face_cb_drop(void *data, const char *type, void *drop);

static int         _pager_count;

static E_Config_DD *_conf_edd;
static E_Config_DD *_conf_face_edd;

/* public module routines. all modules must have these */
void *
e_modapi_init(E_Module *module)
{
   Pager *pager = NULL;

   /* check module api version */
   if (module->api->version < E_MODULE_API_VERSION)
     {
	e_error_dialog_show(_("Module API Error"),
			    _("Error initializing Module: Pager\n"
			      "It requires a minimum module API version of: %i.\n"
			      "The module API advertized by Enlightenment is: %i.\n"
			      "Aborting module."),
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
e_modapi_shutdown(E_Module *module)
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
e_modapi_save(E_Module *module)
{
   Pager *pager;

   pager = module->data;
   e_config_domain_save("module.pager", _conf_edd, pager->conf);

   return 1;
}

int
e_modapi_info(E_Module *module)
{
   char buf[4096];

   module->label = strdup(_("Pager"));
   snprintf(buf, sizeof(buf), "%s/module_icon.png", e_module_dir_get(module));
   module->icon_file = strdup(buf);
   return 1;
}

int
e_modapi_about(E_Module *module)
{
   e_error_dialog_show(_("Enlightenment Pager Module"),
		       _("A pager module to navigate virtual desktops."));
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
   E_CONFIG_VAL(D, T, enabled, UCHAR);
   E_CONFIG_VAL(D, T, scale, UCHAR);
   E_CONFIG_VAL(D, T, resize, UCHAR);

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
   Evas_List *l;

   E_CONFIG_DD_FREE(_conf_edd);
   E_CONFIG_DD_FREE(_conf_face_edd);

   for (l = pager->faces; l; l = l->next)
     _pager_face_free(l->data);
   evas_list_free(pager->faces);

   for (l = pager->menus; l; l = l->next)
     e_object_del(E_OBJECT(l->data));
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
   Evas_Coord   x, y, w, h;

   face = E_NEW(Pager_Face, 1);
   if (!face) return NULL;

   /* store what evas we live in */
   face->evas = zone->container->bg_evas;
   
   /* set up event handles for when windows change and desktops */
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
   face->ev_handler_border_iconify =
      ecore_event_handler_add(E_EVENT_BORDER_ICONIFY,
			      _pager_face_cb_event_border_iconify, face);
   face->ev_handler_border_uniconify =
      ecore_event_handler_add(E_EVENT_BORDER_UNICONIFY,
			      _pager_face_cb_event_border_uniconify, face);
   face->ev_handler_border_stick =
      ecore_event_handler_add(E_EVENT_BORDER_STICK,
			      _pager_face_cb_event_border_stick, face);
   face->ev_handler_border_unstick =
      ecore_event_handler_add(E_EVENT_BORDER_UNSTICK,
			      _pager_face_cb_event_border_unstick, face);
   face->ev_handler_border_desk_set =
      ecore_event_handler_add(E_EVENT_BORDER_DESK_SET,
			      _pager_face_cb_event_border_desk_set, face);
   face->ev_handler_border_raise =
      ecore_event_handler_add(E_EVENT_BORDER_RAISE,
			      _pager_face_cb_event_border_raise, face);
   face->ev_handler_border_lower =
      ecore_event_handler_add(E_EVENT_BORDER_LOWER,
			      _pager_face_cb_event_border_lower, face);
   face->ev_handler_border_icon_change =
      ecore_event_handler_add(E_EVENT_BORDER_ICON_CHANGE,
			      _pager_face_cb_event_border_icon_change, face);
   face->ev_handler_zone_desk_count_set =
      ecore_event_handler_add(E_EVENT_ZONE_DESK_COUNT_SET,
			      _pager_face_cb_event_zone_desk_count_set, face);
   face->ev_handler_desk_show =
      ecore_event_handler_add(E_EVENT_DESK_SHOW,
			      _pager_face_cb_event_desk_show, face);
   face->ev_handler_container_resize =
      ecore_event_handler_add(E_EVENT_CONTAINER_RESIZE,
			      _pager_face_cb_event_container_resize, face);

   /* the bg */
   o = edje_object_add(face->evas);
   face->pager_object = o;
   e_theme_edje_object_set(o, "base/theme/modules/pager",
			   "modules/pager/main");
   evas_object_show(o);
   
   o = e_table_add(face->evas);
   face->table_object = o;
   e_table_homogenous_set(o, 1);   
   edje_object_part_swallow(face->pager_object, "items", face->table_object);
   evas_object_show(o);

   evas_object_resize(face->pager_object, 1000, 1000);
   edje_object_calc_force(face->pager_object);
   edje_object_part_geometry_get(face->pager_object, "items", &x, &y, &w, &h);
   face->inset.l = x;
   face->inset.r = 1000 - (x + w);
   face->inset.t = y;
   face->inset.b = 1000 - (y + h);

   face->drop_handler = e_drop_handler_add(face,
					   _pager_face_cb_enter, _pager_face_cb_move,
					   _pager_face_cb_leave, _pager_face_cb_drop,
					   "enlightenment/border",
					   face->fx, face->fy, face->fw, face->fh);
   
   face->gmc = e_gadman_client_new(zone->container->gadman);
   _pager_face_zone_set(face, zone);
   
   e_gadman_client_domain_set(face->gmc, "module.pager", _pager_count++);
   e_gadman_client_zone_set(face->gmc, face->zone);
   e_gadman_client_policy_set(face->gmc,
			      E_GADMAN_POLICY_ANYWHERE |
			      E_GADMAN_POLICY_HMOVE |
			      E_GADMAN_POLICY_VMOVE |
			      E_GADMAN_POLICY_HSIZE |
			      E_GADMAN_POLICY_VSIZE |
			      E_GADMAN_POLICY_FIXED_ZONE);
   e_gadman_client_min_size_set(face->gmc, 8, 8);
   e_gadman_client_max_size_set(face->gmc, 600, 600);
   e_gadman_client_auto_size_set(face->gmc, 186, 40);
   e_gadman_client_align_set(face->gmc, 0.0, 1.0);
   e_gadman_client_resize(face->gmc, 186, 40);
   e_gadman_client_change_func_set(face->gmc, _pager_face_cb_gmc_change, face);
   e_gadman_client_load(face->gmc);

   return face;
}

void
_pager_face_free(Pager_Face *face)
{
   if (face->pager_object) evas_object_del(face->pager_object);
   if (face->table_object) evas_object_del(face->table_object);
   e_gadman_client_save(face->gmc);
   e_object_del(E_OBJECT(face->gmc));

   e_drop_handler_del(face->drop_handler);

   _pager_face_zone_unset(face);
   ecore_event_handler_del(face->ev_handler_border_resize);
   ecore_event_handler_del(face->ev_handler_border_move);
   ecore_event_handler_del(face->ev_handler_border_add);
   ecore_event_handler_del(face->ev_handler_border_remove);
   ecore_event_handler_del(face->ev_handler_border_iconify);
   ecore_event_handler_del(face->ev_handler_border_uniconify);
   ecore_event_handler_del(face->ev_handler_border_stick);
   ecore_event_handler_del(face->ev_handler_border_unstick);
   ecore_event_handler_del(face->ev_handler_border_desk_set);
   ecore_event_handler_del(face->ev_handler_border_raise);
   ecore_event_handler_del(face->ev_handler_border_lower);
   ecore_event_handler_del(face->ev_handler_border_icon_change);
   ecore_event_handler_del(face->ev_handler_zone_desk_count_set);
   ecore_event_handler_del(face->ev_handler_desk_show);
   ecore_event_handler_del(face->ev_handler_container_resize);

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

   /*
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Enabled");
   e_menu_item_check_set(mi, 1);
   if (face->conf->enabled) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _pager_face_cb_menu_enabled, face);
    */
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Edit Mode"));
   e_menu_item_callback_set(mi, _pager_face_cb_menu_edit, face);
}

static void
_pager_face_enable(Pager_Face *face)
{
   Evas_List  *l;

   face->conf->enabled = 1;
   evas_object_show(face->pager_object);
   for (l = face->desks; l; l = l->next)
     {
	Pager_Desk *pd;
	
	pd = l->data;
	evas_object_show(pd->event_object);
     }
   e_config_save_queue();
}

static void
_pager_face_disable(Pager_Face *face)
{
   Evas_List  *l;

   face->conf->enabled = 0;
   evas_object_hide(face->pager_object);
   for (l = face->desks; l; l = l->next)
     {
	Pager_Desk *pd;
	
	pd = l->data;
	evas_object_hide(pd->event_object);
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
     {
	for (y = 0; y < desks_y; y++)
	  {
	     Pager_Desk *pd;
	     E_Desk     *desk;
	     
	     desk = e_desk_at_xy_get(zone, x, y);
	     pd = _pager_desk_new(face, desk, x, y);
	     if (pd) face->desks = evas_list_append(face->desks, pd);
	  }
     }
//   e_gadman_client_aspect_set(face->gmc, 
//			      (double)(face->xnum * face->zone->w) / (double)(face->ynum * face->zone->h),
//			      (double)(face->xnum * face->zone->w) / (double)(face->ynum * face->zone->h));
}

static void
_pager_face_zone_unset(Pager_Face *face)
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
   Pager_Desk    *pd;
   Evas_Object   *o;
   E_Border_List *bl;
   E_Border      *bd;

   pd = E_NEW(Pager_Desk, 1);
   if (!pd) return NULL;

   pd->xpos = xpos;
   pd->ypos = ypos;

   pd->desk = desk;
   e_object_ref(E_OBJECT(desk));
   pd->face = face;

   o = edje_object_add(face->evas);
   pd->desk_object = o;
   e_theme_edje_object_set(o, "base/theme/modules/pager",
			   "modules/pager/desk");
   e_table_pack(face->table_object, o, xpos, ypos, 1, 1);
   e_table_pack_options_set(o, 1, 1, 1, 1, 0.5, 0.5, 0, 0, -1, -1);   
   evas_object_show(o);
   
   o = evas_object_rectangle_add(face->evas);
   pd->event_object = o;
   evas_object_layer_set(o, 2);
   evas_object_repeat_events_set(o, 1);
   evas_object_color_set(o, 0, 0, 0, 0);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_IN,  _pager_desk_cb_mouse_in,  pd);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_OUT, _pager_desk_cb_mouse_out, pd);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _pager_desk_cb_mouse_down, pd);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_UP, _pager_desk_cb_mouse_up, pd);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_MOVE, _pager_desk_cb_mouse_move, pd);
   evas_object_show(o);
   
   o = e_layout_add(face->evas);
   pd->layout_object = o;
   evas_object_intercept_move_callback_add(o, _pager_desk_cb_intercept_move, pd);
   evas_object_intercept_resize_callback_add(o, _pager_desk_cb_intercept_resize, pd);
   
   e_layout_virtual_size_set(o, desk->zone->w, desk->zone->h);
   edje_object_part_swallow(pd->desk_object, "items", pd->layout_object);
   evas_object_show(o);
   
   if (desk == e_desk_current_get(desk->zone)) _pager_face_desk_select(pd);

   bl = e_container_border_list_first(desk->zone->container);
   while ((bd = e_container_border_list_next(bl)))
     {
	Pager_Win   *pw;
	
	if ((bd->new_client) || (bd->desk != desk)) continue;
	pw = _pager_window_new(pd, bd);
	if (pw)
	  pd->wins = evas_list_append(pd->wins, pw);
     }
   e_container_border_list_free(bl);

   return pd;
}

static void
_pager_desk_free(Pager_Desk *pd)
{
   Evas_List *l;

   if (pd->desk_object) evas_object_del(pd->desk_object);
   if (pd->layout_object) evas_object_del(pd->layout_object);
   if (pd->event_object) evas_object_del(pd->event_object);
   e_object_unref(E_OBJECT(pd->desk));

   for (l = pd->wins; l; l = l->next)
     _pager_window_free(l->data);
   evas_list_free(pd->wins);
   free(pd);
}

static Pager_Win  *
_pager_window_new(Pager_Desk *pd, E_Border *border)
{
   Pager_Win   *pw;
   Evas_Object *o;
   E_App       *app;
   int          visible;

   if ((!border) || (border->client.netwm.state.skip_pager)) return NULL;
   pw = E_NEW(Pager_Win, 1);
   if (!pw) return NULL;

   pw->border = border;
   e_object_ref(E_OBJECT(border));
   visible = !border->iconic;
   pw->desk = pd;

   o = edje_object_add(pd->face->evas);
   pw->window_object = o;
   e_theme_edje_object_set(o, "base/theme/modules/pager",
			   "modules/pager/window");
   if (visible) evas_object_show(o);
   e_layout_pack(pd->layout_object, pw->window_object);
   e_layout_child_raise(pw->window_object);
   app = e_app_window_name_class_find(border->client.icccm.name,
				      border->client.icccm.class);
   if (app)
     {
	o = edje_object_add(pd->face->evas);
	pw->icon_object = o;
	edje_object_file_set(o, app->path, "icon");
	evas_object_show(o);
	edje_object_part_swallow(pw->window_object, "icon", o);
     }

   _pager_window_move(pd->face, pw);
   return pw;
}

static void
_pager_window_free(Pager_Win *pw)
{
   if (pw->window_object) evas_object_del(pw->window_object);
   if (pw->icon_object) evas_object_del(pw->icon_object);
   e_object_unref(E_OBJECT(pw->border));
   free(pw);
}

static void
_pager_window_move(Pager_Face *face, Pager_Win *pw)
{
   e_layout_child_move(pw->window_object,
		       pw->border->x - pw->desk->desk->zone->x,
		       pw->border->y - pw->desk->desk->zone->y);
   e_layout_child_resize(pw->window_object,
			 pw->border->w,
			 pw->border->h);
}

static Pager_Win *
_pager_face_border_find(Pager_Face *face, E_Border *border)
{
   Evas_List  *l;
   
   for (l = face->desks; l; l = l->next)
     {
	Pager_Desk *pd;
	Pager_Win *pw;
	
	pd = l->data;
	pw = _pager_desk_border_find(pd, border);
	if (pw) return pw;
     }
   return NULL;
}

static Pager_Win *
_pager_desk_border_find(Pager_Desk *pd, E_Border *border)
{
   Evas_List  *l;
   
   for (l = pd->wins; l; l = l->next)
     {
	Pager_Win  *pw;
	
	pw = l->data;
	if (pw->border == border) return pw;
     }
   return NULL;
}

static Pager_Desk *
_pager_face_desk_find(Pager_Face *face, E_Desk *desk)
{
   Evas_List *l;
   
   for (l = face->desks; l; l = l->next)
     {
	Pager_Desk *pd;
	
	pd = l->data;
	if (pd->desk == desk) return pd;
     }
   return NULL;
}

static void
_pager_face_desk_select(Pager_Desk *pd)
{
   Evas_List *l;
   
   if (pd->current) return;
   for (l = pd->face->desks; l; l = l->next)
     {
	Pager_Desk *pd2;
	
	pd2 = l->data;
	if (pd == pd2)
	  {
	     pd2->current = 1;
	     edje_object_signal_emit(pd2->desk_object, "active", "");
	  }
	else
	  {
	     if (pd2->current)
	       {
		  pd2->current = 0;
		  edje_object_signal_emit(pd2->desk_object, "passive", "");
	       }
	  }
     }
}

static void
_pager_face_cb_gmc_change(void *data, E_Gadman_Client *gmc, E_Gadman_Change change)
{
   Pager_Face *face;
   Evas_Coord  x, y, w, h;

   face = data;
   e_gadman_client_geometry_get(face->gmc, &x, &y, &w, &h);
   face->fx = x;
   face->fy = y;
   face->fw = w;
   face->fh = h;
   e_drop_handler_geometry_set(face->drop_handler,
			       face->fx + face->inset.l, face->fy + face->inset.t,
			       face->fw - (face->inset.l + face->inset.r),
			       face->fh - (face->inset.t + face->inset.b));
   switch (change)
     {
      case E_GADMAN_CHANGE_MOVE_RESIZE:
	evas_object_move(face->pager_object, face->fx, face->fy);
	evas_object_resize(face->pager_object, face->fw, face->fh);
	break;
      case E_GADMAN_CHANGE_RAISE:
	evas_object_raise(face->pager_object);
	break;
      default:
	break;
     }
}

static int
_pager_face_cb_event_border_resize(void *data, int type, void *event)
{
   E_Event_Border_Resize *ev;
   Pager_Face            *face;
   Evas_List             *l;

   face = data;
   ev = event;
   if (face->zone != ev->border->zone) return 1;
   for (l = face->desks; l; l = l->next)
     {
	Pager_Desk *pd;
	Pager_Win *pw;
	
	pd = l->data;
	pw = _pager_desk_border_find(pd, ev->border);
	if (pw)
	  _pager_window_move(face, pw);
     }
   return 1;
}

static int
_pager_face_cb_event_border_move(void *data, int type, void *event)
{
   E_Event_Border_Move   *ev;
   Pager_Face            *face;
   Evas_List             *l;

   face = data;
   ev = event;
   if (face->zone != ev->border->zone) return 1;
   for (l = face->desks; l; l = l->next)
     {
	Pager_Desk *pd;
	Pager_Win *pw;
	
	pd = l->data;
	pw = _pager_desk_border_find(pd, ev->border);
	if (pw)
	  _pager_window_move(face, pw);
     }
   return 1;
}

static int
_pager_face_cb_event_border_add(void *data, int type, void *event)
{
   Pager_Face         *face;
   Pager_Desk         *pd;
   E_Event_Border_Add *ev;

   face = data;
   ev = event;
   if (face->zone != ev->border->zone) return 1;
   if (_pager_face_border_find(face, ev->border))
     {
	printf("BUG: _pager_face_cb_event_border_add()\n");
	printf("     An already existing border shouldn't be added again\n");
	return 1;
     }

   pd = _pager_face_desk_find(face, ev->border->desk);
   if (pd)
     {
	Pager_Win          *pw;

	pw = _pager_window_new(pd, ev->border);
	if (pw)
	  pd->wins = evas_list_append(pd->wins, pw);
     }
   return 1;
}

static int
_pager_face_cb_event_border_remove(void *data, int type, void *event)
{
   E_Event_Border_Remove *ev;
   Pager_Face            *face;
   Evas_List             *l;

   face = data;
   ev = event;
   if (face->zone != ev->border->zone) return 1;
   for (l = face->desks; l; l = l->next)
     {
	Pager_Desk *pd;
	Pager_Win *pw;
	
	pd = l->data;
	pw = _pager_desk_border_find(pd, ev->border);
	if (pw)
	  {
	     pd->wins = evas_list_remove(pd->wins, pw);
	     _pager_window_free(pw);
	  }
     }
   return 1;
}

static int
_pager_face_cb_event_border_iconify(void *data, int type, void *event)
{
   E_Event_Border_Hide   *ev;
   Pager_Face            *face;
   Evas_List             *l;

   face = data;
   ev = event;
   if (face->zone != ev->border->zone) return 1;
   for (l = face->desks; l; l = l->next)
     {
	Pager_Desk *pd;
	Pager_Win *pw;
	
	pd = l->data;
	pw = _pager_desk_border_find(pd, ev->border);
	if (pw)
	  evas_object_hide(pw->window_object);
     }
   return 1;
}

static int
_pager_face_cb_event_border_uniconify(void *data, int type, void *event)
{
   E_Event_Border_Show   *ev;
   Pager_Face            *face;
   Evas_List             *l;

   face = data;
   ev = event;
   if (face->zone != ev->border->zone) return 1;
   for (l = face->desks; l; l = l->next)
     {
	Pager_Desk *pd;
	Pager_Win *pw;
	
	pd = l->data;
	pw = _pager_desk_border_find(pd, ev->border);
	if (pw)
	  evas_object_show(pw->window_object);
     }
   return 1;
}

static int
_pager_face_cb_event_border_stick(void *data, int type, void *event)
{
   E_Event_Border_Stick  *ev;
   Pager_Face            *face;
   Evas_List             *l;
   Pager_Win             *pw;

   face = data;
   ev = event;
   if (face->zone != ev->border->zone) return 1;
   pw = _pager_face_border_find(face, ev->border);
   if (!pw) return 1;
   for (l = face->desks; l; l = l->next)
     {
	Pager_Desk *pd;
	
	pd = l->data;
	if (ev->border->desk != pd->desk)
	  {
	     pw = _pager_window_new(pd, ev->border);
	     if (pw)
	       pd->wins = evas_list_append(pd->wins, pw);
	  }
     }
   return 1;
}

static int
_pager_face_cb_event_border_unstick(void *data, int type, void *event)
{
   E_Event_Border_Unstick *ev;
   Pager_Face             *face;
   Evas_List              *l;

   face = data;
   ev = event;
   if (face->zone != ev->border->zone) return 1;
   for (l = face->desks; l; l = l->next)
     {
	Pager_Desk *pd;
	
	pd = l->data;
        if (ev->border->desk != pd->desk)
	  {
	     Pager_Win *pw;
	     
	     pw = _pager_desk_border_find(pd, ev->border);
	     if (pw)
	       {
		  pd->wins = evas_list_remove(pd->wins, pw);
		  _pager_window_free(pw);
	       }
	  }
     }
   return 1;
}

static int
_pager_face_cb_event_border_desk_set(void *data, int type, void *event)
{
   E_Event_Border_Desk_Set *ev;
   Pager_Face              *face;
   Pager_Win               *pw;
   Pager_Desk              *pd;
   Evas_List               *l;
   
   face = data;
   ev = event;
   /* if this pager is not for the zone of the border */
   if (face->zone != ev->border->zone)
     {
	/* look at all desks in the pager */
	for (l = face->desks; l; l = l->next)
	  {
	     pd = l->data;
	     /* find this border in this desk */
	     pw = _pager_desk_border_find(pd, ev->border);
	     if (pw)
	       {
		  /* if it is found - remove it. it does not belong in this
		   * pager as it probably moves zones */
		  pd->wins = evas_list_remove(pd->wins, pw);
		  _pager_window_free(pw);
	       }
	  }
	return 1;
     }
   /* and this pager zone is for this border */
   /* see if the window is in this pager at all */
   pw = _pager_face_border_find(face, ev->border);
   if (pw)
     {
	/* is it sticky */
	if (ev->border->sticky)
	  {
	     /* if its sticky and in this pager - its already everywhere, so abort
	      * doing anything else */
	     return 1;
	  }
	/* move it to the right desk */
	/* find the pager desk of the target desk */
	pd = _pager_face_desk_find(face, ev->border->desk);
	if (pd)
	  {
	     /* remove it from whatever desk it was on */
	     pw->desk->wins = evas_list_remove(pw->desk->wins, pw);
	     e_layout_unpack(pw->window_object);

	     /* add it to the one its MEANT to be on */
	     pw->desk = pd;
	     pd->wins = evas_list_append(pd->wins, pw);
	     e_layout_pack(pd->layout_object, pw->window_object);
	     e_layout_child_raise(pw->window_object);
	     _pager_window_move(face, pw);
	  }
     }
   /* the border isnt in this pager at all - it must have moved zones */
   else
     {
	if (!ev->border->sticky)
	  {
	     /* find the pager desk it needs to go to */
	     pd = _pager_face_desk_find(face, ev->border->desk);
	     if (pd)
	       {
		  /* create it and add it */
		  pw = _pager_window_new(pd, ev->border);
		  if (pw)
		    pd->wins = evas_list_append(pd->wins, pw);
	       }
	  }
	else
	  {
	     /* go through all desks */
	     for (l = face->desks; l; l = l->next)
	       {
		  pd = l->data;
		  /* create it and add it */
		  pw = _pager_window_new(pd, ev->border);
		  if (pw)
		    pd->wins = evas_list_append(pd->wins, pw);
	       }
	  }
     }
   return 1;
}

static int
_pager_face_cb_event_border_raise(void *data, int type, void *event)
{
   E_Event_Border_Raise  *ev;
   Pager_Face            *face;
   Evas_List             *l;

   face = data;
   ev = event;
   if (face->zone != ev->border->zone) return 1;
   for (l = face->desks; l; l = l->next)
     {
	Pager_Desk *pd;
	Pager_Win *pw, *pw2 = NULL;
	
	pd = l->data;
	pw = _pager_desk_border_find(pd, ev->border);	
	if (pw)
	  {
	     if (ev->above)
	       pw2 = _pager_desk_border_find(pd, ev->above);
	     if (pw2)
	       e_layout_child_raise_above(pw->window_object, pw2->window_object);
	     else
	       e_layout_child_raise(pw->window_object);
	  }
     }
   return 1;
}

static int
_pager_face_cb_event_border_lower(void *data, int type, void *event)
{
   E_Event_Border_Lower  *ev;
   Pager_Face            *face;
   Evas_List             *l;

   face = data;
   ev = event;
   if (face->zone != ev->border->zone) return 1;
   for (l = face->desks; l; l = l->next)
     {
	Pager_Desk *pd;
	Pager_Win *pw, *pw2 = NULL;
	
	pd = l->data;
	pw = _pager_desk_border_find(pd, ev->border);
	if (pw)
	  {
	     if (ev->below)
	       pw2 = _pager_desk_border_find(pd, ev->below);
	     if (pw2)
	       e_layout_child_lower_below(pw->window_object, pw2->window_object);
	     else
	       e_layout_child_lower(pw->window_object);
	  }
     }
   return 1;
}

static int
_pager_face_cb_event_border_icon_change(void *data, int type, void *event)
{
   E_Event_Border_Icon_Change  *ev;
   Pager_Face                  *face;
   Evas_List                   *l;

   face = data;
   ev = event;
   if (face->zone != ev->border->zone) return 1;
   for (l = face->desks; l; l = l->next)
     {
	Pager_Desk *pd;
	Pager_Win *pw;
	
	pd = l->data;
	pw = _pager_desk_border_find(pd, ev->border);
	if (pw)
	  {
	     E_App *app;
	     
	     if (pw->icon_object)
	       {
		  evas_object_del(pw->icon_object);
		  pw->icon_object = NULL;
	       }
	     app = e_app_window_name_class_find(ev->border->client.icccm.name,
						ev->border->client.icccm.class);
	     if (app)
	       {
		  Evas_Object *o;
		  
		  o = edje_object_add(pd->face->evas);
		  pw->icon_object = o;
		  edje_object_file_set(o, app->path, "icon");
		  evas_object_show(o);
		  edje_object_part_swallow(pw->window_object, "icon", o);
	       }
	  }
     }
   return 1;
}

static int
_pager_face_cb_event_zone_desk_count_set(void *data, int type, void *event)
{
   E_Event_Zone_Desk_Count_Set *ev;
   Pager_Face                  *face;
   Pager_Desk                  *pd, *pd2;
   E_Desk                      *desk;
   Evas_List                   *l;
   int                          desks_x, desks_y;
   int                          max_x, max_y;
   int                          x, y;
   Evas_Coord                   lw, lh, dw, dh;

   face = data;
   ev = event;
   if (face->zone != ev->zone) return 1;
   e_zone_desk_count_get(ev->zone, &desks_x, &desks_y);
   if ((face->xnum == desks_x) && (face->ynum == desks_y)) return 1;
   
   evas_object_geometry_get(face->table_object, NULL, NULL, &lw, &lh);
   if (face->xnum > 0) dw = lw / face->xnum;
   else dw = 0;
   dw *= (desks_x - face->xnum);
   if (face->ynum > 0) dh = lh / face->ynum;
   else dh = 0;
   dh *= (desks_y - face->ynum);
   
   max_x = MAX(face->xnum, desks_x);
   max_y = MAX(face->ynum, desks_y);

   for (x = 0; x < max_x; x++)
     {
	for (y = 0; y < max_y; y++)
	  {
	     if ((x >= face->xnum) || (y >= face->ynum))
	       {
		  /* add desk */
		  desk = e_desk_at_xy_get(ev->zone, x, y);
		  pd = _pager_desk_new(face, desk, x, y);
		  if (pd)
		    face->desks = evas_list_append(face->desks, pd);
	       }
	     else if ((x >= desks_x) || (y >= desks_y))
	       {
		  /* del desk */
		  for (l = face->desks; l; l = l->next)
		    {
		       pd = l->data;
		       if ((pd->xpos == x) && (pd->ypos == y))
			 break;
		    }
		  if (pd->current)
		    {
		       desk = e_desk_current_get(ev->zone);
		       pd2 = _pager_face_desk_find(face, desk);
		       _pager_face_desk_select(pd2);
		    }
		  face->desks = evas_list_remove(face->desks, pd);
		  _pager_desk_free(pd);
	       }
	  }
     }
   
   face->xnum = desks_x;
   face->ynum = desks_y;
//   e_gadman_client_aspect_set(face->gmc, 
//			      (double)(face->xnum * face->zone->w) / (double)(face->ynum * face->zone->h),
//			      (double)(face->xnum * face->zone->w) / (double)(face->ynum * face->zone->h));
   e_gadman_client_resize(face->gmc, face->fw + dw, face->fh + dh);
   return 1;
}

static int
_pager_face_cb_event_desk_show(void *data, int type, void *event)
{
   Pager_Face        *face;
   Pager_Desk        *desk;
   E_Event_Desk_Show *ev;

   face = data;
   ev = event;
   if (face->zone != ev->desk->zone) return 1;
   desk = _pager_face_desk_find(face, ev->desk);
   if (desk) _pager_face_desk_select(desk);
   return 1;
}

static int
_pager_face_cb_event_container_resize(void *data, int type, void *event)
{
   Pager_Face               *face;
   E_Event_Container_Resize *ev;
   Evas_List                *l;
   Evas_Coord                w, h, lw, lh;
   
   face = data;
   ev = event;
   if (face->zone->container != ev->container) return 1;
   for (l = face->desks; l; l = l->next)
     {
	Pager_Desk *pd;
	
	pd = l->data;
	e_layout_virtual_size_set(pd->layout_object, 
				  face->zone->w,
				  face->zone->h);
     }
//   e_gadman_client_aspect_set(face->gmc, 
//			      (double)(face->xnum * face->zone->w) / (double)(face->ynum * face->zone->h),
//			      (double)(face->xnum * face->zone->w) / (double)(face->ynum * face->zone->h));
   w = face->fw;
   h = face->fh;
   evas_object_geometry_get(face->table_object, NULL, NULL, &lw, &lh);
   if ((face->xnum * face->zone->w) > (face->ynum * face->zone->h))
     w = face->xnum * ((face->zone->w * lh) / face->zone->h);
   else
     h = face->ynum * ((face->zone->h * lw) / face->zone->w);
   e_gadman_client_resize(face->gmc, w, h);
   return 1;
}

/*****/

static void
_pager_face_cb_menu_enabled(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Pager_Face *face;
   unsigned char enabled;

   face = data;
   enabled = e_menu_item_toggle_get(mi);
   if ((face->conf->enabled) && (!enabled))
     _pager_face_disable(face);
   else if ((!face->conf->enabled) && (enabled))
     _pager_face_enable(face);
}

static void
_pager_face_cb_menu_edit(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Pager_Face *face;

   face = data;
   e_gadman_mode_set(face->gmc->gadman, E_GADMAN_MODE_EDIT);
}

/*****/

static void
_pager_desk_cb_mouse_in(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_In *ev;
   Pager_Desk *desk;
   
   ev = event_info;
   desk = data;
}

static void
_pager_desk_cb_mouse_out(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Out *ev;
   Pager_Desk *desk;
   
   ev = event_info;
   desk = data;
}

static void
_pager_desk_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   Pager_Desk *pd;
   
   ev = event_info;
   pd = data;
   if (ev->button == 3)
     {
	e_menu_activate_mouse(pd->face->menu, pd->face->zone,
			      ev->output.x, ev->output.y, 1, 1,
			      E_MENU_POP_DIRECTION_DOWN);
	e_util_container_fake_mouse_up_all_later(pd->face->zone->container);
     }
   else if (ev->button == 1)
     {
	e_desk_show(pd->desk);
     }
}

static void
_pager_desk_cb_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Up *ev;
   Pager_Desk *desk;
   
   ev = event_info;
   desk = data;
}

static void
_pager_desk_cb_mouse_move(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Move *ev;
   Pager_Desk *desk;
   
   ev = event_info;
   desk = data;
}

/*****/

static void
_pager_desk_cb_intercept_move(void *data, Evas_Object *o, Evas_Coord x, Evas_Coord y)
{
   Pager_Desk *desk;
   
   desk = data;
   evas_object_move(o, x, y);
   evas_object_move(desk->event_object, x, y);
}

static void
_pager_desk_cb_intercept_resize(void *data, Evas_Object *o, Evas_Coord w, Evas_Coord h)
{
   Pager_Desk *desk;
   
   desk = data;
   evas_object_resize(o, w, h);
   evas_object_resize(desk->event_object, w, h);
}

static void
_pager_face_cb_enter(void *data, const char *type, void *event_info)
{
   E_Event_Dnd_Enter *ev;
   Pager_Face *face;
   int x, y;
   double w, h;

   ev = event_info;
   face = data;

   /* FIXME, check if the pager module has border */
   w = face->fw / (double) face->xnum;
   h = face->fh / (double) face->ynum;

   x = (ev->x - face->fx) / w;
   y = (ev->y - face->fy) / h;
}

static void
_pager_face_cb_move(void *data, const char *type, void *event_info)
{
   E_Event_Dnd_Move *ev;
   Pager_Face *face;
   Pager_Desk *pd;
   int x, y;
   double w, h;
   Evas_List *l;

   ev = event_info;
   face = data;

   w = (face->fw - (face->inset.l + face->inset.r)) / (double) face->xnum;
   h = (face->fh - (face->inset.t + face->inset.b)) / (double) face->ynum;

   x = (ev->x - (face->fx + face->inset.l)) / w;
   y = (ev->y - (face->fy + face->inset.t)) / h;

   for (l = face->desks; l; l = l->next)
     {
	pd = l->data;
	if ((pd->xpos == x) && (pd->ypos == y))
	  edje_object_signal_emit(pd->desk_object, "drag", "in");
	else
	  edje_object_signal_emit(pd->desk_object, "drag", "out");
     }
}

static void
_pager_face_cb_leave(void *data, const char *type, void *event_info)
{
   E_Event_Dnd_Leave *ev;
   Pager_Face *face;
   Evas_List *l;

   ev = event_info;
   face = data;

   for (l = face->desks; l; l = l->next)
     {
	Pager_Desk *pd;
	pd = l->data;
	edje_object_signal_emit(pd->desk_object, "drag", "out");
     }
}

static void
_pager_face_cb_drop(void *data, const char *type, void *event_info)
{
   E_Event_Dnd_Drop *ev;
   Pager_Face *face;
   E_Desk *desk;
   E_Border *bd;
   int x, y;
   double w, h;

   ev = event_info;
   face = data;

   w = (face->fw - (face->inset.l + face->inset.r)) / (double) face->xnum;
   h = (face->fh - (face->inset.t + face->inset.b)) / (double) face->ynum;

   x = (ev->x - (face->fx + face->inset.l)) / w;
   y = (ev->y - (face->fy + face->inset.t)) / h;

   desk = e_desk_at_xy_get(face->zone, x, y);
   bd = ev->data;

   if ((bd) && (desk) && (bd->desk != desk))
     {
	e_border_desk_set(bd, desk);
	e_border_hide(bd, 1);
     }
}
