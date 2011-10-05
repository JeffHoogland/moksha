#include "e.h"

typedef struct _E_Widget_Queue_Item E_Widget_Queue_Item;
typedef struct _E_Widget_Data E_Widget_Data;
typedef struct _E_Widget_Callback E_Widget_Callback;

struct _E_Widget_Data
{
   Evas_Object *o_widget, *o_scrollframe, *o_ilist;
   Eina_List *callbacks;
   char **value;
   struct 
     {
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
   void *data;
   char *value;
};

struct _E_Widget_Queue_Item
{
   int command;
   Evas_Object *icon;
   Evas_Object *end;
   const char *label;
   int header;
   void (*func) (void *data);
   void *data;
   const char *val;
   int relative, use_relative;
   int item;
};

static void _e_wid_del_hook(Evas_Object *obj);
static void _e_wid_focus_hook(Evas_Object *obj);
static void _e_wid_cb_scrollframe_resize(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_wid_cb_item_sel(void *data, void *data2);
static void _e_wid_cb_item_hilight(void *data, void *data2);
static void _e_wid_cb_selected(void *data, Evas_Object *obj, void *event_info);
static void _e_wid_focus_steal(void *data, Evas *e, Evas_Object *obj, void *event_info);

static Eina_Bool _queue_timer(void *data);
static void _queue_queue(Evas_Object *obj);
static void _queue_append(Evas_Object *obj, int command, Evas_Object *icon, Evas_Object *end, const char *label, int header, void (*func) (void *data), void *data, const char *val, int relative, int use_relative, int item);
static void _queue_remove(Evas_Object *obj, E_Widget_Queue_Item *qi, int del);

enum
{
  CMD_ADD,
  CMD_REMOVE,
  CMD_APPEND,
  CMD_PREPEND,
  CMD_APPEND_RELATIVE,
  CMD_PREPEND_RELATIVE,
  CMD_SELECT,
  CMD_UNSELECT,
  CMD_RANGE_SELECT,
  CMD_MULTI_SELECT,
  CMD_LABEL_SET,
  CMD_ICON_SET,
  CMD_END_SET,
  CMD_SHOW
};

static Eina_Bool
_queue_timer(void *data)
{
   Evas_Object *obj;
   E_Widget_Data *wd;
   int num;
   double start = ecore_time_get();

   obj = data;
   wd = e_widget_data_get(obj);
   wd->queue.timer = NULL;
   e_widget_ilist_freeze(obj);
   num = 0;
   while (wd->queue.queue)
     {
        E_Widget_Queue_Item *qi;

        qi = eina_list_data_get(wd->queue.queue);
        if (qi->command == CMD_ADD)
          {
             E_Widget_Callback *wcb, *rcb;

             wcb = E_NEW(E_Widget_Callback, 1);
             if (!wcb) break;
             wcb->func = qi->func;
             wcb->data = qi->data;
             if (qi->val) wcb->value = strdup(qi->val);
             if (qi->use_relative == CMD_APPEND)
               {
                  wd->callbacks = eina_list_append(wd->callbacks, wcb);
                  e_ilist_append(wd->o_ilist, qi->icon, qi->end, qi->label, qi->header,
                                 _e_wid_cb_item_sel, _e_wid_cb_item_hilight, wd, wcb);
               }
             else if (qi->use_relative == CMD_PREPEND)
               {
                  wd->callbacks = eina_list_append(wd->callbacks, wcb);
                  e_ilist_prepend(wd->o_ilist, qi->icon, qi->end, qi->label, qi->header,
                                  _e_wid_cb_item_sel, _e_wid_cb_item_hilight, wd, wcb);
               }
             else if (qi->use_relative == CMD_APPEND_RELATIVE)
               {
                  rcb = eina_list_nth(wd->callbacks, qi->relative);
                  if (rcb)
                    {
                       wd->callbacks = eina_list_append_relative(wd->callbacks, wcb, rcb);
                       e_ilist_append_relative(wd->o_ilist, qi->icon, qi->end, qi->label, qi->header,
                                               _e_wid_cb_item_sel, _e_wid_cb_item_hilight, wd, wcb, qi->relative);
                    }
                  else
                    {
                       wd->callbacks = eina_list_append(wd->callbacks, wcb);
                       e_ilist_append(wd->o_ilist, qi->icon, qi->end, qi->label, qi->header,
                                      _e_wid_cb_item_sel, _e_wid_cb_item_hilight, wd, wcb);
                    }
               }
             else if (qi->use_relative == CMD_PREPEND_RELATIVE)
               {
                  rcb = eina_list_nth(wd->callbacks, qi->relative);
                  if (rcb)
                    {
                       wd->callbacks = eina_list_prepend_relative(wd->callbacks, wcb, rcb);
                       e_ilist_prepend_relative(wd->o_ilist, qi->icon, qi->end, qi->label, qi->header,
                                                _e_wid_cb_item_sel, _e_wid_cb_item_hilight, wd, wcb, qi->relative);
                    }
                  else
                    {
                       wd->callbacks = eina_list_prepend(wd->callbacks, wcb);
                       e_ilist_prepend(wd->o_ilist, qi->icon, qi->end, qi->label, qi->header,
                                       _e_wid_cb_item_sel, _e_wid_cb_item_hilight, wd, wcb);
                    }
               }
             if (qi->icon) evas_object_show(qi->icon);
	     if (qi->end) evas_object_show(qi->end);
          }
        else if (qi->command ==CMD_LABEL_SET)
          e_ilist_nth_label_set(wd->o_ilist, qi->item, qi->label);
        else if (qi->command == CMD_ICON_SET)
          e_ilist_nth_icon_set(wd->o_ilist, qi->item, qi->icon);
        else if (qi->command == CMD_SHOW)
          {
             Evas_Coord x, y, w, h;
	     if (num > 0) break;
	     
             e_ilist_nth_geometry_get(wd->o_ilist, qi->item, &x, &y, &w, &h);
             if (qi->use_relative)
               e_scrollframe_child_pos_set(wd->o_scrollframe, x, y);
             else
               e_scrollframe_child_region_show(wd->o_scrollframe, x, y, w, h);
          }
        else if (qi->command == CMD_SELECT)
          e_ilist_selected_set(wd->o_ilist, qi->item);
        else if (qi->command == CMD_UNSELECT)
          {
             if ((wd->value) && *(wd->value))
               {
                  eina_stringshare_del(*(wd->value));
                  *(wd->value) = NULL;
               }
             e_ilist_unselect(wd->o_ilist);
          }
#if 0
        else if (qi->command == CMD_REMOVE)
          {
             E_Widget_Callback *wcb;
	     Eina_List *item;

             e_ilist_remove_num(wd->o_ilist, qi->item);
	     item = eina_list_nth_list(wd->callbacks, qi->item);
	     if (item)
	       {
		  wcb = eina_list_data_get(item);
		  if (wcb && wcb->value) free(wcb->value);
                  free(wcb);
                  wd->callbacks = eina_list_remove_list(wd->callbacks, item);
               }
          }
#endif	
        else if (qi->command == CMD_MULTI_SELECT)
          e_ilist_multi_select(wd->o_ilist, qi->item);
        else if (qi->command == CMD_RANGE_SELECT)
          e_ilist_range_select(wd->o_ilist, qi->item);
	else if (qi->command == CMD_END_SET)
	  e_ilist_nth_end_set(wd->o_ilist, qi->item, qi->end);
        _queue_remove(obj, qi, 0);

        if ((num++ >= 10) && (ecore_time_get() - start > 0.01))
	  break;
     }
   e_widget_ilist_thaw(obj);
   e_widget_ilist_go(obj);
   _queue_queue(obj);
   return ECORE_CALLBACK_CANCEL;
}

static void
_queue_queue(Evas_Object *obj)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   if (!wd->queue.queue) return;
   if (wd->queue.timer) return;
   wd->queue.timer = ecore_timer_add(0.00001, _queue_timer, obj);
}

static void
_queue_append(Evas_Object *obj, int command, Evas_Object *icon, Evas_Object *end,
              const char *label, int header, void (*func) (void *data), void *data,
              const char *val, int relative, int use_relative, int item)
{
   E_Widget_Data *wd;
   E_Widget_Queue_Item *qi;

   wd = e_widget_data_get(obj);
   qi = E_NEW(E_Widget_Queue_Item, 1);
   if (!qi) return;
   qi->command = command;
   qi->icon = icon;
   qi->end = end;
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
	if (qi->end) evas_object_del(qi->end);
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
     _queue_remove(obj, eina_list_data_get(wd->queue.queue), 1);
   if (wd->queue.timer) ecore_timer_del(wd->queue.timer);
   wd->queue.timer = NULL;
}

/* externally accessible functions */
EAPI Evas_Object *
e_widget_ilist_add(Evas *evas, int icon_w, int icon_h, const char **value)
{
   Evas_Object *obj, *o;
   E_Widget_Data *wd;

   wd = E_NEW(E_Widget_Data, 1);
   if (!wd) return NULL;

   obj = e_widget_add(evas);

   e_widget_del_hook_set(obj, _e_wid_del_hook);
   e_widget_focus_hook_set(obj, _e_wid_focus_hook);
   e_widget_data_set(obj, wd);

   wd->value = (char **)value;

   o = e_scrollframe_add(evas);
   wd->o_scrollframe = o;
   evas_object_show(o);
   e_widget_sub_object_add(obj, o);
   e_widget_resize_object_set(obj, o);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, 
                                  _e_wid_focus_steal, obj);

   o = e_ilist_add(evas);
   wd->o_ilist = o;
   e_ilist_icon_size_set(o, icon_w, icon_h);
   evas_object_event_callback_add(wd->o_scrollframe, EVAS_CALLBACK_RESIZE, 
                                  _e_wid_cb_scrollframe_resize, o);
   e_scrollframe_child_set(wd->o_scrollframe, o);
   e_widget_sub_object_add(obj, o);
   evas_object_show(o);
   evas_object_smart_callback_add(o, "selected", _e_wid_cb_selected, obj);

   evas_object_resize(obj, 32, 32);
   e_widget_size_min_set(obj, 32, 32);
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
   _queue_append(obj, CMD_ADD, icon, NULL, label, 0, func, data, val, 0, CMD_APPEND, 0);
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
e_widget_ilist_append_full(Evas_Object *obj, Evas_Object *icon, Evas_Object *end, const char *label, void (*func) (void *data), void *data, const char *val)
{
   _queue_append(obj, CMD_ADD, icon, end, label, 0, func, data, val, 0, CMD_APPEND, 0);
}

EAPI void
e_widget_ilist_append_relative(Evas_Object *obj, Evas_Object *icon, const char *label, void (*func) (void *data), void *data, const char *val, int relative)
{
   _queue_append(obj, CMD_ADD, icon, NULL, label, 0, func, data, val, relative, CMD_APPEND_RELATIVE, 0);
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
e_widget_ilist_append_relative_full(Evas_Object *obj, Evas_Object *icon, Evas_Object *end, const char *label, void (*func) (void *data), void *data, const char *val, int relative)
{
   _queue_append(obj, CMD_ADD, icon, end, label, 0, func, data, val, relative, CMD_APPEND_RELATIVE, 0);
}

EAPI void
e_widget_ilist_prepend(Evas_Object *obj, Evas_Object *icon, const char *label, void (*func) (void *data), void *data, const char *val)
{
   _queue_append(obj, CMD_ADD, icon, NULL, label, 0, func, data, val, 0, CMD_PREPEND, 0);
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
e_widget_ilist_prepend_full(Evas_Object *obj, Evas_Object *icon, Evas_Object *end, const char *label, void (*func) (void *data), void *data, const char *val)
{
   _queue_append(obj, CMD_ADD, icon, end, label, 0, func, data, val, 0, CMD_PREPEND, 0);
}

EAPI void
e_widget_ilist_prepend_relative(Evas_Object *obj, Evas_Object *icon, const char *label, void (*func) (void *data), void *data, const char *val, int relative)
{
   _queue_append(obj, CMD_ADD, icon, NULL, label, 0, func, data, val, relative, CMD_PREPEND_RELATIVE, 0);
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
e_widget_ilist_prepend_relative_full(Evas_Object *obj, Evas_Object *icon, Evas_Object *end, const char *label, void (*func) (void *data), void *data, const char *val, int relative)
{
   _queue_append(obj, CMD_ADD, icon, end, label, 0, func, data, val, relative, CMD_PREPEND_RELATIVE, 0);
}

EAPI void
e_widget_ilist_header_append(Evas_Object *obj, Evas_Object *icon, const char *label)
{
   _queue_append(obj, CMD_ADD, icon, NULL, label, 1, NULL, NULL, NULL, 0, CMD_APPEND, 0);
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
   e_ilist_size_min_get(wd->o_ilist, &mw, &mh);
   evas_object_resize(wd->o_ilist, mw, mh);
   e_scrollframe_child_viewport_size_get(wd->o_scrollframe, &vw, &vh);
   evas_object_geometry_get(wd->o_scrollframe, NULL, NULL, &w, &h);
   if (mw > vw)
     {
	Evas_Coord wmw, wmh;

	e_widget_size_min_get(obj, &wmw, &wmh);
	e_widget_size_min_set(obj, mw + (w - vw), wmh);
     }
   else if (mw < vw)
     evas_object_resize(wd->o_ilist, vw,mh);
}

EAPI void
e_widget_ilist_clear(Evas_Object *obj)
{
   E_Widget_Data *wd;
   E_Widget_Callback *wcb;

   wd = e_widget_data_get(obj);
   _queue_clear(obj);
   e_ilist_clear(wd->o_ilist);
   e_scrollframe_child_pos_set(wd->o_scrollframe, 0, 0);
   EINA_LIST_FREE(wd->callbacks, wcb)
     {
	if (wcb->value) free(wcb->value);
	free(wcb);
     }
}

EAPI int
e_widget_ilist_count(Evas_Object *obj)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);

   if (wd->queue.queue)
     {
	E_Widget_Queue_Item *qi;
	Eina_List *l;
	int cnt = 0;

	EINA_LIST_FOREACH(wd->queue.queue, l, qi)
	  if (qi->command == CMD_ADD) cnt++;

	return (cnt + e_ilist_count(wd->o_ilist));
     }
   else
     return e_ilist_count(wd->o_ilist);
}

EAPI Eina_List *
e_widget_ilist_items_get(Evas_Object *obj)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   return e_ilist_items_get(wd->o_ilist);
}

EAPI Eina_Bool
e_widget_ilist_nth_is_header(Evas_Object *obj, int n)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   return e_ilist_nth_is_header(wd->o_ilist, n);
}

EAPI void
e_widget_ilist_nth_label_set(Evas_Object *obj, int n, const char *label)
{
   _queue_append(obj, CMD_LABEL_SET, NULL, NULL, label, 0, NULL, NULL, NULL, 0, 0, n);
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
   _queue_append(obj, CMD_ICON_SET, icon, NULL, NULL, 0, NULL, NULL, NULL, 0, 0, n);
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

EAPI void
e_widget_ilist_nth_end_set(Evas_Object *obj, int n, Evas_Object *end)
{
   _queue_append(obj, CMD_END_SET, NULL, end, NULL, 0, NULL, NULL, NULL, 0, 0, n);
}

EAPI Evas_Object *
e_widget_ilist_nth_end_get(Evas_Object *obj, int n)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   return e_ilist_nth_end_get(wd->o_ilist, n);
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

EAPI const char *
e_widget_ilist_nth_value_get(Evas_Object *obj, int n)
{
   E_Widget_Data *wd;
   E_Widget_Callback *wcb;

   wd = e_widget_data_get(obj);
   wcb = eina_list_nth(wd->callbacks, n);

   if (!wcb)
     return NULL;
   else
     return wcb->value;
}

/**
 * Return if the given item returned by e_widget_ilist_items_get()
 * is a header.
 *
 * This avoid expensive lookups to the nth element, however it's not
 * able to check any validity on the given pointer and may crash. Be
 * sure to use only with valid return of e_widget_ilist_items_get().
 */
EAPI Eina_Bool
e_widget_ilist_item_is_header(const E_Ilist_Item *it)
{
   return it->header;
}

/**
 * Return the label of given item returned by e_widget_ilist_items_get().
 *
 * This avoid expensive lookups to the nth element, however it's not
 * able to check any validity on the given pointer and may crash. Be
 * sure to use only with valid return of e_widget_ilist_items_get().
 */
EAPI const char *
e_widget_ilist_item_label_get(const E_Ilist_Item *it)
{
   return it->label;
}

/**
 * Return the icon of given item returned by e_widget_ilist_items_get().
 *
 * This avoid expensive lookups to the nth element, however it's not
 * able to check any validity on the given pointer and may crash. Be
 * sure to use only with valid return of e_widget_ilist_items_get().
 *
 * Do not delete this object!
 */
EAPI Evas_Object *
e_widget_ilist_item_icon_get(const E_Ilist_Item *it)
{
   return it->o_icon;
}

/**
 * Return the end of given item returned by e_widget_ilist_items_get().
 *
 * This avoid expensive lookups to the nth element, however it's not
 * able to check any validity on the given pointer and may crash. Be
 * sure to use only with valid return of e_widget_ilist_items_get().
 *
 * Do not delete this object!
 */
EAPI Evas_Object *
e_widget_ilist_item_end_get(const E_Ilist_Item *it)
{
   return it->o_end;
}

/**
 * Return the data of given item returned by e_widget_ilist_items_get().
 *
 * This avoid expensive lookups to the nth element, however it's not
 * able to check any validity on the given pointer and may crash. Be
 * sure to use only with valid return of e_widget_ilist_items_get().
 *
 * Do not delete this object!
 */
EAPI void *
e_widget_ilist_item_data_get(const E_Ilist_Item *it)
{
   E_Widget_Callback *wcb = it->data2;

   if (!wcb)
     return NULL;
   else
     return wcb->data;
}

EAPI const char *
e_widget_ilist_item_value_get(const E_Ilist_Item *it) 
{
   E_Widget_Callback *wcb = it->data2;

   if (!wcb)
     return NULL;
   else
     return wcb->value;
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
   _queue_append(obj, CMD_SHOW, NULL, NULL, NULL, 0, NULL, NULL, NULL, 0, top, n);
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
   _queue_append(obj, CMD_SELECT, NULL, NULL, NULL, 0, NULL, NULL, NULL, 0, 0, n);
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

EAPI void *
e_widget_ilist_selected_data_get(Evas_Object *obj)
{
   E_Widget_Data *wd;
   E_Widget_Callback *wcb;

   wd = e_widget_data_get(obj);
   wcb = eina_list_nth(wd->callbacks, e_ilist_selected_get(wd->o_ilist));

   return wcb ? wcb->data : NULL;
}

EAPI Evas_Object *
e_widget_ilist_selected_end_get(Evas_Object *obj)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   return e_ilist_selected_end_get(wd->o_ilist);
}

EAPI int
e_widget_ilist_selected_count_get(Evas_Object *obj)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   return e_ilist_selected_count_get(wd->o_ilist);
}

EAPI const char *
e_widget_ilist_selected_value_get(Evas_Object *obj) 
{
   E_Widget_Data *wd;
   E_Widget_Callback *wcb;

   wd = e_widget_data_get(obj);
   wcb = eina_list_nth(wd->callbacks, e_ilist_selected_get(wd->o_ilist));

   if (!wcb)
     return NULL;
   else
     return wcb->value;
}

EAPI void
e_widget_ilist_unselect(Evas_Object *obj)
{
   _queue_append(obj, CMD_UNSELECT, NULL, NULL, NULL, 0, NULL, NULL, NULL, 0, 0, 0);
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
/*    _queue_append(obj, CMD_REMOVE, NULL, NULL, 0, NULL, NULL, NULL, 0, 0, n); */
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
e_widget_ilist_multi_select_set(Evas_Object *obj, Eina_Bool multi)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   e_ilist_multi_select_set(wd->o_ilist, multi);
}

EAPI Eina_Bool 
e_widget_ilist_multi_select_get(Evas_Object *obj)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   return e_ilist_multi_select_get(wd->o_ilist);
}

EAPI void
e_widget_ilist_multi_select(Evas_Object *obj, int n)
{
   _queue_append(obj, CMD_MULTI_SELECT, NULL, NULL, NULL, 0, NULL, NULL, NULL, 0, 0, n);
/*
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   e_ilist_multi_select(wd->o_ilist, n);
 */
}

EAPI void
e_widget_ilist_range_select(Evas_Object *obj, int n)
{
   _queue_append(obj, CMD_RANGE_SELECT, NULL, NULL, NULL, 0, NULL, NULL, NULL, 0, 0, n);
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
   e_ilist_size_min_get(wd->o_ilist, &mw, &mh);
   evas_object_resize(wd->o_scrollframe, ww, hh);
   if (w) *w = 200 - vw + mw;
   if (h) *h = 200 - vh + mh;
}

static void
_e_wid_del_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;
   E_Widget_Callback *wcb;

   wd = e_widget_data_get(obj);
   _queue_clear(obj);
   EINA_LIST_FREE(wd->callbacks, wcb)
     {
	if (wcb->value) free(wcb->value);
	free(wcb);
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
_e_wid_cb_scrollframe_resize(void *data, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   Evas_Coord mw, mh, vw, vh, w, h;

   e_scrollframe_child_viewport_size_get(obj, &vw, &vh);
   e_ilist_size_min_get(data, &mw, &mh);
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
	     if (*(wd->value)) eina_stringshare_del(*(wd->value));
	     if (wcb->value)
	       *(wd->value) = (char*) eina_stringshare_add(wcb->value);
	     else
	       *(wd->value) = NULL;
	  }
	if (wcb->func) wcb->func(wcb->data);
	e_widget_change(wd->o_widget);
     }
}

static void
_e_wid_cb_item_hilight(void *data, void *data2 __UNUSED__)
{
   E_Widget_Data *wd;
   Evas_Coord x, y, w, h;

   wd = data;
   e_ilist_selected_geometry_get(wd->o_ilist, &x, &y, &w, &h);
   e_scrollframe_child_region_show(wd->o_scrollframe, x, y, w, h);
}

static void
_e_wid_cb_selected(void *data, Evas_Object *obj __UNUSED__, void *event_info)
{
   evas_object_smart_callback_call(data, "selected", event_info);
}

static void
_e_wid_focus_steal(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   e_widget_focus_steal(data);
}
