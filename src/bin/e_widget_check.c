/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

typedef struct _E_Widget_Data E_Widget_Data;
struct _E_Widget_Data
{
   Evas_Object *o_check;
   Evas_Object *o_icon;
   int *valptr;
};

static void _e_wid_del_hook(Evas_Object *obj);
static void _e_wid_focus_hook(Evas_Object *obj);
static void _e_wid_do(Evas_Object *obj);
static void _e_wid_activate_hook(Evas_Object *obj);
static void _e_wid_disable_hook(Evas_Object *obj);
static void _e_wid_signal_cb1(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_wid_focus_steal(void *data, Evas *e, Evas_Object *obj, void *event_info);
    
/* local subsystem functions */

/* externally accessible functions */
EAPI Evas_Object *
e_widget_check_add(Evas *evas, char *label, int *val)
{
   Evas_Object *obj, *o;
   E_Widget_Data *wd;
   Evas_Coord mw, mh;
   
   obj = e_widget_add(evas);
   
   e_widget_del_hook_set(obj, _e_wid_del_hook);
   e_widget_focus_hook_set(obj, _e_wid_focus_hook);
   e_widget_activate_hook_set(obj, _e_wid_activate_hook);
   e_widget_disable_hook_set(obj, _e_wid_disable_hook);
   wd = calloc(1, sizeof(E_Widget_Data));
   wd->valptr = val;
   e_widget_data_set(obj, wd);
   
   o = edje_object_add(evas);
   wd->o_check = o;
   e_theme_edje_object_set(o, "base/theme/widgets",
			   "widgets/check");
   edje_object_signal_callback_add(o, "toggled", "*", _e_wid_signal_cb1, obj);
   edje_object_part_text_set(o, "label", label);
   evas_object_show(o);
   edje_object_size_min_calc(o, &mw, &mh);
   e_widget_min_size_set(obj, mw, mh);
   if (wd->valptr)
     {
	if (*(wd->valptr)) edje_object_signal_emit(o, "toggle_on", "");
     }
   
   e_widget_sub_object_add(obj, o);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _e_wid_focus_steal, obj);
   e_widget_resize_object_set(obj, o);
   
   return obj;
}

EAPI void
e_widget_check_checked_set(Evas_Object *check, int checked)
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(check);
   if (wd->valptr)
     *(wd->valptr) = checked;
   if (checked)
     edje_object_signal_emit(wd->o_check, "toggle_on", "");
   else
     edje_object_signal_emit(wd->o_check, "toggle_off", "");
} 

EAPI int
e_widget_check_checked_get(Evas_Object *check)
{
   E_Widget_Data *wd;
   int ret;

   wd = e_widget_data_get(check);
   if (wd->valptr)
     ret = *(wd->valptr);
   else 
     ret = -1;
   
   return ret;
}

EAPI Evas_Object *
e_widget_check_icon_add(Evas *evas, char *label, char *icon, int icon_w, int icon_h, int *val) 
{
   Evas_Object *obj, *o, *o2;
   E_Widget_Data *wd;
   Evas_Coord mw, mh;
   
   obj = e_widget_add(evas);
   
   e_widget_del_hook_set(obj, _e_wid_del_hook);
   e_widget_focus_hook_set(obj, _e_wid_focus_hook);
   e_widget_activate_hook_set(obj, _e_wid_activate_hook);
   e_widget_disable_hook_set(obj, _e_wid_disable_hook);
   wd = calloc(1, sizeof(E_Widget_Data));
   wd->valptr = val;
   e_widget_data_set(obj, wd);

   o = edje_object_add(evas);
   wd->o_check = o;
   e_theme_edje_object_set(o, "base/theme/widgets",
			   "widgets/check_icon");
   edje_object_signal_callback_add(o, "toggled", "*", _e_wid_signal_cb1, obj);
   edje_object_part_text_set(o, "label", label);
   evas_object_show(o);
   if (label)
     {
	edje_object_signal_emit(o, "label_visible", "");
	edje_object_message_signal_process(o);
     }
   if (icon) 
     {
	o2 = edje_object_add(evas);
	wd->o_icon = o2;
	e_util_edje_icon_set(o2, icon);
	edje_extern_object_min_size_set(o2, icon_w, icon_h);
	edje_object_part_swallow(wd->o_check, "icon_swallow", o2);
	evas_object_show(o2);
	e_widget_sub_object_add(obj, o2);
     }
   
   edje_object_size_min_calc(o, &mw, &mh);
   e_widget_min_size_set(obj, mw, mh);
   if (wd->valptr)
     {
	if (*(wd->valptr)) edje_object_signal_emit(o, "toggle_on", "");
     }
   
   e_widget_sub_object_add(obj, o);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _e_wid_focus_steal, obj);
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
_e_wid_focus_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(obj);
   if (e_widget_focus_get(obj))
     {
	edje_object_signal_emit(wd->o_check, "focus_in", "");
	evas_object_focus_set(wd->o_check, 1);
     }
   else
     {
	edje_object_signal_emit(wd->o_check, "focus_out", "");
	evas_object_focus_set(wd->o_check, 0);
     }
}

static void
_e_wid_do(Evas_Object *obj)
{
   E_Widget_Data *wd;

   if (e_widget_disabled_get(obj)) return;
   
   wd = e_widget_data_get(obj);
   if (wd->valptr)
     {
	if (*(wd->valptr) == 0) 
	  {
	     *(wd->valptr) = 1;
	     edje_object_signal_emit(wd->o_check, "toggle_on", "");
	  }
	else 
	  {
	     *(wd->valptr) = 0;
	     edje_object_signal_emit(wd->o_check, "toggle_off", "");
	  }
     }
   evas_object_smart_callback_call(obj, "changed", NULL);
}

static void
_e_wid_activate_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(obj);
   _e_wid_do(obj);
}

static void
_e_wid_disable_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(obj);
   if (e_widget_disabled_get(obj))
     edje_object_signal_emit(wd->o_check, "disabled", "");
   else
     edje_object_signal_emit(wd->o_check, "enabled", "");
}
     
static void
_e_wid_signal_cb1(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   _e_wid_do(data);
   e_widget_change(data);
}

static void
_e_wid_focus_steal(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   e_widget_focus_steal(data);
}
