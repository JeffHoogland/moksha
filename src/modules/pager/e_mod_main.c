/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"

/* TODO
 * which options should be in main menu, and which in face menu?
 * check if a new desk is in the current zone
 * check if padding changes on resize
 * include deskname in padding
 */

/* module private routines */
static Pager      *_pager_new(void);
static void        _pager_free(Pager *pager);
static void        _pager_config_menu_new(Pager *pager);

static Pager_Face *_pager_face_new(Pager *pager, E_Zone *zone, Evas *evas, int use_gmc);
static void        _pager_face_free(Pager_Face *face);
static void        _pager_face_menu_new(Pager_Face *face);
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
static Pager_Desk *_pager_face_desk_at_coord(Pager_Face *face, Evas_Coord x, Evas_Coord y);
static void        _pager_face_desk_select(Pager_Desk *pd);
static void        _pager_popup_free(Pager_Popup *pp);

static int         _pager_cb_event_border_resize(void *data, int type, void *event);
static int         _pager_cb_event_border_move(void *data, int type, void *event);
static int         _pager_cb_event_border_add(void *data, int type, void *event);
static int         _pager_cb_event_border_remove(void *data, int type, void *event);
static int         _pager_cb_event_border_iconify(void *data, int type, void *event);
static int         _pager_cb_event_border_uniconify(void *data, int type, void *event);
static int         _pager_cb_event_border_stick(void *data, int type, void *event);
static int         _pager_cb_event_border_unstick(void *data, int type, void *event);
static int         _pager_cb_event_border_desk_set(void *data, int type, void *event);
static int         _pager_cb_event_border_stack(void *data, int type, void *event);
static int         _pager_cb_event_border_icon_change(void *data, int type, void *event);
static int         _pager_cb_event_zone_desk_count_set(void *data, int type, void *event);
static int         _pager_cb_event_desk_show(void *data, int type, void *event);
static int         _pager_cb_event_desk_name_change(void *data, int type, void *event);

static void        _pager_face_cb_gmc_change(void *data, E_Gadman_Client *gmc, E_Gadman_Change change);
static void        _pager_face_cb_menu_edit(void *data, E_Menu *m, E_Menu_Item *mi);

static void        _pager_desk_cb_mouse_in(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void        _pager_desk_cb_mouse_out(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void        _pager_desk_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void        _pager_desk_cb_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void        _pager_desk_cb_mouse_move(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void        _pager_desk_cb_mouse_wheel(void *data, Evas *e, Evas_Object *obj, void *event_info);

static void        _pager_desk_cb_intercept_move(void *data, Evas_Object *o, Evas_Coord x, Evas_Coord y);
static void        _pager_desk_cb_intercept_resize(void *data, Evas_Object *o, Evas_Coord w, Evas_Coord h);

static void        _pager_window_cb_mouse_in(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void        _pager_window_cb_mouse_out(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void        _pager_window_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void        _pager_window_cb_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void        _pager_window_cb_mouse_move(void *data, Evas *e, Evas_Object *obj, void *event_info);

static void        _pager_window_cb_drag_finished(E_Drag *drag, int dropped);

static void        _pager_face_cb_enter(void *data, const char *type, void *drop);
static void        _pager_face_cb_move(void *data, const char *type, void *drop);
static void        _pager_face_cb_leave(void *data, const char *type, void *drop);
static void        _pager_face_cb_drop(void *data, const char *type, void *drop);

static void        _pager_face_deskname_position_change(Pager_Face *face);
static int         _pager_popup_cb_timeout(void *data);

static void        _pager_menu_cb_aspect_keep_height(void *data, E_Menu *m, E_Menu_Item *mi);
static void        _pager_menu_cb_aspect_keep_width(void *data, E_Menu *m, E_Menu_Item *mi);
static void        _pager_menu_cb_configure(void *data, E_Menu *m, E_Menu_Item *mi);

static int         _pager_count;

static E_Config_DD *_conf_edd;
static E_Config_DD *_conf_face_edd;

/* public module routines. all modules must have these */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
   "Pager"
};

EAPI void *
e_modapi_init(E_Module *module)
{
   Pager *pager = NULL;

   /* actually init pager */
   pager = _pager_new();
   module->config_menu = pager->config_menu;

   return pager;
}

EAPI int
e_modapi_shutdown(E_Module *module)
{
   Pager *pager;

   if (module->config_menu)
     module->config_menu = NULL;

   pager = module->data;
   if (pager) 
     {
	if (pager->config_dialog) 
	  {
	     e_object_del(E_OBJECT(pager->config_dialog));
	     pager->config_dialog = NULL;
	  }
	_pager_free(pager);	
     }
   return 1;
}

EAPI int
e_modapi_save(E_Module *module)
{
   Pager *pager;

   pager = module->data;
   e_config_domain_save("module.pager", _conf_edd, pager->conf);

   return 1;
}

EAPI int
e_modapi_info(E_Module *module)
{
   char buf[4096];

   snprintf(buf, sizeof(buf), "%s/module_icon.png", e_module_dir_get(module));
   module->icon_file = strdup(buf);
   return 1;
}

EAPI int
e_modapi_about(E_Module *module)
{
   e_module_dialog_show(_("Enlightenment Pager Module"),
			_("A pager module to navigate virtual desktops."));
   return 1;
}

EAPI int
e_modapi_config(E_Module *m)
{
   Pager *e;
   Evas_List *l;
   E_Zone *zone;
   E_Container *con;
		
   e = m->data;
   if (!e) return 0;
   if (!e->faces) return 0;
   con = e_container_current_get(e_manager_current_get());
   zone = e_zone_current_get(con);
   for (l = e->faces; l; l = l->next) 
     {
	Pager_Face *face;
	face = l->data;
	if (!face) return 0;
	if (face->zone == zone) 
	  {
	     _config_pager_module(con, e);
	     break;
	  }
     }
   return 1;
}

/* module private routines */
static Pager *
_pager_new(void)
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
   E_CONFIG_VAL(D, T, deskname_pos, UINT);
   E_CONFIG_VAL(D, T, popup_speed, DOUBLE);
   E_CONFIG_VAL(D, T, popup, UINT);
   E_CONFIG_VAL(D, T, drag_resist, UINT);

   pager->conf = e_config_domain_load("module.pager", _conf_edd);

   if (!pager->conf)
     {
	pager->conf = E_NEW(Config, 1);
	pager->conf->deskname_pos = PAGER_DESKNAME_NONE;
	pager->conf->popup_speed = 1.0;
	pager->conf->popup = 1;
	pager->conf->drag_resist = 3;
     }
   E_CONFIG_LIMIT(pager->conf->deskname_pos, PAGER_DESKNAME_NONE, PAGER_DESKNAME_RIGHT);
   E_CONFIG_LIMIT(pager->conf->popup_speed, 0.1, 10.0);
   E_CONFIG_LIMIT(pager->conf->popup, 0, 1);

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

		  face = _pager_face_new(pager, zone, zone->container->bg_evas,
					 1);
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

   /* set up event handles for when windows change and desktops */
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
   pager->ev_handler_border_iconify =
      ecore_event_handler_add(E_EVENT_BORDER_ICONIFY,
			      _pager_cb_event_border_iconify, pager);
   pager->ev_handler_border_uniconify =
      ecore_event_handler_add(E_EVENT_BORDER_UNICONIFY,
			      _pager_cb_event_border_uniconify, pager);
   pager->ev_handler_border_stick =
      ecore_event_handler_add(E_EVENT_BORDER_STICK,
			      _pager_cb_event_border_stick, pager);
   pager->ev_handler_border_unstick =
      ecore_event_handler_add(E_EVENT_BORDER_UNSTICK,
			      _pager_cb_event_border_unstick, pager);
   pager->ev_handler_border_desk_set =
      ecore_event_handler_add(E_EVENT_BORDER_DESK_SET,
			      _pager_cb_event_border_desk_set, pager);
   pager->ev_handler_border_stack =
      ecore_event_handler_add(E_EVENT_BORDER_STACK,
			      _pager_cb_event_border_stack, pager);
   pager->ev_handler_border_icon_change =
      ecore_event_handler_add(E_EVENT_BORDER_ICON_CHANGE,
			      _pager_cb_event_border_icon_change, pager);
   pager->ev_handler_zone_desk_count_set =
      ecore_event_handler_add(E_EVENT_ZONE_DESK_COUNT_SET,
			      _pager_cb_event_zone_desk_count_set, pager);
   pager->ev_handler_desk_show =
      ecore_event_handler_add(E_EVENT_DESK_SHOW,
			      _pager_cb_event_desk_show, pager);
   pager->ev_handler_desk_name_change =
      ecore_event_handler_add(E_EVENT_DESK_NAME_CHANGE,
			      _pager_cb_event_desk_name_change, pager);
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

   if (pager->ev_handler_border_resize)
     ecore_event_handler_del(pager->ev_handler_border_resize);
   if (pager->ev_handler_border_move)
     ecore_event_handler_del(pager->ev_handler_border_move);
   if (pager->ev_handler_border_add)
     ecore_event_handler_del(pager->ev_handler_border_add);
   if (pager->ev_handler_border_remove)
     ecore_event_handler_del(pager->ev_handler_border_remove);
   if (pager->ev_handler_border_iconify)
     ecore_event_handler_del(pager->ev_handler_border_iconify);
   if (pager->ev_handler_border_uniconify)
     ecore_event_handler_del(pager->ev_handler_border_uniconify);
   if (pager->ev_handler_border_stick)
     ecore_event_handler_del(pager->ev_handler_border_stick);
   if (pager->ev_handler_border_unstick)
     ecore_event_handler_del(pager->ev_handler_border_unstick);
   if (pager->ev_handler_border_desk_set)
     ecore_event_handler_del(pager->ev_handler_border_desk_set);
   if (pager->ev_handler_border_stack)
     ecore_event_handler_del(pager->ev_handler_border_stack);
   if (pager->ev_handler_border_icon_change)
     ecore_event_handler_del(pager->ev_handler_border_icon_change);
   if (pager->ev_handler_zone_desk_count_set)
     ecore_event_handler_del(pager->ev_handler_zone_desk_count_set);
   if (pager->ev_handler_desk_show)
     ecore_event_handler_del(pager->ev_handler_desk_show);
   if (pager->ev_handler_desk_name_change)
     ecore_event_handler_del(pager->ev_handler_desk_name_change);

   evas_list_free(pager->conf->faces);
   free(pager->conf);
   free(pager);
}

static void
_pager_config_menu_new(Pager *pager)
{
   E_Menu *mn;
   E_Menu_Item *mi;

   mn = e_menu_new();
   pager->config_menu = mn;

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Configuration"));
   e_util_menu_item_edje_icon_set(mi, "enlightenment/configuration");   
   e_menu_item_callback_set(mi, _pager_menu_cb_configure, pager);

   mi = e_menu_item_new(pager->config_menu);
   e_menu_item_label_set(mi, _("Fix Aspect (Keep Height)"));
   e_menu_item_callback_set(mi, _pager_menu_cb_aspect_keep_height, pager);

   mi = e_menu_item_new(pager->config_menu);
   e_menu_item_label_set(mi, _("Fix Aspect (Keep Width)"));
   e_menu_item_callback_set(mi, _pager_menu_cb_aspect_keep_width, pager);
}

static Pager_Face *
_pager_face_new(Pager *pager, E_Zone *zone, Evas *evas, int use_gmc)
{
   Pager_Face  *face;
   Evas_Object *o;
   Evas_Coord   x, y, w, h;
   double       aspect;
   const char  *drop[] = { "enlightenment/border", "enlightenment/pager_win" };
   E_Gadman_Policy  policy;

   face = E_NEW(Pager_Face, 1);
   if (!face) return NULL;

   face->pager = pager;

   /* store what evas we live in */
   face->evas = evas;

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
					   drop, 2,
					   face->fx, face->fy, face->fw, face->fh);

   _pager_face_zone_set(face, zone);
   _pager_face_deskname_position_change(face);

   /* popup does not want a gadman entry! */
   if (!use_gmc)
     return face;

   face->gmc = e_gadman_client_new(zone->container->gadman);

   e_gadman_client_domain_set(face->gmc, "module.pager", _pager_count++);
   e_gadman_client_zone_set(face->gmc, face->zone);

   policy = E_GADMAN_POLICY_ANYWHERE |
           E_GADMAN_POLICY_HMOVE |
           E_GADMAN_POLICY_VMOVE |
           E_GADMAN_POLICY_HSIZE |
  //                         E_GADMAN_POLICY_FIXED_ZONE |
           E_GADMAN_POLICY_VSIZE;

   e_gadman_client_policy_set(face->gmc, policy);

   e_gadman_client_min_size_set(face->gmc, 8, 8);
   e_gadman_client_max_size_set(face->gmc, 600, 600);
   e_gadman_client_auto_size_set(face->gmc,
				 (face->xnum * 40) + (face->inset.l + face->inset.r),
				 (face->ynum * 30) + (face->inset.t + face->inset.b));
   e_gadman_client_align_set(face->gmc, 0.0, 1.0);
   aspect = (double)(face->xnum * face->zone->w) / (double)(face->ynum * face->zone->h);
   e_gadman_client_aspect_set(face->gmc, aspect, aspect);
   e_gadman_client_padding_set(face->gmc,
			       face->inset.l, face->inset.r,
			       face->inset.t, face->inset.b);
   e_gadman_client_resize(face->gmc,
			  (face->xnum * 40) + (face->inset.l + face->inset.r),
			  (face->ynum * 30) + (face->inset.t + face->inset.b));
   e_gadman_client_change_func_set(face->gmc, _pager_face_cb_gmc_change, face);
   e_gadman_client_load(face->gmc);

   Evas_Coord g, z;
   e_gadman_client_geometry_get(face->gmc, NULL, NULL, &g, &z);

   return face;
}

void
_pager_face_free(Pager_Face *face)
{
   if (face->pager_object) evas_object_del(face->pager_object);
   if (face->table_object) evas_object_del(face->table_object);
   if (face->gmc)
     {
	e_gadman_client_save(face->gmc);
	e_object_del(E_OBJECT(face->gmc));
     }

   e_drop_handler_del(face->drop_handler);

   _pager_face_zone_unset(face);

   if (face->current_popup)
     _pager_popup_free(face->current_popup);

   if (face->menu)
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

   /* Config */
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Configuration"));
   e_util_menu_item_edje_icon_set(mi, "enlightenment/configuration");   
   e_menu_item_callback_set(mi, _pager_menu_cb_configure, face->pager);
    
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Edit Mode"));
   e_util_menu_item_edje_icon_set(mi, "enlightenment/gadgets");   
   e_menu_item_callback_set(mi, _pager_face_cb_menu_edit, face);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Fix Aspect (Keep Height)"));
   e_menu_item_callback_set(mi, _pager_menu_cb_aspect_keep_height, face->pager);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Fix Aspect (Keep Width)"));
   e_menu_item_callback_set(mi, _pager_menu_cb_aspect_keep_width, face->pager);
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

   face->zone = zone;
   e_object_ref(E_OBJECT(zone));
   e_zone_desk_count_get(zone, &desks_x, &desks_y);
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
	     if (pd)
	       {
		  face->desks = evas_list_append(face->desks, pd);
		  if (desk == e_desk_current_get(desk->zone))
		    _pager_face_desk_select(pd);
	       }
	  }
     }
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

   o = e_layout_add(face->evas);
   pd->layout_object = o;
   evas_object_intercept_move_callback_add(o, _pager_desk_cb_intercept_move, pd);
   evas_object_intercept_resize_callback_add(o, _pager_desk_cb_intercept_resize, pd);

   e_layout_virtual_size_set(o, desk->zone->w, desk->zone->h);
   edje_object_part_swallow(pd->desk_object, "items", pd->layout_object);
   evas_object_show(o);

   o = evas_object_rectangle_add(face->evas);
   pd->event_object = o;
   evas_object_layer_set(o, 1);
   evas_object_repeat_events_set(o, 1);
   evas_object_color_set(o, 0, 0, 0, 0);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_IN,  _pager_desk_cb_mouse_in,  pd);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_OUT, _pager_desk_cb_mouse_out, pd);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _pager_desk_cb_mouse_down, pd);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_UP, _pager_desk_cb_mouse_up, pd);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_MOVE, _pager_desk_cb_mouse_move, pd);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_WHEEL, _pager_desk_cb_mouse_wheel, pd);
   evas_object_show(o);

   bl = e_container_border_list_first(desk->zone->container);
   while ((bd = e_container_border_list_next(bl)))
     {
	Pager_Win   *pw;

	if ((bd->new_client) || ((bd->desk != desk) && (!bd->sticky))) continue;
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

   for (l = pd->wins; l; l = l->next)
     _pager_window_free(l->data);
   pd->wins = evas_list_free(pd->wins);
   e_object_unref(E_OBJECT(pd->desk));
   free(pd);
}

static Pager_Win  *
_pager_window_new(Pager_Desk *pd, E_Border *border)
{
   Pager_Win   *pw;
   Evas_Object *o;
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
   o = e_border_icon_add(border, pd->face->evas);
   if (o)
     {
	pw->icon_object = o;
	evas_object_show(o);
	edje_object_part_swallow(pw->window_object, "icon", o);
     }

   /* add an event object */

   o = evas_object_rectangle_add(pd->face->evas);
   pw->event_object = o;

   evas_object_repeat_events_set(o, 1);
   evas_object_color_set(o, 0, 0, 0, 0);
//   evas_object_color_set(o, rand()%255, rand()%255, rand()%255, 255);


   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_IN,  _pager_window_cb_mouse_in,  pw);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_OUT, _pager_window_cb_mouse_out, pw);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _pager_window_cb_mouse_down, pw);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_UP, _pager_window_cb_mouse_up, pw);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_MOVE, _pager_window_cb_mouse_move, pw);

   evas_object_show(o);
   e_layout_pack(pd->layout_object, pw->event_object);
   e_layout_child_raise(pw->event_object);


   _pager_window_move(pd->face, pw);
   return pw;
}

static void
_pager_window_free(Pager_Win *pw)
{
   if (pw->window_object) evas_object_del(pw->window_object);
   if (pw->icon_object) evas_object_del(pw->icon_object);
   if (pw->event_object) evas_object_del(pw->event_object);
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
   e_layout_child_move(pw->event_object,
		       pw->border->x - pw->desk->desk->zone->x,
		       pw->border->y - pw->desk->desk->zone->y);
   e_layout_child_resize(pw->event_object,
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

/**
 * Return Pager_Desk at canvas coord x, y
 */
static Pager_Desk *
_pager_face_desk_at_coord(Pager_Face *face, Evas_Coord x, Evas_Coord y)
{
   Evas_List *l;

   for (l = face->desks; l; l = l->next)
     {
	Pager_Desk *pd;
	Evas_Coord dx, dy, dw, dh;

	pd = l->data;
	evas_object_geometry_get(pd->desk_object, &dx, &dy, &dw, &dh);
	if (E_INSIDE(x, y, dx, dy, dw, dh))
	  {
	     return pd;
	  }
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
   edje_object_part_text_set(pd->face->pager_object, "desktop_name", pd->desk->name);
}

static void
_pager_popup_free(Pager_Popup *pp)
{
   pp->src_face->current_popup = NULL;
   if (pp->timer) ecore_timer_del(pp->timer);
   pp->face->pager->faces = evas_list_remove(pp->face->pager->faces, pp->face);
   evas_object_del(pp->bg_object);
   _pager_face_free(pp->face);
   e_bindings_mouse_ungrab(E_BINDING_CONTEXT_POPUP, pp->popup->evas_win);
   e_bindings_wheel_ungrab(E_BINDING_CONTEXT_POPUP, pp->popup->evas_win);
   e_object_del(E_OBJECT(pp->popup));
   free(pp);
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
_pager_cb_event_border_resize(void *data, int type, void *event)
{
   E_Event_Border_Resize *ev;
   Pager                 *pager;
   Evas_List             *l, *l2;

   pager = data;
   ev = event;
   for (l = pager->faces; l; l = l->next)
     {
	Pager_Face *face;

	face = l->data;
	if (face->zone != ev->border->zone) continue;
	for (l2 = face->desks; l2; l2 = l2->next)
	  {
	     Pager_Desk *pd;
	     Pager_Win *pw;

	     pd = l2->data;
	     pw = _pager_desk_border_find(pd, ev->border);
	     if (pw)
	       _pager_window_move(face, pw);
	  }
     }
   return 1;
}

static int
_pager_cb_event_border_move(void *data, int type, void *event)
{
   E_Event_Border_Move   *ev;
   Pager                 *pager;
   Evas_List             *l, *l2;

   pager = data;
   ev = event;
   for (l = pager->faces; l; l = l->next)
     {
	Pager_Face *face;

	face = l->data;
	if (face->zone != ev->border->zone) continue;
	for (l2 = face->desks; l2; l2 = l2->next)
	  {
	     Pager_Desk *pd;
	     Pager_Win *pw;

	     pd = l2->data;
	     pw = _pager_desk_border_find(pd, ev->border);
	     if (pw)
	       _pager_window_move(face, pw);
	  }
     }
   return 1;
}

static int
_pager_cb_event_border_add(void *data, int type, void *event)
{
   E_Event_Border_Add *ev;
   Pager              *pager;
   Evas_List          *l;

   pager = data;
   ev = event;
   for (l = pager->faces; l; l = l->next)
     {
	Pager_Face *face;
	Pager_Desk *pd;

	face = l->data;
	if ((face->zone != ev->border->zone) ||
	    (_pager_face_border_find(face, ev->border)))
	  continue;

	pd = _pager_face_desk_find(face, ev->border->desk);
	if (pd)
	  {
	     Pager_Win *pw;

	     pw = _pager_window_new(pd, ev->border);
	     if (pw)
	       pd->wins = evas_list_append(pd->wins, pw);
	  }
     }
   return 1;
}

static int
_pager_cb_event_border_remove(void *data, int type, void *event)
{
   E_Event_Border_Remove *ev;
   Pager                 *pager;
   Evas_List             *l, *l2;

   pager = data;
   ev = event;
   for (l = pager->faces; l; l = l->next)
     {
	Pager_Face *face;

	face = l->data;
	if (face->zone != ev->border->zone) continue;
	for (l2 = face->desks; l2; l2 = l2->next)
	  {
	     Pager_Desk *pd;
	     Pager_Win *pw;

	     pd = l2->data;
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
_pager_cb_event_border_iconify(void *data, int type, void *event)
{
   E_Event_Border_Hide   *ev;
   Pager                 *pager;
   Evas_List             *l, *l2;

   pager = data;
   ev = event;
   for (l = pager->faces; l; l = l->next)
     {
	Pager_Face *face;

	face = l->data;
	if (face->zone != ev->border->zone) continue;
	for (l2 = face->desks; l2; l2 = l2->next)
	  {
	     Pager_Desk *pd;
	     Pager_Win *pw;

	     pd = l2->data;
	     pw = _pager_desk_border_find(pd, ev->border);
	     if (pw)
	       {
		  evas_object_hide(pw->window_object);
		  evas_object_hide(pw->event_object);
	       }
	  }
     }
   return 1;
}

static int
_pager_cb_event_border_uniconify(void *data, int type, void *event)
{
   E_Event_Border_Show   *ev;
   Pager                 *pager;
   Evas_List             *l, *l2;

   pager = data;
   ev = event;
   for (l = pager->faces; l; l = l->next)
     {
	Pager_Face *face;

	face = l->data;
	if (face->zone != ev->border->zone) continue;
	for (l2 = face->desks; l2; l2 = l2->next)
	  {
	     Pager_Desk *pd;
	     Pager_Win *pw;

	     pd = l2->data;
	     pw = _pager_desk_border_find(pd, ev->border);
	     if (pw)
	       {
		  evas_object_show(pw->window_object);
		  evas_object_show(pw->event_object);
	       }
	  }
     }
   return 1;
}

static int
_pager_cb_event_border_stick(void *data, int type, void *event)
{
   E_Event_Border_Stick  *ev;
   Pager                 *pager;
   Evas_List             *l, *l2;

   pager = data;
   ev = event;
   for (l = pager->faces; l; l = l->next)
     {
	Pager_Face *face;
	Pager_Win  *pw;

	face = l->data;
	if (face->zone != ev->border->zone) continue;
	pw = _pager_face_border_find(face, ev->border);
	if (!pw) continue;
	for (l2 = face->desks; l2; l2 = l2->next)
	  {
	     Pager_Desk *pd;

	     pd = l2->data;
	     if (ev->border->desk != pd->desk)
	       {
		  pw = _pager_window_new(pd, ev->border);
		  if (pw)
		    pd->wins = evas_list_append(pd->wins, pw);
	       }
	  }
     }
   return 1;
}

static int
_pager_cb_event_border_unstick(void *data, int type, void *event)
{
   E_Event_Border_Unstick *ev;
   Pager                   *pager;
   Evas_List               *l, *l2;

   pager = data;
   ev = event;
   for (l = pager->faces; l; l = l->next)
     {
	Pager_Face *face;

	face = l->data;
	if (face->zone != ev->border->zone) continue;
	for (l2 = face->desks; l2; l2 = l2->next)
	  {
	     Pager_Desk *pd;

	     pd = l2->data;
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
     }
   return 1;
}

static int
_pager_cb_event_border_desk_set(void *data, int type, void *event)
{
   E_Event_Border_Desk_Set *ev;
   Pager                   *pager;
   Evas_List               *l, *l2;

   pager = data;
   ev = event;
   for (l = pager->faces; l; l = l->next)
     {
	Pager_Face              *face;
	Pager_Win               *pw;
	Pager_Desk              *pd;

	face = l->data;

	/* if this pager is not for the zone of the border */
	if (face->zone != ev->border->zone)
	  {
	     /* look at all desks in the pager */
	     for (l2 = face->desks; l2; l2 = l2->next)
	       {
		  pd = l2->data;
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
	     continue;
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
		  continue;
	       }
	     /* move it to the right desk */
	     /* find the pager desk of the target desk */
	     pd = _pager_face_desk_find(face, ev->border->desk);
	     if (pd)
	       {
		  Pager_Win *pw2 = NULL;
		  E_Border *bd;

		  /* remove it from whatever desk it was on */
		  pw->desk->wins = evas_list_remove(pw->desk->wins, pw);
		  e_layout_unpack(pw->window_object);
		  e_layout_unpack(pw->event_object);

		  /* add it to the one its MEANT to be on */
		  pw->desk = pd;
		  pd->wins = evas_list_append(pd->wins, pw);
		  e_layout_pack(pd->layout_object, pw->window_object);
		  e_layout_pack(pd->layout_object, pw->event_object);

		  bd = e_util_desk_border_above(pw->border);
		  if (bd)
		    pw2 = _pager_desk_border_find(pd, bd);
		  if (pw2)
		    {
		       e_layout_child_lower_below(pw->window_object, pw2->window_object);
		       e_layout_child_raise_above(pw->event_object, pw->window_object);
		    }
		  else
		    {
		       e_layout_child_raise(pw->window_object);
		       e_layout_child_raise_above(pw->event_object, pw->window_object);
		    }

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
			 {
			    Pager_Win *pw2 = NULL;
			    E_Border *bd;

			    pd->wins = evas_list_append(pd->wins, pw);
			    bd = e_util_desk_border_above(pw->border);
			    if (bd)
			      pw2 = _pager_desk_border_find(pd, bd);
			    if (pw2)
			      {
				 e_layout_child_lower_below(pw->window_object, pw2->window_object);
				 e_layout_child_raise_above(pw->event_object, pw->window_object);
			      }
			    else
			      {
				 e_layout_child_raise(pw->window_object);
				 e_layout_child_raise_above(pw->event_object, pw->window_object);
			      }

			    _pager_window_move(face, pw);
			 }
		    }
	       }
	     else
	       {
		  /* go through all desks */
		  for (l2 = face->desks; l2; l2 = l2->next)
		    {
		       pd = l2->data;
		       /* create it and add it */
		       pw = _pager_window_new(pd, ev->border);
		       if (pw)
			 {
			    Pager_Win *pw2 = NULL;
			    E_Border *bd;

			    pd->wins = evas_list_append(pd->wins, pw);
			    bd = e_util_desk_border_above(pw->border);
			    if (bd)
			      pw2 = _pager_desk_border_find(pd, bd);
			    if (pw2)
			      {
				 e_layout_child_lower_below(pw->window_object, pw2->window_object);
				 e_layout_child_raise_above(pw->event_object, pw->window_object);
			      }
			    else
			      {
				 e_layout_child_raise(pw->window_object);
				 e_layout_child_raise_above(pw->event_object, pw->window_object);
			      }

			    _pager_window_move(face, pw);
			 }
		    }
	       }
	  }
     }
   return 1;
}

static int
_pager_cb_event_border_stack(void *data, int type, void *event)
{
   E_Event_Border_Stack  *ev;
   Pager                 *pager;
   Evas_List             *l, *l2;

   pager = data;
   ev = event;
   for (l = pager->faces; l; l = l->next)
     {
	Pager_Face *face;

	face = l->data;
	if (face->zone != ev->border->zone) continue;
	for (l2 = face->desks; l2; l2 = l2->next)
	  {
	     Pager_Desk *pd;
	     Pager_Win *pw, *pw2 = NULL;

	     pd = l2->data;
	     pw = _pager_desk_border_find(pd, ev->border);
	     if (pw)
	       {
		  if (ev->stack)
		    {
		       pw2 = _pager_desk_border_find(pd, ev->stack);
		       if (!pw2)
			 {
			    /* This border is on another desk... */
			    E_Border *bd = NULL;

			    if (ev->type == E_STACKING_ABOVE)
			      bd = e_util_desk_border_below(ev->border);
			    else if (ev->type == E_STACKING_BELOW)
			      bd = e_util_desk_border_above(ev->border);

			    if (bd)
			      pw2 = _pager_desk_border_find(pd, bd);
			 }
		    }
		  if (ev->type == E_STACKING_ABOVE)
		    {
		       if (pw2)
			 {
			    e_layout_child_raise_above(pw->window_object, pw2->window_object);
			    e_layout_child_raise_above(pw->event_object, pw->window_object);
			    e_layout_child_raise_above(pw2->event_object, pw2->window_object);
			 }
		       else
			 {
			    /* If we aren't above any window, we are at the bottom */
			    e_layout_child_lower(pw->window_object);
			    e_layout_child_raise_above(pw->event_object, pw->window_object);
			 }
		    }
		  else if (ev->type == E_STACKING_BELOW)
		    {
		       if (pw2)
			 {
			    e_layout_child_lower_below(pw->window_object, pw2->window_object);
			    e_layout_child_raise_above(pw->event_object, pw->window_object);
			 }
		       else
			 {
			    /* If we aren't below any window, we are at the top */
			    e_layout_child_raise(pw->window_object);
			    e_layout_child_raise_above(pw->event_object, pw->window_object);
			 }
		    }
	       }
	  }
     }
   return 1;
}

static int
_pager_cb_event_border_icon_change(void *data, int type, void *event)
{
   E_Event_Border_Icon_Change  *ev;
   Pager                       *pager;
   Evas_List                   *l, *l2;

   pager = data;
   ev = event;
   for (l = pager->faces; l; l = l->next)
     {
	Pager_Face *face;

	face = l->data;
	if (face->zone != ev->border->zone) continue;
	for (l2 = face->desks; l2; l2 = l2->next)
	  {
	     Pager_Desk *pd;
	     Pager_Win *pw;

	     pd = l2->data;
	     pw = _pager_desk_border_find(pd, ev->border);
	     if (pw)
	       {
		  Evas_Object *o;

		  if (pw->icon_object)
		    {
		       evas_object_del(pw->icon_object);
		       pw->icon_object = NULL;
		    }
		  o = e_border_icon_add(ev->border, pd->face->evas);
		  if (o)
		    {
		       pw->icon_object = o;
		       evas_object_show(o);
		       edje_object_part_swallow(pw->window_object, "icon", o);
		    }
	       }
	  }
     }
   return 1;
}

static int
_pager_cb_event_zone_desk_count_set(void *data, int type, void *event)
{
   E_Event_Zone_Desk_Count_Set *ev;
   Pager                       *pager;
   Evas_List                   *l, *l2;

   pager = data;
   ev = event;

   for (l = pager->faces; l; l = l->next)
     {
	Pager_Face                  *face;
	Pager_Desk                  *pd, *pd2;
	E_Desk                      *desk;
	int                          desks_x, desks_y;
	int                          x, y;
	Evas_Coord                   lw, lh, dw, dh;
	double                       aspect;

	face = l->data;

	if (face->zone != ev->zone) continue;
	e_zone_desk_count_get(ev->zone, &desks_x, &desks_y);
	if ((face->xnum == desks_x) && (face->ynum == desks_y)) continue;

	evas_object_geometry_get(face->table_object, NULL, NULL, &lw, &lh);
	if (face->xnum > 0) dw = lw / face->xnum;
	else dw = 0;
	dw *= (desks_x - face->xnum);
	if (face->ynum > 0) dh = lh / face->ynum;
	else dh = 0;
	dh *= (desks_y - face->ynum);

	/* Loop to add new desks */
	for (x = 0; x < desks_x; x++)
	  {
	     for (y = 0; y < desks_y; y++)
	       {
		  if ((x >= face->xnum) || (y >= face->ynum))
		    {
		       /* add desk */
		       desk = e_desk_at_xy_get(ev->zone, x, y);
		       pd = _pager_desk_new(face, desk, x, y);
		       if (pd)
			 face->desks = evas_list_append(face->desks, pd);
		    }
	       }
	  }
	/* Loop to remove extra desks */
	for (l2 = face->desks; l2;)
	  {
	     pd = l2->data;
	     l2 = l2->next;
	     if ((pd->xpos >= desks_x) || (pd->ypos >= desks_y))
	       {
		  /* remove desk */
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

	face->xnum = desks_x;
	face->ynum = desks_y;
	aspect = (double)(face->xnum * face->zone->w) / (double)(face->ynum * face->zone->h);
	e_gadman_client_aspect_set(face->gmc, aspect, aspect);
	e_gadman_client_resize(face->gmc, face->fw + dw, face->fh + dh);
     }
   return 1;
}

static int
_pager_cb_event_desk_show(void *data, int type, void *event)
{
   E_Event_Desk_Show *ev;
   Pager             *pager;
   Pager_Popup       *pp = NULL;
   Evas_List         *l;

   pager = data;
   ev = event;
   for (l = pager->faces; l; l = l->next)
     {
	Pager_Face *face;
	Pager_Desk *pd;

	face = l->data;
	if (face->zone != ev->desk->zone) continue;

	pd = _pager_face_desk_find(face, ev->desk);
	if (pd)
	  {
	     Evas_Coord   w, h;

	     _pager_face_desk_select(pd);

	     /* If the popup is defined, we don't want another */
	     if ((!face->pager->conf->popup) || (pp)) continue;

	     pp = face->current_popup;
	     face->current_popup = NULL;
	     if (pp) _pager_popup_free(pp);

	     pp = E_NEW(Pager_Popup, 1);
	     if (!pp) continue;

	     /* Show popup */
	     pp->popup = e_popup_new(face->zone, 0, 0, 1, 1);
	     if (!pp->popup)
	       {
		  free(pp);
		  continue;
	       }
	     e_popup_layer_set(pp->popup, 999);
	     pp->src_face = face;

	     face->current_popup = pp;

	     evas_object_geometry_get(face->pager_object, NULL, NULL, &w, &h);
	     
	     pp->face = _pager_face_new(face->pager, face->zone,
					pp->popup->evas, 0);
	     evas_object_move(pp->face->pager_object, 0, 0);
	     evas_object_resize(pp->face->pager_object, w, h);

	     pp->bg_object = edje_object_add(pp->face->evas);
	     e_theme_edje_object_set(pp->bg_object, "base/theme/modules/pager",
				     "widgets/pager/popup");
	     edje_object_part_text_set(pp->bg_object, "text", pd->desk->name);
	     evas_object_show(pp->bg_object);
	     edje_extern_object_min_size_set(pp->face->pager_object, w, h);
	     edje_object_part_swallow(pp->bg_object, "pager", pp->face->pager_object);
	     edje_object_calc_force(pp->face->pager_object);
	     edje_object_size_min_calc(pp->bg_object, &w, &h);

	     evas_object_move(pp->bg_object, 0, 0);
	     evas_object_resize(pp->bg_object, w, h);
	     e_popup_edje_bg_object_set(pp->popup, pp->bg_object);
	     e_popup_ignore_events_set(pp->popup, 1);
	     e_popup_move_resize(pp->popup,
				 ((pp->popup->zone->w - w) / 2),
				 ((pp->popup->zone->h - h) / 2),
				 w, h);
	     e_bindings_mouse_grab(E_BINDING_CONTEXT_POPUP, pp->popup->evas_win);
	     e_bindings_wheel_grab(E_BINDING_CONTEXT_POPUP, pp->popup->evas_win);
	     e_popup_show(pp->popup);

	     pp->timer = ecore_timer_add(face->pager->conf->popup_speed,
					 _pager_popup_cb_timeout, pp);
	  }
     }

   return 1;
}

static int
_pager_popup_cb_timeout(void *data)
{
   Pager_Popup *pp;

   pp = data;
   _pager_popup_free(pp);
   return 0;
}

static int
_pager_cb_event_desk_name_change(void *data, int type, void *event)
{
   E_Event_Desk_Show *ev;
   Pager             *pager;
   Evas_List         *l, *l2;

   pager = data;
   ev = event;
   for (l = pager->faces; l; l = l->next)
     {
	Pager_Face *face;

	face = l->data;
	if (face->zone != ev->desk->zone) continue;
	for (l2 = face->desks; l2; l2 = l2->next)
	  {
	     Pager_Desk *pd;
	     pd = l2->data;

	     if ((pd->desk == ev->desk) && (pd->current))
	       {
		  edje_object_part_text_set(pd->face->pager_object, "desktop_name", ev->desk->name);
		  break;
	       }

	  }
     }
   return 1;
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

   edje_object_part_text_set(desk->face->pager_object, "desktop_name", desk->desk->name);
}

static void
_pager_desk_cb_mouse_out(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Out *ev;
   Pager_Desk *desk;
   Evas_List *l;

   ev = event_info;
   desk = data;

   for (l = desk->face->desks; l; l = l->next)
     {
	Pager_Desk *pd;
	pd = l->data;
	if (pd->current)
	  {
	     edje_object_part_text_set(pd->face->pager_object, "desktop_name", pd->desk->name);
	     break;
	  }
     }
}

static void
_pager_desk_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   Pager_Desk *pd;

   ev = event_info;
   pd = data;
   if ((ev->button == 3) && (pd->face->menu))
     {
	e_menu_activate_mouse(pd->face->menu, pd->face->zone,
			      ev->output.x, ev->output.y, 1, 1,
			      E_MENU_POP_DIRECTION_DOWN, ev->timestamp);
	e_util_container_fake_mouse_up_later(pd->face->zone->container, 3);
     }
}

static void
_pager_desk_cb_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Up *ev;
   Pager_Desk *desk;

   ev = event_info;
   desk = data;

   if ((ev->button == 1) && (!desk->face->dragging))
     {
	e_desk_show(desk->desk);
     }
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


/*****/

static void
_pager_window_cb_mouse_in(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_In *ev;
   Pager_Win *pw;

   ev = event_info;
   pw = data;

}

static void
_pager_window_cb_mouse_out(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Out *ev;
   Pager_Win *pw;

   ev = event_info;
   pw = data;

}

static void
_pager_window_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   Pager_Win *pw;

   ev = event_info;
   pw = data;
   if (!pw) return;

   /* make this configurable */
   if (ev->button == 1)
     {
	Evas_Coord ox, oy;

	evas_object_geometry_get(pw->window_object, &ox, &oy, NULL, NULL);
	pw->drag.in_pager = 1;

	pw->drag.x = ev->canvas.x;
	pw->drag.y = ev->canvas.y;
	pw->drag.dx = ox - ev->canvas.x;
	pw->drag.dy = oy - ev->canvas.y;
	pw->drag.start = 1;
     }
}

static void
_pager_window_cb_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Up *ev;
   Pager_Win *pw;

   ev = event_info;
   pw = data;
   if (!pw) return;

   pw->drag.in_pager = 0;
   pw->drag.start = 0;
   pw->desk->face->dragging = 0;
}

static void
_pager_window_cb_mouse_move(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Move *ev;
   Pager_Win *pw;

   ev = event_info;
   pw = data;

   if (!pw) return;

   /* prevent drag for a few pixels */
   if (pw->drag.start)
     {
	Evas_Coord dx, dy;
	unsigned int resist = 0;

	dx = pw->drag.x - ev->cur.output.x;
	dy = pw->drag.y - ev->cur.output.y;
	if (pw->desk && pw->desk->face && pw->desk->face->pager) 
	  resist = pw->desk->face->pager->conf->drag_resist;
	
	if (((dx * dx) + (dy * dy)) <= (resist * resist)) return;

	pw->desk->face->dragging = 1;
	pw->drag.start = 0;
     }

   /* dragging this win around inside the pager */
   if (pw->drag.in_pager)
     {
	Evas_Coord mx, my, vx, vy;
	Pager_Desk *desk;

	/* m for mouse */
	mx = ev->cur.canvas.x;
	my = ev->cur.canvas.y;

	/* find desk at pointer */
	desk = _pager_face_desk_at_coord(pw->desk->face, mx, my);

	if (desk)
	  {
	   e_layout_coord_canvas_to_virtual(desk->layout_object, mx + pw->drag.dx, my + pw->drag.dy, &vx, &vy);

	   if (desk != pw->desk) e_border_desk_set(pw->border, desk->desk);
	   e_border_move(pw->border, vx + desk->desk->zone->x, vy + desk->desk->zone->y);
	  }
	else
	  {
	     /* not over a desk, start dnd drag */
	     if (pw->window_object)
	       {
		  E_Drag *drag;
		  Evas_Object *o, *oo;
		  Evas_Coord x, y, w, h;
		  const char *file, *part;
		  const char *drag_types[] = { "enlightenment/pager_win" };

		  evas_object_geometry_get(pw->window_object,
			&x, &y, &w, &h);

		  /* XXX this relies on screen and canvas coords matching. is this a valid assumption? */
		  drag = e_drag_new(pw->desk->face->zone->container, x, y,
				    drag_types, 1, pw, -1,
				    _pager_window_cb_drag_finished);

		  
		  o = edje_object_add(drag->evas);
		  edje_object_file_get(pw->window_object, &file, &part);
		  edje_object_file_set(o, file, part);

		  oo = o;

		  o = edje_object_add(drag->evas);
		  edje_object_file_get(pw->icon_object, &file, &part);
		  edje_object_file_set(o, file, part);
		  edje_object_part_swallow(oo, "icon", o);

		  e_drag_object_set(drag, oo);

		  e_drag_resize(drag, w, h);
		  e_drag_start(drag, x - pw->drag.dx, y - pw->drag.dy);

		  /* this prevents the desk from switching on drags */
		  pw->drag.from_face = pw->desk->face;
		  pw->drag.from_face->dragging = 1;
		  evas_event_feed_mouse_up(pw->desk->face->evas, 1,
			EVAS_BUTTON_NONE, ecore_time_get(), NULL);
	       }
	     pw->drag.in_pager = 0;
	  }
     }
}

static void
_pager_window_cb_drag_finished(E_Drag *drag, int dropped)
{
   Pager_Win *pw;

   pw = drag->data;

   if (!pw) return;

   if (!dropped)
     {
	/* wasn't dropped (on pager). move it to position of mouse on screen */
	int x, y, dx, dy;
	E_Container *cont;
	E_Zone *zone;
	E_Desk *desk;

	cont = e_container_current_get(e_manager_current_get());
	zone = e_zone_current_get(cont);
	desk = e_desk_current_get(zone);

	e_border_zone_set(pw->border, zone);
	e_border_desk_set(pw->border, desk);

	ecore_x_pointer_last_xy_get(&x, &y);
	x = x + zone->x;
	y = y + zone->y;

	dx = (pw->border->w / 2);
	dy = (pw->border->h / 2);

	/* offset so that center of window is on mouse, but keep within desk bounds */
	if (dx < x)
	  {
	     x -= dx;
	     if ((pw->border->w < zone->w) && (x + pw->border->w > zone->x + zone->w))
	       {
		  x -= x + pw->border->w - (zone->x + zone->w);
	       }
	  }
	else x = 0;

	if (dy < y)
	  {
	     y -= dy;
	     if ((pw->border->h < zone->h) && (y + pw->border->h > zone->y + zone->h))
	       {
		  y -= y + pw->border->h - (zone->y + zone->h);
	       }
	  }
	else y = 0;

	e_border_move(pw->border, x, y);
	
     }
   if (pw && pw->drag.from_face)
     {
	pw->drag.from_face->dragging = 0;
     }

}

static void
_pager_desk_cb_mouse_wheel(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Wheel *ev;
   Evas_List              *l;
   Pager_Desk             *desk;
   Pager_Face             *face;

   ev = event_info;
   desk = data;
   face = desk->face;
   l = face->desks;

   for (l = face->desks; l; l = l->next)
     {
	Pager_Desk *pd;
	pd = l->data;
	if (pd->current)
	  {
	     /* Mouse wheel up, scroll back through desks */
	     if (ev->z < 0)
	       {
		  if (l->prev)
		    {
		       pd = l->prev->data;
		       e_desk_show(pd->desk);
		    }
		  else
		    {
		       /* We've looped around, go to the last desk. Not sure if there's a better way. */
		       Evas_List  *l2;
		       for (l2 = face->desks; l2->next; l2 = l2->next);
		       pd = l2->data;
		       e_desk_show(pd->desk);
		    }
	       }
	     /* Mouse wheel down, scroll forward through desks */
	     else if (ev->z > 0)
	       {
		  if (l->next)
		    {
		       pd = l->next->data;
		       e_desk_show(pd->desk);
		    }
		  else
		    {
		       /* We've looped around, start back at the first desk */
		       pd = face->desks->data;
		       e_desk_show(pd->desk);
		    }
	       }
	     break;
	  }
     }
}
/*****/

static void
_pager_face_cb_enter(void *data, const char *type, void *event_info)
{
   E_Event_Dnd_Enter *ev;
   Pager_Face *face;

   ev = event_info;
   face = data;
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
   Pager_Desk *desk;
   E_Border *bd;
   Evas_List *l;
   int dx = 0, dy = 0;

   ev = event_info;
   face = data;

   /* XXX convert screen -> evas coords? */ 
   desk = _pager_face_desk_at_coord(face, ev->x, ev->y);
   if (desk)
     {
	if (!strcmp(type, "enlightenment/pager_win"))
	  {
	     Pager_Win *pw;
	     pw = (Pager_Win *)(ev->data);
	     if (pw)
	       {
		  bd = pw->border;
		  dx = pw->drag.dx;
		  dy = pw->drag.dy;
	       }
	  }
	else if (!strcmp(type, "enlightenment/border"))
	  {
	     Evas_Coord wx, wy, wx2, wy2;
	     bd = ev->data;
	     e_layout_coord_virtual_to_canvas(desk->layout_object, bd->x, bd->y, &wx, &wy);
	     e_layout_coord_virtual_to_canvas(desk->layout_object, bd->x + bd->w, bd->y + bd->h, &wx2, &wy2);
	     dx = (wx - wx2) / 2;
	     dy = (wy - wy2) / 2;
	  }
	else
	  {
	     return;
	  }

	if ((bd) && (desk))
	  {
	     Evas_Coord nx, ny;
	     e_border_desk_set(bd, desk->desk);
	     e_layout_coord_canvas_to_virtual(desk->layout_object, ev->x + dx, ev->y + dy, &nx, &ny);

	     e_border_move(bd, nx + desk->desk->zone->x, ny + desk->desk->zone->y);
	  }

     }

     for (l = face->desks; l; l = l->next)
       {
	  Pager_Desk *pd;
	  pd = l->data;
	  edje_object_signal_emit(pd->desk_object, "drag", "out");
       }
}

static void
_pager_face_deskname_position_change(Pager_Face *face)
{
   switch (face->pager->conf->deskname_pos)
     {
      case PAGER_DESKNAME_NONE:
	 edje_object_signal_emit(face->pager_object, "desktop_name,none", "");
	 break;
      case PAGER_DESKNAME_TOP:
	 edje_object_signal_emit(face->pager_object, "desktop_name,top", "");
	 break;
      case PAGER_DESKNAME_BOTTOM:
	 edje_object_signal_emit(face->pager_object, "desktop_name,bottom", "");
	 break;
      case PAGER_DESKNAME_LEFT:
	 edje_object_signal_emit(face->pager_object, "desktop_name,left", "");
	 break;
      case PAGER_DESKNAME_RIGHT:
	 edje_object_signal_emit(face->pager_object, "desktop_name,right", "");
	 break;
     }
}

static void
_pager_menu_cb_aspect_keep_height(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Pager *pager;
   Evas_List *l;

   pager = data;

   for (l = pager->faces; l; l = l->next)
     {
	Pager_Face *face;
	int w, h;

	face = l->data;

	h = ((face->fh - (face->inset.t + face->inset.b)) / face->ynum);

	w = h * face->zone->w / (double)face->zone->h;
	w *= face->xnum;
	w += (face->inset.l + face->inset.r);

	e_gadman_client_resize(face->gmc, w, face->fh);
     }
}

static void
_pager_menu_cb_aspect_keep_width(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Pager *pager;
   Evas_List *l;

   pager = data;

   for (l = pager->faces; l; l = l->next)
     {
	Pager_Face *face;
	int w, h;

	face = l->data;

	w = ((face->fw - (face->inset.l + face->inset.r)) / face->xnum);

	h = w * face->zone->h / (double)face->zone->w;
	h *= face->ynum;
	h += (face->inset.t + face->inset.b);

	e_gadman_client_resize(face->gmc, face->fw, h);
     }
}

static void 
_pager_menu_cb_configure(void *data, E_Menu *m, E_Menu_Item *mi) 
{
   Pager *p;
   
   p = data;
   if (!p) return;
   _config_pager_module(e_container_current_get(e_manager_current_get()), p);
}

void 
_pager_cb_config_updated(void *data) 
{
   Pager *pager;
   Evas_List *l;

   /* Handle Desktop Name Position Change */
   pager = data;
   for (l = pager->faces; l; l = l->next)
     {
	Pager_Face *face;

	face = l->data;
	_pager_face_deskname_position_change(face);
     }   
}

