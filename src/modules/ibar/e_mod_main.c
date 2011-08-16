#include "e.h"
#include "e_mod_main.h"

#ifndef MAX
# define MAX(x, y) (((x) > (y)) ? (x) : (y))
#endif

/* TODO:
 * - Track execution status
 */

/* gadcon requirements */
static E_Gadcon_Client *_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style);
static void _gc_shutdown(E_Gadcon_Client *gcc);
static void _gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient);
static char *_gc_label(E_Gadcon_Client_Class *client_class);
static Evas_Object *_gc_icon(E_Gadcon_Client_Class *client_class, Evas *evas);
static const char *_gc_id_new(E_Gadcon_Client_Class *client_class);
static void _gc_id_del(E_Gadcon_Client_Class *client_class, const char *id);

/* and actually define the gadcon class that this module provides (just 1) */
static const E_Gadcon_Client_Class _gadcon_class =
{
   GADCON_CLIENT_CLASS_VERSION,
     "ibar",
     {
        _gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, _gc_id_del,
        e_gadcon_site_is_not_toolbar
     },
   E_GADCON_CLIENT_STYLE_INSET
};

/* actual module specifics */
typedef struct _Instance  Instance;
typedef struct _IBar IBar;
typedef struct _IBar_Icon IBar_Icon;

struct _Instance
{
   E_Gadcon_Client *gcc;
   Evas_Object *o_ibar;
   IBar *ibar;
   E_Drop_Handler *drop_handler;
   Config_Item *ci;
   E_Gadcon_Orient orient;
};

struct _IBar
{
   Instance *inst;
   Evas_Object *o_box, *o_drop;
   Evas_Object *o_drop_over, *o_empty;
   IBar_Icon *ic_drop_before;
   int drop_before;
   E_Order *apps;
   Eina_List *icons;
   Evas_Coord dnd_x, dnd_y;
};

struct _IBar_Icon
{
   IBar *ibar;
   Evas_Object *o_holder, *o_icon;
   Evas_Object *o_holder2, *o_icon2;
   Efreet_Desktop *app;
   int mouse_down;
   struct 
     {
        unsigned char start : 1;
        unsigned char dnd : 1;
        int x, y;
     } drag;
};

static IBar *_ibar_new(Evas *evas, Instance *inst);
static void _ibar_free(IBar *b);
static void _ibar_cb_empty_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _ibar_empty_handle(IBar *b);
static void _ibar_fill(IBar *b);
static void _ibar_empty(IBar *b);
static void _ibar_orient_set(IBar *b, int horizontal);
static void _ibar_resize_handle(IBar *b);
static void _ibar_instance_drop_zone_recalc(Instance *inst);
static Config_Item *_ibar_config_item_get(const char *id);
static IBar_Icon *_ibar_icon_at_coord(IBar *b, Evas_Coord x, Evas_Coord y);
static IBar_Icon *_ibar_icon_new(IBar *b, Efreet_Desktop *desktop);
static void _ibar_icon_free(IBar_Icon *ic);
static void _ibar_icon_fill(IBar_Icon *ic);
static void _ibar_icon_empty(IBar_Icon *ic);
static void _ibar_icon_signal_emit(IBar_Icon *ic, char *sig, char *src);
static void _ibar_cb_app_change(void *data, E_Order *eo);
static void _ibar_cb_obj_moveresize(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _ibar_cb_menu_icon_new(void *data, E_Menu *m, E_Menu_Item *mi);
static void _ibar_cb_menu_icon_add(void *data, E_Menu *m, E_Menu_Item *mi);
static void _ibar_cb_menu_icon_properties(void *data, E_Menu *m, E_Menu_Item *mi);
static void _ibar_cb_menu_icon_remove(void *data, E_Menu *m, E_Menu_Item *mi);
static void _ibar_cb_menu_configuration(void *data, E_Menu *m, E_Menu_Item *mi);
static void _ibar_cb_menu_post(void *data, E_Menu *m);
static void _ibar_cb_icon_mouse_in(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _ibar_cb_icon_mouse_out(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _ibar_cb_icon_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _ibar_cb_icon_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _ibar_cb_icon_mouse_move(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _ibar_cb_icon_move(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _ibar_cb_icon_resize(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _ibar_inst_cb_enter(void *data, const char *type, void *event_info);
static void _ibar_inst_cb_move(void *data, const char *type, void *event_info);
static void _ibar_inst_cb_leave(void *data, const char *type, void *event_info);
static void _ibar_inst_cb_drop(void *data, const char *type, void *event_info);
static void _ibar_cb_drag_finished(E_Drag *data, int dropped);
static void _ibar_drop_position_update(Instance *inst, Evas_Coord x, Evas_Coord y);
static void _ibar_inst_cb_scroll(void *data);
static Eina_Bool  _ibar_cb_config_icons(void *data, int ev_type, void *ev);

static E_Config_DD *conf_edd = NULL;
static E_Config_DD *conf_item_edd = NULL;

static int uuid = 0;

Config *ibar_config = NULL;

static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style)
{
   IBar *b;
   Evas_Object *o;
   E_Gadcon_Client *gcc;
   Instance *inst;
   Evas_Coord x, y, w, h;
   const char *drop[] = { "enlightenment/desktop", "enlightenment/border", "text/uri-list" };
   Config_Item *ci;

   inst = E_NEW(Instance, 1);

   ci = _ibar_config_item_get(id);
   inst->ci = ci;
   if (!ci->dir) ci->dir = eina_stringshare_add("default");
   b = _ibar_new(gc->evas, inst);
   o = b->o_box;
   gcc = e_gadcon_client_new(gc, name, id, style, o);
   gcc->data = inst;

   inst->gcc = gcc;
   inst->o_ibar = o;
   inst->orient = E_GADCON_ORIENT_HORIZ;

   evas_object_geometry_get(o, &x, &y, &w, &h);
   inst->drop_handler =
     e_drop_handler_add(E_OBJECT(inst->gcc), inst,
			_ibar_inst_cb_enter, _ibar_inst_cb_move,
			_ibar_inst_cb_leave, _ibar_inst_cb_drop,
			drop, 3, x, y, w,  h);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOVE,
				  _ibar_cb_obj_moveresize, inst);
   evas_object_event_callback_add(o, EVAS_CALLBACK_RESIZE,
				  _ibar_cb_obj_moveresize, inst);
   ibar_config->instances = eina_list_append(ibar_config->instances, inst);
   return gcc;
}

static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
   Instance *inst;

   inst = gcc->data;
   ibar_config->instances = eina_list_remove(ibar_config->instances, inst);
   e_drop_handler_del(inst->drop_handler);
   _ibar_free(inst->ibar);
   E_FREE(inst);
}

static void
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient)
{
   Instance *inst;

   inst = gcc->data;
   if ((int)orient != -1) inst->orient = orient;

   switch (inst->orient)
     {
      case E_GADCON_ORIENT_FLOAT:
      case E_GADCON_ORIENT_HORIZ:
      case E_GADCON_ORIENT_TOP:
      case E_GADCON_ORIENT_BOTTOM:
      case E_GADCON_ORIENT_CORNER_TL:
      case E_GADCON_ORIENT_CORNER_TR:
      case E_GADCON_ORIENT_CORNER_BL:
      case E_GADCON_ORIENT_CORNER_BR:
	_ibar_orient_set(inst->ibar, 1);
	e_gadcon_client_aspect_set(gcc, eina_list_count(inst->ibar->icons) * 16, 16);
	break;
      case E_GADCON_ORIENT_VERT:
      case E_GADCON_ORIENT_LEFT:
      case E_GADCON_ORIENT_RIGHT:
      case E_GADCON_ORIENT_CORNER_LT:
      case E_GADCON_ORIENT_CORNER_RT:
      case E_GADCON_ORIENT_CORNER_LB:
      case E_GADCON_ORIENT_CORNER_RB:
	_ibar_orient_set(inst->ibar, 0);
	e_gadcon_client_aspect_set(gcc, 16, eina_list_count(inst->ibar->icons) * 16);
	break;
      default:
	break;
     }
   e_gadcon_client_min_size_set(gcc, 16, 16);
}

static char *
_gc_label(E_Gadcon_Client_Class *client_class __UNUSED__)
{
   return _("IBar");
}

static Evas_Object *
_gc_icon(E_Gadcon_Client_Class *client_class __UNUSED__, Evas *evas)
{
   Evas_Object *o;
   char buf[PATH_MAX];

   o = edje_object_add(evas);
   snprintf(buf, sizeof(buf), "%s/e-module-ibar.edj",
	    e_module_dir_get(ibar_config->module));
   edje_object_file_set(o, buf, "icon");
   return o;
}

static const char *
_gc_id_new(E_Gadcon_Client_Class *client_class __UNUSED__)
{
   Config_Item *ci;

   ci = _ibar_config_item_get(NULL);
   return ci->id;
}

static void
_gc_id_del(E_Gadcon_Client_Class *client_class __UNUSED__, const char *id __UNUSED__)
{
/* yes - don't do this. on shutdown gadgets are deleted and this means config
 * for them is deleted - that means empty config is saved. keep them around
 * as if u add a gadget back it can pick up its old config again
 * 
   Config_Item *ci;

   ci = _ibar_config_item_get(id);
   if (ci)
     {
	if (ci->id) eina_stringshare_del(ci->id);
	ibar_config->items = eina_list_remove(ibar_config->items, ci);
     }
 */
}

static IBar *
_ibar_new(Evas *evas, Instance *inst)
{
   IBar *b;
   char buf[PATH_MAX];

   b = E_NEW(IBar, 1);
   inst->ibar = b;
   b->inst = inst;
   b->o_box = e_box_add(evas);
   e_box_homogenous_set(b->o_box, 1);
   e_box_orientation_set(b->o_box, 1);
   e_box_align_set(b->o_box, 0.5, 0.5);
   if (inst->ci->dir[0] != '/')
     e_user_dir_snprintf(buf, sizeof(buf), "applications/bar/%s/.order", 
                         inst->ci->dir);
   else
     eina_strlcpy(buf, inst->ci->dir, sizeof(buf));
   b->apps = e_order_new(buf);
   e_order_update_callback_set(b->apps, _ibar_cb_app_change, b);
   _ibar_fill(b);
   return b;
}

static void
_ibar_free(IBar *b)
{
   _ibar_empty(b);
   evas_object_del(b->o_box);
   if (b->o_drop) evas_object_del(b->o_drop);
   if (b->o_drop_over) evas_object_del(b->o_drop_over);
   if (b->o_empty) evas_object_del(b->o_empty);
   e_order_update_callback_set(b->apps, NULL, NULL);
   if (b->apps) e_object_del(E_OBJECT(b->apps));
   E_FREE(b);
}

static void
_ibar_cb_empty_mouse_down(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   IBar *b;

   ev = event_info;
   b = data;
   if (!ibar_config->menu)
     {
	E_Menu *m;
	E_Menu_Item *mi;
	int cx, cy, cw, ch;

	m = e_menu_new();
	mi = e_menu_item_new(m);
	e_menu_item_label_set(mi, _("Settings"));
	e_util_menu_item_theme_icon_set(mi, "configure");
	e_menu_item_callback_set(mi, _ibar_cb_menu_configuration, b);

	m = e_gadcon_client_util_menu_items_append(b->inst->gcc, m, 0);
	e_menu_post_deactivate_callback_set(m, _ibar_cb_menu_post, NULL);
	ibar_config->menu = m;

	e_gadcon_canvas_zone_geometry_get(b->inst->gcc->gadcon,
					  &cx, &cy, &cw, &ch);
	e_menu_activate_mouse(m,
			      e_util_zone_current_get(e_manager_current_get()),
			      cx + ev->output.x, cy + ev->output.y, 1, 1,
			      E_MENU_POP_DIRECTION_DOWN, ev->timestamp);
     }
}

static void
_ibar_empty_handle(IBar *b)
{
   if (!b->icons)
     {
	if (!b->o_empty)
	  {
	     Evas_Coord w, h;

	     b->o_empty = evas_object_rectangle_add(evas_object_evas_get(b->o_box));
	     evas_object_event_callback_add(b->o_empty, 
                                            EVAS_CALLBACK_MOUSE_DOWN, 
                                            _ibar_cb_empty_mouse_down, b);
	     evas_object_color_set(b->o_empty, 0, 0, 0, 0);
	     evas_object_show(b->o_empty);
	     e_box_pack_end(b->o_box, b->o_empty);
	     evas_object_geometry_get(b->o_box, NULL, NULL, &w, &h);
	     if (e_box_orientation_get(b->o_box))
	       w = h;
	     else
	       h = w;
	     e_box_pack_options_set(b->o_empty,
				    1, 1, /* fill */
				    1, 1, /* expand */
				    0.5, 0.5, /* align */
				    w, h, /* min */
				    9999, 9999 /* max */
				    );
	  }
     }
   else if (b->o_empty)
     {
	evas_object_del(b->o_empty);
	b->o_empty = NULL;
     }
}

static void
_ibar_fill(IBar *b)
{
   if (b->apps)
     {
	Efreet_Desktop *desktop;
	const Eina_List *l;

	EINA_LIST_FOREACH(b->apps->desktops, l, desktop)
	  {
	     IBar_Icon *ic = _ibar_icon_new(b, desktop);
	     b->icons = eina_list_append(b->icons, ic);
	     e_box_pack_end(b->o_box, ic->o_holder);
	  }
     }
   _ibar_empty_handle(b);
   _ibar_resize_handle(b);
}

static void
_ibar_empty(IBar *b)
{
   IBar_Icon *ic;

   EINA_LIST_FREE(b->icons, ic)
     _ibar_icon_free(ic);

   _ibar_empty_handle(b);
}

static void
_ibar_orient_set(IBar *b, int horizontal)
{
   e_box_orientation_set(b->o_box, horizontal);
   e_box_align_set(b->o_box, 0.5, 0.5);
}

static void
_ibar_resize_handle(IBar *b)
{
   const Eina_List *l;
   IBar_Icon *ic;
   Evas_Coord w, h;

   evas_object_geometry_get(b->o_box, NULL, NULL, &w, &h);
   if (e_box_orientation_get(b->o_box))
     w = h;
   else
     h = w;
   e_box_freeze(b->o_box);
   EINA_LIST_FOREACH(b->icons, l, ic)
     {
	e_box_pack_options_set(ic->o_holder,
			       1, 1, /* fill */
			       0, 0, /* expand */
			       0.5, 0.5, /* align */
			       w, h, /* min */
			       w, h /* max */
			       );
     }
   e_box_thaw(b->o_box);
}

static void
_ibar_instance_drop_zone_recalc(Instance *inst)
{
   Evas_Coord x, y, w, h;

   e_gadcon_client_viewport_geometry_get(inst->gcc, &x, &y, &w, &h);
   e_drop_handler_geometry_set(inst->drop_handler, x, y, w, h);
}  

static Config_Item *
_ibar_config_item_get(const char *id)
{
   Config_Item *ci;
   char buf[128];

   if (!id)
     {
	snprintf(buf, sizeof(buf), "%s.%d", _gadcon_class.name, ++uuid);
	id = buf;
     }
   else
     {
	const Eina_List *l;

	/* Find old config, or reuse supplied id */
	EINA_LIST_FOREACH(ibar_config->items, l, ci)
	  {
	     if ((ci->id) && (!strcmp(ci->id, id)))
	       {
		  if (!ci->dir) ci->dir = eina_stringshare_add("default");
		  return ci;
	       }
	  }
     }
   ci = E_NEW(Config_Item, 1);
   ci->id = eina_stringshare_add(id);
   ci->dir = eina_stringshare_add("default");
   ci->show_label = 1;
   ci->eap_label = 0;
   ibar_config->items = eina_list_append(ibar_config->items, ci);
   return ci;
}

void
_ibar_config_update(Config_Item *ci)
{
   const Eina_List *l;
   Instance *inst;

   EINA_LIST_FOREACH(ibar_config->instances, l, inst)
     {
	char buf[PATH_MAX];
	const Eina_List *i;
	IBar_Icon *ic;

	if (inst->ci != ci) continue;

	_ibar_empty(inst->ibar);
	if (inst->ibar->apps)
	  e_object_del(E_OBJECT(inst->ibar->apps));
	if (inst->ci->dir[0] != '/')
          e_user_dir_snprintf(buf, sizeof(buf), "applications/bar/%s/.order", 
                              inst->ci->dir);
	else
	  eina_strlcpy(buf, inst->ci->dir, sizeof(buf));
	inst->ibar->apps = e_order_new(buf);
	_ibar_fill(inst->ibar);
	_ibar_resize_handle(inst->ibar);
	_gc_orient(inst->gcc, -1);

	EINA_LIST_FOREACH(inst->ibar->icons, i, ic)
	  {
	     switch (ci->eap_label)
	       {
		case 0:
		  edje_object_part_text_set(ic->o_holder2, "e.text.label", 
                                            ic->app->name);
		  break;
		case 1:
		  edje_object_part_text_set(ic->o_holder2, "e.text.label", 
                                            ic->app->comment);
		  break;
		case 2:
		  edje_object_part_text_set(ic->o_holder2, "e.text.label", 
                                            ic->app->generic_name);
		  break;
	       }
	  }
     }
}

static IBar_Icon *
_ibar_icon_at_coord(IBar *b, Evas_Coord x, Evas_Coord y)
{
   const Eina_List *l;
   IBar_Icon *ic;

   EINA_LIST_FOREACH(b->icons, l, ic)
     {
        Evas_Coord dx, dy, dw, dh;

        evas_object_geometry_get(ic->o_holder, &dx, &dy, &dw, &dh);
	if (E_INSIDE(x, y, dx, dy, dw, dh)) return ic;
     }
   return NULL;
}

static IBar_Icon *
_ibar_icon_new(IBar *b, Efreet_Desktop *desktop)
{
   IBar_Icon *ic;

   ic = E_NEW(IBar_Icon, 1);
   ic->ibar = b;
   ic->app = desktop;
   ic->o_holder = edje_object_add(evas_object_evas_get(b->o_box));
   e_theme_edje_object_set(ic->o_holder, "base/theme/modules/ibar",
			   "e/modules/ibar/icon");
   evas_object_event_callback_add(ic->o_holder, EVAS_CALLBACK_MOUSE_IN,  
                                  _ibar_cb_icon_mouse_in,  ic);
   evas_object_event_callback_add(ic->o_holder, EVAS_CALLBACK_MOUSE_OUT, 
                                  _ibar_cb_icon_mouse_out, ic);
   evas_object_event_callback_add(ic->o_holder, EVAS_CALLBACK_MOUSE_DOWN, 
                                  _ibar_cb_icon_mouse_down, ic);
   evas_object_event_callback_add(ic->o_holder, EVAS_CALLBACK_MOUSE_UP, 
                                  _ibar_cb_icon_mouse_up, ic);
   evas_object_event_callback_add(ic->o_holder, EVAS_CALLBACK_MOUSE_MOVE, 
                                  _ibar_cb_icon_mouse_move, ic);
   evas_object_event_callback_add(ic->o_holder, EVAS_CALLBACK_MOVE, 
                                  _ibar_cb_icon_move, ic);
   evas_object_event_callback_add(ic->o_holder, EVAS_CALLBACK_RESIZE, 
                                  _ibar_cb_icon_resize, ic);
   evas_object_show(ic->o_holder);

   ic->o_holder2 = edje_object_add(evas_object_evas_get(b->o_box));
   e_theme_edje_object_set(ic->o_holder2, "base/theme/modules/ibar",
			   "e/modules/ibar/icon_overlay");
   evas_object_layer_set(ic->o_holder2, 9999);
   evas_object_pass_events_set(ic->o_holder2, 1);
   evas_object_show(ic->o_holder2);

   _ibar_icon_fill(ic);
   return ic;
}

static void
_ibar_icon_free(IBar_Icon *ic)
{
   if (ibar_config->menu)
     {
	e_menu_post_deactivate_callback_set(ibar_config->menu, NULL, NULL);
	e_object_del(E_OBJECT(ibar_config->menu));
	ibar_config->menu = NULL;
     }
   if (ic->ibar->ic_drop_before == ic)
     ic->ibar->ic_drop_before = NULL;
   _ibar_icon_empty(ic);
   evas_object_del(ic->o_holder);
   evas_object_del(ic->o_holder2);
   E_FREE(ic);
}

static void
_ibar_icon_fill(IBar_Icon *ic)
{
   if (ic->o_icon) evas_object_del(ic->o_icon);
   ic->o_icon = e_icon_add(evas_object_evas_get(ic->ibar->o_box));
   e_icon_fdo_icon_set(ic->o_icon, ic->app->icon);
   edje_object_part_swallow(ic->o_holder, "e.swallow.content", ic->o_icon);
   evas_object_pass_events_set(ic->o_icon, 1);
   evas_object_show(ic->o_icon);
   if (ic->o_icon2) evas_object_del(ic->o_icon2);
   ic->o_icon2 = e_icon_add(evas_object_evas_get(ic->ibar->o_box));
   e_icon_fdo_icon_set(ic->o_icon2, ic->app->icon);
   edje_object_part_swallow(ic->o_holder2, "e.swallow.content", ic->o_icon2);
   evas_object_pass_events_set(ic->o_icon2, 1);
   evas_object_show(ic->o_icon2);

   switch (ic->ibar->inst->ci->eap_label) 
     {
      case 0: /* Eap Name */
	edje_object_part_text_set(ic->o_holder2, "e.text.label", ic->app->name);
	break;
      case 1: /* Eap Comment */
	edje_object_part_text_set(ic->o_holder2, "e.text.label", ic->app->comment);
	break;	
      case 2: /* Eap Generic */
	edje_object_part_text_set(ic->o_holder2, "e.text.label", ic->app->generic_name);
	break;	
     }
}

static void
_ibar_icon_empty(IBar_Icon *ic)
{
   if (ic->o_icon) evas_object_del(ic->o_icon);
   if (ic->o_icon2) evas_object_del(ic->o_icon2);
   ic->o_icon = NULL;
   ic->o_icon2 = NULL;
}

static void
_ibar_icon_signal_emit(IBar_Icon *ic, char *sig, char *src)
{
   if (ic->o_holder)
     edje_object_signal_emit(ic->o_holder, sig, src);
   if (ic->o_icon)
     edje_object_signal_emit(ic->o_icon, sig, src);
   if (ic->o_holder2)
     edje_object_signal_emit(ic->o_holder2, sig, src);
   if (ic->o_icon2)
     edje_object_signal_emit(ic->o_icon2, sig, src);
}

static void
_ibar_cb_app_change(void *data, E_Order *eo __UNUSED__)
{
   IBar *b;

   b = data;
   if (!b->apps) return;
   _ibar_empty(b);
   _ibar_fill(b);
   _ibar_resize_handle(b);
   if (b->inst) _gc_orient(b->inst->gcc, -1);
}

static void
_ibar_cb_obj_moveresize(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Instance *inst;

   inst = data;
   _ibar_resize_handle(inst->ibar);
   _ibar_instance_drop_zone_recalc(inst);
}

static void 
_ibar_cb_menu_icon_new(void *data __UNUSED__, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__) 
{
   E_Container *con;

   if (!e_configure_registry_exists("applications/new_application")) return;
   con = e_container_current_get(e_manager_current_get());
   e_configure_registry_call("applications/new_application", con, NULL);
}

static void 
_ibar_cb_menu_icon_add(void *data __UNUSED__, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__) 
{
   E_Container *con;

   if (!e_configure_registry_exists("applications/ibar_applications")) return;
   con = e_container_current_get(e_manager_current_get());
   e_configure_registry_call("applications/ibar_applications", con, NULL);
}

static void
_ibar_cb_menu_icon_properties(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   IBar_Icon *ic;

   ic = data;
   e_desktop_edit(ic->ibar->inst->gcc->gadcon->zone->container, ic->app);
}

static void
_ibar_cb_menu_icon_remove(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   IBar_Icon *ic;
   E_Gadcon_Client *gc;

   ic = data;
   ic->ibar->icons = eina_list_remove(ic->ibar->icons, ic);
   _ibar_resize_handle(ic->ibar);
   gc = ic->ibar->inst->gcc;
   _gc_orient(gc, -1);
   e_order_remove(ic->ibar->apps, ic->app);
   _ibar_icon_free(ic);
}

static void
_ibar_cb_menu_configuration(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   IBar *b;

   b = data;
   _config_ibar_module(b->inst->ci);
}

/*
static void
_ibar_cb_menu_add(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   IBar *b;

   b = data;
   e_configure_registry_call("internal/ibar_other",
			     b->inst->gcc->gadcon->zone->container,
			     b->apps->path);
}
*/

static void
_ibar_cb_menu_post(void *data __UNUSED__, E_Menu *m __UNUSED__)
{
   if (!ibar_config->menu) return;
   e_object_del(E_OBJECT(ibar_config->menu));
   ibar_config->menu = NULL;
}

static void
_ibar_cb_icon_mouse_in(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   IBar_Icon *ic;

   ic = data;
   _ibar_icon_signal_emit(ic, "e,state,focused", "e");
   if (ic->ibar->inst->ci->show_label)
     _ibar_icon_signal_emit(ic, "e,action,show,label", "e");
}

static void
_ibar_cb_icon_mouse_out(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   IBar_Icon *ic;

   ic = data;
   _ibar_icon_signal_emit(ic, "e,state,unfocused", "e");
   if (ic->ibar->inst->ci->show_label)
     _ibar_icon_signal_emit(ic, "e,action,hide,label", "e");
}

static void
_ibar_cb_icon_mouse_down(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   IBar_Icon *ic;

   ev = event_info;
   ic = data;
   if (ev->button == 1)
     {
	ic->drag.x = ev->output.x;
	ic->drag.y = ev->output.y;
	ic->drag.start = 1;
	ic->drag.dnd = 0;
	ic->mouse_down = 1;
     }
   else if ((ev->button == 3) && (!ibar_config->menu))
     {
	E_Menu *m, *mo;
	E_Menu_Item *mi;
        char buf[256];
	int cx, cy;

	m = e_menu_new();

	/* FIXME: other icon options go here too */
	mo = e_menu_new();

	if (e_configure_registry_exists("applications/new_application")) 
	  {
	     mi = e_menu_item_new(m);
	     e_menu_item_label_set(mi, _("Create new Icon"));
	     e_util_menu_item_theme_icon_set(mi, "document-new");
	     e_menu_item_callback_set(mi, _ibar_cb_menu_icon_new, NULL);

             mi = e_menu_item_new(m);
             e_menu_item_separator_set(mi, 1);
	  }

	if (e_configure_registry_exists("applications/ibar_applications")) 
	  {
	     mi = e_menu_item_new(m);
	     e_menu_item_label_set(mi, _("Contents"));
	     e_util_menu_item_theme_icon_set(mi, "list-add");
	     e_menu_item_callback_set(mi, _ibar_cb_menu_icon_add, NULL);
	  }

	mi = e_menu_item_new(m);
	e_menu_item_label_set(mi, _("Settings"));
	e_util_menu_item_theme_icon_set(mi, "configure");
	e_menu_item_callback_set(mi, _ibar_cb_menu_configuration, ic->ibar);

	m = e_gadcon_client_util_menu_items_append(ic->ibar->inst->gcc, m, 0);
	e_menu_post_deactivate_callback_set(m, _ibar_cb_menu_post, NULL);
	ibar_config->menu = m;

	mi = e_menu_item_new(mo);
	e_menu_item_label_set(mi, _("Properties"));
	e_util_menu_item_theme_icon_set(mi, "configure");
	e_menu_item_callback_set(mi, _ibar_cb_menu_icon_properties, ic);

	mi = e_menu_item_new(mo);
	e_menu_item_label_set(mi, _("Remove"));
	e_util_menu_item_theme_icon_set(mi, "list-remove");
	e_menu_item_callback_set(mi, _ibar_cb_menu_icon_remove, ic);

        mi = e_menu_item_new_relative(m, NULL);
        snprintf(buf, sizeof(buf), "Icon %s", ic->app->name);
        e_menu_item_label_set(mi, _(buf));
        e_util_desktop_menu_item_icon_add(ic->app, 
                                          e_util_icon_size_normalize(24 * e_scale), 
                                          mi);
        e_menu_item_submenu_set(mi, mo);

	e_gadcon_canvas_zone_geometry_get(ic->ibar->inst->gcc->gadcon,
					  &cx, &cy, NULL, NULL);
	e_menu_activate_mouse(m,
			      e_util_zone_current_get(e_manager_current_get()),
			      cx + ev->output.x, cy + ev->output.y, 1, 1,
			      E_MENU_POP_DIRECTION_DOWN, ev->timestamp);
     }
}

static void
_ibar_cb_icon_mouse_up(void *data, Evas *e __UNUSED__, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Up *ev;
   IBar_Icon *ic;

   ev = event_info;
   ic = data;
   if ((ev->button == 1) && (!ic->drag.dnd) && (ic->mouse_down == 1))
     {
	if (ic->app->type == EFREET_DESKTOP_TYPE_APPLICATION)
	  e_exec(ic->ibar->inst->gcc->gadcon->zone, ic->app, NULL, NULL, "ibar");
	else if (ic->app->type == EFREET_DESKTOP_TYPE_LINK)
	  {
	     if (!strncasecmp(ic->app->url, "file:", 5))
	       {
		  E_Action *act;

		  act = e_action_find("fileman");
		  if (act) act->func.go(E_OBJECT(obj), ic->app->url + 5);
	       }
	  }

	ic->drag.start = 0;
	ic->drag.dnd = 0;
	ic->mouse_down = 0;
	/* TODO: bring back "e,action,start|stop" for the startup_notify apps
	 *       when startup_notify is used again
	 */
	_ibar_icon_signal_emit(ic, "e,action,exec", "e");
     }
}

static void
_ibar_cb_icon_mouse_move(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Evas_Event_Mouse_Move *ev;
   IBar_Icon *ic;

   ev = event_info;
   ic = data;
   if (ic->drag.start)
     {
	int dx, dy;

	dx = ev->cur.output.x - ic->drag.x;
	dy = ev->cur.output.y - ic->drag.y;
	if (((dx * dx) + (dy * dy)) >
	    (e_config->drag_resist * e_config->drag_resist))
	  {
	     E_Drag *d;
	     Evas_Object *o;
	     Evas_Coord x, y, w, h;
	     unsigned int size;
	     const char *drag_types[] = { "enlightenment/desktop" };
	     E_Gadcon_Client *gc;

	     ic->drag.dnd = 1;
	     ic->drag.start = 0;

	     evas_object_geometry_get(ic->o_icon, &x, &y, &w, &h);
	     d = e_drag_new(ic->ibar->inst->gcc->gadcon->zone->container,
			    x, y, drag_types, 1,
			    ic->app, -1, NULL, _ibar_cb_drag_finished);
             efreet_desktop_ref(ic->app);
	     size = MAX(w, h);
             o = e_util_desktop_icon_add(ic->app, size, e_drag_evas_get(d));
	     e_drag_object_set(d, o);

	     e_drag_resize(d, w, h);
	     e_drag_start(d, ic->drag.x, ic->drag.y);
	     ic->ibar->icons = eina_list_remove(ic->ibar->icons, ic);
	     _ibar_resize_handle(ic->ibar);
             gc = ic->ibar->inst->gcc;
	     _gc_orient(gc, -1);
	     e_order_remove(ic->ibar->apps, ic->app);
	     _ibar_icon_free(ic);
	  }
     }
}

static void
_ibar_cb_icon_move(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   IBar_Icon *ic;
   Evas_Coord x, y;

   ic = data;
   evas_object_geometry_get(ic->o_holder, &x, &y, NULL, NULL);
   evas_object_move(ic->o_holder2, x, y);
   evas_object_raise(ic->o_holder2);
}

static void
_ibar_cb_icon_resize(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   IBar_Icon *ic;
   Evas_Coord w, h;

   ic = data;
   evas_object_geometry_get(ic->o_holder, NULL, NULL, &w, &h);
   evas_object_resize(ic->o_holder2, w, h);
   evas_object_raise(ic->o_holder2);
}

static void
_ibar_cb_drop_move(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   IBar *b;
   Evas_Coord x, y;

   b = data;
   evas_object_geometry_get(b->o_drop, &x, &y, NULL, NULL);
   evas_object_move(b->o_drop_over, x, y);
}

static void
_ibar_cb_drop_resize(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   IBar *b;
   Evas_Coord w, h;

   b = data;
   evas_object_geometry_get(b->o_drop, NULL, NULL, &w, &h);
   evas_object_resize(b->o_drop_over, w, h);
}

static void
_ibar_cb_drag_finished(E_Drag *drag, int dropped __UNUSED__)
{
   efreet_desktop_unref(drag->data);
}

static void
_ibar_inst_cb_scroll(void *data)
{
   Instance *inst;

   /* Update the position of the dnd to handle for autoscrolling
    * gadgets. */
   inst = data;
   _ibar_drop_position_update(inst, inst->ibar->dnd_x, inst->ibar->dnd_y);
}

static void
_ibar_drop_position_update(Instance *inst, Evas_Coord x, Evas_Coord y)
{
   IBar_Icon *ic;

   inst->ibar->dnd_x = x;
   inst->ibar->dnd_y = y;

   if (inst->ibar->o_drop) e_box_unpack(inst->ibar->o_drop);
   ic = _ibar_icon_at_coord(inst->ibar, x, y);

   inst->ibar->ic_drop_before = ic;
   if (ic)
     {
	Evas_Coord ix, iy, iw, ih;
	int before = 0;

	evas_object_geometry_get(ic->o_holder, &ix, &iy, &iw, &ih);
	if (e_box_orientation_get(inst->ibar->o_box))
	  {
	     if (x < (ix + (iw / 2))) before = 1;
	  }
	else
	  {
	     if (y < (iy + (ih / 2))) before = 1;
	  }
	if (before)
	  e_box_pack_before(inst->ibar->o_box, inst->ibar->o_drop, ic->o_holder);
	else
	  e_box_pack_after(inst->ibar->o_box, inst->ibar->o_drop, ic->o_holder);
	inst->ibar->drop_before = before;
     }
   else e_box_pack_end(inst->ibar->o_box, inst->ibar->o_drop);
   e_box_pack_options_set(inst->ibar->o_drop,
			  1, 1, /* fill */
			  1, 1, /* expand */
			  0.5, 0.5, /* align */
			  1, 1, /* min */
			  -1, -1 /* max */
			  );
   _ibar_resize_handle(inst->ibar);
   _gc_orient(inst->gcc, -1);
}

static void
_ibar_inst_cb_enter(void *data, const char *type __UNUSED__, void *event_info)
{
   E_Event_Dnd_Enter *ev;
   Instance *inst;
   Evas_Object *o, *o2;

   ev = event_info;
   inst = data;
   o = edje_object_add(evas_object_evas_get(inst->ibar->o_box));
   inst->ibar->o_drop = o;
   o2 = edje_object_add(evas_object_evas_get(inst->ibar->o_box));
   inst->ibar->o_drop_over = o2;
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOVE, 
                                  _ibar_cb_drop_move, inst->ibar);
   evas_object_event_callback_add(o, EVAS_CALLBACK_RESIZE, 
                                  _ibar_cb_drop_resize, inst->ibar);
   e_theme_edje_object_set(o, "base/theme/modules/ibar",
			   "e/modules/ibar/drop");
   e_theme_edje_object_set(o2, "base/theme/modules/ibar",
			   "e/modules/ibar/drop_overlay");
   evas_object_layer_set(o2, 19999);
   evas_object_show(o);
   evas_object_show(o2);

   _ibar_drop_position_update(inst, ev->x, ev->y);
   e_gadcon_client_autoscroll_cb_set(inst->gcc, _ibar_inst_cb_scroll, inst);
   e_gadcon_client_autoscroll_update(inst->gcc, ev->x, ev->y);
}

static void
_ibar_inst_cb_move(void *data, const char *type __UNUSED__, void *event_info)
{
   E_Event_Dnd_Move *ev;
   Instance *inst;

   ev = event_info;
   inst = data;
   _ibar_drop_position_update(inst, ev->x, ev->y);
   e_gadcon_client_autoscroll_update(inst->gcc, ev->x, ev->y);
}

static void
_ibar_inst_cb_leave(void *data, const char *type __UNUSED__, void *event_info __UNUSED__)
{
   Instance *inst;

   inst = data;
   inst->ibar->ic_drop_before = NULL;
   evas_object_del(inst->ibar->o_drop);
   inst->ibar->o_drop = NULL;
   evas_object_del(inst->ibar->o_drop_over);
   inst->ibar->o_drop_over = NULL;
   _ibar_resize_handle(inst->ibar);
   e_gadcon_client_autoscroll_cb_set(inst->gcc, NULL, NULL);
   _gc_orient(inst->gcc, -1);
}

static void
_ibar_inst_cb_drop(void *data, const char *type, void *event_info)
{
   E_Event_Dnd_Drop *ev;
   Instance *inst;
   Efreet_Desktop *app = NULL;
   Eina_List *fl = NULL;
   IBar_Icon *ic;

   ev = event_info;
   inst = data;

   if (!strcmp(type, "enlightenment/desktop"))
     app = ev->data;
   else if (!strcmp(type, "enlightenment/border"))
     {
	E_Border *bd;

	bd = ev->data;
	app = bd->desktop;
	if (!app)
	  {
	     app = e_desktop_border_create(bd);
	     efreet_desktop_save(app);
	     e_desktop_edit(e_container_current_get(e_manager_current_get()), 
                            app);
	  }
     }
   else if (!strcmp(type, "text/uri-list"))
     fl = ev->data;

   ic = inst->ibar->ic_drop_before;
   if (ic)
     {
	/* Add new eapp before this icon */
	if (!inst->ibar->drop_before)
	  {
	     const Eina_List *l;
	     IBar_Icon *ic2;

	     EINA_LIST_FOREACH(inst->ibar->icons, l, ic2)
	       {
		  if (ic2 == ic)
		    {
		       if (l->next)
			 ic = l->next->data;
		       else
			 ic = NULL;
		       break;
		    }
	       }
	  }
	if (!ic) goto atend;
	if (app)
	  e_order_prepend_relative(ic->ibar->apps, app, ic->app);
	else if (fl)
	  e_order_files_prepend_relative(ic->ibar->apps, fl, ic->app);
     }
   else
     {
	atend:
	if (inst->ibar->apps)
	  {
	     if (app)
	       e_order_append(inst->ibar->apps, app);
	     else if (fl)
	       e_order_files_append(inst->ibar->apps, fl);
	  }
     }
   evas_object_del(inst->ibar->o_drop);
   inst->ibar->o_drop = NULL;
   evas_object_del(inst->ibar->o_drop_over);
   inst->ibar->o_drop_over = NULL;
   e_gadcon_client_autoscroll_cb_set(inst->gcc, NULL, NULL);
   _ibar_empty_handle(inst->ibar);
   _ibar_resize_handle(inst->ibar);
   _gc_orient(inst->gcc, -1);
}

/* module setup */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION, "IBar"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   conf_item_edd = E_CONFIG_DD_NEW("IBar_Config_Item", Config_Item);
#undef T
#undef D
#define T Config_Item
#define D conf_item_edd
   E_CONFIG_VAL(D, T, id, STR);
   E_CONFIG_VAL(D, T, dir, STR);
   E_CONFIG_VAL(D, T, show_label, INT);
   E_CONFIG_VAL(D, T, eap_label, INT);

   conf_edd = E_CONFIG_DD_NEW("IBar_Config", Config);
#undef T
#undef D
#define T Config
#define D conf_edd
   E_CONFIG_LIST(D, T, items, conf_item_edd);

   ibar_config = e_config_domain_load("module.ibar", conf_edd);

   if (!ibar_config)
     {
	Config_Item *ci;

	ibar_config = E_NEW(Config, 1);

	ci = E_NEW(Config_Item, 1);
	ci->id = eina_stringshare_add("ibar.1");
	ci->dir = eina_stringshare_add("default");
	ci->show_label = 1;
	ci->eap_label = 0;
	ibar_config->items = eina_list_append(ibar_config->items, ci);
     }
   else
     {
	Eina_List *removes = NULL;
	const Eina_List *l;
	Config_Item *ci;

	EINA_LIST_FOREACH(ibar_config->items, l, ci)
	  {
	     if (!ci->id)
	       removes = eina_list_append(removes, ci);
	     else if (!ci->dir)
	       removes = eina_list_append(removes, ci);
	     else
	       {
		  const Eina_List *ll;
		  Config_Item *ci2;

		  EINA_LIST_FOREACH(l->next, ll, ci2)
		    {
		       if ((ci2->id) && (!strcmp(ci->id, ci2->id)))
			 {
			    removes = eina_list_append(removes, ci);
			    break;
			 }
		    }
	       }
	  }
	EINA_LIST_FREE(removes, ci)
	  {
	     ibar_config->items = eina_list_remove(ibar_config->items, ci);
	     if (ci->id) eina_stringshare_del(ci->id);
	     if (ci->dir) eina_stringshare_del(ci->dir);
	     E_FREE(ci);
          }
	EINA_LIST_FOREACH(ibar_config->items, l, ci)
          {
	     if (ci->id)
	       {
		  const char *p;

		  p = strrchr(ci->id, '.');
		  if (p)
		    {
		       int id;

		       id = atoi(p + 1);
		       if (id > uuid) uuid = id;
		    }
	       }
	  }
     }
 
   ibar_config->module = m;

   ibar_config->handlers = 
     eina_list_append(ibar_config->handlers,
                      ecore_event_handler_add(E_EVENT_CONFIG_ICON_THEME, 
                                              _ibar_cb_config_icons, NULL));
   ibar_config->handlers = 
     eina_list_append(ibar_config->handlers,
                      ecore_event_handler_add(EFREET_EVENT_ICON_CACHE_UPDATE,
                                              _ibar_cb_config_icons, NULL));

   e_gadcon_provider_register(&_gadcon_class);
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   Ecore_Event_Handler *eh;
   Config_Item *ci;

   e_gadcon_provider_unregister(&_gadcon_class);

   if (ibar_config->config_dialog)
     e_object_del(E_OBJECT(ibar_config->config_dialog));

   EINA_LIST_FREE(ibar_config->handlers, eh)
     ecore_event_handler_del(eh);

   if (ibar_config->menu)
     {
	e_menu_post_deactivate_callback_set(ibar_config->menu, NULL, NULL);
	e_object_del(E_OBJECT(ibar_config->menu));
	ibar_config->menu = NULL;
     }

   EINA_LIST_FREE(ibar_config->items, ci)
     {
	if (ci->id) eina_stringshare_del(ci->id);
	if (ci->dir) eina_stringshare_del(ci->dir);
	E_FREE(ci);
     }
   E_FREE(ibar_config);
   ibar_config = NULL;
   E_CONFIG_DD_FREE(conf_item_edd);
   E_CONFIG_DD_FREE(conf_edd);
   return 1;
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   e_config_domain_save("module.ibar", conf_edd, ibar_config);
   return 1;
}

static Eina_Bool
_ibar_cb_config_icons(__UNUSED__ void *data, __UNUSED__ int ev_type, void *event)
{
   Efreet_Event_Cache_Update *ev = event;
   const Eina_List *l;
   Instance *inst;

   if (!ev->changed) return ECORE_CALLBACK_PASS_ON;

   EINA_LIST_FOREACH(ibar_config->instances, l, inst)
     {
	const Eina_List *l2;
	IBar_Icon *icon;

	EINA_LIST_FOREACH(inst->ibar->icons, l2, icon)
	  _ibar_icon_fill(icon);
     }
   return ECORE_CALLBACK_PASS_ON;
}
