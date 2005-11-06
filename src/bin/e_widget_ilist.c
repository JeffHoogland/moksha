/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

typedef struct _E_Widget_Data E_Widget_Data;
struct _E_Widget_Data
{
   Evas_Object *o_widget, *o_scrollframe, *o_ilist;
};

static void _e_wid_del_hook(Evas_Object *obj);
static void _e_wid_focus_hook(Evas_Object *obj);
static void _e_wid_cb_scrollframe_resize(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_wid_cb_item_sel(void *data, void *data2);
static void _e_wid_focus_steal(void *data, Evas *e, Evas_Object *obj, void *event_info);

/* externally accessible functions */
Evas_Object *
e_widget_ilist_add(Evas *evas, int icon_w, int icon_h)
{
   Evas_Object *obj, *o;
   E_Widget_Data *wd;
   Evas_Coord mw, mh;
   
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
   
   o = e_ilist_add(evas);
   wd->o_ilist = o;
   e_ilist_icon_size_set(o, icon_w, icon_h);
   evas_object_event_callback_add(wd->o_scrollframe, EVAS_CALLBACK_RESIZE, _e_wid_cb_scrollframe_resize, o);
   e_scrollframe_child_set(wd->o_scrollframe, o);
   e_widget_sub_object_add(obj, o);
   evas_object_show(o);
   
   evas_object_resize(obj, 32, 32);
   e_widget_min_size_set(obj, 32, 32);
//   edje_object_size_min_calc(wd->o_frame, &mw, &mh);
//   e_widget_min_size_set(obj, mw, mh);
   
   return obj;
}

void
e_widget_ilist_append(Evas_Object *obj, Evas_Object *icon, char *label, void (*func) (void *data), void *data)
{
   E_Widget_Data *wd;
   Evas_Coord mw, mh, vw, vh, w, h;
   
   wd = e_widget_data_get(obj);
   e_ilist_append(wd->o_ilist, icon, label, _e_wid_cb_item_sel, wd, data);
   if (icon) evas_object_show(icon);
   e_ilist_min_size_get(wd->o_ilist, &mw, &mh);
   evas_object_resize(wd->o_ilist, mw, mh);
   e_scrollframe_child_viewport_size_get(wd->o_scrollframe, &vw, &vh);
   evas_object_geometry_get(wd->o_scrollframe, NULL, NULL, &w, &h);
   if (mw > vw)
     {
	Evas_Coord wmw, wmh;
	
	e_widget_min_size_get(obj, &wmw, &wmh);
	e_widget_min_size_set(obj, mw + (w - vw), wmh);
     }
}

void
e_widget_ilist_go(Evas_Object *obj)
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(obj);
   wd->o_widget = obj;
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
	evas_object_focus_set(wd->o_ilist, 1);
     }
   else
     {
	edje_object_signal_emit(e_scrollframe_edje_object_get(wd->o_scrollframe), "focus_out", "");
	evas_object_focus_set(wd->o_ilist, 0);
     }
}

static void
_e_wid_cb_scrollframe_resize(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Coord mw, mh, vw, vh, w, h;

   e_scrollframe_child_viewport_size_get(obj, &vw, &vh);
   e_ilist_min_size_get(data, &mw, &mh);
   evas_object_geometry_get(data, NULL, NULL, &w, &h);
   if (vw >= mw)
     {
	if (w != vw) evas_object_resize(data, vw, h);
     }
}

static void
_e_wid_cb_item_sel(void *data, void *data2)
{
   E_Widget_Data *wd;
   Evas_Coord x, y, w, h;

   wd = data;
   e_ilist_selected_geometry_get(wd->o_ilist, &x, &y, &w, &h);
   e_scrollframe_child_region_show(wd->o_scrollframe, x, y, w, h);
   if (wd->o_widget) e_widget_change(wd->o_widget);
}

static void
_e_wid_focus_steal(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   e_widget_focus_steal(data);
}
