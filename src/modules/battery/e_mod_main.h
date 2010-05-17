/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

typedef struct _Config       Config;

#define CHECK_NONE      0
#define CHECK_ACPI      1
#define CHECK_APM       2
#define CHECK_PMU       3
#define CHECK_SYS_ACPI  4

#define UNKNOWN 0
#define NOSUBSYSTEM 1
#define SUBSYSTEM 2

#define POPUP_DEBOUNCE_CYCLES  2

struct _Config
{
   /* saved * loaded config values */
   int              poll_interval;
   int              alert;	/* Alert on minutes remaining */
   int	            alert_p;    /* Alert on percentage remaining */
   int              alert_timeout;  /* Popup dismissal timeout */
   int              force_mode; /* force use of batget or hal */
   /* just config state */
   E_Module        *module;
   E_Config_Dialog *config_dialog;
   Eina_List       *instances;
   E_Menu          *menu;
   Ecore_Exe           *batget_exe;
   Ecore_Event_Handler *batget_data_handler;
   Ecore_Event_Handler *batget_del_handler;
   Ecore_Timer         *alert_timer;
   int                  full;
   int                  time_left;
   int                  time_full;
   int                  have_battery;
   int                  have_power;
   int                  have_subsystem;
#ifdef HAVE_EUDEV
   Eeze_Udev_Watch     *watch;
#else
   struct {
      // FIXME: on bat_conf del dbus_pending_call_cancel(dbus.have);
      //        then set dbus.have to NULL
      DBusPendingCall       *have;
      // FIXME: on bat_conf del e_dbus_signal_handler_del() these
      E_DBus_Signal_Handler *dev_add;
      E_DBus_Signal_Handler *dev_del;
   } dbus;
#endif
};

typedef struct _Battery Battery;
typedef struct _Ac_Adapter Ac_Adapter;

struct _Battery
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

void _battery_dbus_battery_props(void *data, void *reply_data, DBusError *error);
void _battery_dbus_ac_adapter_props(void *data, void *reply_data, DBusError *error);
void _battery_dbus_battery_property_changed(void *data, DBusMessage *msg);
void _battery_dbus_battery_add(const char *udi);
void _battery_dbus_battery_del(const char *udi);
Battery *_battery_battery_find(const char *udi);
void _battery_dbus_ac_adapter_add(const char *udi);
void _battery_dbus_ac_adapter_del(const char *udi);
Ac_Adapter *_battery_ac_adapter_find(const char *udi);
void _battery_dbus_find_battery(void *user_data, void *reply_data, DBusError *err);
void _battery_dbus_find_ac(void *user_data, void *reply_data, DBusError *err);
void _battery_dbus_is_battery(void *user_data, void *reply_data, DBusError *err);
void _battery_dbus_is_ac_adapter(void *user_data, void *reply_data, DBusError *err);
void _battery_dbus_dev_add(void *data, DBusMessage *msg);
void _battery_dbus_dev_del(void *data, DBusMessage *msg);
void _battery_dbus_have_dbus(void);
void _battery_dbus_shutdown(void);
void _battery_device_update(void);

Eina_List *device_batteries;
Eina_List *device_ac_adapters;
double init_time;

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init     (E_Module *m);
EAPI int   e_modapi_shutdown (E_Module *m);
EAPI int   e_modapi_save     (E_Module *m);

E_Config_Dialog *e_int_config_battery_module(E_Container *con, const char *params __UNUSED__);
    
void _battery_config_updated(void);
extern Config *battery_config;

#endif
