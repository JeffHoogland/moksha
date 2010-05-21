#include "e.h"
#include "e_mod_main.h"

int
temperature_udev_update_poll(void *data)
{
   temperature_udev_update(data);
   return 1;
}

void
temperature_udev_update(void *data)
{
   Config_Face *inst;
   Eina_List *l, *l2;
   double temp;
   char *syspath;
   const char *test;
   char buf[256];
   double cpus = 0;

   inst = data;
   temp = -999;

   if (!inst->tempdevs)
     {
        inst->tempdevs = eeze_udev_find_by_type(EEZE_UDEV_TYPE_IS_IT_HOT_OR_IS_IT_COLD_SENSOR, NULL);
        EINA_LIST_FOREACH_SAFE(inst->tempdevs, l, l2, syspath)
          {
             if ((eeze_udev_syspath_get_sysattr(syspath, "fan1_input")) ||
               (eeze_udev_syspath_get_sysattr(syspath, "pwm1_stop_time")))
               {
                  eina_stringshare_del(syspath);
                  inst->tempdevs = eina_list_remove_list(inst->tempdevs, l);
               }
          }
     }
   if (eina_list_count(inst->tempdevs))
     {
        temp = 0;
        EINA_LIST_FOREACH(inst->tempdevs, l, syspath)
          {/*FIXME: probably should make a function to count the cpus and loop this */
             if ((test = eeze_udev_syspath_get_sysattr(syspath, "temp1_input")))
               {
                  temp += (strtod(test, NULL) / 1000); /* udev reports temp in (celcius * 1000) for some reason */
                  cpus++;
               }
             if ((test = eeze_udev_syspath_get_sysattr(syspath, "temp2_input")))
               {
                  temp += (strtod(test, NULL) / 1000); /* udev reports temp in (celcius * 1000) for some reason */
                  cpus++;
               }
             if ((test = eeze_udev_syspath_get_sysattr(syspath, "temp3_input")))
               {
                  temp += (strtod(test, NULL) / 1000); /* udev reports temp in (celcius * 1000) for some reason */
                  cpus++;
               }
          }
        temp /= cpus;
     }
   if (temp != -999)
     {
        if (inst->units == FAHRENHEIT)
          temp = (temp * 9.0 / 5.0) + 32;

        if (!inst->have_temp)
          {
             /* enable therm object */
             edje_object_signal_emit(inst->o_temp, "e,state,known", "");
             inst->have_temp = 1;
          }

        if (inst->units == FAHRENHEIT) 
          snprintf(buf, sizeof(buf), "%2.2f°F", temp);
        else
          snprintf(buf, sizeof(buf), "%2.2f°C", temp);  

        _temperature_face_level_set(inst,
               (double)(temp - inst->low) /
               (double)(inst->high - inst->low));
        edje_object_part_text_set(inst->o_temp, "e.text.reading", buf);
     }
   else
     {
        if (inst->have_temp)
          {
             /* disable therm object */
             edje_object_signal_emit(inst->o_temp, "e,state,unknown", "");
             edje_object_part_text_set(inst->o_temp, "e.text.reading", "N/A");
             _temperature_face_level_set(inst, 0.5);
             inst->have_temp = 0;
          }
     }
}
