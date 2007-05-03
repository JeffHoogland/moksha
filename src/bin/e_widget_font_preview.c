/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

typedef struct _E_Widget_Data E_Widget_Data;
struct _E_Widget_Data
{
   Evas_Object *o_scrollframe;
   Evas_Object *o_text;
};
static void _e_wid_cb_scrollframe_resize(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_wid_focus_steal(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_wid_del_hook(Evas_Object *obj);

/* local subsystem functions */

/* externally accessible functions */
EAPI Evas_Object *
e_widget_font_preview_add(Evas *evas, const char *text)
{
   Evas_Object *obj, *o;
   E_Widget_Data *wd;
   
   obj = e_widget_add(evas);
   e_widget_del_hook_set(obj, _e_wid_del_hook);
   wd = calloc(1, sizeof(E_Widget_Data));
   e_widget_data_set(obj, wd);

   /* Add Scrollpane */
   o = e_scrollframe_add(evas);
   wd->o_scrollframe = o;
   e_scrollframe_policy_set(o, E_SCROLLFRAME_POLICY_OFF, E_SCROLLFRAME_POLICY_OFF);
   evas_object_show(o);
   e_widget_sub_object_add(obj, o);
   e_widget_resize_object_set(obj, o);

   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _e_wid_focus_steal, obj);

   /* Add Text */
   o = edje_object_add(evas);
   wd->o_text = o;
   e_theme_edje_object_set(o, "base/theme/widgets",
			      "e/widgets/fontpreview");
   evas_object_event_callback_add(wd->o_scrollframe, EVAS_CALLBACK_RESIZE, _e_wid_cb_scrollframe_resize, wd);
   e_scrollframe_child_set(wd->o_scrollframe, o);
   e_widget_sub_object_add(obj, o);
   evas_object_show(o);
   
   edje_object_part_text_set(wd->o_text, "e.fontpreview.text", text);

   evas_object_resize(obj, 40, 40);  
   e_widget_min_size_set(obj, 40, 40);
   
   return obj;
}

EAPI void
e_widget_font_preview_font_set(Evas_Object *obj, const char *font, Evas_Font_Size size)
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(obj);

   if (size < 0)
     size = (-size * 10) / 100;
   if (size == 0)
     size = 10;
  
   edje_object_text_class_set(wd->o_text, "_e_font_preview", font, size); 
}


static void
_e_wid_del_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(obj);
   free(wd);
}

static void
_e_wid_cb_scrollframe_resize(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   E_Widget_Data *wd;
   Evas_Coord mw, mh, vw, vh;
   
   wd = data;
   e_scrollframe_child_viewport_size_get(obj, &vw, &vh);
   edje_object_size_min_calc(wd->o_text, &mw, &mh);
   e_scrollframe_child_viewport_size_get(wd->o_scrollframe, &vw, &vh);
   if (vw > mw) mw = vw;
   if (vh > mh) mh = vh;
   evas_object_resize(wd->o_text, mw, mh);
}
   
static void
_e_wid_focus_steal(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   e_widget_focus_steal(data);
}
