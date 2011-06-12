#include "e.h"
#include "e_mod_main.h"

/* gadcon requirements */
static E_Gadcon_Client *_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style);
static void _gc_shutdown(E_Gadcon_Client *gcc);
static void _gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient);
static char *_gc_label(E_Gadcon_Client_Class *client_class);
static Evas_Object *_gc_icon(E_Gadcon_Client_Class *client_class, Evas *evas);
static const char *_gc_id_new(E_Gadcon_Client_Class *client_class);

/* and actually define the gadcon class that this module provides (just 1) */
static const E_Gadcon_Client_Class _gadcon_class =
{
   GADCON_CLIENT_CLASS_VERSION,
     "backlight",
     {
        _gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL, NULL
     },
   E_GADCON_CLIENT_STYLE_PLAIN
};

/* actual module specifics */
typedef struct _Instance Instance;

struct _Instance
{
   E_Gadcon_Client *gcc;
   Evas_Object     *o_backlight, *o_table, *o_slider;
   E_Gadcon_Popup  *popup;
   E_Menu          *menu;
   double           val;
};

static Eina_List *backlight_instances = NULL;
static E_Module *backlight_module = NULL;

static void
_backlight_settings_cb(void *d1, void *d2 __UNUSED__)
{
   Instance *inst = d1;
   e_configure_registry_call("screen/power_management",
                             inst->gcc->gadcon->zone->container, NULL);
   e_object_del(E_OBJECT(inst->popup));
   inst->popup = NULL;
}

static void
_slider_cb(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Instance *inst = data;
   e_backlight_mode_set(inst->gcc->gadcon->zone, E_BACKLIGHT_MODE_NORMAL);
   e_backlight_level_set(inst->gcc->gadcon->zone, inst->val, 0.0);
}

static void
_backlight_popup_new(Instance *inst)
{
   Evas *evas;
   Evas_Object *o;
   
   if (inst->popup) return;

   e_backlight_update();
   e_backlight_mode_set(inst->gcc->gadcon->zone, E_BACKLIGHT_MODE_NORMAL);
   inst->val = e_backlight_level_get(inst->gcc->gadcon->zone);
   
   inst->popup = e_gadcon_popup_new(inst->gcc);
   evas = inst->popup->win->evas;
   
   inst->o_table = e_widget_table_add(evas, 0);

   o = e_widget_slider_add(evas, 0, 0, NULL, 0.0, 1.0, 0.0, 0, &(inst->val), NULL, 200);
   evas_object_smart_callback_add(o, "changed", _slider_cb, inst);
   inst->o_slider = o;
   e_widget_table_object_align_append(inst->o_table, o, 
                                      0, 0, 1, 1, 0, 0, 0, 0, 0.5, 0.5);
   
   o = e_widget_button_add(evas, NULL, "preferences-system",
                           _backlight_settings_cb, inst, NULL);
   e_widget_table_object_align_append(inst->o_table, o, 
                                      0, 1, 1, 1, 0, 0, 0, 0, 0.5, 1.0);
   
   e_gadcon_popup_content_set(inst->popup, inst->o_table);
   e_gadcon_popup_show(inst->popup);
}

static void
_backlight_popup_free(Instance *inst)
{
   if (!inst->popup) return;
   if (inst->popup) e_object_del(E_OBJECT(inst->popup));
   inst->popup = NULL;
}

static void
_backlight_menu_cb_post(void *data, E_Menu *menu __UNUSED__)
{
   Instance *inst = data;
   if ((!inst) || (!inst->menu))
      return;
   if (inst->menu)
     {
        e_object_del(E_OBJECT(inst->menu));
        inst->menu = NULL;
     }
}

static void
_backlight_menu_cb_cfg(void *data, E_Menu *menu __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   Instance *inst = data;
   E_Container *con;
   
   if (inst->popup)
     {
        e_object_del(E_OBJECT(inst->popup));
        inst->popup = NULL;
     }
   con = e_container_current_get(e_manager_current_get());
//   e_int_config_backlight_module(con, NULL);
}

static void
_backlight_cb_mouse_down(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event)
{
   Instance *inst = data;
   Evas_Event_Mouse_Down *ev = event;
   
   if (ev->button == 1)
     {
        if (inst->popup) _backlight_popup_free(inst);
        else _backlight_popup_new(inst);
     }
   else if ((ev->button == 3) && (!inst->menu))
     {
        E_Zone *zone;
        E_Menu *m;
        E_Menu_Item *mi;
        int x, y;
        
        zone = e_util_zone_current_get(e_manager_current_get());
        
        m = e_menu_new();
        
        mi = e_menu_item_new(m);
        e_menu_item_label_set(mi, _("Settings"));
        e_util_menu_item_theme_icon_set(mi, "configure");
        e_menu_item_callback_set(mi, _backlight_menu_cb_cfg, inst);
        
        m = e_gadcon_client_util_menu_items_append(inst->gcc, m, 0);
        e_menu_post_deactivate_callback_set(m, _backlight_menu_cb_post, inst);
        inst->menu = m;
        
        e_gadcon_canvas_zone_geometry_get(inst->gcc->gadcon, &x, &y, NULL, NULL);
        e_menu_activate_mouse(m, zone, x + ev->output.x, y + ev->output.y,
                              1, 1, E_MENU_POP_DIRECTION_AUTO, ev->timestamp);
        evas_event_feed_mouse_up(inst->gcc->gadcon->evas, ev->button,
                                 EVAS_BUTTON_NONE, ev->timestamp, NULL);
     }
}

static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style)
{
   Evas_Object *o;
   E_Gadcon_Client *gcc;
   Instance *inst;
   
   inst = E_NEW(Instance, 1);

   o = edje_object_add(gc->evas);
   e_theme_edje_object_set(o, "base/theme/modules/backlight",
                           "e/modules/backlight/main");
   evas_object_show(o);
   
   gcc = e_gadcon_client_new(gc, name, id, style, o);
   gcc->data = inst;
   
   inst->gcc = gcc;
   inst->o_backlight = o;

   e_backlight_update();
   inst->val = e_backlight_level_get(inst->gcc->gadcon->zone);
   
   evas_object_event_callback_add(inst->o_backlight, 
                                  EVAS_CALLBACK_MOUSE_DOWN,
                                  _backlight_cb_mouse_down,
                                  inst);
   
   e_gadcon_client_util_menu_attach(gcc);

   backlight_instances = eina_list_append(backlight_instances, inst);
   return gcc;
}

static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
   Instance *inst;
   
   inst = gcc->data;
   if (inst->menu)
     {
        e_object_del(E_OBJECT(inst->menu));
        inst->menu = NULL;
     }
   backlight_instances = eina_list_remove(backlight_instances, inst);
   evas_object_del(inst->o_backlight);
   _backlight_popup_free(inst);
   free(inst);
}

static void
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient __UNUSED__)
{
   Instance *inst;
   Evas_Coord mw, mh;
   
   inst = gcc->data;
   mw = 0, mh = 0;
   edje_object_size_min_get(inst->o_backlight, &mw, &mh);
   if ((mw < 1) || (mh < 1))
     edje_object_size_min_calc(inst->o_backlight, &mw, &mh);
   if (mw < 4) mw = 4;
   if (mh < 4) mh = 4;
   e_gadcon_client_aspect_set(gcc, mw, mh);
   e_gadcon_client_min_size_set(gcc, mw, mh);
}

static char *
_gc_label(E_Gadcon_Client_Class *client_class __UNUSED__)
{
   return _("Backlight");
}

static Evas_Object *
_gc_icon(E_Gadcon_Client_Class *client_class __UNUSED__, Evas *evas)
{
   Evas_Object *o;
   char buf[4096];
   
   o = edje_object_add(evas);
   snprintf(buf, sizeof(buf), "%s/e-module-backlight.edj",
	    e_module_dir_get(backlight_module));
   edje_object_file_set(o, buf, "icon");
   return o;
}

static const char *
_gc_id_new(E_Gadcon_Client_Class *client_class __UNUSED__)
{
   return _gadcon_class.name;
}

/* module setup */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
     "Backlight"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   backlight_module = m;
   e_gadcon_provider_register(&_gadcon_class);
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   backlight_module = NULL;
   e_gadcon_provider_unregister(&_gadcon_class);
   return 1;
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   return 1;
}
