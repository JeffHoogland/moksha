/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

/***************************************************************************/
/**/
/* gadcon requirements */
static E_Gadcon_Client *_gc_init(E_Gadcon *gc, char *name, char *id, char *style);
static void _gc_shutdown(E_Gadcon_Client *gcc);
static void _gc_orient(E_Gadcon_Client *gcc);
static char *_gc_label(void);
static Evas_Object *_gc_icon(Evas *evas);
/* and actually define the gadcon class that this module provides (just 1) */
static const E_Gadcon_Client_Class _gadcon_class =
{
   GADCON_CLIENT_CLASS_VERSION,
     "ibox",
     {
	_gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon
     }
};
/**/
/***************************************************************************/

/***************************************************************************/
/**/
/* actual module specifics */

typedef struct _Instance  Instance;

typedef struct _IBox      IBox;
typedef struct _IBox_Icon IBox_Icon;

struct _Instance
{
   E_Gadcon_Client *gcc;
   Evas_Object     *o_ibox;
   IBox            *ibox;
   E_Drop_Handler  *drop_handler;
   Ecore_Timer     *drop_recalc_timer;
};

struct _IBox
{
   Instance       *inst;
   Evas_Object    *o_box;
   Evas_Object    *o_drop;
   Evas_Object    *o_drop_over;
   Evas_Object    *o_empty;
   IBox_Icon      *ic_drop_before;
   int             drop_before;
   Evas_List      *icons;
   int             show_label;
   E_Zone          *zone;
};

struct _IBox_Icon
{
   IBox        *ibox;
   Evas_Object *o_holder;
   Evas_Object *o_icon;
   Evas_Object *o_holder2;
   Evas_Object *o_icon2;
   E_Border    *border;
   struct {
      unsigned char  start : 1;
      unsigned char  dnd : 1;
      int            x, y;
      int            dx, dy;
   } drag;
};

static IBox *_ibox_new(Evas *evas, E_Zone *zone);
static void _ibox_free(IBox *b);
static void _ibox_cb_empty_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _ibox_empty_handle(IBox *b);
static void _ibox_fill(IBox *b);
static void _ibox_empty(IBox *b);
static void _ibox_orient_set(IBox *b, int horizontal);
static void _ibox_resize_handle(IBox *b);
static void _ibox_instance_drop_zone_recalc(Instance *inst);
static IBox_Icon *_ibox_icon_find(IBox *b, E_Border *bd);
static IBox_Icon *_ibox_icon_at_coord(IBox *b, Evas_Coord x, Evas_Coord y);
static IBox_Icon *_ibox_icon_new(IBox *b, E_Border *bd);
static void _ibox_icon_free(IBox_Icon *ic);
static void _ibox_icon_fill(IBox_Icon *ic);
static void _ibox_icon_empty(IBox_Icon *ic);
static void _ibox_icon_signal_emit(IBox_Icon *ic, char *sig, char *src);
static IBox *_ibox_zone_find(E_Zone *zone);
static int _ibox_cb_timer_drop_recalc(void *data);
static void _ibox_cb_obj_moveresize(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _ibox_cb_menu_post(void *data, E_Menu *m);
static void _ibox_cb_menu_configuration(void *data, E_Menu *m, E_Menu_Item *mi);
static void _ibox_cb_icon_mouse_in(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _ibox_cb_icon_mouse_out(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _ibox_cb_icon_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _ibox_cb_icon_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _ibox_cb_icon_mouse_move(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _ibox_cb_icon_move(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _ibox_cb_icon_resize(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _ibox_cb_drag_finished(E_Drag *drag, int dropped);
static void _ibox_inst_cb_enter(void *data, const char *type, void *event_info);
static void _ibox_inst_cb_move(void *data, const char *type, void *event_info);
static void _ibox_inst_cb_leave(void *data, const char *type, void *event_info);
static void _ibox_inst_cb_drop(void *data, const char *type, void *event_info);
static int _ibox_cb_event_border_add(void *data, int type, void *event);
static int _ibox_cb_event_border_remove(void *data, int type, void *event);
static int _ibox_cb_event_border_iconify(void *data, int type, void *event);
static int _ibox_cb_event_border_uniconify(void *data, int type, void *event);
static int _ibox_cb_event_border_icon_change(void *data, int type, void *event);
static int _ibox_cb_event_border_zone_set(void *data, int type, void *event);
static Config_Item *_ibox_config_item_get(const char *id);

static E_Config_DD *conf_edd = NULL;
static E_Config_DD *conf_item_edd = NULL;

Config *ibox_config = NULL;

static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, char *name, char *id, char *style)
{
   IBox *b;
   Evas_Object *o;
   E_Gadcon_Client *gcc;
   Instance *inst;
   Evas_Coord x, y, w, h;
   int cx, cy, cw, ch;
   const char *drop[] = { "enlightenment/border" };
   Evas_List *l;
   Config_Item *ci;
   
   inst = E_NEW(Instance, 1);

   ci = _ibox_config_item_get(id);
   
   b = _ibox_new(gc->evas, gc->zone);
   b->show_label = ci->show_label;
   b->inst = inst;
   inst->ibox = b;
   o = b->o_box;
   gcc = e_gadcon_client_new(gc, name, id, style, o);
   gcc->data = inst;
   
   inst->gcc = gcc;
   inst->o_ibox = o;
   
   e_gadcon_canvas_zone_geometry_get(inst->gcc->gadcon, &cx, &cy, &cw, &ch);
   evas_object_geometry_get(o, &x, &y, &w, &h);
   inst->drop_handler =
     e_drop_handler_add(inst,
			_ibox_inst_cb_enter, _ibox_inst_cb_move,
			_ibox_inst_cb_leave, _ibox_inst_cb_drop,
			drop, 1, cx + x, cy + y, w, h);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOVE,
				  _ibox_cb_obj_moveresize, inst);
   evas_object_event_callback_add(o, EVAS_CALLBACK_RESIZE,
				  _ibox_cb_obj_moveresize, inst);
   ibox_config->instances = evas_list_append(ibox_config->instances, inst);
   /* FIXME: HACK!!!! */
   inst->drop_recalc_timer = ecore_timer_add(1.0, _ibox_cb_timer_drop_recalc,
					     inst);
   return gcc;
}

static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
   Instance *inst;
   
   inst = gcc->data;
   ecore_timer_del(inst->drop_recalc_timer);
   ibox_config->instances = evas_list_remove(ibox_config->instances, inst);
   e_drop_handler_del(inst->drop_handler);
   _ibox_free(inst->ibox);
   free(inst);
}

static void
_gc_orient(E_Gadcon_Client *gcc)
{
   Instance *inst;
   
   inst = gcc->data;
   switch (gcc->gadcon->orient)
     {
      case E_GADCON_ORIENT_FLOAT:
      case E_GADCON_ORIENT_HORIZ:
      case E_GADCON_ORIENT_TOP:
      case E_GADCON_ORIENT_BOTTOM:
      case E_GADCON_ORIENT_CORNER_TL:
      case E_GADCON_ORIENT_CORNER_TR:
      case E_GADCON_ORIENT_CORNER_BL:
      case E_GADCON_ORIENT_CORNER_BR:
	_ibox_orient_set(inst->ibox, 1);
	e_gadcon_client_aspect_set(gcc, evas_list_count(inst->ibox->icons) * 16, 16);
	break;
      case E_GADCON_ORIENT_VERT:
      case E_GADCON_ORIENT_LEFT:
      case E_GADCON_ORIENT_RIGHT:
      case E_GADCON_ORIENT_CORNER_LT:
      case E_GADCON_ORIENT_CORNER_RT:
      case E_GADCON_ORIENT_CORNER_LB:
      case E_GADCON_ORIENT_CORNER_RB:
	_ibox_orient_set(inst->ibox, 0);
	e_gadcon_client_aspect_set(gcc, 16, evas_list_count(inst->ibox->icons) * 16);
	break;
      default:
	break;
     }
   if (evas_list_count(inst->ibox->icons) < 1)
     e_gadcon_client_aspect_set(gcc, 16, 16);
   e_gadcon_client_min_size_set(gcc, 16, 16);
}

static char *
_gc_label(void)
{
   return _("IBox");
}

static Evas_Object *
_gc_icon(Evas *evas)
{
   Evas_Object *o;
   char buf[4096];
   
   o = edje_object_add(evas);
   snprintf(buf, sizeof(buf), "%s/module.eap",
	    e_module_dir_get(ibox_config->module));
   edje_object_file_set(o, buf, "icon");
   return o;
}
/**/
/***************************************************************************/

/***************************************************************************/
/**/

static IBox *
_ibox_new(Evas *evas, E_Zone *zone)
{
   IBox *b;
   
   b = E_NEW(IBox, 1);
   b->o_box = e_box_add(evas);
   e_box_homogenous_set(b->o_box, 1);
   e_box_orientation_set(b->o_box, 1);
   e_box_align_set(b->o_box, 0.5, 0.5);
   b->zone = zone;
   _ibox_fill(b);
   return b;
}

static void
_ibox_free(IBox *b)
{
   _ibox_empty(b);
   evas_object_del(b->o_box);
   if (b->o_drop) evas_object_del(b->o_drop);
   if (b->o_drop_over) evas_object_del(b->o_drop_over);
   if (b->o_empty) evas_object_del(b->o_empty);
   free(b);
}

static void
_ibox_cb_empty_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   IBox *b;
   
   ev = event_info;
   b = data;
   if (!ibox_config->menu)
     {
	E_Menu *mn;
	E_Menu_Item *mi;
	int cx, cy, cw, ch;
	
	mn = e_menu_new();
	e_menu_post_deactivate_callback_set(mn, _ibox_cb_menu_post, NULL);
	ibox_config->menu = mn;

	mi = e_menu_item_new(mn);
	e_menu_item_label_set(mi, _("Configuration"));
	e_util_menu_item_edje_icon_set(mi, "enlightenment/configuration");
	e_menu_item_callback_set(mi, _ibox_cb_menu_configuration, b);

	mi = e_menu_item_new(mn);
	e_menu_item_separator_set(mi, 1);
	
	e_gadcon_client_util_menu_items_append(b->inst->gcc, mn, 0);
	
	e_gadcon_canvas_zone_geometry_get(b->inst->gcc->gadcon,
					  &cx, &cy, &cw, &ch);
	e_menu_activate_mouse(mn,
			      e_util_zone_current_get(e_manager_current_get()),
			      cx + ev->output.x, cy + ev->output.y, 1, 1,
			      E_MENU_POP_DIRECTION_DOWN, ev->timestamp);
	evas_event_feed_mouse_up(b->inst->gcc->gadcon->evas, ev->button,
				 EVAS_BUTTON_NONE, ev->timestamp, NULL);
     }
}

static void
_ibox_empty_handle(IBox *b)
{
   if (!b->icons)
     {
	if (!b->o_empty)
	  {
	     Evas_Coord w, h;
	     
	     b->o_empty = evas_object_rectangle_add(evas_object_evas_get(b->o_box));
	     evas_object_event_callback_add(b->o_empty, EVAS_CALLBACK_MOUSE_DOWN, _ibox_cb_empty_mouse_down, b);
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
				    w, h /* max */
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
_ibox_fill(IBox *b)
{
   IBox_Icon *ic;
   E_Border_List *bl;
   E_Border *bd;
   
   bl = e_container_border_list_first(b->zone->container);
   while ((bd = e_container_border_list_next(bl)))
     {
	if ((bd->zone == b->zone) && (bd->iconic))
	  {
	     ic = _ibox_icon_new(b, bd);
	     b->icons = evas_list_append(b->icons, ic);
	     e_box_pack_end(b->o_box, ic->o_holder);
	  }
     }
   
   _ibox_empty_handle(b);
   _ibox_resize_handle(b);
}

static void
_ibox_empty(IBox *b)
{
   while (b->icons)
     {
	_ibox_icon_free(b->icons->data);
	b->icons = evas_list_remove_list(b->icons, b->icons);
     }
   _ibox_empty_handle(b);
}

static void
_ibox_orient_set(IBox *b, int horizontal)
{
   e_box_orientation_set(b->o_box, horizontal);
   e_box_align_set(b->o_box, 0.5, 0.5);
}

static void
_ibox_resize_handle(IBox *b)
{
   Evas_List *l;
   IBox_Icon *ic;
   Evas_Coord w, h;
   
   evas_object_geometry_get(b->o_box, NULL, NULL, &w, &h);
   if (e_box_orientation_get(b->o_box))
     w = h;
   else
     h = w;
   e_box_freeze(b->o_box);
   for (l = b->icons; l; l = l->next)
     {
	ic = l->data;
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
_ibox_instance_drop_zone_recalc(Instance *inst)
{
   Evas_Coord x, y, w, h;
   int cx, cy, cw, ch;
   
   evas_object_geometry_get(inst->o_ibox, &x, &y, &w, &h);
   e_gadcon_canvas_zone_geometry_get(inst->gcc->gadcon, &cx, &cy, &cw, &ch);
   e_drop_handler_geometry_set(inst->drop_handler, cx + x, cy + y, w, h);
}  

static IBox_Icon *
_ibox_icon_find(IBox *b, E_Border *bd)
{
   Evas_List *l;
   IBox_Icon *ic;
   
   for (l = b->icons; l; l = l->next)
     {
	ic = l->data;
	
	if (ic->border == bd) return ic;
     }
   return NULL;
}

static IBox_Icon *
_ibox_icon_at_coord(IBox *b, Evas_Coord x, Evas_Coord y)
{
   Evas_List *l;
   IBox_Icon *ic;
   
   for (l = b->icons; l; l = l->next)
     {
        Evas_Coord dx, dy, dw, dh;
	ic = l->data;
	
        evas_object_geometry_get(ic->o_holder, &dx, &dy, &dw, &dh);
	if (E_INSIDE(x, y, dx, dy, dw, dh)) return ic;
     }
   return NULL;
}

static IBox_Icon *
_ibox_icon_new(IBox *b, E_Border *bd)
{
   IBox_Icon *ic;
   
   ic = E_NEW(IBox_Icon, 1);
   e_object_ref(E_OBJECT(bd));
   ic->ibox = b;
   ic->border = bd;
   ic->o_holder = edje_object_add(evas_object_evas_get(b->o_box));
   e_theme_edje_object_set(ic->o_holder, "base/theme/modules/ibox",
			   "modules/ibox/icon");
   evas_object_event_callback_add(ic->o_holder, EVAS_CALLBACK_MOUSE_IN,  _ibox_cb_icon_mouse_in,  ic);
   evas_object_event_callback_add(ic->o_holder, EVAS_CALLBACK_MOUSE_OUT, _ibox_cb_icon_mouse_out, ic);
   evas_object_event_callback_add(ic->o_holder, EVAS_CALLBACK_MOUSE_DOWN, _ibox_cb_icon_mouse_down, ic);
   evas_object_event_callback_add(ic->o_holder, EVAS_CALLBACK_MOUSE_UP, _ibox_cb_icon_mouse_up, ic);
   evas_object_event_callback_add(ic->o_holder, EVAS_CALLBACK_MOUSE_MOVE, _ibox_cb_icon_mouse_move, ic);
   evas_object_event_callback_add(ic->o_holder, EVAS_CALLBACK_MOVE, _ibox_cb_icon_move, ic);
   evas_object_event_callback_add(ic->o_holder, EVAS_CALLBACK_RESIZE, _ibox_cb_icon_resize, ic);
   evas_object_show(ic->o_holder);
   
   ic->o_holder2 = edje_object_add(evas_object_evas_get(b->o_box));
   e_theme_edje_object_set(ic->o_holder2, "base/theme/modules/ibox",
			   "modules/ibox/icon_overlay");
   evas_object_layer_set(ic->o_holder2, 9999);
   evas_object_pass_events_set(ic->o_holder2, 1);
   evas_object_show(ic->o_holder2);
   
   _ibox_icon_fill(ic);
   return ic;
}

static void
_ibox_icon_free(IBox_Icon *ic)
{
   Evas_Object *o;

   if (ibox_config->menu)
     {
	e_menu_post_deactivate_callback_set(ibox_config->menu, NULL, NULL);
	e_object_del(E_OBJECT(ibox_config->menu));
	ibox_config->menu = NULL;
     }
   if (ic->ibox->ic_drop_before == ic)
     ic->ibox->ic_drop_before = NULL;
   _ibox_icon_empty(ic);
   evas_object_del(ic->o_holder);
   evas_object_del(ic->o_holder2);
   e_object_unref(E_OBJECT(ic->border));
   free(ic);
}

static void
_ibox_icon_fill(IBox_Icon *ic)
{
   char *label;
   
   ic->o_icon = e_border_icon_add(ic->border, evas_object_evas_get(ic->ibox->o_box));
   edje_object_part_swallow(ic->o_holder, "item", ic->o_icon);
   evas_object_pass_events_set(ic->o_icon, 1);
   evas_object_show(ic->o_icon);
   ic->o_icon2 = e_border_icon_add(ic->border, evas_object_evas_get(ic->ibox->o_box));
   edje_object_part_swallow(ic->o_holder2, "item", ic->o_icon2);
   evas_object_pass_events_set(ic->o_icon2, 1);
   evas_object_show(ic->o_icon2);
   
   /* FIXME: preferences for icon name */
   label = ic->border->client.netwm.icon_name;
   if (!label) label = ic->border->client.icccm.icon_name;
   if (!label) label = ic->border->client.icccm.class;
   if (!label) label = ic->border->client.netwm.name;
   if (!label) label = ic->border->client.icccm.title;
   if (!label) label = "?";
   edje_object_part_text_set(ic->o_holder, "label", label);
   edje_object_part_text_set(ic->o_holder2, "label", label);
}

static void
_ibox_icon_empty(IBox_Icon *ic)
{
   if (ic->o_icon) evas_object_del(ic->o_icon);
   if (ic->o_icon2) evas_object_del(ic->o_icon2);
   ic->o_icon = NULL;
   ic->o_icon2 = NULL;
}

static void
_ibox_icon_signal_emit(IBox_Icon *ic, char *sig, char *src)
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

static IBox *
_ibox_zone_find(E_Zone *zone)
{
   Evas_List *l;
   
   for (l = ibox_config->instances; l; l = l->next)
     {
	Instance *inst;

	inst = l->data;
	if (inst->ibox->zone == zone) return inst->ibox;
     }
   return NULL;
}

static int
_ibox_cb_timer_drop_recalc(void *data)
{
   Instance *inst;
   
   inst = data;
   _ibox_instance_drop_zone_recalc(inst);
   return 1;
}

static void
_ibox_cb_obj_moveresize(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Instance *inst;
   
   inst = data;
   _ibox_resize_handle(inst->ibox);
   _ibox_instance_drop_zone_recalc(inst);
}

static void
_ibox_cb_menu_post(void *data, E_Menu *m)
{
   if (!ibox_config->menu) return;
   e_object_del(E_OBJECT(ibox_config->menu));
   ibox_config->menu = NULL;
}

static void
_ibox_cb_icon_mouse_in(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_In *ev;
   IBox_Icon *ic;
   
   ev = event_info;
   ic = data;
   _ibox_icon_signal_emit(ic, "active", "");
   if (ic->ibox->show_label)
     _ibox_icon_signal_emit(ic, "label_active", "");
}

static void
_ibox_cb_icon_mouse_out(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Out *ev;
   IBox_Icon *ic;
   
   ev = event_info;
   ic = data;
   _ibox_icon_signal_emit(ic, "passive", "");
   if (ic->ibox->show_label)
     _ibox_icon_signal_emit(ic, "label_passive", "");
}

static void
_ibox_cb_icon_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   IBox_Icon *ic;
   
   ev = event_info;
   ic = data;
   if (ev->button == 1)
     {
	ic->drag.x = ev->output.x;
	ic->drag.y = ev->output.y;
	ic->drag.start = 1;
	ic->drag.dnd = 0;
     }
   else if ((ev->button == 3) && (!ibox_config->menu))
     {
	E_Menu *mn;
	E_Menu_Item *mi;
	int cx, cy, cw, ch;
	
	mn = e_menu_new();
	e_menu_post_deactivate_callback_set(mn, _ibox_cb_menu_post, NULL);
	ibox_config->menu = mn;

	/* FIXME: other icon options go here too */
	mi = e_menu_item_new(mn);
	e_menu_item_label_set(mi, _("Configuration"));
	e_util_menu_item_edje_icon_set(mi, "enlightenment/configuration");
	e_menu_item_callback_set(mi, _ibox_cb_menu_configuration, ic->ibox);

	mi = e_menu_item_new(mn);
	e_menu_item_separator_set(mi, 1);
	
	e_gadcon_client_util_menu_items_append(ic->ibox->inst->gcc, mn, 0);
	
	e_gadcon_canvas_zone_geometry_get(ic->ibox->inst->gcc->gadcon,
					  &cx, &cy, &cw, &ch);
	e_menu_activate_mouse(mn,
			      e_util_zone_current_get(e_manager_current_get()),
			      cx + ev->output.x, cy + ev->output.y, 1, 1,
			      E_MENU_POP_DIRECTION_DOWN, ev->timestamp);
	evas_event_feed_mouse_up(ic->ibox->inst->gcc->gadcon->evas, ev->button,
				 EVAS_BUTTON_NONE, ev->timestamp, NULL);
     }
}

static void
_ibox_cb_icon_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Up *ev;
   IBox_Icon *ic;
   
   ev = event_info;
   ic = data;
   if ((ev->button == 1) && (!ic->drag.dnd))
     { 
	e_border_uniconify(ic->border);
	e_border_focus_set(ic->border, 1, 1);
     }
}

static void
_ibox_cb_icon_mouse_move(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Move *ev;
   IBox_Icon *ic;
   
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
	     const char *drag_types[] = { "enlightenment/border" };

	     ic->drag.dnd = 1;
	     ic->drag.start = 0;

	     evas_object_geometry_get(ic->o_icon, &x, &y, &w, &h);
	     d = e_drag_new(ic->ibox->inst->gcc->gadcon->zone->container,
			    x, y, drag_types, 1,
			    ic->border, -1, _ibox_cb_drag_finished);
	     o = e_border_icon_add(ic->border, e_drag_evas_get(d));
	     e_drag_object_set(d, o);

	     e_drag_resize(d, w, h);
	     e_drag_start(d, ic->drag.x, ic->drag.y);
	     evas_event_feed_mouse_up(ic->ibox->inst->gcc->gadcon->evas,
				      1, EVAS_BUTTON_NONE, 
				      ecore_x_current_time_get(), NULL);
	     e_object_ref(E_OBJECT(ic->border));
	     ic->ibox->icons = evas_list_remove(ic->ibox->icons, ic);
	     _ibox_resize_handle(ic->ibox);
	     _gc_orient(ic->ibox->inst->gcc);
	     _ibox_icon_free(ic);
	  }
     }
}

static void
_ibox_cb_icon_move(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   IBox_Icon *ic;
   Evas_Coord x, y;
   
   ic = data;
   evas_object_geometry_get(ic->o_holder, &x, &y, NULL, NULL);
   evas_object_move(ic->o_holder2, x, y);
   evas_object_raise(ic->o_holder2);
}

static void
_ibox_cb_icon_resize(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   IBox_Icon *ic;
   Evas_Coord w, h;
   
   ic = data;
   evas_object_geometry_get(ic->o_holder, NULL, NULL, &w, &h);
   evas_object_resize(ic->o_holder2, w, h);
   evas_object_raise(ic->o_holder2);
}

static void
_ibox_cb_drag_finished(E_Drag *drag, int dropped)
{
   E_Border *bd;
   
   bd = drag->data;
   if (!dropped) e_border_uniconify(bd);
   e_object_unref(E_OBJECT(bd));
}

static void
_ibox_cb_drop_move(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   IBox *b;
   Evas_Coord x, y;
   
   b = data;
   evas_object_geometry_get(b->o_drop, &x, &y, NULL, NULL);
   evas_object_move(b->o_drop_over, x, y);
}

static void
_ibox_cb_drop_resize(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   IBox *b;
   Evas_Coord w, h;
   
   b = data;
   evas_object_geometry_get(b->o_drop, NULL, NULL, &w, &h);
   evas_object_resize(b->o_drop_over, w, h);
}

static void
_ibox_inst_cb_enter(void *data, const char *type, void *event_info)
{
   E_Event_Dnd_Enter *ev;
   Instance *inst;
   Evas_Object *o, *o2;
   IBox_Icon *ic;
   int cx, cy, cw, ch;
   
   ev = event_info;
   inst = data;
   o = edje_object_add(evas_object_evas_get(inst->ibox->o_box));
   inst->ibox->o_drop = o;
   o2 = edje_object_add(evas_object_evas_get(inst->ibox->o_box));
   inst->ibox->o_drop_over = o2;
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOVE, _ibox_cb_drop_move, inst->ibox);
   evas_object_event_callback_add(o, EVAS_CALLBACK_RESIZE, _ibox_cb_drop_resize, inst->ibox);
   e_theme_edje_object_set(o, "base/theme/modules/ibox",
			   "modules/ibox/drop");
   e_theme_edje_object_set(o2, "base/theme/modules/ibox",
			   "modules/ibox/drop_overlay");
   evas_object_layer_set(o2, 19999);
   evas_object_show(o);
   evas_object_show(o2);
   e_gadcon_canvas_zone_geometry_get(inst->gcc->gadcon, &cx, &cy, &cw, &ch);
   ic = _ibox_icon_at_coord(inst->ibox, ev->x - cx, ev->y - cy);
   inst->ibox->ic_drop_before = ic;
   if (ic)
     {
	Evas_Coord ix, iy, iw, ih;
	int before = 0;
	
	evas_object_geometry_get(ic->o_holder, &ix, &iy, &iw, &ih);
	if (e_box_orientation_get(inst->ibox->o_box))
	  {
	     if ((ev->x - cx) < (ix + (iw / 2))) before = 1;
	  }
	else
	  {
	     if ((ev->y - cy) < (iy + (ih / 2))) before = 1;
	  }
	if (before)
	  e_box_pack_before(inst->ibox->o_box, inst->ibox->o_drop, ic->o_holder);
	else
	  e_box_pack_after(inst->ibox->o_box, inst->ibox->o_drop, ic->o_holder);
	inst->ibox->drop_before = before;
     }
   else e_box_pack_end(inst->ibox->o_box, o);
   e_box_pack_options_set(o,
			  1, 1, /* fill */
			  0, 0, /* expand */
			  0.5, 0.5, /* align */
			  1, 1, /* min */
			  -1, -1 /* max */
			  );
   _ibox_resize_handle(inst->ibox);
   _gc_orient(inst->gcc);
}

static void
_ibox_inst_cb_move(void *data, const char *type, void *event_info)
{
   E_Event_Dnd_Move *ev;
   Instance *inst;
   IBox_Icon *ic;
   int cx, cy, cw, ch;
   
   ev = event_info;
   inst = data;
   e_box_unpack(inst->ibox->o_drop);
   e_gadcon_canvas_zone_geometry_get(inst->gcc->gadcon, &cx, &cy, &cw, &ch);
   ic = _ibox_icon_at_coord(inst->ibox, ev->x - cx, ev->y - cy);
   inst->ibox->ic_drop_before = ic;
   if (ic)
     {
	Evas_Coord ix, iy, iw, ih;
	int before = 0;
	
	evas_object_geometry_get(ic->o_holder, &ix, &iy, &iw, &ih);
	if (e_box_orientation_get(inst->ibox->o_box))
	  {
	     if ((ev->x - cx) < (ix + (iw / 2))) before = 1;
	  }
	else
	  {
	     if ((ev->y - cy) < (iy + (ih / 2))) before = 1;
	  }
	if (before)
	  e_box_pack_before(inst->ibox->o_box, inst->ibox->o_drop, ic->o_holder);
	else
	  e_box_pack_after(inst->ibox->o_box, inst->ibox->o_drop, ic->o_holder);
	inst->ibox->drop_before = before;
     }
   else e_box_pack_end(inst->ibox->o_box, inst->ibox->o_drop);
   e_box_pack_options_set(inst->ibox->o_drop,
			  1, 1, /* fill */
			  0, 0, /* expand */
			  0.5, 0.5, /* align */
			  1, 1, /* min */
			  -1, -1 /* max */
			  );
   _ibox_resize_handle(inst->ibox);
   _gc_orient(inst->gcc);
}

static void
_ibox_inst_cb_leave(void *data, const char *type, void *event_info)
{
   E_Event_Dnd_Leave *ev;
   Instance *inst;
   
   ev = event_info;
   inst = data;
   inst->ibox->ic_drop_before = NULL;
   evas_object_del(inst->ibox->o_drop);
   inst->ibox->o_drop = NULL;
   evas_object_del(inst->ibox->o_drop_over);
   inst->ibox->o_drop_over = NULL;
   _ibox_resize_handle(inst->ibox);
   _gc_orient(inst->gcc);
}

static void
_ibox_inst_cb_drop(void *data, const char *type, void *event_info)
{
   E_Event_Dnd_Drop *ev;
   Instance *inst;
   E_Border *bd = NULL;
   IBox *b;
   IBox_Icon *ic, *ic2;
   Evas_List *l;
   
   ev = event_info;
   inst = data;
   if (!strcmp(type, "enlightenment/border"))
     {
	bd = ev->data;
	if (!bd) return;
     }
   
   if (!bd->iconic) e_border_iconify(bd);
   
   ic2 = inst->ibox->ic_drop_before;
   if (ic2)
     {
	/* Add new eapp before this icon */
	if (!inst->ibox->drop_before)
	  {
	     for (l = inst->ibox->icons; l; l = l->next)
	       {
		  if (l->data == ic2)
		    {
		       if (l->next)
			 ic2 = l->next->data;
		       else
			 ic2 = NULL;
		       break;
		    }
	       }
	  }
	if (!ic2) goto atend;
	b = inst->ibox;
	if (_ibox_icon_find(b, bd)) return;
	ic = _ibox_icon_new(b, bd);
	if (!ic) return;
	b->icons = evas_list_prepend_relative(b->icons, ic, ic2);
	e_box_pack_before(b->o_box, ic->o_holder, ic2->o_holder);
     }
   else
     {
	atend:
	b = inst->ibox;
	if (_ibox_icon_find(b, bd)) return;
	ic = _ibox_icon_new(b, bd);
	if (!ic) return;
	b->icons = evas_list_append(b->icons, ic);
	e_box_pack_end(b->o_box, ic->o_holder);
     }
   
   evas_object_del(inst->ibox->o_drop);
   inst->ibox->o_drop = NULL;
   evas_object_del(inst->ibox->o_drop_over);
   inst->ibox->o_drop_over = NULL;
   _ibox_empty_handle(b);
   _ibox_resize_handle(inst->ibox);
   _gc_orient(inst->gcc);
}

static int
_ibox_cb_event_border_add(void *data, int type, void *event)
{
   E_Event_Border_Add *ev;
   IBox *b;
   IBox_Icon *ic;
   
   ev = event;
   /* add if iconic */
   if (ev->border->iconic)
     {
	b = _ibox_zone_find(ev->border->zone);
	if (!b) return 1;
	if (_ibox_icon_find(b, ev->border)) return 1;
	ic = _ibox_icon_new(b, ev->border);
	if (!ic) return 1;
	b->icons = evas_list_append(b->icons, ic);
	e_box_pack_end(b->o_box, ic->o_holder);
	_ibox_empty_handle(b);
	_ibox_resize_handle(b);
	_gc_orient(b->inst->gcc);
     }
   return 1;
}

static int
_ibox_cb_event_border_remove(void *data, int type, void *event)
{
   E_Event_Border_Remove *ev;
   IBox *b;
   IBox_Icon *ic;
   
   ev = event;
   /* find icon and remove if there */
   b = _ibox_zone_find(ev->border->zone);
   if (!b) return 1;
   ic = _ibox_icon_find(b, ev->border);
   if (!ic) return 1;
   _ibox_icon_free(ic);
   b->icons = evas_list_remove(b->icons, ic);
   _ibox_empty_handle(b);
   _ibox_resize_handle(b);
   _gc_orient(b->inst->gcc);
   return 1;
}

static int
_ibox_cb_event_border_iconify(void *data, int type, void *event)
{
   E_Event_Border_Iconify *ev;
   IBox *b;
   IBox_Icon *ic;
   
   ev = event;
   /* add icon for ibox for right zone */
   /* do some sort of anim when iconifying */
   b = _ibox_zone_find(ev->border->zone);
   if (!b) return 1;
   if (_ibox_icon_find(b, ev->border)) return 1;
   ic = _ibox_icon_new(b, ev->border);
   if (!ic) return 1;
   b->icons = evas_list_append(b->icons, ic);
   e_box_pack_end(b->o_box, ic->o_holder);
   _ibox_empty_handle(b);
   _ibox_resize_handle(b);
   _gc_orient(b->inst->gcc);
   return 1;
}

static int
_ibox_cb_event_border_uniconify(void *data, int type, void *event)
{
   E_Event_Border_Uniconify *ev;
   IBox *b;
   IBox_Icon *ic;
   
   ev = event;
   /* del icon for ibox for right zone */
   /* do some sort of anim when uniconifying */
   b = _ibox_zone_find(ev->border->zone);
   if (!b) return 1;
   ic = _ibox_icon_find(b, ev->border);
   if (!ic) return 1;
   _ibox_icon_free(ic);
   b->icons = evas_list_remove(b->icons, ic);
   _ibox_empty_handle(b);
   _ibox_resize_handle(b);
   _gc_orient(b->inst->gcc);
   return 1;
}

static int
_ibox_cb_event_border_icon_change(void *data, int type, void *event)
{
   E_Event_Border_Icon_Change *ev;
   IBox *b;
   IBox_Icon *ic;
   
   ev = event;
   /* update icon */
   b = _ibox_zone_find(ev->border->zone);
   if (!b) return 1;
   ic = _ibox_icon_find(b, ev->border);
   if (!ic) return 1;
   _ibox_icon_empty(ic);
   _ibox_icon_fill(ic);
   return 1;
}

static int
_ibox_cb_event_border_zone_set(void *data, int type, void *event)
{
   E_Event_Border_Zone_Set *ev;
   IBox *b;
   IBox_Icon *ic;
   
   ev = event;
   /* delete from current zone ibox, add to new one */
   if (ev->border->iconic)
     {  
     }
   return 1;
}

static Config_Item *
_ibox_config_item_get(const char *id) 
{
   Evas_List *l;
   Config_Item *ci;
   
   for (l = ibox_config->items; l; l = l->next) 
     {
	ci = l->data;
	if ((ci->id) && (!strcmp(ci->id, id)))
	  return ci;
     }
   ci = E_NEW(Config_Item, 1);
   ci->id = evas_stringshare_add(id);
   ci->show_label = 0;
   ibox_config->items = evas_list_append(ibox_config->items, ci);
   return ci;
}

void
_ibox_config_update(void) 
{
   Evas_List *l;
   for (l = ibox_config->instances; l; l = l->next) 
     {
	Instance *inst;
	Config_Item *ci;
   
	inst = l->data;
	ci = _ibox_config_item_get(inst->gcc->id);
	inst->ibox->show_label = ci->show_label;
     }
}

static void
_ibox_cb_menu_configuration(void *data, E_Menu *m, E_Menu_Item *mi) 
{
   IBox *b;
   Config_Item *ci;
   
   b = data;
   ci = _ibox_config_item_get(b->inst->gcc->id);
   _config_ibox_module(ci);
}

/***************************************************************************/
/**/
/* module setup */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
     "IBox"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   conf_item_edd = E_CONFIG_DD_NEW("IBox_Config_Item", Config_Item);
   #undef T
   #undef D
   #define T Config_Item
   #define D conf_item_edd
   E_CONFIG_VAL(D, T, id, STR);
   E_CONFIG_VAL(D, T, show_label, INT);
   
   conf_edd = E_CONFIG_DD_NEW("IBox_Config", Config);
   #undef T
   #undef D
   #define T Config
   #define D conf_edd
   E_CONFIG_LIST(D, T, items, conf_item_edd);
   
   ibox_config = e_config_domain_load("module.ibox", conf_edd);
   if (!ibox_config) 
     {
	Config_Item *ci;

	ibox_config = E_NEW(Config, 1);
	
	ci = E_NEW(Config_Item, 1);
	ci->id = evas_stringshare_add("0");
	ci->show_label = 0;
	ibox_config->items = evas_list_append(ibox_config->items, ci);
     }
   
   ibox_config->module = m;
   
   ibox_config->handlers = evas_list_append
     (ibox_config->handlers, ecore_event_handler_add
      (E_EVENT_BORDER_ADD, _ibox_cb_event_border_add, NULL));
   ibox_config->handlers = evas_list_append
     (ibox_config->handlers, ecore_event_handler_add
      (E_EVENT_BORDER_REMOVE, _ibox_cb_event_border_remove, NULL));
   ibox_config->handlers = evas_list_append
     (ibox_config->handlers, ecore_event_handler_add
      (E_EVENT_BORDER_ICONIFY, _ibox_cb_event_border_iconify, NULL));
   ibox_config->handlers = evas_list_append
     (ibox_config->handlers, ecore_event_handler_add
      (E_EVENT_BORDER_UNICONIFY, _ibox_cb_event_border_uniconify, NULL));
   ibox_config->handlers = evas_list_append
     (ibox_config->handlers, ecore_event_handler_add
      (E_EVENT_BORDER_ICON_CHANGE, _ibox_cb_event_border_icon_change, NULL));
   ibox_config->handlers = evas_list_append
     (ibox_config->handlers, ecore_event_handler_add
      (E_EVENT_BORDER_ZONE_SET, _ibox_cb_event_border_zone_set, NULL));
   
/* FIXME: add these later for things taskbar-like functionality   
   ibox_config->handlers = evas_list_append
     (ibox_config->handlers, ecore_event_handler_add
      (E_EVENT_BORDER_DESK_SET, _ibox_cb_event_border_zone_set, NULL));
   ibox_config->handlers = evas_list_append
     (ibox_config->handlers, ecore_event_handler_add
      (E_EVENT_BORDER_SHOW, _ibox_cb_event_border_zone_set, NULL));
   ibox_config->handlers = evas_list_append
     (ibox_config->handlers, ecore_event_handler_add
      (E_EVENT_BORDER_HIDE, _ibox_cb_event_border_zone_set, NULL));
   ibox_config->handlers = evas_list_append
     (ibox_config->handlers, ecore_event_handler_add
      (E_EVENT_BORDER_STACK, _ibox_cb_event_border_zone_set, NULL));
   ibox_config->handlers = evas_list_append
     (ibox_config->handlers, ecore_event_handler_add
      (E_EVENT_BORDER_STICK, _ibox_cb_event_border_zone_set, NULL));
 */
   e_gadcon_provider_register(&_gadcon_class);
   return 1;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   e_gadcon_provider_unregister(&_gadcon_class);

   if (ibox_config->config_dialog)
     e_object_del(E_OBJECT(ibox_config->config_dialog));
   
   while (ibox_config->handlers)
     {
	ecore_event_handler_del(ibox_config->handlers->data);
	ibox_config->handlers = evas_list_remove_list(ibox_config->handlers, ibox_config->handlers);
     }
   if (ibox_config->menu)
     {
	e_menu_post_deactivate_callback_set(ibox_config->menu, NULL, NULL);
	e_object_del(E_OBJECT(ibox_config->menu));
	ibox_config->menu = NULL;
     }
   while (ibox_config->items) 
     {
	Config_Item *ci;
	
	ci = ibox_config->items->data;
	ibox_config->items = evas_list_remove_list(ibox_config->items, ibox_config->items);
	if (ci->id)
	  evas_stringshare_del(ci->id);
	free(ci);
     }
   
   free(ibox_config);
   ibox_config = NULL;
   E_CONFIG_DD_FREE(conf_item_edd);
   E_CONFIG_DD_FREE(conf_edd);
   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   e_config_domain_save("module.ibox", conf_edd, ibox_config);
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

/**/
/***************************************************************************/
