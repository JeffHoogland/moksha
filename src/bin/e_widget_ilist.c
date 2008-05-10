/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

typedef struct _E_Widget_Data E_Widget_Data;
typedef struct _E_Widget_Callback E_Widget_Callback;
struct _E_Widget_Data
{
   Evas_Object *o_widget, *o_scrollframe, *o_ilist;
   Evas_List *callbacks;
   char **value;
};
struct _E_Widget_Callback
{
   void (*func) (void *data);
   void  *data;
   char  *value;
};

static void _e_wid_del_hook(Evas_Object *obj);
static void _e_wid_focus_hook(Evas_Object *obj);
static void _e_wid_cb_scrollframe_resize(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_wid_cb_item_sel(void *data, void *data2);
static void _e_wid_cb_item_hilight(void *data, void *data2);
static void _e_wid_cb_selected(void *data, Evas_Object *obj, void *event_info);
static void _e_wid_focus_steal(void *data, Evas *e, Evas_Object *obj, void *event_info);

/* externally accessible functions */
EAPI Evas_Object *
e_widget_ilist_add(Evas *evas, int icon_w, int icon_h, char **value)
{
   Evas_Object *obj, *o;
   E_Widget_Data *wd;

   wd = E_NEW(E_Widget_Data, 1);
   if (!wd) return NULL;

   obj = e_widget_add(evas);

   e_widget_del_hook_set(obj, _e_wid_del_hook);
   e_widget_focus_hook_set(obj, _e_wid_focus_hook);
   e_widget_data_set(obj, wd);

   wd->value = value;

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
   evas_object_smart_callback_add(o, "selected", _e_wid_cb_selected, obj);

   evas_object_resize(obj, 32, 32);
   e_widget_min_size_set(obj, 32, 32);
   return obj;
}

EAPI void
e_widget_ilist_freeze(Evas_Object *obj)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   e_ilist_freeze(wd->o_ilist);
}

EAPI void
e_widget_ilist_thaw(Evas_Object *obj)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   e_ilist_thaw(wd->o_ilist);
}

EAPI void
e_widget_ilist_append(Evas_Object *obj, Evas_Object *icon, const char *label, void (*func) (void *data), void *data, const char *val)
{
   E_Widget_Data *wd;
   E_Widget_Callback *wcb;

   wcb = E_NEW(E_Widget_Callback, 1);
   if (!wcb) return;

   wd = e_widget_data_get(obj);
   wcb->func = func;
   wcb->data = data;
   if (val) wcb->value = strdup(val);
   wd->callbacks = evas_list_append(wd->callbacks, wcb);
   e_ilist_append(wd->o_ilist, icon, label, 0, _e_wid_cb_item_sel, _e_wid_cb_item_hilight, wd, wcb);
   if (icon) evas_object_show(icon);
}

EAPI void
e_widget_ilist_append_relative(Evas_Object *obj, Evas_Object *icon, const char *label, void (*func) (void *data), void *data, const char *val, int relative)
{
   E_Widget_Data *wd;
   E_Widget_Callback *wcb, *rcb;

   wcb = E_NEW(E_Widget_Callback, 1);
   if (!wcb) return;

   wd = e_widget_data_get(obj);
   wcb->func = func;
   wcb->data = data;
   if (val) wcb->value = strdup(val);

   rcb = evas_list_nth(wd->callbacks, relative);
   if (rcb) 
     {
	wd->callbacks = evas_list_append_relative(wd->callbacks, wcb, rcb);
	e_ilist_append_relative(wd->o_ilist, icon, label, 0, _e_wid_cb_item_sel, _e_wid_cb_item_hilight, wd, wcb, relative);
     }
   else 
     {
	wd->callbacks = evas_list_append(wd->callbacks, wcb);
	e_ilist_append(wd->o_ilist, icon, label, 0, _e_wid_cb_item_sel, _e_wid_cb_item_hilight, wd, wcb);
     }

   if (icon) evas_object_show(icon);
}

EAPI void
e_widget_ilist_prepend(Evas_Object *obj, Evas_Object *icon, const char *label, void (*func) (void *data), void *data, const char *val)
{
   E_Widget_Data *wd;
   E_Widget_Callback *wcb;

   wcb = E_NEW(E_Widget_Callback, 1);
   if (!wcb) return;

   wd = e_widget_data_get(obj);
   wcb->func = func;
   wcb->data = data;
   if (val) wcb->value = strdup(val);
   wd->callbacks = evas_list_prepend(wd->callbacks, wcb);
   e_ilist_prepend(wd->o_ilist, icon, label, 0, _e_wid_cb_item_sel, _e_wid_cb_item_hilight, wd, wcb);
   if (icon) evas_object_show(icon);
}

EAPI void
e_widget_ilist_prepend_relative(Evas_Object *obj, Evas_Object *icon, const char *label, void (*func) (void *data), void *data, const char *val, int relative)
{
   E_Widget_Data *wd;
   E_Widget_Callback *wcb, *rcb;

   wcb = E_NEW(E_Widget_Callback, 1);
   if (!wcb) return;

   wd = e_widget_data_get(obj);
   wcb->func = func;
   wcb->data = data;
   if (val) wcb->value = strdup(val);

   rcb = evas_list_nth(wd->callbacks, relative);
   if (rcb) 
     {
	wd->callbacks = evas_list_prepend_relative(wd->callbacks, wcb, rcb);
	e_ilist_prepend_relative(wd->o_ilist, icon, label, 0, _e_wid_cb_item_sel, _e_wid_cb_item_hilight, wd, wcb, relative);
     }
   else 
     {
	wd->callbacks = evas_list_prepend(wd->callbacks, wcb);
	e_ilist_prepend(wd->o_ilist, icon, label, 0, _e_wid_cb_item_sel, _e_wid_cb_item_hilight, wd, wcb);
     }

   if (icon) evas_object_show(icon);
}

EAPI void
e_widget_ilist_header_append(Evas_Object *obj, Evas_Object *icon, const char *label)
{
   E_Widget_Data *wd;
   E_Widget_Callback *wcb;

   wcb = E_NEW(E_Widget_Callback, 1);
   if (!wcb) return;

   wd = e_widget_data_get(obj);
   wd->callbacks = evas_list_append(wd->callbacks, wcb);
   e_ilist_append(wd->o_ilist, icon, label, 1, NULL, NULL, NULL, NULL);
   if (icon) evas_object_show(icon);
}

EAPI void
e_widget_ilist_selector_set(Evas_Object *obj, int selector)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   e_ilist_selector_set(wd->o_ilist, selector);
}

EAPI void
e_widget_ilist_go(Evas_Object *obj)
{
   E_Widget_Data *wd;
   Evas_Coord mw, mh, vw, vh, w, h;

   wd = e_widget_data_get(obj);
   wd->o_widget = obj;
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
   else if (mw < vw)
     evas_object_resize(wd->o_ilist, vw,mh);
}

EAPI void
e_widget_ilist_clear(Evas_Object *obj) 
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   e_ilist_clear(wd->o_ilist);
   e_scrollframe_child_pos_set(wd->o_scrollframe, 0, 0);
   while (wd->callbacks)
     {
	E_Widget_Callback *wcb;

	wcb = wd->callbacks->data;
	if (wcb->value) free(wcb->value);
	free(wcb);
	wd->callbacks = evas_list_remove_list(wd->callbacks, wd->callbacks);
     }
}

EAPI int
e_widget_ilist_count(Evas_Object *obj)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   return e_ilist_count(wd->o_ilist);
}

EAPI Evas_List *
e_widget_ilist_items_get(Evas_Object *obj) 
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   return e_ilist_items_get(wd->o_ilist);
}

EAPI int 
e_widget_ilist_nth_is_header(Evas_Object *obj, int n) 
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   return e_ilist_nth_is_header(wd->o_ilist, n);
}

EAPI void
e_widget_ilist_nth_label_set(Evas_Object *obj, int n, const char *label)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   e_ilist_nth_label_set(wd->o_ilist, n, label);
}

EAPI const char *
e_widget_ilist_nth_label_get(Evas_Object *obj, int n)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   return e_ilist_nth_label_get(wd->o_ilist, n);
}

EAPI void
e_widget_ilist_nth_icon_set(Evas_Object *obj, int n, Evas_Object *icon)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   e_ilist_nth_icon_set(wd->o_ilist, n, icon);
}

EAPI Evas_Object *
e_widget_ilist_nth_icon_get(Evas_Object *obj, int n)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   return e_ilist_nth_icon_get(wd->o_ilist, n);
}

EAPI void *
e_widget_ilist_nth_data_get(Evas_Object *obj, int n)
{
   E_Widget_Data *wd;
   E_Widget_Callback *wcb;

   wd = e_widget_data_get(obj);
   wcb = evas_list_nth(wd->callbacks, n);

   if (!wcb) 
     return NULL;
   else 
     return wcb->data;
}

/**
 * Show the nth element of an ilist
 * @param obj the ilist
 * @param n the number of the element to show
 * @param top if true, place this item at the top, otherwise scroll just 
 * enough to show the element (from the current position).
 */
EAPI void
e_widget_ilist_nth_show(Evas_Object *obj, int n, int top)
{
   E_Widget_Data *wd;
   Evas_Coord x, y, w, h;

   wd = e_widget_data_get(obj);
   e_ilist_nth_geometry_get(wd->o_ilist, n, &x, &y, &w, &h);
   if (top)
     e_scrollframe_child_pos_set(wd->o_scrollframe, x, y);
   else
     e_scrollframe_child_region_show(wd->o_scrollframe, x, y, w, h);
}

EAPI void
e_widget_ilist_selected_set(Evas_Object *obj, int n)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   e_ilist_selected_set(wd->o_ilist, n);
}

EAPI int
e_widget_ilist_selected_get(Evas_Object *obj)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   return e_ilist_selected_get(wd->o_ilist);
}

EAPI const char *
e_widget_ilist_selected_label_get(Evas_Object *obj)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   return e_ilist_selected_label_get(wd->o_ilist);
}

EAPI Evas_Object *
e_widget_ilist_selected_icon_get(Evas_Object *obj)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   return e_ilist_selected_icon_get(wd->o_ilist);
}

EAPI int 
e_widget_ilist_selected_count_get(Evas_Object *obj) 
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   return e_ilist_selected_count_get(wd->o_ilist);
}

EAPI void
e_widget_ilist_unselect(Evas_Object *obj)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   if ((wd->value) && *(wd->value))
       {
          free(*(wd->value));
          *(wd->value) = NULL;
       }
   e_ilist_unselect(wd->o_ilist);
}

EAPI void
e_widget_ilist_remove_num(Evas_Object *obj, int n)
{
   E_Widget_Data *wd;
   E_Widget_Callback *wcb;

   wd = e_widget_data_get(obj);
   e_ilist_remove_num(wd->o_ilist, n);
   if (!(wcb = evas_list_nth(wd->callbacks, n))) return;
   if (wcb->value) free(wcb->value);
   free(wcb);
   wd->callbacks = evas_list_remove(wd->callbacks, wcb);
}

EAPI void 
e_widget_ilist_multi_select_set(Evas_Object *obj, int multi) 
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   e_ilist_multi_select_set(wd->o_ilist, multi);
}

EAPI int 
e_widget_ilist_multi_select_get(Evas_Object *obj) 
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   return e_ilist_multi_select_get(wd->o_ilist);
}

EAPI void 
e_widget_ilist_multi_select(Evas_Object *obj, int n) 
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   e_ilist_multi_select(wd->o_ilist, n);
}

EAPI void 
e_widget_ilist_range_select(Evas_Object *obj, int n) 
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   e_ilist_range_select(wd->o_ilist, n);
}

static void
_e_wid_del_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   while (wd->callbacks)
     {
	E_Widget_Callback *wcb;

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
	edje_object_signal_emit(e_scrollframe_edje_object_get(wd->o_scrollframe), "e,state,focused", "e");
	evas_object_focus_set(wd->o_ilist, 1);
     }
   else
     {
	edje_object_signal_emit(e_scrollframe_edje_object_get(wd->o_scrollframe), "e,state,unfocused", "e");
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
   E_Widget_Callback *wcb;

   wd = data;
   wcb = data2;
   e_ilist_selected_geometry_get(wd->o_ilist, &x, &y, &w, &h);
   e_scrollframe_child_region_show(wd->o_scrollframe, x, y, w, h);
   if (wd->o_widget)
     {
	if (wd->value)
	  {
	     if (*(wd->value)) free(*(wd->value));
	     if (wcb->value)
	       *(wd->value) = strdup(wcb->value);
	     else
	       *(wd->value) = NULL;
	  }
	if (wcb->func) wcb->func(wcb->data);
	e_widget_change(wd->o_widget);
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
   e_ilist_selected_geometry_get(wd->o_ilist, &x, &y, &w, &h);
   e_scrollframe_child_region_show(wd->o_scrollframe, x, y, w, h);
}

static void
_e_wid_cb_selected(void *data, Evas_Object *obj, void *event_info)
{
   evas_object_smart_callback_call(data, "selected", event_info);
}

static void
_e_wid_focus_steal(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   e_widget_focus_steal(data);
}
