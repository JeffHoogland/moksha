#ifdef E_TYPEDEFS
#else
# ifndef E_SMART_MONITOR_H
#  define E_SMART_MONITOR_H

Evas_Object *e_smart_monitor_add(Evas *evas);
void e_smart_monitor_crtc_set(Evas_Object *obj, Ecore_X_Randr_Crtc crtc);
void e_smart_monitor_output_set(Evas_Object *obj, Ecore_X_Randr_Output output);
void e_smart_monitor_grid_set(Evas_Object *obj, Evas_Object *grid);

# endif
#endif
