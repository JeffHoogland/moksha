/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

typedef struct _E_Widget_Data E_Widget_Data;
struct _E_Widget_Data
{
   Evas_Object *o_widget, *o_scrollframe, *o_textblock;
};

static void _e_wid_del_hook(Evas_Object *obj);
static void _e_wid_focus_hook(Evas_Object *obj);
static void _e_wid_cb_scrollframe_resize(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_wid_focus_steal(void *data, Evas *e, Evas_Object *obj, void *event_info);

/* externally accessible functions */
EAPI Evas_Object *
e_widget_textblock_add(Evas *evas)
{
   Evas_Object *obj, *o;
   E_Widget_Data *wd;
   
   obj = e_widget_add(evas);
   
   e_widget_del_hook_set(obj, _e_wid_del_hook);
   e_widget_focus_hook_set(obj, _e_wid_focus_hook);
   wd = calloc(1, sizeof(E_Widget_Data));
   e_widget_data_set(obj, wd);

   o = e_scrollframe_add(evas);
   wd->o_scrollframe = o;
   evas_object_show(o);
   e_widget_sub_object_add(obj, o);
   e_widget_resize_object_set(obj, o);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _e_wid_focus_steal, obj);

   o = edje_object_add(evas);
   e_theme_edje_object_set(o, "base/theme/widgets",
			   "widgets/textblock");
   wd->o_textblock = o;
   evas_object_event_callback_add(wd->o_scrollframe, EVAS_CALLBACK_RESIZE, _e_wid_cb_scrollframe_resize, wd);
   e_scrollframe_child_set(wd->o_scrollframe, o);
   e_widget_sub_object_add(obj, o);
   evas_object_show(o);
		       
   evas_object_resize(obj, 32, 32);
   e_widget_min_size_set(obj, 32, 32);
   return obj;
}

EAPI void
e_widget_textblock_markup_set(Evas_Object *obj, const char *text)
{
   E_Widget_Data *wd;
   Evas_Coord mw, mh, vw, vh;

   wd = e_widget_data_get(obj);
   edje_object_part_text_set(wd->o_textblock, "text", text);
   edje_object_size_min_calc(wd->o_textblock, &mw, &mh);
   e_scrollframe_child_viewport_size_get(wd->o_scrollframe, &vw, &vh);
   if (vw > mw) mw = vw;
   if (vh > mh) mh = vh;
   evas_object_resize(wd->o_textblock, mw, mh);
}

EAPI void
e_widget_textblock_plain_set(Evas_Object *obj, const char *text)
{
   /* FIXME: parse text escape anything htmlish, - generate new text, set
    * as markup
    */
   e_widget_textblock_markup_set(obj, text);
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
	edje_object_signal_emit(e_scrollframe_edje_object_get(wd->o_scrollframe), "focus_in", "");
	evas_object_focus_set(wd->o_scrollframe, 1);
     }
   else
     {
	edje_object_signal_emit(e_scrollframe_edje_object_get(wd->o_scrollframe), "focus_out", "");
	evas_object_focus_set(wd->o_scrollframe, 0);
     }
}

static void
_e_wid_cb_scrollframe_resize(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   E_Widget_Data *wd;
   Evas_Coord mw, mh, vw, vh;
   
   wd = data;
   e_scrollframe_child_viewport_size_get(obj, &vw, &vh);
   edje_object_size_min_calc(wd->o_textblock, &mw, &mh);
   e_scrollframe_child_viewport_size_get(wd->o_scrollframe, &vw, &vh);
   if (vw > mw) mw = vw;
   if (vh > mh) mh = vh;
   evas_object_resize(wd->o_textblock, mw, mh);
}
   
static void
_e_wid_focus_steal(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   e_widget_focus_steal(data);
}
