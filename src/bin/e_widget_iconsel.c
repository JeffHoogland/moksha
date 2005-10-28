/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

typedef struct _E_Widget_Data E_Widget_Data;
struct _E_Widget_Data
{
   E_Container *con;
   Evas_Object *obj;
   Evas_Object *o_button;
   Evas_Object *o_icon;
   char **valptr;
   
   void (*select_func) (Evas_Object *obj, char *file, void *data);
   void  *select_data;
};

static void _e_wid_del_hook(Evas_Object *obj);
static void _e_wid_focus_hook(Evas_Object *obj);
static void _e_wid_activate_hook(Evas_Object *obj);
static void _e_wid_active_hook_cb(E_Fileman *fileman, char *file, void *data);
static void _e_wid_disable_hook(Evas_Object *obj);
static void _e_wid_signal_cb1(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_wid_focus_steal(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_wid_select_cb(E_File_Dialog *dia, char *file, void *data);
    
/* local subsystem functions */

/* externally accessible functions */
Evas_Object *
e_widget_iconsel_add(Evas *evas, Evas_Object *icon, Evas_Coord minw, Evas_Coord minh, char **file)
{
   Evas_Object *obj, *o;
   E_Widget_Data *wd;
   Evas_Coord mw, mh;
   E_Manager *man;  
   
   obj = e_widget_add(evas);
   
   e_widget_del_hook_set(obj, _e_wid_del_hook);
   e_widget_focus_hook_set(obj, _e_wid_focus_hook);
   e_widget_activate_hook_set(obj, _e_wid_activate_hook);
   e_widget_disable_hook_set(obj, _e_wid_disable_hook);
   wd = calloc(1, sizeof(E_Widget_Data));
   wd->valptr = file;
   wd->select_func = NULL;
   wd->select_data = NULL;
   wd->obj = obj;
   e_widget_data_set(obj, wd);
   
   man = e_manager_current_get();
   if (!man) return NULL;
   wd->con = e_container_current_get(man);
   if (!wd->con) wd->con = e_container_number_get(man, 0);
   if (!wd->con) return NULL;      
   
   o = edje_object_add(evas);
   wd->o_button = o;
   e_theme_edje_object_set(o, "base/theme/widgets",
			   "widgets/button");
   edje_object_signal_callback_add(o, "click", "", _e_wid_signal_cb1, obj);
   edje_object_part_text_set(o, "label", "");   
   evas_object_show(o);
   
   e_widget_sub_object_add(obj, o);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _e_wid_focus_steal, obj);
   e_widget_resize_object_set(obj, o);
   
   if (icon)
     {
	wd->o_icon = icon;
	edje_extern_object_min_size_set(icon, minw, minh);
	evas_object_pass_events_set(icon, 1);
	edje_object_part_swallow(wd->o_button, "icon_swallow", wd->o_icon);
	edje_object_signal_emit(wd->o_button, "icon_visible", "");
	edje_object_message_signal_process(wd->o_button);
	evas_object_show(wd->o_icon);
	e_widget_sub_object_add(obj, wd->o_icon);
     }
   
   edje_object_size_min_calc(wd->o_button, &mw, &mh);
   e_widget_min_size_set(obj, mw, mh);
   
   return obj;
}

Evas_Object *
e_widget_iconsel_add_from_file(Evas *evas, char *icon, Evas_Coord minw, Evas_Coord minh, char **file)
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
   wd->valptr = file;
   wd->select_func = NULL;
   wd->select_data = NULL;   
   wd->obj = obj;
   e_widget_data_set(obj, wd);
   
   o = edje_object_add(evas);
   wd->o_button = o;
   e_theme_edje_object_set(o, "base/theme/widgets",
			   "widgets/button");
   edje_object_signal_callback_add(o, "click", "", _e_wid_signal_cb1, obj);
   edje_object_part_text_set(o, "label", "");
   evas_object_show(o);
   
   e_widget_sub_object_add(obj, o);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _e_wid_focus_steal, obj);
   e_widget_resize_object_set(obj, o);
   
   if (icon)
     {
	o = edje_object_add(evas);
	wd->o_icon = o;
	e_util_edje_icon_set(o, icon);
	evas_object_pass_events_set(icon, 1);	
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

void
e_widget_iconsel_select_callback_add(Evas_Object *obj, void (*func)(Evas_Object *obj, char *file, void *data), void *data)
{    
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(obj);
   wd->select_func = func;
   wd->select_data = data;     
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
	edje_object_signal_emit(wd->o_button, "focus_in", "");
	evas_object_focus_set(wd->o_button, 1);
     }
   else
     {
	edje_object_signal_emit(wd->o_button, "focus_out", "");
	evas_object_focus_set(wd->o_button, 0);
     }
}

static void
_e_wid_activate_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;
   E_File_Dialog *dia;
   
   wd = e_widget_data_get(obj);
   
   dia = e_file_dialog_new(wd->con);
   if(!dia) return;
   e_file_dialog_title_set(dia, "Select File");
   e_file_dialog_select_callback_add(dia, _e_wid_select_cb, wd);
   e_file_dialog_show(dia);
}

static void
_e_wid_active_hook_cb(E_Fileman *fileman, char *file, void *data)
{  
   char *ext;
   Evas *evas;
   E_Widget_Data *wd;
   
   
   wd = e_widget_data_get(data);
   
   ext = strrchr(file, '.');
   if(!ext)
     return;
   if(strcasecmp(ext, ".png") && strcasecmp(ext, ".jpg") &&
	 strcasecmp(ext, ".jpeg"))
     return;
        
   e_icon_file_set(wd->o_icon, file);
   E_FREE(*(wd->valptr));
   *(wd->valptr) = strdup(file);
   e_object_del(fileman);
}


static void
_e_wid_disable_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(obj);
   if (e_widget_disabled_get(obj))
     edje_object_signal_emit(wd->o_button, "disabled", "");
   else
     edje_object_signal_emit(wd->o_button, "enabled", "");
}

static void
_e_wid_signal_cb1(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   e_widget_focus_steal(data);
   _e_wid_activate_hook(data);
   e_widget_change(data);
}

static void
_e_wid_focus_steal(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   e_widget_focus_steal(data);
}

static void
_e_wid_select_cb(E_File_Dialog *dia, char *file, void *data)
{
   E_Widget_Data *wd;
   char *ext;
   
   wd = data;
   
   ext = strrchr(file, '.');
   if(!ext)
     return;
   if(strcasecmp(ext, ".png") && strcasecmp(ext, ".jpg") &&
	 strcasecmp(ext, ".jpeg"))
     return;   
   
   if(wd->select_func)
     wd->select_func(wd->obj, file, wd->select_data);
      
   e_icon_file_set(wd->o_icon, file);
   E_FREE(*(wd->valptr));
   *(wd->valptr) = strdup(file);
   e_object_del(dia);
}
