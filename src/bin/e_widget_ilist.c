/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

typedef struct _E_Widget_Queue_Item E_Widget_Queue_Item;

typedef struct _E_Widget_Data E_Widget_Data;
typedef struct _E_Widget_Callback E_Widget_Callback;
struct _E_Widget_Data
{
   Evas_Object *o_widget, *o_scrollframe, *o_ilist;
   Eina_List *callbacks;
   char **value;
   struct {
      Eina_List *queue;
      Ecore_Timer *timer;
      int count;

      int show_nth;
      int select_nth;
   } queue;
};
struct _E_Widget_Callback
{
   void (*func) (void *data);
   void  *data;
   char  *value;
};

struct _E_Widget_Queue_Item
{
   int command;
   Evas_Object *icon;
   const char *label;
   int header;
   void (*func) (void *data);
   void *data;
   const char *val;
   int relative;
   int use_relative;
   int item;
};

static void _e_wid_del_hook(Evas_Object *obj);
static void _e_wid_focus_hook(Evas_Object *obj);
static void _e_wid_cb_scrollframe_resize(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_wid_cb_item_sel(void *data, void *data2);
static void _e_wid_cb_item_hilight(void *data, void *data2);
static void _e_wid_cb_selected(void *data, Evas_Object *obj, void *event_info);
static void _e_wid_focus_steal(void *data, Evas *e, Evas_Object *obj, void *event_info);

static int _queue_timer(void *data);
static void _queue_queue(Evas_Object *obj);
static void _queue_append(Evas_Object *obj, int command, Evas_Object *icon, const char *label, int header, void (*func) (void *data), void *data, const char *val, int relative, int use_relative, int item);
static void _queue_remove(Evas_Object *obj, E_Widget_Queue_Item *qi, int del);

static int
_queue_timer(void *data)
{
   Evas_Object *obj;
   E_Widget_Data *wd;
   int num;

   obj = data;
   wd = e_widget_data_get(obj);
   wd->queue.timer = NULL;
   e_widget_ilist_freeze(obj);
   num = 0;
   while (wd->queue.queue)
     {
        E_Widget_Queue_Item *qi;

        qi = wd->queue.queue->data;
        if (qi->command == 0)
          {
             E_Widget_Callback *wcb, *rcb;
             wcb = E_NEW(E_Widget_Callback, 1);
             if (!wcb) break;
             wcb->func = qi->func;
             wcb->data = qi->data;
             if (qi->val) wcb->value = strdup(qi->val);
             if (qi->use_relative == 0) // append
	     {
               wd->callbacks = eina_list_append(wd->callbacks, wcb);
               e_ilist_append(wd->o_ilist, qi->icon, qi->label, qi->header,
                              _e_wid_cb_item_sel, _e_wid_cb_item_hilight, wd, wcb);
	     }
             else if (qi->use_relative == 2) // prepend
	     {
               wd->callbacks = eina_list_append(wd->callbacks, wcb);
               e_ilist_prepend(wd->o_ilist, qi->icon, qi->label, qi->header,
                               _e_wid_cb_item_sel, _e_wid_cb_item_hilight, wd, wcb);
	     }
             else if (qi->use_relative == 1) // append relative
               {
                  rcb = eina_list_nth(wd->callbacks, qi->relative);
                  if (rcb)
                    {
                       wd->callbacks = eina_list_append_relative(wd->callbacks, wcb, rcb);
                       e_ilist_append_relative(wd->o_ilist, qi->icon, qi->label, qi->header,
                                               _e_wid_cb_item_sel, _e_wid_cb_item_hilight, wd, wcb, qi->relative);
                    }
                  else
                    {
                       wd->callbacks = eina_list_append(wd->callbacks, wcb);
                       e_ilist_append(wd->o_ilist, qi->icon, qi->label, qi->header,
                                      _e_wid_cb_item_sel, _e_wid_cb_item_hilight, wd, wcb);
                    }
               }
             else if (qi->use_relative == 3) // prepend relative
               {
                  rcb = eina_list_nth(wd->callbacks, qi->relative);
                  if (rcb)
                    {
                       wd->callbacks = eina_list_prepend_relative(wd->callbacks, wcb, rcb);
                       e_ilist_prepend_relative(wd->o_ilist, qi->icon, qi->label, qi->header,
                                                _e_wid_cb_item_sel, _e_wid_cb_item_hilight, wd, wcb, qi->relative);
                    }
                  else
                    {
                       wd->callbacks = eina_list_prepend(wd->callbacks, wcb);
                       e_ilist_prepend(wd->o_ilist, qi->icon, qi->label, qi->header,
                                       _e_wid_cb_item_sel, _e_wid_cb_item_hilight, wd, wcb);
                    }
               }
             if (qi->icon) evas_object_show(qi->icon);
          }
        else if (qi->command == 1)
          {
             e_ilist_nth_label_set(wd->o_ilist, qi->item, qi->label);
          }
        else if (qi->command == 2)
          {
             e_ilist_nth_icon_set(wd->o_ilist, qi->item, qi->icon);
          }
        else if (qi->command == 3)
          {
             Evas_Coord x, y, w, h;

             e_ilist_nth_geometry_get(wd->o_ilist, qi->item, &x, &y, &w, &h);
             if (qi->use_relative)
               e_scrollframe_child_pos_set(wd->o_scrollframe, x, y);
             else
               e_scrollframe_child_region_show(wd->o_scrollframe, x, y, w, h);
          }
        else if (qi->command == 4)
          {
             e_ilist_selected_set(wd->o_ilist, qi->item);
          }
        else if (qi->command == 5)
          {
             if ((wd->value) && *(wd->value))
               {
                  free(*(wd->value));
                  *(wd->value) = NULL;
               }
             e_ilist_unselect(wd->o_ilist);
          }
        else if (qi->command == 6)
          {
             E_Widget_Callback *wcb;

             e_ilist_remove_num(wd->o_ilist, qi->item);
             if ((wcb = eina_list_nth(wd->callbacks, qi->item)))
               {
                  if (wcb->value) free(wcb->value);
                  free(wcb);
                  wd->callbacks = eina_list_remove(wd->callbacks, wcb);
               }
          }
        else if (qi->command == 7)
          {
             e_ilist_multi_select(wd->o_ilist, qi->item);
          }
        else if (qi->command == 8)
          {
             e_ilist_range_select(wd->o_ilist, qi->item);
          }
        _queue_remove(obj, qi, 0);
        num++;
        if (num >= 10) break;
     }
   e_widget_ilist_thaw(obj);
   e_widget_ilist_go(obj);
   _queue_queue(obj);
   return 0;
}

static void
_queue_queue(Evas_Object *obj)
{
   E_Widget_Data *wd;
   wd = e_widget_data_get(obj);
   if (!wd->queue.queue) return;
   if (wd->queue.timer) return;
   wd->queue.timer = ecore_timer_add(0.05, _queue_timer, obj);
}

static void
_queue_append(Evas_Object *obj,
              int command,
              Evas_Object *icon,
              const char *label,
              int header,
              void (*func) (void *data),
              void *data,
              const char *val,
              int relative,
              int use_relative,
              int item)
{
   E_Widget_Data *wd;
   E_Widget_Queue_Item *qi;

   wd = e_widget_data_get(obj);
   qi = E_NEW(E_Widget_Queue_Item, 1);
   if (!qi) return;
   qi->command = command;
   qi->icon = icon;
   qi->label = eina_stringshare_add(label);
   qi->header = header;
   qi->func = func;
   qi->data = data;
   qi->val = eina_stringshare_add(val);
   qi->relative = relative;
   qi->use_relative = use_relative;
   qi->item = item;
   wd->queue.queue = eina_list_append(wd->queue.queue, qi);
   _queue_queue(obj);
}

static void
_queue_remove(Evas_Object *obj, E_Widget_Queue_Item *qi, int del)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   wd->queue.queue = eina_list_remove(wd->queue.queue, qi);
   if (del)
     {
        if (qi->icon) evas_object_del(qi->icon);
     }
   eina_stringshare_del(qi->label);
   eina_stringshare_del(qi->val);
   free(qi);
}

static void
_queue_clear(Evas_Object *obj)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   while (wd->queue.queue)
     _queue_remove(obj, wd->queue.queue->data, 1);
   if (wd->queue.timer) ecore_timer_del(wd->queue.timer);
   wd->queue.timer = NULL;
}

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
   _queue_append(obj, 0, icon, label, 0, func, data, val, 0, 0, 0);
/*
   E_Widget_Data *wd;
   E_Widget_Callback *wcb;

   wcb = E_NEW(E_Widget_Callback, 1);
   if (!wcb) return;

   wd = e_widget_data_get(obj);
   wcb->func = func;
   wcb->data = data;
   if (val) wcb->value = strdup(val);
   wd->callbacks = eina_list_append(wd->callbacks, wcb);
   e_ilist_append(wd->o_ilist, icon, label, 0, _e_wid_cb_item_sel, _e_wid_cb_item_hilight, wd, wcb);
   if (icon) evas_object_show(icon);
 */
}

EAPI void
e_widget_ilist_append_relative(Evas_Object *obj, Evas_Object *icon, const char *label, void (*func) (void *data), void *data, const char *val, int relative)
{
   _queue_append(obj, 0, icon, label, 0, func, data, val, relative,1, 0);
/*
   E_Widget_Data *wd;
   E_Widget_Callback *wcb, *rcb;

   wcb = E_NEW(E_Widget_Callback, 1);
   if (!wcb) return;

   wd = e_widget_data_get(obj);
   wcb->func = func;
   wcb->data = data;
   if (val) wcb->value = strdup(val);

   rcb = eina_list_nth(wd->callbacks, relative);
   if (rcb)
     {
	wd->callbacks = eina_list_append_relative(wd->callbacks, wcb, rcb);
	e_ilist_append_relative(wd->o_ilist, icon, label, 0, _e_wid_cb_item_sel, _e_wid_cb_item_hilight, wd, wcb, relative);
     }
   else
     {
	wd->callbacks = eina_list_append(wd->callbacks, wcb);
	e_ilist_append(wd->o_ilist, icon, label, 0, _e_wid_cb_item_sel, _e_wid_cb_item_hilight, wd, wcb);
     }

   if (icon) evas_object_show(icon);
 */
}

EAPI void
e_widget_ilist_prepend(Evas_Object *obj, Evas_Object *icon, const char *label, void (*func) (void *data), void *data, const char *val)
{
   _queue_append(obj, 0, icon, label, 0, func, data, val, 0, 2, 0);
/*
   E_Widget_Data *wd;
   E_Widget_Callback *wcb;

   wcb = E_NEW(E_Widget_Callback, 1);
   if (!wcb) return;

   wd = e_widget_data_get(obj);
   wcb->func = func;
   wcb->data = data;
   if (val) wcb->value = strdup(val);
   wd->callbacks = eina_list_prepend(wd->callbacks, wcb);
   e_ilist_prepend(wd->o_ilist, icon, label, 0, _e_wid_cb_item_sel, _e_wid_cb_item_hilight, wd, wcb);
   if (icon) evas_object_show(icon);
 */
}

EAPI void
e_widget_ilist_prepend_relative(Evas_Object *obj, Evas_Object *icon, const char *label, void (*func) (void *data), void *data, const char *val, int relative)
{
   _queue_append(obj, 0, icon, label, 0, func, data, val, relative, 3, 0);
/*
   E_Widget_Data *wd;
   E_Widget_Callback *wcb, *rcb;

   wcb = E_NEW(E_Widget_Callback, 1);
   if (!wcb) return;

   wd = e_widget_data_get(obj);
   wcb->func = func;
   wcb->data = data;
   if (val) wcb->value = strdup(val);

   rcb = eina_list_nth(wd->callbacks, relative);
   if (rcb)
     {
	wd->callbacks = eina_list_prepend_relative(wd->callbacks, wcb, rcb);
	e_ilist_prepend_relative(wd->o_ilist, icon, label, 0, _e_wid_cb_item_sel, _e_wid_cb_item_hilight, wd, wcb, relative);
     }
   else
     {
	wd->callbacks = eina_list_prepend(wd->callbacks, wcb);
	e_ilist_prepend(wd->o_ilist, icon, label, 0, _e_wid_cb_item_sel, _e_wid_cb_item_hilight, wd, wcb);
     }

   if (icon) evas_object_show(icon);
 */
}

EAPI void
e_widget_ilist_header_append(Evas_Object *obj, Evas_Object *icon, const char *label)
{
   _queue_append(obj, 0, icon, label, 1, NULL, NULL, NULL, 0, 0, 0);
/*
   E_Widget_Data *wd;
   E_Widget_Callback *wcb;

   wcb = E_NEW(E_Widget_Callback, 1);
   if (!wcb) return;

   wd = e_widget_data_get(obj);
   wd->callbacks = eina_list_append(wd->callbacks, wcb);
   e_ilist_append(wd->o_ilist, icon, label, 1, NULL, NULL, NULL, NULL);
   if (icon) evas_object_show(icon);
 */
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
   _queue_clear(obj);
   e_ilist_clear(wd->o_ilist);
   e_scrollframe_child_pos_set(wd->o_scrollframe, 0, 0);
   while (wd->callbacks)
     {
	E_Widget_Callback *wcb;

	wcb = wd->callbacks->data;
	if (wcb->value) free(wcb->value);
	free(wcb);
	wd->callbacks = eina_list_remove_list(wd->callbacks, wd->callbacks);
     }
}

EAPI int
e_widget_ilist_count(Evas_Object *obj)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   return e_ilist_count(wd->o_ilist);
}

EAPI Eina_List *
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
   _queue_append(obj, 1, NULL, label, 0, NULL, NULL, NULL, 0, 0, n);
/*
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   e_ilist_nth_label_set(wd->o_ilist, n, label);
 */
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
   _queue_append(obj, 2, icon, NULL, 0, NULL, NULL, NULL, 0, 0, n);
/*
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   e_ilist_nth_icon_set(wd->o_ilist, n, icon);
 */
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
   wcb = eina_list_nth(wd->callbacks, n);

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
   _queue_append(obj, 3, NULL, NULL, 0, NULL, NULL, NULL, 0, top, n);
/*
   E_Widget_Data *wd;
   Evas_Coord x, y, w, h;

   wd = e_widget_data_get(obj);
   e_ilist_nth_geometry_get(wd->o_ilist, n, &x, &y, &w, &h);
   if (top)
     e_scrollframe_child_pos_set(wd->o_scrollframe, x, y);
   else
     e_scrollframe_child_region_show(wd->o_scrollframe, x, y, w, h);
 */
}

EAPI void
e_widget_ilist_selected_set(Evas_Object *obj, int n)
{
   _queue_append(obj, 4, NULL, NULL, 0, NULL, NULL, NULL, 0, 0, n);
/*
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   e_ilist_selected_set(wd->o_ilist, n);
 */
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
   _queue_append(obj, 5, NULL, NULL, 0, NULL, NULL, NULL, 0, 0, 0);
/*
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   if ((wd->value) && *(wd->value))
       {
          free(*(wd->value));
          *(wd->value) = NULL;
       }
   e_ilist_unselect(wd->o_ilist);
 */
}

EAPI void
e_widget_ilist_remove_num(Evas_Object *obj, int n)
{
/*    _queue_append(obj, 6, NULL, NULL, 0, NULL, NULL, NULL, 0, 0, n); */
   E_Widget_Callback *wcb;
   E_Widget_Data *wd;
   Eina_List *item;

   wd = e_widget_data_get(obj);
   e_ilist_remove_num(wd->o_ilist, n);
   item = eina_list_nth_list(wd->callbacks, n);
   if (item)
     {
	wcb = eina_list_data_get(item);
	if (wcb && wcb->value) free(wcb->value);
	free(wcb);
	wd->callbacks = eina_list_remove_list(wd->callbacks, item);
     }
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
   _queue_append(obj, 7, NULL, NULL, 0, NULL, NULL, NULL, 0, 0, n);
/*
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   e_ilist_multi_select(wd->o_ilist, n);
 */
}

EAPI void
e_widget_ilist_range_select(Evas_Object *obj, int n)
{
   _queue_append(obj, 8, NULL, NULL, 0, NULL, NULL, NULL, 0, 0, n);
/*
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   e_ilist_range_select(wd->o_ilist, n);
 */
}

EAPI void
e_widget_ilist_preferred_size_get(Evas_Object *obj, Evas_Coord *w, Evas_Coord *h)
{
   Evas_Coord ww, hh, mw, mh, vw, vh;
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   evas_object_geometry_get(wd->o_scrollframe, NULL, NULL, &ww, &hh);
   evas_object_resize(wd->o_scrollframe, 200, 200);
   e_scrollframe_child_viewport_size_get(wd->o_scrollframe, &vw, &vh);
   e_ilist_min_size_get(wd->o_ilist, &mw, &mh);
   evas_object_resize(wd->o_scrollframe, ww, hh);
   if (w) *w = 200 - vw + mw;
   if (h) *h = 200 - vh + mh;
}

static void
_e_wid_del_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   _queue_clear(obj);
   while (wd->callbacks)
     {
	E_Widget_Callback *wcb;

	wcb = wd->callbacks->data;
	if (wcb->value) free(wcb->value);
	free(wcb);
	wd->callbacks = eina_list_remove_list(wd->callbacks, wd->callbacks);
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
