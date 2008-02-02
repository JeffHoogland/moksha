/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

#define SMART_NAME "e_slidecore"
#define API_ENTRY E_Smart_Data *sd; sd = evas_object_smart_data_get(obj); if ((!obj) || (!sd) || (evas_object_type_get(obj) && strcmp(evas_object_type_get(obj), SMART_NAME)))
#define INTERNAL_ENTRY E_Smart_Data *sd; sd = evas_object_smart_data_get(obj); if (!sd) return;
typedef struct _E_Smart_Data E_Smart_Data;
typedef struct _E_Smart_Item E_Smart_Item;

struct _E_Smart_Data
{ 
   Evas_Coord   x, y, w, h;
   
   Evas_Object *smart_obj;
   Evas_Object *event_obj;
   Evas_Object *o1, *o2;
   Evas_List *items;
   Evas_Coord dist, pos;
   int p1, p2, pn;
   unsigned char down : 1;
}; 

struct _E_Smart_Item
{
   const char *label;
   const char *icon;
   void (*func) (void *data);
   void *data;
};

/* local subsystem functions */
static void _e_smart_event_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_smart_event_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_smart_event_mouse_move(void *data, Evas *e, Evas_Object *obj, void *event_info);
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
e_slidecore_add(Evas *evas)
{
   _e_smart_init();
   return evas_object_smart_add(evas, _e_smart);
}

EAPI void
e_slidecore_item_distance_set(Evas_Object *obj, Evas_Coord dist)
{
   API_ENTRY return;
   if (dist < 1) dist = 1;
   sd->dist = dist;
   _e_smart_reconfigure(sd);
}

EAPI void
e_slidecore_item_add(Evas_Object *obj, const char *label, const char *icon,  void (*func) (void *data), void *data)
{
   E_Smart_Item *it;
   
   API_ENTRY return;
   it = calloc(1, sizeof(E_Smart_Item));
   if (!it) return;
   if (label) it->label = evas_stringshare_add(label);
   if (icon) it->icon = evas_stringshare_add(icon);
   it->func = func;
   it->data = data;
   sd->items = evas_list_append(sd->items, it);
   _e_smart_reconfigure(sd);
}

EAPI void
e_slidecore_jump(Evas_Object *obj, int num)
{
   API_ENTRY return;
   _e_smart_reconfigure(sd);
}


/* local subsystem functions */

static void
_e_smart_event_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   E_Smart_Data *sd;

   sd = data;
   ev = event_info;
   if (ev->button == 1) sd->down = 1;
}

static void
_e_smart_event_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   E_Smart_Data *sd;

   sd = data;
   ev = event_info;
   if (ev->button == 1) sd->down = 0;
}

static void
_e_smart_event_mouse_move(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Move *ev;
   E_Smart_Data *sd;

   sd = data;
   ev = event_info;
   if (!sd->down) return;
   sd->pos += ev->cur.canvas.x - ev->prev.canvas.x;
   _e_smart_reconfigure(sd);
}

static void
_e_smart_reconfigure(E_Smart_Data *sd)
{
   Evas_Coord dp, pos;
   int p1, p2, at, pl1, pl2, n;
   int r, g, b, a;
   
   evas_object_move(sd->event_obj, sd->x, sd->y);
   evas_object_resize(sd->event_obj, sd->w, sd->h);
   
   pos = sd->pos;
   n = evas_list_count(sd->items);
   while (pos < 0) pos += (sd->dist * n);
   p1 = pos / sd->dist;
   p2 = (pos + sd->dist) / sd->dist;
   dp = pos - (p1 * sd->dist);
   at = (dp * 255) / (sd->dist - 1);
   
   printf("? %i %i | %i : %i # %i\n", p1, p2, dp, at, pos);
   while ((p1 < 0) || (p2 < 0))
     {
	p1 += n;
	p2 += n;
	at += 255;
	dp += sd->dist;
     }
   printf("??? %i %i | %i : %i # %i\n", p1, p2, dp, at, pos);
   if ((sd->p1 != p1) || (sd->p2 != p2) || (sd->pn != n))
     {
	E_Smart_Item *it1, *it2;
	
	/* FIXME: delete old o1, o2, create new o1, o2 */
	sd->pn = n;
	if (n > 0)
	  {
	     sd->p1 = p1;
	     sd->p2 = p2;
	     if (sd->o1) evas_object_del(sd->o1);
	     if (sd->o2) evas_object_del(sd->o2);
	     sd->o1 = NULL;
	     sd->o2 = NULL;
	     pl1 = sd->p1 % n;
	     pl2 = sd->p2 % n;
	     printf("## %i -> %i | %i -> %i ||| %i\n", p1, pl1, p2, pl2, dp);
	     it1 = evas_list_nth(sd->items, pl1);
	     it2 = evas_list_nth(sd->items, pl2);
	     if (it1 && it2)
	       {
		  sd->o1 =  e_util_icon_theme_icon_add(it1->icon, "480x480", 
						       evas_object_evas_get(sd->smart_obj));
		  if (sd->o1)
		    {
		       evas_object_stack_below(sd->o1, sd->event_obj);
		       evas_object_pass_events_set(sd->o1, 1);
		       evas_object_smart_member_add(sd->o1, sd->smart_obj);
		       e_icon_fill_inside_set(sd->o1, 0);
		       evas_object_clip_set(sd->o1, evas_object_clip_get(sd->smart_obj));
		       evas_object_show(sd->o1);
		    }
		  sd->o2 =  e_util_icon_theme_icon_add(it2->icon, "480x480", 
						       evas_object_evas_get(sd->smart_obj));
		  if (sd->o2)
		    {
		       evas_object_stack_below(sd->o2, sd->event_obj);
		       evas_object_pass_events_set(sd->o2, 1);
		       evas_object_smart_member_add(sd->o2, sd->smart_obj);
		       e_icon_fill_inside_set(sd->o2, 0);
		       evas_object_clip_set(sd->o2, evas_object_clip_get(sd->smart_obj));
		       evas_object_show(sd->o2);
		    }
		  if (a < 128)
		    {
		       if (it1->func) it1->func(it1->data);
		    }
		  else
		    {
		       if (it2->func) it2->func(it2->data);
		    }
	       }
	  }
     }
   evas_object_color_get(sd->smart_obj, &r, &g, &b, &a);
   
   evas_object_move(sd->o1, sd->x - sd->dist + dp, sd->y);
   evas_object_resize(sd->o1, sd->w + sd->dist + sd->dist, sd->h);
   evas_object_color_set(sd->o1, r, g, b, a);
   
   evas_object_move(sd->o2, sd->x - sd->dist - sd->dist + dp, sd->y);
   evas_object_resize(sd->o2, sd->w + sd->dist + sd->dist, sd->h);
   evas_object_color_set(sd->o2, (r * at) / 255, (g * at) / 255, (b * at) / 255, (a * at) / 255);
}

static void
_e_smart_add(Evas_Object *obj)
{
   E_Smart_Data *sd;
   Evas_Object *o;
   
   sd = calloc(1, sizeof(E_Smart_Data));
   if (!sd) return;
   evas_object_smart_data_set(obj, sd);
   
   sd->smart_obj = obj;
   sd->x = 0;
   sd->y = 0;
   sd->w = 0;
   sd->h = 0;
   
   evas_object_propagate_events_set(obj, 0);

   sd->dist = 48;
   sd->pos = 0;
   sd->p1 = -1;
   sd->p2 = -1;
   
   o = evas_object_rectangle_add(evas_object_evas_get(obj));
   sd->event_obj = o;
   evas_object_color_set(o, 0, 0, 0, 0);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _e_smart_event_mouse_down, sd);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_UP, _e_smart_event_mouse_up, sd);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_MOVE, _e_smart_event_mouse_move, sd);
   evas_object_smart_member_add(o, obj);
   evas_object_repeat_events_set(o, 1);
}

static void
_e_smart_del(Evas_Object *obj)
{
   INTERNAL_ENTRY;
   evas_object_del(sd->event_obj);
   free(sd);
}

static void
_e_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   INTERNAL_ENTRY;
   sd->x = x;
   sd->y = y;
   _e_smart_reconfigure(sd);
}

static void
_e_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   INTERNAL_ENTRY;
   sd->w = w;
   sd->h = h;
   _e_smart_reconfigure(sd);
}

static void
_e_smart_show(Evas_Object *obj)
{
   INTERNAL_ENTRY;
   evas_object_show(sd->event_obj);
}

static void
_e_smart_hide(Evas_Object *obj)
{
   INTERNAL_ENTRY;
   evas_object_hide(sd->event_obj);
}

static void
_e_smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   INTERNAL_ENTRY;
   if (sd->o1) evas_object_color_set(sd->o1, r, g, b, a);
   if (sd->o2) evas_object_color_set(sd->o2, r, g, b, a);
}

static void
_e_smart_clip_set(Evas_Object *obj, Evas_Object * clip)
{
   INTERNAL_ENTRY;
   evas_object_clip_set(sd->event_obj, clip);
   if (sd->o1) evas_object_clip_set(sd->o1, clip);
   if (sd->o2) evas_object_clip_set(sd->o2, clip);
}

static void
_e_smart_clip_unset(Evas_Object *obj)
{
   INTERNAL_ENTRY;
   evas_object_clip_unset(sd->event_obj);
}  

/* never need to touch this */

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
	       NULL
	  };
	_e_smart = evas_smart_class_new(&sc);
     }
}

