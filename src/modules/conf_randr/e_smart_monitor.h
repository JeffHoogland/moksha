#ifdef E_TYPEDEFS
#else
# ifndef E_SMART_MONITOR_H
#  define E_SMART_MONITOR_H

typedef enum _E_Smart_Monitor_Changes E_Smart_Monitor_Changes;
enum _E_Smart_Monitor_Changes
{
   E_SMART_MONITOR_CHANGED_NONE = 0,
   E_SMART_MONITOR_CHANGED_MODE = (1 << 0),
   E_SMART_MONITOR_CHANGED_POSITION = (1 << 1),
   E_SMART_MONITOR_CHANGED_ORIENTATION = (1 << 2),
   E_SMART_MONITOR_CHANGED_ENABLED = (1 << 3),
   E_SMART_MONITOR_CHANGED_PRIMARY = (1 << 4)
};

Evas_Object *e_smart_monitor_add(Evas *evas);
void e_smart_monitor_crtc_set(Evas_Object *obj, Ecore_X_Randr_Crtc crtc, Evas_Coord cx, Evas_Coord cy, Evas_Coord cw, Evas_Coord ch);
Ecore_X_Randr_Crtc e_smart_monitor_crtc_get(Evas_Object *obj);
void e_smart_monitor_output_set(Evas_Object *obj, Ecore_X_Randr_Output output);
void e_smart_monitor_grid_set(Evas_Object *obj, Evas_Object *grid, Evas_Coord gx, Evas_Coord gy, Evas_Coord gw, Evas_Coord gh);
void e_smart_monitor_grid_virtual_size_set(Evas_Object *obj, Evas_Coord vw, Evas_Coord vh);
void e_smart_monitor_background_set(Evas_Object *obj, Evas_Coord dx, Evas_Coord dy);
void e_smart_monitor_current_geometry_set(Evas_Object *obj, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h);
void e_smart_monitor_current_geometry_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y, Evas_Coord *w, Evas_Coord *h);
void e_smart_monitor_previous_geometry_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y, Evas_Coord *w, Evas_Coord *h);
void e_smart_monitor_clone_set(Evas_Object *obj, Evas_Object *parent);
Evas_Object *e_smart_monitor_clone_parent_get(Evas_Object *obj);
E_Smart_Monitor_Changes e_smart_monitor_changes_get(Evas_Object *obj);
Eina_Bool e_smart_monitor_changes_apply(Evas_Object *obj);
const char *e_smart_monitor_name_get(Evas_Object *obj);
Ecore_X_Randr_Output e_smart_monitor_output_get(Evas_Object *obj);
void e_smart_monitor_indicator_available_set(Evas_Object *obj, Eina_Bool available);

# endif
#endif
