/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

#define SMART_NAME "e_pan"
#define API_ENTRY E_Smart_Data *sd; sd = evas_object_smart_data_get(obj); if (!sd)
#define INTERNAL_ENTRY E_Smart_Data *sd; sd = evas_object_smart_data_get(obj); if (!sd) return;
typedef struct _E_Smart_Data E_Smart_Data;

struct _E_Smart_Data
{ 
   Evas_Object *smart_obj;
   Evas_Object *child_obj;
   Evas_Coord   x, y, w, h;
   Evas_Coord   child_w, child_h, px, py;
}; 

/* local subsystem functions */
static void _e_smart_child_del_hook(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _e_smart_child_resize_hook(void *data, Evas *e, Evas_Object *obj, void *event_info);

static void _e_smart_reconfigure(E_Smart_Data *sd);
static void _e_smart_add(Evas_Object *obj);
static void _e_smart_del(Evas_Object *obj);
static void _e_smart_layer_set(Evas_Object *obj, int layer);
static void _e_smart_raise(Evas_Object *obj);
static void _e_smart_lower(Evas_Object *obj);
static void _e_smart_stack_above(Evas_Object *obj, Evas_Object *above);
static void _e_smart_stack_below(Evas_Object *obj, Evas_Object *below);
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
Evas_Object *
e_pan_add(Evas *evas)
{
   _e_smart_init();
   return evas_object_smart_add(evas, _e_smart);
}

void
e_pan_child_set(Evas_Object *obj, Evas_Object *child)
{
   API_ENTRY return;
   if (child == sd->child_obj) return;
   if (sd->child_obj)
     {
	evas_object_clip_unset(sd->child_obj);
	evas_object_smart_member_del(sd->child_obj);
        evas_object_event_callback_del(sd->child_obj, EVAS_CALLBACK_FREE, _e_smart_child_del_hook);
	evas_object_event_callback_del(sd->child_obj, EVAS_CALLBACK_RESIZE, _e_smart_child_resize_hook);
	sd->child_obj = NULL;
     }
   if (child)
     {
	Evas_Coord w, h;
	int r, g, b, a;
	
	sd->child_obj = child;
	evas_object_smart_member_add(sd->child_obj, sd->smart_obj);
	evas_object_geometry_get(sd->child_obj, NULL, NULL, &w, &h);
	sd->child_w = w;
	sd->child_h = h;
	evas_object_event_callback_add(child, EVAS_CALLBACK_FREE, _e_smart_child_del_hook, sd);
	evas_object_event_callback_add(child, EVAS_CALLBACK_RESIZE, _e_smart_child_resize_hook, sd);
	evas_object_layer_set(sd->child_obj, evas_object_layer_get(sd->smart_obj));
	evas_object_stack_above(sd->child_obj, sd->smart_obj);
	evas_object_color_get(sd->smart_obj, &r, &g, &b, &a);
	evas_object_color_set(sd->child_obj, r, g, b, a);
	evas_object_clip_set(sd->child_obj, evas_object_clip_get(sd->smart_obj));
	if (evas_object_visible_get(sd->smart_obj)) evas_object_show(sd->child_obj);
	else evas_object_hide(sd->child_obj);
	_e_smart_reconfigure(sd);
     }
   evas_object_smart_callback_call(sd->smart_obj, "changed", NULL);
}

Evas_Object *
e_pan_child_get(Evas_Object *obj)
{
   API_ENTRY return NULL;
   return sd->child_obj;
}

void
e_pan_set(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   API_ENTRY return;
   sd->px = x;
   sd->py = y;
   if (x > (sd->w - sd->child_w)) x = sd->w - sd->child_w;
   if (y > (sd->h - sd->child_h)) y = sd->h - sd->child_h;
   if (x < 0) x = 0;
   if (y < 0) y = 0;
   if ((x == sd->px) && (y == sd->py)) return;
   _e_smart_reconfigure(sd);
   evas_object_smart_callback_call(sd->smart_obj, "changed", NULL);
}

void
e_pan_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y)
{
   API_ENTRY return;
   if (x) *x = sd->px;
   if (y) *y = sd->py;
}

void
e_pan_max_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y)
{
   API_ENTRY return;
   if (x)
     {
	if (sd->w > sd->child_w) *x = sd->w - sd->child_w;
	else *x = 0;
     }
   if (y)
     {
	if (sd->h > sd->child_h) *y = sd->h - sd->child_h;
	else *y = 0;
     }
}

void
e_pan_child_size_get(Evas_Object *obj, Evas_Coord *w, Evas_Coord *h)
{
   API_ENTRY return;
   if (w) *w = sd->child_w;
   if (h) *h = sd->child_h;
}

/* local subsystem functions */
static void
_e_smart_child_del_hook(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   E_Smart_Data *sd;
   
   sd = data;
   sd->child_obj = NULL;
   evas_object_smart_callback_call(sd->smart_obj, "changed", NULL);
}

static void
_e_smart_child_resize_hook(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   E_Smart_Data *sd;
   Evas_Coord w, h;
   
   sd = data;
   evas_object_geometry_get(sd->child_obj, NULL, NULL, &w, &h);
   if ((w != sd->child_w) || (h != sd->child_h))
     {
	sd->child_w = w;
	sd->child_h = h;
	_e_smart_reconfigure(sd);
     }
   evas_object_smart_callback_call(sd->smart_obj, "changed", NULL);
}

static void
_e_smart_reconfigure(E_Smart_Data *sd)
{
   evas_object_move(sd->child_obj, sd->x - sd->px, sd->y - sd->py);
}

static void
_e_smart_add(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = calloc(1, sizeof(E_Smart_Data));
   if (!sd) return;
   sd->smart_obj = obj;
   sd->x = 0;
   sd->y = 0;
   sd->w = 0;
   sd->h = 0;
   evas_object_smart_data_set(obj, sd);
}

static void
_e_smart_del(Evas_Object *obj)
{
   INTERNAL_ENTRY;
   e_pan_child_set(obj, NULL);
   free(sd);
}
   
static void
_e_smart_layer_set(Evas_Object *obj, int layer)
{
   INTERNAL_ENTRY;
   evas_object_layer_set(sd->child_obj, layer);
}

static void
_e_smart_raise(Evas_Object *obj)
{
   INTERNAL_ENTRY;
   evas_object_raise(sd->child_obj);
}

static void
_e_smart_lower(Evas_Object *obj)
{
   INTERNAL_ENTRY;
   evas_object_lower(sd->child_obj);
}
                                                             
static void
_e_smart_stack_above(Evas_Object *obj, Evas_Object *above)
{
   INTERNAL_ENTRY;
   evas_object_stack_above(sd->child_obj, above);
}
   
static void
_e_smart_stack_below(Evas_Object *obj, Evas_Object *below)
{
   INTERNAL_ENTRY;
   evas_object_stack_below(sd->child_obj, below);
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
   evas_object_smart_callback_call(sd->smart_obj, "changed", NULL);
}

static void
_e_smart_show(Evas_Object *obj)
{
   INTERNAL_ENTRY;
   evas_object_show(sd->child_obj);
}

static void
_e_smart_hide(Evas_Object *obj)
{
   INTERNAL_ENTRY;
   evas_object_hide(sd->child_obj);
}

static void
_e_smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   INTERNAL_ENTRY;
   evas_object_color_set(sd->child_obj, r, g, b, a);
}

static void
_e_smart_clip_set(Evas_Object *obj, Evas_Object * clip)
{
   INTERNAL_ENTRY;
   evas_object_clip_set(sd->child_obj, clip);
}

static void
_e_smart_clip_unset(Evas_Object *obj)
{
   INTERNAL_ENTRY;
   evas_object_clip_unset(sd->child_obj);
}  

/* never need to touch this */

static void
_e_smart_init(void)
{
   if (_e_smart) return;
   _e_smart = evas_smart_new
     (SMART_NAME, _e_smart_add, _e_smart_del, _e_smart_layer_set,
      _e_smart_raise, _e_smart_lower, _e_smart_stack_above,
      _e_smart_stack_below, _e_smart_move, _e_smart_resize,
      _e_smart_show, _e_smart_hide, _e_smart_color_set,
      _e_smart_clip_set, _e_smart_clip_unset, NULL);
}

