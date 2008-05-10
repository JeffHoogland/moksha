/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

typedef struct _E_Widget_Data E_Widget_Data;
struct _E_Widget_Data
{
   Evas_Object *o_scrollframe, *o_child, *o_fobj;
};

static void _e_wid_del_hook(Evas_Object *obj);
static void _e_wid_focus_hook(Evas_Object *obj);
static void _e_wid_focus_steal(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_wid_cb_scrollframe_resize(void *data, Evas *e, Evas_Object *obj,
void *event_info);

/* externally accessible functions */
EAPI Evas_Object *
e_widget_scrollframe_simple_add(Evas *evas, Evas_Object *child)
{
   E_Widget_Data *wd;
   Evas_Object *obj, *o;

   obj = e_widget_add(evas);

   e_widget_del_hook_set(obj, _e_wid_del_hook);
   e_widget_focus_hook_set(obj, _e_wid_focus_hook);
   wd = calloc(1, sizeof(E_Widget_Data));
   e_widget_data_set(obj, wd);

   o = e_scrollframe_add(evas);
   e_scrollframe_policy_set(o, E_SCROLLFRAME_POLICY_AUTO, 
			       E_SCROLLFRAME_POLICY_AUTO);
   wd->o_scrollframe = o;
   evas_object_show(o);
   e_widget_sub_object_add(obj, o);
   e_widget_resize_object_set(obj, o);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, 
                                  _e_wid_focus_steal, obj);

   e_scrollframe_child_set(wd->o_scrollframe, child);
   evas_object_show(child);
   wd->o_child = child;
   e_widget_sub_object_add(obj, child);
   evas_object_event_callback_add(wd->o_scrollframe, EVAS_CALLBACK_RESIZE, 
				  _e_wid_cb_scrollframe_resize, wd->o_child);

   return obj;
}

EAPI Evas_Object *
e_widget_scrollframe_pan_add(Evas *evas, Evas_Object *pan, void (*pan_set) (Evas_Object *obj, Evas_Coord x, Evas_Coord y), void (*pan_get) (Evas_Object *obj, Evas_Coord *x, Evas_Coord *y), void (*pan_max_get) (Evas_Object *obj, Evas_Coord *x, Evas_Coord *y), void (*pan_child_size_get) (Evas_Object *obj, Evas_Coord *x, Evas_Coord *y))
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
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, 
                                  _e_wid_focus_steal, obj);

   e_scrollframe_extern_pan_set(o, pan, pan_set, pan_get, pan_max_get, 
                                pan_child_size_get);
   evas_object_show(pan);
   e_widget_sub_object_add(obj, pan);

   return obj;
}

EAPI void
e_widget_scrollframe_child_pos_set(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   e_scrollframe_child_pos_set(wd->o_scrollframe, x, y);
}

EAPI void
e_widget_scrollframe_child_pos_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   e_scrollframe_child_pos_get(wd->o_scrollframe, x, y);
}

EAPI void
e_widget_scrollframe_child_region_show(Evas_Object *obj, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   e_scrollframe_child_region_show(wd->o_scrollframe, x, y, w, h);
}

EAPI void
e_widget_scrollframe_focus_object_set(Evas_Object *obj, Evas_Object *fobj)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   wd->o_fobj = fobj;
}

EAPI Evas_Object *
e_widget_scrollframe_object_get(Evas_Object *obj)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   return wd->o_scrollframe;
}

/* Private functions */
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
	edje_object_signal_emit(e_scrollframe_edje_object_get(wd->o_scrollframe), "e,state,focused", "e");
	if (wd->o_fobj) evas_object_focus_set(wd->o_fobj, 1);
	else evas_object_focus_set(wd->o_scrollframe, 1);
     }
   else
     {
	edje_object_signal_emit(e_scrollframe_edje_object_get(wd->o_scrollframe), "e,state,unfocused", "e");
	if (wd->o_fobj) evas_object_focus_set(wd->o_fobj, 0);
	evas_object_focus_set(wd->o_scrollframe, 0);
     }
}

static void
_e_wid_focus_steal(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   e_widget_focus_steal(data);
}

static void
_e_wid_cb_scrollframe_resize(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Coord mw, mh, vw, vh, w, h;    

   e_scrollframe_child_viewport_size_get(obj, &vw, &vh);
   e_widget_min_size_get(data, &mw, &mh);
   evas_object_geometry_get(data, NULL, NULL, &w, &h);
   if (vw >= mw)
     {
	if (w != vw) evas_object_resize(data, vw, h);
     }
}
