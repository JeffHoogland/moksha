/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

#define UNKNOWN 0
#define NODBUS 1
#define DBUS 2

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

   e_dbus_init();
#ifdef HAVE_EUKIT
   e_ukit_init();
#else
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

   e_dbus_shutdown();
#ifdef HAVE_EUKIT
   e_ukit_shutdown();
#else
   e_hal_shutdown();
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
typedef struct _Dbus_Battery Dbus_Battery;
typedef struct _Dbus_Ac_Adapter Dbus_Ac_Adapter;

struct _Dbus_Battery
{
   const char *udi;
   E_DBus_Signal_Handler *prop_change;
   Eina_Bool present:1;
   Eina_Bool can_charge:1;
   int state;
#ifdef HAVE_EUKIT
   double percent;
   double current_charge;
   double design_charge;
   double last_full_charge;
   double charge_rate;
   int64_t time_full;
   int64_t time_left;
#else
   int percent;
   int current_charge;
   int design_charge;
   int last_full_charge;
   int charge_rate;
   int time_full;
   int time_left;
#endif
   const char *technology;
   const char *type;
   const char *charge_units;
   const char *model;
   const char *vendor;
   Eina_Bool got_prop:1;
};

struct _Dbus_Ac_Adapter
{
   const char *udi;
   E_DBus_Signal_Handler *prop_change;
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
static Dbus_Battery *_battery_dbus_battery_find(const char *udi);
static void _battery_dbus_ac_adapter_add(const char *udi);
static void _battery_dbus_ac_adapter_del(const char *udi);
static Dbus_Ac_Adapter *_battery_dbus_ac_adapter_find(const char *udi);
static void _battery_dbus_find_battery(void *user_data, void *reply_data, DBusError *err);
static void _battery_dbus_find_ac(void *user_data, void *reply_data, DBusError *err);
static void _battery_dbus_is_battery(void *user_data, void *reply_data, DBusError *err);
static void _battery_dbus_is_ac_adapter(void *user_data, void *reply_data, DBusError *err);
static void _battery_dbus_dev_add(void *data, DBusMessage *msg);
static void _battery_dbus_dev_del(void *data, DBusMessage *msg);
static void _battery_dbus_have_dbus(void);

static Eina_List *dbus_batteries = NULL;
static Eina_List *dbus_ac_adapters = NULL;
static double init_time = 0;

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
   
   for (l = dbus_ac_adapters; l; l = l->next)
     {
        Dbus_Battery *hac;
        
        hac = l->data;
        if (hac->present) acnum++;
     }
   for (l = dbus_batteries; l; l = l->next)
     {
        Dbus_Battery *hbat;
        
        hbat = l->data;
	if (!hbat->got_prop)
	  continue;
        have_battery = 1;
        batnum++;
        if (hbat->state == 1) have_power = 1;
        if (full == -1) full = 0;
        if (hbat->last_full_charge > 0)
          full += (hbat->current_charge * 100) / hbat->last_full_charge;
        else if (hbat->design_charge > 0)
          full += (hbat->current_charge * 100) / hbat->design_charge;
        else if (hbat->percent >= 0)
          full += hbat->percent;
        if (hbat->time_left > 0)
          {
             if (time_left < 0) time_left = hbat->time_left;
             else time_left += hbat->time_left;
          }
        if (hbat->time_full > 0)
          {
             if (time_full < 0) time_full = hbat->time_full;
             else time_full += hbat->time_full;
          }
        state += hbat->state;
     }

   if ((dbus_batteries) && (batnum == 0))
     return; /* not ready yet, no properties received for any battery */

   if (batnum > 0) full /= batnum;

   if (!state) time_left = -1;
   if (time_left < 1) time_left = -1;
   if (time_full < 1) time_full = -1;
   
   _battery_update(full, time_left, time_full, have_battery, have_power);
   if ((acnum >= 0) && (batnum == 0))
     e_powersave_mode_set(E_POWERSAVE_MODE_LOW);
}

static void
_battery_dbus_shutdown(void)
{
   E_DBus_Connection *conn;
   Dbus_Ac_Adapter *hac;
   Dbus_Battery *hbat;
   
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
   EINA_LIST_FREE(dbus_ac_adapters, hac)
     {
        e_dbus_signal_handler_del(conn, hac->prop_change);
        eina_stringshare_del(hac->udi);
        free(hac);
     }
   EINA_LIST_FREE(dbus_batteries, hbat)
     {
        e_dbus_signal_handler_del(conn, hbat->prop_change);
        eina_stringshare_del(hbat->udi);
        free(hbat);
     }
}

static void
_battery_dbus_battery_props(void *data, void *reply_data, DBusError *error __UNUSED__)
{
#ifdef HAVE_EUKIT
   E_Ukit_Get_All_Properties_Return *ret = reply_data;
#else
   E_Hal_Properties *ret = reply_data;
#endif
   Dbus_Battery *hbat = data;
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
#ifdef HAVE_EUKIT
#define GET_BOOL(val, s) hbat->val = e_ukit_property_bool_get(ret, s, &err)
#define GET_INT(val, s) hbat->val = e_ukit_property_int_get(ret, s, &err)
#define GET_INT64(val, s) hbat->val = e_ukit_property_int64_get(ret, s, &err)
#define GET_UINT64(val, s) hbat->val = e_ukit_property_uint64_get(ret, s, &err)
#define GET_DOUBLE(val, s) hbat->val = e_ukit_property_double_get(ret, s, &err)
#define GET_STR(val, s) \
   if (hbat->val) eina_stringshare_del(hbat->val); \
   hbat->val = NULL; \
   str = e_ukit_property_string_get(ret, s, &err); \
   if (str) \
     { \
        hbat->val = str; \
     }
   
   GET_BOOL(present, "IsPresent");
   tmp = e_ukit_property_uint64_get(ret, "Type", &err);
   switch (tmp)
     {
        case E_UPOWER_SOURCE_UNKNOWN:
          hbat->type = eina_stringshare_add("unknown");
          break;
        case E_UPOWER_SOURCE_AC:
          hbat->type = eina_stringshare_add("ac");
          break;
        case E_UPOWER_SOURCE_BATTERY:
          hbat->type = eina_stringshare_add("battery");
          break;
        case E_UPOWER_SOURCE_UPS:
          hbat->type = eina_stringshare_add("ups");
          break;
        case E_UPOWER_SOURCE_MONITOR:
          hbat->type = eina_stringshare_add("monitor");
          break;
        case E_UPOWER_SOURCE_MOUSE:
          hbat->type = eina_stringshare_add("mouse");
          break;
        case E_UPOWER_SOURCE_KEYBOARD:
          hbat->type = eina_stringshare_add("keyboard");
          break;
        case E_UPOWER_SOURCE_PDA:
          hbat->type = eina_stringshare_add("pda");
          break;
        case E_UPOWER_SOURCE_PHONE:
          hbat->type = eina_stringshare_add("phone");
          break;
     }
   GET_STR(model, "Model");
   GET_STR(vendor, "Vendor");
   tmp = e_ukit_property_uint64_get(ret, "Technology", &err);
   switch (tmp)
     {
        case E_UPOWER_BATTERY_UNKNOWN:
          hbat->technology = eina_stringshare_add("unknown");
          break;
        case E_UPOWER_BATTERY_LION:
          hbat->technology = eina_stringshare_add("lithium ion");
          break;
        case E_UPOWER_BATTERY_LPOLYMER:
          hbat->technology = eina_stringshare_add("lithium polymer");
          break;
        case E_UPOWER_BATTERY_LIRONPHOS:
          hbat->technology = eina_stringshare_add("lithium iron phosphate");
          break;
        case E_UPOWER_BATTERY_LEAD:
          hbat->technology = eina_stringshare_add("lead acid");
          break;
        case E_UPOWER_BATTERY_NICAD:
          hbat->technology = eina_stringshare_add("nickel cadmium");
          break;
        case E_UPOWER_BATTERY_METALHYDRYDE:
          hbat->technology = eina_stringshare_add("nickel metal hydride");
          break;
        default:
          break;
     }
          
   if (hbat->charge_units) eina_stringshare_del(hbat->charge_units);
   /* upower always reports in Wh */
   hbat->charge_units = eina_stringshare_add("Wh");
   GET_DOUBLE(percent, "Percent");
   GET_BOOL(can_charge, "IsRechargeable");
   GET_DOUBLE(current_charge, "Energy");
   GET_DOUBLE(charge_rate, "EnergyRate");
   GET_DOUBLE(design_charge, "EnergyFullDesign");
   GET_DOUBLE(last_full_charge, "EnergyFull");
   GET_INT64(time_left, "TimeToEmpty");
   GET_INT64(time_full, "TimeToFull");
   GET_UINT64(state, "State");
#else
#define GET_BOOL(val, s) hbat->val = e_hal_property_bool_get(ret, s, &err)
#define GET_INT(val, s) hbat->val = e_hal_property_int_get(ret, s, &err)
#define GET_STR(val, s) \
   if (hbat->val) eina_stringshare_del(hbat->val); \
   hbat->val = NULL; \
   str = e_hal_property_string_get(ret, s, &err); \
   if (str) \
     { \
        hbat->val = str; \
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
     hbat->state = 1;
   else
     hbat->state = 2;
#endif
   hbat->got_prop = 1;
   _battery_dbus_update();
}

static void
_battery_dbus_ac_adapter_props(void *data, void *reply_data, DBusError *error __UNUSED__)
{
#ifdef HAVE_EUKIT
   E_Ukit_Get_All_Properties_Return *ret = reply_data;
#else
   E_Hal_Properties *ret = reply_data;
#endif
   Dbus_Ac_Adapter *hac = data;
   int err = 0;
   char *str;

   if (dbus_error_is_set(error))
     {
        dbus_error_free(error);
        return;
     }
   if (!ret) return;
   
#undef GET_BOOL
#undef GET_STR
#ifdef HAVE_EUKIT
#define GET_BOOL(val, s) hac->val = e_ukit_property_bool_get(ret, s, &err)
#define GET_STR(val, s) \
   if (hac->val) eina_stringshare_del(hac->val); \
   hac->val = NULL; \
   str = e_ukit_property_string_get(ret, s, &err); \
   if (str) \
     { \
        hac->val = str; \
     }
   
   GET_BOOL(present, "IsPresent");
   GET_STR(product, "Model");

#else
#define GET_BOOL(val, s) hac->val = e_hal_property_bool_get(ret, s, &err)
#define GET_STR(val, s) \
   if (hac->val) eina_stringshare_del(hac->val); \
   hac->val = NULL; \
   str = e_hal_property_string_get(ret, s, &err); \
   if (str) \
     { \
        hac->val = str; \
     }
   
   GET_BOOL(present, "ac_adapter.present");
   GET_STR(product, "info.product");
#endif
   _battery_dbus_update();
}

static void
_battery_dbus_battery_property_changed(void *data, DBusMessage *msg __UNUSED__)
{
   E_DBus_Connection *conn;
   
   conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
   if (!conn) return;
   // FIXME: e_dbus doesnt allow us to track this pending call
#ifdef HAVE_EUKIT
   e_upower_get_all_properties(conn, ((Dbus_Battery *)data)->udi,
                                   _battery_dbus_battery_props, data);
#else
   e_hal_device_get_all_properties(conn, ((Dbus_Battery *)data)->udi,
                                   _battery_dbus_battery_props, data);
#endif
}

static void
_battery_dbus_ac_adapter_property_changed(void *data, DBusMessage *msg __UNUSED__)
{
   E_DBus_Connection *conn;
   
   conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
   if (!conn) return;
   // FIXME: e_dbus doesnt allow us to track this pending call
#ifdef HAVE_EUKIT
   e_upower_get_all_properties(conn, ((Dbus_Ac_Adapter *)data)->udi,
                                   _battery_dbus_ac_adapter_props, data);
#else
   e_hal_device_get_all_properties(conn, ((Dbus_Ac_Adapter *)data)->udi,
                                   _battery_dbus_ac_adapter_props, data);
#endif
}

static void
_battery_dbus_battery_add(const char *udi)
{
   E_DBus_Connection *conn;
   Dbus_Battery *hbat;

   hbat = _battery_dbus_battery_find(udi);
   conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
   if (!conn) return;
   if (!hbat)
     {
        hbat = E_NEW(Dbus_Battery, 1);
        if (!hbat) return;
        hbat->udi = eina_stringshare_add(udi);
        dbus_batteries = eina_list_append(dbus_batteries, hbat);
        hbat->prop_change =
#ifdef HAVE_EUKIT
     e_dbus_signal_handler_add(conn, E_UPOWER_BUS, udi,
                               E_UPOWER_BUS, "DeviceChanged",
                               _battery_dbus_battery_property_changed,
                               hbat);
     e_dbus_signal_handler_add(conn, E_UPOWER_BUS, udi,
                               E_UPOWER_INTERFACE, "Changed",
                               _battery_dbus_battery_property_changed,
                               hbat);
     }
   // FIXME: e_dbus doesnt allow us to track this pending call
   e_upower_get_all_properties(conn, udi, 
#else
     e_dbus_signal_handler_add(conn, E_HAL_SENDER, udi,
                               E_HAL_DEVICE_INTERFACE, "PropertyModified",
                               _battery_dbus_battery_property_changed,
                               hbat);
     }
   // FIXME: e_dbus doesnt allow us to track this pending call
   e_hal_device_get_all_properties(conn, udi, 
#endif
                                   _battery_dbus_battery_props, hbat);

   _battery_dbus_update();
}

static void
_battery_dbus_battery_del(const char *udi)
{
   E_DBus_Connection *conn;
   Eina_List *l;
   Dbus_Battery *hbat;
   
   conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
   if (!conn) return;
   hbat = _battery_dbus_battery_find(udi);
   if (hbat)
     {
        e_dbus_signal_handler_del(conn, hbat->prop_change);
        l = eina_list_data_find(dbus_batteries, hbat);
        eina_stringshare_del(hbat->udi);
        free(hbat);
        dbus_batteries = eina_list_remove_list(dbus_batteries, l);
        return;
     }
   _battery_dbus_update();
}

static Dbus_Battery *
_battery_dbus_battery_find(const char *udi)
{
   Eina_List *l;
   Dbus_Battery *hbat;
   EINA_LIST_FOREACH(dbus_batteries, l, hbat)
     {
        if (!strcmp(udi, hbat->udi)) return hbat;
     }

   return NULL;     
}

static void
_battery_dbus_ac_adapter_add(const char *udi)
{
   E_DBus_Connection *conn;
   Dbus_Ac_Adapter *hac;

   conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
   if (!conn) return;
   hac = E_NEW(Dbus_Ac_Adapter, 1);
   if (!hac) return;
   hac->udi = eina_stringshare_add(udi);
   dbus_ac_adapters = eina_list_append(dbus_ac_adapters, hac);
   hac->prop_change =
#ifdef HAVE_EUKIT
     e_dbus_signal_handler_add(conn, E_UPOWER_BUS, udi,
                               E_UPOWER_BUS, "DeviceChanged",
                               _battery_dbus_ac_adapter_property_changed,
                               hac);
     e_dbus_signal_handler_add(conn, E_UPOWER_BUS, udi,
                               E_UPOWER_INTERFACE, "Changed",
                               _battery_dbus_ac_adapter_property_changed,
                               hac);
   // FIXME: e_dbus doesnt allow us to track this pending call
   e_upower_get_all_properties(conn, udi, 
                                   _battery_dbus_ac_adapter_props, hac);

#else
     e_dbus_signal_handler_add(conn, E_HAL_SENDER, udi,
                               E_HAL_DEVICE_INTERFACE, "PropertyModified",
                               _battery_dbus_ac_adapter_property_changed, 
                               hac);
   // FIXME: e_dbus doesnt allow us to track this pending call
   e_hal_device_get_all_properties(conn, udi, 
                                   _battery_dbus_ac_adapter_props, hac);
#endif
   _battery_dbus_update();
}

static void
_battery_dbus_ac_adapter_del(const char *udi)
{
   E_DBus_Connection *conn;
   Eina_List *l;
   Dbus_Ac_Adapter *hac;
   
   conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
   if (!conn) return;
   hac = _battery_dbus_ac_adapter_find(udi);
   if (hac)
     {
        e_dbus_signal_handler_del(conn, hac->prop_change);
        l = eina_list_data_find(dbus_ac_adapters, hac);
        eina_stringshare_del(hac->udi);
        free(hac);
        dbus_ac_adapters = eina_list_remove_list(dbus_ac_adapters, l);
        return;
     }
   _battery_dbus_update();
}

static Dbus_Ac_Adapter *
_battery_dbus_ac_adapter_find(const char *udi)
{
   Eina_List *l;
   Dbus_Ac_Adapter *hac;
   EINA_LIST_FOREACH(dbus_ac_adapters, l, hac)
     {
        if (!strcmp(udi, hac->udi)) return hac;
     }

   return NULL;     
}

static void
_battery_dbus_find_battery(void *user_data __UNUSED__, void *reply_data, DBusError *err __UNUSED__)
{
   Eina_List *l;
   char *device;
#ifdef HAVE_EUKIT
   E_Ukit_Get_All_Devices_Return *ret;
   E_DBus_Connection *conn;
   
   conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
#else
   E_Hal_Manager_Find_Device_By_Capability_Return *ret;
#endif

   ret = reply_data;
   if (dbus_error_is_set(err))
     {
        dbus_error_free(err);
        return;
     }
   if (!ret) return;

   if (eina_list_count(ret->strings) < 1) return;
   EINA_LIST_FOREACH(ret->strings, l, device)
#ifdef HAVE_EUKIT
   e_upower_get_property(conn, device, "Type",
     _battery_dbus_is_battery, eina_stringshare_add(device));
#else
     _battery_dbus_battery_add(device);
#endif
}

static void
_battery_dbus_find_ac(void *user_data __UNUSED__, void *reply_data, DBusError *err __UNUSED__)
{
   Eina_List *l;
   char *device;
#ifdef HAVE_EUKIT
   E_Ukit_Get_All_Devices_Return *ret;
   E_DBus_Connection *conn;
   
   conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
#else
   E_Hal_Manager_Find_Device_By_Capability_Return *ret;
#endif
   
   ret = reply_data;
   if (dbus_error_is_set(err))
     {
        dbus_error_free(err);
        return;
     }
   if (!ret) return;

   if (eina_list_count(ret->strings) < 1) return;
   EINA_LIST_FOREACH(ret->strings, l, device)
#ifdef HAVE_EUKIT
   e_upower_get_property(conn, device, "Type",
     _battery_dbus_is_ac_adapter, eina_stringshare_add(device));
#else
     _battery_dbus_ac_adapter_add(device);
#endif
}

static void
_battery_dbus_is_battery(void *user_data, void *reply_data, DBusError *err)
{
   char *udi = user_data;
#ifdef HAVE_EUKIT
   E_Ukit_Get_Property_Return *ret;
#else
   E_Hal_Device_Query_Capability_Return *ret;
#endif
   
   ret = reply_data;
   if (dbus_error_is_set(err))
     {
        dbus_error_free(err);
        goto error;
     }
   if (!ret) goto error;
#ifdef HAVE_EUKIT
   if (ret->val.u == E_UPOWER_SOURCE_BATTERY)
#else
   if (ret->boolean)
#endif
     _battery_dbus_battery_add(udi);
   error:
   eina_stringshare_del(udi);
}

static void
_battery_dbus_is_ac_adapter(void *user_data, void *reply_data, DBusError *err)
{
   char *udi = user_data;
#ifdef HAVE_EUKIT
   E_Ukit_Get_Property_Return *ret;
#else
   E_Hal_Device_Query_Capability_Return *ret;
#endif
   
   ret = reply_data;
   if (dbus_error_is_set(err))
     {
        dbus_error_free(err);
        goto error;
     }
   if (!ret) goto error;
#ifdef HAVE_EUKIT
   if (ret->val.u == E_UPOWER_SOURCE_AC)
#else
   if (ret->boolean)
#endif
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
#ifdef HAVE_EUKIT
   e_upower_get_property(conn, udi, "Type",
                                 _battery_dbus_is_battery, eina_stringshare_add(udi));
   e_upower_get_property(conn, udi, "Type",
                                 _battery_dbus_is_ac_adapter, eina_stringshare_add(udi));
#else
   e_hal_device_query_capability(conn, udi, "battery",
                                 _battery_dbus_is_battery, eina_stringshare_add(udi));
   e_hal_device_query_capability(conn, udi, "ac_adapter",
                                 _battery_dbus_is_ac_adapter, eina_stringshare_add(udi));
#endif
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
#ifdef HAVE_EUKIT
   e_upower_get_all_devices
     (conn, _battery_dbus_find_battery, NULL);
   e_upower_get_all_devices
     (conn, _battery_dbus_find_ac, NULL);
   battery_config->dbus.dev_add = 
     e_dbus_signal_handler_add(conn, E_UPOWER_BUS,
                               E_UPOWER_PATH,
                               E_UPOWER_BUS,
                               "DeviceAdded", _battery_dbus_dev_add, NULL);
   battery_config->dbus.dev_del = 
     e_dbus_signal_handler_add(conn, E_UPOWER_BUS,
                               E_UPOWER_PATH,
                               E_UPOWER_BUS,
                               "DeviceRemoved", _battery_dbus_dev_del, NULL);
#else
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
#endif
   init_time = ecore_time_get();
}
  
/* end dbus stuff */

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
   if (battery_config->have_dbus == UNKNOWN)
     {
        if (!e_dbus_bus_get(DBUS_BUS_SYSTEM))
          battery_config->have_dbus = NODBUS;
     }

   if ((battery_config->have_dbus == NODBUS) ||
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
   else if ((battery_config->have_dbus == UNKNOWN) ||
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
             battery_config->have_dbus = DBUS;
             _battery_dbus_have_dbus();
          }
        else
          battery_config->have_dbus = NODBUS;
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
                             &full, &time_left, &time_full, &have_battery, &have_power)
		      == 5)
                    _battery_update(full, time_left, time_full, have_battery, have_power);
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
