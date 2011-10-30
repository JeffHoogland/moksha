
#include <err.h>

#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/sensors.h>

#include "e.h"
#include "e_mod_main.h"

static Eina_Bool _battery_openbsd_battery_update_poll(void *data);
static void _battery_openbsd_battery_update();

extern Eina_List *device_batteries;
extern Eina_List *device_ac_adapters;
extern double init_time;

Ac_Adapter *ac;
Battery *bat;

int 
_battery_openbsd_start(void)
{
   Eina_List *devices;
   int mib[] = {CTL_HW, HW_SENSORS, 0, 0 ,0};
   int devn;
   struct sensordev snsrdev;
   size_t sdlen = sizeof(struct sensordev);
   struct sensor s;
   size_t slen = sizeof(struct sensor);

   for (devn = 0 ; ; devn++) {
	mib[2] = devn;
	if (sysctl(mib, 3, &snsrdev, &sdlen, NULL, 0) == -1) {
		if (errno == ENXIO)
			continue;
		if (errno == ENOENT)
			break;
	}

	if (!strcmp("acpibat0", snsrdev.xname)) {
		if (!(bat = E_NEW(Battery, 1)))
        		return 0;
        	bat->udi = eina_stringshare_add("acpibat0"),
        	bat->mib = malloc(sizeof(int) * 5);
		bat->mib[0] = mib[0];
		bat->mib[1] = mib[1];
		bat->mib[2] = mib[2];	
        	bat->technology = eina_stringshare_add("Unknow");
                bat->model = eina_stringshare_add("Unknow");
                bat->vendor = eina_stringshare_add("Unknow");
                bat->last_update = ecore_time_get();
                bat->poll = ecore_poller_add(ECORE_POLLER_CORE, 
				battery_config->poll_interval, 
				_battery_openbsd_battery_update_poll, NULL);
                device_batteries = eina_list_append(device_batteries, bat);
	}
	else if (!strcmp("acpiac0", snsrdev.xname)) {
		if (!(ac = E_NEW(Ac_Adapter, 1)))
		   return 0;
		ac->udi = eina_stringshare_add("acpiac0");
		ac->mib = malloc(sizeof(int) * 5);
		ac->mib[0] = mib[0];
		ac->mib[1] = mib[1];
		ac->mib[2] = mib[2];	
                device_ac_adapters = eina_list_append(device_ac_adapters, ac);
	}
   }

   _battery_openbsd_battery_update();

   init_time = ecore_time_get();
   return 1;
}

void 
_battery_openbsd_stop(void)
{
   if (ac)
      free(ac);
   if (bat) {
      eina_stringshare_del(bat->udi);
      eina_stringshare_del(bat->technology);
      eina_stringshare_del(bat->model);
      eina_stringshare_del(bat->vendor);
      ecore_poller_del(bat->poll);
      free(bat->mib);
      free(bat);
   }
}

static Eina_Bool 
_battery_openbsd_battery_update_poll(void *data)
{
	_battery_openbsd_battery_update();
	return EINA_TRUE;
}

static void 
_battery_openbsd_battery_update()
{
	double time, charge;
	struct sensor s;
   	size_t slen = sizeof(struct sensor);
   
   	/* update the poller interval */
   	ecore_poller_poller_interval_set(bat->poll,
   	    battery_config->poll_interval);

	/* last full capacity */
	bat->mib[3] = 8;
	bat->mib[4] = 0;
	if (sysctl(bat->mib, 5, &s, &slen, NULL, 0) != -1) {
		bat->last_full_charge = (double)s.value;
	}
		
 	/* remaining capcity */
	bat->mib[3] = 8;
	bat->mib[4] = 3;
	if (sysctl(bat->mib, 5, &s, &slen, NULL, 0) != -1) {
		charge = (double)s.value;
	}

        time = ecore_time_get();
        if ((bat->got_prop) && (charge != bat->current_charge))
          bat->charge_rate = ((charge - bat->current_charge) / (time - bat->last_update));
        bat->last_update = time;
        bat->current_charge = charge;
        bat->percent = 100 * (bat->current_charge / bat->last_full_charge);
        if (bat->got_prop)
          {
             if (bat->charge_rate > 0)
               {
                  if (battery_config->fuzzy && (++battery_config->fuzzcount <= 10) && (bat->time_full > 0))
                    bat->time_full = (((bat->last_full_charge - bat->current_charge) / bat->charge_rate) + bat->time_full) / 2;
                  else
                    bat->time_full = (bat->last_full_charge - bat->current_charge) / bat->charge_rate;
                  bat->time_left = -1;
               }
             else
               {
                  if (battery_config->fuzzy && (battery_config->fuzzcount <= 10) && (bat->time_left > 0))
                    bat->time_left = (((0 - bat->current_charge) / bat->charge_rate) + bat->time_left) / 2;
                  else
                    bat->time_left = (0 - bat->current_charge) / bat->charge_rate;
                  bat->time_full = -1;
               }
          }
        else
          {
             bat->time_full = -1;
             bat->time_left = -1;
          }

	/* battery state 1: discharge, 2: charge */
	bat->mib[3] = 10;
	bat->mib[4] = 0;
	if (sysctl(bat->mib, 5, &s, &slen, NULL, 0) == -1) {
		if (s.value == 2)
			bat->charging = 1;
		else
			bat->charging = 0;
	}

	/* AC State */
	ac->mib[3] = 9;
	ac->mib[4] = 0;
	if (sysctl(ac->mib, 5, &s, &slen, NULL, 0) == -1) {
		if (s.value)
			ac->present = 1;
		else
			ac->present = 0;
	}

	

	if (bat->got_prop)
		_battery_device_update();
	bat->got_prop = 1;
}
