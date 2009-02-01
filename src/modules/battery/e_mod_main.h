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
   int                  have_hal;
   int                  full;
   int                  time_left;
   int                  have_battery;
   int                  have_power;
   struct {
      // FIXME: on bat_conf del dbus_pending_call_cancel(hal.have);
      //        then set hal.have to NULL
      DBusPendingCall       *have;
      // FIXME: on bat_conf del e_dbus_signal_handler_del() these
      E_DBus_Signal_Handler *dev_add;
      E_DBus_Signal_Handler *dev_del;
   } hal;
};

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init     (E_Module *m);
EAPI int   e_modapi_shutdown (E_Module *m);
EAPI int   e_modapi_save     (E_Module *m);

EAPI E_Config_Dialog *e_int_config_battery_module(E_Container *con, const char *params __UNUSED__);
    
void _battery_config_updated(void);
extern Config *battery_config;

#endif
