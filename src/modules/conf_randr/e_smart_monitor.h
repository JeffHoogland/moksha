#ifdef E_TYPEDEFS
#else
# ifndef E_SMART_MONITOR_H

Evas_Object *e_smart_monitor_add(Evas *evas);
void e_smart_monitor_layout_set(Evas_Object *obj, Evas_Object *layout);
void e_smart_monitor_crtc_set(Evas_Object *obj, E_Randr_Crtc_Info *crtc);
E_Randr_Crtc_Info *e_smart_monitor_crtc_get(Evas_Object *obj);
void e_smart_monitor_crtc_geometry_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y, Evas_Coord *w, Evas_Coord *h);
void e_smart_monitor_move_geometry_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y, Evas_Coord *w, Evas_Coord *h);
Eina_Bool e_smart_monitor_moving_get(Evas_Object *obj);

#  define E_SMART_MONITOR_H
# endif
#endif
