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
/* and actually define the gadcon class that this module provides (just 1) */
static const E_Gadcon_Client_Class _gadcon_class =
{
   GADCON_CLIENT_CLASS_VERSION,
     "start",
     {
	_gc_init, _gc_shutdown, _gc_orient
     }
};
/**/
/***************************************************************************/

/***************************************************************************/
/**/
/* actual module specifics */

typedef struct _Instance Instance;

struct _Instance
{
   E_Gadcon_Client *gcc;
   Evas_Object     *o_button;
   E_Menu          *main_menu;
};

static void _button_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _menu_cb_post(void *data, E_Menu *m);

static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, char *name, char *id, char *style)
{
   Evas_Object *o;
   E_Gadcon_Client *gcc;
   Instance *inst;
   
   inst = E_NEW(Instance, 1);
   
   o = edje_object_add(gc->evas);
   e_theme_edje_object_set(o, "base/theme/modules/start", "modules/start/main");
   edje_object_signal_emit(o, "passive", "");
   
   gcc = e_gadcon_client_new(gc, name, id, style, o);
   gcc->data = inst;
   
   inst->gcc = gcc;
   inst->o_button = o;
   inst->main_menu = NULL;
   
   e_gadcon_client_util_menu_attach(gcc);
   
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN,
				  _button_cb_mouse_down, inst);
   return gcc;
}
    
static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
   Instance *inst;
   
   inst = gcc->data;
   evas_object_del(inst->o_button);
   free(inst);
}
    
static void
_gc_orient(E_Gadcon_Client *gcc)
{
   Instance *inst;
   
   inst = gcc->data;
   e_gadcon_client_aspect_set(gcc, 16, 16);
   e_gadcon_client_min_size_set(gcc, 16, 16);
}
/**/
/***************************************************************************/

/***************************************************************************/
/**/
static void
_button_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Instance *inst;
   Evas_Event_Mouse_Down *ev;
   
   inst = data;
   ev = event_info;
   if (ev->button == 1)
     {
	Evas_Coord x, y, w, h;
	int cx, cy, cw, ch;
	
	evas_object_geometry_get(inst->o_button, &x, &y, &w, &h); 
	e_gadcon_canvas_zone_geometry_get(inst->gcc->gadcon,
					  &cx, &cy, &cw, &ch);
	x += cx;
	y += cy;
	if (!inst->main_menu)
	  inst->main_menu = e_int_menus_main_new();
	if (inst->main_menu)
	  {
	     int dir;
	     
	     e_menu_post_deactivate_callback_set(inst->main_menu,
						 _menu_cb_post,
						 inst);
	     dir = E_MENU_POP_DIRECTION_AUTO;
	     switch (inst->gcc->gadcon->orient)
	       {
		case E_GADCON_ORIENT_TOP:
		  dir = E_MENU_POP_DIRECTION_DOWN;
		  break;
		case E_GADCON_ORIENT_BOTTOM:
		  dir = E_MENU_POP_DIRECTION_UP;
		  break;
		case E_GADCON_ORIENT_LEFT:
		  dir = E_MENU_POP_DIRECTION_RIGHT;
		  break;
		case E_GADCON_ORIENT_RIGHT:
		  dir = E_MENU_POP_DIRECTION_LEFT;
		  break;
		case E_GADCON_ORIENT_FLOAT:
		case E_GADCON_ORIENT_CORNER_TL:
		case E_GADCON_ORIENT_CORNER_TR:
		case E_GADCON_ORIENT_CORNER_BL:
		case E_GADCON_ORIENT_CORNER_BR:
		default:
		  dir = E_MENU_POP_DIRECTION_AUTO;
		  break;
	       }
	     e_menu_activate_mouse(inst->main_menu,
				   e_util_zone_current_get(e_manager_current_get()),
				   x, y, w, h,
				   dir, ev->timestamp);
	     edje_object_signal_emit(inst->o_button, "active", "");
	     evas_event_feed_mouse_up(inst->gcc->gadcon->evas, ev->button,
				      EVAS_BUTTON_NONE, ev->timestamp, NULL);
	  }
     }
}

static void
_menu_cb_post(void *data, E_Menu *m)
{
   Instance *inst;
   
   inst = data;
   if (!inst->main_menu) return;
   edje_object_signal_emit(inst->o_button, "passive", "");
   e_object_del(E_OBJECT(inst->main_menu));
   inst->main_menu = NULL;
}
/**/
/***************************************************************************/

/***************************************************************************/
/**/
/* module setup */
EAPI E_Module_Api e_modapi = 
{
   E_MODULE_API_VERSION,
     "Start"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   e_gadcon_provider_register(&_gadcon_class);
   return 1;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   e_gadcon_provider_unregister(&_gadcon_class);
   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
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
   e_module_dialog_show(_("Enlightenment Start Module"),
			_("Experimental Button module for E17"));
   return 1;
}
/**/
/***************************************************************************/
