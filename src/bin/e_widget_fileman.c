/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
    

typedef struct _E_Widget_Data E_Widget_Data;
struct _E_Widget_Data
{
   Evas_Object *o_fm;
   char **valptr;
   Ecore_Event_Handler *change_handler;
};

static void _e_wid_del_hook(Evas_Object *obj);
static void _e_wid_focus_steal(void *data, Evas *e, Evas_Object *obj, void *event_info);
static int  _e_wid_text_change(void *data, Evas_Object *entry, char *key);
    
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

static int
_e_wid_text_change(void *data, Evas_Object *entry, char *key)
{
#if 0   
   E_Widget_Data *wd;
   int size;
   
   wd = data;      
   
   if(*(wd->valptr) == NULL) 
     {
	size = (strlen(key) + 1) * sizeof(char);
	*(wd->valptr) = realloc(*(wd->valptr), size);   
	snprintf(*(wd->valptr), size, "%s", key);
     }
   else
     {
	char *tmp;
	
	size = (strlen(*(wd->valptr)) + strlen(key) + 1) * sizeof(char);
	tmp = E_NEW(char *, strlen(*(wd->valptr)) + 1);
	snprintf(tmp, strlen(*(wd->valptr)) + 1, "%s", *(wd->valptr));	
	*(wd->valptr) = realloc(*(wd->valptr), size);   
	snprintf(*(wd->valptr), size, "%s%s\0", tmp, key);
	E_FREE(tmp);
     }
#endif   
}

/* externally accessible functions */
Evas_Object *
e_widget_fileman_add(Evas *evas, char **val)
{   
   Evas_Object *obj, *o, *bg, *scrollbar;
   E_Widget_Data *wd;
   Evas_Coord mw, mh;   
   
   obj = e_widget_add(evas);
   
   e_widget_del_hook_set(obj, _e_wid_del_hook);

   wd = calloc(1, sizeof(E_Widget_Data));
   wd->valptr = val;
   e_widget_data_set(obj, wd);
   
   bg = edje_object_add(evas);
   e_theme_edje_object_set(bg, "base/theme/widgets/fileselector",
			   "widgets/fileselector/main");   
   evas_object_show(bg);
   
   o = e_fm_add(evas);
   wd->o_fm = o;
   
   scrollbar = e_scrollbar_add(evas);
   e_scrollbar_direction_set(scrollbar, E_SCROLLBAR_VERTICAL);
   
   edje_object_part_swallow(bg, "items", wd->o_fm);
   edje_object_part_swallow(bg, "vscrollbar", scrollbar);
   evas_object_show(bg);
   evas_object_resize(bg, 300, 200);
   e_widget_min_size_set(obj, 300, 200);
      
   e_widget_sub_object_add(obj, bg);
   evas_object_event_callback_add(bg, EVAS_CALLBACK_MOUSE_DOWN, _e_wid_focus_steal, obj);
   e_widget_resize_object_set(obj, bg);
   
   return obj;         
}
