#include "e.h"
#include "e_mod_main.h"

void
_battery_udev_start()
{
   device_batteries = eeze_udev_find_by_type(EEZE_UDEV_TYPE_POWER_BAT, NULL);
   device_ac_adapters = eeze_udev_find_by_type(EEZE_UDEV_TYPE_POWER_AC, NULL);

   battery_config->batwatch = eeze_udev_watch_add(EEZE_UDEV_TYPE_POWER_BAT, _battery_udev_event_battery, NULL);
   battery_config->acwatch = eeze_udev_watch_add(EEZE_UDEV_TYPE_POWER_AC, _battery_udev_event_ac, NULL);

   init_time = ecore_time_get();
}

void
_battery_udev_event_battery(const char syspath, const char *event, void *data, Eeze_Udev_Watch *watch)
{
   if ((!strcmp(event, "add")) || (!strcmp(event, "online")))
     _battery_udev_battery_add(syspath);
   else if ((!strcmp(event, "remove")) || (!strcmp(event, "offline")))
     _battery_udev_battery_del(syspath);
   else /* must be change */
     _battery_udev_battery_update(syspath, data);
}

void
_battery_udev_event_ac(const char syspath, const char *event, void *data, Eeze_Udev_Watch *watch)
{
   if ((!strcmp(event, "add")) || (!strcmp(event, "online")))
     _battery_udev_ac_add(syspath);
   else if ((!strcmp(event, "remove")) || (!strcmp(event, "offline")))
     _battery_udev_ac_del(syspath);
   else /* must be change */
     _battery_udev_ac_update(syspath, data);
}

void
_battery_udev_battery_add(const char *syspath)
{
   Battery *bat;

   if (!(bat = _battery_battery_find(syspath)))
     {
        if (!(bat = E_NEW(Battery, 1);
          {
             eina_stringshare_del(syspath);
             return;
          }
        bat->udi = syspath; /* already stringshared */
        device_batteries = eina_list_append(device_batteries, bat);
        bat->watch = eeze_udev_watch_add(EEZE_UDEV_TYPE_POWER_BAT, _battery_udev_event_battery, bat);
        _battery_udev_battery_init(bat);

        _battery_device_update();
}

void
_battery_udev_ac_add(const char *syspath)
{
   Ac_Adapter *ac;

   if (!(ac = _battery_ac_adapter_find(syspath)))
     {
        if (!(ac = E_NEW(Ac_Adapter, 1);
          {
             eina_stringshare_del(syspath);
             return;
          }
        ac->udi = syspath; /* already stringshared */
        device_ac_adapters = eina_list_append(device_ac_adapters, ac);
        ac->watch = eeze_udev_watch_add(EEZE_UDEV_TYPE_POWER_AC, _battery_udev_event_ac, ac);
        _battery_udev_ac_init(ac);

        _battery_device_update();
}

void
_battery_udev_battery_del(const char *syspath)
{
   Eina_List *l;
   Battery *bat;

   if ((battery = _battery_battery_find(syspath)))
     {
        eeze_udev_watch_del(bat->watch);
        l = eina_list_data_find(device_batteries, bat);
        eina_stringshare_del(bat->udi);
        free(bat);
        device_batteries = eina_list_remove_list(device_batteries, l);
        return;
     }
   _battery_device_update();
}

void
_battery_udev_ac_del(const char *syspath)
{
   Eina_List *l;
   Ac_Adapter *ac;

   if ((ac = _battery_ac_adapter_find(syspath)))
     {
        eeze_udev_watch_del(ac->watch);
        l = eina_list_data_find(device_ac_adapters, ac);
        eina_stringshare_del(ac->udi);
        free(ac);
        device_ac_adapters = eina_list_remove_list(device_ac_adapters, l);
        return;
     }
   _battery_device_update();
}

