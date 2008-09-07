/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

#define SMART_NAME "e_ilist"
#define API_ENTRY E_Smart_Data *sd; sd = evas_object_smart_data_get(obj); if ((!obj) || (!sd) || (evas_object_type_get(obj) && strcmp(evas_object_type_get(obj), SMART_NAME)))
#define INTERNAL_ENTRY E_Smart_Data *sd; sd = evas_object_smart_data_get(obj); if (!sd) return;

typedef struct _E_Smart_Data E_Smart_Data;
struct _E_Smart_Data 
{
   Evas_Coord x, y, w, h, iw, ih;
   Evas_Object *o_smart, *o_box;
   Evas_List *items;
   int selected;
   unsigned char selector : 1;
   unsigned char multi_select : 1;
   unsigned char on_hold : 1;
};

static void _e_smart_init             (void);
static void _e_smart_add              (Evas_Object *obj);
static void _e_smart_del              (Evas_Object *obj);
static void _e_smart_show             (Evas_Object *obj);
static void _e_smart_hide             (Evas_Object *obj);
static void _e_smart_move             (Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _e_smart_resize           (Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static void _e_smart_color_set        (Evas_Object *obj, int r, int g, int b, int a);
static void _e_smart_clip_set         (Evas_Object *obj, Evas_Object *clip);
static void _e_smart_clip_unset       (Evas_Object *obj);
static void _e_smart_reconfigure      (E_Smart_Data *sd);
static void _e_smart_event_mouse_down (void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void _e_smart_event_mouse_up   (void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void _e_smart_event_key_down   (void *data, Evas *evas, Evas_Object *obj, void *event_info);

static Evas_Smart *_e_smart = NULL;

EAPI Evas_Object *
e_ilist_add(Evas *evas) 
{
   _e_smart_init();
   return evas_object_smart_add(evas, _e_smart);
}

EAPI void 
e_ilist_append(Evas_Object *obj, Evas_Object *icon, const char *label, int header, void (*func) (void *data, void *data2), void (*func_hilight) (void *data, void *data2), void *data, void *data2)
{
   E_Ilist_Item *si;
   Evas_Coord mw = 0, mh = 0;
   int isodd;
   const char *stacking;

   API_ENTRY return;
   si = E_NEW(E_Ilist_Item, 1);
   si->sd = sd;
   si->o_base = edje_object_add(evas_object_evas_get(sd->o_smart));

   isodd = evas_list_count(sd->items) & 0x1;
   if (header) 
     {
	if (isodd)
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
	if (isodd)
	  e_theme_edje_object_set(si->o_base, "base/theme/widgets",
				  "e/widgets/ilist_odd");
	else
	  e_theme_edje_object_set(si->o_base, "base/theme/widgets",
				  "e/widgets/ilist");
     }
   if (label)
     edje_object_part_text_set(si->o_base, "e.text.label", label);
   si->o_icon = icon;
   if (si->o_icon) 
     {
	edje_extern_object_min_size_set(si->o_icon, sd->iw, sd->ih);
	edje_object_part_swallow(si->o_base, "e.swallow.icon", si->o_icon);
	evas_object_show(si->o_icon);
     }
   si->func = func;
   si->func_hilight = func_hilight;
   si->data = data;
   si->data2 = data2;
   si->header = header;
   sd->items = evas_list_append(sd->items, si);

   edje_object_size_min_calc(si->o_base, &mw, &mh);
   e_box_freeze(sd->o_box);
   e_box_pack_end(sd->o_box, si->o_base);
   e_box_pack_options_set(si->o_base, 1, 1, 1, 1, 0.5, 0.5, 
			  mw, mh, 99999, 99999);
   stacking = edje_object_data_get(si->o_base, "stacking");
   if (stacking)
     {
	if (!strcmp(stacking, "below")) evas_object_lower(si->o_base);
	else if (!strcmp(stacking, "above")) evas_object_raise(si->o_base);
     }
   e_box_thaw(sd->o_box);

   evas_object_lower(sd->o_box);
   evas_object_event_callback_add(si->o_base, EVAS_CALLBACK_MOUSE_DOWN,
				  _e_smart_event_mouse_down, si);
   evas_object_event_callback_add(si->o_base, EVAS_CALLBACK_MOUSE_UP,
				  _e_smart_event_mouse_up, si);
   evas_object_show(si->o_base);
}

EAPI void 
e_ilist_append_relative(Evas_Object *obj, Evas_Object *icon, const char *label, int header, void (*func) (void *data, void *data2), void (*func_hilight) (void *data, void *data2), void *data, void *data2, int relative) 
{
   E_Ilist_Item *si, *ri;
   Evas_Coord mw = 0, mh = 0;
   int isodd;
   const char *stacking;

   API_ENTRY return;
   si = E_NEW(E_Ilist_Item, 1);
   si->sd = sd;
   si->o_base = edje_object_add(evas_object_evas_get(sd->o_smart));

   isodd = evas_list_count(sd->items) & 0x1;
   if (header) 
     {
	if (isodd)
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
	if (isodd)
	  e_theme_edje_object_set(si->o_base, "base/theme/widgets",
				  "e/widgets/ilist_odd");
	else
	  e_theme_edje_object_set(si->o_base, "base/theme/widgets",
				  "e/widgets/ilist");
     }
   if (label)
     edje_object_part_text_set(si->o_base, "e.text.label", label);
   si->o_icon = icon;
   if (si->o_icon) 
     {
	edje_extern_object_min_size_set(si->o_icon, sd->iw, sd->ih);
	edje_object_part_swallow(si->o_base, "e.swallow.icon", si->o_icon);
	evas_object_show(si->o_icon);
     }
   si->func = func;
   si->func_hilight = func_hilight;
   si->data = data;
   si->data2 = data2;
   si->header = header;

   ri = evas_list_nth(sd->items, relative);
   if (ri)
     sd->items = evas_list_append_relative(sd->items, si, ri);
   else
     sd->items = evas_list_append(sd->items, si);

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
	else if (!strcmp(stacking, "above")) evas_object_raise(si->o_base);
     }
   e_box_thaw(sd->o_box);

   evas_object_lower(sd->o_box);
   evas_object_event_callback_add(si->o_base, EVAS_CALLBACK_MOUSE_DOWN,
				  _e_smart_event_mouse_down, si);
   evas_object_event_callback_add(si->o_base, EVAS_CALLBACK_MOUSE_UP,
				  _e_smart_event_mouse_up, si);
   evas_object_show(si->o_base);
}

EAPI void 
e_ilist_prepend(Evas_Object *obj, Evas_Object *icon, const char *label, int header, void (*func) (void *data, void *data2), void (*func_hilight) (void *data, void *data2), void *data, void *data2) 
{
   E_Ilist_Item *si;
   Evas_Coord mw = 0, mh = 0;

   API_ENTRY return;
   si = E_NEW(E_Ilist_Item, 1);
   si->sd = sd;
   si->o_base = edje_object_add(evas_object_evas_get(sd->o_smart));

   if (header) 
     e_theme_edje_object_set(si->o_base, "base/theme/widgets", 
			     "e/widgets/ilist_header");
   else if (evas_list_count(sd->items) & 0x1)
     e_theme_edje_object_set(si->o_base, "base/theme/widgets",
			     "e/widgets/ilist_odd");
   else
     e_theme_edje_object_set(si->o_base, "base/theme/widgets",
			     "e/widgets/ilist");
   if (label)
     edje_object_part_text_set(si->o_base, "e.text.label", label);
   si->o_icon = icon;
   if (si->o_icon) 
     {
	edje_extern_object_min_size_set(si->o_icon, sd->iw, sd->ih);
	edje_object_part_swallow(si->o_base, "e.swallow.icon", si->o_icon);
	evas_object_show(si->o_icon);
     }
   si->func = func;
   si->func_hilight = func_hilight;
   si->data = data;
   si->data2 = data2;
   si->header = header;
   sd->items = evas_list_prepend(sd->items, si);

   edje_object_size_min_calc(si->o_base, &mw, &mh);
   e_box_freeze(sd->o_box);
   e_box_pack_start(sd->o_box, si->o_base);
   e_box_pack_options_set(si->o_base, 1, 1, 1, 1, 0.5, 0.5, 
			  mw, mh, 99999, 99999);
   e_box_thaw(sd->o_box);

   evas_object_lower(sd->o_box);
   evas_object_event_callback_add(si->o_base, EVAS_CALLBACK_MOUSE_DOWN,
				  _e_smart_event_mouse_down, si);
   evas_object_event_callback_add(si->o_base, EVAS_CALLBACK_MOUSE_UP,
				  _e_smart_event_mouse_up, si);
   evas_object_show(si->o_base);
}

EAPI void 
e_ilist_prepend_relative(Evas_Object *obj, Evas_Object *icon, const char *label, int header, void (*func) (void *data, void *data2), void (*func_hilight) (void *data, void *data2), void *data, void *data2, int relative) 
{
   E_Ilist_Item *si, *ri;
   Evas_Coord mw = 0, mh = 0;

   API_ENTRY return;
   si = E_NEW(E_Ilist_Item, 1);
   si->sd = sd;
   si->o_base = edje_object_add(evas_object_evas_get(sd->o_smart));

   if (header) 
     e_theme_edje_object_set(si->o_base, "base/theme/widgets", 
			     "e/widgets/ilist_header");
   else if (evas_list_count(sd->items) & 0x1)
     e_theme_edje_object_set(si->o_base, "base/theme/widgets",
			     "e/widgets/ilist_odd");
   else
     e_theme_edje_object_set(si->o_base, "base/theme/widgets",
			     "e/widgets/ilist");
   if (label)
     edje_object_part_text_set(si->o_base, "e.text.label", label);
   si->o_icon = icon;
   if (si->o_icon) 
     {
	edje_extern_object_min_size_set(si->o_icon, sd->iw, sd->ih);
	edje_object_part_swallow(si->o_base, "e.swallow.icon", si->o_icon);
	evas_object_show(si->o_icon);
     }
   si->func = func;
   si->func_hilight = func_hilight;
   si->data = data;
   si->data2 = data2;
   si->header = header;

   ri = evas_list_nth(sd->items, relative);
   if (ri) 
     sd->items = evas_list_prepend_relative(sd->items, si, ri);
   else 
     sd->items = evas_list_prepend(sd->items, si);

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
   evas_object_event_callback_add(si->o_base, EVAS_CALLBACK_MOUSE_DOWN,
				  _e_smart_event_mouse_down, si);
   evas_object_event_callback_add(si->o_base, EVAS_CALLBACK_MOUSE_UP,
				  _e_smart_event_mouse_up, si);
   evas_object_show(si->o_base);
}

EAPI void 
e_ilist_clear(Evas_Object *obj) 
{
   API_ENTRY return;

   e_ilist_freeze(obj);
   while (sd->items) 
     {
	E_Ilist_Item *si = NULL;

	if (!(si = sd->items->data)) continue;
	sd->items = evas_list_remove_list(sd->items, sd->items);
	if (si->o_icon) evas_object_del(si->o_icon);
	evas_object_del(si->o_base);
	E_FREE(si);
     }
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
   return evas_list_count(sd->items);
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

EAPI int 
e_ilist_multi_select_get(Evas_Object *obj) 
{
   API_ENTRY return 0;
   return sd->multi_select;
}

EAPI void 
e_ilist_multi_select_set(Evas_Object *obj, int multi) 
{
   API_ENTRY return;
   sd->multi_select = multi;
}

EAPI void 
e_ilist_min_size_get(Evas_Object *obj, Evas_Coord *w, Evas_Coord *h) 
{
   API_ENTRY return;
   e_box_min_size_get(sd->o_box, w, h);
}

EAPI void 
e_ilist_unselect(Evas_Object *obj) 
{
   Evas_List *l = NULL;
   const char *stacking, *selectraise;

   API_ENTRY return;

   if (!sd->items) return;
   if (sd->selected < 0) return;
   for (l = sd->items; l; l = l->next) 
     {
	E_Ilist_Item *si = NULL;

	if (!(si = l->data)) continue;
	if (!si->selected) continue;
	edje_object_signal_emit(si->o_base, "e,state,unselected", "e");
	si->selected = 0;
	stacking = edje_object_data_get(si->o_base, "stacking");
	selectraise = edje_object_data_get(si->o_base, "selectraise");
	if ((selectraise) && (!strcmp(selectraise, "on")))
	  {
	     if ((stacking) && (!strcmp(stacking, "below")))
	       evas_object_lower(si->o_base);
	  }
     }
   sd->selected = -1;
}

EAPI void 
e_ilist_selected_set(Evas_Object *obj, int n) 
{
   E_Ilist_Item *si = NULL;
   Evas_List *l = NULL;
   int i;
   const char *stacking, *selectraise;

   API_ENTRY return;
   if (!sd->items) return;

   i = evas_list_count(sd->items);
   if (n >= i) n = i - 1;
   else if (n < 0) n = 0;

   for (l = sd->items; l; l = l->next) 
     {
	if (!(si = l->data)) continue;
	if ((!si->selected) || (si->header)) continue;
	edje_object_signal_emit(si->o_base, "e,state,unselected", "e");
	si->selected = 0;
	stacking = edje_object_data_get(si->o_base, "stacking");
	selectraise = edje_object_data_get(si->o_base, "selectraise");
	if ((selectraise) && (!strcmp(selectraise, "on")))
	  {
	     if ((stacking) && (!strcmp(stacking, "below")))
	       evas_object_lower(si->o_base);
	  }
     }
   sd->selected = -1;
   if (!(si = evas_list_nth(sd->items, n))) return;

   /* NB: Remove this if headers ever become selectable */
   if (si->header) return;

   si->selected = 1;
   selectraise = edje_object_data_get(si->o_base, "selectraise");
   if ((selectraise) && (!strcmp(selectraise, "on")))
     evas_object_raise(si->o_base);
   edje_object_signal_emit(si->o_base, "e,state,selected", "e");
   sd->selected = n;
   if (si->func_hilight) si->func_hilight(si->data, si->data2);
   if (sd->selector) return;
   if (!sd->on_hold)
     {
	if (si->func) si->func(si->data, si->data2);
     }
}

EAPI int 
e_ilist_selected_get(Evas_Object *obj) 
{
   Evas_List *l = NULL;
   int i, j;

   API_ENTRY return -1;
   if (!sd->items) return -1;
   if (!sd->multi_select)
     return sd->selected;
   else
     {
	j = -1;
	for (i = 0, l = sd->items; l; l = l->next, i++) 
	  {
	     E_Ilist_Item *li = NULL;

	     if (!(li = l->data)) continue;
	     if (li->selected) j = i;
	  }
	return j;
     }
}

EAPI const char *
e_ilist_selected_label_get(Evas_Object *obj) 
{
   E_Ilist_Item *si = NULL;

   API_ENTRY return NULL;
   if (!sd->items) return NULL;
   if (sd->multi_select) return NULL;
   if (sd->selected < 0) return NULL;
   si = evas_list_nth(sd->items, sd->selected);
   if (si) return edje_object_part_text_get(si->o_base, "e.text.label");
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
   si = evas_list_nth(sd->items, sd->selected);
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
   si = evas_list_nth(sd->items, sd->selected);
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
   si = evas_list_nth(sd->items, sd->selected);
   if (si) return si->o_icon;
   return NULL;
}

EAPI void 
e_ilist_selected_geometry_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y, Evas_Coord *w, Evas_Coord *h) 
{
   E_Ilist_Item *si = NULL;

   API_ENTRY return;
   if (!sd->items) return;
   if (sd->selected < 0) return;
   if (!(si = evas_list_nth(sd->items, sd->selected))) return;
   evas_object_geometry_get(si->o_base, x, y, w, h);
   *x -= sd->x;
   *y -= sd->y;
}

EAPI int 
e_ilist_selected_count_get(Evas_Object *obj) 
{
   Evas_List *l = NULL;
   int count = 0;

   API_ENTRY return 0;
   if (!sd->items) return 0;
   for (l = sd->items; l; l = l->next) 
     {
	E_Ilist_Item *si = NULL;

	if (!(si = l->data)) continue;
	if (si->selected) count++;
     }
   return count;
}

EAPI void 
e_ilist_remove_num(Evas_Object *obj, int n) 
{
   E_Ilist_Item *si = NULL;

   API_ENTRY return;
   if (!sd->items) return;
   if (!(si = evas_list_nth(sd->items, n))) return;
   sd->items = evas_list_remove(sd->items, si);
   if (sd->selected == n) sd->selected = -1;
   if (si->o_icon) evas_object_del(si->o_icon);
   evas_object_del(si->o_base);
   E_FREE(si);
}

EAPI const char *
e_ilist_nth_label_get(Evas_Object *obj, int n) 
{
   E_Ilist_Item *si = NULL;

   API_ENTRY return NULL;
   if (!sd->items) return NULL;
   si = evas_list_nth(sd->items, n);
   if (si) return edje_object_part_text_get(si->o_base, "e.text.label");
   return NULL;
}

EAPI void 
e_ilist_nth_label_set(Evas_Object *obj, int n, const char *label) 
{
   E_Ilist_Item *si = NULL;

   /* check for a NULL label first...simplier, faster check then doing
    * API_ENTRY check first */
   if (!label) return;
   API_ENTRY return;
   if (!sd->items) return;
   si = evas_list_nth(sd->items, n);
   if (si) edje_object_part_text_set(si->o_base, "e.text.label", label);
}

EAPI Evas_Object *
e_ilist_nth_icon_get(Evas_Object *obj, int n) 
{
   E_Ilist_Item *si = NULL;

   API_ENTRY return NULL;
   if (!sd->items) return NULL;
   si = evas_list_nth(sd->items, n);
   if (si) return si->o_icon;
   return NULL;
}

EAPI void 
e_ilist_nth_icon_set(Evas_Object *obj, int n, Evas_Object *icon) 
{
   E_Ilist_Item *si = NULL;

   API_ENTRY return;
   if (!sd->items) return;
   if (!(si = evas_list_nth(sd->items, n))) return;
   if (si->o_icon) 
     {
	edje_object_part_unswallow(si->o_base, si->o_icon);
	evas_object_del(si->o_icon);
     }
   si->o_icon = icon;
   if (si->o_icon) 
     {
	edje_extern_object_min_size_set(si->o_icon, sd->iw, sd->ih);
	edje_object_part_swallow(si->o_base, "e.swallow.icon", si->o_icon);
	evas_object_show(si->o_icon);
     }
}

EAPI int 
e_ilist_nth_is_header(Evas_Object *obj, int n) 
{
   E_Ilist_Item *si = NULL;

   API_ENTRY return 0;
   if (!sd->items) return 0;
   si = evas_list_nth(sd->items, n);
   if (si) return si->header;
   return 0;
}

EAPI void
e_ilist_nth_geometry_get(Evas_Object *obj, int n, Evas_Coord *x, Evas_Coord *y, Evas_Coord *w, Evas_Coord *h)
{
  E_Ilist_Item *si = NULL;

  API_ENTRY return;
  if (!sd->items) return;
  if (!(si = evas_list_nth(sd->items, n))) return;
  evas_object_geometry_get(si->o_base, x, y, w, h);
  *x -= sd->x;
  *y -= sd->y;
}

EAPI void 
e_ilist_icon_size_set(Evas_Object *obj, Evas_Coord w, Evas_Coord h) 
{
   Evas_List *l = NULL;

   API_ENTRY return;
   if ((sd->iw == w) && (sd->ih == h)) return;
   sd->iw = w;
   sd->ih = h;
   for (l = sd->items; l; l = l->next) 
     {
	E_Ilist_Item *si = NULL;
	Evas_Coord mw = 0, mh = 0;

	if (!(si = l->data)) continue;
	if (!si->o_icon) continue;
	edje_extern_object_min_size_set(si->o_icon, w, h);
	edje_object_part_swallow(si->o_base, "e.swallow.icon", si->o_icon);
	edje_object_size_min_calc(si->o_base, &mw, &mh);
	e_box_pack_options_set(si->o_icon, 1, 1, 1, 0, 0.5, 0.5, 
			       mw, mh, 99999, 99999);
     }
}

EAPI Evas_List *
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
   const char *stacking, *selectraise;

   API_ENTRY return;
   if ((!sd->items) || (!sd->multi_select)) return;

   i = evas_list_count(sd->items);
   if (n >= i) n = i - 1;
   else if (n < 0) n = 0;

   if (!(si = evas_list_nth(sd->items, n))) return;
   sd->selected = n;
   if (si->selected) 
     {
	edje_object_signal_emit(si->o_base, "e,state,unselected", "e");
	si->selected = 0;
	stacking = edje_object_data_get(si->o_base, "stacking");
	selectraise = edje_object_data_get(si->o_base, "selectraise");
	if ((selectraise) && (!strcmp(selectraise, "on")))
	  {
	     if ((stacking) && (!strcmp(stacking, "below")))
	       evas_object_lower(si->o_base);
	  }
	if (si->func_hilight) si->func_hilight(si->data, si->data2);
	if (sd->selector) return;
	if (!sd->on_hold)
	  {
	     if (si->func) si->func(si->data, si->data2);
	  }
	return;
     }
   si->selected = 1;
   if ((selectraise) && (!strcmp(selectraise, "on")))
     evas_object_raise(si->o_base);
   edje_object_signal_emit(si->o_base, "e,state,selected", "e");
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
   E_Ilist_Item *si = NULL;
   int i, j, dir;

   API_ENTRY return;
   if ((!sd->items) || (!sd->multi_select)) return;

   i = evas_list_count(sd->items);
   if (n >= i) n = i - 1;
   else if (n < 0) n = 0;

   if (n < sd->selected) dir = 0;
   else dir = 1;

   if (!(si = evas_list_nth(sd->items, n))) return;
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

/* SMART FUNCTIONS */
static void 
_e_smart_init(void) 
{
   if (_e_smart) return;
     {
	static const Evas_Smart_Class sc =
	  {
	     SMART_NAME,
	       EVAS_SMART_CLASS_VERSION,
	       _e_smart_add,
	       _e_smart_del, 
	       _e_smart_move,
	       _e_smart_resize,
	       _e_smart_show,
	       _e_smart_hide,
	       _e_smart_color_set, 
	       _e_smart_clip_set,
	       _e_smart_clip_unset,
	       NULL,
	       NULL
	  };
        _e_smart = evas_smart_class_new(&sc);
     }
}

static void 
_e_smart_add(Evas_Object *obj) 
{
   E_Smart_Data *sd;

   sd = calloc(1, sizeof(E_Smart_Data));
   if (!sd) return;
   evas_object_smart_data_set(obj, sd);

   sd->o_smart = obj;
   sd->x = sd->y = sd->w = sd->h = 0;
   sd->iw = sd->ih = 24;
   sd->selected = -1;
   sd->multi_select = 0;

   sd->o_box = e_box_add(evas_object_evas_get(obj));
   e_box_align_set(sd->o_box, 0.0, 0.0);
   e_box_homogenous_set(sd->o_box, 0);
   evas_object_smart_member_add(sd->o_box, obj);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_KEY_DOWN, 
				  _e_smart_event_key_down, sd);
   evas_object_propagate_events_set(obj, 0);
}

static void 
_e_smart_del(Evas_Object *obj) 
{
   INTERNAL_ENTRY;
   e_ilist_clear(obj);
   evas_object_del(sd->o_box);
   free(sd);
}

static void 
_e_smart_show(Evas_Object *obj) 
{
   INTERNAL_ENTRY;
   evas_object_show(sd->o_box);
}

static void 
_e_smart_hide(Evas_Object *obj) 
{
   INTERNAL_ENTRY;
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
   evas_object_color_set(sd->o_box, r, g, b, a);
}

static void 
_e_smart_clip_set(Evas_Object *obj, Evas_Object *clip) 
{
   INTERNAL_ENTRY;
   evas_object_clip_set(sd->o_box, clip);
}

static void 
_e_smart_clip_unset(Evas_Object *obj) 
{
   INTERNAL_ENTRY;
   evas_object_clip_unset(sd->o_box);
}

static void 
_e_smart_reconfigure(E_Smart_Data *sd) 
{
   evas_object_move(sd->o_box, sd->x, sd->y);
   evas_object_resize(sd->o_box, sd->w, sd->h);
}

static void 
_e_smart_event_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event_info) 
{
   E_Smart_Data *sd;
   Evas_Event_Mouse_Down *ev;
   E_Ilist_Item *si;
   Evas_List *l = NULL;
   int i;

   ev = event_info;
   si = data;
   sd = si->sd;

   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) sd->on_hold = 1;
   else sd->on_hold = 0;

   /* NB: Remove if headers ever become selectable */
   if (si->header) return;

   if (!sd->items) return;
   for (i = 0, l = sd->items; l; l = l->next, i++) 
     {
	if (l->data == si) 
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
     }

   if (ev->flags & EVAS_BUTTON_DOUBLE_CLICK)
     evas_object_smart_callback_call(sd->o_smart, "selected", NULL);
}

static void 
_e_smart_event_mouse_up(void *data, Evas *evas, Evas_Object *obj, void *event_info) 
{
   E_Smart_Data *sd;
   Evas_Event_Mouse_Up *ev;
   E_Ilist_Item *si;

   ev = event_info;
   si = data;
   sd = si->sd;

   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) sd->on_hold = 1;
   else sd->on_hold = 0;

   /* NB: Remove if headers ever become selectable */
   if (si->header) return;

   if ((!sd->items) || (!sd->selector)) return;
   if (!(si = evas_list_nth(sd->items, sd->selected))) return;
   if (sd->on_hold)
     {
	sd->on_hold = 0;
	return;
     }
   if (si->func) si->func(si->data, si->data2);
}

static void 
_e_smart_event_key_down(void *data, Evas *evas, Evas_Object *obj, void *event_info) 
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
	     si = evas_list_nth(sd->items, n);
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
	     if (n == (evas_list_count(sd->items) - 1))
	       {
		  n = ns;
		  break;
	       }
	     ++n;
	     si = evas_list_nth(sd->items, n);
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
	     if (n == (evas_list_count(sd->items) - 1))
	       {
		  n = ns;
		  break;
	       }
	     ++n;
	     si = evas_list_nth(sd->items, n);
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
	n = evas_list_count(sd->items);
	do
	  {
	     if (n == 0)
	       {
		  n = ns;
		  break;
	       }
	     --n;
	     si = evas_list_nth(sd->items, n);
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
	    (!strcmp(ev->keyname, "space")))
     {
	if (!sd->on_hold)
	  {
	     si = evas_list_nth(sd->items, sd->selected);
	     if (si)
	       {
		  if (si->func) si->func(si->data, si->data2);
	       }
	  }
     }   
   sd->on_hold = 0;
}
