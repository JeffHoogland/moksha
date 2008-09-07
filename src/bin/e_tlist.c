/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */

/* FIXME: This should really be merged with ilist, but raster says not to hack ilist. */

#include "e.h"

#define SMART_NAME "e_tlist"
#define API_ENTRY E_Smart_Data *sd; sd = evas_object_smart_data_get(obj); if ((!obj) || (!sd) || (evas_object_type_get(obj) && strcmp(evas_object_type_get(obj), SMART_NAME)))
#define INTERNAL_ENTRY E_Smart_Data *sd; sd = evas_object_smart_data_get(obj); if (!sd) return;
typedef struct _E_Smart_Data E_Smart_Data;
typedef struct _E_Smart_Item E_Smart_Item;

struct _E_Smart_Data
{
   Evas_Coord x, y, w, h;

   Evas_Object *smart_obj;
   Evas_Object *box_obj;
   Evas_List *items;
   int selected;
   unsigned char selector : 1;
   unsigned char on_hold : 1;
};

struct _E_Smart_Item
{
   E_Smart_Data *sd;
   Evas_Object *base_obj;
   void (*func) (void *data, void *data2);
   void (*func_hilight) (void *data, void *data2);
   void *data, *data2;
   unsigned char markup:1;
};

/* local subsystem functions */
static void _e_tlist_append(Evas_Object * obj, const char *label,
                            void (*func) (void *data, void *data2),
                            void (*func_hilight) (void *data,
                                                  void *data2),
                            void *data, void *data2, int markup);
static void _e_smart_event_mouse_down(void *data, Evas * e,
					      Evas_Object * obj,
					      void *event_info);
static void _e_smart_event_mouse_up(void *data, Evas * e,
					    Evas_Object * obj,
					    void *event_info);
static void _e_smart_event_key_down(void *data, Evas * e,
					    Evas_Object * obj,
					    void *event_info);
static void _e_smart_reconfigure(E_Smart_Data * sd);
static void _e_smart_add(Evas_Object * obj);
static void _e_smart_del(Evas_Object * obj);
static void _e_smart_move(Evas_Object * obj, Evas_Coord x,
                          Evas_Coord y);
static void _e_smart_resize(Evas_Object * obj, Evas_Coord w,
                            Evas_Coord h);
static void _e_smart_show(Evas_Object * obj);
static void _e_smart_hide(Evas_Object * obj);
static void _e_smart_color_set(Evas_Object * obj, int r, int g, int b,
                               int a);
static void _e_smart_clip_set(Evas_Object * obj, Evas_Object * clip);
static void _e_smart_clip_unset(Evas_Object * obj);
static void _e_smart_init(void);

/* local subsystem globals */
static Evas_Smart *_e_smart = NULL;

/* externally accessible functions */
EAPI Evas_Object *
e_tlist_add(Evas *evas)
{
   _e_smart_init();
   return evas_object_smart_add(evas, _e_smart);
}

EAPI void
e_tlist_append(Evas_Object *obj, const char *label,
	       void (*func) (void *data, void *data2),
	       void (*func_hilight) (void *data, void *data2), void *data,
	       void *data2)
{
   _e_tlist_append(obj, label, func, func_hilight, data, data2, 0);
}

EAPI void
e_tlist_markup_append(Evas_Object *obj, const char *label,
		      void (*func) (void *data, void *data2),
		      void (*func_hilight) (void *data, void *data2),
		      void *data, void *data2)
{
   _e_tlist_append(obj, label, func, func_hilight, data, data2, 1);
}

EAPI void
e_tlist_selected_set(Evas_Object *obj, int n)
{
   E_Smart_Item *si;

   API_ENTRY return;
   if (!sd->items) return;
   if (n >= evas_list_count(sd->items))
     n = evas_list_count(sd->items) - 1;
   else if (n < 0)
     n = 0;
   if (sd->selected == n) return;
   si = evas_list_nth(sd->items, sd->selected);
   if (si)
     edje_object_signal_emit(si->base_obj, "e,state,unselected", "e");
   sd->selected = n;
   si = evas_list_nth(sd->items, sd->selected);
   if (si)
     {
	evas_object_raise(si->base_obj);
	edje_object_signal_emit(si->base_obj, "e,state,selected", "e");
	if (si->func_hilight) si->func_hilight(si->data, si->data2);
	if (!sd->selector)
	  {
	     if (!sd->on_hold)
	       {
		  if (si->func) si->func(si->data, si->data2);
	       }
	  }
     }
}

EAPI int
e_tlist_selected_get(Evas_Object *obj)
{
   API_ENTRY return -1;
   if (!sd->items) return -1;
   return sd->selected;
}

EAPI const char *
e_tlist_selected_label_get(Evas_Object *obj)
{
   E_Smart_Item *si;

   API_ENTRY return NULL;
   if (!sd->items) return NULL;
   si = evas_list_nth(sd->items, sd->selected);
   if (si)
     {
	if (si->markup)
          return edje_object_part_text_get(si->base_obj, "e.textblock.label");
	else
          return edje_object_part_text_get(si->base_obj, "e.text.label");
     }
   return NULL;
}

EAPI void *
e_tlist_selected_data_get(Evas_Object *obj)
{
   E_Smart_Item *si;

   API_ENTRY return NULL;
   if (!sd->items) return NULL;
   si = evas_list_nth(sd->items, sd->selected);
   if (si) return si->data;
   return NULL;
}

EAPI void *
e_tlist_selected_data2_get(Evas_Object *obj)
{
   E_Smart_Item *si;

   API_ENTRY return NULL;
   if (!sd->items) return NULL;
   si = evas_list_nth(sd->items, sd->selected);
   if (si) return si->data2;
   return NULL;
}

EAPI void
e_tlist_selected_geometry_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y,
			      Evas_Coord *w, Evas_Coord *h)
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
e_tlist_min_size_get(Evas_Object *obj, Evas_Coord *w, Evas_Coord *h)
{
   API_ENTRY return;
   e_box_min_size_get(sd->box_obj, w, h);
}

EAPI void
e_tlist_selector_set(Evas_Object *obj, int selector)
{
   API_ENTRY return;
   sd->selector = selector;
}

EAPI int
e_tlist_selector_get(Evas_Object *obj)
{
   API_ENTRY return 0;
   return sd->selector;
}

EAPI void
e_tlist_remove_num(Evas_Object *obj, int n)
{
   E_Smart_Item *si;

   API_ENTRY return;
   if (!sd->items) return;
   si = evas_list_nth(sd->items, n);
   if (si)
     {
	sd->items = evas_list_remove(sd->items, si);
	if (e_tlist_selected_get(obj) == n) sd->selected = -1;
	evas_object_del(si->base_obj);
	free(si);
     }
}

EAPI void
e_tlist_remove_label(Evas_Object *obj, const char *label)
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

	     if (si->markup)
               t = strdup(edje_object_part_text_get
                          (si->base_obj, "e.textblock.label"));
	     else
               t = strdup(edje_object_part_text_get(si->base_obj, 
                                                    "e.text.label"));
	     if (!strcmp(t, label))
	       {
		  evas_object_del(si->base_obj);
		  sd->items = evas_list_remove(sd->items, si);
		  free(si);
		  break;
	       }
	     free(t);
	  }
     }
}

EAPI int
e_tlist_count(Evas_Object *obj)
{
   API_ENTRY return 0;
   return evas_list_count(sd->items);
}

EAPI void
e_tlist_clear(Evas_Object *obj) 
{
   INTERNAL_ENTRY;
   while (sd->items)
     {
	E_Smart_Item *si;

	si = sd->items->data;
	sd->items = evas_list_remove_list(sd->items, sd->items);
	evas_object_del(si->base_obj);
	free(si);
     }
   sd->selected = -1;
}

/* local subsystem functions */
static void
_e_tlist_append(Evas_Object *obj, const char *label,
		void (*func) (void *data, void *data2),
		void (*func_hilight) (void *data, void *data2), void *data,
		void *data2, int markup)
{
   E_Smart_Item *si;
   Evas_Coord mw = 0, mh = 0;

   API_ENTRY return;
   si = E_NEW(E_Smart_Item, 1);
   si->sd = sd;
   si->markup = markup;
   si->base_obj = edje_object_add(evas_object_evas_get(sd->smart_obj));

   /* FIXME: Use a color class or something to avoid duplicating the theme with only the background piccie being different. */
   if (evas_list_count(sd->items) & 0x1)
     e_theme_edje_object_set(si->base_obj, "base/theme/widgets",
                             "e/widgets/tlist_odd");
   else
     e_theme_edje_object_set(si->base_obj, "base/theme/widgets",
                             "e/widgets/tlist");
   if (si->markup)
     edje_object_part_text_set(si->base_obj, "e.textblock.label", label);
   else
     edje_object_part_text_set(si->base_obj, "e.text.label", label);
   si->func = func;
   si->func_hilight = func_hilight;
   si->data = data;
   si->data2 = data2;
   sd->items = evas_list_append(sd->items, si);
   edje_object_size_min_calc(si->base_obj, &mw, &mh);
   e_box_pack_end(sd->box_obj, si->base_obj);
   e_box_pack_options_set(si->base_obj, 1, 1,	/* fill */
			  1, 1,	/* expand */
			  0.5, 0.5,	/* align */
			  mw, mh,	/* min */
			  99999, 99999	/* max */
                          );
   evas_object_lower(si->base_obj);
   evas_object_event_callback_add(si->base_obj, EVAS_CALLBACK_MOUSE_DOWN,
				  _e_smart_event_mouse_down, si);
   evas_object_event_callback_add(si->base_obj, EVAS_CALLBACK_MOUSE_UP,
				  _e_smart_event_mouse_up, si);
   evas_object_show(si->base_obj);
}

static void
_e_smart_event_mouse_down(void *data, Evas *e, Evas_Object *obj,
			  void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   E_Smart_Item *si;
   Evas_List *l;
   int i;

   si = data;
   ev = event_info;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) si->sd->on_hold = 1;
   else si->sd->on_hold = 0;

   for (i = 0, l = si->sd->items; l; l = l->next, i++)
     {
	if (l->data == si)
	  {
	     e_tlist_selected_set(si->sd->smart_obj, i);
	     break;
	  }
     }
}

static void
_e_smart_event_mouse_up(void *data, Evas *e, Evas_Object *obj,
			void *event_info)
{
   Evas_Event_Mouse_Up *ev;
   E_Smart_Item *si;

   si = data;
   ev = event_info;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) si->sd->on_hold = 1;
   else si->sd->on_hold = 0;

   if (si->sd->selector)
     {
	si = evas_list_nth(si->sd->items, si->sd->selected);
	if (si)
	  {
	     if (!si->sd->on_hold)
	       {
		  if (si->func) si->func(si->data, si->data2);
	       }
	  }
     }
   si->sd->on_hold = 0;
}

static void
_e_smart_event_key_down(void *data, Evas *e, Evas_Object *obj,
			void *event_info)
{
   Evas_Event_Key_Down *ev;
   E_Smart_Data *sd;
   int n;

   sd = data;
   ev = event_info;
   if (!strcmp(ev->keyname, "Up"))
     {
	n = e_tlist_selected_get(sd->smart_obj);
	e_tlist_selected_set(sd->smart_obj, n - 1);
     }
   else if (!strcmp(ev->keyname, "Down"))
     {
	n = e_tlist_selected_get(sd->smart_obj);
	e_tlist_selected_set(sd->smart_obj, n + 1);
     }
   else if ((!strcmp(ev->keyname, "Return")) || 
            (!strcmp(ev->keyname, "space")))
     {
        if (!sd->on_hold)
	  {
	     E_Smart_Item *si;

	     si = evas_list_nth(sd->items, sd->selected);
	     if (si)
	       {
		  if (si->func) si->func(si->data, si->data2);
	       }
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
   sd->selected = -1;

   sd->box_obj = e_box_add(evas_object_evas_get(obj));
   e_box_align_set(sd->box_obj, 0.0, 0.0);
   e_box_homogenous_set(sd->box_obj, 0);
   evas_object_smart_member_add(sd->box_obj, obj);

   evas_object_event_callback_add(obj, EVAS_CALLBACK_KEY_DOWN,
				  _e_smart_event_key_down, sd);
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
_e_smart_clip_set(Evas_Object *obj, Evas_Object *clip)
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
   static const Evas_Smart_Class sc =
     {
        SMART_NAME,
          EVAS_SMART_CLASS_VERSION,
          _e_smart_add, _e_smart_del, _e_smart_move, _e_smart_resize,
          _e_smart_show, _e_smart_hide, _e_smart_color_set,
	_e_smart_clip_set, _e_smart_clip_unset, NULL, NULL
     };
   _e_smart = evas_smart_class_new(&sc);
}
