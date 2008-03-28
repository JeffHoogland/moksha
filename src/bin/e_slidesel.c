/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

#define SMART_NAME "e_slidesel"
#define API_ENTRY E_Smart_Data *sd; sd = evas_object_smart_data_get(obj); if ((!obj) || (!sd) || (evas_object_type_get(obj) && strcmp(evas_object_type_get(obj), SMART_NAME)))
#define INTERNAL_ENTRY E_Smart_Data *sd; sd = evas_object_smart_data_get(obj); if (!sd) return;
typedef struct _E_Smart_Data E_Smart_Data;
typedef struct _E_Smart_Item E_Smart_Item;

struct _E_Smart_Data
{ 
   Evas_Coord   x, y, w, h;
   
   Evas_Object *smart_obj;
   Evas_Object *edje_obj;
   Evas_Object *event_obj;
   Evas_Object *slide_obj;
   Evas_List *items;
   Evas_Coord down_x, down_y;
   E_Smart_Item *cur;
   double down_time;
   unsigned char down : 1;
   unsigned char down_cancel : 1;
}; 

struct _E_Smart_Item
{
   E_Smart_Data *sd;
   const char *label;
   const char *icon;
   void (*func) (void *data);
   void *data;
};

/* local subsystem functions */
static void _e_smart_event_wheel(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_smart_event_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_smart_event_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_smart_event_mouse_move(void *data, Evas *e, Evas_Object *obj, void *event_info);
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

static void
_e_smart_label_change(void *data)
{
   E_Smart_Item *it;
   
   it = data;
   edje_object_part_text_set(it->sd->edje_obj, "e.text.label", it->label);
   it->sd->cur = it;
}

/* externally accessible functions */
EAPI Evas_Object *
e_slidesel_add(Evas *evas)
{
   _e_smart_init();
   return evas_object_smart_add(evas, _e_smart);
}

EAPI void
e_slidesel_item_distance_set(Evas_Object *obj, Evas_Coord dist)
{
   API_ENTRY return;
   e_slidecore_item_distance_set(sd->slide_obj, dist);
}

EAPI void
e_slidesel_item_add(Evas_Object *obj, const char *label, const char *icon, void (*func) (void *data), void *data)
{
   E_Smart_Item *it;
   
   API_ENTRY return;
   it = calloc(1, sizeof(E_Smart_Item));
   if (!it) return;
   it->sd = sd;
   if (label) it->label = evas_stringshare_add(label);
   if (icon) it->icon = evas_stringshare_add(icon);
   it->func = func;
   it->data = data;
   sd->items = evas_list_append(sd->items, it);
   e_slidecore_item_add(sd->slide_obj, label, icon, _e_smart_label_change, it);
}

EAPI void
e_slidesel_jump(Evas_Object *obj, int num)
{
   API_ENTRY return;
   e_slidecore_jump(sd->slide_obj, num);
}

/* local subsystem functions */
static void
_e_smart_event_wheel(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Wheel *ev;
   E_Smart_Data *sd;

   sd = data;
   ev = event_info;
}

static void
_e_smart_event_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   E_Smart_Data *sd;

   sd = data;
   ev = event_info;
   if (ev->button == 1)
     {
	sd->down_time = ecore_time_get();
	sd->down = 1;
	sd->down_cancel = 0;
	sd->down_x = ev->canvas.x;
	sd->down_y = ev->canvas.y;
	edje_object_signal_emit(sd->edje_obj, "e,state,slide,hint,on", "e");
     }
}

static void
_e_smart_event_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *ev;
   E_Smart_Data *sd;

   sd = data;
   ev = event_info;
   if (ev->button == 1)
     {
	double t;
	
	t = ecore_time_get();
	if (!sd->down_cancel)
	  {
	     edje_object_signal_emit(sd->edje_obj, "e,state,slide,hint,off", "e");
	     if (!(ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD))
	       {
		  if (sd->cur)
		    {
		       /* get rid of accidental release and presses */
//		       if ((t - sd->down_time) > 0.2)
			 {
			    edje_object_signal_emit(sd->edje_obj, "e,action,select", "e");
			    if (sd->cur->func) sd->cur->func(sd->cur->data);
			 }
		    }
	       }
	  }
	sd->down = 0;
     }
}

static void
_e_smart_event_mouse_move(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Move *ev;
   E_Smart_Data *sd;

   sd = data;
   ev = event_info;
   if ((sd->down) && (!sd->down_cancel))
     {
	Evas_Coord d1, d2, d;
	
	printf("DRAG @ %3.3f\n", ecore_time_get());
	d1 = ev->cur.canvas.x - sd->down_x;
	d2 = ev->cur.canvas.y - sd->down_y;
	d = (d1 * d1) + (d2 * d2);
	if (d > (64 * 64))
	  {
	     edje_object_signal_emit(sd->edje_obj, "e,state,slide,hint,off", "e");
	     sd->down_cancel = 1;
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
/*   
   if (!strcmp(ev->keyname, "Left"))
     x -= sd->step.x;
   else if (!strcmp(ev->keyname, "Right"))
     x += sd->step.x;
   else if (!strcmp(ev->keyname, "Up"))
     y -= sd->step.y;
   else if (!strcmp(ev->keyname, "Home"))
     y = 0;
   else if (!strcmp(ev->keyname, "End"))
     y = my;
   else if (!strcmp(ev->keyname, "Down"))
     y += sd->step.y;
   else if (!strcmp(ev->keyname, "Prior"))
     {
	if (sd->page.y < 0)
	  y -= -(sd->page.y * vh) / 100;
	else
	  y -= sd->page.y;
     }
   else if (!strcmp(ev->keyname, "Next"))
     {
	if (sd->page.y < 0)
	  y += -(sd->page.y * vh) / 100;
	else
	  y += sd->page.y;
     }
 */
}
                    
static void
_e_smart_reconfigure(E_Smart_Data *sd)
{
   evas_object_move(sd->edje_obj, sd->x, sd->y);
   evas_object_resize(sd->edje_obj, sd->w, sd->h);
   evas_object_move(sd->event_obj, sd->x, sd->y);
   evas_object_resize(sd->event_obj, sd->w, sd->h);
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
   
   evas_object_event_callback_add(obj, EVAS_CALLBACK_KEY_DOWN, _e_smart_event_key_down, sd);
   evas_object_propagate_events_set(obj, 0);
   
   o = edje_object_add(evas_object_evas_get(obj));
   sd->edje_obj = o;
   e_theme_edje_object_set(o, "base/theme/widgets",
			   "e/widgets/slidesel");
   evas_object_smart_member_add(o, obj);
   
   o = e_slidecore_add(evas_object_evas_get(obj));
   sd->slide_obj = o;
   edje_object_part_swallow(sd->edje_obj, "e.swallow.content", o);
   evas_object_show(o);
   
   o = evas_object_rectangle_add(evas_object_evas_get(obj));
   sd->event_obj = o;
   evas_object_color_set(o, 0, 0, 0, 0);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_WHEEL, _e_smart_event_wheel, sd);
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
   evas_object_del(sd->slide_obj);
   evas_object_del(sd->edje_obj);
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
   evas_object_show(sd->edje_obj);
   evas_object_show(sd->event_obj);
}

static void
_e_smart_hide(Evas_Object *obj)
{
   INTERNAL_ENTRY;
   evas_object_hide(sd->edje_obj);
   evas_object_hide(sd->event_obj);
}

static void
_e_smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   INTERNAL_ENTRY;
   evas_object_color_set(sd->edje_obj, r, g, b, a);
}

static void
_e_smart_clip_set(Evas_Object *obj, Evas_Object * clip)
{
   INTERNAL_ENTRY;
   evas_object_clip_set(sd->edje_obj, clip);
   evas_object_clip_set(sd->event_obj, clip);
}

static void
_e_smart_clip_unset(Evas_Object *obj)
{
   INTERNAL_ENTRY;
   evas_object_clip_unset(sd->edje_obj);
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

