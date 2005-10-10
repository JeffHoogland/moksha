/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

typedef struct _E_Widget_Data E_Widget_Data;
struct _E_Widget_Data
{
   int *valptr;
};

static void _e_wid_del_hook(Evas_Object *obj);
static void _e_wid_signal_cb1(void *data, Evas_Object *obj, const char *emission, const char *source);
/* local subsystem functions */

/* externally accessible functions */
Evas_Object *
e_widget_checkbox_add(Evas *evas, char *label, int *val)
{
   Evas_Object *obj, *o;
   E_Widget_Data *wd;
   Evas_Coord mw, mh;
   
   obj = e_widget_add(evas);
   
   e_widget_del_hook_set(obj, _e_wid_del_hook);
   wd = calloc(1, sizeof(E_Widget_Data));
   wd->valptr = val;
   e_widget_data_set(obj, wd);
   
   o = edje_object_add(evas);
   e_theme_edje_object_set(o, "base/theme/widgets",
			   "widgets/check");
   edje_object_signal_callback_add(o, "toggled", "*", _e_wid_signal_cb1, obj);
   edje_object_part_text_set(o, "label", label);
   edje_object_size_min_calc(o, &mw, &mh);
   e_widget_min_size_set(obj, mw, mh);
   if (wd->valptr)
     {
	if (*(wd->valptr)) edje_object_signal_emit(o, "toggle_on", "");
     }
   
   e_widget_sub_object_add(obj, o);
   e_widget_resize_object_set(obj, o);
   
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
   if (wd->valptr)
     {
	if (!strcmp(source, "on")) *(wd->valptr) = 1;
	else *(wd->valptr) = 0;
     }
}
