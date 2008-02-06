/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */

/* FIXME: This should really be merged with ilist, but raster says not to hack ilist. */

#include "e.h"

typedef struct _E_Widget_Data E_Widget_Data;
typedef struct _E_Widget_Callback E_Widget_Callback;
struct _E_Widget_Data
{
   Evas_Object *o_widget, *o_scrollframe, *o_tlist;
   Evas_List  *callbacks;
   char  **value;
};
struct _E_Widget_Callback
{
   void (*func) (void *data);
   void *data;
   char *value;
};

static void _e_widget_tlist_append(Evas_Object *obj, const char *label,
                                   void (*func) (void *data),
                                   void *data, const char *val,
                                   int markup);
static void _e_wid_del_hook(Evas_Object *obj);
static void _e_wid_focus_hook(Evas_Object *obj);
static void _e_wid_cb_scrollframe_resize(void *data, Evas *e, 
                                         Evas_Object *obj,
                                         void *event_info);
static void _e_wid_cb_item_sel(void *data, void *data2);
static void _e_wid_cb_item_hilight(void *data, void *data2);
static void _e_wid_focus_steal(void *data, Evas *e, Evas_Object *obj,
                               void *event_info);

/* externally accessible functions */
EAPI Evas_Object *
e_widget_tlist_add(Evas *evas, char **value)
{
   Evas_Object *obj, *o;
   E_Widget_Data *wd;

   obj = e_widget_add(evas);

   e_widget_del_hook_set(obj, _e_wid_del_hook);
   e_widget_focus_hook_set(obj, _e_wid_focus_hook);
   wd = calloc(1, sizeof(E_Widget_Data));
   e_widget_data_set(obj, wd);

   wd->value = value;

   o = e_scrollframe_add(evas);
   wd->o_scrollframe = o;
   evas_object_show(o);
   e_widget_sub_object_add(obj, o);
   e_widget_resize_object_set(obj, o);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN,
				  _e_wid_focus_steal, obj);

   o = e_tlist_add(evas);
   wd->o_tlist = o;
   evas_object_event_callback_add(wd->o_scrollframe, EVAS_CALLBACK_RESIZE,
				  _e_wid_cb_scrollframe_resize, o);
   e_scrollframe_child_set(wd->o_scrollframe, o);
   e_widget_sub_object_add(obj, o);
   evas_object_show(o);

   evas_object_resize(obj, 32, 32);
   e_widget_min_size_set(obj, 32, 32);
   return obj;
}

EAPI void
e_widget_tlist_append(Evas_Object *obj, const char *label,
		      void (*func) (void *data), void *data, const char *val)
{
   _e_widget_tlist_append(obj, label, func, data, val, 0);
}

EAPI void
e_widget_tlist_markup_append(Evas_Object *obj, const char *label,
			     void (*func) (void *data), void *data,
			     const char *val)
{
   _e_widget_tlist_append(obj, label, func, data, val, 1);
}

EAPI void
e_widget_tlist_selected_set(Evas_Object *obj, int n)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   e_tlist_selected_set(wd->o_tlist, n);
}

EAPI int
e_widget_tlist_selected_get(Evas_Object *obj)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   return e_tlist_selected_get(wd->o_tlist);
}

EAPI const char *
e_widget_tlist_selected_label_get(Evas_Object *obj)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   return e_tlist_selected_label_get(wd->o_tlist);
}

EAPI void
e_widget_tlist_selector_set(Evas_Object *obj, int selector)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   e_tlist_selector_set(wd->o_tlist, selector);
}

EAPI void
e_widget_tlist_go(Evas_Object *obj)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   wd->o_widget = obj;
}

EAPI void
e_widget_tlist_remove_num(Evas_Object *obj, int n)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   e_tlist_remove_num(wd->o_tlist, n);
}

EAPI void
e_widget_tlist_remove_label(Evas_Object *obj, const char *label)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   e_tlist_remove_label(wd->o_tlist, label);
}

EAPI int
e_widget_tlist_count(Evas_Object *obj)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   return e_tlist_count(wd->o_tlist);
}

static void
_e_widget_tlist_append(Evas_Object *obj, const char *label,
		       void (*func) (void *data), void *data, const char *val,
		       int markup)
{
   E_Widget_Data *wd;
   Evas_Coord mw, mh, vw, vh, w, h;
   E_Widget_Callback *wcb;

   wd = e_widget_data_get(obj);
   wcb = E_NEW(E_Widget_Callback, 1);
   wcb->func = func;
   wcb->data = data;
   if (val) wcb->value = strdup(val);
   wd->callbacks = evas_list_append(wd->callbacks, wcb);
   if (markup)
     e_tlist_markup_append(wd->o_tlist, label, _e_wid_cb_item_sel,
                           _e_wid_cb_item_hilight, wd, wcb);
   else
     e_tlist_append(wd->o_tlist, label, _e_wid_cb_item_sel,
                    _e_wid_cb_item_hilight, wd, wcb);
   e_tlist_min_size_get(wd->o_tlist, &mw, &mh);
   evas_object_resize(wd->o_tlist, mw, mh);
   e_scrollframe_child_viewport_size_get(wd->o_scrollframe, &vw, &vh);
   evas_object_geometry_get(wd->o_scrollframe, NULL, NULL, &w, &h);
   if (mw > vw)
     {
	Evas_Coord wmw, wmh;

	e_widget_min_size_get(obj, &wmw, &wmh);
	e_widget_min_size_set(obj, mw + (w - vw), wmh);
     }
   else if (mw < vw)
     evas_object_resize(wd->o_tlist, vw, mh);
}

EAPI void
e_widget_tlist_clear(Evas_Object *obj) 
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   e_tlist_clear(wd->o_tlist);
}

static void
_e_wid_del_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   while (wd->callbacks)
     {
	E_Widget_Callback  *wcb;

	wcb = wd->callbacks->data;
	if (wcb->value) free(wcb->value);
	free(wcb);
	wd->callbacks = evas_list_remove_list(wd->callbacks, wd->callbacks);
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
	edje_object_signal_emit(e_scrollframe_edje_object_get
				(wd->o_scrollframe), "e,state,focused", "e");
	evas_object_focus_set(wd->o_tlist, 1);
     }
   else
     {
	edje_object_signal_emit(e_scrollframe_edje_object_get
				(wd->o_scrollframe), "e,state,unfocused", "e");
	evas_object_focus_set(wd->o_tlist, 0);
     }
}

static void
_e_wid_cb_scrollframe_resize(void *data, Evas *e, Evas_Object *obj,
			     void *event_info)
{
   Evas_Coord mw, mh, vw, vh, w, h;

   e_scrollframe_child_viewport_size_get(obj, &vw, &vh);
   e_tlist_min_size_get(data, &mw, &mh);
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
   E_Widget_Callback *wcb;

   wd = data;
   wcb = data2;
   e_tlist_selected_geometry_get(wd->o_tlist, &x, &y, &w, &h);
   e_scrollframe_child_region_show(wd->o_scrollframe, x, y, w, h);
   if (wd->o_widget)
     {
	e_widget_change(wd->o_widget);
	if (wd->value)
	  {
	     if (*(wd->value))
               free(*(wd->value));
	     if (wcb->value)
               *(wd->value) = strdup(wcb->value);
	     else
               *(wd->value) = NULL;
	  }
	if (wcb->func) wcb->func(wcb->data);
     }
}

static void
_e_wid_cb_item_hilight(void *data, void *data2)
{
   E_Widget_Data *wd;
   Evas_Coord x, y, w, h;
   E_Widget_Callback *wcb;

   wd = data;
   wcb = data2;
   e_tlist_selected_geometry_get(wd->o_tlist, &x, &y, &w, &h);
   e_scrollframe_child_region_show(wd->o_scrollframe, x, y, w, h);
}

static void
_e_wid_focus_steal(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   e_widget_focus_steal(data);
}
