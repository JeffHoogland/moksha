#include "e.h"
#include "e_mod_main.h"

/* TODO List:
 * 
 * * icon labels & label tooltips supported for the name of the app
 * * use part list to know how many icons & where to put in the overlay of an icon
 * * description bubbles/tooltips for icons
 * * support dynamic iconsize change on the fly
 * * app subdirs - need to somehow handle these...
 * * use overlay object and repeat events for doing auto hide/show
 * * emit signals on hide/show due to autohide/show
 * * virtualise autoshow/hide to later allow for key bindings, mouse events elsewhere, ipc and other singals to show/hide
 * * save and load config
 * 
 * BONUS Features (maybe do this later):
 * 
 * * allow ibox icons to be dragged around to re-order/delete
 * 
 */

/* const strings */
static const char *_ibox_main_orientation[] =
{"bottom", "top", "left", "right"};

/* module private routines */
static IBox   *_ibox_init(E_Module *m);
static void    _ibox_shutdown(IBox *ib);
static void    _ibox_app_change(void *data, E_App *a, E_App_Change ch);
static E_Menu *_ibox_config_menu_new(IBox *ib);
static void    _ibox_config_menu_del(IBox *ib, E_Menu *m);
static void    _ibox_cb_width_fixed(void *data, E_Menu *m, E_Menu_Item *mi);
static void    _ibox_cb_width_auto(void *data, E_Menu *m, E_Menu_Item *mi);
static void    _ibox_cb_width_fill(void *data, E_Menu *m, E_Menu_Item *mi);
static void    _ibox_box_iconsize_change(IBox_Box *ibb);
static IBox_Icon *_ibox_box_icon_find(IBox_Box *ibb, E_App *a);
static void    _ibox_box_icon_del(IBox_Icon *ic);
static IBox_Icon *_ibox_box_icon_new(IBox_Box *ibb, E_App *a, E_Border *bd);
static void    _ibox_box_icon_resize(IBox_Icon *ic);
static void    _ibox_box_icon_reorder_before(IBox_Icon *ic, IBox_Icon *before);
static void    _ibox_box_icon_reorder_after(IBox_Icon *ic, IBox_Icon *after);
static void    _ibox_box_frame_resize(IBox_Box *ibb);
static void    _ibox_box_init(IBox_Box *ibb);
static void    _ibox_box_free(IBox_Box *ibb);
static void    _ibox_motion_handle(IBox_Box *ibb, Evas_Coord mx, Evas_Coord my);
static void    _ibox_timer_handle(IBox_Box *ibb);
static void    _ibox_box_reconfigure(IBox_Box *ibb);
static void    _ibox_box_follower_reset(IBox_Box *ibb);
static void    _ibox_box_convert_move_resize_to_config(IBox_Box *ibb);
static void    _ibox_box_edge_change(IBox_Box *ibb, int edge);
static void    _ibox_cb_intercept_icon_move(void *data, Evas_Object *o, Evas_Coord x, Evas_Coord y);
static void    _ibox_cb_intercept_icon_resize(void *data, Evas_Object *o, Evas_Coord w, Evas_Coord h);
static void    _ibox_cb_intercept_box_move(void *data, Evas_Object *o, Evas_Coord x, Evas_Coord y);
static void    _ibox_cb_intercept_box_resize(void *data, Evas_Object *o, Evas_Coord w, Evas_Coord h);
static void    _ibox_cb_icon_in(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void    _ibox_cb_icon_out(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void    _ibox_cb_icon_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void    _ibox_cb_icon_up(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void    _ibox_cb_box_in(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void    _ibox_cb_box_out(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void    _ibox_cb_box_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void    _ibox_cb_box_up(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void    _ibox_cb_box_move(void *data, Evas *e, Evas_Object *obj, void *event_info);
static int     _ibox_cb_box_timer(void *data);
static int     _ibox_cb_box_animator(void *data);
static void    _ibox_cb_box_move_start(void *data, Evas_Object *obj, const char *emission, const char *source);
static void    _ibox_cb_box_move_stop(void *data, Evas_Object *obj, const char *emission, const char *source);
static void    _ibox_cb_box_resize1_start(void *data, Evas_Object *obj, const char *emission, const char *source);
static void    _ibox_cb_box_resize1_stop(void *data, Evas_Object *obj, const char *emission, const char *source);
static void    _ibox_cb_box_resize2_start(void *data, Evas_Object *obj, const char *emission, const char *source);
static void    _ibox_cb_box_resize2_stop(void *data, Evas_Object *obj, const char *emission, const char *source);
static void    _ibox_cb_box_move_go(void *data, Evas_Object *obj, const char *emission, const char *source);
static int     _ibox_cb_event_container_resize(void *data, int type, void *event);

static int     _ibox_cb_event_border_iconify(void *data, int type, void *event);
static int     _ibox_cb_event_border_uniconify(void *data, int type, void *event); 

/* public module routines. all modules must have these */
void *
init(E_Module *m)
{
   IBox *ib;
   
   e_error_dialog_show("IBox Disabled",
		       "The IBox module is diabled for now during\n"
		       "the mini freeze period. Come back later.\n");
   return NULL;
   /* check module api version */
   if (m->api->version < E_MODULE_API_VERSION)
     {
	e_error_dialog_show("Module API Error",
			    "Error initializing Module: IBox\n"
			    "It requires a minimum module API version of: %i.\n"
			    "The module API advertized by Enlightenment is: %i.\n"
			    "Aborting module.",
			    E_MODULE_API_VERSION,
			    m->api->version);
	return NULL;
     }
   /* actually init ibox */
   ib = _ibox_init(m);
   m->config_menu = _ibox_config_menu_new(ib);
   return ib;
}

int
shutdown(E_Module *m)
{
   IBox *ib;
   
   ib = m->data;
   if (ib)
     {
	if (m->config_menu)
	  {
	     _ibox_config_menu_del(ib, m->config_menu);
	     m->config_menu = NULL;
	  }
	_ibox_shutdown(ib);
     }
   return 1;
}

int
save(E_Module *m)
{
   IBox *ib;
   
   ib = m->data;
   e_config_domain_save("module.ibox", ib->conf_edd, ib->conf);
   return 1;
}

int
info(E_Module *m)
{
   char buf[4096];
   
   m->label = strdup("IBox");
   snprintf(buf, sizeof(buf), "%s/module_icon.png", e_module_dir_get(m));
   m->icon_file = strdup(buf);
   return 1;
}

int
about(E_Module *m)
{
   e_error_dialog_show("Enlightenment IBox Module",
		       "This is the IBox iconbar module for Enlightenment.\n");
   return 1;
}

/* module private routines */
static IBox *
_ibox_init(E_Module *m)
{
   IBox *ib;
   char buf[4096];
   Evas_List *managers, *l, *l2;
   
   ib = calloc(1, sizeof(IBox));
   if (!ib) return NULL;

   ib->conf_edd = E_CONFIG_DD_NEW("Ibox_Config", Config);
#undef T
#undef D
#define T Config
#define D ib->conf_edd
   E_CONFIG_VAL(D, T, follow_speed, DOUBLE);
   E_CONFIG_VAL(D, T, autoscroll_speed, DOUBLE);
   E_CONFIG_VAL(D, T, width, INT);
   E_CONFIG_VAL(D, T, iconsize, INT);
   E_CONFIG_VAL(D, T, edge, INT);
   E_CONFIG_VAL(D, T, anchor, DOUBLE);
   E_CONFIG_VAL(D, T, handle, DOUBLE);
   E_CONFIG_VAL(D, T, autohide, UCHAR);
   
   ib->conf = e_config_domain_load("module.ibox", ib->conf_edd);
   if (!ib->conf)
     {
	ib->conf = E_NEW(Config, 1);
	ib->conf->follow_speed = 9;
	ib->conf->autoscroll_speed = 0.95;
	ib->conf->width = 400;
	ib->conf->iconsize = 32;
	ib->conf->edge = EDGE_BOTTOM;
	ib->conf->anchor = 0.5;
	ib->conf->handle = 0.5;
	ib->conf->autohide = 0;
     }
   E_CONFIG_LIMIT(ib->conf->follow_speed, 0.01, 1.0);
   E_CONFIG_LIMIT(ib->conf->autoscroll_speed, 0.01, 1.0);
   E_CONFIG_LIMIT(ib->conf->width, -1, 4000);
   E_CONFIG_LIMIT(ib->conf->iconsize, 2, 400);
   E_CONFIG_LIMIT(ib->conf->edge, EDGE_BOTTOM, EDGE_RIGHT);
   E_CONFIG_LIMIT(ib->conf->anchor, 0.0, 1.0);
   E_CONFIG_LIMIT(ib->conf->handle, 0.0, 1.0);
   E_CONFIG_LIMIT(ib->conf->autohide, 0, 1);
   
   e_app_change_callback_add(_ibox_app_change, ib);
   
   managers = e_manager_list();
   for (l = managers; l; l = l->next)
     {
	E_Manager *man;
	
	man = l->data;
	for (l2 = man->containers; l2; l2 = l2->next)
	  {
	     E_Container *con;
	     IBox_Box *ibb;
	     
	     con = l2->data;
	     ibb = calloc(1, sizeof(IBox_Box));
	     if (ibb)
	       {
		  ibb->ibox = ib;
		  ibb->con = con;
		  ibb->evas = con->bg_evas;
		  ib->boxs = evas_list_append(ib->boxs, ibb);
		  _ibox_box_init(ibb);
	       }
	  }
     }

   ecore_event_handler_add(E_EVENT_BORDER_HIDE,
			   _ibox_cb_event_border_iconify, ib);
   ecore_event_handler_add(E_EVENT_BORDER_SHOW,
			   _ibox_cb_event_border_uniconify, ib);

   return ib;
}

static void
_ibox_shutdown(IBox *ib)
{
   free(ib->conf);
   E_CONFIG_DD_FREE(ib->conf_edd);
   e_app_change_callback_del(_ibox_app_change, ib);
   while (ib->boxs)
     {
	IBox_Box *ibb;
	
	ibb = ib->boxs->data;
	ib->boxs = evas_list_remove_list(ib->boxs, ib->boxs);
	_ibox_box_free(ibb);
     }
   free(ib);
}

static void
_ibox_app_change(void *data, E_App *a, E_App_Change ch)
{
   IBox *ib;
   Evas_List *l, *ll;
   
   ib = data;
}

/* FIXME: none of these work runtime... only on restart */
static void
_ibox_cb_iconsize_microscopic(void *data, E_Menu *m, E_Menu_Item *mi)
{
   IBox *ib;
   Evas_List *l;
   
   ib = data;
   ib->conf->iconsize = 8;
   for (l = ib->boxs; l; l = l->next)
     {
	IBox_Box *ibb;
	
	ibb = l->data;
	_ibox_box_iconsize_change(ibb);
	_ibox_box_edge_change(ibb, ib->conf->edge);
     }
   e_config_save_queue();
}

static void
_ibox_cb_iconsize_tiny(void *data, E_Menu *m, E_Menu_Item *mi)
{
   IBox *ib;
   Evas_List *l;
   
   ib = data;
   ib->conf->iconsize = 12;
   for (l = ib->boxs; l; l = l->next)
     {
	IBox_Box *ibb;
	
	ibb = l->data;
	_ibox_box_iconsize_change(ibb);
	_ibox_box_edge_change(ibb, ib->conf->edge);
     }
   e_config_save_queue();
}

static void
_ibox_cb_iconsize_very_small(void *data, E_Menu *m, E_Menu_Item *mi)
{
   IBox *ib;
   Evas_List *l;
   
   ib = data;
   ib->conf->iconsize = 16;
   for (l = ib->boxs; l; l = l->next)
     {
	IBox_Box *ibb;
	
	ibb = l->data;
	_ibox_box_iconsize_change(ibb);
	_ibox_box_edge_change(ibb, ib->conf->edge);
     }
   e_config_save_queue();
}

static void
_ibox_cb_iconsize_small(void *data, E_Menu *m, E_Menu_Item *mi)
{
   IBox *ib;
   Evas_List *l;
   
   ib = data;
   ib->conf->iconsize = 24;
   for (l = ib->boxs; l; l = l->next)
     {
	IBox_Box *ibb;
	
	ibb = l->data;
	_ibox_box_iconsize_change(ibb);
	_ibox_box_edge_change(ibb, ib->conf->edge);
     }
   e_config_save_queue();
}

static void
_ibox_cb_iconsize_medium(void *data, E_Menu *m, E_Menu_Item *mi)
{
   IBox *ib;
   Evas_List *l;
   
   ib = data;
   ib->conf->iconsize = 32;
   for (l = ib->boxs; l; l = l->next)
     {
	IBox_Box *ibb;
	
	ibb = l->data;
	_ibox_box_iconsize_change(ibb);
	_ibox_box_edge_change(ibb, ib->conf->edge);
     }
   e_config_save_queue();
}

static void
_ibox_cb_iconsize_large(void *data, E_Menu *m, E_Menu_Item *mi)
{
   IBox *ib;
   Evas_List *l;
   
   ib = data;
   ib->conf->iconsize = 40;
   for (l = ib->boxs; l; l = l->next)
     {
	IBox_Box *ibb;
	
	ibb = l->data;
	_ibox_box_iconsize_change(ibb);
	_ibox_box_edge_change(ibb, ib->conf->edge);
     }
   e_config_save_queue();
}

static void
_ibox_cb_iconsize_very_large(void *data, E_Menu *m, E_Menu_Item *mi)
{
   IBox *ib;
   Evas_List *l;
   
   ib = data;
   ib->conf->iconsize = 48;
   for (l = ib->boxs; l; l = l->next)
     {
	IBox_Box *ibb;
	
	ibb = l->data;
	_ibox_box_iconsize_change(ibb);
	_ibox_box_edge_change(ibb, ib->conf->edge);
     }
   e_config_save_queue();
}

static void
_ibox_cb_iconsize_extremely_large(void *data, E_Menu *m, E_Menu_Item *mi)
{
   IBox *ib;
   Evas_List *l;
   
   ib = data;
   ib->conf->iconsize = 56;
   for (l = ib->boxs; l; l = l->next)
     {
	IBox_Box *ibb;
	
	ibb = l->data;
	_ibox_box_iconsize_change(ibb);
	_ibox_box_edge_change(ibb, ib->conf->edge);
     }
   e_config_save_queue();
}

static void
_ibox_cb_iconsize_huge(void *data, E_Menu *m, E_Menu_Item *mi)
{
   IBox *ib;
   Evas_List *l;
   
   ib = data;
   ib->conf->iconsize = 64;
   for (l = ib->boxs; l; l = l->next)
     {
	IBox_Box *ibb;
	
	ibb = l->data;
	_ibox_box_iconsize_change(ibb);
	_ibox_box_edge_change(ibb, ib->conf->edge);
     }
   e_config_save_queue();
}

static void
_ibox_cb_iconsize_enormous(void *data, E_Menu *m, E_Menu_Item *mi)
{
   IBox *ib;
   Evas_List *l;
   
   ib = data;
   ib->conf->iconsize = 96;
   for (l = ib->boxs; l; l = l->next)
     {
	IBox_Box *ibb;
	
	ibb = l->data;
	_ibox_box_iconsize_change(ibb);
	_ibox_box_edge_change(ibb, ib->conf->edge);
     }
   e_config_save_queue();
}

static void
_ibox_cb_iconsize_gigantic(void *data, E_Menu *m, E_Menu_Item *mi)
{
   IBox *ib;
   Evas_List *l;
   
   ib = data;
   ib->conf->iconsize = 128;
   for (l = ib->boxs; l; l = l->next)
     {
	IBox_Box *ibb;
	
	ibb = l->data;
	_ibox_box_iconsize_change(ibb);
	_ibox_box_edge_change(ibb, ib->conf->edge);
     }
   e_config_save_queue();
}

static E_Menu *
_ibox_config_menu_new(IBox *ib)
{
   E_Menu *mn;
   E_Menu_Item *mi;

   /* FIXME: hook callbacks to each menu item */
   mn = e_menu_new();
   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Fixed width");
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (ib->conf->width > 0) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ibox_cb_width_fixed, ib);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Auto fit icons");
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (ib->conf->width < 0) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ibox_cb_width_auto, ib);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Fill edge");
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 1);
   if (ib->conf->width == 0) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ibox_cb_width_fill, ib);
   
   mi = e_menu_item_new(mn);
   e_menu_item_separator_set(mi, 1);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Microscopic");
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 2);
   if (ib->conf->iconsize == 8) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ibox_cb_iconsize_microscopic, ib);
   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Tiny");
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 2);
   if (ib->conf->iconsize == 12) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ibox_cb_iconsize_tiny, ib);
   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Very Small");
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 2);
   if (ib->conf->iconsize == 16) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ibox_cb_iconsize_very_small, ib);
   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Small");
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 2);
   if (ib->conf->iconsize == 24) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ibox_cb_iconsize_small, ib);
   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Medium");
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 2);
   if (ib->conf->iconsize == 32) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ibox_cb_iconsize_medium, ib);
   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Large");
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 2);
   if (ib->conf->iconsize == 40) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ibox_cb_iconsize_large, ib);
   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Very Large");
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 2);
   if (ib->conf->iconsize == 48) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ibox_cb_iconsize_very_large, ib);
   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Exteremely Large");
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 2);
   if (ib->conf->iconsize == 56) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ibox_cb_iconsize_extremely_large, ib);
   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Huge");
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 2);
   if (ib->conf->iconsize == 64) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ibox_cb_iconsize_huge, ib);
   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Enormous");
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 2);
   if (ib->conf->iconsize == 96) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ibox_cb_iconsize_enormous, ib);
   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Gigantic");
   e_menu_item_radio_set(mi, 1);
   e_menu_item_radio_group_set(mi, 2);
   if (ib->conf->iconsize == 128) e_menu_item_toggle_set(mi, 1);
   e_menu_item_callback_set(mi, _ibox_cb_iconsize_gigantic, ib);
   
/*   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "Auto hide");
   e_menu_item_check_set(mi, 1);
   if (ib->conf->autohide == 0) e_menu_item_toggle_set(mi, 1);

   mi = e_menu_item_new(mn);
   e_menu_item_separator_set(mi, 1);
   
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, "More Options...");
*/
   ib->config_menu = mn;
   
   return mn;
}

static void
_ibox_config_menu_del(IBox *ib, E_Menu *m)
{
   e_object_del(E_OBJECT(m));
}

static void
_ibox_cb_width_fixed(void *data, E_Menu *m, E_Menu_Item *mi)
{
   IBox *ib;
   
   ib = data;
   if (ib->conf->width <= 0)
     {
	Evas_List *l;
	
	ib->conf->width = 400;
	for (l = ib->boxs; l; l = l->next)
	  {
	     IBox_Box *ibb;
	     
	     ibb = l->data;
	     _ibox_box_edge_change(ibb, ib->conf->edge);
	  }
     }
   e_config_save_queue();
}

static void
_ibox_cb_width_auto(void *data, E_Menu *m, E_Menu_Item *mi)
{
   IBox *ib;
   
   ib = data;
   if (ib->conf->width >= 0)
     {
	Evas_List *l;
	
	ib->conf->width = -1;
	for (l = ib->boxs; l; l = l->next)
	  {
	     IBox_Box *ibb;
	     
	     ibb = l->data;
	     _ibox_box_edge_change(ibb, ib->conf->edge);
	  }
     }
   e_config_save_queue();
}

static void
_ibox_cb_width_fill(void *data, E_Menu *m, E_Menu_Item *mi)
{
   IBox *ib;
   
   ib = data;
   if (ib->conf->width != 0)
     {
	Evas_List *l;
	
	ib->conf->width = 0;
	ib->conf->anchor = 0.5;
	ib->conf->handle = 0.5;
	for (l = ib->boxs; l; l = l->next)
	  {
	     IBox_Box *ibb;
	     
	     ibb = l->data;
	     _ibox_box_edge_change(ibb, ib->conf->edge);
	  }
     }
   e_config_save_queue();
}

static void
_ibox_box_iconsize_change(IBox_Box *ibb)
{
   Evas_List *l;

   _ibox_box_frame_resize(ibb);
   
   for (l = ibb->icons; l; l = l->next)
     {
	IBox_Icon *ic;
	
	ic = l->data;
	_ibox_box_icon_resize(ic);
     }
   _ibox_box_convert_move_resize_to_config(ibb);
}

static IBox_Icon *
_ibox_box_icon_find(IBox_Box *ibb, E_App *a)
{
   Evas_List *l;
   
   for (l = ibb->icons; l; l = l->next)
     {
	IBox_Icon *ic;
	
	ic = l->data;
	if (ic->app == a) return ic;
     }
   return NULL;
}

static void
_ibox_box_icon_del(IBox_Icon *ic)
{
   ic->ibb->icons = evas_list_remove(ic->ibb->icons, ic);
   if (ic->bg_object) evas_object_del(ic->bg_object);
   if (ic->overlay_object) evas_object_del(ic->overlay_object);
   if (ic->icon_object) evas_object_del(ic->icon_object);
   if (ic->event_object) evas_object_del(ic->event_object);
   while (ic->extra_icons)
     {
	Evas_Object *o;
	
	o = ic->extra_icons->data;
	ic->extra_icons = evas_list_remove_list(ic->extra_icons, ic->extra_icons);
	evas_object_del(o);
     }
   e_object_unref(E_OBJECT(ic->app));
   e_object_unref(E_OBJECT(ic->border));
   free(ic);
}

static IBox_Icon *
_ibox_box_icon_new(IBox_Box *ibb, E_App *a, E_Border *bd)
{
   IBox_Icon *ic;
   char *str;
   Evas_Object *o;
   Evas_Coord bw, bh;
   
   ic = calloc(1, sizeof(IBox_Icon));
   if (!ic) return NULL;
   ic->ibb = ibb;
   ic->app = a;
   e_object_ref(E_OBJECT(a));
   ic->border = bd;
   e_object_ref(E_OBJECT(bd));
   ibb->icons = evas_list_append(ibb->icons, ic);
   
   o = evas_object_rectangle_add(ibb->evas);
   ic->event_object = o;
   evas_object_layer_set(o, 1);
   evas_object_clip_set(o, evas_object_clip_get(ibb->base_object));
   evas_object_color_set(o, 0, 0, 0, 0);
   evas_object_repeat_events_set(o, 1);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_IN,  _ibox_cb_icon_in,  ic);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_OUT, _ibox_cb_icon_out, ic);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _ibox_cb_icon_down, ic);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_UP, _ibox_cb_icon_up, ic);
   evas_object_show(o);
   
   o = edje_object_add(ibb->evas);
   ic->bg_object = o;
   evas_object_intercept_move_callback_add(o, _ibox_cb_intercept_icon_move, ic);
   evas_object_intercept_resize_callback_add(o, _ibox_cb_intercept_icon_resize, ic);
   edje_object_file_set(o,
			/* FIXME: "default.edj" needs to come from conf */
			e_path_find(path_themes, "default.edj"),
			"modules/ibar/icon");
   edje_object_signal_emit(o, "set_orientation", _ibox_main_orientation[ibb->ibox->conf->edge]);
   edje_object_message_signal_process(o);
   evas_object_show(o);
   
   o = edje_object_add(ibb->evas);
   ic->icon_object = o;
   edje_object_file_set(o, ic->app->path, "icon");
   edje_extern_object_min_size_set(o, ibb->ibox->conf->iconsize, ibb->ibox->conf->iconsize);
   edje_object_part_swallow(ic->bg_object, "item", o);
   edje_object_size_min_calc(ic->bg_object, &bw, &bh);
   evas_object_pass_events_set(o, 1);
   evas_object_show(o);
   
   o = edje_object_add(ibb->evas);
   ic->overlay_object = o;
   evas_object_intercept_move_callback_add(o, _ibox_cb_intercept_icon_move, ic);
   evas_object_intercept_resize_callback_add(o, _ibox_cb_intercept_icon_resize, ic);
   edje_object_file_set(o,
			/* FIXME: "default.edj" needs to come from conf */
			e_path_find(path_themes, "default.edj"),
			"modules/ibar/icon_overlay");
   edje_object_signal_emit(o, "set_orientation", _ibox_main_orientation[ibb->ibox->conf->edge]);
   edje_object_message_signal_process(o);
   evas_object_show(o);
   
   o = edje_object_add(ibb->evas);
   ic->extra_icons = evas_list_append(ic->extra_icons, o);
   edje_object_file_set(o, ic->app->path, "icon");
   edje_object_part_swallow(ic->overlay_object, "item", o);
   evas_object_pass_events_set(o, 1);
   evas_object_show(o);
   
   evas_object_raise(ic->event_object);
   
   e_box_pack_end(ibb->base_object, ic->bg_object);
   e_box_pack_options_set(ic->bg_object,
			  1, 1, /* fill */
			  0, 0, /* expand */
			  0.5, 0.5, /* align */
			  bw, bh, /* min */
			  bw, bh /* max */
			  );
   
   str = (char *)edje_object_data_get(ic->icon_object, "raise_on_hilight");
   if (str)
     {
	if (atoi(str) == 1) ic->raise_on_hilight = 1;
     }
   
   edje_object_signal_emit(ic->bg_object, "passive", "");
   edje_object_signal_emit(ic->overlay_object, "passive", "");
   return ic;
}

static void
_ibox_box_icon_resize(IBox_Icon *ic)
{
   Evas_Object *o;
   Evas_Coord bw, bh;
   
   e_box_freeze(ic->ibb->base_object);
   o = ic->icon_object;
   edje_extern_object_min_size_set(o, ic->ibb->ibox->conf->iconsize, ic->ibb->ibox->conf->iconsize);

   evas_object_resize(o, ic->ibb->ibox->conf->iconsize, ic->ibb->ibox->conf->iconsize);

   edje_object_part_swallow(ic->bg_object, "item", o);
   edje_object_size_min_calc(ic->bg_object, &bw, &bh);

   e_box_pack_options_set(ic->bg_object,
			  1, 1, /* fill */
			  0, 0, /* expand */
			  0.5, 0.5, /* align */
			  bw, bh, /* min */
			  bw, bh /* max */
			  );
   e_box_thaw(ic->ibb->base_object);
}

static void
_ibox_box_icon_reorder_before(IBox_Icon *ic, IBox_Icon *before)
{
   Evas_Coord bw, bh;
   
   e_box_freeze(ic->ibb->base_object);
   e_box_unpack(ic->bg_object);
   ic->ibb->icons = evas_list_remove(ic->ibb->icons, ic);
   if (before)
     {
	ic->ibb->icons = evas_list_prepend_relative(ic->ibb->icons, ic, before);
	e_box_pack_before(ic->ibb->base_object, ic->bg_object, before->bg_object);
     }
   else
     {
	ic->ibb->icons = evas_list_prepend(ic->ibb->icons, ic);
	e_box_pack_start(ic->ibb->base_object, ic->bg_object);
     }
   edje_object_size_min_calc(ic->bg_object, &bw, &bh);
   e_box_pack_options_set(ic->bg_object,
			  1, 1, /* fill */
			  0, 0, /* expand */
			  0.5, 0.5, /* align */
			  bw, bh, /* min */
			  bw, bh /* max */
			  );
   e_box_thaw(ic->ibb->base_object);
}

static void
_ibox_box_icon_reorder_after(IBox_Icon *ic, IBox_Icon *after)
{
   Evas_Coord bw, bh;
   
   e_box_freeze(ic->ibb->base_object);
   e_box_unpack(ic->bg_object);
   ic->ibb->icons = evas_list_remove(ic->ibb->icons, ic);
   if (after)
     {
	ic->ibb->icons = evas_list_append_relative(ic->ibb->icons, ic, after);
	e_box_pack_after(ic->ibb->base_object, ic->bg_object, after->bg_object);
     }
   else
     {
	ic->ibb->icons = evas_list_append(ic->ibb->icons, ic);
	e_box_pack_end(ic->ibb->base_object, ic->bg_object);
     }
   edje_object_size_min_calc(ic->bg_object, &bw, &bh);
   e_box_pack_options_set(ic->bg_object,
			  1, 1, /* fill */
			  0, 0, /* expand */
			  0.5, 0.5, /* align */
			  bw, bh, /* min */
			  bw, bh /* max */
			  );
   e_box_thaw(ic->ibb->base_object);
}

static void
_ibox_box_frame_resize(IBox_Box *ibb)
{
   Evas_Coord ww, hh, bw, bh;
   Evas_Object *o;
   
   evas_event_freeze(ibb->evas);
   e_box_freeze(ibb->base_object);
   
   evas_output_viewport_get(ibb->evas, NULL, NULL, &ww, &hh);
   o = ibb->box_object;
   if (ibb->ibox->conf->width < 0)
     {
	if ((ibb->ibox->conf->edge == EDGE_BOTTOM) ||
	    (ibb->ibox->conf->edge == EDGE_TOP))
	  e_box_orientation_set(ibb->base_object, 1);
	else if ((ibb->ibox->conf->edge == EDGE_LEFT) ||
		 (ibb->ibox->conf->edge == EDGE_RIGHT))
	  e_box_orientation_set(ibb->base_object, 0);
	
	e_box_min_size_get(ibb->base_object, &bw, &bh);
	edje_extern_object_min_size_set(ibb->base_object, bw, bh);
	edje_object_part_swallow(o, "items", ibb->base_object);
	edje_object_size_min_calc(o, &bw, &bh);
	if ((ibb->ibox->conf->edge == EDGE_BOTTOM) ||
	    (ibb->ibox->conf->edge == EDGE_TOP))
	  {
	     if (bw > ww) bw = ww;
	  }
	else if ((ibb->ibox->conf->edge == EDGE_LEFT) ||
		 (ibb->ibox->conf->edge == EDGE_RIGHT))
	  {
	     if (bh > hh) bh = hh;
	  }
     }
   else if (ibb->ibox->conf->width == 0)
     {
	if ((ibb->ibox->conf->edge == EDGE_BOTTOM) ||
	    (ibb->ibox->conf->edge == EDGE_TOP))
	  {
	     e_box_orientation_set(ibb->base_object, 1);
	     e_box_min_size_get(ibb->base_object, &bw, &bh);
	     edje_extern_object_min_size_set(ibb->base_object, ww, bh);
	  }
	else if ((ibb->ibox->conf->edge == EDGE_LEFT) ||
		 (ibb->ibox->conf->edge == EDGE_RIGHT))
	  {
	     e_box_orientation_set(ibb->base_object, 0);
	     e_box_min_size_get(ibb->base_object, &bw, &bh);
	     edje_extern_object_min_size_set(ibb->base_object, bw, hh);
	  }
	
	edje_object_part_swallow(o, "items", ibb->base_object);
	edje_object_size_min_calc(o, &bw, &bh);
     }
   else
     {
	if ((ibb->ibox->conf->edge == EDGE_BOTTOM) ||
	    (ibb->ibox->conf->edge == EDGE_TOP))
	  {
	     e_box_orientation_set(ibb->base_object, 1);
	     e_box_min_size_get(ibb->base_object, &bw, &bh);
	     edje_extern_object_min_size_set(ibb->base_object, ibb->ibox->conf->width, bh);
	     edje_object_part_swallow(o, "items", ibb->base_object);
	     edje_object_size_min_calc(o, &bw, &bh);
	     edje_extern_object_min_size_set(ibb->base_object, 0, 0);
	     edje_object_part_swallow(o, "items", ibb->base_object);
	  }
	else if ((ibb->ibox->conf->edge == EDGE_LEFT) ||
		 (ibb->ibox->conf->edge == EDGE_RIGHT))
	  {
	     e_box_orientation_set(ibb->base_object, 0);
	     e_box_min_size_get(ibb->base_object, &bw, &bh);
	     edje_extern_object_min_size_set(ibb->base_object, bw, ibb->ibox->conf->width);
	     edje_object_part_swallow(o, "items", ibb->base_object);
	     edje_object_size_min_calc(o, &bw, &bh);
	     edje_extern_object_min_size_set(ibb->base_object, 0, 0);
	     edje_object_part_swallow(o, "items", ibb->base_object);
	  }
	if ((ibb->ibox->conf->edge == EDGE_BOTTOM) ||
	    (ibb->ibox->conf->edge == EDGE_TOP))
	  {
	     if (bw > ww) bw = bw;
	  }
	else if ((ibb->ibox->conf->edge == EDGE_LEFT) ||
		 (ibb->ibox->conf->edge == EDGE_RIGHT))
	  {
	     if (bh > hh) bh = hh;
	  }
     }
   
   if ((ibb->ibox->conf->edge == EDGE_BOTTOM) ||
       (ibb->ibox->conf->edge == EDGE_TOP))
     ibb->maxsize = bh;
   else if ((ibb->ibox->conf->edge == EDGE_LEFT) ||
	    (ibb->ibox->conf->edge == EDGE_RIGHT))
     ibb->maxsize = bw;

   ibb->w = bw;
   ibb->h = bh;
   if (ibb->ibox->conf->edge == EDGE_BOTTOM)
     {
	ibb->x = (ww * ibb->ibox->conf->anchor) - (bw * ibb->ibox->conf->handle);
	ibb->y = hh - bh;
     }
   else if (ibb->ibox->conf->edge == EDGE_TOP)
     {
	ibb->x = (ww * ibb->ibox->conf->anchor) - (bw * ibb->ibox->conf->handle);
	ibb->y = 0;
     }
   else if (ibb->ibox->conf->edge == EDGE_LEFT)
     {
	ibb->y = (hh * ibb->ibox->conf->anchor) - (bh * ibb->ibox->conf->handle);
	ibb->x = 0;
     }
   else if (ibb->ibox->conf->edge == EDGE_RIGHT)
     {
	ibb->y = (hh * ibb->ibox->conf->anchor) - (bh * ibb->ibox->conf->handle);
	ibb->x = ww - bw;
     }

   _ibox_box_reconfigure(ibb);
   
   e_box_thaw(ibb->base_object);

   _ibox_box_follower_reset(ibb);
   _ibox_timer_handle(ibb);
   
   evas_event_thaw(ibb->evas);
}

static void
_ibox_box_init(IBox_Box *ibb)
{
   Evas_List *l;
   Evas_Coord bw, bh;
   Evas_Object *o;

   ibb->ev_handler_container_resize = 
     ecore_event_handler_add(E_EVENT_CONTAINER_RESIZE, 
			     _ibox_cb_event_container_resize,
			     ibb);
   evas_event_freeze(ibb->evas);
   o = edje_object_add(ibb->evas);
   ibb->box_object = o;
   edje_object_file_set(o,
			/* FIXME: "default.edj" needs to come from conf */
			e_path_find(path_themes, "default.edj"),
			"modules/ibar/main");
   edje_object_signal_emit(o, "set_orientation", _ibox_main_orientation[ibb->ibox->conf->edge]);
   edje_object_message_signal_process(o);
   edje_object_signal_callback_add(o, "move_start", "", _ibox_cb_box_move_start, ibb);
   edje_object_signal_callback_add(o, "move_stop", "", _ibox_cb_box_move_stop, ibb);
   edje_object_signal_callback_add(o, "resize1_start", "", _ibox_cb_box_resize1_start, ibb);
   edje_object_signal_callback_add(o, "resize1_stop", "", _ibox_cb_box_resize1_stop, ibb);
   edje_object_signal_callback_add(o, "resize2_start", "", _ibox_cb_box_resize2_start, ibb);
   edje_object_signal_callback_add(o, "resize2_stop", "", _ibox_cb_box_resize2_stop, ibb);
   edje_object_signal_callback_add(o, "mouse,move", "*", _ibox_cb_box_move_go, ibb);
   evas_object_show(o);
   
   o = edje_object_add(ibb->evas);
   ibb->overlay_object = o;
   evas_object_layer_set(o, 1);
   edje_object_file_set(o,
			/* FIXME: "default.edj" needs to come from conf */
			e_path_find(path_themes, "default.edj"),
			"modules/ibar/follower");
   edje_object_signal_emit(o, "set_orientation", _ibox_main_orientation[ibb->ibox->conf->edge]);
   edje_object_message_signal_process(o);
   evas_object_show(o);
   
   o = evas_object_rectangle_add(ibb->evas);
   ibb->event_object = o;
   evas_object_layer_set(o, 2);
   evas_object_repeat_events_set(o, 1);
   evas_object_color_set(o, 0, 0, 0, 0);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_IN,  _ibox_cb_box_in,  ibb);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_OUT, _ibox_cb_box_out, ibb);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _ibox_cb_box_down, ibb);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_UP, _ibox_cb_box_up, ibb);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_MOVE, _ibox_cb_box_move, ibb);
   evas_object_show(o);
   
   o = e_box_add(ibb->evas);
   ibb->base_object = o;
   evas_object_intercept_move_callback_add(o, _ibox_cb_intercept_box_move, ibb);
   evas_object_intercept_resize_callback_add(o, _ibox_cb_intercept_box_resize, ibb);
   e_box_freeze(o);
   edje_object_part_swallow(ibb->box_object, "items", o);
   evas_object_show(o);

   edje_object_size_min_calc(ibb->box_object, &bw, &bh);
   if ((ibb->ibox->conf->edge == EDGE_BOTTOM) ||
       (ibb->ibox->conf->edge == EDGE_TOP))
     ibb->minsize = bh;
   else if ((ibb->ibox->conf->edge == EDGE_BOTTOM) ||
	    (ibb->ibox->conf->edge == EDGE_TOP))
     ibb->minsize = bw;

   /* FIXME search for iconified apps */
/*
   if (ibb->ibox->apps)
     {
	for (l = ibb->ibox->apps->subapps; l; l = l->next)
	  {
	     E_App *a;
	     IBox_Icon *ic;
	     
	     a = l->data;
	     ic = _ibox_box_icon_new(ibb, a);
	  }
     }
*/
   ibb->align_req = 0.5;
   ibb->align = 0.5;
   e_box_align_set(ibb->base_object, 0.5, 0.5);

   _ibox_box_frame_resize(ibb);
   
   e_box_thaw(ibb->base_object);

   _ibox_box_follower_reset(ibb);
   _ibox_timer_handle(ibb);
   
   evas_event_thaw(ibb->evas);
   
//   edje_object_signal_emit(ibb->box_object, "passive", "");
//   edje_object_signal_emit(ibb->overlay_object, "passive", "");
}

static void
_ibox_box_free(IBox_Box *ibb)
{
   ecore_event_handler_del(ibb->ev_handler_container_resize);
   while (ibb->icons)
     {
	IBox_Icon *ic;
	
	ic = ibb->icons->data;
	_ibox_box_icon_del(ic);
     }
   if (ibb->timer) ecore_timer_del(ibb->timer);
   if (ibb->animator) ecore_animator_del(ibb->animator);
   evas_object_del(ibb->box_object);
   evas_object_del(ibb->overlay_object);
   evas_object_del(ibb->base_object);
   evas_object_del(ibb->event_object);
   free(ibb);
}

static void
_ibox_motion_handle(IBox_Box *ibb, Evas_Coord mx, Evas_Coord my)
{
   Evas_Coord x, y, w, h;
   double relx, rely;
   
   evas_object_geometry_get(ibb->base_object, &x, &y, &w, &h);
   if (w > 0) relx = (double)(mx - x) / (double)w;
   else relx = 0.0;
   if (h > 0) rely = (double)(my - y) / (double)h;
   else rely = 0.0;
   if ((ibb->ibox->conf->edge == EDGE_BOTTOM) ||
       (ibb->ibox->conf->edge == EDGE_TOP))
     {
	ibb->align_req = 1.0 - relx;
	ibb->follow_req = relx;
     }
   else if ((ibb->ibox->conf->edge == EDGE_LEFT) ||
	    (ibb->ibox->conf->edge == EDGE_RIGHT))
     {
	ibb->align_req = 1.0 - rely;
	ibb->follow_req = rely;
     }
}

static void
_ibox_timer_handle(IBox_Box *ibb)
{
   if (!ibb->timer)
     ibb->timer = ecore_timer_add(0.01, _ibox_cb_box_timer, ibb);
   if (!ibb->animator)
     ibb->animator = ecore_animator_add(_ibox_cb_box_animator, ibb);
}

static void
_ibox_box_reconfigure(IBox_Box *ibb)
{
   evas_object_move(ibb->box_object, ibb->x, ibb->y);
   evas_object_resize(ibb->box_object, ibb->w, ibb->h);
   _ibox_timer_handle(ibb);
}

static void
_ibox_box_follower_reset(IBox_Box *ibb)
{
   Evas_Coord ww, hh, bx, by, bw, bh, d1, d2, mw, mh;
   
   evas_output_viewport_get(ibb->evas, NULL, NULL, &ww, &hh);
   evas_object_geometry_get(ibb->base_object, &bx, &by, &bw, &bh);
   edje_object_size_min_get(ibb->overlay_object, &mw, &mh);
   if ((ibb->ibox->conf->edge == EDGE_BOTTOM) ||
       (ibb->ibox->conf->edge == EDGE_TOP))
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
   else if ((ibb->ibox->conf->edge == EDGE_LEFT) ||
	    (ibb->ibox->conf->edge == EDGE_RIGHT))
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
_ibox_box_convert_move_resize_to_config(IBox_Box *ibb)
{
   Evas_Coord bx, by, bw, bh, bbx, bby, bbw, bbh, ww, hh;
   
   evas_output_viewport_get(ibb->evas, NULL, NULL, &ww, &hh);
   evas_object_geometry_get(ibb->base_object, &bx, &by, &bw, &bh);
   evas_object_geometry_get(ibb->box_object, &bbx, &bby, &bbw, &bbh);
   
   if ((ibb->ibox->conf->edge == EDGE_BOTTOM) ||
       (ibb->ibox->conf->edge == EDGE_TOP))
     {
	double a = 0.5;
	
	if (ibb->ibox->conf->width < 0) /* auto size to fit */
	  {
	     if ((ww - ibb->w) != 0)
	       a = (double)ibb->x / (double)(ww - ibb->w);
	     else
	       a = 0.5;
	  }
	else if (ibb->ibox->conf->width == 0) /* full width */
	  {
	  }
	else
	  {
	     ibb->ibox->conf->width = ibb->w - (bbw - bw);
	     if ((ww - ibb->w) != 0)
	       a = (double)ibb->x / (double)(ww - ibb->w);
	     else
	       a = 0.5;
	  }
	ibb->ibox->conf->anchor = a;
	ibb->ibox->conf->handle = a;
     }
   else if ((ibb->ibox->conf->edge == EDGE_LEFT) ||
	    (ibb->ibox->conf->edge == EDGE_RIGHT))
     {
	double a = 0.5;
	
	if (ibb->ibox->conf->width < 0) /* auto size to fit */
	  {
	     if ((hh - ibb->h) != 0)
	       a = (double)ibb->y / (double)(hh - ibb->h);
	     else
	       a = 0.5;
	  }
	else if (ibb->ibox->conf->width == 0) /* full width */
	  {
	  }
	else
	  {
	     ibb->ibox->conf->width = ibb->h - (bbh - bh);
	     if ((hh - ibb->h) != 0)
	       a = (double)ibb->y / (double)(hh - ibb->h);
	     else
	       a = 0.5;
	  }
	ibb->ibox->conf->anchor = a;
	ibb->ibox->conf->handle = a;
     }
   e_config_save_queue();
}

static void
_ibox_box_edge_change(IBox_Box *ibb, int edge)
{
   Evas_List *l;
   Evas_Coord ww, hh, bw, bh;
   Evas_Object *o;
   
   ibb->ibox->conf->edge = edge;
   
   evas_event_freeze(ibb->evas);
   o = ibb->box_object;
   edje_object_signal_emit(o, "set_orientation", _ibox_main_orientation[ibb->ibox->conf->edge]);
   edje_object_message_signal_process(o);
   
   o = ibb->overlay_object;
   edje_object_signal_emit(o, "set_orientation", _ibox_main_orientation[ibb->ibox->conf->edge]);
   edje_object_message_signal_process(o);
   
   e_box_freeze(ibb->base_object);

   edje_object_size_min_calc(ibb->box_object, &bw, &bh);
   if ((ibb->ibox->conf->edge == EDGE_BOTTOM) ||
       (ibb->ibox->conf->edge == EDGE_TOP))
     ibb->minsize = bh;
   else if ((ibb->ibox->conf->edge == EDGE_BOTTOM) ||
	    (ibb->ibox->conf->edge == EDGE_TOP))
     ibb->minsize = bw;
   
   for (l = ibb->icons; l; l = l->next)
     {
	IBox_Icon *ic;
	     
	ic = l->data;
	o = ic->bg_object;
	edje_object_signal_emit(o, "set_orientation", _ibox_main_orientation[ibb->ibox->conf->edge]);
	edje_object_message_signal_process(o);
	edje_object_size_min_calc(ic->bg_object, &bw, &bh);
		  
	o = ic->overlay_object;
	edje_object_signal_emit(o, "set_orientation", _ibox_main_orientation[ibb->ibox->conf->edge]);
	edje_object_message_signal_process(o);
		  
	e_box_pack_options_set(ic->bg_object,
			       1, 1, /* fill */
			       0, 0, /* expand */
			       0.5, 0.5, /* align */
			       bw, bh, /* min */
			       bw, bh /* max */
			       );
     }
   evas_output_viewport_get(ibb->evas, NULL, NULL, &ww, &hh);
   
   ibb->align_req = 0.5;
   ibb->align = 0.5;
   e_box_align_set(ibb->base_object, 0.5, 0.5);

   _ibox_box_frame_resize(ibb);
   
   e_box_thaw(ibb->base_object);

   _ibox_box_follower_reset(ibb);
   _ibox_timer_handle(ibb);
   
   evas_event_thaw(ibb->evas);
}

static void
_ibox_cb_intercept_icon_move(void *data, Evas_Object *o, Evas_Coord x, Evas_Coord y)
{
   IBox_Icon *ic;

   ic = data;
   evas_object_move(o, x, y);
   evas_object_move(ic->event_object, x, y);
   evas_object_move(ic->overlay_object, x, y);
}

static void
_ibox_cb_intercept_icon_resize(void *data, Evas_Object *o, Evas_Coord w, Evas_Coord h)
{
   IBox_Icon *ic;

   ic = data;
   evas_object_resize(o, w, h);
   evas_object_resize(ic->event_object, w, h);
   evas_object_resize(ic->overlay_object, w, h);
}

static void
_ibox_cb_intercept_box_move(void *data, Evas_Object *o, Evas_Coord x, Evas_Coord y)
{
   IBox_Box *ibb;

   ibb = data;
   evas_object_move(o, x, y);
   evas_object_move(ibb->event_object, x, y);
}

static void
_ibox_cb_intercept_box_resize(void *data, Evas_Object *o, Evas_Coord w, Evas_Coord h)
{
   IBox_Box *ibb;

   ibb = data;

   evas_object_resize(o, w, h);
   evas_object_resize(ibb->event_object, w, h);
}

static void
_ibox_cb_icon_in(void *data, Evas *e, Evas_Object *obj, void *event_info)
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
   edje_object_signal_emit(ic->ibb->overlay_object, "active", "");
}

static void
_ibox_cb_icon_out(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Out *ev;
   IBox_Icon *ic;
   
   ev = event_info;
   ic = data;
   edje_object_signal_emit(ic->bg_object, "passive", "");
   edje_object_signal_emit(ic->overlay_object, "passive", "");
   edje_object_signal_emit(ic->ibb->overlay_object, "passive", "");
}

static void
_ibox_cb_icon_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   IBox_Icon *ic;
   
   ev = event_info;
   ic = data;
   if (ev->button == 1)
     {
	edje_object_signal_emit(ic->bg_object, "start", "");
	edje_object_signal_emit(ic->overlay_object, "start", "");
	edje_object_signal_emit(ic->ibb->overlay_object, "start", "");
	e_border_uniconify(ic->border);
     }
}

static void
_ibox_cb_icon_up(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Up *ev;
   IBox_Icon *ic;
   
   ev = event_info;
   ic = data;
   if (ev->button == 1)
     {
	edje_object_signal_emit(ic->bg_object, "start_end", "");
	edje_object_signal_emit(ic->overlay_object, "start_end", "");
	edje_object_signal_emit(ic->ibb->overlay_object, "start_end", "");
     }
}

static void
_ibox_cb_box_in(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_In *ev;
   IBox_Box *ibb;
   
   ev = event_info;
   ibb = data;
   edje_object_signal_emit(ibb->overlay_object, "active", "");
   _ibox_motion_handle(ibb, ev->canvas.x, ev->canvas.y);
   _ibox_timer_handle(ibb);
}

static void
_ibox_cb_box_out(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Out *ev;
   IBox_Box *ibb;
   
   ev = event_info;
   ibb = data;
   edje_object_signal_emit(ibb->overlay_object, "passive", "");
   _ibox_box_follower_reset(ibb);
   _ibox_timer_handle(ibb);
}

static void
_ibox_cb_box_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   IBox_Box *ibb;
   
   ev = event_info;
   ibb = data;
   if (ev->button == 3)
     {
	e_menu_activate_mouse(ibb->ibox->config_menu, e_zone_current_get(ibb->con),
			      ev->output.x, ev->output.y, 1, 1,
			      E_MENU_POP_DIRECTION_DOWN);
	e_util_container_fake_mouse_up_all_later(ibb->con);
     }
}

static void
_ibox_cb_box_up(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Up *ev;
   IBox_Box *ibb;
   
   ev = event_info;
   ibb = data;
}

static void
_ibox_cb_box_move(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Move *ev;
   IBox_Box *ibb;
   
   ev = event_info;
   ibb = data;
   _ibox_motion_handle(ibb, ev->cur.canvas.x, ev->cur.canvas.y);
   _ibox_timer_handle(ibb);
}

static int
_ibox_cb_box_timer(void *data)
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
_ibox_cb_box_animator(void *data)
{
   IBox_Box *ibb;
   Evas_Coord x, y, w, h, mw, mh;
   
   ibb = data;
   
   if ((ibb->ibox->conf->edge == EDGE_BOTTOM) ||
       (ibb->ibox->conf->edge == EDGE_TOP))
     {
	e_box_align_set(ibb->base_object, ibb->align, 0.5);
	
	evas_object_geometry_get(ibb->base_object, &x, &y, &w, &h);
	edje_object_size_min_get(ibb->overlay_object, &mw, &mh);
	evas_object_resize(ibb->overlay_object, mw, h);
	evas_object_move(ibb->overlay_object, x + (w * ibb->follow) - (mw / 2), y);
     }
   else if ((ibb->ibox->conf->edge == EDGE_LEFT) ||
	    (ibb->ibox->conf->edge == EDGE_RIGHT))
     {
	e_box_align_set(ibb->base_object, 0.5, ibb->align);
	
	evas_object_geometry_get(ibb->base_object, &x, &y, &w, &h);
	edje_object_size_min_get(ibb->overlay_object, &mw, &mh);
	evas_object_resize(ibb->overlay_object, w, mh);
	evas_object_move(ibb->overlay_object, x, y + (h * ibb->follow) - (mh / 2));
     }
   if (ibb->timer) return 1;
   ibb->animator = NULL;
   return 0;
}

static void
_ibox_cb_box_move_start(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   IBox_Box *ibb;
   
   ibb = data;
   if (ibb->ibox->conf->width == 0) return;
   ibb->move = 1;
   evas_pointer_canvas_xy_get(evas_object_evas_get(obj), &ibb->start_x, &ibb->start_y);
   ibb->start_bx = ibb->x;
   ibb->start_by = ibb->y;
   ibb->start_bw = ibb->w;
   ibb->start_bh = ibb->h;
}

static void
_ibox_cb_box_move_stop(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   IBox_Box *ibb;
   
   ibb = data;
   if (ibb->move)
     {
	_ibox_box_convert_move_resize_to_config(ibb);
	ibb->move = 0;
     }
}

static void
_ibox_cb_box_resize1_start(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   IBox_Box *ibb;
   
   ibb = data;
   if (ibb->ibox->conf->width <= 0) return;
   ibb->resize1 = 1;
   evas_pointer_canvas_xy_get(evas_object_evas_get(obj), &ibb->start_x, &ibb->start_y);
   ibb->start_bx = ibb->x;
   ibb->start_by = ibb->y;
   ibb->start_bw = ibb->w;
   ibb->start_bh = ibb->h;
}

static void
_ibox_cb_box_resize1_stop(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   IBox_Box *ibb;
   
   ibb = data;
   if (ibb->resize1)
     {
	_ibox_box_convert_move_resize_to_config(ibb);
	ibb->resize1 = 0;
     }
}

static void
_ibox_cb_box_resize2_start(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   IBox_Box *ibb;
   
   ibb = data;
   if (ibb->ibox->conf->width <= 0) return;
   ibb->resize2 = 1;
   evas_pointer_canvas_xy_get(evas_object_evas_get(obj), &ibb->start_x, &ibb->start_y);
   ibb->start_bx = ibb->x;
   ibb->start_by = ibb->y;
   ibb->start_bw = ibb->w;
   ibb->start_bh = ibb->h;
}

static void
_ibox_cb_box_resize2_stop(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   IBox_Box *ibb;
   
   ibb = data;
   if (ibb->resize2)
     {
	_ibox_box_convert_move_resize_to_config(ibb);
	ibb->resize2 = 0;
     }
}

static void
_ibox_cb_box_move_go(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   IBox_Box *ibb;
   
   ibb = data;
   if (ibb->move)
     {
	Evas_Coord x, y, bx, by, bw, bh, ww, hh;
	int edge;
	double xr, yr;
	int edge_done;
	
	edge_done = 0;
	do_pos:
	evas_output_viewport_get(ibb->evas, NULL, NULL, &ww, &hh);
	evas_object_geometry_get(ibb->box_object, &bx, &by, &bw, &bh);
	evas_pointer_canvas_xy_get(ibb->evas, &x, &y);
	if ((ibb->ibox->conf->edge == EDGE_BOTTOM) ||
	    (ibb->ibox->conf->edge == EDGE_TOP))
	  {
	     Evas_Coord d;
	     
	     d = x - ibb->start_x;
	     ibb->x = ibb->start_bx + d;
	     if (ibb->x < 0) ibb->x = 0;
	     else if ((ibb->x + ibb->w) > ww) ibb->x = ww - ibb->w;
	  }
	else if ((ibb->ibox->conf->edge == EDGE_LEFT) ||
		 (ibb->ibox->conf->edge == EDGE_RIGHT))
	  {
	     Evas_Coord d;
	     
	     d = y - ibb->start_y;
	     ibb->y = ibb->start_by + d;
	     if (ibb->y < 0) ibb->y = 0;
	     else if ((ibb->y + ibb->h) > hh) ibb->y = hh - ibb->h;
	  }

	if (!edge_done)
	  {
	     edge = ibb->ibox->conf->edge;
	     xr = (double)x / (double)ww;
	     yr = (double)y / (double)hh;
	     if ((xr + yr) <= 1.0) /* top or left */
	       {
		  if (((1.0 - yr) + xr) <= 1.0) edge = EDGE_LEFT;
		  else edge = EDGE_TOP;
	       }
	     else /* bottom or right */
	       {
		  if (((1.0 - yr) + xr) <= 1.0) edge = EDGE_BOTTOM;
		  else edge = EDGE_RIGHT;
	       }
	     if (edge != ibb->ibox->conf->edge)
	       {
		  _ibox_box_edge_change(ibb, edge);
		  edge_done = 1;
		  goto do_pos;
	       }
	  }
	_ibox_box_reconfigure(ibb);
	_ibox_box_follower_reset(ibb);
	_ibox_timer_handle(ibb);
	return;
     }
   else if (ibb->resize1)
     {
	Evas_Coord x, y, bx, by, bw, bh, bbx, bby, bbw, bbh, ww, hh;
	
	evas_output_viewport_get(ibb->evas, NULL, NULL, &ww, &hh);
	evas_object_geometry_get(ibb->base_object, &bx, &by, &bw, &bh);
	evas_object_geometry_get(ibb->box_object, &bbx, &bby, &bbw, &bbh);
	evas_pointer_canvas_xy_get(ibb->evas, &x, &y);
	if ((ibb->ibox->conf->edge == EDGE_BOTTOM) ||
	    (ibb->ibox->conf->edge == EDGE_TOP))
	  {
	     Evas_Coord d;
	     
	     d = x - ibb->start_x;
	     ibb->x = ibb->start_bx + d;
	     ibb->w = ibb->start_bw - d;
	     if (ibb->w < (bbw - bw + ibb->ibox->conf->iconsize))
	       {
		  ibb->x += ibb->w - (bbw - bw + ibb->ibox->conf->iconsize);
		  ibb->w = bbw - bw + ibb->ibox->conf->iconsize;
	       }
	     else if (ibb->w > ww)
	       {
		  ibb->x += (ibb->w - ww);
		  ibb->w = ww;
	       }
	     if (ibb->x < 0)
	       {
		  ibb->w += ibb->x;
		  ibb->x = 0;
	       }
	     else if ((ibb->x + ibb->w) > ww)
	       {
		  ibb->x = ww - ibb->w;
	       }
	  }
	else if ((ibb->ibox->conf->edge == EDGE_LEFT) ||
		 (ibb->ibox->conf->edge == EDGE_RIGHT))
	  {
	     Evas_Coord d;
	     
	     d = y - ibb->start_y;
	     ibb->y = ibb->start_by + d;
	     ibb->h = ibb->start_bh - d;
	     if (ibb->h < (bbh - bh + ibb->ibox->conf->iconsize))
	       {
		  ibb->y += ibb->h - (bbh - bh + ibb->ibox->conf->iconsize);
		  ibb->h = bbh - bh + ibb->ibox->conf->iconsize;
	       }
	     else if (ibb->h > hh)
	       {
		  ibb->y += (ibb->h - hh);
		  ibb->h = hh;
	       }
	     if (ibb->y < 0)
	       {
		  ibb->h += ibb->y;
		  ibb->y = 0;
	       }
	     else if ((ibb->y + ibb->h) > hh)
	       {
		  ibb->y = hh - ibb->h;
	       }
	  }
	_ibox_box_reconfigure(ibb);
	_ibox_box_follower_reset(ibb);
	_ibox_timer_handle(ibb);
	return;
     }
   else if (ibb->resize2)
     {
	Evas_Coord x, y, bx, by, bw, bh, bbx, bby, bbw, bbh, ww, hh;
	
	evas_output_viewport_get(ibb->evas, NULL, NULL, &ww, &hh);
	evas_object_geometry_get(ibb->base_object, &bx, &by, &bw, &bh);
	evas_object_geometry_get(ibb->box_object, &bbx, &bby, &bbw, &bbh);
	evas_pointer_canvas_xy_get(ibb->evas, &x, &y);
	if ((ibb->ibox->conf->edge == EDGE_BOTTOM) ||
	    (ibb->ibox->conf->edge == EDGE_TOP))
	  {
	     Evas_Coord d;
	     
	     d = x - ibb->start_x;
	     ibb->w = ibb->start_bw + d;
	     if (ibb->w < (bbw - bw + ibb->ibox->conf->iconsize))
	       {
		  ibb->w = bbw - bw + ibb->ibox->conf->iconsize;
	       }
	     else if (ibb->w > ww)
	       {
		  ibb->w = ww;
	       }
	     if ((ibb->x + ibb->w) > ww)
	       {
		  ibb->w = ww - ibb->x;
	       }
	  }
	else if ((ibb->ibox->conf->edge == EDGE_LEFT) ||
		 (ibb->ibox->conf->edge == EDGE_RIGHT))
	  {
	     Evas_Coord d;
	     
	     d = y - ibb->start_y;
	     ibb->h = ibb->start_bh + d;
	     if (ibb->h < (bbh - bh + ibb->ibox->conf->iconsize))
	       {
		  ibb->h = bbh - bh + ibb->ibox->conf->iconsize;
	       }
	     else if (ibb->h > hh)
	       {
		  ibb->h = hh;
	       }
	     if ((ibb->y + ibb->h) > hh)
	       {
		  ibb->h = hh - ibb->y;
	       }
	  }
	_ibox_box_reconfigure(ibb);
	_ibox_box_follower_reset(ibb);
	_ibox_timer_handle(ibb);
	return;
     }
}

static int
_ibox_cb_event_container_resize(void *data, int type, void *event)
{
   IBox_Box *ibb;
   
   ibb = data;
   _ibox_box_frame_resize(ibb);
   return 1;
}

static int
_ibox_cb_event_border_iconify(void *data, int type, void *event)
{
   IBox     *ib;
   IBox_Box *ibb;
   E_App    *app;
   E_Event_Border_Hide *e;

   e = event;
   ib = data;
   ibb = ib->boxs->data;
   app = e_app_window_name_class_find(e->border->client.icccm.name,
				      e->border->client.icccm.class);
   if (app)
     {
	_ibox_box_icon_new(ibb, app, e->border);
	_ibox_box_convert_move_resize_to_config(ibb);
	_ibox_box_frame_resize(ibb);
     }
   else
     printf("FIXME WE HAVE NO ICON TO DISPLAY\n");
   return 1;
}

static int
_ibox_cb_event_border_uniconify(void *data, int type, void *event)
{
   IBox      *ib;
   IBox_Box  *ibb;
   E_Event_Border_Show *e;
   Evas_List *icons;
   IBox_Icon *icon;

   ib = data;
   ibb = ib->boxs->data;
   e = event;

   icons = ((IBox_Box *)ib->boxs->data)->icons;
   while (icons)
     {
	icon = icons->data;
	if (icon->border == e->border)
	  {
	     _ibox_box_icon_del(icon);
	     _ibox_box_convert_move_resize_to_config(ibb);
	     _ibox_box_frame_resize(ibb);
	     break;
	  }
	icons = icons->next;
     }
   return 1;
}

