#include "e.h"

typedef struct _E_Widget_Data E_Widget_Data;
typedef struct _Item Item;
struct _E_Widget_Data
{
   Evas_Object *o_base, *o_box;
   int icon_w, icon_h;
   Eina_List *items;
   Eina_Bool scrollable : 1;
   Eina_Bool focus_steal : 1;
};

struct _Item
{
   Evas_Object *o_toolbar, *o_base, *o_icon;
   void (*func) (void *data1, void *data2);
   const void *data1, *data2;
   Eina_Bool selected : 1;
};

static void _e_wid_del_hook(Evas_Object *obj);
static void _e_wid_focus_hook(Evas_Object *obj);
static void _e_wid_focus_steal(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_wid_disable_hook(Evas_Object *obj);
static void _e_wid_signal_cb1(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_wid_cb_scrollframe_resize(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_wid_cb_key_down(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void _e_wid_signal_prev(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_wid_signal_next(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _item_show(Item *it);
static void _item_select(Item *it);
static void _item_unselect(Item *it);

/* local subsystem functions */

/* externally accessible functions */
EAPI Evas_Object *
e_widget_toolbar_add(Evas *evas, int icon_w, int icon_h)
{
   Evas_Object *obj, *o;
   E_Widget_Data *wd;
   Evas_Coord mw = 0, mh = 0;

   obj = e_widget_add(evas);

   e_widget_del_hook_set(obj, _e_wid_del_hook);
   e_widget_focus_hook_set(obj, _e_wid_focus_hook);
   e_widget_disable_hook_set(obj, _e_wid_disable_hook);
   wd = E_NEW(E_Widget_Data, 1);
   e_widget_data_set(obj, wd);
   wd->icon_w = icon_w;
   wd->icon_h = icon_h;
   wd->focus_steal = EINA_TRUE;

   o = e_scrollframe_add(evas);
   wd->o_base = o;
   o = e_box_add(evas);
   wd->o_box = o;
   o = wd->o_base;
   e_scrollframe_custom_theme_set(o, "base/theme/widgets", "e/widgets/toolbar");
   e_scrollframe_single_dir_set(o, 1);
   e_scrollframe_policy_set(o, E_SCROLLFRAME_POLICY_AUTO,
                            E_SCROLLFRAME_POLICY_OFF);
   e_scrollframe_thumbscroll_force(o, 1);
   evas_object_event_callback_add(o, EVAS_CALLBACK_RESIZE,
                                  _e_wid_cb_scrollframe_resize, obj);
   evas_object_event_callback_add(e_scrollframe_edje_object_get(wd->o_base),
                                  EVAS_CALLBACK_MOUSE_DOWN,
                                  _e_wid_focus_steal, obj);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_KEY_DOWN,
                                  _e_wid_cb_key_down, obj);

   edje_object_signal_callback_add(e_scrollframe_edje_object_get(o),
                                   "e,action,prev", "e",
                                   _e_wid_signal_prev, obj);
   edje_object_signal_callback_add(e_scrollframe_edje_object_get(o),
                                   "e,action,next", "e",
                                   _e_wid_signal_next, obj);
   evas_object_show(o);
   e_widget_sub_object_add(obj, o);
   e_widget_resize_object_set(obj, o);

   o = wd->o_box;
   e_box_orientation_set(o, 1);
   e_box_homogenous_set(o, 1);
   e_scrollframe_child_set(wd->o_base, o);
   e_widget_sub_object_add(obj, o);
   evas_object_show(o);

   edje_object_size_min_calc(e_scrollframe_edje_object_get(wd->o_base),
                             &mw, &mh);
   e_widget_size_min_set(obj, mw, mh);

   return obj;
}

EAPI const Eina_List *
e_widget_toolbar_items_get(Evas_Object *obj)
{
   E_Widget_Data *wd;
   if (!obj) return NULL;
   if (!(wd = e_widget_data_get(obj))) return NULL;
   return wd->items;
}

EAPI unsigned int
e_widget_toolbar_items_count(Evas_Object *obj)
{
   E_Widget_Data *wd;
   if (!obj) return 0;
   if (!(wd = e_widget_data_get(obj))) return 0;
   return eina_list_count(wd->items);
}

EAPI const char *
e_widget_toolbar_item_label_get(void *item)
{
   Item *it = item;
   if (!it) return NULL;
   return edje_object_part_text_get(it->o_base, "e.text.label");
}

EAPI void
e_widget_toolbar_item_append(Evas_Object *obj, Evas_Object *icon, const char *label, void (*func) (void *data1, void *data2), const void *data1, const void *data2)
{
   E_Widget_Data *wd;
   Evas_Object *o;
   Item *it;
   Evas_Coord mw = 0, mh = 0, vw = 0, vh = 0;

   if (!obj) return;
   if (!(wd = e_widget_data_get(obj))) return;
   o = edje_object_add(evas_object_evas_get(obj));
   e_theme_edje_object_set(o, "base/theme/widgets",
                           "e/widgets/toolbar/item");
   it = E_NEW(Item, 1);
   it->o_toolbar = obj;
   it->o_base = o;
   it->o_icon = icon;
   it->func = func;
   it->data1 = data1;
   it->data2 = data2;
   wd->items = eina_list_append(wd->items, it);

   edje_object_signal_callback_add(o, "e,action,click", "e",
                                   _e_wid_signal_cb1, it);
   evas_object_size_hint_min_set(icon, wd->icon_w, wd->icon_h);
   if (icon)
     {
        edje_object_part_swallow(o, "e.swallow.icon", icon);
        evas_object_show(icon);
        edje_object_signal_emit(o, "e,icon,enabled", "e");
     }
   else
     edje_object_signal_emit(o, "e,icon,disabled", "e");
   edje_object_message_signal_process(o);
   edje_object_part_text_set(o, "e.text.label", label);
   edje_object_size_min_calc(o, &mw, &mh);
   e_widget_sub_object_add(obj, o);
   e_box_pack_end(wd->o_box, o);
   e_box_pack_options_set(o,
			  1, 1, /* fill */
			  0, 0, /* expand */
			  0.5, 0.5, /* align */
			  mw, mh, /* min */
			  9999, 9999 /* max */
			  );
   evas_object_show(o);
   e_box_size_min_get(wd->o_box, &mw, &mh);
   evas_object_resize(wd->o_box, mw, mh);
   evas_object_resize(wd->o_base, 500, 500);
   e_scrollframe_child_viewport_size_get(wd->o_base, &vw, &vh);
   if (wd->scrollable)
     e_widget_size_min_set(obj, 500 - vw, mh + (500 - vh));
   else
     e_widget_size_min_set(obj, mw + (500 - vw), mh + (500 - vh));
   evas_object_resize(wd->o_box, mw, mh);
}

EAPI void
e_widget_toolbar_item_remove(Evas_Object *obj, int num)
{
   E_Widget_Data *wd;
   Item *it;

   if (!obj) return;
   if (!(wd = e_widget_data_get(obj))) return;
   it = eina_list_nth(wd->items, num);
   if (it)
     {
	evas_object_del(it->o_base);
	if (it->o_icon) evas_object_del(it->o_icon);
	wd->items = eina_list_remove(wd->items, it);
	E_FREE(it);
     }
}

EAPI void
e_widget_toolbar_item_select(Evas_Object *obj, int num)
{
   E_Widget_Data *wd = NULL;
   Eina_List *l = NULL;
   Item *it = NULL;
   int i = 0;

   if (!obj) return;
   if (!(wd = e_widget_data_get(obj))) return;
   EINA_LIST_FOREACH(wd->items, l, it)
     {
        if (i == num)
          {
             if (!it->selected) _item_select(it);
          }
        else
          {
             if (it->selected) _item_unselect(it);
          }
	i++;
     }
}

EAPI void
e_widget_toolbar_item_label_set(Evas_Object *obj, int num, const char *label)
{
   E_Widget_Data *wd = NULL;
   Item *it = NULL;

   if (!obj) return;
   if (!(wd = e_widget_data_get(obj))) return;
   it = eina_list_nth(wd->items, num);
   if (it)
     {
	int mw, mh;

	edje_object_part_text_set(it->o_base, "e.text.label", label);
	edje_object_size_min_calc(it->o_base, &mw, &mh);
	e_box_pack_options_set(it->o_base,
			       1, 1, /* fill */
			       0, 0, /* expand */
			       0.5, 0.5, /* align */
			       mw, mh, /* min */
			       9999, 9999 /* max */
			       );
     }
}

EAPI void
e_widget_toolbar_scrollable_set(Evas_Object *obj, Eina_Bool scrollable)
{
   E_Widget_Data *wd;
   Evas_Coord mw = 0, mh = 0, vw = 0, vh = 0;

   if (!obj) return;
   if (!(wd = e_widget_data_get(obj))) return;
   wd->scrollable = scrollable;
   e_box_size_min_get(wd->o_box, &mw, &mh);
   evas_object_resize(wd->o_box, mw, mh);
   evas_object_resize(wd->o_base, 500, 500);
   e_scrollframe_child_viewport_size_get(wd->o_base, &vw, &vh);
   if (wd->scrollable)
     e_widget_size_min_set(obj, 500 - vw, mh + (500 - vh));
   else
     e_widget_size_min_set(obj, mw + (500 - vw), mh + (500 - vh));
   evas_object_resize(wd->o_box, mw, mh);
}

EAPI void
e_widget_toolbar_focus_steal_set(Evas_Object *obj, Eina_Bool steal)
{
   E_Widget_Data *wd;

   if (!obj) return;
   if (!(wd = e_widget_data_get(obj))) return;
   if (wd->focus_steal == steal) return;
   if (steal)
     {
	evas_object_event_callback_add(e_scrollframe_edje_object_get(wd->o_base),
				       EVAS_CALLBACK_MOUSE_DOWN,
				       _e_wid_focus_steal, obj);
	wd->focus_steal = EINA_TRUE;
     }
   else
     {
	evas_object_event_callback_del(e_scrollframe_edje_object_get(wd->o_base),
				       EVAS_CALLBACK_MOUSE_DOWN,
				       _e_wid_focus_steal);
	wd->focus_steal = EINA_FALSE;
     }
}

EAPI void
e_widget_toolbar_clear(Evas_Object *obj)
{
   E_Widget_Data *wd = NULL;
   Item *it = NULL;

   if (!obj) return;
   if (!(wd = e_widget_data_get(obj))) return;
   EINA_LIST_FREE(wd->items, it)
     {
        evas_object_del(it->o_base);
        if (it->o_icon) evas_object_del(it->o_icon);
        E_FREE(it);
     }
}

EAPI int
e_widget_toolbar_item_selected_get(Evas_Object *obj)
{
   E_Widget_Data *wd = NULL;
   Eina_List *l = NULL;
   Item *it = NULL;
   int i = 0;

   if (!obj) return 0;
   if (!(wd = e_widget_data_get(obj))) return 0;
   EINA_LIST_FOREACH(wd->items, l, it)
     {
        if (it->selected) return i;
        i++;
     }
   return 0;
}

/* local functions */
static void
_e_wid_del_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;
   Item *it;

   if (!obj) return;
   if (!(wd = e_widget_data_get(obj))) return;
   EINA_LIST_FREE(wd->items, it)
     {
        evas_object_del(it->o_base);
        if (it->o_icon) evas_object_del(it->o_icon);
        E_FREE(it);
     }
   E_FREE(wd);
}

static void
_e_wid_disable_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;

   if (!obj) return;
   if (!(wd = e_widget_data_get(obj))) return;
   if (e_widget_disabled_get(obj))
     edje_object_signal_emit(e_scrollframe_edje_object_get(wd->o_base),
                             "e,state,disabled", "e");
   else
     edje_object_signal_emit(e_scrollframe_edje_object_get(wd->o_base),
                             "e,state,enabled", "e");
}

static void
_e_wid_signal_cb1(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Item *it, *it2;
   E_Widget_Data *wd;
   Eina_List *l;

   if (!(it = data)) return;
   if (it->selected) return;
   wd = e_widget_data_get(it->o_toolbar);
   EINA_LIST_FOREACH(wd->items, l, it2)
     {
        if (it2->selected)
          {
             _item_unselect(it2);
             break;
          }
     }
   _item_select(it);
}

static void
_e_wid_signal_prev(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   E_Widget_Data *wd;
   Eina_List *l, *l2;
   Item *it = NULL, *it2 = NULL;

   if (!(wd = e_widget_data_get(data))) return;
   if ((!wd->o_base) || (!wd->o_box)) return;
   EINA_LIST_FOREACH(wd->items, l, it)
     {
        if (it->selected)
          {
             l2 = eina_list_prev(l);
             if (l2) it2 = eina_list_data_get(l2);
             break;
          }
     }
   if ((it) && (it2) && (it != it2))
     {
	_item_unselect(it);
	_item_select(it2);
     }
}

static void
_e_wid_signal_next(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   E_Widget_Data *wd;
   Eina_List *l, *l2;
   Item *it = NULL, *it2 = NULL;

   if (!(wd = e_widget_data_get(data))) return;
   if ((!wd->o_base) || (!wd->o_box)) return;
   EINA_LIST_FOREACH(wd->items, l, it)
     {
        if (it->selected)
          {
             l2 = eina_list_next(l);
             if (l2) it2 = eina_list_data_get(l2);
             break;
          }
     }
   if ((it) && (it2) && (it != it2))
     {
	_item_unselect(it);
	_item_select(it2);
     }
}

static void
_e_wid_cb_scrollframe_resize(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   E_Widget_Data *wd;
   Evas_Coord mw, mh, vw, vh, w, h;
   Eina_List *l;
   Item *it;

   if (!(wd = e_widget_data_get(data))) return;
   if ((!wd->o_base) || (!wd->o_box)) return;

   e_scrollframe_child_viewport_size_get(wd->o_base, &vw, &vh);
   e_box_size_min_get(wd->o_box, &mw, &mh);
   evas_object_geometry_get(wd->o_box, NULL, NULL, &w, &h);
   if (vw >= mw)
     {
        if (w != vw) evas_object_resize(wd->o_box, vw, h);
     }
   EINA_LIST_FOREACH(wd->items, l, it)
     {
        if (it->selected)
          {
             _item_show(it);
             break;
          }
     }
}

static void
_e_wid_cb_key_down(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Evas_Event_Key_Down *ev;
   E_Widget_Data *wd;
   Eina_List *l, *l2;
   Item *it = NULL, *it2 = NULL;

   ev = event_info;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return;
   wd = e_widget_data_get(data);
   if ((!strcmp(ev->key, "Up")) || (!strcmp(ev->key, "KP_Up")) ||
       (!strcmp(ev->key, "Left")) || (!strcmp(ev->key, "KP_Left")))
     {
	EINA_LIST_FOREACH(wd->items, l, it)
          {
             if (it->selected)
               {
		  l2 = eina_list_prev(l);
                  if (l2) it2 = eina_list_data_get(l2);
                  break;
               }
          }
     }
   else if ((!strcmp(ev->key, "Down")) ||
            (!strcmp(ev->key, "KP_Down")) ||
            (!strcmp(ev->key, "Right")) ||
            (!strcmp(ev->key, "KP_Right")))
     {
	EINA_LIST_FOREACH(wd->items, l, it)
          {
             if (it->selected)
               {
		  l2 = eina_list_next(l);
                  if (l2) it2 = eina_list_data_get(l2);
                  break;
               }
          }
     }
   else if ((!strcmp(ev->key, "Home")) ||
            (!strcmp(ev->key, "KP_Home")))
     {
	EINA_LIST_FOREACH(wd->items, l, it)
	  {
	     if (it->selected)
	       {
		  it2 = eina_list_data_get(wd->items);
		  break;
	       }
	  }
     }
   else if ((!strcmp(ev->key, "End")) || (!strcmp(ev->key, "KP_End")))
     {
        EINA_LIST_FOREACH(wd->items, l, it)
          {
	     if (it->selected)
	       {
		  it2 = eina_list_last_data_get(wd->items);
		  break;
	       }
          }
     }
   if ((it) && (it2) && (it != it2))
     {
	_item_unselect(it);
	_item_select(it2);
     }
}

static void
_e_wid_focus_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;

   if (!obj) return;
   if (!(wd = e_widget_data_get(obj))) return;
   if (e_widget_focus_get(obj))
     {
        edje_object_signal_emit(wd->o_base, "e,state,focused", "e");
        evas_object_focus_set(obj, EINA_TRUE);
     }
   else
     {
        edje_object_signal_emit(wd->o_base, "e,state,unfocused", "e");
        evas_object_focus_set(obj, EINA_FALSE);
     }
}

static void
_e_wid_focus_steal(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   e_widget_focus_steal(data);
}

static void
_item_show(Item *it)
{
   E_Widget_Data *wd;
   Evas_Coord x, y, w, h, bx, by;

   wd = e_widget_data_get(it->o_toolbar);
   evas_object_geometry_get(wd->o_box, &bx, &by, NULL, NULL);
   evas_object_geometry_get(it->o_base, &x, &y, &w, &h);
   e_scrollframe_child_region_show(wd->o_base, x - bx, y - by, w, h);
}

static void
_item_select(Item *it)
{
   it->selected = EINA_TRUE;
   edje_object_signal_emit(it->o_base, "e,state,selected", "e");
   if (it->o_icon)
     {
        if (strcmp(evas_object_type_get(it->o_icon), "e_icon"))
          edje_object_signal_emit(it->o_icon, "e,state,selected", "e");
        else
          e_icon_selected_set(it->o_icon, EINA_TRUE);
     }
   _item_show(it);
   if (it->func) it->func((void *)it->data1, (void *)it->data2);
}

static void
_item_unselect(Item *it)
{
   it->selected = EINA_FALSE;
   edje_object_signal_emit(it->o_base, "e,state,unselected", "e");
   if (it->o_icon)
     {
        if (strcmp(evas_object_type_get(it->o_icon), "e_icon"))
          edje_object_signal_emit(it->o_icon, "e,state,unselected", "e");
        else
          e_icon_selected_set(it->o_icon, EINA_FALSE);
     }
}
