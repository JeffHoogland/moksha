/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"
#include <math.h>

/* TODO List:
 *
 * * Create separate config for each box
 * * Fix menu
 *
 * * icon labels & label tooltips supported for the name of the app
 * * use part list to know how many icons & where to put in the overlay of an icon
 * * description bubbles/tooltips for icons
 * * support dynamic iconsize change on the fly
 * * app subdirs - need to somehow handle these...
 * * use overlay object and repeat events for doing auto hide/show
 * * emit signals on hide/show due to autohide/show
 * * virtualise autoshow/hide to later allow for key bindings, mouse events elsewhere, ipc and other singals to show/hide
 *
 * BONUS Features (maybe do this later):
 *
 * * allow ibox icons to be dragged around to re-order/delete
 *
 */

static int box_count;
static E_Config_DD *conf_edd;
static E_Config_DD *conf_box_edd;

/* const strings */
static const char *_ibox_main_orientation[] =
{"left", "right", "top", "bottom"};

/* module private routines */
static IBox   *_ibox_new();
static void    _ibox_free(IBox *ib);
static void    _ibox_config_menu_new(IBox *ib);

static IBox_Box *_ibox_box_new(IBox *ib, E_Container *con);
static void    _ibox_box_free(IBox_Box *ibb);
static void    _ibox_box_menu_new(IBox_Box *ibb);
static void    _ibox_box_disable(IBox_Box *ibb);
static void    _ibox_box_frame_resize(IBox_Box *ibb);
static void    _ibox_box_edge_change(IBox_Box *ibb, int edge);
static void    _ibox_box_update_policy(IBox_Box *ibb);
static void    _ibox_box_motion_handle(IBox_Box *ibb, Evas_Coord mx, Evas_Coord my);
static void    _ibox_box_timer_handle(IBox_Box *ibb);
static void    _ibox_box_follower_reset(IBox_Box *ibb);

static IBox_Icon *_ibox_icon_new(IBox_Box *ibb, E_Border *bd);
static void    _ibox_icon_free(IBox_Icon *ic);
static IBox_Icon *_ibox_icon_find(IBox_Box *ibb, E_Border *bd);

static void    _ibox_box_cb_gmc_change(void *data, E_Gadman_Client *gmc, E_Gadman_Change change);
static void    _ibox_box_cb_intercept_move(void *data, Evas_Object *o, Evas_Coord x, Evas_Coord y);
static void    _ibox_box_cb_intercept_resize(void *data, Evas_Object *o, Evas_Coord w, Evas_Coord h);
static void    _ibox_box_cb_mouse_in(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void    _ibox_box_cb_mouse_out(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void    _ibox_box_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void    _ibox_box_cb_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void    _ibox_box_cb_mouse_move(void *data, Evas *e, Evas_Object *obj, void *event_info);
static int     _ibox_box_cb_timer(void *data);
static int     _ibox_box_cb_animator(void *data);

static int     _ibox_box_cb_event_border_iconify(void *data, int type, void *event);
static int     _ibox_box_cb_event_border_uniconify(void *data, int type, void *event);
static int     _ibox_box_cb_event_border_remove(void *data, int type, void *event);

static void    _ibox_icon_cb_intercept_move(void *data, Evas_Object *o, Evas_Coord x, Evas_Coord y);
static void    _ibox_icon_cb_intercept_resize(void *data, Evas_Object *o, Evas_Coord w, Evas_Coord h);
static void    _ibox_icon_cb_mouse_in(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void    _ibox_icon_cb_mouse_out(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void    _ibox_icon_cb_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info);

static void    _ibox_box_cb_menu_edit(void *data, E_Menu *m, E_Menu_Item *mi);
static void    _ibox_box_cb_menu_configure(void *data, E_Menu *m, E_Menu_Item *mi);

/* Config Updated Function Protos */
static void    _ibox_box_cb_width_auto(void *data);
static void    _ibox_box_cb_follower(void *data);
static void    _ibox_box_cb_iconsize_change(void *data);

/* public module routines. all modules must have these */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
     "IBox"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   IBox *ib;

   /* actually init ibox */
   ib = _ibox_new();
   m->config_menu = ib->config_menu;
   return ib;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   IBox *ib;

   if (m->config_menu)
     m->config_menu = NULL;

   ib = m->data;
   if (ib) 
     {
	if (ib->config_dialog) 
	  {
	     e_object_del(E_OBJECT(ib->config_dialog));
	     ib->config_dialog = NULL;
	  }
	_ibox_free(ib);	
     }  
   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   IBox *ib;

   ib = m->data;
   e_config_domain_save("module.ibox", conf_edd, ib->conf);
   return 1;
}

EAPI int
e_modapi_info(E_Module *m)
{
   char buf[4096];

   snprintf(buf, sizeof(buf), "%s/module_icon.png", e_module_dir_get(m));
   m->icon_file = strdup(buf);
   return 1;
}

EAPI int
e_modapi_about(E_Module *m)
{
   e_module_dialog_show(_("Enlightenment IBox Module"),
			_("This is the IBox Iconified Application module for Enlightenment.<br>"
			  "It will hold minimized applications"));
   return 1;
}

EAPI int
e_modapi_config(E_Module *m)
{
   IBox *e;
   Evas_List *l;
   
   e = m->data;
   if (!e) return 0;
   if (!e->boxes) return 0;
   
   for (l = e->boxes; l; l = l->next) 
     {
	IBox_Box *face;
	face = l->data;
	if (!face) return 0;
	if (face->con == e_container_current_get(e_manager_current_get())) 
	  {
	     _config_ibox_module(face->con, face->ibox);
	     break;
	  }	
     }
   return 1;
}

/* module private routines */
static IBox *
_ibox_new()
{
   IBox *ib;
   Evas_List *managers, *l, *l2, *cl;

   box_count = 0;
   ib = E_NEW(IBox, 1);
   if (!ib) return NULL;

   conf_box_edd = E_CONFIG_DD_NEW("IBox_Config_Box", Config_Box);
#undef T
#undef D
#define T Config_Box
#define D conf_box_edd
   E_CONFIG_VAL(D, T, enabled, UCHAR);

   conf_edd = E_CONFIG_DD_NEW("IBox_Config", Config);
#undef T
#undef D
#define T Config
#define D conf_edd
   E_CONFIG_VAL(D, T, follower, INT);
   E_CONFIG_VAL(D, T, follow_speed, DOUBLE);
   E_CONFIG_VAL(D, T, autoscroll_speed, DOUBLE);
   E_CONFIG_VAL(D, T, iconsize, INT);
   E_CONFIG_VAL(D, T, width, INT);
   E_CONFIG_LIST(D, T, boxes, conf_box_edd);

   ib->conf = e_config_domain_load("module.ibox", conf_edd);
   if (!ib->conf)
     {
	ib->conf = E_NEW(Config, 1);
	ib->conf->follower = 1;
	ib->conf->follow_speed = 0.9;
	ib->conf->autoscroll_speed = 0.95;
	ib->conf->iconsize = 24;
	ib->conf->width = IBOX_WIDTH_AUTO;
     }
   E_CONFIG_LIMIT(ib->conf->follow_speed, 0.01, 1.0);
   E_CONFIG_LIMIT(ib->conf->autoscroll_speed, 0.01, 1.0);
   E_CONFIG_LIMIT(ib->conf->iconsize, 2, 400);
   E_CONFIG_LIMIT(ib->conf->width, -2, -1);

   _ibox_config_menu_new(ib);

   managers = e_manager_list();
   cl = ib->conf->boxes;
   for (l = managers; l; l = l->next)
     {
	E_Manager *man;

	man = l->data;
	for (l2 = man->containers; l2; l2 = l2->next)
	  {
	     E_Container *con;
	     IBox_Box *ibb;
	     /* Config */
	     con = l2->data;
	     ibb = _ibox_box_new(ib, con);
	     if (ibb)
	       {
		  E_Menu_Item *mi;

		  if (!cl)
		    {
		       ibb->conf = E_NEW(Config_Box, 1);
		       ibb->conf->enabled = 1;
		       ib->conf->boxes = evas_list_append(ib->conf->boxes, ibb->conf);
		    }
		  else
		    {
		       ibb->conf = cl->data;
		       cl = cl->next;
		    }
		  /* Menu */
		  _ibox_box_menu_new(ibb);

		  /* Add main menu to box menu */
		  mi = e_menu_item_new(ib->config_menu);
		  e_menu_item_label_set(mi, _("Configuration"));
		  e_util_menu_item_edje_icon_set(mi, "enlightenment/configuration");		  
		  e_menu_item_callback_set(mi, _ibox_box_cb_menu_configure, ibb);

		  mi = e_menu_item_new(ib->config_menu);
		  e_menu_item_label_set(mi, con->name);
		  e_menu_item_submenu_set(mi, ibb->menu);

		  /* Setup */
		  if (!ibb->conf->enabled)
		    _ibox_box_disable(ibb);

	       }
	  }
     }
   return ib;
}

static void
_ibox_free(IBox *ib)
{
   E_CONFIG_DD_FREE(conf_edd);
   E_CONFIG_DD_FREE(conf_box_edd);

   while (ib->boxes)
     _ibox_box_free(ib->boxes->data);

   e_object_del(E_OBJECT(ib->config_menu));
   evas_list_free(ib->conf->boxes);
   free(ib->conf);
   free(ib);
}

static IBox_Box *
_ibox_box_new(IBox *ib, E_Container *con)
{
   IBox_Box *ibb;
   Evas_Object *o;
   E_Gadman_Policy policy;
   Evas_Coord x, y, w, h;
   E_Border_List *bl;
   E_Border *bd;

   ibb = E_NEW(IBox_Box, 1);
   if (!ibb) return NULL;
   ibb->ibox = ib;
   ib->boxes = evas_list_append(ib->boxes, ibb);

   ibb->con = con;
   e_object_ref(E_OBJECT(con));
   ibb->evas = con->bg_evas;

   ibb->x = ibb->y = ibb->w = ibb->h = -1;

   evas_event_freeze(ibb->evas);
   o = edje_object_add(ibb->evas);
   ibb->box_object = o;
   e_theme_edje_object_set(o, "base/theme/modules/ibox",
			   "modules/ibox/main");
   evas_object_show(o);

   if (ibb->ibox->conf->follower)
     {
	o = edje_object_add(ibb->evas);
	ibb->overlay_object = o;
	evas_object_layer_set(o, 1);
	e_theme_edje_object_set(o, "base/theme/modules/ibox",
				"modules/ibox/follower");
	evas_object_show(o);
     }

   o = evas_object_rectangle_add(ibb->evas);
   ibb->event_object = o;
   evas_object_layer_set(o, 2);
   evas_object_repeat_events_set(o, 1);
   evas_object_color_set(o, 0, 0, 0, 0);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_IN,  _ibox_box_cb_mouse_in,  ibb);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_OUT, _ibox_box_cb_mouse_out, ibb);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _ibox_box_cb_mouse_down, ibb);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_UP, _ibox_box_cb_mouse_up, ibb);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_MOVE, _ibox_box_cb_mouse_move, ibb);
   evas_object_show(o);

   o = e_box_add(ibb->evas);
   ibb->item_object = o;
   evas_object_intercept_move_callback_add(o, _ibox_box_cb_intercept_move, ibb);
   evas_object_intercept_resize_callback_add(o, _ibox_box_cb_intercept_resize, ibb);
   e_box_freeze(o);
   edje_object_part_swallow(ibb->box_object, "items", o);
   evas_object_show(o);

   ibb->ev_handler_border_iconify =
     ecore_event_handler_add(E_EVENT_BORDER_ICONIFY, _ibox_box_cb_event_border_iconify, ibb);
   ibb->ev_handler_border_uniconify =
     ecore_event_handler_add(E_EVENT_BORDER_UNICONIFY, _ibox_box_cb_event_border_uniconify, ibb);
   ibb->ev_handler_border_remove =
     ecore_event_handler_add(E_EVENT_BORDER_REMOVE, _ibox_box_cb_event_border_remove, ibb);

   bl = e_container_border_list_first(ibb->con);
   while ((bd = e_container_border_list_next(bl)))
     {
	IBox_Icon *ic;

	if (!bd->iconic) continue;

	ic = _ibox_icon_new(ibb, bd);
     }
   e_container_border_list_free(bl);

   ibb->align_req = 0.5;
   ibb->align = 0.5;
   e_box_align_set(ibb->item_object, 0.5, 0.5);

   evas_object_resize(ibb->box_object, 1000, 1000);
   edje_object_calc_force(ibb->box_object);
   edje_object_part_geometry_get(ibb->box_object, "items", &x, &y, &w, &h);
   ibb->box_inset.l = x;
   ibb->box_inset.r = 1000 - (x + w);
   ibb->box_inset.t = y;
   ibb->box_inset.b = 1000 - (y + h);

   /* Calculate icon inset */
   o = edje_object_add(ibb->evas);
   e_theme_edje_object_set(o, "base/theme/modules/ibox",
	                   "modules/ibox/icon");
   evas_object_resize(o, 100, 100);
   edje_object_calc_force(o);
   edje_object_part_geometry_get(o, "item", &x, &y, &w, &h);
   ibb->icon_inset.l = x;
   ibb->icon_inset.r = 100 - (x + w);
   ibb->icon_inset.t = y;
   ibb->icon_inset.b = 100 - (y + h);
   evas_object_del(o);

   e_box_thaw(ibb->item_object);

   ibb->gmc = e_gadman_client_new(ibb->con->gadman);
   e_gadman_client_domain_set(ibb->gmc, "module.ibox", box_count++);
   policy = E_GADMAN_POLICY_EDGES | E_GADMAN_POLICY_HMOVE | E_GADMAN_POLICY_VMOVE;
   if (ibb->ibox->conf->width == IBOX_WIDTH_FIXED)
     policy |= E_GADMAN_POLICY_VSIZE;

   e_gadman_client_policy_set(ibb->gmc, policy);
   e_gadman_client_min_size_set(ibb->gmc, 8, 8);
   e_gadman_client_max_size_set(ibb->gmc, 3200, 3200);
   e_gadman_client_auto_size_set(ibb->gmc, -1, -1);
   e_gadman_client_align_set(ibb->gmc, 1.0, 1.0);
   e_gadman_client_resize(ibb->gmc, 400, 32 + ibb->box_inset.t + ibb->box_inset.b);
   e_gadman_client_change_func_set(ibb->gmc, _ibox_box_cb_gmc_change, ibb);
   e_gadman_client_edge_set(ibb->gmc, E_GADMAN_EDGE_BOTTOM);
   e_gadman_client_load(ibb->gmc);

   evas_event_thaw(ibb->evas);

   /* We need to resize, if the width is auto and the number
    * of apps has changed since last startup */
   _ibox_box_frame_resize(ibb);

   /*
   edje_object_signal_emit(ibb->box_object, "passive", "");
   edje_object_signal_emit(ibb->overlay_object, "passive", "");
   */

   return ibb;
}

static void
_ibox_box_free(IBox_Box *ibb)
{
   e_object_unref(E_OBJECT(ibb->con));
   e_object_del(E_OBJECT(ibb->menu));

   ecore_event_handler_del(ibb->ev_handler_border_iconify);
   ecore_event_handler_del(ibb->ev_handler_border_uniconify);
   ecore_event_handler_del(ibb->ev_handler_border_remove);

   while (ibb->icons)
     _ibox_icon_free(ibb->icons->data);

   if (ibb->timer) ecore_timer_del(ibb->timer);
   if (ibb->animator) ecore_animator_del(ibb->animator);
   evas_object_del(ibb->box_object);
   if (ibb->overlay_object) evas_object_del(ibb->overlay_object);
   evas_object_del(ibb->item_object);
   evas_object_del(ibb->event_object);

   e_gadman_client_save(ibb->gmc);
   e_object_del(E_OBJECT(ibb->gmc));

   ibb->ibox->boxes = evas_list_remove(ibb->ibox->boxes, ibb);

   free(ibb->conf);
   free(ibb);
   box_count--;
}

static void
_ibox_box_menu_new(IBox_Box *ibb)
{
   E_Menu *mn;
   E_Menu_Item *mi;

   mn = e_menu_new();
   ibb->menu = mn;

   /* Config */
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Configuration"));
   e_util_menu_item_edje_icon_set(mi, "enlightenment/configuration");   
   e_menu_item_callback_set(mi, _ibox_box_cb_menu_configure, ibb);

   /* Edit */
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Edit Mode"));
   e_util_menu_item_edje_icon_set(mi, "enlightenment/gadgets");   
   e_menu_item_callback_set(mi, _ibox_box_cb_menu_edit, ibb);
}

static void
_ibox_box_cb_menu_configure(void *data, E_Menu *m, E_Menu_Item *mi)
{
   IBox_Box *ibb;

   ibb = (IBox_Box *)data;
   if (!ibb) return;
   _config_ibox_module(ibb->con, ibb->ibox);
}

static void
_ibox_box_disable(IBox_Box *ibb)
{
   ibb->conf->enabled = 0;
   evas_object_hide(ibb->box_object);
   if (ibb->overlay_object) evas_object_hide(ibb->overlay_object);
   evas_object_hide(ibb->item_object);
   evas_object_hide(ibb->event_object);
   e_config_save_queue();
}

static IBox_Icon *
_ibox_icon_new(IBox_Box *ibb, E_Border *bd)
{
   IBox_Icon *ic;
   Evas_Object *o;
   Evas_Coord w, h;

   /* FIXME: Add default icon! */
   if (!bd->icon_object) return NULL;

   ic = E_NEW(IBox_Icon, 1);
   if (!ic) return NULL;
   ic->ibb = ibb;
   ic->border = bd;
   e_object_ref(E_OBJECT(bd));
   ibb->icons = evas_list_append(ibb->icons, ic);

   o = evas_object_rectangle_add(ibb->evas);
   ic->event_object = o;
   evas_object_layer_set(o, 1);
   evas_object_clip_set(o, evas_object_clip_get(ibb->item_object));
   evas_object_color_set(o, 0, 0, 0, 0);
   evas_object_repeat_events_set(o, 1);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_IN,  _ibox_icon_cb_mouse_in,  ic);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_OUT, _ibox_icon_cb_mouse_out, ic);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_UP, _ibox_icon_cb_mouse_up, ic);
   evas_object_show(o);

   o = edje_object_add(ibb->evas);
   ic->bg_object = o;
   evas_object_intercept_move_callback_add(o, _ibox_icon_cb_intercept_move, ic);
   evas_object_intercept_resize_callback_add(o, _ibox_icon_cb_intercept_resize, ic);
   e_theme_edje_object_set(o, "base/theme/modules/ibox",
			   "modules/ibox/icon");
   evas_object_show(o);

   o = e_border_icon_add(ic->border, ibb->evas);
   ic->icon_object = o;
   evas_object_resize(o, ibb->ibox->conf->iconsize, ibb->ibox->conf->iconsize);
   edje_object_part_swallow(ic->bg_object, "item", o);
   evas_object_pass_events_set(o, 1);
   evas_object_show(o);

   o = edje_object_add(ibb->evas);
   ic->overlay_object = o;
   evas_object_intercept_move_callback_add(o, _ibox_icon_cb_intercept_move, ic);
   evas_object_intercept_resize_callback_add(o, _ibox_icon_cb_intercept_resize, ic);
   e_theme_edje_object_set(o, "base/theme/modules/ibox",
			   "modules/ibox/icon_overlay");
   evas_object_show(o);

   evas_object_raise(ic->event_object);

   w = ibb->ibox->conf->iconsize + ibb->icon_inset.l + ibb->icon_inset.r;
   h = ibb->ibox->conf->iconsize + ibb->icon_inset.t + ibb->icon_inset.b;
   e_box_pack_end(ibb->item_object, ic->bg_object);
   e_box_pack_options_set(ic->bg_object,
			  1, 1, /* fill */
			  0, 0, /* expand */
			  0.5, 0.5, /* align */
			  w, h, /* min */
			  w, h /* max */
			  );

   /*
   str = (char *)edje_object_data_get(ic->icon_object, "raise_on_hilight");
   if (str)
     {
	if (atoi(str) == 1) ic->raise_on_hilight = 1;
     }
   */

   edje_object_signal_emit(ic->bg_object, "passive", "");
   edje_object_signal_emit(ic->overlay_object, "passive", "");
   return ic;
}

static void
_ibox_icon_free(IBox_Icon *ic)
{
   if (!ic) return;

   ic->ibb->icons = evas_list_remove(ic->ibb->icons, ic);
   if (ic->bg_object) evas_object_del(ic->bg_object);
   if (ic->overlay_object) evas_object_del(ic->overlay_object);
   if (ic->icon_object) evas_object_del(ic->icon_object);
   if (ic->event_object) evas_object_del(ic->event_object);
   e_object_unref(E_OBJECT(ic->border));
   free(ic);
}

static IBox_Icon *
_ibox_icon_find(IBox_Box *ibb, E_Border *bd)
{
   Evas_List *l;

   for (l = ibb->icons; l; l = l->next)
     {
	IBox_Icon *ic;

	ic = l->data;
	if (ic->border == bd) return ic;
     }
   return NULL;
}

void
_ibox_config_menu_new(IBox *ib)
{
   E_Menu *mn;

   mn = e_menu_new();
   ib->config_menu = mn;
}

static void
_ibox_box_frame_resize(IBox_Box *ibb)
{
   Evas_Coord w, h, bw, bh;
   /* Not finished loading config yet! */
   if ((ibb->x == -1) ||
       (ibb->y == -1) ||
       (ibb->w == -1) ||
       (ibb->h == -1))
     return;

   evas_event_freeze(ibb->evas);
   e_box_freeze(ibb->item_object);

   if (e_box_pack_count_get(ibb->item_object))
     e_box_min_size_get(ibb->item_object, &w, &h);
   else
     {
	/* FIXME: This isn't correct with FIXED_WIDTH! */
	w = ibb->ibox->conf->iconsize + ibb->icon_inset.l + ibb->icon_inset.r;
	h = ibb->ibox->conf->iconsize + ibb->icon_inset.t + ibb->icon_inset.b;
     }
   edje_extern_object_min_size_set(ibb->item_object, w, h);
   edje_object_part_swallow(ibb->box_object, "items", ibb->item_object);
   edje_object_size_min_calc(ibb->box_object, &bw, &bh);
   edje_extern_object_min_size_set(ibb->item_object, 0, 0);
   edje_object_part_swallow(ibb->box_object, "items", ibb->item_object);

   e_box_thaw(ibb->item_object);
   evas_event_thaw(ibb->evas);

   if (ibb->ibox->conf->width == IBOX_WIDTH_AUTO)
     {
	e_gadman_client_resize(ibb->gmc, bw, bh);
     }
   else
     {
	if ((e_gadman_client_edge_get(ibb->gmc) == E_GADMAN_EDGE_LEFT) ||
	    (e_gadman_client_edge_get(ibb->gmc) == E_GADMAN_EDGE_RIGHT))
	  {
	     /* h is the width of the box */
	     e_gadman_client_resize(ibb->gmc, bw, ibb->h);
	  }
	else if ((e_gadman_client_edge_get(ibb->gmc) == E_GADMAN_EDGE_TOP) ||
	         (e_gadman_client_edge_get(ibb->gmc) == E_GADMAN_EDGE_BOTTOM))
	  {
	     /* w is the width of the box */
	     e_gadman_client_resize(ibb->gmc, ibb->w, bh);
	  }
     }
}

static void
_ibox_box_edge_change(IBox_Box *ibb, int edge)
{
   Evas_List *l;
   Evas_Coord bw, bh, tmp;
   Evas_Object *o;
   E_Gadman_Policy policy;
   int changed;

   evas_event_freeze(ibb->evas);
   o = ibb->box_object;
   edje_object_signal_emit(o, "set_orientation", _ibox_main_orientation[edge]);
   edje_object_message_signal_process(o);

   if (ibb->overlay_object)
     {
	o = ibb->overlay_object;
	edje_object_signal_emit(o, "set_orientation", _ibox_main_orientation[edge]);
	edje_object_message_signal_process(o);
     }

   e_box_freeze(ibb->item_object);

   for (l = ibb->icons; l; l = l->next)
     {
	IBox_Icon *ic;

	ic = l->data;
	o = ic->bg_object;
	edje_object_signal_emit(o, "set_orientation", _ibox_main_orientation[edge]);
	edje_object_message_signal_process(o);

	o = ic->overlay_object;
	edje_object_signal_emit(o, "set_orientation", _ibox_main_orientation[edge]);
	edje_object_message_signal_process(o);

	bw = ibb->ibox->conf->iconsize + ibb->icon_inset.l + ibb->icon_inset.r;
	bh = ibb->ibox->conf->iconsize + ibb->icon_inset.t + ibb->icon_inset.b;
	e_box_pack_options_set(ic->bg_object,
			       1, 1, /* fill */
			       0, 0, /* expand */
			       0.5, 0.5, /* align */
			       bw, bh, /* min */
			       bw, bh /* max */
			       );
     }

   ibb->align_req = 0.5;
   ibb->align = 0.5;
   e_box_align_set(ibb->item_object, 0.5, 0.5);

   policy = E_GADMAN_POLICY_EDGES | E_GADMAN_POLICY_HMOVE | E_GADMAN_POLICY_VMOVE;

   if ((edge == E_GADMAN_EDGE_BOTTOM) ||
       (edge == E_GADMAN_EDGE_TOP))
     {
	changed = (e_box_orientation_get(ibb->item_object) != 1);
	if (changed)
	  {
	     e_box_orientation_set(ibb->item_object, 1);
	     if (ibb->ibox->conf->width == IBOX_WIDTH_FIXED)
	       policy |= E_GADMAN_POLICY_HSIZE;
	     e_gadman_client_policy_set(ibb->gmc, policy);
	     tmp = ibb->w;
	     ibb->w = ibb->h;
	     ibb->h = tmp;
	  }
     }
   else if ((edge == E_GADMAN_EDGE_LEFT) ||
	    (edge == E_GADMAN_EDGE_RIGHT))
     {
	changed = (e_box_orientation_get(ibb->item_object) != 0);
	if (changed)
	  {
	     e_box_orientation_set(ibb->item_object, 0);
	     if (ibb->ibox->conf->width == IBOX_WIDTH_FIXED)
	       policy |= E_GADMAN_POLICY_VSIZE;
	     e_gadman_client_policy_set(ibb->gmc, policy);
	     tmp = ibb->w;
	     ibb->w = ibb->h;
	     ibb->h = tmp;
	  }
     }

   e_box_thaw(ibb->item_object);
   evas_event_thaw(ibb->evas);

   _ibox_box_frame_resize(ibb);
}

static void
_ibox_box_update_policy(IBox_Box *ibb)
{
   E_Gadman_Policy policy;

   policy = E_GADMAN_POLICY_EDGES | E_GADMAN_POLICY_HMOVE | E_GADMAN_POLICY_VMOVE;

   if ((e_gadman_client_edge_get(ibb->gmc) == E_GADMAN_EDGE_BOTTOM) ||
       (e_gadman_client_edge_get(ibb->gmc) == E_GADMAN_EDGE_TOP))
     {
	if (ibb->ibox->conf->width == IBOX_WIDTH_FIXED)
	  policy |= E_GADMAN_POLICY_HSIZE;
	e_gadman_client_policy_set(ibb->gmc, policy);
     }
   else if ((e_gadman_client_edge_get(ibb->gmc) == E_GADMAN_EDGE_LEFT) ||
	    (e_gadman_client_edge_get(ibb->gmc) == E_GADMAN_EDGE_RIGHT))
     {
	if (ibb->ibox->conf->width == IBOX_WIDTH_FIXED)
	  policy |= E_GADMAN_POLICY_VSIZE;
	e_gadman_client_policy_set(ibb->gmc, policy);
     }
}

static void
_ibox_box_motion_handle(IBox_Box *ibb, Evas_Coord mx, Evas_Coord my)
{
   Evas_Coord x, y, w, h;
   double relx, rely;

   evas_object_geometry_get(ibb->item_object, &x, &y, &w, &h);
   if (w > 0) relx = (double)(mx - x) / (double)w;
   else relx = 0.0;
   if (h > 0) rely = (double)(my - y) / (double)h;
   else rely = 0.0;
   if ((e_gadman_client_edge_get(ibb->gmc) == E_GADMAN_EDGE_BOTTOM) ||
       (e_gadman_client_edge_get(ibb->gmc) == E_GADMAN_EDGE_TOP))
     {
	ibb->align_req = 1.0 - relx;
	ibb->follow_req = relx;
     }
   else if ((e_gadman_client_edge_get(ibb->gmc) == E_GADMAN_EDGE_LEFT) ||
	    (e_gadman_client_edge_get(ibb->gmc) == E_GADMAN_EDGE_RIGHT))
     {
	ibb->align_req = 1.0 - rely;
	ibb->follow_req = rely;
     }
}

static void
_ibox_box_timer_handle(IBox_Box *ibb)
{
   if (!ibb->timer)
     ibb->timer = ecore_timer_add(0.01, _ibox_box_cb_timer, ibb);
   if (!ibb->animator)
     ibb->animator = ecore_animator_add(_ibox_box_cb_animator, ibb);
}

static void
_ibox_box_follower_reset(IBox_Box *ibb)
{
   Evas_Coord ww, hh, bx, by, bw, bh, d1, d2, mw, mh;

   if (!ibb->overlay_object) return;

   evas_output_viewport_get(ibb->evas, NULL, NULL, &ww, &hh);
   evas_object_geometry_get(ibb->item_object, &bx, &by, &bw, &bh);
   edje_object_size_min_get(ibb->overlay_object, &mw, &mh);
   if ((e_gadman_client_edge_get(ibb->gmc) == E_GADMAN_EDGE_BOTTOM) ||
       (e_gadman_client_edge_get(ibb->gmc) == E_GADMAN_EDGE_TOP))
     {
	d1 = bx;
	d2 = ww - (bx + bw);
	if (bw > 0)
	  {
	     if (d1 < d2)
	       ibb->follow_req = -((double)(d1 + (mw * 4)) / (double)bw);
	     else
	       ibb->follow_req = 1.0 + ((double)(d2 + (mw * 4)) / (double)bw);
	  }
     }
   else if ((e_gadman_client_edge_get(ibb->gmc) == E_GADMAN_EDGE_LEFT) ||
	    (e_gadman_client_edge_get(ibb->gmc) == E_GADMAN_EDGE_RIGHT))
     {
	d1 = by;
	d2 = hh - (by + bh);
	if (bh > 0)
	  {
	     if (d1 < d2)
	       ibb->follow_req = -((double)(d1 + (mh * 4)) / (double)bh);
	     else
	       ibb->follow_req = 1.0 + ((double)(d2 + (mh * 4)) / (double)bh);
	  }
     }
}

static void
_ibox_icon_cb_intercept_move(void *data, Evas_Object *o, Evas_Coord x, Evas_Coord y)
{
   IBox_Icon *ic;

   ic = data;
   evas_object_move(o, x, y);
   evas_object_move(ic->event_object, x, y);
   evas_object_move(ic->overlay_object, x, y);
}

static void
_ibox_icon_cb_intercept_resize(void *data, Evas_Object *o, Evas_Coord w, Evas_Coord h)
{
   IBox_Icon *ic;

   ic = data;
   evas_object_resize(o, w, h);
   evas_object_resize(ic->event_object, w, h);
   evas_object_resize(ic->overlay_object, w, h);
}

static void
_ibox_box_cb_intercept_move(void *data, Evas_Object *o, Evas_Coord x, Evas_Coord y)
{
   IBox_Box *ibb;

   ibb = data;
   evas_object_move(o, x, y);
   evas_object_move(ibb->event_object, x, y);
}

static void
_ibox_box_cb_intercept_resize(void *data, Evas_Object *o, Evas_Coord w, Evas_Coord h)
{
   IBox_Box *ibb;

   ibb = data;

   evas_object_resize(o, w, h);
   evas_object_resize(ibb->event_object, w, h);
}

static void
_ibox_icon_cb_mouse_in(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_In *ev;
   IBox_Icon *ic;

   ev = event_info;
   ic = data;
   evas_event_freeze(ic->ibb->evas);
   evas_object_raise(ic->event_object);
   if (ic->raise_on_hilight)
     evas_object_stack_below(ic->bg_object, ic->event_object);
   evas_object_stack_below(ic->overlay_object, ic->event_object);
   evas_event_thaw(ic->ibb->evas);
   edje_object_signal_emit(ic->bg_object, "active", "");
   edje_object_signal_emit(ic->overlay_object, "active", "");
   if (ic->ibb->overlay_object)
     edje_object_signal_emit(ic->ibb->overlay_object, "active", "");
}

static void
_ibox_icon_cb_mouse_out(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Out *ev;
   IBox_Icon *ic;

   ev = event_info;
   ic = data;
   edje_object_signal_emit(ic->bg_object, "passive", "");
   edje_object_signal_emit(ic->overlay_object, "passive", "");
   if (ic->ibb->overlay_object)
     edje_object_signal_emit(ic->ibb->overlay_object, "passive", "");
}

static void
_ibox_icon_cb_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Up *ev;
   IBox_Icon *ic;

   ev = event_info;
   ic = data;
   if (ev->button == 1)
     e_border_uniconify(ic->border);
}

static void
_ibox_box_cb_mouse_in(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_In *ev;
   IBox_Box *ibb;

   ev = event_info;
   ibb = data;
   if (ibb->overlay_object)
     edje_object_signal_emit(ibb->overlay_object, "active", "");
   _ibox_box_motion_handle(ibb, ev->canvas.x, ev->canvas.y);
   _ibox_box_timer_handle(ibb);
}

static void
_ibox_box_cb_mouse_out(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Out *ev;
   IBox_Box *ibb;

   ev = event_info;
   ibb = data;
   if (ibb->overlay_object)
     edje_object_signal_emit(ibb->overlay_object, "passive", "");
   _ibox_box_follower_reset(ibb);
   _ibox_box_timer_handle(ibb);
}

static void
_ibox_box_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   IBox_Box *ibb;

   ev = event_info;
   ibb = data;
   if (ev->button == 3)
     {
	e_menu_activate_mouse(ibb->menu, e_zone_current_get(ibb->con),
			      ev->output.x, ev->output.y, 1, 1,
			      E_MENU_POP_DIRECTION_DOWN, ev->timestamp);
	e_util_container_fake_mouse_up_later(ibb->con, 3);
     }
}

static void
_ibox_box_cb_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Up *ev;
   IBox_Box *ibb;

   ev = event_info;
   ibb = data;
}

static void
_ibox_box_cb_mouse_move(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Move *ev;
   IBox_Box *ibb;

   ev = event_info;
   ibb = data;
   _ibox_box_motion_handle(ibb, ev->cur.canvas.x, ev->cur.canvas.y);
   _ibox_box_timer_handle(ibb);
}

static int
_ibox_box_cb_timer(void *data)
{
   IBox_Box *ibb;
   double dif, dif2;
   double v;

   ibb = data;
   v = ibb->ibox->conf->autoscroll_speed;
   ibb->align = (ibb->align_req * (1.0 - v)) + (ibb->align * v);
   v = ibb->ibox->conf->follow_speed;
   ibb->follow = (ibb->follow_req * (1.0 - v)) + (ibb->follow * v);

   dif = ibb->align - ibb->align_req;
   if (dif < 0) dif = -dif;
   if (dif < 0.001) ibb->align = ibb->align_req;

   dif2 = ibb->follow - ibb->follow_req;
   if (dif2 < 0) dif2 = -dif2;
   if (dif2 < 0.001) ibb->follow = ibb->follow_req;

   if ((dif < 0.001) && (dif2 < 0.001))
     {
	ibb->timer = NULL;
	return 0;
     }
   return 1;
}

static int
_ibox_box_cb_animator(void *data)
{
   IBox_Box *ibb;
   Evas_Coord x, y, w, h, mw, mh;

   ibb = data;

   if ((e_gadman_client_edge_get(ibb->gmc) == E_GADMAN_EDGE_BOTTOM) ||
       (e_gadman_client_edge_get(ibb->gmc) == E_GADMAN_EDGE_TOP))
     {
        e_box_min_size_get(ibb->item_object, &mw, &mh);
	evas_object_geometry_get(ibb->item_object, NULL, NULL, &w, &h);
	if (mw > w)
	  e_box_align_set(ibb->item_object, ibb->align, 0.5);
	else
	  e_box_align_set(ibb->item_object, 0.5, 0.5);

	if (ibb->overlay_object)
	  {
	     evas_object_geometry_get(ibb->item_object, &x, &y, &w, &h);
	     edje_object_size_min_get(ibb->overlay_object, &mw, &mh);
	     evas_object_resize(ibb->overlay_object, mw, h);
	     evas_object_move(ibb->overlay_object, x + (w * ibb->follow) - (mw / 2), y);
	  }
     }
   else if ((e_gadman_client_edge_get(ibb->gmc) == E_GADMAN_EDGE_LEFT) ||
	    (e_gadman_client_edge_get(ibb->gmc) == E_GADMAN_EDGE_RIGHT))
     {
        e_box_min_size_get(ibb->item_object, &mw, &mh);
	evas_object_geometry_get(ibb->item_object, NULL, NULL, &w, &h);
	if (mh > h)
	  e_box_align_set(ibb->item_object, 0.5, ibb->align);
	else
	  e_box_align_set(ibb->item_object, 0.5, 0.5);

	if (ibb->overlay_object)
	  {
	     evas_object_geometry_get(ibb->item_object, &x, &y, &w, &h);
	     edje_object_size_min_get(ibb->overlay_object, &mw, &mh);
	     evas_object_resize(ibb->overlay_object, w, mh);
	     evas_object_move(ibb->overlay_object, x, y + (h * ibb->follow) - (mh / 2));
	  }
     }
   if (ibb->timer) return 1;
   ibb->animator = NULL;
   return 0;
}

static int
_ibox_box_cb_event_border_iconify(void *data, int type, void *event)
{
   E_Event_Border_Iconify *ev;
   IBox_Box *ibb;
   IBox_Icon *ic;

   ev = event;
   ibb = data;

   if (!_ibox_icon_find(ibb, ev->border))
     ic = _ibox_icon_new(ibb, ev->border);
   _ibox_box_frame_resize(ibb);

   return 1;
}

static int
_ibox_box_cb_event_border_uniconify(void *data, int type, void *event)
{
   E_Event_Border_Uniconify *ev;
   IBox_Box *ibb;
   IBox_Icon *ic;

   ev = event;
   ibb = data;

   ic = _ibox_icon_find(ibb, ev->border);
   if (ic)
     _ibox_icon_free(ic);
   _ibox_box_frame_resize(ibb);

   return 1;
}

static int
_ibox_box_cb_event_border_remove(void *data, int type, void *event)
{
   E_Event_Border_Remove *ev;
   IBox_Box *ibb;
   IBox_Icon *ic;

   ev = event;
   ibb = data;

   ic = _ibox_icon_find(ibb, ev->border);
   if (ic)
     _ibox_icon_free(ic);
   _ibox_box_frame_resize(ibb);

   return 1;
}

static void
_ibox_box_cb_gmc_change(void *data, E_Gadman_Client *gmc, E_Gadman_Change change)
{
   IBox_Box *ibb;

   ibb = data;
   switch (change)
     {
      case E_GADMAN_CHANGE_MOVE_RESIZE:
	e_gadman_client_geometry_get(ibb->gmc, &ibb->x, &ibb->y, &ibb->w, &ibb->h);

	edje_extern_object_min_size_set(ibb->item_object, 0, 0);
	edje_object_part_swallow(ibb->box_object, "items", ibb->item_object);

	evas_object_move(ibb->box_object, ibb->x, ibb->y);
	evas_object_resize(ibb->box_object, ibb->w, ibb->h);

	_ibox_box_follower_reset(ibb);
	_ibox_box_timer_handle(ibb);

	break;
      case E_GADMAN_CHANGE_EDGE:
	_ibox_box_edge_change(ibb, e_gadman_client_edge_get(ibb->gmc));
	break;
      case E_GADMAN_CHANGE_RAISE:
      case E_GADMAN_CHANGE_ZONE:
	 /* Ignore */
	break;
     }
}

static void
_ibox_box_cb_width_auto(void *data)
{
   IBox          *ib;
   IBox_Box      *ibb;
   Evas_List     *l;

   ib = (IBox *)data;
   for (l = ib->boxes; l; l = l->next)
     {
	ibb = l->data;
	_ibox_box_update_policy(ibb);
	_ibox_box_frame_resize(ibb);
     }
}

static void
_ibox_box_cb_iconsize_change(void *data)
{
   IBox *ib;
   IBox_Box *ibb;
   Evas_List *l, *ll;

   ib = (IBox *)data;
   for (l = ib->boxes; l; l = l->next)
     {
	ibb = l->data;

	e_box_freeze(ibb->item_object);
	for (ll = ibb->icons; ll; ll = ll->next)
	  {
	     IBox_Icon *ic;
	     Evas_Object *o;
	     Evas_Coord bw, bh;

	     ic = ll->data;
	     o = ic->icon_object;
	     evas_object_resize(o, ibb->ibox->conf->iconsize, ibb->ibox->conf->iconsize);
	     edje_object_part_swallow(ic->bg_object, "item", o);

	     bw = ibb->ibox->conf->iconsize + ibb->icon_inset.l + ibb->icon_inset.r;
	     bh = ibb->ibox->conf->iconsize + ibb->icon_inset.t + ibb->icon_inset.b;
	     e_box_pack_options_set(ic->bg_object,
				    1, 1, /* fill */
				    0, 0, /* expand */
				    0.5, 0.5, /* align */
				    bw, bh, /* min */
				    bw, bh /* max */
				    );
	  }
	e_box_thaw(ibb->item_object);
	_ibox_box_frame_resize(ibb);
     }
}

static void
_ibox_box_cb_menu_edit(void *data, E_Menu *m, E_Menu_Item *mi)
{
   IBox_Box *ibb;

   ibb = data;
   e_gadman_mode_set(ibb->gmc->gadman, E_GADMAN_MODE_EDIT);
}

static void
_ibox_box_cb_follower(void *data)
{
   IBox          *ib;
   IBox_Box      *ibb;
   unsigned char  enabled;
   Evas_List     *l;

   ib = (IBox *)data;
   enabled = ib->conf->follower;
   if (enabled)
     {
	for (l = ib->boxes; l; l = l->next)
	  {
	     Evas_Object *o;

	     ibb = l->data;
	     if (ibb->overlay_object) continue;
	     o = edje_object_add(ibb->evas);
	     ibb->overlay_object = o;
	     evas_object_layer_set(o, 1);
	     e_theme_edje_object_set(o, "base/theme/modules/ibox",
				     "modules/ibox/follower");
	     edje_object_signal_emit(o, "set_orientation", _ibox_main_orientation[e_gadman_client_edge_get(ibb->gmc)]);
	     edje_object_message_signal_process(o);
	     evas_object_show(o);
	  }
     }
   else if (!enabled)
     {
	for (l = ib->boxes; l; l = l->next)
	  {
	     ibb = l->data;
	     if (!ibb->overlay_object) continue;
	     evas_object_del(ibb->overlay_object);
	     ibb->overlay_object = NULL;
	  }
     }
}

void
_ibox_box_cb_config_updated(void *data)
{
   /* Call Any Needed Funcs To Let Module Handle Config Changes */
   _ibox_box_cb_follower(data);
   _ibox_box_cb_width_auto(data);
   _ibox_box_cb_iconsize_change(data);
}
