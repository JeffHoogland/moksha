/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
    
/*- DESCRIPTION -*/
/* This widget simply wraps e_file_selector into a widget. When a file is 
 * selected, the assigned value to valptr changes and contains the new file.
 */ 

typedef struct _E_Widget_Data E_Widget_Data;
struct _E_Widget_Data
{
   Evas_Object *wid;
   Evas_Object *o_fm;
   char **valptr;
   void (*select_func) (Evas_Object *obj, char *file, void *data);
   void  *select_data;
   void (*hilite_func) (Evas_Object *obj, char *file, void *data);
   void  *hilite_data;   
};

static void _e_wid_del_hook(Evas_Object *obj);
static void _e_wid_focus_steal(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_wid_fileman_selected_cb(Evas_Object *obj, char *file, void *data);
static void _e_wid_fileman_hilited_cb(Evas_Object *obj, char *file, void *data);
        
/* local subsystem functions */

static void
_e_wid_del_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(obj);
   free(wd);
}

static void
_e_wid_focus_steal(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   e_widget_focus_steal(data);
}

static void
_e_wid_fileman_selected_cb(Evas_Object *obj, char *file, void *data)
{
   E_Widget_Data *wd;
   
   wd = data;

/* this is crashing, see why */
#if 0
   E_FREE(*(wd->valptr));
#endif   
   if(wd->valptr)
     *(wd->valptr) = strdup(file);


   if (wd->select_func)
     wd->select_func(wd->wid, file, wd->select_data);

   printf("e_widget_fileman (selected) : %s\n", file);
}

static void
_e_wid_fileman_hilited_cb(Evas_Object *obj, char *file, void *data)
{
   E_Widget_Data *wd;
   
   wd = data;

/* this is crashing, see why */
#if 0
   E_FREE(*(wd->valptr));
#endif
   if(wd->valptr)
     *(wd->valptr) = strdup(file);


   if (wd->hilite_func)
     wd->hilite_func(wd->wid, file, wd->hilite_data);
     
   printf("e_widget_fileman (hilited): %s\n", file);
}

/* externally accessible functions */
EAPI Evas_Object *
e_widget_fileman_add(Evas *evas, char **val)
{   
   Evas_Object *obj;
   E_Widget_Data *wd;
   
   obj = e_widget_add(evas);
   
   e_widget_del_hook_set(obj, _e_wid_del_hook);

   wd = calloc(1, sizeof(E_Widget_Data));
   wd->valptr = val;
   wd->select_func = NULL;
   wd->select_data = NULL;
   e_widget_data_set(obj, wd);
   
   wd->o_fm = e_file_selector_add(evas);
   e_file_selector_callback_add(wd->o_fm, _e_wid_fileman_selected_cb, _e_wid_fileman_hilited_cb, wd);
   evas_object_show(wd->o_fm);   
   evas_object_resize(wd->o_fm, 300, 200);
   e_widget_min_size_set(obj, 300, 200);
   
   e_widget_sub_object_add(obj, wd->o_fm);
   evas_object_event_callback_add(wd->o_fm, EVAS_CALLBACK_MOUSE_DOWN, _e_wid_focus_steal, obj);
   e_widget_resize_object_set(obj, wd->o_fm);
   
   wd->wid = obj;
   return obj;         
}

EAPI void
e_widget_fileman_select_callback_add(Evas_Object *obj, void (*func) (Evas_Object *obj, char *file, void *data), void *data)
{
   E_Widget_Data *wd;   
   
   wd = e_widget_data_get(obj);
   wd->select_func = func;
   wd->select_data = data;
}

EAPI void
e_widget_fileman_hilite_callback_add(Evas_Object *obj, void (*func) (Evas_Object *obj, char *file, void *data), void *data)
{
   E_Widget_Data *wd;   
   
   wd = e_widget_data_get(obj);
   wd->hilite_func = func;
   wd->hilite_data = data;
}

EAPI void
e_widget_fileman_dir_set(Evas_Object *obj, const char *dir)
{
	E_Widget_Data *wd;
	
	wd = e_widget_data_get(obj);
	e_file_selector_dir_set(wd->o_fm, dir);
}
