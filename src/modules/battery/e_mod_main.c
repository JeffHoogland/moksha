/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

#define UNKNOWN 0
#define NOSUBSYSTEM 1
#define SUBSYSTEM 2

#define POPUP_DEBOUNCE_CYCLES  2

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
     "battery",
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
   Evas_Object     *o_battery;
   Evas_Object     *popup_battery;
   E_Gadcon_Popup  *warning;
};

static void _battery_update(int full, int time_left, int time_full, Eina_Bool have_battery, Eina_Bool have_power);
static int _battery_cb_exe_data(void *data, int type, void *event);
static int _battery_cb_exe_del(void *data, int type, void *event);
static void _button_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _menu_cb_post(void *data, E_Menu *m);
static void _battery_face_level_set(Evas_Object *battery, double level);
static void _battery_face_time_set(Evas_Object *battery, int time);
static void _battery_face_cb_menu_configure(void *data, E_Menu *m, E_Menu_Item *mi);

static int  _battery_cb_warning_popup_timeout(void *data);
static void _battery_cb_warning_popup_hide(void *data, Evas *e, Evas_Object *obj, void *event);
static void _battery_warning_popup_destroy(Instance *inst);
static void _battery_warning_popup(Instance *inst, int time, double percent);

static E_Config_DD *conf_edd = NULL;

Config *battery_config = NULL;

static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style)
{
   Evas_Object *o;
   E_Gadcon_Client *gcc;
   Instance *inst;

   battery_config->full = -2;
   battery_config->time_left = -2;
   battery_config->time_full = -2;
   battery_config->have_battery = -2;
   battery_config->have_power = -2;

   inst = E_NEW(Instance, 1);

   o = edje_object_add(gc->evas);
   e_theme_edje_object_set(o, "base/theme/modules/battery",
			   "e/modules/battery/main");

   gcc = e_gadcon_client_new(gc, name, id, style, o);
   gcc->data = inst;

   inst->gcc = gcc;
   inst->o_battery = o;   
   inst->warning = NULL;
   inst->popup_battery = NULL;

#ifdef HAVE_EUDEV
   eeze_udev_init();
#else
   e_dbus_init();
   e_hal_init();
#endif

   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN,
				  _button_cb_mouse_down, inst);
   battery_config->instances = 
     eina_list_append(battery_config->instances, inst);
   _battery_config_updated();

   return gcc;
}

static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
   Instance *inst;

#ifdef HAVE_EUDEV
   e_udev_shutdown();
#else
   e_hal_shutdown();
   e_dbus_shutdown();
#endif

   inst = gcc->data;
   battery_config->instances = 
     eina_list_remove(battery_config->instances, inst);
   evas_object_del(inst->o_battery);
   if (inst->warning)
     {
        e_object_del(E_OBJECT(inst->warning));
        inst->popup_battery = NULL;
     }
   E_FREE(inst);
}

static void
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient __UNUSED__)
{
   Instance *inst;
   Evas_Coord mw, mh, mxw, mxh;

   inst = gcc->data;
   mw = 0, mh = 0;
   edje_object_size_min_get(inst->o_battery, &mw, &mh);
   edje_object_size_max_get(inst->o_battery, &mxw, &mxh);
   if ((mw < 1) || (mh < 1))
     edje_object_size_min_calc(inst->o_battery, &mw, &mh); 
   if (mw < 4) mw = 4;
   if (mh < 4) mh = 4;
   if ((mxw > 0) && (mxh > 0))
     e_gadcon_client_aspect_set(gcc, mxw, mxh);
   e_gadcon_client_min_size_set(gcc, mw, mh);
}

static char *
_gc_label(E_Gadcon_Client_Class *client_class __UNUSED__)
{
   return _("Battery");
}

static Evas_Object *
_gc_icon(E_Gadcon_Client_Class *client_class __UNUSED__, Evas *evas)
{
   Evas_Object *o;
   char buf[4096];

   o = edje_object_add(evas);
   snprintf(buf, sizeof(buf), "%s/e-module-battery.edj",
	    e_module_dir_get(battery_config->module));
   edje_object_file_set(o, buf, "icon");
   return o;
}

static const char *
_gc_id_new(E_Gadcon_Client_Class *client_class __UNUSED__)
{
   return _gadcon_class.name;
}

static void
_button_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Instance *inst;
   Evas_Event_Mouse_Down *ev;

   inst = data;
   ev = event_info;
   if ((ev->button == 3) && (!battery_config->menu))
     {
	E_Menu *mn;
	E_Menu_Item *mi;
	int cx, cy;

	mn = e_menu_new();
	e_menu_post_deactivate_callback_set(mn, _menu_cb_post, inst);
	battery_config->menu = mn;

	mi = e_menu_item_new(mn);
	e_menu_item_label_set(mi, _("Settings"));
	e_util_menu_item_theme_icon_set(mi, "configure");
	e_menu_item_callback_set(mi, _battery_face_cb_menu_configure, NULL);

	e_gadcon_client_util_menu_items_append(inst->gcc, mn, 0);

	e_gadcon_canvas_zone_geometry_get(inst->gcc->gadcon,
					  &cx, &cy, NULL, NULL);
	e_menu_activate_mouse(mn,
			      e_util_zone_current_get(e_manager_current_get()),
			      cx + ev->output.x, cy + ev->output.y, 1, 1,
			      E_MENU_POP_DIRECTION_DOWN, ev->timestamp);
     }
   if (ev->button == 1)
     _battery_cb_warning_popup_hide(data, e, obj, event_info);
}

static void
_menu_cb_post(void *data __UNUSED__, E_Menu *m __UNUSED__)
{
   if (!battery_config->menu) return;
   e_object_del(E_OBJECT(battery_config->menu));
   battery_config->menu = NULL;
}

static void
_battery_face_level_set(Evas_Object *battery, double level)
{
   Edje_Message_Float msg;
   char buf[256];

   snprintf(buf, sizeof(buf), "%i%%", (int)(level*100.0));
   edje_object_part_text_set(battery, "e.text.reading", buf);

   if (level < 0.0) level = 0.0;
   else if (level > 1.0) level = 1.0;
   msg.val = level;
   edje_object_message_send(battery, EDJE_MESSAGE_FLOAT, 1, &msg);
}

static void 
_battery_face_time_set(Evas_Object *battery, int time)
{
   char buf[256];
   int hrs, mins;

   if (time < 0) return;

   hrs = (time / 3600);
   mins = ((time) / 60 - (hrs * 60));
   if (hrs < 0) hrs = 0;
   if (mins < 0) mins = 0;
   snprintf(buf, sizeof(buf), "%i:%02i", hrs, mins);
   edje_object_part_text_set(battery, "e.text.time", buf);
}

static void
_battery_face_cb_menu_configure(void *data __UNUSED__, E_Menu *m, E_Menu_Item *mi __UNUSED__)
{
   if (!battery_config) return;
   if (battery_config->config_dialog) return;
   e_int_config_battery_module(m->zone->container, NULL);
}

/* dbus stuff */
typedef struct __Battery _Battery;
typedef struct _Ac_Adapter Ac_Adapter;

struct __Battery
{
   const char *udi;
#ifdef HAVE_EUDEV
   Eeze_Udev_Watch *watch;
#else
   E_DBus_Signal_Handler *prop_change;
#endif
   Eina_Bool present:1;
   Eina_Bool can_charge:1;
   int state;
   int percent;
   int current_charge;
   int design_charge;
   int last_full_charge;
   int charge_rate;
   int time_full;
   int time_left;
   const char *technology;
   const char *type;
   const char *charge_units;
   const char *model;
   const char *vendor;
   Eina_Bool got_prop:1;
};

struct _Ac_Adapter
{
   const char *udi;
#ifdef HAVE_EUDEV
   Eeze_Udev_Watch *watch;
#else
   E_DBus_Signal_Handler *prop_change;
#endif
   Eina_Bool present:1;
   const char *product;
};

static void _battery_dbus_update(void);
static void _battery_dbus_shutdown(void);
static void _battery_dbus_battery_props(void *data, void *reply_data, DBusError *error);
static void _battery_dbus_ac_adapter_props(void *data, void *reply_data, DBusError *error);
static void _battery_dbus_battery_property_changed(void *data, DBusMessage *msg);
static void _battery_dbus_battery_add(const char *udi);
static void _battery_dbus_battery_del(const char *udi);
static _Battery *_battery_dbus_battery_find(const char *udi);
static void _battery_dbus_ac_adapter_add(const char *udi);
static void _battery_dbus_ac_adapter_del(const char *udi);
static Ac_Adapter *_battery_dbus_ac_adapter_find(const char *udi);
static void _battery_dbus_find_battery(void *user_data, void *reply_data, DBusError *err);
static void _battery_dbus_find_ac(void *user_data, void *reply_data, DBusError *err);
static void _battery_dbus_is_battery(void *user_data, void *reply_data, DBusError *err);
static void _battery_dbus_is_ac_adapter(void *user_data, void *reply_data, DBusError *err);
static void _battery_dbus_dev_add(void *data, DBusMessage *msg);
static void _battery_dbus_dev_del(void *data, DBusMessage *msg);
static void _battery_dbus_have_dbus(void);

static Eina_List *device_batteries = NULL;
static Eina_List *device_ac_adapters = NULL;
static double init_time = 0;

static void
_battery_dbus_shutdown(void)
{
   E_DBus_Connection *conn;
   Ac_Adapter *ac;
   _Battery *bat;
   
   conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
   if (!conn) return;
   if (battery_config->dbus.have)
     {
        dbus_pending_call_cancel(battery_config->dbus.have);
        battery_config->dbus.have = NULL;
     }
   if (battery_config->dbus.dev_add)
     {
        e_dbus_signal_handler_del(conn, battery_config->dbus.dev_add);
        battery_config->dbus.dev_add = NULL;
     }
   if (battery_config->dbus.dev_del)
     {
        e_dbus_signal_handler_del(conn, battery_config->dbus.dev_del);
        battery_config->dbus.dev_del = NULL;
     }
   EINA_LIST_FREE(device_ac_adapters, ac)
     {
        e_dbus_signal_handler_del(conn, ac->prop_change);
        eina_stringshare_del(ac->udi);
	eina_stringshare_del(ac->product);
        free(ac);
     }
   EINA_LIST_FREE(device_batteries, bat)
     {
        e_dbus_signal_handler_del(conn, bat->prop_change);
        eina_stringshare_del(bat->udi);
	eina_stringshare_del(bat->technology);
	eina_stringshare_del(bat->type);
	eina_stringshare_del(bat->charge_units);
	eina_stringshare_del(bat->model);
	eina_stringshare_del(bat->vendor);
        free(bat);
     }
}

static void
_battery_dbus_battery_props(void *data, void *reply_data, DBusError *error __UNUSED__)
{
   E_Hal_Properties *ret = reply_data;
   _Battery *bat = data;
   int err = 0;
   const char *str;
   uint64_t tmp;

   if (dbus_error_is_set(error))
     {
        dbus_error_free(error);
        return;
     }
   if (!ret) return;

#undef GET_BOOL
#undef GET_INT
#undef GET_STR
#define GET_BOOL(val, s) bat->val = e_hal_property_bool_get(ret, s, &err)
#define GET_INT(val, s) bat->val = e_hal_property_int_get(ret, s, &err)
#define GET_STR(val, s) \
   if (bat->val) eina_stringshare_del(bat->val); \
   bat->val = NULL; \
   str = e_hal_property_string_get(ret, s, &err); \
   if (str) \
     { \
        bat->val = eina_stringshare_add(str); \
     }
   
   GET_BOOL(present, "battery.present");
   GET_STR(technology, "battery.reporting.technology");
   GET_STR(model, "battery.model");
   GET_STR(vendor, "battery.vendor");
   GET_STR(type, "battery.type");
   GET_STR(charge_units, "battery.reporting.unit");
   GET_INT(percent, "battery.charge_level.percentage");
   GET_BOOL(can_charge, "battery.is_rechargeable");
   GET_INT(current_charge, "battery.charge_level.current");
   GET_INT(charge_rate, "battery.charge_level.rate");
   GET_INT(design_charge, "battery.charge_level.design");
   GET_INT(last_full_charge, "battery.charge_level.last_full");
   GET_INT(time_left, "battery.remaining_time");
   GET_INT(time_full, "battery.remaining_time");
   /* conform to upower */
   if (e_hal_property_bool_get(ret, "battery.rechargeable.is_charging", &err))
     bat->state = 1;
   else
     bat->state = 2;
   bat->got_prop = 1;
   _battery_dbus_update();
}

static void
_battery_dbus_ac_adapter_props(void *data, void *reply_data, DBusError *error __UNUSED__)
{
   E_Hal_Properties *ret = reply_data;
   Ac_Adapter *ac = data;
   int err = 0;
   const char *str;

   if (dbus_error_is_set(error))
     {
        dbus_error_free(error);
        return;
     }
   if (!ret) return;
   
#undef GET_BOOL
#undef GET_STR
#define GET_BOOL(val, s) ac->val = e_hal_property_bool_get(ret, s, &err)
#define GET_STR(val, s) \
   if (ac->val) eina_stringshare_del(ac->val); \
   ac->val = NULL; \
   str = e_hal_property_string_get(ret, s, &err); \
   if (str) \
     { \
        ac->val = eina_stringshare_add(str); \
     }
   
   GET_BOOL(present, "ac_adapter.present");
   GET_STR(product, "info.product");
   _battery_dbus_update();
}

static void
_battery_dbus_battery_property_changed(void *data, DBusMessage *msg __UNUSED__)
{
   E_DBus_Connection *conn;
   
   conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
   if (!conn) return;
   // FIXME: e_dbus doesnt allow us to track this pending call
   e_hal_device_get_all_properties(conn, ((_Battery *)data)->udi,
                                   _battery_dbus_battery_props, data);
}

static void
_battery_dbus_ac_adapter_property_changed(void *data, DBusMessage *msg __UNUSED__)
{
   E_DBus_Connection *conn;
   
   conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
   if (!conn) return;
   // FIXME: e_dbus doesnt allow us to track this pending call
   e_hal_device_get_all_properties(conn, ((Ac_Adapter *)data)->udi,
                                   _battery_dbus_ac_adapter_props, data);
}

static void
_battery_dbus_battery_add(const char *udi)
{
   E_DBus_Connection *conn;
   _Battery *bat;

   bat = _battery_dbus_battery_find(udi);
   conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
   if (!conn) return;
   if (!bat)
     {
        bat = E_NEW(_Battery, 1);
        if (!bat) return;
        bat->udi = eina_stringshare_add(udi);
        device_batteries = eina_list_append(device_batteries, bat);
        bat->prop_change =
     e_dbus_signal_handler_add(conn, E_HAL_SENDER, udi,
                               E_HAL_DEVICE_INTERFACE, "PropertyModified",
                               _battery_dbus_battery_property_changed,
                               bat);
     }
   // FIXME: e_dbus doesnt allow us to track this pending call
   e_hal_device_get_all_properties(conn, udi, 
                                   _battery_dbus_battery_props, bat);

   _battery_dbus_update();
}

static void
_battery_dbus_battery_del(const char *udi)
{
   E_DBus_Connection *conn;
   Eina_List *l;
   _Battery *bat;
   
   conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
   if (!conn) return;
   bat = _battery_dbus_battery_find(udi);
   if (bat)
     {
        e_dbus_signal_handler_del(conn, bat->prop_change);
        l = eina_list_data_find(device_batteries, bat);
        eina_stringshare_del(bat->udi);
        free(bat);
        device_batteries = eina_list_remove_list(device_batteries, l);
        return;
     }
   _battery_dbus_update();
}

static _Battery *
_battery_dbus_battery_find(const char *udi)
{
   Eina_List *l;
   _Battery *bat;
   EINA_LIST_FOREACH(device_batteries, l, bat)
     {
        if (!strcmp(udi, bat->udi)) return bat;
     }

   return NULL;     
}

static void
_battery_dbus_ac_adapter_add(const char *udi)
{
   E_DBus_Connection *conn;
   Ac_Adapter *ac;

   conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
   if (!conn) return;
   ac = E_NEW(Ac_Adapter, 1);
   if (!ac) return;
   ac->udi = eina_stringshare_add(udi);
   device_ac_adapters = eina_list_append(device_ac_adapters, ac);
   ac->prop_change =
     e_dbus_signal_handler_add(conn, E_HAL_SENDER, udi,
                               E_HAL_DEVICE_INTERFACE, "PropertyModified",
                               _battery_dbus_ac_adapter_property_changed, 
                               ac);
   // FIXME: e_dbus doesnt allow us to track this pending call
   e_hal_device_get_all_properties(conn, udi, 
                                   _battery_dbus_ac_adapter_props, ac);
   _battery_dbus_update();
}

static void
_battery_dbus_ac_adapter_del(const char *udi)
{
   E_DBus_Connection *conn;
   Eina_List *l;
   Ac_Adapter *ac;
   
   conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
   if (!conn) return;
   ac = _battery_dbus_ac_adapter_find(udi);
   if (ac)
     {
        e_dbus_signal_handler_del(conn, ac->prop_change);
        l = eina_list_data_find(device_ac_adapters, ac);
        eina_stringshare_del(ac->udi);
        free(ac);
        device_ac_adapters = eina_list_remove_list(device_ac_adapters, l);
        return;
     }
   _battery_dbus_update();
}

static Ac_Adapter *
_battery_dbus_ac_adapter_find(const char *udi)
{
   Eina_List *l;
   Ac_Adapter *ac;
   EINA_LIST_FOREACH(device_ac_adapters, l, ac)
     {
        if (!strcmp(udi, ac->udi)) return ac;
     }

   return NULL;     
}

static void
_battery_dbus_find_battery(void *user_data __UNUSED__, void *reply_data, DBusError *err __UNUSED__)
{
   Eina_List *l;
   char *device;
   E_Hal_Manager_Find_Device_By_Capability_Return *ret;

   ret = reply_data;
   if (dbus_error_is_set(err))
     {
        dbus_error_free(err);
        return;
     }
   if (!ret) return;

   if (eina_list_count(ret->strings) < 1) return;
   EINA_LIST_FOREACH(ret->strings, l, device)
     _battery_dbus_battery_add(device);
}

static void
_battery_dbus_find_ac(void *user_data __UNUSED__, void *reply_data, DBusError *err __UNUSED__)
{
   Eina_List *l;
   char *device;
   E_Hal_Manager_Find_Device_By_Capability_Return *ret;

   
   ret = reply_data;
   if (dbus_error_is_set(err))
     {
        dbus_error_free(err);
        return;
     }
   if (!ret) return;

   if (eina_list_count(ret->strings) < 1) return;
   EINA_LIST_FOREACH(ret->strings, l, device)
     _battery_dbus_ac_adapter_add(device);

}

static void
_battery_dbus_is_battery(void *user_data, void *reply_data, DBusError *err)
{
   char *udi = user_data;
   E_Hal_Device_Query_Capability_Return *ret;

   
   ret = reply_data;
   if (dbus_error_is_set(err))
     {
        dbus_error_free(err);
        goto error;
     }
   if (!ret) goto error;
   if (ret->boolean)
     _battery_dbus_battery_add(udi);
   error:
   eina_stringshare_del(udi);
}

static void
_battery_dbus_is_ac_adapter(void *user_data, void *reply_data, DBusError *err)
{
   char *udi = user_data;
   E_Hal_Device_Query_Capability_Return *ret;

   
   ret = reply_data;
   if (dbus_error_is_set(err))
     {
        dbus_error_free(err);
        goto error;
     }
   if (!ret) goto error;

   if (ret->boolean)
     _battery_dbus_ac_adapter_add(udi);
   error:
   eina_stringshare_del(udi);
}

static void
_battery_dbus_dev_add(void *data __UNUSED__, DBusMessage *msg)
{
   DBusError err;
   char *udi = NULL;
   E_DBus_Connection *conn;
   
   dbus_error_init(&err);
   dbus_message_get_args(msg, &err, DBUS_TYPE_STRING, &udi, DBUS_TYPE_INVALID);
   if (!udi) return;
   conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
   if (!conn) return;
   // FIXME: e_dbus doesnt allow us to track this pending call
   e_hal_device_query_capability(conn, udi, "battery",
                                 _battery_dbus_is_battery, (void*)eina_stringshare_add(udi));
   e_hal_device_query_capability(conn, udi, "ac_adapter",
                                 _battery_dbus_is_ac_adapter, (void*)eina_stringshare_add(udi));
}

static void
_battery_dbus_dev_del(void *data __UNUSED__, DBusMessage *msg)
{
   DBusError err;
   char *udi = NULL;
   
   dbus_error_init(&err);
   dbus_message_get_args(msg, &err, DBUS_TYPE_STRING, &udi, DBUS_TYPE_INVALID);
   if (!udi) return;
   _battery_dbus_battery_del(udi);
   _battery_dbus_ac_adapter_del(udi);
}

static void
_battery_dbus_have_dbus(void)
{
   E_DBus_Connection *conn;

   conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
   if (!conn) return;
   
   // FIXME: e_dbus doesnt allow us to track this pending call
   e_hal_manager_find_device_by_capability
     (conn, "battery", _battery_dbus_find_battery, NULL);
   e_hal_manager_find_device_by_capability
     (conn, "ac_adapter", _battery_dbus_find_ac, NULL);
   battery_config->dbus.dev_add = 
     e_dbus_signal_handler_add(conn, E_HAL_SENDER,
                               E_HAL_MANAGER_PATH,
                               E_HAL_MANAGER_INTERFACE,
                               "DeviceAdded", _battery_dbus_dev_add, NULL);
   battery_config->dbus.dev_del = 
     e_dbus_signal_handler_add(conn, E_HAL_SENDER,
                               E_HAL_MANAGER_PATH,
                               E_HAL_MANAGER_INTERFACE,
                               "DeviceRemoved", _battery_dbus_dev_del, NULL);
   init_time = ecore_time_get();
}
  
/* end dbus stuff */


static void
_battery_dbus_update(void)
{
   Eina_List *l;
   int full = -1;
   int time_left = -1;
   int time_full = -1;
   int have_battery = 0;
   int have_power = 0;

   int batnum = 0;
   int acnum = 0;
   int state = 0;
   
   for (l = device_ac_adapters; l; l = l->next)
     {
        _Battery *ac;
        
        ac = l->data;
        if (ac->present) acnum++;
     }
   for (l = device_batteries; l; l = l->next)
     {
        _Battery *bat;
        
        bat = l->data;
	if (!bat->got_prop)
	  continue;
        have_battery = 1;
        batnum++;
        if (bat->state == 1) have_power = 1;
        if (full == -1) full = 0;
        if (bat->last_full_charge > 0)
          full += (bat->current_charge * 100) / bat->last_full_charge;
        else if (bat->design_charge > 0)
          full += (bat->current_charge * 100) / bat->design_charge;
        else if (bat->percent >= 0)
          full += bat->percent;
        if (bat->time_left > 0)
          {
             if (time_left < 0) time_left = bat->time_left;
             else time_left += bat->time_left;
          }
        if (bat->time_full > 0)
          {
             if (time_full < 0) time_full = bat->time_full;
             else time_full += bat->time_full;
          }
        state += bat->state;
     }

   if ((device_batteries) && (batnum == 0))
     return; /* not ready yet, no properties received for any battery */

   if (batnum > 0) full /= batnum;

   if (!state) time_left = -1;
   if (time_left < 1) time_left = -1;
   if (time_full < 1) time_full = -1;
   
   _battery_update(full, time_left, time_full, have_battery, have_power);
   if ((acnum >= 0) && (batnum == 0))
     e_powersave_mode_set(E_POWERSAVE_MODE_LOW);
}


void
_battery_config_updated(void)
{
   Eina_List *l = NULL;
   char buf[4096];

   if (!battery_config) return;

   if (battery_config->instances)
     {
        for (l = battery_config->instances; l; l = l->next)
          _battery_warning_popup_destroy(l->data);
     }
   if (battery_config->have_subsystem == UNKNOWN)
     {
        if (!e_dbus_bus_get(DBUS_BUS_SYSTEM))
          battery_config->have_subsystem = NOSUBSYSTEM;
     }

   if ((battery_config->have_subsystem == NOSUBSYSTEM) ||
       (battery_config->force_mode == 1))
     {
        if (battery_config->batget_exe)
          {
             ecore_exe_terminate(battery_config->batget_exe);
             ecore_exe_free(battery_config->batget_exe);
          }
        snprintf(buf, sizeof(buf), "%s/%s/batget %i",
                 e_module_dir_get(battery_config->module), MODULE_ARCH,
                 battery_config->poll_interval);
        
        battery_config->batget_exe = 
          ecore_exe_pipe_run(buf, ECORE_EXE_PIPE_READ |
                             ECORE_EXE_PIPE_READ_LINE_BUFFERED |
                             ECORE_EXE_NOT_LEADER, NULL);
     }
   else if ((battery_config->have_subsystem == UNKNOWN) ||
            (battery_config->force_mode == 2))
     {
        E_DBus_Connection *conn;

        if (battery_config->batget_exe)
          {
             ecore_exe_terminate(battery_config->batget_exe);
             ecore_exe_free(battery_config->batget_exe);
             battery_config->batget_exe = NULL;
          }
        conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
        if (conn)
          {
             battery_config->have_subsystem = SUBSYSTEM;
             _battery_dbus_have_dbus();
          }
        else
          battery_config->have_subsystem = NOSUBSYSTEM;
     }
}

static int
_battery_cb_warning_popup_timeout(void *data)
{
   Instance *inst;

   inst = data;
   e_gadcon_popup_hide(inst->warning);
   battery_config->alert_timer = NULL;
   return 0;
}

static void
_battery_cb_warning_popup_hide(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event __UNUSED__)
{
   Instance *inst = NULL;

   inst = (Instance *)data;
   if ((!inst) || (!inst->warning)) return;
   e_gadcon_popup_hide(inst->warning);
}

static void
_battery_warning_popup_destroy(Instance *inst)
{
   if (battery_config->alert_timer)
     {
        ecore_timer_del(battery_config->alert_timer);
        battery_config->alert_timer = NULL;
     }
   if ((!inst) || (!inst->warning)) return;
   e_object_del(E_OBJECT(inst->warning));
   inst->warning = NULL;
   inst->popup_battery = NULL;
}

static void
_battery_warning_popup(Instance *inst, int time, double percent)
{
   Evas *e = NULL;
   Evas_Object *rect = NULL, *popup_bg = NULL;
   int x,y,w,h;

   if ((!inst) || (inst->warning)) return;

   inst->warning = e_gadcon_popup_new(inst->gcc);
   if (!inst->warning) return;

   e = inst->warning->win->evas;

   popup_bg = edje_object_add(e);
   inst->popup_battery = edje_object_add(e);
     
   if ((!popup_bg) || (!inst->popup_battery))
     {
        e_object_free(E_OBJECT(inst->warning));
        inst->warning = NULL;
        return;
     }

   e_theme_edje_object_set(popup_bg, "base/theme/modules/battery/popup",
     "e/modules/battery/popup");
   e_theme_edje_object_set(inst->popup_battery, "base/theme/modules/battery",
     "e/modules/battery/main");
   edje_object_part_swallow(popup_bg, "battery", inst->popup_battery);

   edje_object_part_text_set(popup_bg, "e.text.title",
			     _("Your battery is low!"));
   edje_object_part_text_set(popup_bg, "e.text.label",
			     _("AC power is recommended."));

   e_gadcon_popup_content_set(inst->warning, popup_bg);
   e_gadcon_popup_show(inst->warning);

   evas_object_geometry_get(inst->warning->o_bg, &x, &y, &w, &h);

   rect = evas_object_rectangle_add(e);
   if (rect)
     {
        evas_object_move(rect, x, y);
        evas_object_resize(rect, w, h);
        evas_object_color_set(rect, 255, 255, 255, 0);
        evas_object_event_callback_add(rect, EVAS_CALLBACK_MOUSE_DOWN , 
                                       _battery_cb_warning_popup_hide, inst);
        evas_object_repeat_events_set(rect, 1);
        evas_object_show(rect);
     }

   _battery_face_time_set(inst->popup_battery, time);
   _battery_face_level_set(inst->popup_battery, percent);
   edje_object_signal_emit(inst->popup_battery, "e,state,discharging", "e");

   if ((battery_config->alert_timeout > 0) &&
       (!battery_config->alert_timer))
     {
        battery_config->alert_timer =
          ecore_timer_add(battery_config->alert_timeout, 
                          _battery_cb_warning_popup_timeout, inst);
     }
}

static void
_battery_update(int full, int time_left, int time_full, Eina_Bool have_battery, Eina_Bool have_power)
{
   Eina_List *l;
   Instance *inst;
   static double debounce_time = 0.0;
   
   EINA_LIST_FOREACH(battery_config->instances, l, inst)
     {
        if (have_power != battery_config->have_power)
          {
             if (have_power)
               edje_object_signal_emit(inst->o_battery, 
                                       "e,state,charging", 
                                       "e");
             else
               {
                  edje_object_signal_emit(inst->o_battery, 
                                          "e,state,discharging", 
                                          "e");
                  if (inst->popup_battery)
                    edje_object_signal_emit(inst->popup_battery, 
                                            "e,state,discharging", "e");
               }
          }
        if (have_battery)
          {
             if (battery_config->full != full)
               {
                  double val;
                  
                  if (full >= 100) val = 1.0;
                  else val = (double)full / 100.0;
                  _battery_face_level_set(inst->o_battery, val);
                  if (inst->popup_battery)
                    _battery_face_level_set(inst->popup_battery, val);
               }
          }
        else
          {
             _battery_face_level_set(inst->o_battery, 0.0);
             edje_object_part_text_set(inst->o_battery, 
                                       "e.text.reading", 
                                       _("N/A"));
          }
        
        if (time_left != battery_config->time_left && !have_power)
          {
             _battery_face_time_set(inst->o_battery, time_left);
             if (inst->popup_battery)
               _battery_face_time_set(inst->popup_battery, 
                                      time_left);
          }
        else if (time_full != battery_config->time_full && have_power)
          {
             _battery_face_time_set(inst->o_battery, time_full);
             if (inst->popup_battery)
               _battery_face_time_set(inst->popup_battery, 
                                      time_full);
          }
        if (have_battery && (!have_power) && (full != 100) &&
            (((time_left > 0) && battery_config->alert && ((time_left / 60) <= battery_config->alert)) || 
             (battery_config->alert_p && (full <= battery_config->alert_p)))
            )
          {
             double t;
             
             t = ecore_time_get();
             if ((t - debounce_time) > 30.0)
               {
                  debounce_time = t;
                  if ((t - init_time) > 5.0)
                    _battery_warning_popup(inst, time_left, (double)full / 100.0);
               }
          }
        else if (have_power)
          _battery_warning_popup_destroy(inst);
     }
   if (!have_battery)
     e_powersave_mode_set(E_POWERSAVE_MODE_LOW);
   else
     {
        if ((have_power) || (full > 95))
          e_powersave_mode_set(E_POWERSAVE_MODE_LOW);
        else if (full > 30)
          e_powersave_mode_set(E_POWERSAVE_MODE_HIGH);
        else
          e_powersave_mode_set(E_POWERSAVE_MODE_EXTREME);
     }
   battery_config->full = full;
   battery_config->time_left = time_left;
   battery_config->have_battery = have_battery;
   battery_config->have_power = have_power;
}

static int
_battery_cb_exe_data(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_Exe_Event_Data *ev;

   ev = event;
   if (ev->exe != battery_config->batget_exe) return 1;
   if ((ev->lines) && (ev->lines[0].line))
     {
	int i;

	for (i = 0; ev->lines[i].line; i++)
	  {
	     if (!strcmp(ev->lines[i].line, "ERROR"))
	       {
		  Eina_List *l;

		  for (l = battery_config->instances; l; l = l->next)
		    {
		       Instance *inst;

		       inst = l->data;
		       edje_object_signal_emit(inst->o_battery, 
                                               "e,state,unknown", "e");
		       edje_object_part_text_set(inst->o_battery, 
                                                 "e.text.reading", _("ERROR"));
		       edje_object_part_text_set(inst->o_battery, 
                                                 "e.text.time", _("ERROR"));

                       if (inst->popup_battery)
                         {
                            edje_object_signal_emit(inst->popup_battery, 
                                                    "e,state,unknown", "e");
                            edje_object_part_text_set(inst->popup_battery, 
                                                      "e.text.reading", _("ERROR"));
                            edje_object_part_text_set(inst->popup_battery, 
                                                      "e.text.time", _("ERROR"));
                         }
		    }
	       }
	     else
	       {
		  int full = 0;
		  int time_left = 0;
                  int time_full = 0;
		  int have_battery = 0;
		  int have_power = 0;
		  
                  if (sscanf(ev->lines[i].line, "%i %i %i %i %i",
                             &full, &time_left, &time_full, 
                             &have_battery, &have_power)
		      == 5)
                    _battery_update(full, time_left, time_full,
                                    have_battery, have_power);
                  else
                    e_powersave_mode_set(E_POWERSAVE_MODE_LOW);
	       }
	  }
     }
   return 0;
}                          

static int
_battery_cb_exe_del(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   Ecore_Exe_Event_Del *ev;

   ev = event;
   if (ev->exe != battery_config->batget_exe) return 1;
   battery_config->batget_exe = NULL;
   return 1;
}

/* module setup */
EAPI E_Module_Api e_modapi = 
{
   E_MODULE_API_VERSION, "Battery"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   char buf[4096];

   conf_edd = E_CONFIG_DD_NEW("Battery_Config", Config);
#undef T
#undef D
#define T Config
#define D conf_edd
   E_CONFIG_VAL(D, T, poll_interval, INT);
   E_CONFIG_VAL(D, T, alert, INT);
   E_CONFIG_VAL(D, T, alert_p, INT);
   E_CONFIG_VAL(D, T, alert_timeout, INT);
   E_CONFIG_VAL(D, T, force_mode, INT);

   battery_config = e_config_domain_load("module.battery", conf_edd);
   if (!battery_config)
     {
       battery_config = E_NEW(Config, 1);
       battery_config->poll_interval = 512;
       battery_config->alert = 30;
       battery_config->alert_p = 10;
       battery_config->alert_timeout = 0;
       battery_config->force_mode = 0;
     }
   E_CONFIG_LIMIT(battery_config->poll_interval, 4, 4096);
   E_CONFIG_LIMIT(battery_config->alert, 0, 60);
   E_CONFIG_LIMIT(battery_config->alert_p, 0, 100);
   E_CONFIG_LIMIT(battery_config->alert_timeout, 0, 300);
   E_CONFIG_LIMIT(battery_config->force_mode, 0, 2);

   battery_config->module = m;
   battery_config->full = -2;
   battery_config->time_left = -2;
   battery_config->time_full = -2;
   battery_config->have_battery = -2;
   battery_config->have_power = -2;

   battery_config->batget_data_handler =
     ecore_event_handler_add(ECORE_EXE_EVENT_DATA,
			     _battery_cb_exe_data, NULL);
   battery_config->batget_del_handler =
     ecore_event_handler_add(ECORE_EXE_EVENT_DEL,
			     _battery_cb_exe_del, NULL);

   e_gadcon_provider_register(&_gadcon_class);

   snprintf(buf, sizeof(buf), "%s/e-module-battery.edj", e_module_dir_get(m));
   e_configure_registry_category_add("advanced", 80, _("Advanced"), NULL, 
                                     "preferences-advanced");
   e_configure_registry_item_add("advanced/battery", 100, _("Battery Meter"), 
                                 NULL, buf, e_int_config_battery_module);

   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   e_configure_registry_item_del("advanced/battery");
   e_configure_registry_category_del("advanced");
   e_gadcon_provider_unregister(&_gadcon_class);

   if (battery_config->alert_timer)
     ecore_timer_del(battery_config->alert_timer);
   
   if (battery_config->batget_exe)
     {
	ecore_exe_terminate(battery_config->batget_exe);
	ecore_exe_free(battery_config->batget_exe);
	battery_config->batget_exe = NULL;
     }

   if (battery_config->batget_data_handler)
     {
	ecore_event_handler_del(battery_config->batget_data_handler);
	battery_config->batget_data_handler = NULL;
     }
   if (battery_config->batget_del_handler)
     {
	ecore_event_handler_del(battery_config->batget_del_handler);
	battery_config->batget_del_handler = NULL;
     }

   if (battery_config->config_dialog)
     e_object_del(E_OBJECT(battery_config->config_dialog));
   if (battery_config->menu)
     {
        e_menu_post_deactivate_callback_set(battery_config->menu, NULL, NULL);
	e_object_del(E_OBJECT(battery_config->menu));
	battery_config->menu = NULL;
     }
   
   _battery_dbus_shutdown();
   
   free(battery_config);
   battery_config = NULL;
   E_CONFIG_DD_FREE(conf_edd);
   return 1;
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   e_config_domain_save("module.battery", conf_edd, battery_config);
   return 1;
}
