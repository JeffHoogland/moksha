#ifdef E_TYPEDEFS
#else
#ifndef E_SLIDER_H
#define E_SLIDER_H

EAPI Evas_Object *e_slider_add                      (Evas *evas);
EAPI void         e_slider_orientation_set          (Evas_Object *obj, int horizontal);
EAPI int          e_slider_orientation_get          (Evas_Object *obj);
EAPI void         e_slider_value_set                (Evas_Object *obj, double val);
EAPI double       e_slider_value_get                (Evas_Object *obj);
EAPI void         e_slider_value_range_set          (Evas_Object *obj, double min, double max);
EAPI void         e_slider_value_range_get          (Evas_Object *obj, double *min, double *max);
EAPI void         e_slider_value_step_size_set      (Evas_Object *obj, double step_size);
EAPI double       e_slider_value_step_size_get      (Evas_Object *obj);
EAPI void         e_slider_value_step_count_set     (Evas_Object *obj, int step_count);
EAPI int          e_slider_value_step_count_get     (Evas_Object *obj);
EAPI void         e_slider_value_format_display_set (Evas_Object *obj, const char *format);
EAPI const char  *e_slider_value_format_display_get (Evas_Object *obj);
EAPI void         e_slider_direction_set            (Evas_Object *obj, int reversed);
EAPI int          e_slider_direction_get            (Evas_Object *obj);
EAPI void         e_slider_size_min_get             (Evas_Object *obj, Evas_Coord *minw, Evas_Coord *minh);
EAPI Evas_Object *e_slider_edje_object_get          (Evas_Object *obj);
EAPI void         e_slider_disabled_set(Evas_Object *obj, Eina_Bool disable);
EAPI void         e_slider_special_value_add        (Evas_Object *obj, double value, double error, const char *label);
#endif
#endif
