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
void e_smart_monitor_layout_set(Evas_Object *obj, Evas_Object *layout);
Evas_Object *e_smart_monitor_layout_get(Evas_Object *obj);
void e_smart_monitor_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
void e_smart_monitor_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h);

# endif
#endif
