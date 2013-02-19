#ifdef E_TYPEDEFS
#else
# ifndef E_SMART_MONITOR_H
#  define E_SMART_MONITOR_H

Evas_Object *e_smart_monitor_add(Evas *evas);
void e_smart_monitor_crtc_set(Evas_Object *obj, E_Randr_Crtc_Config *crtc);
void e_smart_monitor_output_set(Evas_Object *obj, E_Randr_Output_Config *output);

# endif
#endif
