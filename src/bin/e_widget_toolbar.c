/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

typedef struct _E_Widget_Data E_Widget_Data;
typedef struct _Item Item;
struct _E_Widget_Data
{
   Evas_Object *o_base, *o_box, *o_scrollframe0;
   int icon_w, icon_h;
   Eina_List *items;
   Evas_Bool scrollable : 1;
};

struct _Item
{
   Evas_Object *o_toolbar, *o_base, *o_icon;
   void (*func) (void *data1, void *data2);
   const void *data1, *data2;
   Evas_Bool selected : 1;
};

static void _e_wid_del_hook(Evas_Object *obj);
static void _e_wid_focus_hook(Evas_Object *obj);
static void _e_wid_focus_steal(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_wid_disable_hook(Evas_Object *obj);
static void _e_wid_signal_cb1(void *data, Evas_Object *obj, const char *emission, const char *source);
static void _e_wid_cb_scrollframe_resize(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_wid_cb_key_down(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void _item_show(Item *it);

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
   wd = calloc(1, sizeof(E_Widget_Data));
   e_widget_data_set(obj, wd);
   wd->icon_w = icon_w;
   wd->icon_h = icon_h;
   
   o = e_scrollframe_add(evas);
   wd->o_base = o;
   o = e_box_add(evas);
   wd->o_box = o;
   o = wd->o_base;
   e_scrollframe_custom_theme_set(o, "base/theme/widgets", "e/widgets/toolbar");
   e_scrollframe_single_dir_set(o, 1);
   e_scrollframe_policy_set(o, E_SCROLLFRAME_POLICY_AUTO, E_SCROLLFRAME_POLICY_OFF);
   e_scrollframe_thumbscroll_force(o, 1);
   evas_object_event_callback_add(o, EVAS_CALLBACK_RESIZE, 
                                  _e_wid_cb_scrollframe_resize, obj);
   evas_object_event_callback_add(e_scrollframe_edje_object_get(wd->o_base), 
                                  EVAS_CALLBACK_MOUSE_DOWN,
                                  _e_wid_focus_steal, obj);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_KEY_DOWN,
                                  _e_wid_cb_key_down, obj);
   evas_object_show(o);
   e_widget_sub_object_add(obj, o);
   e_widget_resize_object_set(obj, o);

   o = wd->o_box;
   e_box_orientation_set(o, 1);
   e_box_homogenous_set(o, 1);
   e_scrollframe_child_set(wd->o_base, o);
   e_widget_sub_object_add(obj, o);
   evas_object_show(o);
   
   edje_object_size_min_calc
     (e_scrollframe_edje_object_get(wd->o_base), &mw, &mh);
   e_widget_min_size_set(obj, mw, mh);
   
   return obj;
}

EAPI void
e_widget_toolbar_item_append(Evas_Object *obj, Evas_Object *icon, const char *label, void (*func) (void *data1, void *data2), const void *data1, const void *data2)
{
   E_Widget_Data *wd;
   Evas_Object *o;
   Item *it;
   Evas_Coord mw = 0, mh = 0, vw = 0, vh = 0;
   
   wd = e_widget_data_get(obj);
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
   edje_extern_object_min_size_set(icon, wd->icon_w, wd->icon_h);
   edje_object_part_swallow(o, "e.swallow.icon", icon);
   evas_object_show(icon);
   edje_object_part_text_set(o, "e.text.label", label);
   edje_object_size_min_calc(o, &mw, &mh);
   e_widget_sub_object_add(obj, o);
   e_box_pack_end(wd->o_box, o);
   evas_object_show(o);
   e_box_pack_options_set(o,
			  1, 1, /* fill */
			  0, 0, /* expand */
			  0.5, 0.5, /* align */
			  mw, mh, /* min */
			  9999, 9999 /* max */
			  );
   e_box_min_size_get(wd->o_box, &mw, &mh);
   evas_object_resize(wd->o_box, mw, mh);
   evas_object_resize(wd->o_base, 500, 500);
   e_scrollframe_child_viewport_size_get(wd->o_base, &vw, &vh);
   if (wd->scrollable)
     e_widget_min_size_set(obj, 500 - vw, mh + (500 - vh));
   else
     e_widget_min_size_set(obj, mw + (500 - vw), mh + (500 - vh));
}

EAPI void
e_widget_toolbar_item_select(Evas_Object *obj, int num)
{
   E_Widget_Data *wd;
   Eina_List *l;
   Item *it;
   int i;
   
   wd = e_widget_data_get(obj);
   for (i = 0, l = wd->items; l; l = l->next, i++)
     {
        it = l->data;
        if (i == num)
          {
             if (!it->selected)
               {
                  it->selected = 1;
                  edje_object_signal_emit(it->o_base, "e,state,selected", "e");
                  edje_object_signal_emit(it->o_icon, "e,state,selected", "e");
                  _item_show(it);
                  if (it->func) it->func(it->data1, it->data2);
               }
          }
        else
          {
             if (it->selected)
               {
                  it->selected = 0;
                  edje_object_signal_emit(it->o_base, "e,state,unselected", "e");
                  edje_object_signal_emit(it->o_icon, "e,state,unselected", "e");
               }
          }
     }
}

EAPI void
e_widget_toolbar_scrollable_set(Evas_Object *obj, Evas_Bool scrollable)
{
   E_Widget_Data *wd;
   Evas_Coord mw = 0, mh = 0, vw = 0, vh = 0;
   
   wd = e_widget_data_get(obj);
   wd->scrollable = scrollable;
   e_box_min_size_get(wd->o_box, &mw, &mh);
   evas_object_resize(wd->o_box, mw, mh);
   evas_object_resize(wd->o_base, 500, 500);
   e_scrollframe_child_viewport_size_get(wd->o_base, &vw, &vh);
   if (wd->scrollable)
     e_widget_min_size_set(obj, 500 - vw, mh + (500 - vh));
   else
     e_widget_min_size_set(obj, mw + (500 - vw), mh + (500 - vh));
}

static void
_e_wid_del_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(obj);
   while (wd->items)
     {
        Item *it;
        
        it = wd->items->data;
        evas_object_del(it->o_base);
        evas_object_del(it->o_icon);
        free(it);
        wd->items = eina_list_remove_list(wd->items, wd->items);
     }
   free(wd);
}

static void
_e_wid_disable_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(obj);
   if (e_widget_disabled_get(obj))
     edje_object_signal_emit
     (e_scrollframe_edje_object_get(wd->o_base), "e,state,disabled", "e");
   else
     edje_object_signal_emit
     (e_scrollframe_edje_object_get(wd->o_base), "e,state,enabled", "e");
}

static void
_e_wid_signal_cb1(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   Item *it, *it2;
   E_Widget_Data *wd;
   Eina_List *l;
   
   it = data;
   if (it->selected) return;
   wd = e_widget_data_get(it->o_toolbar);
   for (l = wd->items; l; l = l->next)
     {
        it2 = l->data;
        if (it2->selected)
          {
             it2->selected = 0;
             edje_object_signal_emit(it2->o_base, "e,state,unselected", "e");
             edje_object_signal_emit(it2->o_icon, "e,state,unselected", "e");
             break;
          }
     }
   it->selected = 1;
   edje_object_signal_emit(it->o_base, "e,state,selected", "e");
   edje_object_signal_emit(it->o_icon, "e,state,selected", "e");
   _item_show(it);
   if (it->func) it->func(it->data1, it->data2);
}

static void
_e_wid_cb_scrollframe_resize(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   E_Widget_Data *wd;
   Evas_Coord mw, mh, vw, vh, w, h;
   Eina_List *l;
   Item *it;
   
   wd = e_widget_data_get(data);

   if (wd->o_base == NULL || wd->o_box == NULL) return ;

   e_scrollframe_child_viewport_size_get(wd->o_base, &vw, &vh);
   e_box_min_size_get(wd->o_box, &mw, &mh);
   evas_object_geometry_get(wd->o_box, NULL, NULL, &w, &h);
   if (vw >= mw)
     {
        if (w != vw) evas_object_resize(wd->o_box, vw, h);
     }
   for (l = wd->items; l; l = l->next)
     {
        it = l->data;
        if (it->selected)
          {
             _item_show(it);
             break;
          }
     }
}

static void
_e_wid_cb_key_down(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
   Evas_Event_Key_Down *ev;
   E_Widget_Data *wd;
   Eina_List *l;
   Item *it = NULL, *it2 = NULL;
   
   ev = event_info;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return;
   wd = e_widget_data_get(data);
   if ((!strcmp(ev->keyname, "Up")) || (!strcmp(ev->keyname, "KP_Up")) ||
       (!strcmp(ev->keyname, "Left")) || (!strcmp(ev->keyname, "KP_Left"))
       )
     {
        for (l = wd->items; l; l = l->next)
          {
             it = l->data;
             if (it->selected)
               {
                  if (l->prev) it2 = l->prev->data;
                  break;
               }
          }
     }
   else if ((!strcmp(ev->keyname, "Down")) || (!strcmp(ev->keyname, "KP_Down")) ||
            (!strcmp(ev->keyname, "Right")) || (!strcmp(ev->keyname, "KP_Right"))
       )
     {
        for (l = wd->items; l; l = l->next)
          {
             it = l->data;
             if (it->selected)
               {
                  if (l->next) it2 = l->next->data;
                  break;
               }
          }
     }
   else if ((!strcmp(ev->keyname, "Home")) || (!strcmp(ev->keyname, "KP_Home")))
     {
	for (l = wd->items; l; l = l->next)
	  {
	     it = l->data;
	     if (it->selected)
	       {
		  it2 = wd->items->data;
		  break;
	       }
	  }
     }
   else if ((!strcmp(ev->keyname, "End")) || (!strcmp(ev->keyname, "KP_End")))
     {
	for (l = wd->items; l; l = l->next)
	  {
	     it = l->data;
	     if (it->selected)
	       {
		  it2 = eina_list_last(wd->items)->data;
		  break;
	       }
	  }
     }
   if ((it) && (it2) && (it != it2))
     {
        it->selected = 0;
        edje_object_signal_emit(it->o_base, "e,state,unselected", "e");
        edje_object_signal_emit(it->o_icon, "e,state,unselected", "e");
        it2->selected = 1;
        edje_object_signal_emit(it2->o_base, "e,state,selected", "e");
        edje_object_signal_emit(it2->o_icon, "e,state,selected", "e");
        _item_show(it2);
        if (it2->func) it->func(it2->data1, it2->data2);
     }
}

static void
_e_wid_focus_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(obj);
   if (e_widget_focus_get(obj))
     {
        edje_object_signal_emit(wd->o_base, "e,state,focused", "e");
        evas_object_focus_set(obj, 1);
     }
   else
     {
        edje_object_signal_emit(wd->o_base, "e,state,unfocused", "e");
        evas_object_focus_set(obj, 0);
     }
}

static void
_e_wid_focus_steal(void *data, Evas *e, Evas_Object *obj, void *event_info)
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

