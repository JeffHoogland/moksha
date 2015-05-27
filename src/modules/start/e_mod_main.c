/**
 * @addtogroup Optional_Gadgets
 * @{
 *
 * @defgroup Module_Start Start Button
 *
 * Shows a "start here" button or icon.
 *
 * @}
 */

#include "e.h"

/* gadcon requirements */
static E_Gadcon_Client *_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style);
static void _gc_shutdown(E_Gadcon_Client *gcc);
static void _gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient);
static const char *_gc_label(const E_Gadcon_Client_Class *client_class);
static Evas_Object *_gc_icon(const E_Gadcon_Client_Class *client_class, Evas *evas);
static const char *_gc_id_new(const E_Gadcon_Client_Class *client_class);
/* and actually define the gadcon class that this module provides (just 1) */
static const E_Gadcon_Client_Class _gadcon_class =
{
   GADCON_CLIENT_CLASS_VERSION,
     "start",
     {
        _gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL,
        e_gadcon_site_is_not_toolbar
     },
   E_GADCON_CLIENT_STYLE_PLAIN
};

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

static E_Module *start_module = NULL;

static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style)
{
   Evas_Object *o;
   E_Gadcon_Client *gcc;
   Instance *inst;

   inst = E_NEW(Instance, 1);

   o = edje_object_add(gc->evas);
   e_theme_edje_object_set(o, "base/theme/modules/start", 
                           "e/modules/start/main");
   edje_object_signal_emit(o, "e,state,unfocused", "e");

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
   if (inst->main_menu)
     {
	e_menu_post_deactivate_callback_set(inst->main_menu, NULL, NULL);
	e_object_del(E_OBJECT(inst->main_menu));
	inst->main_menu = NULL;
     }
   evas_object_del(inst->o_button);
   free(inst);
}
    
static void
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient)
{
   Instance *inst;
   Evas_Coord mw, mh;
   char buf[4096];
   const char *s = "float";
   
   inst = gcc->data;
   switch (orient)
     {
      case E_GADCON_ORIENT_FLOAT:
        s = "float";
        break;
      case E_GADCON_ORIENT_HORIZ:
        s = "horizontal";
        break;
      case E_GADCON_ORIENT_VERT:
        s = "vertical";
        break;
      case E_GADCON_ORIENT_LEFT:
        s = "left";
        break;
      case E_GADCON_ORIENT_RIGHT:
        s = "right";
        break;
      case E_GADCON_ORIENT_TOP:
        s = "top";
        break;
      case E_GADCON_ORIENT_BOTTOM:
        s = "bottom";
        break;
      case E_GADCON_ORIENT_CORNER_TL:
        s = "top_left";
        break;
      case E_GADCON_ORIENT_CORNER_TR:
        s = "top_right";
        break;
      case E_GADCON_ORIENT_CORNER_BL:
        s = "bottom_left";
        break;
      case E_GADCON_ORIENT_CORNER_BR:
        s = "bottom_right";
        break;
      case E_GADCON_ORIENT_CORNER_LT:
        s = "left_top";
        break;
      case E_GADCON_ORIENT_CORNER_RT:
        s = "right_top";
        break;
      case E_GADCON_ORIENT_CORNER_LB:
        s = "left_bottom";
        break;
      case E_GADCON_ORIENT_CORNER_RB:
        s = "right_bottom";
        break;
      default:
        break;
     }
   snprintf(buf, sizeof(buf), "e,state,orientation,%s", s);
   edje_object_signal_emit(inst->o_button, buf, "e");
   edje_object_message_signal_process(inst->o_button);
   
   mw = 0, mh = 0;
   edje_object_size_min_get(inst->o_button, &mw, &mh);
   if ((mw < 1) || (mh < 1))
     edje_object_size_min_calc(inst->o_button, &mw, &mh);
   if (mw < 4) mw = 4;
   if (mh < 4) mh = 4;
   e_gadcon_client_aspect_set(gcc, mw, mh);
   e_gadcon_client_min_size_set(gcc, mw, mh);
}
   
static const char *
_gc_label(const E_Gadcon_Client_Class *client_class __UNUSED__)
{
   return _("Start");
}

static Evas_Object *
_gc_icon(const E_Gadcon_Client_Class *client_class __UNUSED__, Evas *evas)
{
   Evas_Object *o;
   char buf[PATH_MAX];

   o = edje_object_add(evas);
   snprintf(buf, sizeof(buf), "%s/e-module-start.edj",
	    e_module_dir_get(start_module));
   edje_object_file_set(o, buf, "icon");
   return o;
}

static const char *
_gc_id_new(const E_Gadcon_Client_Class *client_class __UNUSED__)
{
   return _gadcon_class.name;
}

static void
_button_cb_mouse_down(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Instance *inst;
   Evas_Event_Mouse_Down *ev;

   inst = data;
   ev = event_info;
   if (ev->button == 1)
     {
	Evas_Coord x, y, w, h;
	int cx, cy;

	evas_object_geometry_get(inst->o_button, &x, &y, &w, &h); 
	e_gadcon_canvas_zone_geometry_get(inst->gcc->gadcon,
					  &cx, &cy, NULL, NULL);
	x += cx;
	y += cy;
	if (!inst->main_menu)
	  inst->main_menu = e_int_menus_main_new();
	if (inst->main_menu)
	  {
	     int dir;

	     e_menu_post_deactivate_callback_set(inst->main_menu,
						 _menu_cb_post, inst);
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
		case E_GADCON_ORIENT_CORNER_TL:
		  dir = E_MENU_POP_DIRECTION_DOWN;
		  break;
		case E_GADCON_ORIENT_CORNER_TR:
		  dir = E_MENU_POP_DIRECTION_DOWN;
		  break;
		case E_GADCON_ORIENT_CORNER_BL:
		  dir = E_MENU_POP_DIRECTION_UP;
		  break;
		case E_GADCON_ORIENT_CORNER_BR:
		  dir = E_MENU_POP_DIRECTION_UP;
		  break;
		case E_GADCON_ORIENT_CORNER_LT:
		  dir = E_MENU_POP_DIRECTION_RIGHT;
		  break;
		case E_GADCON_ORIENT_CORNER_RT:
		  dir = E_MENU_POP_DIRECTION_LEFT;
		  break;
		case E_GADCON_ORIENT_CORNER_LB:
		  dir = E_MENU_POP_DIRECTION_RIGHT;
		  break;
		case E_GADCON_ORIENT_CORNER_RB:
		  dir = E_MENU_POP_DIRECTION_LEFT;
		  break;
		case E_GADCON_ORIENT_FLOAT:
		case E_GADCON_ORIENT_HORIZ:
		case E_GADCON_ORIENT_VERT:
		default:
		  dir = E_MENU_POP_DIRECTION_AUTO;
		  break;
	       }

	     e_gadcon_locked_set(inst->gcc->gadcon, 1);
	     e_menu_activate_mouse(inst->main_menu,
				   e_util_zone_current_get(e_manager_current_get()),
				   x, y, w, h, dir, ev->timestamp);
	     edje_object_signal_emit(inst->o_button, "e,state,focused", "e");
	  }
     }
}

static void
_menu_cb_post(void *data, E_Menu *m __UNUSED__)
{
   Instance *inst;

   inst = data;
   if (stopping || (!inst->main_menu)) return;
   e_gadcon_locked_set(inst->gcc->gadcon, 0);
   edje_object_signal_emit(inst->o_button, "e,state,unfocused", "e");
   e_object_del(E_OBJECT(inst->main_menu));
   inst->main_menu = NULL;
}

/* module setup */
EAPI E_Module_Api e_modapi = 
{
   E_MODULE_API_VERSION,
     "Start"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   start_module = m;
   e_gadcon_provider_register(&_gadcon_class);
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   start_module = NULL;
   e_gadcon_provider_unregister(&_gadcon_class);
   return 1;
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   return 1;
}
