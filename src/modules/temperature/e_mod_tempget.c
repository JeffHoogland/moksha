#include "e.h"
#include "e_mod_main.h"


int
_temperature_cb_exe_data(void *data, int type, void *event)
{    
   Ecore_Exe_Event_Data *ev;
   Config_Face *inst;
   int temp;

   ev = event;
   inst = data;
   if (ev->exe != inst->tempget_exe) return 1;
   temp = -999;
   if ((ev->lines) && (ev->lines[0].line))
     {
        int i;

        for (i = 0; ev->lines[i].line; i++)
          {
             if (!strcmp(ev->lines[i].line, "ERROR"))
               temp = -999;
             else
               temp = atoi(ev->lines[i].line);
          }
     }
   if (temp != -999)
     {
        char buf[256];

        if (inst->units == FAHRENHEIT)
          temp = (temp * 9.0 / 5.0) + 32;

        if (!inst->have_temp)
          {
             /* enable therm object */
             edje_object_signal_emit(inst->o_temp, "e,state,known", "");
             inst->have_temp = 1;
          }

        if (inst->units == FAHRENHEIT) 
          snprintf(buf, sizeof(buf), "%i°F", temp);
        else
          snprintf(buf, sizeof(buf), "%i°C", temp);  

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
   return 0;
}

int
_temperature_cb_exe_del(void *data, int type, void *event)
{
   Ecore_Exe_Event_Del *ev;
   Config_Face *inst;

   ev = event;
   inst = data;
   if (ev->exe != inst->tempget_exe) return 1;
   inst->tempget_exe = NULL;
   return 0;
}
