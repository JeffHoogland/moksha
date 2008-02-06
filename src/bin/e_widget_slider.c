/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

typedef struct _E_Widget_Data E_Widget_Data;
struct _E_Widget_Data
{
   Evas_Object *o_widget, *o_slider;
   double      *dval;
   int         *ival;
};

static void _e_wid_del_hook(Evas_Object *obj);
static void _e_wid_focus_hook(Evas_Object *obj);
static void _e_wid_focus_steal(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_wid_cb_changed(void *data, Evas_Object *obj, void *event_info);
static void _e_wid_disable_hook(Evas_Object *obj);

/* externally accessible functions */
EAPI Evas_Object *
e_widget_slider_add(Evas *evas, int horiz, int rev, const char *fmt, double min, double max, double step, int count, double *dval, int *ival, Evas_Coord size)
{
   Evas_Object *obj, *o;
   E_Widget_Data *wd;
   Evas_Coord mw, mh;
   
   obj = e_widget_add(evas);
   
   e_widget_del_hook_set(obj, _e_wid_del_hook);
   e_widget_focus_hook_set(obj, _e_wid_focus_hook);
   e_widget_disable_hook_set(obj, _e_wid_disable_hook);
   wd = calloc(1, sizeof(E_Widget_Data));
   e_widget_data_set(obj, wd);
   wd->o_widget = obj;

   o = e_slider_add(evas);
   wd->o_slider = o;
   evas_object_show(o);
   e_widget_sub_object_add(obj, o);
   e_widget_resize_object_set(obj, o);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _e_wid_focus_steal, obj);
   
   e_slider_orientation_set(o, horiz);
   e_slider_direction_set(o, rev);
   e_slider_value_range_set(o, min, max);
   e_slider_value_format_display_set(o, fmt);
   e_slider_value_step_count_set(o, count);
   e_slider_value_step_size_set(o, step);
   if (dval) e_slider_value_set(o, *dval);
   else if (ival) e_slider_value_set(o, *ival);
   
   e_slider_min_size_get(o, &mw, &mh);
   if (horiz)
     e_widget_min_size_set(obj, mw + size, mh);
   else
     e_widget_min_size_set(obj, mw, mh + size);
   
   wd->dval = dval;
   wd->ival = ival;
   evas_object_smart_callback_add(o, "changed", _e_wid_cb_changed, wd);
   
   return obj;
}

/**  
 * Set the double value for the slider. This will also move the slider to
 * the correct position and update the text indecator. Will not do anything
 * if the slider was not created with a double value.
 *  
 * @param slider pointer to the slider to be modified
 * @param dval the double value to set the slider to
 * @return 1 if value set, return 0 if value not set
 */
EAPI int
e_widget_slider_value_double_set(Evas_Object *slider, double dval)
{
   E_Widget_Data *wd;   

   wd = e_widget_data_get(slider);
   if (!wd->dval) return 0;
   *(wd->dval) = dval;
   e_slider_value_set(wd->o_slider, dval);
   return 1;   
}

/**  
 * Set the integer value for the slider. This will also move the slider to
 * the correct position and update the text indecator. Will not do anything
 * if the slider was not created with an integer value. 
 *  
 * @param slider pointer to the slider to be modified
 * @param int the integer value to set the slider to
 * @return 1 if value set, return 0 if value not set
 */
EAPI int
e_widget_slider_value_int_set(Evas_Object *slider, int ival)
{
   E_Widget_Data *wd;   

   wd = e_widget_data_get(slider);
   if (!wd->ival) return 0;
   *(wd->ival) = ival;
   e_slider_value_set(wd->o_slider, ival);
   return 1; 
}

/**  
 * Get the double value for the slider. The value of dval is undefined
 * if the slider was not created with a double value.
 *  
 * @param slider pointer to the slider to be queried
 * @param dval the pointer to the double value to be set with the value of the slider
 * @return 1 if value returned, return 0 if value not returned
 */
EAPI int
e_widget_slider_value_double_get(Evas_Object *slider, double *dval)
{
   E_Widget_Data *wd;   

   wd = e_widget_data_get(slider);
   if (!wd->dval) return 0;
   if (!dval) return 0;

   *dval = *(wd->dval);
   return 1;   
}

/**  
 * Get the integer value for the slider. The value of ival is undefined
 * if the slider was not created with an integer value.
 *  
 * @param slider pointer to the slider to be queried
 * @param ival the pointer to the integer value to be set with the value of the slider
 * @return 1 if value returned, return 0 if value not returned
 */
EAPI int
e_widget_slider_value_int_get(Evas_Object *slider, int *ival)
{
   E_Widget_Data *wd;   

   wd = e_widget_data_get(slider);
   if (!wd->ival) return 0;
   if (!ival) return 0;
   
   *ival = *(wd->ival);
   return 1; 
}

/* Private functions */
static void
_e_wid_del_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(obj);
   free(wd);
}

static void
_e_wid_focus_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(obj);
   if (e_widget_focus_get(obj))
     {
	edje_object_signal_emit(e_slider_edje_object_get(wd->o_slider), "e,state,focused", "e");
	evas_object_focus_set(wd->o_slider, 1);
     }
   else
     {
	edje_object_signal_emit(e_slider_edje_object_get(wd->o_slider), "e,state,unfocused", "e");
	evas_object_focus_set(wd->o_slider, 0);
     }
}

static void
_e_wid_focus_steal(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   e_widget_focus_steal(data);
}

static void
_e_wid_cb_changed(void *data, Evas_Object *obj, void *event_info)
{
   E_Widget_Data *wd;
   
   wd = data;
   if (wd->dval) *(wd->dval) = e_slider_value_get(wd->o_slider);
   else if (wd->ival) *(wd->ival) = e_slider_value_get(wd->o_slider);
   e_widget_change(wd->o_widget);
}

static void
_e_wid_disable_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(obj);
   if (e_widget_disabled_get(obj))
     edje_object_signal_emit(e_slider_edje_object_get(wd->o_slider),
			     "e,state,disabled", "e");
   else
     edje_object_signal_emit(e_slider_edje_object_get(wd->o_slider),
			     "e,state,enabled", "e");
}
