/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
    

typedef struct _E_Widget_Data E_Widget_Data;
struct _E_Widget_Data
{
   Evas_Object *o_entry;
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
	edje_object_signal_emit(wd->o_entry, "focus", "");
	evas_object_focus_set(wd->o_entry, 1);
	e_entry_cursor_show(wd->o_entry);
     }
   else
     {
	edje_object_signal_emit(wd->o_entry, "unfocus", "");
	evas_object_focus_set(wd->o_entry, 0);
	e_entry_cursor_hide(wd->o_entry);
     }
}

static void
_e_wid_do(Evas_Object *obj)
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(obj);
#if 0   
   if (wd->valptr)
     {
	if (*(wd->valptr) == 0) *(wd->valptr) = 1;
	else *(wd->valptr) = 0;
     }
#endif   
}

static void
_e_wid_activate_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(obj);
#if 0   
   _e_wid_do(obj);
   if (wd->valptr)
     {
	if (*(wd->valptr)) edje_object_signal_emit(wd->o_entry, "toggle_on", "");
	else edje_object_signal_emit(wd->o_entry, "toggle_off", "");
     }
#endif   
}

static void
_e_wid_disable_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(obj);
   if (e_widget_disabled_get(obj))
     edje_object_signal_emit(wd->o_entry, "disabled", "");
   else
     edje_object_signal_emit(wd->o_entry, "enabled", "");
}

static void
_e_wid_signal_cb1(void *data, Evas_Object *obj, const char *emission, const char *source)
{
#if 0   
   _e_wid_do(data);
   e_widget_change(data);
#endif   
}

static void
_e_wid_focus_steal(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   e_widget_focus_steal(data);
}

/* externally accessible functions */
Evas_Object *
e_widget_entry_add(Evas *evas, char *val)
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
   
   o = e_entry_add(evas);
   wd->o_entry = o;
   e_theme_edje_object_set(o, "base/theme/widgets",
			   "widgets/entry");
   evas_object_show(o);
   edje_object_size_min_calc(o, &mw, &mh);
   e_widget_min_size_set(obj, mw, mh);
   if (wd->valptr)
     {
	
     }
   
   e_widget_sub_object_add(obj, o);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _e_wid_focus_steal, obj);
   e_widget_resize_object_set(obj, o);
   
   return obj;         
}



void             
e_widget_entry_text_set(Evas_Object *entry, const char *text)
{
   e_entry_text_set(entry, text);     
}

const char *
e_widget_entry_text_get(Evas_Object *entry)
{
   return e_entry_text_get(entry);
}

void
e_widget_entry_text_insert(Evas_Object *entry, const char *text)
{
   e_entry_text_insert(entry, text);
}

void
e_widget_entry_delete_char_before(Evas_Object *entry)
{
   e_entry_delete_char_before(entry);
}

void
e_widget_entry_delete_char_after(Evas_Object *entry)
{
   e_entry_delete_char_after(entry);     
}

void
e_widget_entry_cursor_move_at_start(Evas_Object *entry)
{
   e_entry_cursor_move_at_start(entry);     
}

void
e_widget_entry_cursor_move_at_end(Evas_Object *entry)
{
   e_entry_cursor_move_at_end(entry);     
}

void
e_widget_entry_cursor_move_left(Evas_Object *entry)
{
   e_entry_cursor_move_left(entry);
}

void
e_widget_entry_cursor_move_right(Evas_Object *entry)
{
   e_entry_cursor_move_right(entry);
}

void
e_widget_entry_cursor_show(Evas_Object *entry)
{
   e_entry_cursor_show(entry);
}

void
e_widget_entry_cursor_hide(Evas_Object *entry)
{
   e_entry_cursor_hide(entry);
}
