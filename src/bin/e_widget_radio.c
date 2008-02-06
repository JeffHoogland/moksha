/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

struct _E_Radio_Group
{
   int *valptr;
   Evas_List *radios;
};

typedef struct _E_Widget_Data E_Widget_Data;
struct _E_Widget_Data
{
   E_Radio_Group *group;
   Evas_Object *o_radio;
   Evas_Object *o_icon;
   int valnum;
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
EAPI E_Radio_Group *
e_widget_radio_group_new(int *val)
{
   E_Radio_Group *group;
   
   group = calloc(1, sizeof(E_Radio_Group));
   group->valptr = val;
   return group;
}

EAPI Evas_Object *
e_widget_radio_add(Evas *evas, const char *label, int valnum, E_Radio_Group *group)
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
   wd->group = group;
   wd->valnum = valnum;
   e_widget_data_set(obj, wd);
   
   o = edje_object_add(evas);
   wd->o_radio = o;
   e_theme_edje_object_set(o, "base/theme/widgets",
			   "e/widgets/radio");
   edje_object_signal_callback_add(o, "e,action,toggle", "*", _e_wid_signal_cb1, obj);
   edje_object_part_text_set(o, "e.text.label", label);
   evas_object_show(o);
   edje_object_size_min_calc(o, &mw, &mh);
   e_widget_min_size_set(obj, mw, mh);
   if ((wd->group) && (wd->group->valptr))
     {
	if (*(wd->group->valptr) == valnum) edje_object_signal_emit(o, "e,state,on", "e");
     }
   if (wd->group)
     {
	wd->group->radios = evas_list_append(wd->group->radios, obj);
     }
   
   e_widget_sub_object_add(obj, o);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _e_wid_focus_steal, obj);
   e_widget_resize_object_set(obj, o);
   
   return obj;
}

EAPI Evas_Object *
e_widget_radio_icon_add(Evas *evas, const char *label, const char *icon, int icon_w, int icon_h, int valnum, E_Radio_Group *group)
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
   wd->group = group;
   wd->valnum = valnum;
   e_widget_data_set(obj, wd);
   
   o = edje_object_add(evas);
   wd->o_radio = o;
   e_theme_edje_object_set(o, "base/theme/widgets",
			   "e/widgets/radio_icon");
   edje_object_signal_callback_add(o, "e,action,toggle", "*", _e_wid_signal_cb1, obj);
   edje_object_part_text_set(o, "e.text.label", label);
   evas_object_show(o);

   if (label)
     {
	edje_object_signal_emit(o, "e,state,labeled", "e");
	edje_object_message_signal_process(o);
     }
   
   if (icon)
     {
	o2 = edje_object_add(evas);
	wd->o_icon = o2;
	e_util_edje_icon_set(o2, icon);
	edje_extern_object_min_size_set(o2, icon_w, icon_h);
	edje_object_part_swallow(wd->o_radio, "e.swallow.icon", o2);
	evas_object_show(o2);
	e_widget_sub_object_add(obj, o2);
     }
  
   edje_object_size_min_calc(o, &mw, &mh);
   e_widget_min_size_set(obj, mw, mh);
   if ((wd->group) && (wd->group->valptr))
     {
	if (*(wd->group->valptr) == valnum) edje_object_signal_emit(o, "e,state,on", "e");
     }
   if (wd->group)
     {
	wd->group->radios = evas_list_append(wd->group->radios, obj);
     }
   
   e_widget_sub_object_add(obj, o);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _e_wid_focus_steal, obj);
   e_widget_resize_object_set(obj, o);
   
   return obj;
}

EAPI void
e_widget_radio_toggle_set(Evas_Object *obj, int toggle)
{
   E_Widget_Data  *wd;

   wd = e_widget_data_get(obj);
   if (!wd) return;

   if (toggle)
     {
	Evas_List *l;
	
	for (l = wd->group->radios; l; l = l->next)
	  {
	     if (l->data != obj)
	       {
		  wd = e_widget_data_get(l->data);
		  if (wd->valnum == *(wd->group->valptr))
		    {
		       edje_object_signal_emit(wd->o_radio, "e,state,off", "e");
		       break;
		    }
	       }
	  }
	wd = e_widget_data_get(obj);
	*(wd->group->valptr) = wd->valnum;
	edje_object_signal_emit(wd->o_radio, "e,state,on", "e");
     }
   else
     edje_object_signal_emit(wd->o_radio, "e,state,off", "e");
}

static void
_e_wid_del_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(obj);
   if (wd->group)
     {
	wd->group->radios = evas_list_remove(wd->group->radios, obj);
	if (!wd->group->radios) free(wd->group);
     }
   free(wd);
}

static void
_e_wid_focus_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(obj);
   if (e_widget_focus_get(obj))
     {
	edje_object_signal_emit(wd->o_radio, "e,state,focused", "e");
	evas_object_focus_set(wd->o_radio, 1);
     }
   else
     {
	edje_object_signal_emit(wd->o_radio, "e,state,unfocused", "e");
	evas_object_focus_set(wd->o_radio, 0);
     }
}

static void
_e_wid_do(Evas_Object *obj)
{
   E_Widget_Data *wd;
 
   if (e_widget_disabled_get(obj)) return;

   wd = e_widget_data_get(obj);
   if ((wd->group) && (wd->group->valptr))
     {
	Evas_List *l;
	int toggled = 0;
	
	for (l = wd->group->radios; l; l = l->next)
	  {
	     wd = e_widget_data_get(l->data);
	     if (l->data != obj)
	       {
		  wd = e_widget_data_get(l->data);
		  if (wd->valnum == *(wd->group->valptr))
		    {
		       edje_object_signal_emit(wd->o_radio, "e,state,off", "e");
		       toggled = 1;
		       break;
		    }
	       }
	  }
	if (!toggled) return;
	wd = e_widget_data_get(obj);
	*(wd->group->valptr) = wd->valnum;
	edje_object_signal_emit(wd->o_radio, "e,state,on", "e");
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
     edje_object_signal_emit(wd->o_radio, "e,state,disabled", "e");
   else
     edje_object_signal_emit(wd->o_radio, "e,state,enabled", "e");
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
