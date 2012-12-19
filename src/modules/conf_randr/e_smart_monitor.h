#ifdef E_TYPEDEFS
#else
# ifndef E_SMART_MONITOR_H
#  define E_SMART_MONITOR_H

typedef enum _E_Smart_Monitor_Changes E_Smart_Monitor_Changes;
enum _E_Smart_Monitor_Changes
{
   E_SMART_MONITOR_CHANGED_NONE = 0,
   E_SMART_MONITOR_CHANGED_CRTC = (1 << 0),
   E_SMART_MONITOR_CHANGED_MODE = (1 << 1),
   E_SMART_MONITOR_CHANGED_POSITION = (1 << 2),
   E_SMART_MONITOR_CHANGED_ROTATION = (1 << 3),
   E_SMART_MONITOR_CHANGED_REFRESH = (1 << 4),
   E_SMART_MONITOR_CHANGED_RESOLUTION = (1 << 5),
   E_SMART_MONITOR_CHANGED_ENABLED = (1 << 6)
};

Evas_Object *e_smart_monitor_add(Evas *evas);
void e_smart_monitor_output_set(Evas_Object *obj, E_Randr_Output_Info *output);
E_Randr_Output_Info *e_smart_monitor_output_get(Evas_Object *obj);
void e_smart_monitor_crtc_set(Evas_Object *obj, E_Randr_Crtc_Info *crtc);
void e_smart_monitor_layout_set(Evas_Object *obj, Evas_Object *layout);
Evas_Object *e_smart_monitor_layout_get(Evas_Object *obj);
void e_smart_monitor_setup(Evas_Object *obj);
E_Smart_Monitor_Changes e_smart_monitor_changes_get(Evas_Object *obj);
void e_smart_monitor_changes_reset(Evas_Object *obj);
void e_smart_monitor_changes_apply(Evas_Object *obj);

void e_smart_monitor_current_geometry_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y, Evas_Coord *w, Evas_Coord *h);
Ecore_X_Randr_Orientation e_smart_monitor_current_orientation_get(Evas_Object *mon);
Ecore_X_Randr_Mode_Info *e_smart_monitor_current_mode_get(Evas_Object *obj);
Eina_Bool e_smart_monitor_current_enabled_get(Evas_Object *obj);

void e_smart_monitor_clone_add(Evas_Object *obj, Evas_Object *mon);
void e_smart_monitor_clone_del(Evas_Object *obj, Evas_Object *mon);
void e_smart_monitor_drop_zone_set(Evas_Object *obj, Eina_Bool can_drop);

# endif
#endif
