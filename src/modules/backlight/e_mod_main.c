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
   Ecore_X_Window   input_win;
   Ecore_Event_Handler *hand_mouse_down;
   Ecore_Event_Handler *hand_key_down;
};

static Eina_List *backlight_instances = NULL;
static E_Module *backlight_module = NULL;
static E_Action *act = NULL;

static void _backlight_popup_free(Instance *inst);

static void
_backlight_gadget_update(Instance *inst)
{
   Edje_Message_Float msg;
   
   msg.val = inst->val;
   if (msg.val < 0.0) msg.val = 0.0;
   else if (msg.val > 1.0) msg.val = 1.0;
   edje_object_message_send(inst->o_backlight, EDJE_MESSAGE_FLOAT, 0, &msg);
}

static void
_backlight_input_win_del(Instance *inst)
{
   e_grabinput_release(0, inst->input_win);
   ecore_x_window_free(inst->input_win);
   inst->input_win = 0;
   ecore_event_handler_del(inst->hand_mouse_down);
   inst->hand_mouse_down = NULL;
   ecore_event_handler_del(inst->hand_key_down);
   inst->hand_key_down = NULL;
}

static Eina_Bool
_backlight_input_win_mouse_down_cb(void *data, int type __UNUSED__, void *event)
{
   Ecore_Event_Mouse_Button *ev = event;
   Instance *inst = data;
   
   if (ev->window != inst->input_win) return ECORE_CALLBACK_PASS_ON;
   _backlight_popup_free(inst);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_backlight_input_win_key_down_cb(void *data, int type __UNUSED__, void *event)
{
   Ecore_Event_Key *ev = event;
   Instance *inst = data;
   const char *keysym;
   
   if (ev->window != inst->input_win) return ECORE_CALLBACK_PASS_ON;
   
   keysym = ev->key;
   if (!strcmp(keysym, "Escape"))
      _backlight_popup_free(inst);
   else if ((!strcmp(keysym, "Up")) ||
            (!strcmp(keysym, "Left")) ||
            (!strcmp(keysym, "KP_Up")) ||
            (!strcmp(keysym, "KP_Left")) ||
            (!strcmp(keysym, "w")) ||
            (!strcmp(keysym, "d")) ||
            (!strcmp(keysym, "bracketright")) ||
            (!strcmp(keysym, "Prior")))
     {
        double v = inst->val + 0.1;
        if (v > 1.0) v = 1.0;
        e_widget_slider_value_double_set(inst->o_slider, v);
        inst->val = v;
        e_backlight_mode_set(inst->gcc->gadcon->zone, E_BACKLIGHT_MODE_NORMAL);
        e_backlight_level_set(inst->gcc->gadcon->zone, v, 0.0);
        _backlight_gadget_update(inst);
     }
   else if ((!strcmp(keysym, "Down")) ||
            (!strcmp(keysym, "Right")) ||
            (!strcmp(keysym, "KP_Down")) ||
            (!strcmp(keysym, "KP_Right")) ||
            (!strcmp(keysym, "s")) ||
            (!strcmp(keysym, "a")) ||
            (!strcmp(keysym, "bracketleft")) ||
            (!strcmp(keysym, "Next")))
     {
        double v = inst->val - 0.1;
        if (v < 0.0) v = 0.0;
        e_widget_slider_value_double_set(inst->o_slider, v);
        inst->val = v;
        e_backlight_mode_set(inst->gcc->gadcon->zone, E_BACKLIGHT_MODE_NORMAL);
        e_backlight_level_set(inst->gcc->gadcon->zone, v, 0.0);
        _backlight_gadget_update(inst);
     }
   else if ((!strcmp(keysym, "0")) ||
            (!strcmp(keysym, "1")) ||
            (!strcmp(keysym, "2")) ||
            (!strcmp(keysym, "3")) ||
            (!strcmp(keysym, "4")) ||
            (!strcmp(keysym, "5")) ||
            (!strcmp(keysym, "6")) ||
            (!strcmp(keysym, "7")) ||
            (!strcmp(keysym, "8")) ||
            (!strcmp(keysym, "9")))
     {
        double v = (double)atoi(keysym) / 9.0;
        e_widget_slider_value_double_set(inst->o_slider, v);
        inst->val = v;
        e_backlight_mode_set(inst->gcc->gadcon->zone, E_BACKLIGHT_MODE_NORMAL);
        e_backlight_level_set(inst->gcc->gadcon->zone, v, 0.0);
        _backlight_gadget_update(inst);
     }
   else
     {
        Eina_List *l;
        E_Config_Binding_Key *bind;
        E_Binding_Modifier mod;
        
        for (l = e_config->key_bindings; l; l = l->next)
          {
             bind = l->data;
             
             if (bind->action && strcmp(bind->action, "backlight")) continue;
             
             mod = 0;
             
             if (ev->modifiers & ECORE_EVENT_MODIFIER_SHIFT)
                mod |= E_BINDING_MODIFIER_SHIFT;
             if (ev->modifiers & ECORE_EVENT_MODIFIER_CTRL)
                mod |= E_BINDING_MODIFIER_CTRL;
             if (ev->modifiers & ECORE_EVENT_MODIFIER_ALT)
                mod |= E_BINDING_MODIFIER_ALT;
             if (ev->modifiers & ECORE_EVENT_MODIFIER_WIN)
                mod |= E_BINDING_MODIFIER_WIN;
             
             if (bind->key && (!strcmp(bind->key, ev->keyname)) &&
                 ((bind->modifiers == (int)mod) || (bind->any_mod)))
               {
                  _backlight_popup_free(inst);
                  break;
               }
          }
     }
   return ECORE_CALLBACK_PASS_ON;
}

static void
_backlight_input_win_new(Instance *inst)
{
   Ecore_X_Window_Configure_Mask mask;
   Ecore_X_Window w, popup_w;
   E_Manager *man;
   
   man = inst->gcc->gadcon->zone->container->manager;
   
   w = ecore_x_window_input_new(man->root, 0, 0, man->w, man->h);
   mask = (ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE |
           ECORE_X_WINDOW_CONFIGURE_MASK_SIBLING);
   popup_w = inst->popup->win->evas_win;
   ecore_x_window_configure(w, mask, 0, 0, 0, 0, 0, popup_w,
                            ECORE_X_WINDOW_STACK_BELOW);
   ecore_x_window_show(w);
   
   inst->hand_mouse_down =
      ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_DOWN,
                              _backlight_input_win_mouse_down_cb, inst);
   inst->hand_key_down =
      ecore_event_handler_add(ECORE_EVENT_KEY_DOWN,
                              _backlight_input_win_key_down_cb, inst);
   inst->input_win = w;
   e_grabinput_get(0, 0, inst->input_win);
}

static void
_backlight_settings_cb(void *d1, void *d2 __UNUSED__)
{
   Instance *inst = d1;
   e_configure_registry_call("screen/power_management",
                             inst->gcc->gadcon->zone->container, NULL);
   _backlight_popup_free(inst);
}

static void
_slider_cb(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Instance *inst = data;
   e_backlight_mode_set(inst->gcc->gadcon->zone, E_BACKLIGHT_MODE_NORMAL);
   e_backlight_level_set(inst->gcc->gadcon->zone, inst->val, 0.0);
   _backlight_gadget_update(inst);
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
   _backlight_gadget_update(inst);
   
   inst->popup = e_gadcon_popup_new(inst->gcc);
   evas = inst->popup->win->evas;
   
   inst->o_table = e_widget_table_add(evas, 0);

   o = e_widget_slider_add(evas, 0, 0, NULL, 0.0, 1.0, 0.05, 0, &(inst->val), NULL, 100);
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
   _backlight_input_win_new(inst);
}

static void
_backlight_popup_free(Instance *inst)
{
   if (!inst->popup) return;
   if (inst->popup)
     {
        _backlight_input_win_del(inst);
        e_object_del(E_OBJECT(inst->popup));
        inst->popup = NULL;
     }
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

   _backlight_popup_free(inst);
   e_configure_registry_call("screen/power_management",
                             inst->gcc->gadcon->zone->container, NULL);
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

static void
_backlight_level_decrease(Instance *inst)
{
   double v = inst->val - 0.1;
   if (v < 0.0) v = 0.0;
   e_backlight_level_set(inst->gcc->gadcon->zone, v, 0.0);
}

static void
_backlight_level_increase(Instance *inst)
{
   double v = inst->val + 0.1;
   if (v > 1.0) v = 1.0;
   e_backlight_level_set(inst->gcc->gadcon->zone, v, 0.0);
}

static void
_backlight_cb_mouse_wheel(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event)
{
   Eina_List *l;
   Evas_Event_Mouse_Wheel *ev = event;
   double v;
   Instance *inst = data;

   inst->val = e_backlight_level_get(inst->gcc->gadcon->zone);
   if (ev->z > 0)
     _backlight_level_decrease(inst);
   else if (ev->z < 0)
     _backlight_level_increase(inst);
   v = inst->val;

   EINA_LIST_FOREACH(backlight_instances, l, inst)
     {
        inst->val = v;
        _backlight_gadget_update(inst);
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
   _backlight_gadget_update(inst);
   
   evas_object_event_callback_add(inst->o_backlight, 
                                  EVAS_CALLBACK_MOUSE_DOWN,
                                  _backlight_cb_mouse_down,
                                  inst);
   evas_object_event_callback_add(inst->o_backlight, 
                                  EVAS_CALLBACK_MOUSE_WHEEL,
                                  _backlight_cb_mouse_wheel,
                                  inst);
   
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
        e_menu_post_deactivate_callback_set(inst->menu, NULL, NULL);
        _backlight_input_win_del(inst);
        e_object_del(E_OBJECT(inst->menu));
        inst->menu = NULL;
     }
   _backlight_popup_free(inst);
   backlight_instances = eina_list_remove(backlight_instances, inst);
   evas_object_del(inst->o_backlight);
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

static void
_e_mod_action_cb(E_Object *obj __UNUSED__,
                 const char *params __UNUSED__)
{
   Eina_List *l;
   Instance *inst;
   
   EINA_LIST_FOREACH(backlight_instances, l, inst)
     {
        if (inst->popup) _backlight_popup_free(inst);
        else _backlight_popup_new(inst);
     }
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
   act = e_action_add("backlight");
   if (act)
     {
        act->func.go = _e_mod_action_cb;
        e_action_predef_name_set(_("Screen"), _("Backlight Controls"), "backlight", NULL, NULL, 0);
     }
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   if (act)
     {
        e_action_predef_name_del(_("Screen"), _("Backlight Controls"));
        e_action_del("backlight");
        act = NULL;
     }
   backlight_module = NULL;
   e_gadcon_provider_unregister(&_gadcon_class);
   return 1;
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   return 1;
}
