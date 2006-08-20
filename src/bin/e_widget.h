/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS
#else
#ifndef E_WIDGET_H
#define E_WIDGET_H

EAPI Evas_Object *e_widget_add(Evas *evas);
EAPI void e_widget_del_hook_set(Evas_Object *obj, void (*func) (Evas_Object *obj));
EAPI void e_widget_focus_hook_set(Evas_Object *obj, void (*func) (Evas_Object *obj));
EAPI void e_widget_activate_hook_set(Evas_Object *obj, void (*func) (Evas_Object *obj));
EAPI void e_widget_disable_hook_set(Evas_Object *obj, void (*func) (Evas_Object *obj));
EAPI void e_widget_on_focus_hook_set(Evas_Object *obj, void (*func) (void *data, Evas_Object *obj), void *data);
EAPI void e_widget_on_change_hook_set(Evas_Object *obj, void (*func) (void *data, Evas_Object *obj), void *data);
EAPI void e_widget_data_set(Evas_Object *obj, void *data);
EAPI void *e_widget_data_get(Evas_Object *obj);
EAPI void e_widget_min_size_set(Evas_Object *obj, Evas_Coord minw, Evas_Coord minh);
EAPI void e_widget_min_size_get(Evas_Object *obj, Evas_Coord *minw, Evas_Coord *minh);
EAPI void e_widget_sub_object_add(Evas_Object *obj, Evas_Object *sobj);
EAPI void e_widget_sub_object_del(Evas_Object *obj, Evas_Object *sobj);  
EAPI void e_widget_resize_object_set(Evas_Object *obj, Evas_Object *sobj);
EAPI void e_widget_can_focus_set(Evas_Object *obj, int can_focus);
EAPI int e_widget_can_focus_get(Evas_Object *obj);
EAPI int e_widget_focus_get(Evas_Object *obj);
EAPI Evas_Object *e_widget_focused_object_get(Evas_Object *obj);
EAPI int e_widget_focus_jump(Evas_Object *obj, int forward);
EAPI void e_widget_focus_set(Evas_Object *obj, int first);
EAPI void e_widget_focused_object_clear(Evas_Object *obj);
EAPI Evas_Object *e_widget_parent_get(Evas_Object *obj);
EAPI void e_widget_focus_steal(Evas_Object *obj);
EAPI void e_widget_activate(Evas_Object *obj);
EAPI void e_widget_change(Evas_Object *obj);
EAPI void e_widget_disabled_set(Evas_Object *obj, int disabled);
EAPI int  e_widget_disabled_get(Evas_Object *obj);
EAPI E_Pointer *e_widget_pointer_get(Evas_Object *obj);

#endif
#endif
