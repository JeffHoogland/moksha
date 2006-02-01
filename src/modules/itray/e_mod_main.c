/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"
#include <math.h>

#include "Ecore_X.h"
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

/* Currently unused */
#define SYSTEM_TRAY_REQUEST_DOCK    0
#define SYSTEM_TRAY_BEGIN_MESSAGE   1
#define SYSTEM_TRAY_CANCEL_MESSAGE  2
#define _NET_SYSTEM_TRAY_ORIENTATION_HORZ 0
#define _NET_SYSTEM_TRAY_ORIENTATION_VERT 1
/* ---------------- */

#define XEMBED_EMBEDDED_NOTIFY      0

#define TRAY_ICON_SIZE		    27

/* Bug List:
 *
 * Changing the number of rows breaks the layout.
 *	It's magically fixed when the module is moved.
 *
 * TODO List:
 * 
 * Make sure only one tray exists.
 * Use the abovestated defines.
 * Change the size of the tray icons.
 * Use the shape extension for the ecore_x_window so that the "gray"
 *	itself is not visible, but the tray icons are.
 *
 * The Big Picture:
 *
 * Get rid of the ecore_x_window stuff. 
 * Make the tray survive an E restart.
 *
 */

static int box_count;
static E_Config_DD *conf_edd;
static E_Config_DD *conf_box_edd;

/* const strings */
static const char *_itray_main_orientation[] =
{"left", "right", "top", "bottom"};

/* module private routines */
static ITray   *_itray_new();
static void    _itray_free(ITray *it);
static void    _itray_config_menu_new(ITray *it);

static ITray_Box *_itray_box_new(ITray *it, E_Container *con);
static void    _itray_box_free(ITray_Box *itb);
static void    _itray_box_menu_new(ITray_Box *itb);
static void    _itray_box_disable(ITray_Box *itb);
static void    _itray_box_frame_resize(ITray_Box *itb);
static void    _itray_box_edge_change(ITray_Box *itb, int edge);
static void    _itray_box_update_policy(ITray_Box *itb);

static void    _itray_box_cb_gmc_change(void *data, E_Gadman_Client *gmc, E_Gadman_Change change);
static void    _itray_box_cb_mouse_in(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void    _itray_box_cb_mouse_out(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void    _itray_box_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void    _itray_box_cb_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void    _itray_box_cb_mouse_move(void *data, Evas *e, Evas_Object *obj, void *event_info);

static void    _itray_box_cb_menu_configure(void *data, E_Menu *m, E_Menu_Item *mi);
static void    _itray_box_cb_menu_edit(void *data, E_Menu *m, E_Menu_Item *mi);

/* Config Updated Function Protos */
static void    _itray_box_cb_width_auto(void *data);

/* Tray */
static void    _itray_tray_init(ITray_Box *itb);
static void    _itray_tray_shutdown(ITray_Box *itb);
static int     _itray_tray_cb_msg(void *data, int type, void *event);
static int     _itray_tray_cb_destroy(void *data, int type, void *event);
static int     _itray_tray_cb_hide(void *data, int type, void *event);
static int     _itray_tray_cb_shape(void *data, int type, void *event);
static int     _itray_exists(ITray_Box *itb, Ecore_X_Window win);
static void    _itray_reshape(ITray_Box *itb);
static void    _itray_tray_active_set();

static void    _itray_tray_add(ITray_Box *itb, Ecore_X_Window win);
static void    _itray_tray_remove(ITray_Box *itb, Ecore_X_Window win);

static void    _itray_tray_cb_move(void *data, Evas_Object *o, Evas_Coord x, Evas_Coord y);
static void    _itray_tray_cb_resize(void *data, Evas_Object *o, Evas_Coord w, Evas_Coord h);

static void    _itray_tray_layout(ITray_Box *itb);

/* public module routines. all modules must have these */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
     "ITray"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   ITray *it;

   /* actually init ibox */
   it = _itray_new();
   m->config_menu = it->config_menu;
   return it;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   ITray *it;

   if (m->config_menu)
     m->config_menu = NULL;

   it = m->data;
   if (it) 
     {
	if (it->config_dialog) 
	  {
	     e_object_del(E_OBJECT(it->config_dialog));
	     it->config_dialog = NULL;
	  }
	_itray_free(it);	
     }  
   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   ITray *it;

   it = m->data;
   e_config_domain_save("module.itray", conf_edd, it->conf);
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
   e_module_dialog_show(_("Enlightenment ITray Module"),
			_("This is the ITray system tray module for Enlightenment.<br>"
			  "It will hold system tray icons."));
   return 1;
}

EAPI int
e_modapi_config(E_Module *m)
{
   ITray *e;
   Evas_List *l;
   
   e = m->data;
   if (!e) return 0;
   if (!e->boxes) return 0;
   
   for (l = e->boxes; l; l = l->next) 
     {
	ITray_Box *face;
	face = l->data;
	if (!face) return 0;
	if (face->con == e_container_current_get(e_manager_current_get())) 
	  {
	     _config_itray_module(face->con, face->itray);
	     break;
	  }	
     }
   return 1;
}

/* module private routines */
static ITray *
_itray_new()
{
   ITray *it;
   Evas_List *managers, *l, *l2, *cl;

   box_count = 0;
   it = E_NEW(ITray, 1);
   if (!it) return NULL;

   conf_box_edd = E_CONFIG_DD_NEW("ITray_Config_Box", Config_Box);
#undef T
#undef D
#define T Config_Box
#define D conf_box_edd
   E_CONFIG_VAL(D, T, enabled, UCHAR);

   conf_edd = E_CONFIG_DD_NEW("ITray_Config", Config);
#undef T
#undef D
#define T Config
#define D conf_edd
   E_CONFIG_VAL(D, T, rowsize, INT);
   E_CONFIG_VAL(D, T, width, INT);
   E_CONFIG_LIST(D, T, boxes, conf_box_edd);

   it->conf = e_config_domain_load("module.itray", conf_edd);
   if (!it->conf)
     {
	it->conf = E_NEW(Config, 1);
	it->conf->rowsize = 1;
	it->conf->width = ITRAY_WIDTH_AUTO;
     }
   E_CONFIG_LIMIT(it->conf->rowsize, 1, 6);
   E_CONFIG_LIMIT(it->conf->width, -2, -1);

   _itray_config_menu_new(it);

   managers = e_manager_list();
   cl = it->conf->boxes;
   for (l = managers; l; l = l->next)
     {
	E_Manager *man;

	man = l->data;
	for (l2 = man->containers; l2; l2 = l2->next)
	  {
	     E_Container *con;
	     ITray_Box *itb;
	     /* Config */
	     con = l2->data;
	     itb = _itray_box_new(it, con);
	     if (itb)
	       {
		  E_Menu_Item *mi;

		  if (!cl)
		    {
		       itb->conf = E_NEW(Config_Box, 1);
		       itb->conf->enabled = 1;
		       it->conf->boxes = evas_list_append(it->conf->boxes, itb->conf);
		    }
		  else
		    {
		       itb->conf = cl->data;
		       cl = cl->next;
		    }

		  /* Menu */
		  _itray_box_menu_new(itb);

		  /* Add main menu to box menu */
		  mi = e_menu_item_new(it->config_menu);
		  e_menu_item_label_set(mi, con->name);
		  e_menu_item_submenu_set(mi, itb->menu);

		  /* Setup */
		  if (!itb->conf->enabled)
		    _itray_box_disable(itb);
	       }
	  }
     }
   return it;
}

static void
_itray_free(ITray *it)
{
   E_CONFIG_DD_FREE(conf_edd);
   E_CONFIG_DD_FREE(conf_box_edd);

   while (it->boxes)
     _itray_box_free(it->boxes->data);

   e_object_del(E_OBJECT(it->config_menu));
   evas_list_free(it->conf->boxes);
   free(it->conf);
   free(it);
}

static ITray_Box *
_itray_box_new(ITray *it, E_Container *con)
{
   ITray_Box *itb;
   Evas_Object *o;
   E_Gadman_Policy policy;
   Evas_Coord x, y, w, h;

   itb = E_NEW(ITray_Box, 1);
   if (!itb) return NULL;
   itb->itray = it;
   it->boxes = evas_list_append(it->boxes, itb);

   itb->con = con;
   e_object_ref(E_OBJECT(con));
   itb->evas = con->bg_evas;

   itb->tray = NULL;

   itb->x = itb->y = itb->w = itb->h = -1;

   evas_event_freeze(itb->evas);
   o = edje_object_add(itb->evas);
   itb->box_object = o;
   e_theme_edje_object_set(o, "base/theme/modules/itray",
			   "modules/itray/main");
   evas_object_show(o);

   o = evas_object_rectangle_add(itb->evas);
   itb->event_object = o;
   evas_object_layer_set(o, 2);
   evas_object_repeat_events_set(o, 1);
   evas_object_color_set(o, 255, 255, 255, 255);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_IN,  _itray_box_cb_mouse_in,  itb);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_OUT, _itray_box_cb_mouse_out, itb);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _itray_box_cb_mouse_down, itb);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_UP, _itray_box_cb_mouse_up, itb);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_MOVE, _itray_box_cb_mouse_move, itb);
   evas_object_show(o);

   o = e_box_add(itb->evas);
   itb->item_object = o;
   _itray_tray_init(itb);
   e_box_freeze(o);
   edje_object_part_swallow(itb->box_object, "tray", o);
   evas_object_show(o);

   e_box_align_set(itb->item_object, 0.5, 0.5);

   evas_object_resize(itb->box_object, 1000, 1000);
   edje_object_calc_force(itb->box_object);
   edje_object_part_geometry_get(itb->box_object, "tray", &x, &y, &w, &h);
   itb->box_inset.l = x;
   itb->box_inset.r = 1000 - (x + w);
   itb->box_inset.t = y;
   itb->box_inset.b = 1000 - (y + h);

   e_box_thaw(itb->item_object);

   itb->gmc = e_gadman_client_new(itb->con->gadman);
   e_gadman_client_domain_set(itb->gmc, "module.itray", box_count++);
   policy = E_GADMAN_POLICY_EDGES | E_GADMAN_POLICY_HMOVE | E_GADMAN_POLICY_VMOVE;
   if (itb->itray->conf->width == ITRAY_WIDTH_FIXED)
     policy |= E_GADMAN_POLICY_VSIZE;
   e_gadman_client_policy_set(itb->gmc, policy);
   e_gadman_client_min_size_set(itb->gmc, 8, 8);
   e_gadman_client_max_size_set(itb->gmc, 3200, 3200);
   e_gadman_client_auto_size_set(itb->gmc, -1, -1);
   e_gadman_client_align_set(itb->gmc, 0.0, 0.5);
   e_gadman_client_resize(itb->gmc, 400, 32);
   e_gadman_client_change_func_set(itb->gmc, _itray_box_cb_gmc_change, itb);
   e_gadman_client_edge_set(itb->gmc, E_GADMAN_EDGE_LEFT);
   e_gadman_client_load(itb->gmc);

   evas_event_thaw(itb->evas);

   /* We need to resize, if the width is auto and the number
    * of apps has changed since last startup */
   _itray_box_frame_resize(itb);
   return itb;
}

static void
_itray_box_free(ITray_Box *itb)
{
   e_object_unref(E_OBJECT(itb->con));
   e_object_del(E_OBJECT(itb->menu));

   evas_object_del(itb->box_object);
   evas_object_del(itb->item_object);
   evas_object_del(itb->event_object);

   if (itb->tray)
     {
	_itray_tray_shutdown(itb);
	itb->tray = NULL;
     }

   e_gadman_client_save(itb->gmc);
   e_object_del(E_OBJECT(itb->gmc));

   itb->itray->boxes = evas_list_remove(itb->itray->boxes, itb);

   free(itb->conf);
   free(itb);
   box_count--;
}

static void
_itray_box_menu_new(ITray_Box *itb)
{
   E_Menu *mn;
   E_Menu_Item *mi;

   mn = e_menu_new();
   itb->menu = mn;

   /* Config */
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Configuration"));
   e_menu_item_callback_set(mi, _itray_box_cb_menu_configure, itb);

   /* Edit */
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Edit Mode"));
   e_menu_item_callback_set(mi, _itray_box_cb_menu_edit, itb);
}

static void
_itray_box_cb_menu_configure(void *data, E_Menu *m, E_Menu_Item *mi)
{
   ITray_Box *itb;

   itb = (ITray_Box *)data;
   if (!itb) return;
   _config_itray_module(itb->con, itb->itray);
}

static void
_itray_box_disable(ITray_Box *itb)
{
   itb->conf->enabled = 0;
   evas_object_hide(itb->box_object);
   evas_object_hide(itb->item_object);
   evas_object_hide(itb->event_object);
   e_config_save_queue();
}

void
_itray_config_menu_new(ITray *it)
{
   E_Menu *mn;

   mn = e_menu_new();
   it->config_menu = mn;
}

static void
_itray_box_frame_resize(ITray_Box *itb)
{
   Evas_Coord w, h, bw, bh;
   int icons_per_row;
   /* Not finished loading config yet! */
   if ((itb->x == -1) ||
       (itb->y == -1) ||
       (itb->w == -1) ||
       (itb->h == -1))
     return;

   evas_event_freeze(itb->evas);
   e_box_freeze(itb->item_object);

   if (e_box_pack_count_get(itb->item_object))
     e_box_min_size_get(itb->item_object, &w, &h);
   else
     {
	if (!itb->tray)
	  w = h = TRAY_ICON_SIZE + itb->box_inset.l + itb->box_inset.r;
	else if (!itb->tray->icons)
	  w = h = TRAY_ICON_SIZE + itb->box_inset.l + itb->box_inset.r;
	else 
	  {
	     icons_per_row = (itb->tray->icons + 
		   (itb->tray->icons % itb->tray->rows)) / itb->tray->rows;
	     w = (icons_per_row * TRAY_ICON_SIZE)
		+ itb->box_inset.l + itb->box_inset.r;
	     h = (itb->tray->rows * TRAY_ICON_SIZE)
		+ itb->box_inset.t + itb->box_inset.b;
	  }
     }
   edje_object_part_unswallow(itb->box_object, itb->item_object);
/*   edje_extern_object_min_size_set(itb->item_object, w, h);*/
/*   edje_object_part_swallow(itb->box_object, "tray", itb->item_object);*/
/*   edje_object_size_min_calc(itb->box_object, &bw, &bh);*/
   edje_extern_object_min_size_set(itb->item_object, 0, 0);
   edje_object_part_swallow(itb->box_object, "tray", itb->item_object);

   e_box_thaw(itb->item_object);
   evas_event_thaw(itb->evas);

   if (itb->itray->conf->width == ITRAY_WIDTH_AUTO)
     {
	if ((e_gadman_client_edge_get(itb->gmc) == E_GADMAN_EDGE_LEFT) ||
	    (e_gadman_client_edge_get(itb->gmc) == E_GADMAN_EDGE_RIGHT))
	  {
	     e_gadman_client_resize(itb->gmc, h, w);
	  }
	else if ((e_gadman_client_edge_get(itb->gmc) == E_GADMAN_EDGE_TOP) ||
	         (e_gadman_client_edge_get(itb->gmc) == E_GADMAN_EDGE_BOTTOM))
	  {
	     e_gadman_client_resize(itb->gmc, w, h);
	  }
     }
   else
     {
	if ((e_gadman_client_edge_get(itb->gmc) == E_GADMAN_EDGE_LEFT) ||
	    (e_gadman_client_edge_get(itb->gmc) == E_GADMAN_EDGE_RIGHT))
	  {
	     /* h is the width of the box */
	     e_gadman_client_resize(itb->gmc, bw, itb->h);
	  }
	else if ((e_gadman_client_edge_get(itb->gmc) == E_GADMAN_EDGE_TOP) ||
	         (e_gadman_client_edge_get(itb->gmc) == E_GADMAN_EDGE_BOTTOM))
	  {
	     /* w is the width of the box */
	     e_gadman_client_resize(itb->gmc, itb->w, bh);
	  }
     }
}

static void
_itray_box_edge_change(ITray_Box *itb, int edge)
{
   Evas_Coord bw, bh, tmp;
   Evas_Object *o;
   E_Gadman_Policy policy;
   int changed;

   evas_event_freeze(itb->evas);
   o = itb->box_object;
   edje_object_signal_emit(o, "set_orientation", _itray_main_orientation[edge]);
   edje_object_message_signal_process(o);

   e_box_freeze(itb->item_object);
   e_box_align_set(itb->item_object, 0.5, 0.5);

   policy = E_GADMAN_POLICY_EDGES | E_GADMAN_POLICY_HMOVE | E_GADMAN_POLICY_VMOVE;
   if ((edge == E_GADMAN_EDGE_BOTTOM) ||
       (edge == E_GADMAN_EDGE_TOP))
     {
	changed = (e_box_orientation_get(itb->item_object) != 1);
	if (changed)
	  {
	     e_box_orientation_set(itb->item_object, 1);
	     if (itb->itray->conf->width == ITRAY_WIDTH_FIXED)
	       policy |= E_GADMAN_POLICY_HSIZE;
	     e_gadman_client_policy_set(itb->gmc, policy);
	     tmp = itb->w;
	     itb->w = itb->h;
	     itb->h = tmp;
	  }
     }
   else if ((edge == E_GADMAN_EDGE_LEFT) ||
	    (edge == E_GADMAN_EDGE_RIGHT))
     {
	changed = (e_box_orientation_get(itb->item_object) != 0);
	if (changed)
	  {
	     e_box_orientation_set(itb->item_object, 0);
	     if (itb->itray->conf->width == ITRAY_WIDTH_FIXED)
	       policy |= E_GADMAN_POLICY_VSIZE;
	     e_gadman_client_policy_set(itb->gmc, policy);
	     tmp = itb->w;
	     itb->w = itb->h;
	     itb->h = tmp;
	  }
     }

   e_box_thaw(itb->item_object);
   evas_event_thaw(itb->evas);

   _itray_box_frame_resize(itb);
}

static void
_itray_box_update_policy(ITray_Box *itb)
{
   E_Gadman_Policy policy;

   policy = E_GADMAN_POLICY_EDGES | E_GADMAN_POLICY_HMOVE | E_GADMAN_POLICY_VMOVE;
   if ((e_gadman_client_edge_get(itb->gmc) == E_GADMAN_EDGE_BOTTOM) ||
       (e_gadman_client_edge_get(itb->gmc) == E_GADMAN_EDGE_TOP))
     {
	if (itb->itray->conf->width == ITRAY_WIDTH_FIXED)
	  policy |= E_GADMAN_POLICY_HSIZE;
	e_gadman_client_policy_set(itb->gmc, policy);
     }
   else if ((e_gadman_client_edge_get(itb->gmc) == E_GADMAN_EDGE_LEFT) ||
	    (e_gadman_client_edge_get(itb->gmc) == E_GADMAN_EDGE_RIGHT))
     {
	if (itb->itray->conf->width == ITRAY_WIDTH_FIXED)
	  policy |= E_GADMAN_POLICY_VSIZE;
	e_gadman_client_policy_set(itb->gmc, policy);
     }
}

static void
_itray_box_cb_mouse_in(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_In *ev;
   ITray_Box *itb;

   ev = event_info;
   itb = data;
}

static void
_itray_box_cb_mouse_out(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Out *ev;
   ITray_Box *itb;

   ev = event_info;
   itb = data;
}

static void
_itray_box_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   ITray_Box *itb;

   ev = event_info;
   itb = data;
/*   
   if (ev->button == 3)
     {
	e_menu_activate_mouse(itb->menu, e_zone_current_get(itb->con),
			      ev->output.x, ev->output.y, 1, 1,
			      E_MENU_POP_DIRECTION_DOWN, ev->timestamp);
	e_util_container_fake_mouse_up_later(itb->con, 3);
     }
   else
 */
     {
	Ecore_X_Window win;
	int x, y, w, h, xx, yy;
	Evas_List *l;

	for (l = itb->tray->wins; l; l = l->next)
	  {
	     win = (Ecore_X_Window)l->data;
	     
	     evas_pointer_output_xy_get(itb->evas, &xx, &yy);
	     evas_object_geometry_get(itb->item_object, &x, &y, NULL, NULL);
	     xx -= x;
	     yy -= y;
	     ecore_x_window_geometry_get(win, &x, &y, &w, &h);
	     if (E_CONTAINS(x, y, w, h, xx, yy, 1, 1))
	       {
		  x = xx - x;
		  y = yy - y;
		  win = ecore_x_window_at_xy_begin_get(win, x, y);
		  ecore_x_pointer_ungrab();
		  ecore_x_flush();
		  ecore_x_mouse_down_send(win, x, y, ev->button);
		  e_util_container_fake_mouse_up_later(itb->con, ev->button);
		  break;
	       }
	  }
     }
}

static void
_itray_box_cb_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Up *ev;
   ITray_Box *itb;

   ev = event_info;
   itb = data;
/*   
   if (ev->button == 3)
     {
     }
   else
 */
     {
	Ecore_X_Window win;
	int x, y, w, h, xx, yy;
	Evas_List *l;
	
	for (l = itb->tray->wins; l; l = l->next)
	  {
	     win = (Ecore_X_Window)l->data;
	     
	     evas_pointer_output_xy_get(itb->evas, &xx, &yy);
	     evas_object_geometry_get(itb->item_object, &x, &y, NULL, NULL);
	     xx -= x;
	     yy -= y;
	     ecore_x_window_geometry_get(win, &x, &y, &w, &h);
	     if (E_CONTAINS(x, y, w, h, xx, yy, 1, 1))
	       {
		  x = xx - x;
		  y = yy - y;
		  win = ecore_x_window_at_xy_begin_get(win, x, y);
		  ecore_x_mouse_up_send(win, x, y, ev->button);
		  break;
	       }
	  }
     }
}

static void
_itray_box_cb_mouse_move(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Move *ev;
   ITray_Box *itb;
   
   ev = event_info;
   itb = data;
     {
	Ecore_X_Window win;
	int x, y, w, h, xx, yy;
	Evas_List *l;
	
	for (l = itb->tray->wins; l; l = l->next)
	  {
	     win = (Ecore_X_Window)l->data;
	     
	     evas_pointer_output_xy_get(itb->evas, &xx, &yy);
	     evas_object_geometry_get(itb->item_object, &x, &y, NULL, NULL);
	     xx -= x;
	     yy -= y;
	     ecore_x_window_geometry_get(win, &x, &y, &w, &h);
	     if (E_CONTAINS(x, y, w, h, xx, yy, 1, 1))
	       {
		  x = xx - x;
		  y = yy - y;
		  win = ecore_x_window_at_xy_begin_get(win, x, y);
		  ecore_x_mouse_move_send(win, x, y);
		  break;
	       }
	  }
     }
}

static void
_itray_box_cb_gmc_change(void *data, E_Gadman_Client *gmc, E_Gadman_Change change)
{
   ITray_Box *itb;

   itb = data;
   switch (change)
     {
      case E_GADMAN_CHANGE_MOVE_RESIZE:
	e_gadman_client_geometry_get(itb->gmc, &itb->x, &itb->y, &itb->w, &itb->h);

	edje_extern_object_min_size_set(itb->item_object, 0, 0);
	edje_object_part_swallow(itb->box_object, "tray", itb->item_object);

	evas_object_move(itb->box_object, itb->x, itb->y);
	evas_object_resize(itb->box_object, itb->w, itb->h);
	break;
      case E_GADMAN_CHANGE_EDGE:
	_itray_box_edge_change(itb, e_gadman_client_edge_get(itb->gmc));
	break;
      case E_GADMAN_CHANGE_RAISE:
      case E_GADMAN_CHANGE_ZONE:
	 /* Ignore */
	break;
     }
}

static void
_itray_box_cb_width_auto(void *data)
{
   ITray          *it;
   ITray_Box      *itb;
   Evas_List     *l;

   it = (ITray *)data;
   for (l = it->boxes; l; l = l->next)
     {
	itb = l->data;
	_itray_box_update_policy(itb);
	_itray_box_frame_resize(itb);
     }
}

static void
_itray_box_cb_menu_edit(void *data, E_Menu *m, E_Menu_Item *mi)
{
   ITray_Box *itb;

   itb = data;
   e_gadman_mode_set(itb->gmc->gadman, E_GADMAN_MODE_EDIT);
}

void
_itray_box_cb_config_updated(void *data)
{
   /* Call Any Needed Funcs To Let Module Handle Config Changes */
   _itray_box_cb_width_auto(data);
}

void
_itray_tray_init(ITray_Box *itb)
{
   Evas_Coord x, y, w, h;
   Ecore_X_Window *windows;
   int wnum;

   /* FIXME - temp */
   itb->tray = malloc(sizeof(ITray_Tray));
   itb->tray->icons = 0;
   itb->tray->w = 1;
   itb->tray->h = 1;
   itb->tray->wins = NULL;

   evas_object_resize(itb->item_object, itb->tray->w, itb->tray->h);
   evas_object_color_set(itb->item_object, 0, 0, 0, 0);
   evas_object_intercept_move_callback_add(itb->item_object, _itray_tray_cb_move, itb);
   evas_object_intercept_resize_callback_add(itb->item_object, _itray_tray_cb_resize, itb);

   _itray_tray_active_set(itb, 1);

   evas_object_geometry_get(itb->item_object, &x, &y, &w, &h);
   itb->tray->win = ecore_x_window_new(itb->con->bg_win, x, y, w, h);
   ecore_x_window_container_manage(itb->tray->win);
   /* this sucks because some icons inherit the bg pixmap/color of the parent win and thus the color... and are not shaped */
   ecore_x_window_background_color_set(itb->tray->win, 0xffff, 0xffff, 0xffff);

   itb->tray->msg_handler = ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE, _itray_tray_cb_msg, itb);
   itb->tray->dst_handler = ecore_event_handler_add(ECORE_X_EVENT_WINDOW_DESTROY, _itray_tray_cb_destroy, itb);

   windows = ecore_x_window_children_get(itb->con->manager->root, &wnum);
   if (windows)
     {
        int i;
	Ecore_X_Atom atom_xmbed, atom_kde_netwm_systray, atom_kwm_dockwindow,
	  atom_window;
               
	atom_window = ecore_x_atom_get("WINDOW");
	atom_xmbed = ecore_x_atom_get("_XEMBED_INFO");
	atom_kde_netwm_systray = ecore_x_atom_get("_KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR");
	atom_kwm_dockwindow = ecore_x_atom_get("KWM_DOCKWINDOW");
	for (i = 0; i < wnum; i++)
	  {
             Ecore_X_Window_Attributes att;
	     
             if (ecore_x_window_attributes_get(windows[i], &att))
	       {
//		  if (0)
//		  if (att.visible)
		    {
		       unsigned char *data = NULL;
		       int count;
		       
		       if (!ecore_x_window_prop_property_get(windows[i],
							     atom_xmbed,
							     atom_xmbed, 32, 
							     &data, &count))
			 data = NULL;
		       if (!data)
			 {
			    if (!ecore_x_window_prop_property_get(windows[i],
								  atom_kde_netwm_systray,
								  atom_xmbed, 32, 
								  &data, &count))
			      data = NULL;
			 }
		       if (!data)
			 {
			    if (!ecore_x_window_prop_property_get(windows[i],
								  atom_kwm_dockwindow,
								  atom_kwm_dockwindow, 32, 
								  &data, &count))
			      data = NULL;
			 }
		       if (data)
			 {
			    printf("found %x\n", windows[i]);
			    _itray_tray_add(itb, windows[i]);
			    free(data);
			    data = NULL;
			 }
		    }
	       }
	  }
        free(windows);
     }
   _itray_reshape(itb);
}

void
_itray_tray_shutdown(ITray_Box *itb)
{
   _itray_tray_active_set(itb, 0);

   while (itb->tray->wins)
     {
	Ecore_X_Window win;
	Ecore_X_Window_Attributes att;
	
	win = (Ecore_X_Window)(itb->tray->wins->data);
	ecore_x_window_hide(win);
	ecore_x_window_reparent(win, itb->con->manager->root, 0, 0);
	itb->tray->wins = evas_list_remove_list(itb->tray->wins, itb->tray->wins);
     }
   evas_object_del(itb->item_object);
   
   ecore_event_handler_del(itb->tray->msg_handler);
   ecore_event_handler_del(itb->tray->dst_handler);
   ecore_x_window_del(itb->tray->win);
}

static void
_itray_tray_active_set(ITray_Box *itb, int active)
{
   Ecore_X_Window win;
   Display *display;
   char buf[32];
   Ecore_X_Atom selection_atom;

   win = 0;
   if (active)
     win = itb->con->bg_win;

   display = ecore_x_display_get();
   snprintf(buf, sizeof(buf), "_NET_SYSTEM_TRAY_S%d", itb->con->manager->num);
   selection_atom = ecore_x_atom_get(buf);
   /* FIXXME: nasty xlib snippets! */
   XSetSelectionOwner(display, selection_atom, win, CurrentTime);
   if ((active) &&
       (XGetSelectionOwner(display, selection_atom) == itb->con->bg_win))
     {
	printf("send message thast we own the tray now\n");
	ecore_x_client_message32_send(itb->con->manager->root,
				      ecore_x_atom_get("MANAGER"),
				      ECORE_X_EVENT_MASK_WINDOW_CONFIGURE,
				      CurrentTime, selection_atom, win, 0, 0);
     }
}

static void
_itray_tray_add(ITray_Box *itb, Ecore_X_Window win) 
{
   if (!win) return;
   if (evas_list_find(itb->tray->wins, (void *)win))
     return;
   e_stolen_win_add(win);
   ecore_x_window_show(itb->tray->win);
   evas_object_show(itb->item_object);

   /* FIXME: adding a window id AS a pointer - BAD! */
   /* FIXME: need to listen for shape change events on icons */
   /* FIXME: on shape change need to inherit shape */
   
   /* properties u might find on sample windows:
    * 
    * GDK_TIMESTAMP_PROP(GDK_TIMESTAMP_PROP) = 0x61
    * _NET_WM_SYNC_REQUEST_COUNTER(CARDINAL) = 29360209
    * _XEMBED_INFO(_XEMBED_INFO) = 0x1, 0x1
    * WM_CLIENT_LEADER(WINDOW): window id # 0x1c00001
    * _NET_WM_PID(CARDINAL) = 17802
    * WM_LOCALE_NAME(STRING) = "ja_JP.UTF-8"
    * WM_CLIENT_MACHINE(STRING) = "icky"
    * WM_NORMAL_HINTS(WM_SIZE_HINTS):
    *                 program specified minimum size: 24 by 24
    *                 window gravity: NorthWest
    * WM_PROTOCOLS(ATOM): protocols  WM_DELETE_WINDOW, WM_TAKE_FOCUS, _NET_WM_PING, _NET_WM_SYNC_REQUEST
    * WM_CLASS(STRING) = "gaim", "Gaim"
    * WM_ICON_NAME(STRING) = "Gaim"
    * _NET_WM_ICON_NAME(UTF8_STRING) = 0x47, 0x61, 0x69, 0x6d
    * WM_NAME(STRING) = "Gaim"
    * _NET_WM_NAME(UTF8_STRING) = 0x47, 0x61, 0x69, 0x6d
    *---
    * GDK_TIMESTAMP_PROP(GDK_TIMESTAMP_PROP) = 0x61
    * _NET_WM_SYNC_REQUEST_COUNTER(CARDINAL) = 29360172
    * _XEMBED_INFO(_XEMBED_INFO) = 0x1, 0x1
    * WM_CLIENT_LEADER(WINDOW): window id # 0x1c00001
    * _NET_WM_PID(CARDINAL) = 17841
    * WM_LOCALE_NAME(STRING) = "ja_JP.UTF-8"
    * WM_CLIENT_MACHINE(STRING) = "icky"
    * WM_NORMAL_HINTS(WM_SIZE_HINTS):
    *                 program specified minimum size: 16 by 16
    *                 window gravity: NorthWest
    * WM_PROTOCOLS(ATOM): protocols  WM_DELETE_WINDOW, WM_TAKE_FOCUS, _NET_WM_PING, _NET_WM_SYNC_REQUEST
    * WM_CLASS(STRING) = "gnomemeeting", "Gnomemeeting"
    * WM_ICON_NAME(STRING) = "GnomeMeeting Tray Icon"
    * _NET_WM_ICON_NAME(UTF8_STRING) = 0x47, 0x6e, 0x6f, 0x6d, 0x65, 0x4d, 0x65, 0x65, 0x74, 0x69, 0x6e, 0x67, 0x20, 0x54, 0x72, 0x61, 0x79, 0x20, 0x49, 0x63, 0x6f, 0x6e
    * WM_NAME(STRING) = "GnomeMeeting Tray Icon"
    * _NET_WM_NAME(UTF8_STRING) = 0x47, 0x6e, 0x6f, 0x6d, 0x65, 0x4d, 0x65, 0x65, 0x74, 0x69, 0x6e, 0x67, 0x20, 0x54, 0x72, 0x61, 0x79, 0x20, 0x49, 0x63, 0x6f, 0x6e
    *---
    * _KDE_NET_WM_SYSTEM_TRAY_WINDOW_FOR(WINDOW): window id # 0x0
    * KWM_DOCKWINDOW(KWM_DOCKWINDOW) = 0x0
    * WM_CLIENT_LEADER(WINDOW): window id # 0x1c00006
    * WM_WINDOW_ROLE(STRING) = "skypedock"
    * _NET_WM_PID(CARDINAL) = 18018
    * _NET_WM_WINDOW_TYPE(ATOM) = _NET_WM_WINDOW_TYPE_NORMAL
    * WM_PROTOCOLS(ATOM): protocols  WM_DELETE_WINDOW, WM_TAKE_FOCUS, _NET_WM_PING
    * WM_NAME(STRING) = "skype"
    * WM_LOCALE_NAME(STRING) = "ja_JP.UTF-8"
    * WM_CLASS(STRING) = "skypedock", "Skype"
    * WM_HINTS(WM_HINTS):
    *                 Initial state is Normal State.
    *                 bitmap id # to use for icon: 0x1c00025
    *                 bitmap id # of mask for icon: 0x1c00026
    *                 window id # to use for icon: 0x1c000b1
    *                 window id # of group leader: 0x1c000b1
    * WM_NORMAL_HINTS(WM_SIZE_HINTS):
    *                 user specified size: 22 by 22
    *                 program specified size: 22 by 22
    *                 program specified minimum size: 22 by 22
    *                 program specified maximum size: 22 by 22
    *                 window gravity: NorthWest
    * WM_CLIENT_MACHINE(STRING) = "icky"
    * 
    */
   /* we want to insert at the end, so as not to move all icons on each add */
   itb->tray->wins = evas_list_append(itb->tray->wins, (void *)win);
   itb->tray->icons++;
   ecore_x_window_client_manage(win);
   ecore_x_window_resize(win, TRAY_ICON_SIZE, TRAY_ICON_SIZE);
   
   ecore_x_window_save_set_add(win);
   ecore_x_window_shape_events_select(win, 1);
   ecore_x_window_reparent(win, itb->tray->win, 0, 0);
   ecore_x_window_raise(itb->con->event_win);
   _itray_tray_layout(itb);
   _itray_box_frame_resize(itb);

   ecore_x_window_show(win);

}

static void
_itray_tray_remove(ITray_Box *itb, Ecore_X_Window win)
{

   if (!win)
     return;
   if (!evas_list_find(itb->tray->wins, (void *)win)) /* if was not found */
     return;
   e_stolen_win_del(win);
   itb->tray->wins = evas_list_remove(itb->tray->wins, (void *)win);
   itb->tray->icons--;
   _itray_tray_layout(itb);
   _itray_box_frame_resize(itb);
   if (itb->tray->icons == 0) 
     {
	ecore_x_window_hide(itb->tray->win);
	evas_object_show(itb->item_object);
     }
}

static int
_itray_tray_cb_msg(void *data, int type, void *event)
{
   Ecore_X_Event_Client_Message *ev;
   ITray_Box *itb;
   static Ecore_X_Atom atom_opcode = 0;
   static Ecore_X_Atom atom_message = 0;
   static Ecore_X_Atom atom_xmbed = 0;

   itb = data;
   ev = event;
   if (atom_opcode == 0)
     atom_opcode = ecore_x_atom_get("_NET_SYSTEM_TRAY_OPCODE");
   if (atom_message == 0)
     atom_message =  ecore_x_atom_get("_NET_SYSTEM_TRAY_MESSAGE_DATA");
   if (atom_xmbed == 0)
     atom_xmbed = ecore_x_atom_get("_XEMBED");
   if (ev->message_type == atom_opcode)
     {
	if (ev->data.l[1] == SYSTEM_TRAY_REQUEST_DOCK)
	  {
	     _itray_tray_add(itb, (Ecore_X_Window)ev->data.l[2]);
	     /* Should proto be set according to clients _XEMBED_INFO? */
	     ecore_x_client_message32_send(ev->data.l[2], 
					   atom_xmbed,
					   ECORE_X_EVENT_MASK_NONE, CurrentTime,
					   XEMBED_EMBEDDED_NOTIFY, 0, itb->con->bg_win, /*proto*/1);
	     
	     _itray_reshape(itb);
	  }
	else if (ev->data.l[1] == SYSTEM_TRAY_BEGIN_MESSAGE)
	  {
	     printf("begin message\n");
	  }
	else if (ev->data.l[1] == SYSTEM_TRAY_CANCEL_MESSAGE)
	  {
	     printf("cancel message\n");
	  }
     }
   else if (ev->message_type == atom_message)
     {
	printf("got message\n");
     } 
   return 1;

}

static int
_itray_tray_cb_destroy(void *data, int type, void *event)
{
   Ecore_X_Event_Window_Destroy *ev;
   ITray_Box *itb;
   int exists;
   
   itb = data;
   ev = event;
   exists = _itray_exists(itb, ev->win);
   _itray_tray_remove(itb, ev->win);
   if (exists) _itray_reshape(itb);
   return 1;
}

static int
_itray_tray_cb_hide(void *data, int type, void *event)
{
   Ecore_X_Event_Window_Destroy *ev;
   ITray_Box *itb;
   int exists;
   
   itb = data;
   ev = event;
   exists = _itray_exists(itb, ev->win);
   _itray_tray_remove(itb, ev->win);
   if (exists) _itray_reshape(itb);
   return 1;
}

static int
_itray_tray_cb_shape(void *data, int type, void *event)
{
   Ecore_X_Event_Window_Shape *ev;
   ITray_Box *itb;
   Evas_List *l;
   
   itb = data;
   ev = event;
   if (_itray_exists(itb, ev->win)) _itray_reshape(itb);
   return 1;
}

static int
_itray_exists(ITray_Box *itb, Ecore_X_Window win)
{
   Evas_List *l;
   
   for (l = itb->tray->wins; l; l = l->next)
     {
	Ecore_X_Window w;
	
	w = (Ecore_X_Window)l->data;
	if (w == win) return 1;
     }
   return 0;
}

static void
_itray_reshape(ITray_Box *itb)
{
   Evas_List *l;
   Ecore_X_Window twin;
   int i;

   printf("reshape %p win %x\n", itb, itb->tray->win);
   twin = ecore_x_window_override_new(itb->tray->win, 0, 0, 100, 100);
   for (i = 0, l = itb->tray->wins; l; l = l->next, i++)
     {
	Ecore_X_Window win;
        Ecore_X_Rectangle *rects;
	int num;
	int x, y, w, h;
	
	win = (Ecore_X_Window)l->data;
	printf("SH%i %x\n", i, win);
	ecore_x_window_geometry_get(win, &x, &y, &w, &h);
        rects = ecore_x_window_shape_rectangles_get(win, &num);
	if (rects)
	  {
	     printf("shaped! %i\n", num);
	     if (i == 0)
	       {
		  ecore_x_window_shape_window_set_xy(twin, win, x, y);
	       }
	     else
	       {
		  ecore_x_window_shape_window_add_xy(twin, win, x, y);
	       }
	     free(rects);
	  }
	else
	  {
	     printf("not shaped!\n");
	     if (i == 0)
	       {
		  ecore_x_window_shape_rectangle_set(twin, x, y, w, h);
	       }
	     else
	       {
		  ecore_x_window_shape_rectangle_add(twin, x, y, w, h);
	       }
	  }
	if (i > 1000) break;
     }
   ecore_x_window_shape_window_set(itb->tray->win, twin);
   ecore_x_window_del(twin);
}

static void
_itray_tray_cb_move(void *data, Evas_Object *o, Evas_Coord x, Evas_Coord y)
{
   ITray_Box *itb;

   itb = data;
   evas_object_move(o, x, y);
   evas_object_move(itb->event_object, x, y);
   ecore_x_window_move(itb->tray->win, (int) x, (int) y);
}

static void
_itray_tray_cb_resize(void *data, Evas_Object *o, Evas_Coord w, Evas_Coord h)
{
   ITray_Box *itb;

   itb = data;
   evas_object_resize(o, w, h);
   evas_object_resize(itb->event_object, w, h);

   _itray_tray_layout(itb);
   _itray_box_frame_resize(itb);
}

static void
_itray_tray_layout(ITray_Box *itb)
{
   Evas_Coord w, h, c, d;
   int x, y;
   Evas_List *wins;
   E_Gadman_Edge edge;

   if (!itb->gmc) return;
   edge = e_gadman_client_edge_get(itb->gmc); 
   h = itb->itray->conf->rowsize * TRAY_ICON_SIZE;
   if (h < TRAY_ICON_SIZE)
     h = TRAY_ICON_SIZE;
   c = (h - (h % TRAY_ICON_SIZE)) / TRAY_ICON_SIZE;
   if (itb->tray->icons > 0 && itb->tray->icons < c + 1)
     {
	c = itb->tray->icons;
	h = c * TRAY_ICON_SIZE;
     }
   w = ((itb->tray->icons + (itb->tray->icons % c)) / c) * TRAY_ICON_SIZE;

   if (edge == E_GADMAN_EDGE_BOTTOM || edge == E_GADMAN_EDGE_TOP)
     {
	edje_object_part_unswallow(itb->box_object, itb->item_object);
	evas_object_resize(itb->item_object, w, h);
	ecore_x_window_resize(itb->tray->win, (int) w, (int) h);
	
	edje_extern_object_min_size_set(itb->item_object, w, h);
	edje_extern_object_max_size_set(itb->item_object, w, h);
	edje_object_part_swallow(itb->box_object, "tray", itb->item_object);
     }
   else
     {
	edje_object_part_unswallow(itb->box_object, itb->item_object);
	evas_object_resize(itb->item_object, h, w);
	ecore_x_window_resize(itb->tray->win, (int) h, (int) w);
	
	edje_extern_object_min_size_set(itb->item_object, w, h);
	edje_extern_object_max_size_set(itb->item_object, w, h);
	edje_object_part_swallow(itb->box_object, "tray", itb->item_object);
     }
   
   x = 0;
   if (edge == E_GADMAN_EDGE_BOTTOM || edge == E_GADMAN_EDGE_RIGHT)
     y = h - TRAY_ICON_SIZE;
   else
     y = 0;
   d = 0;
   for (wins = itb->tray->wins; wins; wins = wins->next)
     {
	if (edge == E_GADMAN_EDGE_BOTTOM || edge == E_GADMAN_EDGE_TOP)
	  ecore_x_window_move((Ecore_X_Window) wins->data, x, y);
	else
	  ecore_x_window_move((Ecore_X_Window) wins->data, y, x);
	
	d++;
	if (d % c == 0)
	  {
	     x += TRAY_ICON_SIZE;
	     if (edge == E_GADMAN_EDGE_BOTTOM || edge == E_GADMAN_EDGE_RIGHT)
	       y = h - TRAY_ICON_SIZE;
	     else
	       y = 0;
	  }
	else if (edge == E_GADMAN_EDGE_BOTTOM || edge == E_GADMAN_EDGE_RIGHT)
	  y -= TRAY_ICON_SIZE;
	else
	  y += TRAY_ICON_SIZE;
     }
   itb->tray->rows = c;
}
