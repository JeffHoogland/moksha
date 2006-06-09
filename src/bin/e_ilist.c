/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

#define SMART_NAME "e_ilist"
#define API_ENTRY E_Smart_Data *sd; sd = evas_object_smart_data_get(obj); if ((!obj) || (!sd) || (evas_object_type_get(obj) && strcmp(evas_object_type_get(obj), SMART_NAME)))
#define INTERNAL_ENTRY E_Smart_Data *sd; sd = evas_object_smart_data_get(obj); if (!sd) return;
typedef struct _E_Smart_Data E_Smart_Data;
typedef struct _E_Smart_Item E_Smart_Item;

struct _E_Smart_Data
{ 
   Evas_Coord   x, y, w, h;
   
   Evas_Object   *smart_obj;
   Evas_Object   *box_obj;
   Evas_List     *items;
   int            selected;
   Evas_Coord     icon_w, icon_h;
   unsigned char  selector : 1;
};

struct _E_Smart_Item
{
   E_Smart_Data  *sd;
   Evas_Object   *base_obj;
   Evas_Object   *icon_obj;
   void         (*func) (void *data, void *data2);
   void         (*func_hilight) (void *data, void *data2);
   void          *data;
   void          *data2;
   unsigned char  header : 1;
};

/* local subsystem functions */
static void _e_smart_event_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_smart_event_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_smart_event_key_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_smart_reconfigure(E_Smart_Data *sd);
static void _e_smart_add(Evas_Object *obj);
static void _e_smart_del(Evas_Object *obj);
static void _e_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _e_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static void _e_smart_show(Evas_Object *obj);
static void _e_smart_hide(Evas_Object *obj);
static void _e_smart_color_set(Evas_Object *obj, int r, int g, int b, int a);
static void _e_smart_clip_set(Evas_Object *obj, Evas_Object * clip);
static void _e_smart_clip_unset(Evas_Object *obj);
static void _e_smart_init(void);

/* local subsystem globals */
static Evas_Smart *_e_smart = NULL;

/* externally accessible functions */
EAPI Evas_Object *
e_ilist_add(Evas *evas)
{
   _e_smart_init();
   return evas_object_smart_add(evas, _e_smart);
}

EAPI void
e_ilist_icon_size_set(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   Evas_List *l;
   
   API_ENTRY return;
   if ((sd->icon_w == w) && (sd->icon_h == h)) return;
   sd->icon_w = w;
   sd->icon_h = h;
   for (l = sd->items; l; l = l->next)
     {
	E_Smart_Item *si;
	
	si = l->data;
	if (si->icon_obj)
	  {
	     Evas_Coord mw = 0, mh = 0;
	     
	     edje_extern_object_min_size_set(si->icon_obj, sd->icon_w, sd->icon_h);
	     edje_object_part_swallow(si->base_obj, "icon_swallow", si->icon_obj);
	     edje_object_size_min_calc(si->base_obj, &mw, &mh);
	     e_box_pack_options_set(si->icon_obj,
				    1, 1, /* fill */
				    1, 0, /* expand */
				    0.5, 0.5, /* align */
				    mw, mh, /* min */
				    99999, 99999 /* max */
				    );
	  }
     }
}

EAPI void
e_ilist_append(Evas_Object *obj, Evas_Object *icon, const char *label, int header, void (*func) (void *data, void *data2), void (*func_hilight) (void *data, void *data2), void *data, void *data2)
{
   E_Smart_Item *si;
   Evas_Coord mw = 0, mh = 0;
   
   API_ENTRY return;
   si = E_NEW(E_Smart_Item, 1);
   si->sd = sd;
   si->base_obj = edje_object_add(evas_object_evas_get(sd->smart_obj));
   
   if (header)
     e_theme_edje_object_set(si->base_obj, "base/theme/widgets",
			     "widgets/ilist_header");
   else if (evas_list_count(sd->items) & 0x1)
     e_theme_edje_object_set(si->base_obj, "base/theme/widgets",
			     "widgets/ilist_odd");
   else
     e_theme_edje_object_set(si->base_obj, "base/theme/widgets",
			     "widgets/ilist");
   edje_object_part_text_set(si->base_obj, "label", label);
   si->icon_obj = icon;
   if (si->icon_obj)
     {
	edje_extern_object_min_size_set(si->icon_obj, sd->icon_w, sd->icon_h);
	edje_object_part_swallow(si->base_obj, "icon_swallow", si->icon_obj);
	evas_object_show(si->icon_obj);
     }
   si->func = func; 
   si->func_hilight = func_hilight;
   si->data = data;
   si->data2 = data2;
   si->header = header;
   sd->items = evas_list_append(sd->items, si);
   edje_object_size_min_calc(si->base_obj, &mw, &mh);
   e_box_pack_end(sd->box_obj, si->base_obj);
   e_box_pack_options_set(si->base_obj,
			  1, 1, /* fill */
			  1, 1, /* expand */
			  0.5, 0.5, /* align */
			  mw, mh, /* min */
			  99999, 99999 /* max */
			  );
   evas_object_lower(si->base_obj);
   evas_object_event_callback_add(si->base_obj, EVAS_CALLBACK_MOUSE_DOWN, _e_smart_event_mouse_down, si);
   evas_object_event_callback_add(si->base_obj, EVAS_CALLBACK_MOUSE_UP, _e_smart_event_mouse_up, si);
   evas_object_show(si->base_obj);
}

EAPI void
e_ilist_selected_set(Evas_Object *obj, int n)
{
   E_Smart_Item *si;
   
   API_ENTRY return;
   if (!sd->items) return;
   if (n >= evas_list_count(sd->items)) n = evas_list_count(sd->items) - 1;
   else if (n < 0) n = 0;
   if (sd->selected == n) return;
   si = evas_list_nth(sd->items, sd->selected);
   if (si) edje_object_signal_emit(si->base_obj, "passive", "");
   sd->selected = n;
   si = evas_list_nth(sd->items, sd->selected);
   if (si)
     {
	evas_object_raise(si->base_obj);
	edje_object_signal_emit(si->base_obj, "active", "");
	if (si->func_hilight) si->func_hilight(si->data, si->data2);
	if (!sd->selector)
	  {
	     if (si->func) si->func(si->data, si->data2);
	  }
     }
}

EAPI int
e_ilist_selected_get(Evas_Object *obj)
{
   API_ENTRY return -1;
   if (!sd->items) return -1;
   return sd->selected;
}

EAPI const char *
e_ilist_selected_label_get(Evas_Object *obj)
{
   E_Smart_Item *si;
   
   API_ENTRY return NULL;
   if (!sd->items) return NULL;
   si = evas_list_nth(sd->items, sd->selected);
   if (si) return edje_object_part_text_get(si->base_obj, "label");
   return NULL;
}

EAPI Evas_Object *
e_ilist_selected_icon_get(Evas_Object *obj)
{
   E_Smart_Item *si;
   
   API_ENTRY return NULL;
   if (!sd->items) return NULL;
   si = evas_list_nth(sd->items, sd->selected);
   if (si) return si->icon_obj;
   return NULL;
}

EAPI void *
e_ilist_selected_data_get(Evas_Object *obj)
{
   E_Smart_Item *si;
   
   API_ENTRY return NULL;
   if (!sd->items) return NULL;
   si = evas_list_nth(sd->items, sd->selected);
   if (si) return si->data;
   return NULL;
}

EAPI void *
e_ilist_selected_data2_get(Evas_Object *obj)
{
   E_Smart_Item *si;
   
   API_ENTRY return NULL;
   if (!sd->items) return NULL;
   si = evas_list_nth(sd->items, sd->selected);
   if (si) return si->data2;
   return NULL;
}

EAPI void
e_ilist_selected_geometry_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y, Evas_Coord *w, Evas_Coord *h)
{
   E_Smart_Item *si;
   
   API_ENTRY return;
   si = evas_list_nth(sd->items, sd->selected);
   if (si)
     {
	evas_object_geometry_get(si->base_obj, x, y, w, h);
	*x -= sd->x;
	*y -= sd->y;
     }
}

EAPI void
e_ilist_min_size_get(Evas_Object *obj, Evas_Coord *w, Evas_Coord *h)
{
   API_ENTRY return;
   e_box_min_size_get(sd->box_obj, w, h);
}

EAPI void
e_ilist_selector_set(Evas_Object *obj, int selector)
{
   API_ENTRY return;
   sd->selector = selector;
}

EAPI int
e_ilist_selector_get(Evas_Object *obj)
{
   API_ENTRY return 0;
   return sd->selector;
}

EAPI void
e_ilist_remove_num(Evas_Object *obj, int n)
{
   E_Smart_Item *si;
   
   API_ENTRY return;
   if (!sd->items) return;
   si = evas_list_nth(sd->items, n);
   if (si) 
   {
      sd->items = evas_list_remove(sd->items, si);
      if (e_ilist_selected_get(obj) == n)
	sd->selected = -1;
      if (si->icon_obj) evas_object_del(si->icon_obj);
      evas_object_del(si->base_obj);
      free(si);  
   }
}

EAPI void
e_ilist_remove_label(Evas_Object *obj, const char *label)
{
   E_Smart_Item *si;
   Evas_List *l;
   int i;
   
   API_ENTRY return;
   if (!sd->items) return;
   if (!label) return;
   for (i = 0, l = sd->items; l; l = l->next, i++) 
     {
	si = l->data;
	if (si) 
	  {
	     char *t;
	     t = strdup(edje_object_part_text_get(si->base_obj, "label"));
	     if (!strcmp(t, label)) 
	       {
		  if (si->icon_obj) evas_object_del(si->icon_obj);
		  evas_object_del(si->base_obj);
		  sd->items = evas_list_remove(sd->items, si);
		  free(si);
		  break;
	       }
	     free(t);
	  }
     }
}

EAPI const char *
e_ilist_nth_label_get(Evas_Object *obj, int n)
{ 
   E_Smart_Item *si;
   
   API_ENTRY return NULL;
   if (!sd->items) return NULL;
   si = evas_list_nth(sd->items, n);
   if (si) return edje_object_part_text_get(si->base_obj, "label");
   return NULL;
}

EAPI void
e_ilist_nth_label_set(Evas_Object *obj, int n, const char *label)
{ 
   E_Smart_Item *si;
   
   API_ENTRY return;
   if (!sd->items) return;
   si = evas_list_nth(sd->items, n);
   if (si) edje_object_part_text_set(si->base_obj, "label", label);
}

EAPI Evas_Object *
e_ilist_nth_icon_get(Evas_Object *obj, int n)
{ 
   E_Smart_Item *si;
   
   API_ENTRY return NULL;
   if (!sd->items) return NULL;
   si = evas_list_nth(sd->items, n);
   if (si) return si->icon_obj;
   return NULL;
}

EAPI void
e_ilist_nth_icon_set(Evas_Object *obj, int n, Evas_Object *icon)
{ 
   E_Smart_Item *si;
   
   API_ENTRY return;
   if (!sd->items) return;
   si = evas_list_nth(sd->items, n);
   if (si) 
     { 
	if (si->icon_obj) 
	  {
	     edje_object_part_unswallow(si->base_obj, si->icon_obj);
	     evas_object_hide(si->icon_obj);
	     evas_object_del(si->icon_obj);
	  }

	si->icon_obj = icon;
	if (si->icon_obj)
	  {
	     edje_extern_object_min_size_set(si->icon_obj, sd->icon_w, sd->icon_h);
	     edje_object_part_swallow(si->base_obj, "icon_swallow", si->icon_obj);
	     evas_object_show(si->icon_obj);
	  }
     }
}

EAPI int
e_ilist_count(Evas_Object *obj)
{
   API_ENTRY return 0;
   return evas_list_count(sd->items);
}

EAPI void
e_ilist_clear(Evas_Object *obj) 
{
   INTERNAL_ENTRY;
   while (sd->items)
     {
	E_Smart_Item *si;

	si = sd->items->data;
	sd->items = evas_list_remove_list(sd->items, sd->items);
	if (si->icon_obj) evas_object_del(si->icon_obj);
	evas_object_del(si->base_obj);
	free(si);
     }
   sd->selected = -1;
}

EAPI int
e_ilist_nth_is_header(Evas_Object *obj, int n) 
{
   E_Smart_Item *si;

   API_ENTRY return 0;
   if (!sd->items) return 0;
   si = evas_list_nth(sd->items, n);
   if (si) return si->header;
   return 0;
}

/* local subsystem functions */
static void 
_e_smart_event_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   E_Smart_Item *si;
   Evas_List *l;
   int i;
   
   si = data;
   ev = event_info;

   if (si->header) return;

   for (i = 0, l = si->sd->items; l; l = l->next, i++)
     {
	if (l->data == si)
	  {
	     e_ilist_selected_set(si->sd->smart_obj, i);
	     break;
	  }
     }
}

static void 
_e_smart_event_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Up *ev;
   E_Smart_Item *si;
   
   si = data;
   ev = event_info;

   if (si->header) return;

   if (si->sd->selector)
     {
	si = evas_list_nth(si->sd->items, si->sd->selected);
	if (si)
	  {
	     if (si->func) si->func(si->data, si->data2);
	  }
     }
}

static void
_e_smart_event_key_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Key_Down *ev;
   E_Smart_Data *sd;
   
   sd = data;
   ev = event_info;
   if ((!strcmp(ev->keyname, "Up")) ||
       (!strcmp(ev->keyname, "KP_Up")))
     {
	int n, ns;
	E_Smart_Item *si;
	   
	ns = e_ilist_selected_get(sd->smart_obj);
	n = ns;
	do
	  {
	     if (n == 0)
	       {
		  n = ns;
		  break;
	       }
	     -- n;
	     si = evas_list_nth(sd->items, n);
	  }
	while ((si) && (si->header));
	if (n != ns) e_ilist_selected_set(sd->smart_obj, n);
     }
   else if ((!strcmp(ev->keyname, "Down")) ||
	    (!strcmp(ev->keyname, "KP_Down")))
     {
	int n, ns;
        E_Smart_Item *si;
	
	ns = e_ilist_selected_get(sd->smart_obj);
	n = ns;
	do
	  {
	     if (n == (evas_list_count(sd->items) - 1))
	       {
		  n = ns;
		  break;
	       }
	     ++ n;
	     si = evas_list_nth(sd->items, n);
	  }
	while ((si) && (si->header));
	if (n != ns) e_ilist_selected_set(sd->smart_obj, n);
     }
   else if ((!strcmp(ev->keyname, "Home")) ||
	    (!strcmp(ev->keyname, "KP_Home")))
     {
	int n, ns;
	E_Smart_Item *si;
	
	ns = e_ilist_selected_get(sd->smart_obj);
	n = -1;
	do
	  {
	     if (n == (evas_list_count(sd->items) - 1))
	       {
		  n = ns;
		  break;
	       }
	     ++ n;
	     si = evas_list_nth(sd->items, n);
	  }
	while ((si) && (si->header));
	if (n != ns) e_ilist_selected_set(sd->smart_obj, n);
     }
   else if ((!strcmp(ev->keyname, "End")) ||
	    (!strcmp(ev->keyname, "KP_End")))
     {
	int n, ns;
	E_Smart_Item *si;
	
	ns = e_ilist_selected_get(sd->smart_obj);
	n = evas_list_count(sd->items);
	do
	  {
	     if (n == 0)
	       {
		  n = ns;
		  break;
	       }
	     -- n;
	     si = evas_list_nth(sd->items, n);
	  }
	while ((si) && (si->header));
	if (n != ns) e_ilist_selected_set(sd->smart_obj, n);
     }
   else if ((!strcmp(ev->keyname, "Return")) ||
	    (!strcmp(ev->keyname, "KP_Enter")) ||
	    (!strcmp(ev->keyname, "space")))
     {
	E_Smart_Item *si;

	si = evas_list_nth(sd->items, sd->selected);
	if (si)
	  {
	     if (si->func) si->func(si->data, si->data2);
	  }
     }
}

static void
_e_smart_reconfigure(E_Smart_Data *sd)
{
   evas_object_move(sd->box_obj, sd->x, sd->y);
   evas_object_resize(sd->box_obj, sd->w, sd->h);
}

static void
_e_smart_add(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = calloc(1, sizeof(E_Smart_Data));
   if (!sd) return;
   evas_object_smart_data_set(obj, sd);
   
   sd->smart_obj = obj;
   sd->x = 0;
   sd->y = 0;
   sd->w = 0;
   sd->h = 0;
   
   sd->icon_w = 24;
   sd->icon_h = 24;
   
   sd->selected = -1;
   
   sd->box_obj = e_box_add(evas_object_evas_get(obj));
   e_box_align_set(sd->box_obj, 0.0, 0.0);
   e_box_homogenous_set(sd->box_obj, 0);
   evas_object_smart_member_add(sd->box_obj, obj);
   
   evas_object_event_callback_add(obj, EVAS_CALLBACK_KEY_DOWN, _e_smart_event_key_down, sd);
   evas_object_propagate_events_set(obj, 0);
}

static void
_e_smart_del(Evas_Object *obj)
{
   INTERNAL_ENTRY;
   while (sd->items)
     {
	E_Smart_Item *si;

	si = sd->items->data;
	sd->items = evas_list_remove_list(sd->items, sd->items);
	if (si->icon_obj) evas_object_del(si->icon_obj);
	evas_object_del(si->base_obj);
	free(si);
     }
   evas_object_del(sd->box_obj);
   free(sd);
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
_e_smart_show(Evas_Object *obj)
{
   INTERNAL_ENTRY;
   evas_object_show(sd->box_obj);
}

static void
_e_smart_hide(Evas_Object *obj)
{
   INTERNAL_ENTRY;
   evas_object_hide(sd->box_obj);
}

static void
_e_smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   INTERNAL_ENTRY;
   evas_object_color_set(sd->box_obj, r, g, b, a);
}

static void
_e_smart_clip_set(Evas_Object *obj, Evas_Object * clip)
{
   INTERNAL_ENTRY;
   evas_object_clip_set(sd->box_obj, clip);
}

static void
_e_smart_clip_unset(Evas_Object *obj)
{
   INTERNAL_ENTRY;
   evas_object_clip_unset(sd->box_obj);
}  

/* never need to touch this */

static void
_e_smart_init(void)
{
   if (_e_smart) return;
   _e_smart = evas_smart_new(SMART_NAME,
			     _e_smart_add,
			     _e_smart_del, 
			     NULL, NULL, NULL, NULL, NULL,
			     _e_smart_move,
			     _e_smart_resize,
			     _e_smart_show,
			     _e_smart_hide,
			     _e_smart_color_set,
			     _e_smart_clip_set,
			     _e_smart_clip_unset,
			     NULL);
}

