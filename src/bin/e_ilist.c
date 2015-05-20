#include "e.h"

#define SMART_NAME     "e_ilist"
#define API_ENTRY      E_Smart_Data * sd; sd = evas_object_smart_data_get(obj); if ((!obj) || (!sd) || (evas_object_type_get(obj) && strcmp(evas_object_type_get(obj), SMART_NAME)))
#define INTERNAL_ENTRY E_Smart_Data * sd; sd = evas_object_smart_data_get(obj); if (!sd) return;

typedef struct _E_Smart_Data E_Smart_Data;
struct _E_Smart_Data
{
   Evas_Coord    x, y, w, h, iw, ih;
   Evas_Object  *o_smart, *o_edje, *o_box;
   Eina_List    *items;
   Eina_List    *selected_items;
   int           selected;
   const char *theme;
   unsigned char selector : 1;
   unsigned char multi_select : 1;
   unsigned char on_hold : 1;

   struct
   {
      char        *buf;
      unsigned int size;
      Ecore_Timer *timer;
   } typebuf;
};

static void      _e_smart_init(void);
static void      _e_smart_add(Evas_Object *obj);
static void      _e_smart_del(Evas_Object *obj);
static void      _e_smart_show(Evas_Object *obj);
static void      _e_smart_hide(Evas_Object *obj);
static void      _e_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void      _e_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static void      _e_smart_color_set(Evas_Object *obj, int r, int g, int b, int a);
static void      _e_smart_clip_set(Evas_Object *obj, Evas_Object *clip);
static void      _e_smart_clip_unset(Evas_Object *obj);
static void      _e_smart_reconfigure(E_Smart_Data *sd);
static void      _e_smart_event_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void      _e_smart_event_mouse_up(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void      _e_smart_event_key_down(void *data, Evas *evas, Evas_Object *obj, void *event_info);

static void      _e_typebuf_add(Evas_Object *obj, const char *s);
static void      _e_typebuf_match(Evas_Object *obj);
static Eina_Bool _e_typebuf_timer_cb(void *data);
static void      _e_typebuf_timer_update(Evas_Object *obj);
static void      _e_typebuf_timer_delete(Evas_Object *obj);
static void      _e_typebuf_clean(Evas_Object *obj);

static E_Ilist_Item *_e_ilist_item_new(E_Smart_Data *sd, Evas_Object *icon, Evas_Object *end, const char *label, int header, Ecore_End_Cb func, Ecore_End_Cb func_hilight, void *data, void *data2);
static void      _e_ilist_item_theme_set(E_Ilist_Item *si, Eina_Bool custom, Eina_Bool header, Eina_Bool even);
static void      _e_ilist_widget_hack_cb(E_Smart_Data *sd, Evas_Object *obj __UNUSED__, Evas_Object *scr);

static void      _item_select(E_Ilist_Item *si);
static void      _item_unselect(E_Ilist_Item *si);

static Evas_Smart *_e_smart = NULL;

EAPI Evas_Object *
e_ilist_add(Evas *evas)
{
   _e_smart_init();
   return evas_object_smart_add(evas, _e_smart);
}

EAPI void
e_ilist_append(Evas_Object *obj, Evas_Object *icon, Evas_Object *end, const char *label, int header, void (*func)(void *data, void *data2), void (*func_hilight)(void *data, void *data2), void *data, void *data2)
{
   E_Ilist_Item *si;
   Evas_Coord mw = 0, mh = 0;
   const char *stacking;

   API_ENTRY return;
   si = _e_ilist_item_new(sd, icon, end, label, header, func, func_hilight, data, data2);
   sd->items = eina_list_append(sd->items, si);

   edje_object_size_min_calc(si->o_base, &mw, &mh);
   e_box_freeze(sd->o_box);
   e_box_pack_end(sd->o_box, si->o_base);
   e_box_pack_options_set(si->o_base, 1, 1, 1, 1, 0.5, 0.5,
                          mw, mh, 99999, 99999);
   stacking = edje_object_data_get(si->o_base, "stacking");
   if (stacking)
     {
        if (!strcmp(stacking, "below")) evas_object_lower(si->o_base);
        else if (!strcmp(stacking, "above"))
          evas_object_raise(si->o_base);
     }
   e_box_thaw(sd->o_box);

   evas_object_lower(sd->o_box);

   evas_object_show(si->o_base);
}

EAPI void
e_ilist_append_relative(Evas_Object *obj, Evas_Object *icon, Evas_Object *end, const char *label, int header, void (*func)(void *data, void *data2), void (*func_hilight)(void *data, void *data2), void *data, void *data2, int relative)
{
   E_Ilist_Item *si, *ri;
   Evas_Coord mw = 0, mh = 0;
   const char *stacking;

   API_ENTRY return;
   si = _e_ilist_item_new(sd, icon, end, label, header, func, func_hilight, data, data2);

   ri = eina_list_nth(sd->items, relative);
   if (ri)
     sd->items = eina_list_append_relative(sd->items, si, ri);
   else
     sd->items = eina_list_append(sd->items, si);

   edje_object_size_min_calc(si->o_base, &mw, &mh);
   e_box_freeze(sd->o_box);
   if (ri)
     e_box_pack_after(sd->o_box, si->o_base, ri->o_base);
   else
     e_box_pack_end(sd->o_box, si->o_base);
   e_box_pack_options_set(si->o_base, 1, 1, 1, 1, 0.5, 0.5,
                          mw, mh, 99999, 99999);
   stacking = edje_object_data_get(si->o_base, "stacking");
   if (stacking)
     {
        if (!strcmp(stacking, "below")) evas_object_lower(si->o_base);
        else if (!strcmp(stacking, "above"))
          evas_object_raise(si->o_base);
     }
   e_box_thaw(sd->o_box);

   evas_object_lower(sd->o_box);
   evas_object_show(si->o_base);
}

EAPI void
e_ilist_prepend(Evas_Object *obj, Evas_Object *icon, Evas_Object *end, const char *label, int header, void (*func)(void *data, void *data2), void (*func_hilight)(void *data, void *data2), void *data, void *data2)
{
   E_Ilist_Item *si;
   Evas_Coord mw = 0, mh = 0;

   API_ENTRY return;
   si = _e_ilist_item_new(sd, icon, end, label, header, func, func_hilight, data, data2);
   sd->items = eina_list_prepend(sd->items, si);

   edje_object_size_min_calc(si->o_base, &mw, &mh);
   e_box_freeze(sd->o_box);
   e_box_pack_start(sd->o_box, si->o_base);
   e_box_pack_options_set(si->o_base, 1, 1, 1, 1, 0.5, 0.5,
                          mw, mh, 99999, 99999);
   e_box_thaw(sd->o_box);

   evas_object_lower(sd->o_box);
   evas_object_show(si->o_base);
}

EAPI void
e_ilist_prepend_relative(Evas_Object *obj, Evas_Object *icon, Evas_Object *end, const char *label, int header, void (*func)(void *data, void *data2), void (*func_hilight)(void *data, void *data2), void *data, void *data2, int relative)
{
   E_Ilist_Item *si, *ri;
   Evas_Coord mw = 0, mh = 0;

   API_ENTRY return;
   si = _e_ilist_item_new(sd, icon, end, label, header, func, func_hilight, data, data2);

   ri = eina_list_nth(sd->items, relative);
   if (ri)
     sd->items = eina_list_prepend_relative(sd->items, si, ri);
   else
     sd->items = eina_list_prepend(sd->items, si);

   edje_object_size_min_calc(si->o_base, &mw, &mh);
   e_box_freeze(sd->o_box);
   if (ri)
     e_box_pack_before(sd->o_box, si->o_base, ri->o_base);
   else
     e_box_pack_end(sd->o_box, si->o_base);
   e_box_pack_options_set(si->o_base, 1, 1, 1, 1, 0.5, 0.5,
                          mw, mh, 99999, 99999);
   e_box_thaw(sd->o_box);

   evas_object_lower(sd->o_box);
   evas_object_show(si->o_base);
}

EAPI void
e_ilist_clear(Evas_Object *obj)
{
   E_Ilist_Item *si = NULL;

   API_ENTRY return;

   e_ilist_freeze(obj);
   EINA_LIST_FREE(sd->items, si)
     {
        if (!si) continue;
        if (si->o_icon) evas_object_del(si->o_icon);
        if (si->o_end) evas_object_del(si->o_end);
        if (si->label) eina_stringshare_del(si->label);
        evas_object_del(si->o_base);
        E_FREE(si);
     }
   if (sd->selected_items) sd->selected_items = eina_list_free(sd->selected_items);
   e_ilist_thaw(obj);
   sd->selected = -1;
}

EAPI void
e_ilist_freeze(Evas_Object *obj)
{
   API_ENTRY return;
   e_box_freeze(sd->o_box);
}

EAPI void
e_ilist_thaw(Evas_Object *obj)
{
   API_ENTRY return;
   e_box_thaw(sd->o_box);
}

EAPI int
e_ilist_count(Evas_Object *obj)
{
   API_ENTRY return 0;
   return eina_list_count(sd->items);
}

EAPI int
e_ilist_selector_get(Evas_Object *obj)
{
   API_ENTRY return 0;
   return sd->selector;
}

EAPI void
e_ilist_selector_set(Evas_Object *obj, int selector)
{
   API_ENTRY return;
   sd->selector = selector;
}

EAPI Eina_Bool
e_ilist_multi_select_get(Evas_Object *obj)
{
   API_ENTRY return 0;
   return sd->multi_select;
}

EAPI void
e_ilist_multi_select_set(Evas_Object *obj, Eina_Bool multi)
{
   API_ENTRY return;
   sd->multi_select = multi;
}

EAPI void
e_ilist_size_min_get(Evas_Object *obj, Evas_Coord *w, Evas_Coord *h)
{
   API_ENTRY return;
   e_box_size_min_get(sd->o_box, w, h);
}

EAPI void
e_ilist_unselect(Evas_Object *obj)
{
   API_ENTRY return;

   if (!sd->items) return;
   if (sd->selected < 0) return;
   while (sd->selected_items)
     _item_unselect(sd->selected_items->data);
   sd->selected = -1;
}

EAPI void
e_ilist_selected_set(Evas_Object *obj, int n)
{
   E_Ilist_Item *si = NULL;
   int i;

   API_ENTRY return;
   if (!sd->items) return;

   i = eina_list_count(sd->items);
   if (n >= i) n = i - 1;
   else if (n < 0)
     n = 0;

   e_ilist_unselect(obj);
   if (!(si = eina_list_nth(sd->items, n))) return;

   /* NB: Remove this if headers ever become selectable */
   while (si->header && ((++n) < i))
     if (!(si = eina_list_nth(sd->items, n))) return;
   while (si->header && ((--n) >= 0))
     if (!(si = eina_list_nth(sd->items, n))) return;
   if (si->header) return;

   _item_select(si);
   sd->selected = n;
   if (si->func_hilight) si->func_hilight(si->data, si->data2);
   if (sd->selector) return;
   if (!sd->on_hold)
     {
        if (si->func) si->func(si->data, si->data2);
     }
}

EAPI const Eina_List *
e_ilist_selected_items_get(Evas_Object *obj)
{
   API_ENTRY return NULL;
   return sd->selected_items;
}

EAPI int
e_ilist_selected_get(Evas_Object *obj)
{
   Eina_List *l = NULL;
   E_Ilist_Item *li = NULL;
   int i, j;

   API_ENTRY return -1;
   if (!sd->items) return -1;
   if (!sd->multi_select)
     return sd->selected;
   j = -1;
   i = 0;
   EINA_LIST_FOREACH(sd->selected_items, l, li)
     {
        if (li && li->selected) j = i;
        i++;
     }
   return j;
}

EAPI const char *
e_ilist_selected_label_get(Evas_Object *obj)
{
   E_Ilist_Item *si = NULL;

   API_ENTRY return NULL;
   if (!sd->items) return NULL;
   if (sd->multi_select) return NULL;
   if (sd->selected < 0) return NULL;
   si = eina_list_nth(sd->items, sd->selected);
   if (si)
     {
        if (!si->label)
          {
             si->label =
               eina_stringshare_add(edje_object_part_text_get(si->o_base,
                                                              "e.text.label"));
          }
        if (si->label) return si->label;
     }
   return NULL;
}

EAPI void *
e_ilist_selected_data_get(Evas_Object *obj)
{
   E_Ilist_Item *si = NULL;

   API_ENTRY return NULL;
   if (!sd->items) return NULL;
   if (sd->multi_select) return NULL;
   if (sd->selected < 0) return NULL;
   si = eina_list_nth(sd->items, sd->selected);
   if (si) return si->data;
   return NULL;
}

EAPI void *
e_ilist_selected_data2_get(Evas_Object *obj)
{
   E_Ilist_Item *si = NULL;

   API_ENTRY return NULL;
   if (!sd->items) return NULL;
   if (sd->multi_select) return NULL;
   if (sd->selected < 0) return NULL;
   si = eina_list_nth(sd->items, sd->selected);
   if (si) return si->data2;
   return NULL;
}

EAPI Evas_Object *
e_ilist_selected_icon_get(Evas_Object *obj)
{
   E_Ilist_Item *si = NULL;

   API_ENTRY return NULL;
   if (!sd->items) return NULL;
   if (sd->multi_select) return NULL;
   if (sd->selected < 0) return NULL;
   si = eina_list_nth(sd->items, sd->selected);
   if (si) return si->o_icon;
   return NULL;
}

EAPI Evas_Object *
e_ilist_selected_end_get(Evas_Object *obj)
{
   E_Ilist_Item *si = NULL;

   API_ENTRY return NULL;
   if (!sd->items) return NULL;
   if (sd->multi_select) return NULL;
   if (sd->selected < 0) return NULL;
   si = eina_list_nth(sd->items, sd->selected);
   if (si) return si->o_end;
   return NULL;
}

EAPI void
e_ilist_selected_geometry_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y, Evas_Coord *w, Evas_Coord *h)
{
   E_Ilist_Item *si = NULL;

   API_ENTRY return;
   if (!sd->items) return;
   if (sd->selected < 0) return;
   if (!(si = eina_list_nth(sd->items, sd->selected))) return;
   evas_object_geometry_get(si->o_base, x, y, w, h);
   *x -= sd->x;
   *y -= sd->y;
}

EAPI int
e_ilist_selected_count_get(Evas_Object *obj)
{
   API_ENTRY return 0;
   if (!sd->items) return 0;
   return eina_list_count(sd->selected_items);
}

EAPI void
e_ilist_remove_num(Evas_Object *obj, int n)
{
   E_Ilist_Item *si = NULL;
   Eina_List *item;
   int w, h;
   Eina_Bool resize = EINA_FALSE;

   API_ENTRY return;
   if (!sd->items) return;
   item = eina_list_nth_list(sd->items, n);
   if (!item) return;
   si = eina_list_data_get(item);
   if (!si) return;
   sd->items = eina_list_remove_list(sd->items, item);
   if (si->selected) sd->selected_items = eina_list_remove(sd->selected_items, si);

   evas_object_geometry_get(sd->o_box, NULL, NULL, &w, &h);
   if ((sd->w == w) && (sd->h == h))
     resize = e_box_item_size_get(si->o_base, &w, &h);

   if (sd->selected == n) sd->selected = -1;
   if (si->o_icon) evas_object_del(si->o_icon);
   if (si->o_end) evas_object_del(si->o_end);
   if (si->label) eina_stringshare_del(si->label);
   evas_object_del(si->o_base);
   E_FREE(si);

   /* if ilist size is size of box (e_widget_ilist),
    * autoresize here to prevent skewed perspective as in ticket #1678
    */
   if (!resize) return;
   if (!sd->items) return;
   si = eina_list_data_get(sd->items);
   evas_object_resize(sd->o_smart, w, sd->h - h);
}

EAPI const char *
e_ilist_nth_label_get(Evas_Object *obj, int n)
{
   E_Ilist_Item *si = NULL;

   API_ENTRY return NULL;
   if (!sd->items) return NULL;
   si = eina_list_nth(sd->items, n);
   if (si)
     {
        if (!si->label)
          {
             si->label =
               eina_stringshare_add(edje_object_part_text_get(si->o_base,
                                                              "e.text.label"));
          }
        if (si->label) return si->label;
     }
   return NULL;
}

EAPI void
e_ilist_item_label_set(E_Ilist_Item *si, const char *label)
{
   EINA_SAFETY_ON_NULL_RETURN(si);
   eina_stringshare_replace(&si->label, label);
   edje_object_part_text_set(si->o_base, "e.text.label", label);
}

EAPI void
e_ilist_nth_label_set(Evas_Object *obj, int n, const char *label)
{
   E_Ilist_Item *si = NULL;

   /* check for a NULL label first...simpler, faster check then doing
    * API_ENTRY check first */
   if (!label) return;
   API_ENTRY return;
   if (!sd->items) return;
   si = eina_list_nth(sd->items, n);
   if (si) e_ilist_item_label_set(si, label);
}

EAPI Evas_Object *
e_ilist_nth_icon_get(Evas_Object *obj, int n)
{
   E_Ilist_Item *si = NULL;

   API_ENTRY return NULL;
   if (!sd->items) return NULL;
   si = eina_list_nth(sd->items, n);
   if (si) return si->o_icon;
   return NULL;
}

EAPI void
e_ilist_nth_icon_set(Evas_Object *obj, int n, Evas_Object *icon)
{
   E_Ilist_Item *si = NULL;

   API_ENTRY return;
   if (!sd->items) return;
   if (!(si = eina_list_nth(sd->items, n))) return;
   if (si->o_icon)
     {
        edje_object_part_unswallow(si->o_base, si->o_icon);
        evas_object_del(si->o_icon);
     }
   si->o_icon = icon;
   if (si->o_icon)
     {
        evas_object_size_hint_min_set(si->o_icon, sd->iw, sd->ih);
        edje_object_part_swallow(si->o_base, "e.swallow.icon", si->o_icon);
        evas_object_show(si->o_icon);
     }
}

EAPI Evas_Object *
e_ilist_nth_end_get(Evas_Object *obj, int n)
{
   E_Ilist_Item *si = NULL;

   API_ENTRY return NULL;
   if (!sd->items) return NULL;
   si = eina_list_nth(sd->items, n);
   if (si) return si->o_end;
   return NULL;
}

EAPI void
e_ilist_nth_end_set(Evas_Object *obj, int n, Evas_Object *end)
{
   E_Ilist_Item *si = NULL;

   API_ENTRY return;
   if (!sd->items) return;
   if (!(si = eina_list_nth(sd->items, n))) return;
   if (si->o_end)
     {
        edje_object_part_unswallow(si->o_base, si->o_end);
        evas_object_del(si->o_end);
     }
   si->o_end = end;
   if (si->o_end)
     {
        evas_object_size_hint_min_set(si->o_end, sd->iw, sd->ih);
        edje_object_part_swallow(si->o_base, "e.swallow.end", si->o_end);
        evas_object_show(si->o_end);
     }
}

EAPI Eina_Bool
e_ilist_nth_is_header(Evas_Object *obj, int n)
{
   E_Ilist_Item *si = NULL;

   API_ENTRY return 0;
   if (!sd->items) return 0;
   si = eina_list_nth(sd->items, n);
   if (si) return si->header;
   return 0;
}

EAPI void
e_ilist_nth_geometry_get(Evas_Object *obj, int n, Evas_Coord *x, Evas_Coord *y, Evas_Coord *w, Evas_Coord *h)
{
   E_Ilist_Item *si = NULL;

   API_ENTRY return;
   if (!sd->items) return;
   if (!(si = eina_list_nth(sd->items, n))) return;
   evas_object_geometry_get(si->o_base, x, y, w, h);
   *x -= sd->x;
   *y -= sd->y;
}

EAPI void
e_ilist_icon_size_set(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   Eina_List *l = NULL;
   E_Ilist_Item *si = NULL;

   API_ENTRY return;
   if ((sd->iw == w) && (sd->ih == h)) return;
   sd->iw = w;
   sd->ih = h;
   EINA_LIST_FOREACH(sd->items, l, si)
     {
        Evas_Coord mw = 0, mh = 0;

        if (!si) continue;
        if (!si->o_icon) continue;
        evas_object_size_hint_min_set(si->o_icon, w, h);
        edje_object_part_swallow(si->o_base, "e.swallow.icon", si->o_icon);

        if (si->o_end)
          {
             Evas_Coord ew = 0, eh = 0;

             evas_object_size_hint_min_get(si->o_end, &ew, &eh);
             if ((ew <= 0) || (eh <= 0))
               {
                  ew = w;
                  eh = h;
               }
             evas_object_size_hint_min_set(si->o_end, ew, eh);
          }

        edje_object_size_min_calc(si->o_base, &mw, &mh);
        e_box_pack_options_set(si->o_icon, 1, 1, 1, 0, 0.5, 0.5,
                               mw, mh, 99999, 99999);
     }
}

EAPI const Eina_List *
e_ilist_items_get(Evas_Object *obj)
{
   API_ENTRY return NULL;
   return sd->items;
}

EAPI void
e_ilist_multi_select(Evas_Object *obj, int n)
{
   E_Ilist_Item *si = NULL;
   int i;

   API_ENTRY return;
   if ((!sd->items) || (!sd->multi_select)) return;

   i = eina_list_count(sd->items);
   if (n >= i) n = i - 1;
   else if (n < 0)
     n = 0;

   if (!(si = eina_list_nth(sd->items, n))) return;
   if (si->header) return;
   sd->selected = n;
   if (si->selected)
     {
        _item_unselect(si);
        if (si->func_hilight) si->func_hilight(si->data, si->data2);
        if (sd->selector) return;
        if (!sd->on_hold)
          {
             if (si->func) si->func(si->data, si->data2);
          }
        return;
     }
   _item_select(si);
   if (si->func_hilight) si->func_hilight(si->data, si->data2);
   if (sd->selector) return;
   if (!sd->on_hold)
     {
        if (si->func) si->func(si->data, si->data2);
     }
}

EAPI void
e_ilist_range_select(Evas_Object *obj, int n)
{
   int i, j, dir;

   API_ENTRY return;
   if ((!sd->items) || (!sd->multi_select)) return;

   i = eina_list_count(sd->items);
   if (n >= i) n = i - 1;
   else if (n < 0)
     n = 0;

   if (n < sd->selected) dir = 0;
   else dir = 1;

   if (!eina_list_nth(sd->items, n)) return;
   if (dir == 1)
     {
        for (j = (sd->selected + 1); ((j < i) && (j <= n)); j++)
          e_ilist_multi_select(sd->o_smart, j);
     }
   else
     {
        for (j = (sd->selected - 1); ((j >= 0) && (j >= n)); j--)
          e_ilist_multi_select(sd->o_smart, j);
     }
}

EAPI Eina_Bool
e_ilist_custom_edje_file_set(Evas_Object *obj, const char *file, const char *group)
{
   Eina_List *l;
   E_Ilist_Item *si;
   Eina_Bool even = EINA_FALSE;

   API_ENTRY return EINA_FALSE;

   edje_object_file_set(sd->o_edje, file, group);
   eina_stringshare_replace(&sd->theme, group);

   EINA_LIST_FOREACH(sd->items, l, si)
     {
        _e_ilist_item_theme_set(si, !!sd->theme, si->header, even);
        if (si->o_icon)
          edje_object_part_swallow(si->o_base, "e.swallow.icon", si->o_icon);
        if (si->o_end)
          edje_object_part_swallow(si->o_base, "e.swallow.end", si->o_end);
        even = !even;
     }
   return EINA_TRUE;
}

/* SMART FUNCTIONS */
static void
_e_smart_init(void)
{
   if (_e_smart) return;
   {
      static Evas_Smart_Class sc = EVAS_SMART_CLASS_INIT_NAME_VERSION(SMART_NAME);
      if (!sc.add)
        {
           sc.add = _e_smart_add;
           sc.del = _e_smart_del;
           sc.move = _e_smart_move;
           sc.resize = _e_smart_resize;
           sc.show = _e_smart_show;
           sc.hide = _e_smart_hide;
           sc.color_set = _e_smart_color_set;
           sc.clip_set = _e_smart_clip_set;
           sc.clip_unset = _e_smart_clip_unset;
        }
      _e_smart = evas_smart_class_new(&sc);
   }
}

static void
_e_smart_add(Evas_Object *obj)
{
   E_Smart_Data *sd;
   Evas *e;

   sd = calloc(1, sizeof(E_Smart_Data));
   if (!sd) return;
   evas_object_smart_data_set(obj, sd);

   e = evas_object_evas_get(obj);

   sd->o_smart = obj;
   sd->x = sd->y = sd->w = sd->h = 0;
   sd->iw = sd->ih = 24;
   sd->selected = -1;
   sd->multi_select = 0;

   sd->typebuf.buf = NULL;
   sd->typebuf.size = 0;
   sd->typebuf.timer = NULL;

   sd->o_box = e_box_add(e);
   e_box_align_set(sd->o_box, 0.0, 0.0);
   e_box_homogenous_set(sd->o_box, 0);
   evas_object_smart_member_add(sd->o_box, obj);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_KEY_DOWN,
                                  _e_smart_event_key_down, sd);
   evas_object_propagate_events_set(obj, 0);

   sd->o_edje = edje_object_add(e);
   e_theme_edje_object_set(sd->o_edje, "base/theme/widgets", "e/ilist");
   evas_object_smart_member_add(sd->o_edje, obj);

   evas_object_smart_callback_add(obj, "changed", (Evas_Smart_Cb)_e_ilist_widget_hack_cb, sd);
}

static void
_e_smart_del(Evas_Object *obj)
{
   INTERNAL_ENTRY;

   _e_typebuf_clean(obj);

   e_ilist_clear(obj);
   evas_object_del(sd->o_box);
   evas_object_del(sd->o_edje);
   eina_stringshare_del(sd->theme);
   free(sd);
}

static void
_e_smart_show(Evas_Object *obj)
{
   INTERNAL_ENTRY;
   evas_object_show(sd->o_edje);
   evas_object_show(sd->o_box);
}

static void
_e_smart_hide(Evas_Object *obj)
{
   INTERNAL_ENTRY;
   evas_object_hide(sd->o_edje);
   evas_object_hide(sd->o_box);
}

static void
_e_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   INTERNAL_ENTRY;
   if ((sd->x == x) && (sd->y == y)) return;
   sd->x = x;
   sd->y = y;
   _e_smart_reconfigure(sd);
}

static void
_e_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   INTERNAL_ENTRY;
   if ((sd->w == w) && (sd->h == h)) return;
   sd->w = w;
   sd->h = h;
   _e_smart_reconfigure(sd);
}

static void
_e_smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   INTERNAL_ENTRY;
   evas_object_color_set(sd->o_edje, r, g, b, a);
   evas_object_color_set(sd->o_box, r, g, b, a);
}

static void
_e_smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
   INTERNAL_ENTRY;
   evas_object_clip_set(sd->o_edje, clip);
   evas_object_clip_set(sd->o_box, clip);
}

static void
_e_smart_clip_unset(Evas_Object *obj)
{
   INTERNAL_ENTRY;
   evas_object_clip_unset(sd->o_edje);
   evas_object_clip_unset(sd->o_box);
}

static void
_e_smart_reconfigure(E_Smart_Data *sd)
{
   evas_object_move(sd->o_edje, sd->x, sd->y);
   evas_object_resize(sd->o_edje, sd->w, sd->h);
   evas_object_move(sd->o_box, sd->x, sd->y);
   evas_object_resize(sd->o_box, sd->w, sd->h);
}

static void
_e_smart_event_mouse_down(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   E_Smart_Data *sd;
   Evas_Event_Mouse_Down *ev;
   E_Ilist_Item *si;

   ev = event_info;
   si = data;
   sd = si->sd;

   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) sd->on_hold = 1;
   else sd->on_hold = 0;

   /* NB: Remove if headers ever become selectable */
   if (si->header) return;

   if (ev->flags & EVAS_BUTTON_DOUBLE_CLICK)
     evas_object_smart_callback_call(sd->o_smart, "selected", NULL);
}

static void
_e_smart_event_mouse_up(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   E_Smart_Data *sd;
   Evas_Event_Mouse_Up *ev;
   E_Ilist_Item *si, *item;
   Eina_List *l = NULL;
   int i;

   ev = event_info;
   si = data;
   sd = si->sd;

   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) sd->on_hold = 1;
   else sd->on_hold = 0;

   /* NB: Remove if headers ever become selectable */
   if (si->header) return;

   if (sd->on_hold)
     {
        sd->on_hold = 0;
        return;
     }

   if (!sd->items) return;

   i = 0;
   EINA_LIST_FOREACH(sd->items, l, item)
     {
        if (item == si)
          {
             if (!sd->multi_select)
               e_ilist_selected_set(sd->o_smart, i);
             else
               {
                  if (evas_key_modifier_is_set(ev->modifiers, "Shift"))
                    e_ilist_range_select(sd->o_smart, i);
                  else if (evas_key_modifier_is_set(ev->modifiers, "Control"))
                    e_ilist_multi_select(sd->o_smart, i);
                  else
                    e_ilist_selected_set(sd->o_smart, i);
               }
             break;
          }
        i++;
     }

   if (!sd->selector) return;
   if (!(si = eina_list_nth(sd->items, sd->selected))) return;
   if (si->func) si->func(si->data, si->data2);
}

static void
_e_smart_event_key_down(void *data, Evas *evas __UNUSED__, Evas_Object *obj, void *event_info)
{
   Evas_Event_Key_Down *ev;
   E_Smart_Data *sd;
   E_Ilist_Item *si;
   int n, ns;

   sd = data;
   ev = event_info;
   ns = sd->selected;

   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) sd->on_hold = 1;
   else sd->on_hold = 0;

   if ((!strcmp(ev->keyname, "Up")) || (!strcmp(ev->keyname, "KP_Up")))
     {
        n = ns;
        do
          {
             if (n == 0)
               {
                  n = ns;
                  break;
               }
             --n;
             si = eina_list_nth(sd->items, n);
          }
        while ((si) && (si->header));
        if (n != ns)
          {
             if (!sd->multi_select)
               e_ilist_selected_set(sd->o_smart, n);
             else if (evas_key_modifier_is_set(ev->modifiers, "Control"))
               e_ilist_multi_select(sd->o_smart, n);
             else if (evas_key_modifier_is_set(ev->modifiers, "Shift"))
               e_ilist_range_select(sd->o_smart, n);
             else
               e_ilist_selected_set(sd->o_smart, n);
          }
     }
   else if ((!strcmp(ev->keyname, "Down")) || (!strcmp(ev->keyname, "KP_Down")))
     {
        n = ns;
        do
          {
             if (n == ((int)eina_list_count(sd->items) - 1))
               {
                  n = ns;
                  break;
               }
             ++n;
             si = eina_list_nth(sd->items, n);
          }
        while ((si) && (si->header));
        if (n != ns)
          {
             if (!sd->multi_select)
               e_ilist_selected_set(sd->o_smart, n);
             else if (evas_key_modifier_is_set(ev->modifiers, "Control"))
               e_ilist_multi_select(sd->o_smart, n);
             else if (evas_key_modifier_is_set(ev->modifiers, "Shift"))
               e_ilist_range_select(sd->o_smart, n);
             else
               e_ilist_selected_set(sd->o_smart, n);
          }
     }
   else if ((!strcmp(ev->keyname, "Home")) || (!strcmp(ev->keyname, "KP_Home")))
     {
        n = -1;
        do
          {
             if (n == ((int)eina_list_count(sd->items) - 1))
               {
                  n = ns;
                  break;
               }
             ++n;
             si = eina_list_nth(sd->items, n);
          }
        while ((si) && (si->header));
        if (n != ns)
          {
             if (!sd->multi_select)
               e_ilist_selected_set(sd->o_smart, n);
             else if (evas_key_modifier_is_set(ev->modifiers, "Control"))
               e_ilist_multi_select(sd->o_smart, n);
             else if (evas_key_modifier_is_set(ev->modifiers, "Shift"))
               e_ilist_range_select(sd->o_smart, n);
             else
               e_ilist_selected_set(sd->o_smart, n);
          }
     }
   else if ((!strcmp(ev->keyname, "End")) || (!strcmp(ev->keyname, "KP_End")))
     {
        n = eina_list_count(sd->items);
        do
          {
             if (n == 0)
               {
                  n = ns;
                  break;
               }
             --n;
             si = eina_list_nth(sd->items, n);
          }
        while ((si) && (si->header));
        if (n != ns)
          {
             if (!sd->multi_select)
               e_ilist_selected_set(sd->o_smart, n);
             else if (evas_key_modifier_is_set(ev->modifiers, "Control"))
               e_ilist_multi_select(sd->o_smart, n);
             else if (evas_key_modifier_is_set(ev->modifiers, "Shift"))
               e_ilist_range_select(sd->o_smart, n);
             else
               e_ilist_selected_set(sd->o_smart, n);
          }
     }
   else if ((!strcmp(ev->keyname, "Return")) ||
            (!strcmp(ev->keyname, "KP_Enter")) ||
            (!strcmp(ev->keyname, "space") && !sd->typebuf.buf))
     {
        if (!sd->on_hold)
          {
             si = eina_list_nth(sd->items, sd->selected);
             if (si)
               {
                  if (si->func) si->func(si->data, si->data2);
               }
          }
     }
   else if (!strcmp(ev->keyname, "Escape"))
     _e_typebuf_clean(obj);
   else if (strcmp(ev->keyname, "BackSpace") && strcmp(ev->keyname, "Tab") && ev->string)
     _e_typebuf_add(obj, ev->string);

   sd->on_hold = 0;
}

static void
_e_ilist_widget_hack_cb(E_Smart_Data *sd, Evas_Object *obj __UNUSED__, Evas_Object *scr)
{
   int w, h;
   e_scrollframe_child_viewport_size_get(scr, &w, &h);
   evas_object_resize(sd->o_edje, w, h);
}

static void
_e_typebuf_add(Evas_Object *obj, const char *s)
{
   int len;

   INTERNAL_ENTRY;

   if (!sd->typebuf.buf)
     {
        sd->typebuf.buf = malloc(16);
        if (sd->typebuf.buf)
          {
             sd->typebuf.size = 16;
             sd->typebuf.buf[0] = '\0';
          }
        else
          {
             _e_typebuf_clean(obj);
             return;
          }
     }

   len = strlen(sd->typebuf.buf);
   if (len + strlen(s) + 2 + 1 >= sd->typebuf.size)
     {
        char *p = realloc(sd->typebuf.buf, sd->typebuf.size + strlen(s) + 16);
        if (p)
          {
             sd->typebuf.buf = p;
             sd->typebuf.size = sd->typebuf.size + strlen(s) + 16;
          }
        else
          {
             _e_typebuf_clean(obj);
             return;
          }
     }

   strcat(sd->typebuf.buf, s);
   edje_object_part_text_set(sd->o_edje, "e.text.typebuf_label", sd->typebuf.buf);
   edje_object_signal_emit(sd->o_edje, "e,state,typebuf,start", "e");
   _e_typebuf_match(obj);
   _e_typebuf_timer_update(obj);
}

static void
_e_typebuf_match(Evas_Object *obj)
{
   char *match;
   Eina_List *l;
   int n;
   E_Ilist_Item *si = NULL;

   INTERNAL_ENTRY;

   match = malloc(strlen(sd->typebuf.buf) + 2 + 1);
   if (!match) return;

   strcpy(match, "*");
   strcat(match, sd->typebuf.buf);
   strcat(match, "*");

   n = 0;
   EINA_LIST_FOREACH(sd->items, l, si)
     {
        const char *label = NULL;

        if (si)
          {
             if (si->label)
               label = si->label;
             else
               label = edje_object_part_text_get(si->o_base, "e.text.label");

             if (e_util_glob_case_match(label, match))
               {
                  e_ilist_selected_set(obj, n);
                  break;
               }
          }
        n++;
     }

   free(match);
}

static Eina_Bool
_e_typebuf_timer_cb(void *data)
{
   Evas_Object *obj = data;

   _e_typebuf_clean(obj);
   return ECORE_CALLBACK_CANCEL;
}

static void
_e_typebuf_timer_update(Evas_Object *obj)
{
   INTERNAL_ENTRY;

   if (sd->typebuf.timer)
     ecore_timer_del(sd->typebuf.timer);

   sd->typebuf.timer = ecore_timer_add(3.0, _e_typebuf_timer_cb, obj);
}

static void
_e_typebuf_timer_delete(Evas_Object *obj)
{
   INTERNAL_ENTRY;

   if (sd->typebuf.timer)
     {
        ecore_timer_del(sd->typebuf.timer);
        sd->typebuf.timer = NULL;
     }
}

static void
_e_typebuf_clean(Evas_Object *obj)
{
   INTERNAL_ENTRY;

   E_FREE(sd->typebuf.buf);
   sd->typebuf.size = 0;
   _e_typebuf_timer_delete(obj);
   edje_object_signal_emit(sd->o_edje, "e,state,typebuf,stop", "e");
}

static void
_item_select(E_Ilist_Item *si)
{
   const char *selectraise;
   E_Smart_Data *sd = si->sd;
   si->selected = EINA_TRUE;
   selectraise = edje_object_data_get(si->o_base, "selectraise");
   if ((selectraise) && (!strcmp(selectraise, "on")))
     evas_object_raise(si->o_base);
   edje_object_signal_emit(si->o_base, "e,state,selected", "e");
   if (si->o_icon)
     {
        const char *t = evas_object_type_get(si->o_icon);

        if (!strcmp(t, "edje"))
          edje_object_signal_emit(si->o_icon, "e,state,selected", "e");
        else if (!strcmp(t, "e_icon"))
          e_icon_selected_set(si->o_icon, EINA_TRUE);
     }
   sd->selected_items = eina_list_append(sd->selected_items, si);
}

static void
_item_unselect(E_Ilist_Item *si)
{
   const char *stacking, *selectraise;
   E_Smart_Data *sd = si->sd;
   si->selected = EINA_FALSE;
   edje_object_signal_emit(si->o_base, "e,state,unselected", "e");
   if (si->o_icon)
     {
        if (strcmp(evas_object_type_get(si->o_icon), "e_icon"))
          edje_object_signal_emit(si->o_icon, "e,state,unselected", "e");
        else
          e_icon_selected_set(si->o_icon, EINA_FALSE);
     }
   stacking = edje_object_data_get(si->o_base, "stacking");
   selectraise = edje_object_data_get(si->o_base, "selectraise");
   if ((selectraise) && (!strcmp(selectraise, "on")))
     {
        if ((stacking) && (!strcmp(stacking, "below")))
          evas_object_lower(si->o_base);
     }
   sd->selected_items = eina_list_remove(sd->selected_items, si);
}

static void
_e_ilist_item_theme_set(E_Ilist_Item *si, Eina_Bool custom, Eina_Bool header, Eina_Bool even)
{
   E_Smart_Data *sd = si->sd;
   const char *file;
   char buf[4096];

   if ((!custom) || (!sd->theme))
     {
        if (header)
          {
             if (!even)
               {
                  if (!e_theme_edje_object_set(si->o_base, "base/theme/widgets",
                                               "e/widgets/ilist_header_odd"))
                    e_theme_edje_object_set(si->o_base, "base/theme/widgets",
                                            "e/widgets/ilist_header");
               }
             else
               e_theme_edje_object_set(si->o_base, "base/theme/widgets",
                                       "e/widgets/ilist_header");
          }
        else
          {
             if (!even)
               e_theme_edje_object_set(si->o_base, "base/theme/widgets",
                                       "e/widgets/ilist_odd");
             else
               e_theme_edje_object_set(si->o_base, "base/theme/widgets",
                                       "e/widgets/ilist");
          }
        return;
     }
   edje_object_file_get(sd->o_edje, &file, NULL);
   if (header)
     {
        if (even)
          {
             snprintf(buf, sizeof(buf), "%s/ilist_header", sd->theme);
             if (edje_object_file_set(si->o_base, file, buf)) return;
             _e_ilist_item_theme_set(si, EINA_FALSE, header, even);
             return;
          }
        snprintf(buf, sizeof(buf), "%s/ilist_header_odd", sd->theme);
        if (edje_object_file_set(si->o_base, file, buf)) return;
        _e_ilist_item_theme_set(si, EINA_FALSE, header, even);
        return;
     }
   if (even)
     {
        snprintf(buf, sizeof(buf), "%s/ilist", sd->theme);
        if (edje_object_file_set(si->o_base, file, buf)) return;
        _e_ilist_item_theme_set(si, EINA_FALSE, header, even);
        return;
     }
   snprintf(buf, sizeof(buf), "%s/ilist_odd", sd->theme);
   if (edje_object_file_set(si->o_base, file, buf)) return;
   _e_ilist_item_theme_set(si, EINA_FALSE, header, even);
   return;
}

static E_Ilist_Item *
_e_ilist_item_new(E_Smart_Data *sd, Evas_Object *icon, Evas_Object *end, const char *label, int header, Ecore_End_Cb func, Ecore_End_Cb func_hilight, void *data, void *data2)
{
   int isodd;
   E_Ilist_Item *si;

   si = E_NEW(E_Ilist_Item, 1);
   si->sd = sd;
   si->o_base = edje_object_add(evas_object_evas_get(sd->o_smart));

   isodd = eina_list_count(sd->items) & 0x1;
   _e_ilist_item_theme_set(si, !!sd->theme, header, !isodd);
   if (label)
     {
        si->label = eina_stringshare_add(label);
        edje_object_part_text_set(si->o_base, "e.text.label", label);
     }

   si->o_icon = icon;
   if (si->o_icon)
     {
        evas_object_size_hint_min_set(si->o_icon, sd->iw, sd->ih);
        edje_object_part_swallow(si->o_base, "e.swallow.icon", si->o_icon);
        evas_object_show(si->o_icon);
     }
   si->o_end = end;
   if (si->o_end)
     {
        Evas_Coord ew = 0, eh = 0;

        evas_object_size_hint_min_get(si->o_end, &ew, &eh);
        if ((ew <= 0) || (eh <= 0))
          {
             ew = sd->iw;
             eh = sd->ih;
          }
        evas_object_size_hint_min_set(si->o_end, ew, eh);
        edje_object_part_swallow(si->o_base, "e.swallow.end", si->o_end);
        evas_object_show(si->o_end);
     }
   si->func = func;
   si->func_hilight = func_hilight;
   si->data = data;
   si->data2 = data2;
   si->header = header;

   evas_object_event_callback_add(si->o_base, EVAS_CALLBACK_MOUSE_DOWN,
                                  _e_smart_event_mouse_down, si);
   evas_object_event_callback_add(si->o_base, EVAS_CALLBACK_MOUSE_UP,
                                  _e_smart_event_mouse_up, si);
   return si;
}
