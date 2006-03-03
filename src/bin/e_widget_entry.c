/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
    

typedef struct _E_Widget_Data E_Widget_Data;
struct _E_Widget_Data
{
   Evas_Object *o_entry;
   Evas_Object *obj;
   char **valptr;
   void (*on_change_func) (void *data, Evas_Object *obj);
   void  *on_change_data;
};

static void _e_wid_del_hook(Evas_Object *obj);
static void _e_wid_focus_hook(Evas_Object *obj);
static void _e_wid_disable_hook(Evas_Object *obj);
static void _e_wid_on_change_hook(void *data, Evas_Object *obj);    
//static void _e_wid_signal_cb1(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_wid_focus_steal(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_wid_text_change(void *data, Evas_Object *entry, char *key);
    
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
	e_entry_focus(wd->o_entry);
	evas_object_focus_set(wd->o_entry, 1);
	e_entry_cursor_move_at_end(wd->o_entry);
	e_entry_cursor_show(wd->o_entry);
     }
   else
     {
	e_entry_unfocus(wd->o_entry);
	evas_object_focus_set(wd->o_entry, 0);
	e_entry_cursor_hide(wd->o_entry);
     }
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
_e_wid_focus_steal(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   e_widget_focus_steal(data);
}

static void
_e_wid_on_change_hook(void *data, Evas_Object *obj)
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(obj);
   if(wd->on_change_func)
     wd->on_change_func(wd->on_change_data, obj);
}

static void
_e_wid_text_change(void *data, Evas_Object *entry, char *key)
{
   E_Widget_Data *wd;
   const char *text;
   
   wd = data;         
   E_FREE(*(wd->valptr));
   text = e_entry_text_get(wd->o_entry);
   if (!text)
     *(wd->valptr) = strdup("");
   else
     *(wd->valptr) = strdup(text);
   e_widget_change(wd->obj);   
}

/* externally accessible functions */
EAPI Evas_Object *
e_widget_entry_add(Evas *evas, char **val)
{   
   Evas_Object *obj, *o;
   E_Widget_Data *wd;
   Evas_Coord mw, mh;
   
   obj = e_widget_add(evas);

   e_widget_on_change_hook_set(obj, _e_wid_on_change_hook, NULL);
   e_widget_del_hook_set(obj, _e_wid_del_hook);
   e_widget_focus_hook_set(obj, _e_wid_focus_hook);
   e_widget_disable_hook_set(obj, _e_wid_disable_hook);
   wd = calloc(1, sizeof(E_Widget_Data));
   wd->valptr = val;
   wd->obj = obj;
   wd->on_change_func = NULL;
   wd->on_change_data = NULL;
   e_widget_data_set(obj, wd);
   
   o = e_entry_add(evas);
   wd->o_entry = o;
   e_theme_edje_object_set(o, "base/theme/widgets",
			   "widgets/entry");
   evas_object_show(o);
   edje_object_size_min_calc(o, &mw, &mh);
   e_widget_min_size_set(obj, mw, mh);
   
   if (*(wd->valptr))     
     e_entry_text_set(wd->o_entry, *(wd->valptr));
   
   e_widget_sub_object_add(obj, o);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _e_wid_focus_steal, obj);
   e_widget_resize_object_set(obj, o);

   e_entry_change_handler_set(wd->o_entry, _e_wid_text_change, wd);
   
   return obj;         
}

EAPI void
e_widget_entry_on_change_callback_set(Evas_Object *obj, void (*func) (void *data, Evas_Object *obj), void *data)
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(obj);
   wd->on_change_func = func;
   wd->on_change_data = data;
}

#if 0
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
#endif
