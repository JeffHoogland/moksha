/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

#define SMART_NAME "e_widget"
#define API_ENTRY E_Smart_Data *sd; sd = evas_object_smart_data_get(obj); if (!sd)
#define INTERNAL_ENTRY E_Smart_Data *sd; sd = evas_object_smart_data_get(obj); if (!sd) return;
typedef struct _E_Smart_Data E_Smart_Data;

struct _E_Smart_Data
{ 
   Evas_Coord     x, y, w, h;
   Evas_Coord     minw, minh, maxw, maxh;
   Evas_List     *subobjs;
   Evas_Object   *resize_obj;
   void         (*del_func) (Evas_Object *obj);
   void          *data;
}; 

/* local subsystem functions */
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
e_widget_add(Evas *evas)
{
   _e_smart_init();
   return evas_object_smart_add(evas, _e_smart);
}

void
e_widget_del_hook_set(Evas_Object *obj, void (*func) (Evas_Object *obj))
{
   API_ENTRY return;
   sd->del_func = func;
}

void
e_widget_data_set(Evas_Object *obj, void *data)
{
   API_ENTRY return;
   sd->data = data;
}

void *
e_widget_data_get(Evas_Object *obj)
{
   API_ENTRY return NULL;
   return sd->data;
}

void
e_widget_min_size_set(Evas_Object *obj, Evas_Coord minw, Evas_Coord minh)
{
   API_ENTRY return;
   sd->minw = minw;
   sd->minh = minh;
}

void
e_widget_min_size_get(Evas_Object *obj, Evas_Coord *minw, Evas_Coord *minh)
{
   API_ENTRY return;
   if (minw) *minw = sd->minw;
   if (minh) *minh = sd->minh;
}

void
e_widget_max_size_set(Evas_Object *obj, Evas_Coord maxw, Evas_Coord maxh)
{
   API_ENTRY return;
   sd->minw = maxw;
   sd->maxh = maxh;
}

void
e_widget_max_size_get(Evas_Object *obj, Evas_Coord *maxw, Evas_Coord *maxh)
{
   API_ENTRY return;
   if (maxw) *maxw = sd->maxw;
   if (maxh) *maxh = sd->maxh;
}

void
e_widget_sub_object_add(Evas_Object *obj, Evas_Object *sobj)
{
   API_ENTRY return;
   sd->subobjs = evas_list_append(sd->subobjs, sobj);
}

void
e_widget_sub_object_del(Evas_Object *obj, Evas_Object *sobj)
{
   API_ENTRY return;
   sd->subobjs = evas_list_remove(sd->subobjs, sobj);
}

void
e_widget_resize_object_set(Evas_Object *obj, Evas_Object *sobj)
{
   API_ENTRY return;
   if (sd->resize_obj) evas_object_smart_member_del(sd->resize_obj);
   sd->resize_obj = sobj;
   evas_object_smart_member_add(obj, sobj);
   _e_smart_reconfigure(sd);
}

/* local subsystem functions */
static void
_e_smart_reconfigure(E_Smart_Data *sd)
{
   if (sd->resize_obj)
     {
	evas_object_move(sd->resize_obj, sd->x, sd->y);
	evas_object_resize(sd->resize_obj, sd->w, sd->h);
     }
}

static void
_e_smart_add(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = calloc(1, sizeof(E_Smart_Data));
   if (!sd) return;
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
   if (sd->del_func) sd->del_func(obj);
   while (sd->subobjs)
     {
	evas_object_del(sd->subobjs->data);
	sd->subobjs = evas_list_remove_list(sd->subobjs, sd->subobjs);
     }
   free(sd);
}
   
static void
_e_smart_layer_set(Evas_Object *obj, int layer)
{
   INTERNAL_ENTRY;
   evas_object_layer_set(sd->resize_obj, layer);
}

static void
_e_smart_raise(Evas_Object *obj)
{
   INTERNAL_ENTRY;
   evas_object_raise(sd->resize_obj);
}

static void
_e_smart_lower(Evas_Object *obj)
{
   INTERNAL_ENTRY;
   evas_object_lower(sd->resize_obj);
}
                                                             
static void
_e_smart_stack_above(Evas_Object *obj, Evas_Object *above)
{
   INTERNAL_ENTRY;
   evas_object_stack_above(sd->resize_obj, above);
}
   
static void
_e_smart_stack_below(Evas_Object *obj, Evas_Object *below)
{
   INTERNAL_ENTRY;
   evas_object_stack_below(sd->resize_obj, below);
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
   evas_object_show(sd->resize_obj);
}

static void
_e_smart_hide(Evas_Object *obj)
{
   INTERNAL_ENTRY;
   evas_object_hide(sd->resize_obj);
}

static void
_e_smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   INTERNAL_ENTRY;
   evas_object_color_set(sd->resize_obj, r, g, b, a);
}

static void
_e_smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
   INTERNAL_ENTRY;
   evas_object_clip_set(sd->resize_obj, clip);
}

static void
_e_smart_clip_unset(Evas_Object *obj)
{
   INTERNAL_ENTRY;
   evas_object_clip_unset(sd->resize_obj);
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

