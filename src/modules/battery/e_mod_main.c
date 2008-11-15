/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

#define UNKNOWN 0
#define NOHAL 1
#define HAL 2

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
        _gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL
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

static void _battery_update(int full, int time_left, int have_battery, int have_power);
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
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient)
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
_gc_label(E_Gadcon_Client_Class *client_class)
{
   return _("Battery");
}

static Evas_Object *
_gc_icon(E_Gadcon_Client_Class *client_class, Evas *evas)
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
_gc_id_new(E_Gadcon_Client_Class *client_class)
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
	e_menu_item_label_set(mi, _("Configuration"));
	e_util_menu_item_edje_icon_set(mi, "widget/config");
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
_menu_cb_post(void *data, E_Menu *m)
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
_battery_face_time_set(Evas_Object *battery, int time_left)
{
   char buf[256];
   int hrs, mins;

   hrs = (time_left / 3600);
   mins = ((time_left) / 60 - (hrs * 60));
   snprintf(buf, sizeof(buf), "%i:%02i", hrs, mins);
   if (hrs < 0) hrs = 0;
   if (mins < 0) mins = 0;
   edje_object_part_text_set(battery, "e.text.time", buf);
}

static void 
_battery_face_cb_menu_configure(void *data, E_Menu *m, E_Menu_Item *mi) 
{
   if (!battery_config) return;
   if (battery_config->config_dialog) return;
   e_int_config_battery_module(m->zone->container, NULL);
}

/* dbus/hal stuff */
typedef struct _Hal_Battery Hal_Battery;
typedef struct _Hal_Ac_Adapter Hal_Ac_Adapter;

struct _Hal_Battery
{
   const char *udi;
   E_DBus_Signal_Handler *prop_change;
   int present;
   int percent;
   int can_charge;
   int current_charge;
   int design_charge;
   int last_full_charge;
   int charge_rate;
   int time_left;
   int is_charging;
   int is_discharging;
   const char *charge_units;
   const char *technology;
   const char *model;
   const char *vendor;
   const char *type;
};

struct _Hal_Ac_Adapter
{
   const char *udi;
   E_DBus_Signal_Handler *prop_change;
   int present;
   const char *product;
};

static void _battery_hal_update(void);
static void _battery_hal_shutdown(void);
static void _battery_hal_battery_props(void *data, void *reply_data, DBusError *error);
static void _battery_hal_ac_adapter_props(void *data, void *reply_data, DBusError *error);
static void _battery_hal_battery_property_changed(void *data, DBusMessage *msg);
static void _battery_hal_ac_adatper_property_changed(void *data, DBusMessage *msg);
static void _battery_hal_battery_add(const char *udi);
static void _battery_hal_battery_del(const char *udi);
static void _battery_hal_ac_adapter_add(const char *udi);
static void _battery_hal_ac_adapter_del(const char *udi);
static void _battery_hal_find_battery(void *user_data, void *reply_data, DBusError *error);
static void _battery_hal_find_ac(void *user_data, void *reply_data, DBusError *error);
static void _battery_hal_is_battery(void *user_data, void *reply_data, DBusError *err);
static void _battery_hal_is_ac_adapter(void *user_data, void *reply_data, DBusError *err);
static void _battery_hal_dev_add(void *data, DBusMessage *msg);
static void _battery_hal_dev_del(void *data, DBusMessage *msg);
static void _battery_hal_have_hal(void *data, DBusMessage *msg, DBusError *err);

static Eina_List *hal_batteries = NULL;
static Eina_List *hal_ac_adapters = NULL;
static double init_time = 0;

static void
_battery_hal_update(void)
{
   Eina_List *l;
   int full = -1;
   int time_left = -1;
   int have_battery = 0;
   int have_power = 0;

   int batnum = 0;
   int acnum = 0;
   
   for (l = hal_ac_adapters; l; l = l->next)
     {
        Hal_Battery *hac;
        
        hac = l->data;
        if (hac->present) acnum++;
     }
   for (l = hal_batteries; l; l = l->next)
     {
        Hal_Battery *hbat;
        
        hbat = l->data;
        have_battery = 1;
        batnum++;
        if (hbat->is_charging) have_power = 1;
        full += hbat->percent;
        if (hbat->time_left > 0)
          {
             if (time_left < 0) time_left = hbat->time_left;
             else time_left += hbat->time_left;
          }
     }
   if (batnum > 0) full /= batnum;

   if ((acnum >= 0) && (batnum == 0))
     _battery_update(full, time_left, have_battery, have_power);
   else
     {
        _battery_update(full, time_left, have_battery, have_power);
        e_powersave_mode_set(E_POWERSAVE_MODE_LOW);
     }
}

static void
_battery_hal_shutdown(void)
{
   E_DBus_Connection *conn;
   
   conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
   if (!conn) return;
   // FIXME: free all bat/ac adapters, cattery_config stuff etc.
   if (battery_config->hal.have)
     {
        dbus_pending_call_cancel(battery_config->hal.have);
        battery_config->hal.have = NULL;
     }
   if (battery_config->hal.dev_add)
     {
        e_dbus_signal_handler_del(conn, battery_config->hal.dev_add);
        battery_config->hal.dev_add = NULL;
     }
   if (battery_config->hal.dev_del)
     {
        e_dbus_signal_handler_del(conn, battery_config->hal.dev_del);
        battery_config->hal.dev_del = NULL;
     }
   while (hal_ac_adapters)
     {
        Hal_Ac_Adapter *hac;
        
        hac = hal_ac_adapters->data;
        e_dbus_signal_handler_del(conn, hac->prop_change);
        eina_stringshare_del(hac->udi);
        free(hac);
        hal_ac_adapters = eina_list_remove_list(hal_ac_adapters, hal_ac_adapters);
     }
   while (hal_batteries)
     {
        Hal_Battery *hbat;
        
        hbat = hal_batteries->data;
        e_dbus_signal_handler_del(conn, hbat->prop_change);
        eina_stringshare_del(hbat->udi);
        free(hbat);
        hal_batteries = eina_list_remove_list(hal_batteries, hal_batteries);
     }
}

static void
_battery_hal_battery_props(void *data, void *reply_data, DBusError *error)
{
   E_Hal_Properties *ret = reply_data;
   int err = 0;
   int i;
   char *str;
   Hal_Battery *hbat;
   
   hbat = data;

#undef GET_BOOL
#undef GET_INT
#undef GET_STR
#define GET_BOOL(val, s) hbat->val = e_hal_property_bool_get(ret, s, &err)
#define GET_INT(val, s) hbat->val = e_hal_property_int_get(ret, s, &err)
#define GET_STR(val, s) \
   if (hbat->val) eina_stringshare_del(hbat->val); \
   hbat->val = NULL; \
   str = e_hal_property_string_get(ret, s, &err); \
   if (str) \
     { \
        hbat->val = eina_stringshare_add(str); \
        free(str); \
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
   GET_BOOL(is_charging, "battery.rechargeable.is_charging");
   GET_BOOL(is_discharging, "battery.rechargeable.is_discharging");
   
   _battery_hal_update();
}

static void
_battery_hal_ac_adapter_props(void *data, void *reply_data, DBusError *error)
{
   E_Hal_Properties *ret = reply_data;
   int err = 0;
   int i;
   char *str;
   Hal_Ac_Adapter *hac;

   hac = data;
   
#undef GET_BOOL
#undef GET_INT
#undef GET_STR
#define GET_BOOL(val, s) hac->val = e_hal_property_bool_get(ret, s, &err)
#define GET_INT(val, s) hac->val = e_hal_property_int_get(ret, s, &err)
#define GET_STR(val, s) \
   if (hac->val) eina_stringshare_del(hac->val); \
   hac->val = NULL; \
   str = e_hal_property_string_get(ret, s, &err); \
   if (str) \
     { \
        hac->val = eina_stringshare_add(str); \
        free(str); \
     }
   
   GET_BOOL(present, "ac_adapter.present");
   GET_STR(product, "info.product");
   _battery_hal_update();
}

static void
_battery_hal_battery_property_changed(void *data, DBusMessage *msg)
{
   E_DBus_Connection *conn;
   
   conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
   if (!conn) return;
   // FIXME: e_dbus doesnt allow us to track this pending call
   e_hal_device_get_all_properties(conn, ((Hal_Battery *)data)->udi,
                                   _battery_hal_battery_props, data);
}

static void
_battery_hal_ac_adapter_property_changed(void *data, DBusMessage *msg)
{
   E_DBus_Connection *conn;
   
   conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
   if (!conn) return;
   // FIXME: e_dbus doesnt allow us to track this pending call
   e_hal_device_get_all_properties(conn, ((Hal_Ac_Adapter *)data)->udi,
                                   _battery_hal_ac_adapter_props, data);
}

static void
_battery_hal_battery_add(const char *udi)
{
   E_DBus_Connection *conn;
   Hal_Battery *hbat;

   conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
   if (!conn) return;
   // FIXME: look for battery with same udi - if so delete and replace
   hbat = E_NEW(Hal_Battery, 1);
   if (!hbat) return;
   hbat->udi = eina_stringshare_add(udi);
   hal_batteries = eina_list_append(hal_batteries, hbat);
   hbat->prop_change =
     e_dbus_signal_handler_add(conn, E_HAL_SENDER, udi,
                               E_HAL_DEVICE_INTERFACE, "PropertyModified",
                               _battery_hal_battery_property_changed,
                               hbat);
   // FIXME: e_dbus doesnt allow us to track this pending call
   e_hal_device_get_all_properties(conn, udi, 
                                   _battery_hal_battery_props, hbat);
   _battery_hal_update();
}

static void
_battery_hal_battery_del(const char *udi)
{
   E_DBus_Connection *conn;
   Eina_List *l;
   
   conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
   if (!conn) return;
   for (l = hal_batteries; l; l = l->next)
     {
        Hal_Battery *hbat;
        
        hbat = l->data;
        if (!strcmp(udi, hbat->udi))
          {
             e_dbus_signal_handler_del(conn, hbat->prop_change);
             eina_stringshare_del(hbat->udi);
             free(hbat);
             hal_batteries = eina_list_remove_list(hal_batteries, l);
             return;
          }
     }
   _battery_hal_update();
}

static void
_battery_hal_ac_adapter_add(const char *udi)
{
   E_DBus_Connection *conn;
   Hal_Ac_Adapter *hac;

   conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
   if (!conn) return;
   hac = E_NEW(Hal_Ac_Adapter, 1);
   if (!hac) return;
   hac->udi = eina_stringshare_add(udi);
   hal_ac_adapters = eina_list_append(hal_ac_adapters, hac);
   hac->prop_change =
     e_dbus_signal_handler_add(conn, E_HAL_SENDER, udi,
                               E_HAL_DEVICE_INTERFACE, "PropertyModified",
                               _battery_hal_ac_adapter_property_changed, 
                               hac);
   // FIXME: e_dbus doesnt allow us to track this pending call
   e_hal_device_get_all_properties(conn, udi, 
                                   _battery_hal_ac_adapter_props, hac);
   _battery_hal_update();
}

static void
_battery_hal_ac_adapter_del(const char *udi)
{
   E_DBus_Connection *conn;
   Eina_List *l;
   
   conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
   if (!conn) return;
   for (l = hal_ac_adapters; l; l = l->next)
     {
        Hal_Ac_Adapter *hac;
        
        hac = l->data;
        if (!strcmp(udi, hac->udi))
          {
             e_dbus_signal_handler_del(conn, hac->prop_change);
             eina_stringshare_del(hac->udi);
             free(hac);
             hal_ac_adapters = eina_list_remove_list(hal_ac_adapters, l);
             return;
          }
     }
   _battery_hal_update();
}

static void
_battery_hal_find_battery(void *user_data, void *reply_data, DBusError *error)
{
   char *device;
   E_Hal_Manager_Find_Device_By_Capability_Return *ret;
   
   ret = reply_data;
   if (ecore_list_count(ret->strings) < 1) return;
   ecore_list_first_goto(ret->strings);
   while ((device = ecore_list_next(ret->strings)))
     _battery_hal_battery_add(device);
}

static void
_battery_hal_find_ac(void *user_data, void *reply_data, DBusError *err)
{
   char *device;
   E_Hal_Manager_Find_Device_By_Capability_Return *ret;
   
   ret = reply_data;
   if (ecore_list_count(ret->strings) < 1) return;
   ecore_list_first_goto(ret->strings);
   while ((device = ecore_list_next(ret->strings)))
     _battery_hal_ac_adapter_add(device);
}

static void
_battery_hal_is_battery(void *user_data, void *reply_data, DBusError *err)
{
   char *udi = user_data;
   E_Hal_Device_Query_Capability_Return *ret;
   
   ret = reply_data;
   if (dbus_error_is_set(err))
     {
        dbus_error_free(err);
        goto error;
     }
   if (ret && ret->boolean) _battery_hal_battery_add(udi);
   error:
   free(udi);
}

static void
_battery_hal_is_ac_adapter(void *user_data, void *reply_data, DBusError *err)
{
   char *udi = user_data;
   E_Hal_Device_Query_Capability_Return *ret;
   
   ret = reply_data;
   if (dbus_error_is_set(err))
     {
        dbus_error_free(err);
        goto error;
     }
   if (ret && ret->boolean) _battery_hal_ac_adapter_add(udi);
   error:
   free(udi);
}

static void
_battery_hal_dev_add(void *data, DBusMessage *msg)
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
                                 _battery_hal_is_battery, strdup(udi));
   e_hal_device_query_capability(conn, udi, "ac_adapter",
                                 _battery_hal_is_ac_adapter, strdup(udi));
}

static void
_battery_hal_dev_del(void *data, DBusMessage *msg)
{
   DBusError err;
   char *udi = NULL;
   
   dbus_error_init(&err);
   dbus_message_get_args(msg, &err, DBUS_TYPE_STRING, &udi, DBUS_TYPE_INVALID);
   if (!udi) return;
   _battery_hal_battery_del(udi);
   _battery_hal_ac_adapter_del(udi);
}

static void
_battery_hal_have_hal(void *data, DBusMessage *msg, DBusError *err)
{
   dbus_bool_t ok = 0;
   E_DBus_Connection *conn;

   battery_config->hal.have = NULL;
   if (dbus_error_is_set(err))
     {
        battery_config->have_hal = NOHAL;
        _battery_config_updated();
        return;
     }
   dbus_message_get_args(msg, err, DBUS_TYPE_BOOLEAN, &ok, DBUS_TYPE_INVALID);
   if (!ok)
     {
        battery_config->have_hal = NOHAL;
        _battery_config_updated();
        return;
     }
   battery_config->have_hal = HAL;
   conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
   if (!conn) return;
   // FIXME: e_dbus doesnt allow us to track this pending call
   e_hal_manager_find_device_by_capability
     (conn, "battery", _battery_hal_find_battery, NULL);
   e_hal_manager_find_device_by_capability
     (conn, "ac_adapter", _battery_hal_find_ac, NULL);
   battery_config->hal.dev_add = 
     e_dbus_signal_handler_add(conn, "org.freedesktop.Hal",
                               "/org/freedesktop/Hal/Manager",
                               "org.freedesktop.Hal.Manager",
                               "DeviceAdded", _battery_hal_dev_add, NULL);
   battery_config->hal.dev_del = 
     e_dbus_signal_handler_add(conn, "org.freedesktop.Hal",
                               "/org/freedesktop/Hal/Manager",
                               "org.freedesktop.Hal.Manager",
                               "DeviceRemoved", _battery_hal_dev_del, NULL);
   init_time = ecore_time_get();
}
  
/* end dbus/hal stuff */

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
   if (battery_config->have_hal == UNKNOWN)
     {
        if (!e_dbus_bus_get(DBUS_BUS_SYSTEM))
          battery_config->have_hal = NOHAL;
     }

   if (battery_config->have_hal == NOHAL)
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
   else if (battery_config->have_hal == UNKNOWN)
     {
        E_DBus_Connection *conn;
        DBusPendingCall *call;
        
        conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
        if (conn)
          battery_config->hal.have = 
          e_dbus_name_has_owner(conn, "org.freedesktop.Hal",
                                _battery_hal_have_hal, NULL);
     }
}

static int
_battery_cb_warning_popup_timeout(void *data)
{
   Instance *inst;

   inst = data;
   e_gadcon_popup_hide(inst->warning);
   return 0;
}

static void
_battery_cb_warning_popup_hide(void *data, Evas *e, Evas_Object *obj, void *event)
{
   Instance *inst = NULL;

   inst = (Instance *)data;
   if ((!inst) || (!inst->warning)) return;
   e_gadcon_popup_hide(inst->warning);
}

static void
_battery_warning_popup_destroy(Instance *inst)
{
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

   inst->warning = 
     e_gadcon_popup_new(inst->gcc, NULL);
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

   if (battery_config->alert_timeout) 
     {
        ecore_timer_add(battery_config->alert_timeout, 
                        _battery_cb_warning_popup_timeout, inst);
     }
}

static void
_battery_update(int full, int time_left, int have_battery, int have_power)
{
   Eina_List *l;
   static int debounce_popup = 0;
   
   if (debounce_popup > POPUP_DEBOUNCE_CYCLES)
     debounce_popup = 0;
   for (l = battery_config->instances; l; l = l->next)
     {
        Instance *inst;
        
        inst = l->data;
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
                  _battery_face_level_set(inst->o_battery, 
                                          (double)full / 100.0);
                  if (inst->popup_battery)
                    _battery_face_level_set(inst->popup_battery, 
                                            (double)full / 100.0);
               }
          }
        else
          {
             _battery_face_level_set(inst->o_battery, 0.0);
             edje_object_part_text_set(inst->o_battery, 
                                       "e.text.reading", 
                                       _("N/A"));
          }
        
        if (time_left != battery_config->time_left)
          {
             _battery_face_time_set(inst->o_battery, time_left);
             if (inst->popup_battery)
               _battery_face_time_set(inst->popup_battery, 
                                      time_left);
          }
        
        if (have_battery && (!have_power) && (full != 100) &&
            ((battery_config->alert && ((time_left / 60) <= battery_config->alert)) || 
             (battery_config->alert_p && (full <= battery_config->alert_p)))
            )
          {
             if (++debounce_popup == POPUP_DEBOUNCE_CYCLES)
               {
                  double t;
                  
                  t = ecore_time_get() - init_time;
                  if (t > 5.0)
                    _battery_warning_popup(inst, time_left, (double)full/100.0);
                  else
                    debounce_popup = 0;
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
_battery_cb_exe_data(void *data, int type, void *event)
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
		  int have_battery = 0;
		  int have_power = 0;
		  
                  if (sscanf(ev->lines[i].line, "%i %i %i %i",
                             &full, &time_left, &have_battery, &have_power)
		      == 4)
                    _battery_update(full, time_left, have_battery, have_power);
                  else
                    e_powersave_mode_set(E_POWERSAVE_MODE_LOW);
	       }
	  }
     }
   return 0;
}                          

static int
_battery_cb_exe_del(void *data, int type, void *event)
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

   battery_config = e_config_domain_load("module.battery", conf_edd);
   if (!battery_config)
     {
       battery_config = E_NEW(Config, 1);
       battery_config->poll_interval = 512;
       battery_config->alert = 30;
       battery_config->alert_p = 10;
       battery_config->alert_timeout = 0;
     }
   E_CONFIG_LIMIT(battery_config->poll_interval, 4, 4096);
   E_CONFIG_LIMIT(battery_config->alert, 0, 60);
   E_CONFIG_LIMIT(battery_config->alert_p, 0, 100);
   E_CONFIG_LIMIT(battery_config->alert_timeout, 0, 300);

   battery_config->module = m;
   battery_config->full = -2;
   battery_config->time_left = -2;
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
                                     "enlightenment/advanced");
   e_configure_registry_item_add("advanced/battery", 100, _("Battery Meter"), 
                                 NULL, buf, e_int_config_battery_module);

   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   e_configure_registry_item_del("advanced/battery");
   e_configure_registry_category_del("advanced");
   e_gadcon_provider_unregister(&_gadcon_class);

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
   
   _battery_hal_shutdown();
   
   free(battery_config);
   battery_config = NULL;
   E_CONFIG_DD_FREE(conf_edd);
   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   e_config_domain_save("module.battery", conf_edd, battery_config);
   return 1;
}
