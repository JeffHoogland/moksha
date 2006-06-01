/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

/***************************************************************************/
/**/
/* gadcon requirements */
static E_Gadcon_Client *_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style);
static void _gc_shutdown(E_Gadcon_Client *gcc);
static void _gc_orient(E_Gadcon_Client *gcc);
static char *_gc_label(void);
static Evas_Object *_gc_icon(Evas *evas);
/* and actually define the gadcon class that this module provides (just 1) */
static const E_Gadcon_Client_Class _gadcon_class =
{
   GADCON_CLIENT_CLASS_VERSION,
     "ibar",
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

typedef struct _IBar      IBar;
typedef struct _IBar_Icon IBar_Icon;

struct _Instance
{
   E_Gadcon_Client *gcc;
   Evas_Object     *o_ibar;
   IBar            *ibar;
   E_Drop_Handler  *drop_handler;
   Ecore_Timer     *drop_recalc_timer;
   const char            *dir;
};

struct _IBar
{
   Instance       *inst;
   Evas_Object    *o_box;
   Evas_Object    *o_drop;
   Evas_Object    *o_drop_over;
   Evas_Object    *o_empty;
   IBar_Icon      *ic_drop_before;
   int             drop_before;
   E_App          *apps;
   Evas_List      *icons;
   int             show_label;
};

struct _IBar_Icon
{
   IBar        *ibar;
   Evas_Object *o_holder;
   Evas_Object *o_icon;
   Evas_Object *o_holder2;
   Evas_Object *o_icon2;
   E_App       *app;
   struct {
      unsigned char  start : 1;
      unsigned char  dnd : 1;
      int            x, y;
      int            dx, dy;
   } drag;
};

static IBar *_ibar_new(Evas *evas, const char *dir);
static void _ibar_free(IBar *b);
static void _ibar_cb_empty_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _ibar_empty_handle(IBar *b);
static void _ibar_fill(IBar *b);
static void _ibar_empty(IBar *b);
static void _ibar_orient_set(IBar *b, int horizontal);
static void _ibar_resize_handle(IBar *b);
static void _ibar_instance_drop_zone_recalc(Instance *inst);
static Config_Item *_ibar_config_item_get(const char *id);
static IBar_Icon *_ibar_icon_find(IBar *b, E_App *a);
static IBar_Icon *_ibar_icon_at_coord(IBar *b, Evas_Coord x, Evas_Coord y);
static IBar_Icon *_ibar_icon_new(IBar *b, E_App *a);
static void _ibar_icon_free(IBar_Icon *ic);
static void _ibar_icon_fill(IBar_Icon *ic);
static void _ibar_icon_empty(IBar_Icon *ic);
static void _ibar_icon_signal_emit(IBar_Icon *ic, char *sig, char *src);
static void _ibar_cb_app_change(void *data, E_App *a, E_App_Change ch);
static int _ibar_cb_timer_drop_recalc(void *data);
static void _ibar_cb_obj_moveresize(void *data, Evas *e, Evas_Object *obj, void *event_info);
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
static void _ibar_cb_drag_finished(E_Drag *drag, int dropped);
static void _ibar_inst_cb_enter(void *data, const char *type, void *event_info);
static void _ibar_inst_cb_move(void *data, const char *type, void *event_info);
static void _ibar_inst_cb_leave(void *data, const char *type, void *event_info);
static void _ibar_inst_cb_drop(void *data, const char *type, void *event_info);

static E_Config_DD *conf_edd = NULL;
static E_Config_DD *conf_item_edd = NULL;

Config *ibar_config = NULL;

static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style)
{
   IBar *b;
   Evas_Object *o;
   E_Gadcon_Client *gcc;
   Instance *inst;
   Evas_Coord x, y, w, h;
   int cx, cy, cw, ch;
   const char *drop[] = { "enlightenment/eapp", "enlightenment/border", "text/uri-list" };
   Config_Item *ci;
   
   inst = E_NEW(Instance, 1);

   ci = _ibar_config_item_get(id);
   if (!ci->dir) ci->dir = evas_stringshare_add("default");
   inst->dir = evas_stringshare_add(ci->dir);
   b = _ibar_new(gc->evas, ci->dir);
   b->show_label = ci->show_label;
   b->inst = inst;
   inst->ibar = b;
   o = b->o_box;
   gcc = e_gadcon_client_new(gc, name, id, style, o);
   gcc->data = inst;
   
   inst->gcc = gcc;
   inst->o_ibar = o;
   
   e_gadcon_canvas_zone_geometry_get(inst->gcc->gadcon, &cx, &cy, &cw, &ch);
   evas_object_geometry_get(o, &x, &y, &w, &h);
   inst->drop_handler =
     e_drop_handler_add(inst,
			_ibar_inst_cb_enter, _ibar_inst_cb_move,
			_ibar_inst_cb_leave, _ibar_inst_cb_drop,
			drop, 3, cx + x, cy + y, w,  h);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOVE,
				  _ibar_cb_obj_moveresize, inst);
   evas_object_event_callback_add(o, EVAS_CALLBACK_RESIZE,
				  _ibar_cb_obj_moveresize, inst);
   ibar_config->instances = evas_list_append(ibar_config->instances, inst);
   /* FIXME: HACK!!!! */
   inst->drop_recalc_timer = ecore_timer_add(1.0, _ibar_cb_timer_drop_recalc,
					     inst);
   return gcc;
}

static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
   Instance *inst;
   
   inst = gcc->data;
   evas_stringshare_del(inst->dir);
   ecore_timer_del(inst->drop_recalc_timer);
   ibar_config->instances = evas_list_remove(ibar_config->instances, inst);
   e_drop_handler_del(inst->drop_handler);
   _ibar_free(inst->ibar);
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
	_ibar_orient_set(inst->ibar, 1);
	e_gadcon_client_aspect_set(gcc, evas_list_count(inst->ibar->icons) * 16, 16);
	break;
      case E_GADCON_ORIENT_VERT:
      case E_GADCON_ORIENT_LEFT:
      case E_GADCON_ORIENT_RIGHT:
      case E_GADCON_ORIENT_CORNER_LT:
      case E_GADCON_ORIENT_CORNER_RT:
      case E_GADCON_ORIENT_CORNER_LB:
      case E_GADCON_ORIENT_CORNER_RB:
	_ibar_orient_set(inst->ibar, 0);
	e_gadcon_client_aspect_set(gcc, 16, evas_list_count(inst->ibar->icons) * 16);
	break;
      default:
	break;
     }
   if (evas_list_count(inst->ibar->icons) < 1)
     e_gadcon_client_aspect_set(gcc, 16, 16);
   e_gadcon_client_min_size_set(gcc, 16, 16);
}

static char *
_gc_label(void)
{
   return _("IBar");
}

static Evas_Object *
_gc_icon(Evas *evas)
{
   Evas_Object *o;
   char buf[4096];
   
   o = edje_object_add(evas);
   snprintf(buf, sizeof(buf), "%s/module.eap",
	    e_module_dir_get(ibar_config->module));
   edje_object_file_set(o, buf, "icon");
   return o;
}
/**/
/***************************************************************************/

/***************************************************************************/
/**/

static IBar *
_ibar_new(Evas *evas, const char *dir)
{
   IBar *b;
   char buf[4096];
   
   b = E_NEW(IBar, 1);
   b->o_box = e_box_add(evas);
   e_box_homogenous_set(b->o_box, 1);
   e_box_orientation_set(b->o_box, 1);
   e_box_align_set(b->o_box, 0.5, 0.5);
   if (dir[0] != '/')
     {
	char *homedir;

	homedir = e_user_homedir_get();
	if (homedir)
	  {
	     snprintf(buf, sizeof(buf), "%s/.e/e/applications/bar/%s", homedir, dir);
	     free(homedir);
	  }
     }
   else
     snprintf(buf, sizeof(buf), dir);
   b->apps = e_app_new(buf, 0);
   if (b->apps) e_app_subdir_scan(b->apps, 0);
   e_app_change_callback_add(_ibar_cb_app_change, b);
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
   if (b->apps) e_object_unref(E_OBJECT(b->apps));
   e_app_change_callback_del(_ibar_cb_app_change, b);
   free(b);
}

static void
_ibar_cb_empty_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   IBar *b;
   
   ev = event_info;
   b = data;
   if (!ibar_config->menu)
     {
	E_Menu *mn;
	E_Menu_Item *mi;
	int cx, cy, cw, ch;
	
	mn = e_menu_new();
	e_menu_post_deactivate_callback_set(mn, _ibar_cb_menu_post, NULL);
	ibar_config->menu = mn;

	mi = e_menu_item_new(mn);
	e_menu_item_label_set(mi, _("Configuration"));
	e_util_menu_item_edje_icon_set(mi, "enlightenment/configuration");
	e_menu_item_callback_set(mi, _ibar_cb_menu_configuration, b);
	
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
_ibar_empty_handle(IBar *b)
{
   if (!b->icons)
     {
	if (!b->o_empty)
	  {
	     Evas_Coord w, h;
	     
	     b->o_empty = evas_object_rectangle_add(evas_object_evas_get(b->o_box));
	     evas_object_event_callback_add(b->o_empty, EVAS_CALLBACK_MOUSE_DOWN, _ibar_cb_empty_mouse_down, b);
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
_ibar_fill(IBar *b)
{
   IBar_Icon *ic;
   Evas_List *l;
   E_App *a;

   if (b->apps)
     {
	for (l = b->apps->subapps; l; l = l->next)
	  {
	     a = l->data;
	     ic = _ibar_icon_new(b, a);
	     b->icons = evas_list_append(b->icons, ic);
	     e_box_pack_end(b->o_box, ic->o_holder);
	  }
     }
   _ibar_empty_handle(b);
   _ibar_resize_handle(b);
}

static void
_ibar_empty(IBar *b)
{
   while (b->icons)
     {
	_ibar_icon_free(b->icons->data);
	b->icons = evas_list_remove_list(b->icons, b->icons);
     }
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
   Evas_List *l;
   IBar_Icon *ic;
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
_ibar_instance_drop_zone_recalc(Instance *inst)
{
   Evas_Coord x, y, w, h;
   int cx, cy, cw, ch;
   
   evas_object_geometry_get(inst->o_ibar, &x, &y, &w, &h);
   e_gadcon_canvas_zone_geometry_get(inst->gcc->gadcon, &cx, &cy, &cw, &ch);
   e_drop_handler_geometry_set(inst->drop_handler, cx + x, cy + y, w, h);
}  

static Config_Item *
_ibar_config_item_get(const char *id)
{
   Evas_List *l;
   Config_Item *ci;
   
   for (l = ibar_config->items; l; l = l->next)
     {
	ci = l->data;
	if ((ci->id) && (ci->dir) && (!strcmp(ci->id, id)))
	  return ci;
     }
   ci = E_NEW(Config_Item, 1);
   ci->id = evas_stringshare_add(id);
   ci->show_label = 1;
   ibar_config->items = evas_list_append(ibar_config->items, ci);
   return ci;
}

void
_ibar_config_update(void)
{
   Evas_List *l;
   
   for (l = ibar_config->instances; l; l = l->next)
     {
	Instance *inst;
	Config_Item *ci;
	
	inst = l->data;
	ci = _ibar_config_item_get(inst->gcc->id);
	if ((inst->dir) && (ci->dir) && (strcmp(ci->dir, inst->dir)))
	  {
	     char buf[4096];
	     
	     evas_stringshare_del(inst->dir);
	     inst->dir = evas_stringshare_add(ci->dir);
	     _ibar_empty(inst->ibar);
	     if (inst->ibar->apps)
	       e_object_unref(E_OBJECT(inst->ibar->apps));
	     if (inst->dir[0] != '/')
	       {
		  char *homedir;
		  
		  homedir = e_user_homedir_get();
		  if (homedir)
		    {
		       snprintf(buf, sizeof(buf), "%s/.e/e/applications/bar/%s", homedir, inst->dir);
		       free(homedir);
		    }
	       }
	     else
	       snprintf(buf, sizeof(buf), inst->dir);
	     inst->ibar->apps = e_app_new(buf, 0);
	     if (inst->ibar->apps) e_app_subdir_scan(inst->ibar->apps, 0);
	     _ibar_fill(inst->ibar);
	     _ibar_resize_handle(inst->ibar);
	     _gc_orient(inst->gcc);
	  }
	inst->ibar->show_label = ci->show_label;
     }
}

static IBar_Icon *
_ibar_icon_find(IBar *b, E_App *a)
{
   Evas_List *l;
   IBar_Icon *ic;
   
   for (l = b->icons; l; l = l->next)
     {
	ic = l->data;
	
	if (ic->app == a) return ic;
     }
   return NULL;
}

static IBar_Icon *
_ibar_icon_at_coord(IBar *b, Evas_Coord x, Evas_Coord y)
{
   Evas_List *l;
   IBar_Icon *ic;
   
   for (l = b->icons; l; l = l->next)
     {
        Evas_Coord dx, dy, dw, dh;
	ic = l->data;
	
        evas_object_geometry_get(ic->o_holder, &dx, &dy, &dw, &dh);
	if (E_INSIDE(x, y, dx, dy, dw, dh)) return ic;
     }
   return NULL;
}

static IBar_Icon *
_ibar_icon_new(IBar *b, E_App *a)
{
   IBar_Icon *ic;
   
   ic = E_NEW(IBar_Icon, 1);
   e_object_ref(E_OBJECT(a));
   ic->ibar = b;
   ic->app = a;
   ic->o_holder = edje_object_add(evas_object_evas_get(b->o_box));
   e_theme_edje_object_set(ic->o_holder, "base/theme/modules/ibar",
			   "modules/ibar/icon");
   evas_object_event_callback_add(ic->o_holder, EVAS_CALLBACK_MOUSE_IN,  _ibar_cb_icon_mouse_in,  ic);
   evas_object_event_callback_add(ic->o_holder, EVAS_CALLBACK_MOUSE_OUT, _ibar_cb_icon_mouse_out, ic);
   evas_object_event_callback_add(ic->o_holder, EVAS_CALLBACK_MOUSE_DOWN, _ibar_cb_icon_mouse_down, ic);
   evas_object_event_callback_add(ic->o_holder, EVAS_CALLBACK_MOUSE_UP, _ibar_cb_icon_mouse_up, ic);
   evas_object_event_callback_add(ic->o_holder, EVAS_CALLBACK_MOUSE_MOVE, _ibar_cb_icon_mouse_move, ic);
   evas_object_event_callback_add(ic->o_holder, EVAS_CALLBACK_MOVE, _ibar_cb_icon_move, ic);
   evas_object_event_callback_add(ic->o_holder, EVAS_CALLBACK_RESIZE, _ibar_cb_icon_resize, ic);
   evas_object_show(ic->o_holder);
   
   ic->o_holder2 = edje_object_add(evas_object_evas_get(b->o_box));
   e_theme_edje_object_set(ic->o_holder2, "base/theme/modules/ibar",
			   "modules/ibar/icon_overlay");
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
   e_object_unref(E_OBJECT(ic->app));
   free(ic);
}

static void
_ibar_icon_fill(IBar_Icon *ic)
{
   ic->o_icon = edje_object_add(evas_object_evas_get(ic->ibar->o_box));
   edje_object_file_set(ic->o_icon, ic->app->path, "icon");
   edje_object_part_swallow(ic->o_holder, "item", ic->o_icon);
   evas_object_pass_events_set(ic->o_icon, 1);
   evas_object_show(ic->o_icon);
   ic->o_icon2 = edje_object_add(evas_object_evas_get(ic->ibar->o_box));
   edje_object_file_set(ic->o_icon2, ic->app->path, "icon");
   edje_object_part_swallow(ic->o_holder2, "item", ic->o_icon2);
   evas_object_pass_events_set(ic->o_icon2, 1);
   evas_object_show(ic->o_icon2);
   
   edje_object_part_text_set(ic->o_holder, "label", ic->app->name);
   edje_object_part_text_set(ic->o_holder2, "label", ic->app->name);
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
_ibar_cb_app_change(void *data, E_App *a, E_App_Change ch)
{
   IBar *b;

   b = data;
   if (!b->apps) return;
   switch (ch)
     {
      case E_APP_ADD:
	if (e_app_is_parent(b->apps, a))
	  {
	     E_App *a2, *a_before = NULL;
	     IBar_Icon *ic2 = NULL, *ic;
	     Evas_List *l;
	     
	     ic = _ibar_icon_find(b, a);
	     if (!ic)
	       {
		  for (l = b->apps->subapps; l; l = l->next)
		    {
		       a2 = l->data;
		       if ((a2 == a) && (l->next))
			 {
			    a_before = l->next->data;
			    break;
			 }
		    }
		  ic = _ibar_icon_new(b, a);
		  if (a_before) ic2 = _ibar_icon_find(b, a_before);
		  if (ic2)
		    {
		       b->icons = evas_list_prepend_relative(b->icons, ic, ic2);
		       e_box_pack_before(b->o_box, ic->o_holder, ic2->o_holder);
		    }
		  else
		    {
		       b->icons = evas_list_append(b->icons, ic);
		       e_box_pack_end(b->o_box, ic->o_holder);
		    }
		  _ibar_empty_handle(b);
		  _ibar_resize_handle(b);
		  _gc_orient(b->inst->gcc);
	       }
	     else
	       {
		  /* FIXME: complain */
	       }
	  }
	break;
      case E_APP_DEL:
	if (e_app_is_parent(b->apps, a))
	  {
	     IBar_Icon *ic;
	     
	     ic = _ibar_icon_find(b, a);
	     if (ic)
	       {
		  b->icons = evas_list_remove(b->icons, ic);
		  _ibar_icon_free(ic);
	       }
	     _ibar_empty_handle(b);
	     _ibar_resize_handle(b);
	     _gc_orient(b->inst->gcc);
	  }
	break;
      case E_APP_CHANGE:
	if (e_app_is_parent(b->apps, a))
	  {
	     IBar_Icon *ic;
	     
	     ic = _ibar_icon_find(b, a);
	     if (ic)
	       {
		  _ibar_icon_empty(ic);
		  _ibar_icon_fill(ic);
	       }
	     _ibar_resize_handle(b);
	     _gc_orient(b->inst->gcc);
	  }
	break;
      case E_APP_ORDER:
	if (a == b->apps)
	  {
	     _ibar_empty(b);
	     _ibar_fill(b);
	     _ibar_resize_handle(b);
             _gc_orient(b->inst->gcc);
	  }
	break;
      case E_APP_EXEC:
	if (e_app_is_parent(b->apps, a))
	  {
	     IBar_Icon *ic;
	     
	     ic = _ibar_icon_find(b, a);
	     if (ic)
	       {
		  if (a->startup_notify)
		    _ibar_icon_signal_emit(ic, "start", "");
		  else
		    _ibar_icon_signal_emit(ic, "exec", "");
	       }
	  }
	break;
      case E_APP_READY:
      case E_APP_READY_EXPIRE:
      case E_APP_EXIT:
	if (e_app_is_parent(b->apps, a))
	  {
	     IBar_Icon *ic;
	     
	     ic = _ibar_icon_find(b, a);
	     if (ic) _ibar_icon_signal_emit(ic, "stop", "");
	  }
	break;
      default:
	break;
     }
}

static int
_ibar_cb_timer_drop_recalc(void *data)
{
   Instance *inst;
   
   inst = data;
   _ibar_instance_drop_zone_recalc(inst);
   return 1;
}

static void
_ibar_cb_obj_moveresize(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Instance *inst;
   
   inst = data;
   _ibar_resize_handle(inst->ibar);
   _ibar_instance_drop_zone_recalc(inst);
}

static void
_ibar_cb_menu_icon_properties(void *data, E_Menu *m, E_Menu_Item *mi)
{
   IBar_Icon *ic;
   
   ic = data;
   if (ic->app->orig)
     e_eap_edit_show(ic->ibar->inst->gcc->gadcon->zone->container,
		     ic->app->orig);
   else
     e_eap_edit_show(ic->ibar->inst->gcc->gadcon->zone->container,
		     ic->app);
}

static void
_ibar_cb_menu_icon_remove(void *data, E_Menu *m, E_Menu_Item *mi)
{
   IBar_Icon *ic;
   
   ic = data;
   ic->ibar->icons = evas_list_remove(ic->ibar->icons, ic);
   _ibar_resize_handle(ic->ibar);
   _gc_orient(ic->ibar->inst->gcc);
   e_app_remove(ic->app);
   _ibar_icon_free(ic);
}

static void
_ibar_cb_menu_configuration(void *data, E_Menu *m, E_Menu_Item *mi)
{
   IBar *b;
   Config_Item *ci;
   
   b = data;
   ci = _ibar_config_item_get(b->inst->gcc->id);
   _config_ibar_module(ci);
}

static void
_ibar_cb_menu_post(void *data, E_Menu *m)
{
   if (!ibar_config->menu) return;
   e_object_del(E_OBJECT(ibar_config->menu));
   ibar_config->menu = NULL;
}

static void
_ibar_cb_icon_mouse_in(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_In *ev;
   IBar_Icon *ic;
   
   ev = event_info;
   ic = data;
   _ibar_icon_signal_emit(ic, "active", "");
   if (ic->ibar->show_label)
     _ibar_icon_signal_emit(ic, "label_active", "");
}

static void
_ibar_cb_icon_mouse_out(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Out *ev;
   IBar_Icon *ic;
   
   ev = event_info;
   ic = data;
   _ibar_icon_signal_emit(ic, "passive", "");
   if (ic->ibar->show_label)
     _ibar_icon_signal_emit(ic, "label_passive", "");
}

static void
_ibar_cb_icon_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
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
     }
   else if ((ev->button == 3) && (!ibar_config->menu))
     {
	E_Menu *mn;
	E_Menu_Item *mi;
	int cx, cy, cw, ch;
	
	mn = e_menu_new();
	e_menu_post_deactivate_callback_set(mn, _ibar_cb_menu_post, NULL);
	ibar_config->menu = mn;

	/* FIXME: other icon options go here too */
	mi = e_menu_item_new(mn);
	e_menu_item_label_set(mi, _("Change Icon Properties"));
	e_util_menu_item_edje_icon_set(mi, "enlightenment/properties");
	e_menu_item_callback_set(mi, _ibar_cb_menu_icon_properties, ic);
	
	mi = e_menu_item_new(mn);
	e_menu_item_label_set(mi, _("Remove Icon"));
	e_util_menu_item_edje_icon_set(mi, "enlightenment/delete");
	e_menu_item_callback_set(mi, _ibar_cb_menu_icon_remove, ic);
	
	mi = e_menu_item_new(mn);
	e_menu_item_separator_set(mi, 1);
	
	mi = e_menu_item_new(mn);
	e_menu_item_label_set(mi, _("Configuration"));
	e_util_menu_item_edje_icon_set(mi, "enlightenment/configuration");
	e_menu_item_callback_set(mi, _ibar_cb_menu_configuration, ic->ibar);
	
	e_gadcon_client_util_menu_items_append(ic->ibar->inst->gcc, mn, 0);
	
	e_gadcon_canvas_zone_geometry_get(ic->ibar->inst->gcc->gadcon,
					  &cx, &cy, &cw, &ch);
	e_menu_activate_mouse(mn,
			      e_util_zone_current_get(e_manager_current_get()),
			      cx + ev->output.x, cy + ev->output.y, 1, 1,
			      E_MENU_POP_DIRECTION_DOWN, ev->timestamp);
	evas_event_feed_mouse_up(ic->ibar->inst->gcc->gadcon->evas, ev->button,
				 EVAS_BUTTON_NONE, ev->timestamp, NULL);
     }
}

static void
_ibar_cb_icon_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Up *ev;
   IBar_Icon *ic;
   
   ev = event_info;
   ic = data;
   if ((ev->button == 1) && (!ic->drag.dnd))
     {
	e_zone_app_exec(ic->ibar->inst->gcc->gadcon->zone, ic->app);
	e_exehist_add("ibar", ic->app->exe);
	ic->drag.start = 0;
	ic->drag.dnd = 0;
     }
}

static void
_ibar_cb_icon_mouse_move(void *data, Evas *e, Evas_Object *obj, void *event_info)
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
	     const char *drag_types[] = { "enlightenment/eapp" };

	     ic->drag.dnd = 1;
	     ic->drag.start = 0;

	     evas_object_geometry_get(ic->o_icon, &x, &y, &w, &h);
	     d = e_drag_new(ic->ibar->inst->gcc->gadcon->zone->container,
			    x, y, drag_types, 1,
			    ic->app, -1, _ibar_cb_drag_finished);
	     o = edje_object_add(e_drag_evas_get(d));
	     edje_object_file_set(o, ic->app->path, "icon");
	     e_drag_object_set(d, o);

	     e_drag_resize(d, w, h);
	     e_drag_start(d, ic->drag.x, ic->drag.y);
	     evas_event_feed_mouse_up(ic->ibar->inst->gcc->gadcon->evas,
				      1, EVAS_BUTTON_NONE, 
				      ecore_x_current_time_get(), NULL);
	     e_object_ref(E_OBJECT(ic->app));
	     ic->ibar->icons = evas_list_remove(ic->ibar->icons, ic);
	     _ibar_resize_handle(ic->ibar);
	     _gc_orient(ic->ibar->inst->gcc);
	     e_app_remove(ic->app);
	     _ibar_icon_free(ic);
	  }
     }
}

static void
_ibar_cb_icon_move(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   IBar_Icon *ic;
   Evas_Coord x, y;
   
   ic = data;
   evas_object_geometry_get(ic->o_holder, &x, &y, NULL, NULL);
   evas_object_move(ic->o_holder2, x, y);
   evas_object_raise(ic->o_holder2);
}

static void
_ibar_cb_icon_resize(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   IBar_Icon *ic;
   Evas_Coord w, h;
   
   ic = data;
   evas_object_geometry_get(ic->o_holder, NULL, NULL, &w, &h);
   evas_object_resize(ic->o_holder2, w, h);
   evas_object_raise(ic->o_holder2);
}

static void
_ibar_cb_drag_finished(E_Drag *drag, int dropped)
{
   e_object_unref(E_OBJECT(drag->data));
}

static void
_ibar_cb_drop_move(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   IBar *b;
   Evas_Coord x, y;
   
   b = data;
   evas_object_geometry_get(b->o_drop, &x, &y, NULL, NULL);
   evas_object_move(b->o_drop_over, x, y);
}

static void
_ibar_cb_drop_resize(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   IBar *b;
   Evas_Coord w, h;
   
   b = data;
   evas_object_geometry_get(b->o_drop, NULL, NULL, &w, &h);
   evas_object_resize(b->o_drop_over, w, h);
}

static void
_ibar_inst_cb_enter(void *data, const char *type, void *event_info)
{
   E_Event_Dnd_Enter *ev;
   Instance *inst;
   Evas_Object *o, *o2;
   IBar_Icon *ic;
   int cx, cy, cw, ch;
   
   ev = event_info;
   inst = data;
   o = edje_object_add(evas_object_evas_get(inst->ibar->o_box));
   inst->ibar->o_drop = o;
   o2 = edje_object_add(evas_object_evas_get(inst->ibar->o_box));
   inst->ibar->o_drop_over = o2;
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOVE, _ibar_cb_drop_move, inst->ibar);
   evas_object_event_callback_add(o, EVAS_CALLBACK_RESIZE, _ibar_cb_drop_resize, inst->ibar);
   e_theme_edje_object_set(o, "base/theme/modules/ibar",
			   "modules/ibar/drop");
   e_theme_edje_object_set(o2, "base/theme/modules/ibar",
			   "modules/ibar/drop_overlay");
   evas_object_layer_set(o2, 19999);
   evas_object_show(o);
   evas_object_show(o2);
   e_gadcon_canvas_zone_geometry_get(inst->gcc->gadcon, &cx, &cy, &cw, &ch);
   ic = _ibar_icon_at_coord(inst->ibar, ev->x - cx, ev->y - cy);
   inst->ibar->ic_drop_before = ic;
   if (ic)
     {
	Evas_Coord ix, iy, iw, ih;
	int before = 0;
	
	evas_object_geometry_get(ic->o_holder, &ix, &iy, &iw, &ih);
	if (e_box_orientation_get(inst->ibar->o_box))
	  {
	     if ((ev->x - cx) < (ix + (iw / 2))) before = 1;
	  }
	else
	  {
	     if ((ev->y - cy) < (iy + (ih / 2))) before = 1;
	  }
	if (before)
	  e_box_pack_before(inst->ibar->o_box, inst->ibar->o_drop, ic->o_holder);
	else
	  e_box_pack_after(inst->ibar->o_box, inst->ibar->o_drop, ic->o_holder);
	inst->ibar->drop_before = before;
     }
   else e_box_pack_end(inst->ibar->o_box, o);
   e_box_pack_options_set(o,
			  1, 1, /* fill */
			  0, 0, /* expand */
			  0.5, 0.5, /* align */
			  1, 1, /* min */
			  -1, -1 /* max */
			  );
   _ibar_resize_handle(inst->ibar);
   _gc_orient(inst->gcc);
}

static void
_ibar_inst_cb_move(void *data, const char *type, void *event_info)
{
   E_Event_Dnd_Move *ev;
   Instance *inst;
   IBar_Icon *ic;
   int cx, cy, cw, ch;
   
   ev = event_info;
   inst = data;
   e_box_unpack(inst->ibar->o_drop);
   e_gadcon_canvas_zone_geometry_get(inst->gcc->gadcon, &cx, &cy, &cw, &ch);
   ic = _ibar_icon_at_coord(inst->ibar, ev->x - cx, ev->y - cy);
   inst->ibar->ic_drop_before = ic;
   if (ic)
     {
	Evas_Coord ix, iy, iw, ih;
	int before = 0;
	
	evas_object_geometry_get(ic->o_holder, &ix, &iy, &iw, &ih);
	if (e_box_orientation_get(inst->ibar->o_box))
	  {
	     if ((ev->x - cx) < (ix + (iw / 2))) before = 1;
	  }
	else
	  {
	     if ((ev->y - cy) < (iy + (ih / 2))) before = 1;
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
			  0, 0, /* expand */
			  0.5, 0.5, /* align */
			  1, 1, /* min */
			  -1, -1 /* max */
			  );
   _ibar_resize_handle(inst->ibar);
   _gc_orient(inst->gcc);
}

static void
_ibar_inst_cb_leave(void *data, const char *type, void *event_info)
{
   E_Event_Dnd_Leave *ev;
   Instance *inst;
   
   ev = event_info;
   inst = data;
   inst->ibar->ic_drop_before = NULL;
   evas_object_del(inst->ibar->o_drop);
   inst->ibar->o_drop = NULL;
   evas_object_del(inst->ibar->o_drop_over);
   inst->ibar->o_drop_over = NULL;
   _ibar_resize_handle(inst->ibar);
   _gc_orient(inst->gcc);
}

static void
_ibar_inst_cb_drop(void *data, const char *type, void *event_info)
{
   E_Event_Dnd_Drop *ev;
   Instance *inst;
   E_App *app = NULL;
   Evas_List *l = NULL;
   IBar_Icon *ic;
   
   ev = event_info;
   inst = data;
   if (!strcmp(type, "enlightenment/eapp"))
     {
	app = ev->data;
     }
   else if (!strcmp(type, "enlightenment/border"))
     {
	E_Border *bd;

	bd = ev->data;
	app = bd->app;

	if (!app)
	  {
	     app = e_app_window_name_class_title_role_find(bd->client.icccm.name,
							   bd->client.icccm.class,
							   e_border_name_get(bd),
							   bd->client.icccm.window_role);
	  }
	if (!app)
	  {
	     app = e_app_launch_id_pid_find(bd->client.netwm.startup_id,
					    bd->client.netwm.pid);
	  }
	if (!app)
	  {
	     E_Dialog *dia;

	     dia = e_dialog_new(e_container_current_get(e_manager_current_get()));
	     e_dialog_title_set(dia, _("Cannot add icon"));
	     e_dialog_text_set(dia,
			       _("You tried to drop an icon of an application that<br>"
				 "does not have a matching application file.<br>"
				 "<br>"
				 "The icon cannot be added to IBar."
				 ));
	     e_dialog_button_add(dia, _("OK"), NULL, NULL, NULL);
	     e_dialog_button_focus_num(dia, 1);
	     e_win_centered_set(dia->win, 1);
	     e_dialog_show(dia);
	     return;
	  }
     }
   else if (!strcmp(type, "text/uri-list"))
     {
	l = ev->data;
     }
   
   ic = inst->ibar->ic_drop_before;
   if (ic)
     {
	/* Add new eapp before this icon */
	if (!inst->ibar->drop_before)
	  {
	     for (l = inst->ibar->icons; l; l = l->next)
	       {
		  if (l->data == ic)
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
	  e_app_list_prepend_relative(app, ic->app);
	else if (l)
	  e_app_files_list_prepend_relative(l, ic->app);
     }
   else
     {
	atend:
	if (inst->ibar->apps)
	  {
	     if (app)
	       e_app_list_append(app, inst->ibar->apps);
	     else if (l)
	       e_app_files_list_append(l, inst->ibar->apps);
	  }
     }
   
   evas_object_del(inst->ibar->o_drop);
   inst->ibar->o_drop = NULL;
   evas_object_del(inst->ibar->o_drop_over);
   inst->ibar->o_drop_over = NULL;
   _ibar_empty_handle(inst->ibar);
   _ibar_resize_handle(inst->ibar);
   _gc_orient(inst->gcc);
}

/***************************************************************************/
/**/
/* module setup */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
     "IBar"
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
	ci->id = evas_stringshare_add("0");
	ci->dir = evas_stringshare_add("default");
	ci->show_label = 1;
	
	ibar_config->items = evas_list_append(ibar_config->items, ci);
     }
   
   ibar_config->module = m;
   
   e_gadcon_provider_register(&_gadcon_class);
   return ibar_config;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   e_gadcon_provider_unregister(&_gadcon_class);
   
   if (ibar_config->config_dialog)
     e_object_del(E_OBJECT(ibar_config->config_dialog));
   while (ibar_config->handlers)
     {
	ecore_event_handler_del(ibar_config->handlers->data);
	ibar_config->handlers = evas_list_remove_list(ibar_config->handlers, ibar_config->handlers);
     }
   if (ibar_config->menu)
     {
	e_menu_post_deactivate_callback_set(ibar_config->menu, NULL, NULL);
	e_object_del(E_OBJECT(ibar_config->menu));
	ibar_config->menu = NULL;
     }
   while (ibar_config->items)
     {
	Config_Item *ci;
	
	ci = ibar_config->items->data;
	ibar_config->items = evas_list_remove_list(ibar_config->items, ibar_config->items);
	if (ci->id) evas_stringshare_del(ci->id);
	if (ci->dir) evas_stringshare_del(ci->dir);
	free(ci);
     }
   free(ibar_config);
   ibar_config = NULL;
   E_CONFIG_DD_FREE(conf_item_edd);
   E_CONFIG_DD_FREE(conf_edd);
   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   Evas_List *l;
   
   for (l = ibar_config->instances; l; l = l->next)
     {
	Instance *inst;
	Config_Item *ci;
	
	inst = l->data;
	ci = _ibar_config_item_get(inst->gcc->id);
	if (ci->dir) evas_stringshare_del(ci->dir);
	/* FIXME: path should be recorded from setup */
	ci->dir = evas_stringshare_add(inst->dir);
     }
   e_config_domain_save("module.ibar", conf_edd, ibar_config);
   return 1;
}

EAPI int
e_modapi_about(E_Module *m)
{
   e_module_dialog_show(_("Enlightenment IBar Module"),
			_("This is the IBar Application Launcher bar module for Enlightenment.<br>"
			  "It is a first example module and is being used to flesh out several<br>"
			  "interfaces in Enlightenment 0.17.0. It is under heavy development,<br>"
			  "so expect it to <hilight>break often</hilight> and change as it improves."));
   return 1;
}

/**/
/***************************************************************************/
