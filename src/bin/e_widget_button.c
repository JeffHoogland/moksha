/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

typedef struct _E_Widget_Data E_Widget_Data;
struct _E_Widget_Data
{
   Evas_Object *o_button;
   Evas_Object *o_icon;
   void (*func) (void *data, void *data2);
   void *data;
   void *data2;   
};

static void _e_wid_del_hook(Evas_Object *obj);
static void _e_wid_signal_cb1(void *data, Evas_Object *obj, const char *emission, const char *source);
/* local subsystem functions */

/* externally accessible functions */
Evas_Object *
e_widget_button_add(Evas *evas, char *label, char *icon, void (*func) (void *data, void *data2), void *data, void *data2)
{
   Evas_Object *obj, *o;
   E_Widget_Data *wd;
   Evas_Coord mw, mh;
   
   obj = e_widget_add(evas);
   
   e_widget_del_hook_set(obj, _e_wid_del_hook);
   wd = calloc(1, sizeof(E_Widget_Data));
   wd->func = func;
   wd->data = data;
   wd->data2 = data2;
   e_widget_data_set(obj, wd);
   
   o = edje_object_add(evas);
   wd->o_button = o;
   e_theme_edje_object_set(o, "base/theme/widgets",
			   "widgets/button");
   edje_object_signal_callback_add(o, "click", "", _e_wid_signal_cb1, obj);
   edje_object_part_text_set(o, "label", label);
   evas_object_show(o);
   
   e_widget_sub_object_add(obj, o);
   e_widget_resize_object_set(obj, o);
   
   if (icon)
     {
	o = edje_object_add(evas);
	wd->o_icon = o;
	e_util_edje_icon_set(o, icon);
	edje_object_part_swallow(wd->o_button, "icon_swallow", o);
	edje_object_signal_emit(wd->o_button, "icon_visible", "");
	edje_object_message_signal_process(wd->o_button);
	evas_object_show(o);
	e_widget_sub_object_add(obj, o);
     }
   
   edje_object_size_min_calc(wd->o_button, &mw, &mh);
   e_widget_min_size_set(obj, mw, mh);
   
   return obj;
}

static void
_e_wid_del_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(obj);
   free(wd);
}

static void
_e_wid_signal_cb1(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(data);
   if (wd->func) wd->func(wd->data, wd->data2);
}
