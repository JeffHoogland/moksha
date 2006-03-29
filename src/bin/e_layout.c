/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* FIXME: something is weird here - i had to reverse all stacking logic to make
 * it work... */

typedef struct _E_Smart_Data E_Smart_Data;
typedef struct _E_Layout_Item E_Layout_Item;

struct _E_Smart_Data
{ 
   Evas_Coord       x, y, w, h;
   Evas_Coord       vw, vh;
   Evas_Object     *obj;
   Evas_Object     *clip;
   int              frozen;
   unsigned char    changed : 1;
   Evas_List       *items;
}; 

struct _E_Layout_Item
{
   E_Smart_Data    *sd;
   Evas_Coord       x, y, w, h;
   Evas_Object     *obj;
};

/* local subsystem functions */
static E_Layout_Item *_e_layout_smart_adopt(E_Smart_Data *sd, Evas_Object *obj);
static void        _e_layout_smart_disown(Evas_Object *obj);
static void        _e_layout_smart_item_del_hook(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void        _e_layout_smart_reconfigure(E_Smart_Data *sd);
static void        _e_layout_smart_move_resize_item(E_Layout_Item *li);

static void _e_layout_smart_init(void);
static void _e_layout_smart_add(Evas_Object *obj);
static void _e_layout_smart_del(Evas_Object *obj);
static void _e_layout_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _e_layout_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static void _e_layout_smart_show(Evas_Object *obj);
static void _e_layout_smart_hide(Evas_Object *obj);
static void _e_layout_smart_color_set(Evas_Object *obj, int r, int g, int b, int a);
static void _e_layout_smart_clip_set(Evas_Object *obj, Evas_Object *clip);
static void _e_layout_smart_clip_unset(Evas_Object *obj);

/* local subsystem globals */
static Evas_Smart *_e_smart = NULL;

/* externally accessible functions */
EAPI Evas_Object *
e_layout_add(Evas *evas)
{
   _e_layout_smart_init();
   return evas_object_smart_add(evas, _e_smart);
}

EAPI int
e_layout_freeze(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   sd->frozen++;
   return sd->frozen;
}

EAPI int
e_layout_thaw(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   sd->frozen--;
   if (sd->frozen <= 0) _e_layout_smart_reconfigure(sd);
   return sd->frozen;
}

EAPI void
e_layout_virtual_size_set(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (w < 1) w = 1;
   if (h < 1) h = 1;
   if ((sd->vw == w) && (sd->vh == h)) return;
   sd->vw = w;
   sd->vh = h;
   sd->changed = 1;
   if (sd->frozen <= 0) _e_layout_smart_reconfigure(sd);
}

EAPI void
e_layout_virtual_size_get(Evas_Object *obj, Evas_Coord *w, Evas_Coord *h)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (w) *w = sd->vw;
   if (h) *h = sd->vh;
}

EAPI void
e_layout_coord_canvas_to_virtual(Evas_Object *obj, Evas_Coord cx, Evas_Coord cy, Evas_Coord *vx, Evas_Coord *vy)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);

   if (vx) *vx = (cx - sd->x) * ((double)(sd->vw) / sd->w);
   if (vy) *vy = (cy - sd->y) * ((double)(sd->vh) / sd->h);
}

EAPI void
e_layout_coord_virtual_to_canvas(Evas_Object *obj, Evas_Coord vx, Evas_Coord vy, Evas_Coord *cx, Evas_Coord *cy)
{
   E_Smart_Data *sd;

   sd = evas_object_smart_data_get(obj);

   if (cx) *cx = vx * ((double)(sd->w) / sd->vw) + sd->x;
   if (cy) *cy = vy * ((double)(sd->h) / sd->vh) + sd->y;
}

EAPI void
e_layout_pack(Evas_Object *obj, Evas_Object *child)
{
   E_Smart_Data *sd;
   E_Layout_Item *li;
   
   sd = evas_object_smart_data_get(obj);
   _e_layout_smart_adopt(sd, child);
   sd->items = evas_list_append(sd->items, child);
   li = evas_object_data_get(child, "e_layout_data");
   _e_layout_smart_move_resize_item(li);
}

EAPI void
e_layout_child_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   E_Layout_Item *li;
   
   li = evas_object_data_get(obj, "e_layout_data");
   if (!li) return;
   if ((li->x == x) && (li->y == y)) return;
   li->x = x;
   li->y = y;
   _e_layout_smart_move_resize_item(li);
}

EAPI void
e_layout_child_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Layout_Item *li;
   
   li = evas_object_data_get(obj, "e_layout_data");
   if (!li) return;
   if (w < 0) w = 0;
   if (h < 0) h = 0;
   if ((li->w == w) && (li->h == h)) return;
   li->w = w;
   li->h = h;
   _e_layout_smart_move_resize_item(li);
}

EAPI void
e_layout_child_lower(Evas_Object *obj)
{
   E_Layout_Item *li;
   
   li = evas_object_data_get(obj, "e_layout_data");
   if (!li) return;
   if (!evas_list_find(li->sd->items, obj)) return;
   if ((li->sd->items) && (li->sd->items->next))
     {
	li->sd->items = evas_list_remove(li->sd->items, obj);
	evas_object_lower(obj);
	li->sd->items = evas_list_prepend(li->sd->items, obj);
     }
}

EAPI void
e_layout_child_raise(Evas_Object *obj)
{
   E_Layout_Item *li;
   
   li = evas_object_data_get(obj, "e_layout_data");
   if (!li) return;
   if (!evas_list_find(li->sd->items, obj)) return;
   if ((li->sd->items) && (li->sd->items->next))
     {
	li->sd->items = evas_list_remove(li->sd->items, obj);
	evas_object_raise(obj);
	li->sd->items = evas_list_append(li->sd->items, obj);
     }
}

EAPI void
e_layout_child_lower_below(Evas_Object *obj, Evas_Object *below)
{
   E_Layout_Item *li;
   
   li = evas_object_data_get(obj, "e_layout_data");
   if (!li) return;
   if (!evas_list_find(li->sd->items, below)) return;
   if (!evas_list_find(li->sd->items, obj)) return;
   if ((li->sd->items) && (li->sd->items->next))
     {
	li->sd->items = evas_list_remove(li->sd->items, obj);
	evas_object_stack_below(obj, below);
	li->sd->items = evas_list_prepend_relative(li->sd->items, obj, below);
     }
}

EAPI void
e_layout_child_raise_above(Evas_Object *obj, Evas_Object *above)
{
   E_Layout_Item *li;
   
   li = evas_object_data_get(obj, "e_layout_data");
   if (!li) return;
   if (!evas_list_find(li->sd->items, above)) return;
   if (!evas_list_find(li->sd->items, obj)) return;
   if ((li->sd->items) && (li->sd->items->next))
     {
	li->sd->items = evas_list_remove(li->sd->items, obj);
	evas_object_stack_above(obj, above);
	li->sd->items = evas_list_append_relative(li->sd->items, obj, above);
     }
}

EAPI void
e_layout_child_geometry_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y, Evas_Coord *w, Evas_Coord *h)
{
   E_Layout_Item *li;

   li = evas_object_data_get(obj, "e_layout_data");
   if (!li) return;
   
   if (x) *x = li->x;
   if (y) *y = li->y;
   if (w) *w = li->w;
   if (h) *h = li->h;
}

EAPI void
e_layout_unpack(Evas_Object *obj)
{
   E_Layout_Item *li;
   E_Smart_Data *sd;
   
   li = evas_object_data_get(obj, "e_layout_data");
   if (!li) return;
   sd = li->sd;
   sd->items = evas_list_remove(sd->items, obj);
   _e_layout_smart_disown(obj);
}

/* local subsystem functions */
static E_Layout_Item *
_e_layout_smart_adopt(E_Smart_Data *sd, Evas_Object *obj)
{
   E_Layout_Item *li;
   
   li = calloc(1, sizeof(E_Layout_Item));
   if (!li) return NULL;
   li->sd = sd;
   li->obj = obj;
   /* defaults */
   li->x = 0;
   li->y = 0;
   li->w = 0;
   li->h = 0;
   evas_object_clip_set(obj, sd->clip);
   evas_object_smart_member_add(obj, li->sd->obj);
   evas_object_data_set(obj, "e_layout_data", li);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_FREE,
				  _e_layout_smart_item_del_hook, NULL);
   if ((!evas_object_visible_get(sd->clip)) &&
       (evas_object_visible_get(sd->obj)))
     evas_object_show(sd->clip);
   return li;
}

static void
_e_layout_smart_disown(Evas_Object *obj)
{
   E_Layout_Item *li;
   
   li = evas_object_data_get(obj, "e_layout_data");
   if (!li) return;
   if (!li->sd->items)
     {
	if (evas_object_visible_get(li->sd->clip))
	  evas_object_hide(li->sd->clip);
     }
   evas_object_event_callback_del(obj,
				  EVAS_CALLBACK_FREE,
				  _e_layout_smart_item_del_hook);
   evas_object_smart_member_del(obj);
   evas_object_data_del(obj, "e_layout_data");
   free(li);
}

static void
_e_layout_smart_item_del_hook(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   e_layout_unpack(obj);
}

static void
_e_layout_smart_reconfigure(E_Smart_Data *sd)
{
   Evas_Coord x, y, w, h;
   Evas_List *l;

   if (!sd->changed) return;
   
   x = sd->x;
   y = sd->y;
   w = sd->w;
   h = sd->h;

   for (l = sd->items; l; l = l->next)
     {
	E_Layout_Item *li;
	Evas_Object *obj;
	
	obj = l->data;
	li = evas_object_data_get(obj, "e_layout_data");
	_e_layout_smart_move_resize_item(li);
     }
   sd->changed = 0;
}

static void
_e_layout_smart_move_resize_item(E_Layout_Item *li)
{
   evas_object_move(li->obj,
		    li->sd->x + ((li->x * li->sd->w) / li->sd->vw),
		    li->sd->y + ((li->y * li->sd->h) / li->sd->vh));
   evas_object_resize(li->obj,
		      (li->w * li->sd->w) / li->sd->vw,
		      (li->h * li->sd->h) / li->sd->vh);
}

static void
_e_layout_smart_init(void)
{
   if (_e_smart) return;
   _e_smart = evas_smart_new("e_layout",
			     _e_layout_smart_add,
			     _e_layout_smart_del,
			     NULL, NULL, NULL, NULL, NULL,
			     _e_layout_smart_move,
			     _e_layout_smart_resize,
			     _e_layout_smart_show,
			     _e_layout_smart_hide,
			     _e_layout_smart_color_set,
			     _e_layout_smart_clip_set,
			     _e_layout_smart_clip_unset,
			     NULL);
}

static void
_e_layout_smart_add(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = calloc(1, sizeof(E_Smart_Data));
   if (!sd) return;
   sd->obj = obj;
   sd->x = 0;
   sd->y = 0;
   sd->w = 0;
   sd->h = 0;
   sd->vw = 1;
   sd->vh = 1;
   sd->clip = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_smart_member_add(sd->clip, obj);
   evas_object_move(sd->clip, -100001, -100001);
   evas_object_resize(sd->clip, 200002, 200002);
   evas_object_color_set(sd->clip, 255, 255, 255, 255);
   evas_object_smart_data_set(obj, sd);
}
   
static void
_e_layout_smart_del(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   while (sd->items)
     {
	Evas_Object *child;
	
	child = sd->items->data;
	e_layout_unpack(child);
     }
   evas_object_del(sd->clip);
   free(sd);
}

static void
_e_layout_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if ((x == sd->x) && (y == sd->y)) return;
   if ((x == sd->x) && (y == sd->y)) return;
     {
	Evas_List *l;
	Evas_Coord dx, dy;
	
	dx = x - sd->x;
	dy = y - sd->y;
	for (l = sd->items; l; l = l->next)
	  {
	     Evas_Coord ox, oy;
	     
	     evas_object_geometry_get(l->data, &ox, &oy, NULL, NULL);
	     evas_object_move(l->data, ox + dx, oy + dy);
	  }
     }
   sd->x = x;
   sd->y = y;
}

static void
_e_layout_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if ((w == sd->w) && (h == sd->h)) return;
   sd->w = w;
   sd->h = h;
   sd->changed = 1;
   _e_layout_smart_reconfigure(sd);
}

static void
_e_layout_smart_show(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if (sd->items) evas_object_show(sd->clip);
}

static void
_e_layout_smart_hide(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_hide(sd->clip);
}

static void
_e_layout_smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;   
   evas_object_color_set(sd->clip, r, g, b, a);
}

static void
_e_layout_smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_clip_set(sd->clip, clip);
}

static void
_e_layout_smart_clip_unset(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_clip_unset(sd->clip);
}  
