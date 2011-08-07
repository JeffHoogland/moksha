#include "e.h"
#include "e_mod_main.h"

static void _battery_dbus_battery_props(void *data, void *reply_data, DBusError *error);
static void _battery_dbus_ac_adapter_props(void *data, void *reply_data, DBusError *error);
static void _battery_dbus_battery_property_changed(void *data, DBusMessage *msg);
static void _battery_dbus_battery_add(const char *udi);
static void _battery_dbus_battery_del(const char *udi);
static void _battery_dbus_ac_adapter_add(const char *udi);
static void _battery_dbus_ac_adapter_del(const char *udi);
static void _battery_dbus_find_battery(void *user_data, void *reply_data, DBusError *err);
static void _battery_dbus_find_ac(void *user_data, void *reply_data, DBusError *err);
static void _battery_dbus_is_battery(void *user_data, void *reply_data, DBusError *err);
static void _battery_dbus_is_ac_adapter(void *user_data, void *reply_data, DBusError *err);
static void _battery_dbus_dev_add(void *data, DBusMessage *msg);
static void _battery_dbus_dev_del(void *data, DBusMessage *msg);

extern Eina_List *device_batteries;
extern Eina_List *device_ac_adapters;
extern double init_time;

static E_DBus_Connection *e_dbus_conn = NULL;

int
_battery_dbus_start(void)
{
   e_dbus_conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
   if (!e_dbus_conn) return 0;
   // FIXME: e_dbus doesn't allow us to track this pending call
   e_hal_manager_find_device_by_capability
     (e_dbus_conn, "battery", _battery_dbus_find_battery, NULL);
   e_hal_manager_find_device_by_capability
     (e_dbus_conn, "ac_adapter", _battery_dbus_find_ac, NULL);
   battery_config->dbus.dev_add = 
     e_dbus_signal_handler_add(e_dbus_conn, E_HAL_SENDER,
                               E_HAL_MANAGER_PATH,
                               E_HAL_MANAGER_INTERFACE,
                               "DeviceAdded", _battery_dbus_dev_add, NULL);
   battery_config->dbus.dev_del = 
     e_dbus_signal_handler_add(e_dbus_conn, E_HAL_SENDER,
                               E_HAL_MANAGER_PATH,
                               E_HAL_MANAGER_INTERFACE,
                               "DeviceRemoved", _battery_dbus_dev_del, NULL);
   init_time = ecore_time_get();
   return 1;
}


void
_battery_dbus_stop(void)
{
   Ac_Adapter *ac;
   Battery *bat;

   if (!e_dbus_conn) return;
   
   if (battery_config->dbus.have)
     {
        dbus_pending_call_cancel(battery_config->dbus.have);
        battery_config->dbus.have = NULL;
     }
   if (battery_config->dbus.dev_add)
     {
        e_dbus_signal_handler_del(e_dbus_conn, battery_config->dbus.dev_add);
        battery_config->dbus.dev_add = NULL;
     }
   if (battery_config->dbus.dev_del)
     {
        e_dbus_signal_handler_del(e_dbus_conn, battery_config->dbus.dev_del);
        battery_config->dbus.dev_del = NULL;
     }
   EINA_LIST_FREE(device_ac_adapters, ac)
     {
        e_dbus_signal_handler_del(e_dbus_conn, ac->prop_change);
        eina_stringshare_del(ac->udi);
        eina_stringshare_del(ac->product);
        free(ac);
     }
   EINA_LIST_FREE(device_batteries, bat)
     {
        e_dbus_signal_handler_del(e_dbus_conn, bat->prop_change);
        eina_stringshare_del(bat->udi);
        eina_stringshare_del(bat->technology);
        eina_stringshare_del(bat->type);
        eina_stringshare_del(bat->charge_units);
        eina_stringshare_del(bat->model);
        eina_stringshare_del(bat->vendor);
        free(bat);
     }
   e_dbus_connection_close(e_dbus_conn);
   e_dbus_conn = NULL;
}

static void
_battery_dbus_battery_props(void *data, void *reply_data, DBusError *error __UNUSED__)
{
   E_Hal_Properties *ret = reply_data;
   Battery *bat = data;
   int err = 0;
   const char *str;

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
        bat->val = eina_stringshare_ref(str); \
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
   if (e_hal_property_bool_get(ret, "battery.rechargeable.is_charging", &err))
     {
        bat->charging = 1;
        GET_INT(time_full, "battery.remaining_time");
        bat->time_left = -1;
     }
   else
     {
        bat->charging = 0;
        GET_INT(time_left, "battery.remaining_time");
        bat->time_full = -1;
     }
   bat->got_prop = 1;
   _battery_device_update();
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
        ac->val = eina_stringshare_ref(str); \
     }
   
   GET_BOOL(present, "ac_adapter.present");
   GET_STR(product, "info.product");
   _battery_device_update();
}

static void
_battery_dbus_battery_property_changed(void *data, DBusMessage *msg __UNUSED__)
{
   // FIXME: e_dbus doesn't allow us to track this pending call
   e_hal_device_get_all_properties(e_dbus_conn, ((Battery *)data)->udi,
                                   _battery_dbus_battery_props, data);
}

static void
_battery_dbus_ac_adapter_property_changed(void *data, DBusMessage *msg __UNUSED__)
{
   // FIXME: e_dbus doesn't allow us to track this pending call
   e_hal_device_get_all_properties(e_dbus_conn, ((Ac_Adapter *)data)->udi,
                                   _battery_dbus_ac_adapter_props, data);
}

static void
_battery_dbus_battery_add(const char *udi)
{
   Battery *bat;

   bat = _battery_battery_find(udi);
   if (!bat)
     {
        bat = E_NEW(Battery, 1);
        if (!bat) return;
        bat->udi = eina_stringshare_add(udi);
        device_batteries = eina_list_append(device_batteries, bat);
        bat->prop_change =
        e_dbus_signal_handler_add(e_dbus_conn, E_HAL_SENDER, udi,
                                  E_HAL_DEVICE_INTERFACE, "PropertyModified",
                                  _battery_dbus_battery_property_changed,
                                  bat);
     }
   // FIXME: e_dbus doesn't allow us to track this pending call
   e_hal_device_get_all_properties(e_dbus_conn, udi, 
                                   _battery_dbus_battery_props, bat);

   _battery_device_update();
}

static void
_battery_dbus_battery_del(const char *udi)
{
   Battery *bat;
   
   bat = _battery_battery_find(udi);
   if (bat)
     {
        e_dbus_signal_handler_del(e_dbus_conn, bat->prop_change);
	device_batteries = eina_list_remove(device_batteries, bat);
        eina_stringshare_del(bat->udi);
        eina_stringshare_del(bat->technology);
        eina_stringshare_del(bat->type);
        eina_stringshare_del(bat->charge_units);
        eina_stringshare_del(bat->model);
        eina_stringshare_del(bat->vendor);
        free(bat);
        return;
     }
   _battery_device_update();
}

static void
_battery_dbus_ac_adapter_add(const char *udi)
{
   Ac_Adapter *ac;

   ac = E_NEW(Ac_Adapter, 1);
   if (!ac) return;
   ac->udi = eina_stringshare_add(udi);
   device_ac_adapters = eina_list_append(device_ac_adapters, ac);
   ac->prop_change =
     e_dbus_signal_handler_add(e_dbus_conn, E_HAL_SENDER, udi,
                               E_HAL_DEVICE_INTERFACE, "PropertyModified",
                               _battery_dbus_ac_adapter_property_changed, 
                               ac);
   // FIXME: e_dbus doesn't allow us to track this pending call
   e_hal_device_get_all_properties(e_dbus_conn, udi, 
                                   _battery_dbus_ac_adapter_props, ac);
   _battery_device_update();
}

static void
_battery_dbus_ac_adapter_del(const char *udi)
{
   Ac_Adapter *ac;
   
   ac = _battery_ac_adapter_find(udi);
   if (ac)
     {
        e_dbus_signal_handler_del(e_dbus_conn, ac->prop_change);
	device_ac_adapters = eina_list_remove(device_ac_adapters, ac);
        eina_stringshare_del(ac->udi);
        eina_stringshare_del(ac->product);
        free(ac);
        return;
     }
   _battery_device_update();
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
   
   dbus_error_init(&err);
   dbus_message_get_args(msg, &err, DBUS_TYPE_STRING, &udi, DBUS_TYPE_INVALID);
   if (!udi) return;
   // FIXME: e_dbus doesn't allow us to track this pending call
   e_hal_device_query_capability(e_dbus_conn, udi, "battery",
                                 _battery_dbus_is_battery, (void*)eina_stringshare_add(udi));
   e_hal_device_query_capability(e_dbus_conn, udi, "ac_adapter",
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
