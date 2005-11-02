#include <Evas.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct _E_Smart_Data E_Smart_Data;
typedef struct _E_Icon_Layout_Item E_Icon_Layout_Item;

struct _E_Smart_Data
{
   Evas_Coord       x, y, w, h;
   Evas_Coord       vw, vh;
   Evas_Coord       xs, ys;
   Evas_Coord       xc, yc;
   Evas_Object     *clip;
   Evas_Object     *obj;
   int              frozen;
   int              clip_frozen;
   int              fixed;
   unsigned char    changed : 1;
   Evas_List       *items;         
};

struct _E_Icon_Layout_Item
{
   E_Smart_Data    *sd;
   Evas_Coord       x, y, w, h;
   Evas_Object     *obj;
};

/* local subsystem functions */
static E_Icon_Layout_Item *_e_icon_layout_smart_adopt(E_Smart_Data *sd, Evas_Object *obj);
static void                _e_icon_layout_smart_disown(Evas_Object *obj);
static void                _e_icon_layout_smart_item_del_hook(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void                _e_icon_layout_smart_reconfigure(E_Smart_Data *sd);
static void                _e_icon_layout_smart_move_resize_item(E_Icon_Layout_Item *li);


static void _e_icon_layout_smart_init(void);
static void _e_icon_layout_smart_add(Evas_Object *obj);
static void _e_icon_layout_smart_show(Evas_Object *obj);
static void _e_icon_layout_smart_hide(Evas_Object *obj);
static void _e_icon_layout_smart_del(Evas_Object *obj);
static void _e_icon_layout_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _e_icon_layout_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static void _e_icon_layout_smart_color_set(Evas_Object *obj, int r, int g, int b, int a);
static void _e_icon_layout_smart_clip_set(Evas_Object *obj, Evas_Object *clip);
static void _e_icon_layout_smart_clip_unset(Evas_Object *obj);

/* local subsystem globals */
static Evas_Smart *_e_smart = NULL;

/* externally accessible functions */
Evas_Object *
e_icon_layout_add(Evas *evas)
{
   _e_icon_layout_smart_init();
   return evas_object_smart_add(evas, _e_smart);
}

int
e_icon_layout_freeze(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   if ((!obj) || !(sd = evas_object_smart_data_get(obj)))
     return -1;
   
   sd->frozen++;
   return sd->frozen;
}

int
e_icon_layout_thaw(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   if ((!obj) || !(sd = evas_object_smart_data_get(obj)))
     return -1;
         
   sd->frozen--;
   if (sd->frozen <= 0) _e_icon_layout_smart_reconfigure(sd);     
   
   return sd->frozen;
}

void
e_icon_layout_virtual_size_get(Evas_Object *obj, Evas_Coord *w, Evas_Coord *h)
{
   E_Smart_Data *sd;
   
   if ((!obj) || !(sd = evas_object_smart_data_get(obj)))
     return;
   
   if (w) *w = sd->vw;
   if (h) *h = sd->vh;
}

void
e_icon_layout_width_fix(Evas_Object *obj, Evas_Coord w)
{
   E_Smart_Data *sd;
   
   if ((!obj) || !(sd = evas_object_smart_data_get(obj)))
     return;
   
   if(w < 1) w = 1;
   if (sd->vw == w) return;
   sd->fixed = 0;
   sd->vw = w;
   sd->vh = 0;
   sd->changed = 1;
   if (sd->frozen <= 0) _e_icon_layout_smart_reconfigure(sd);   
}

void
e_icon_layout_height_fix(Evas_Object *obj, Evas_Coord h)
{
   E_Smart_Data *sd;
   
   if ((!obj) || !(sd = evas_object_smart_data_get(obj)))
     return;
   
   if(h < 1) h = 1;
   if (sd->vh == h) return;
   sd->fixed = 1;
   sd->vw = 0;
   sd->vh = h;
   sd->changed = 1;
   if (sd->frozen <= 0) _e_icon_layout_smart_reconfigure(sd);   
}

void
e_icon_layout_pack(Evas_Object *obj, Evas_Object *child)
{
   E_Smart_Data *sd;
   E_Icon_Layout_Item *li;
   
   if ((!obj) || !(sd = evas_object_smart_data_get(obj)))
     return;
   
   _e_icon_layout_smart_adopt(sd, child);
   sd->items = evas_list_append(sd->items, child);
   li = evas_object_data_get(child, "e_icon_layout_data");
   li->x = sd->xc;
   li->y = sd->yc;
   _e_icon_layout_smart_move_resize_item(li);
}

void
e_icon_layout_child_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Icon_Layout_Item *li;
   
   li = evas_object_data_get(obj, "e_icon_layout_data");
   if (!li) return;
   if (w < 0) w = 0;
   if (h < 0) h = 0;
   if ((li->w == w) && (li->h == h)) return;
   li->w = w;
   li->h = h;
   _e_icon_layout_smart_move_resize_item(li);
}

void
e_icon_layout_unpack(Evas_Object *obj)
{
   E_Icon_Layout_Item *li;
   E_Smart_Data *sd;
   
   li = evas_object_data_get(obj, "e_icon_layout_data");
   if (!li) return;
   sd = li->sd;
   sd->items = evas_list_remove(sd->items, obj);
   _e_icon_layout_smart_disown(obj);
}

void
e_icon_layout_spacing_set(Evas_Object *obj, Evas_Coord xs, Evas_Coord ys)
{
   E_Smart_Data *sd;
   
   if ((!obj) || !(sd = evas_object_smart_data_get(obj)))
     return;
   
   if(sd->xs == xs && sd->ys == ys)
     return;
   
   sd->xs = xs;
   sd->ys = ys;
   
   sd->xc = sd->x + sd->xs;
   sd->yc = sd->y + sd->ys;
   
   sd->changed = 1;
   
   if (sd->frozen <= 0) _e_icon_layout_smart_reconfigure(sd);   
}

void         
e_icon_layout_redraw_force(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   if ((!obj) || !(sd = evas_object_smart_data_get(obj)))
     return;
   
   sd->changed = 1;
   
   if (sd->frozen <= 0) _e_icon_layout_smart_reconfigure(sd);   
}

void
e_icon_layout_reset(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   if ((!obj) || !(sd = evas_object_smart_data_get(obj)))
     return;
   
   sd->xc = sd->x + sd->xs;
   sd->yc = sd->y + sd->ys;
   
   while (sd->items)
    {
       Evas_Object *child;
       
       child = sd->items->data;
       e_icon_layout_unpack(child);
    }      
}

void
e_icon_layout_clip_freeze(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   if ((!obj) || !(sd = evas_object_smart_data_get(obj)))
     return;
   
   sd->clip_frozen = 1;
}

void
e_icon_layout_clip_thaw(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   if ((!obj) || !(sd = evas_object_smart_data_get(obj)))
     return;
   
   sd->clip_frozen = 0;
}

/* local subsystem functions */
static E_Icon_Layout_Item *
_e_icon_layout_smart_adopt(E_Smart_Data *sd, Evas_Object *obj)
{
   E_Icon_Layout_Item *li;
   
   li = calloc(1, sizeof(E_Icon_Layout_Item));
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
   evas_object_show(obj);
   evas_object_data_set(obj, "e_icon_layout_data", li);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_FREE,
				  _e_icon_layout_smart_item_del_hook, NULL);
   if (!evas_object_visible_get(sd->clip))
     evas_object_show(sd->clip);
   return li;
}

static void
_e_icon_layout_smart_disown(Evas_Object *obj)
{
   E_Icon_Layout_Item *li;
   
   li = evas_object_data_get(obj, "e_icon_layout_data");
   if (!li) return;
   if (!li->sd->items)
    {
       if (evas_object_visible_get(li->sd->clip))
	 evas_object_hide(li->sd->clip);
    }
   evas_object_event_callback_del(obj,
				  EVAS_CALLBACK_FREE,
				  _e_icon_layout_smart_item_del_hook);
   evas_object_smart_member_del(li->sd->obj);
   evas_object_data_del(obj, "e_icon_layout_data");
   free(li);
}

static void
_e_icon_layout_smart_item_del_hook(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   e_icon_layout_unpack(obj);
}

static void
_e_icon_layout_smart_reconfigure(E_Smart_Data *sd)
{
   Evas_Coord x, y, w, h, maxw, maxh;
   Evas_List *l;
   
   //if (!sd->changed) return;
   
   x = sd->x + sd->xs;
   y = sd->y + sd->ys;
   w = sd->vw;
   h = sd->vh;   
   maxw = 0;
   maxh = 0;
   
   if (sd->fixed == 0)
    {
       for (l = sd->items; l; l = l->next)
	{
	   E_Icon_Layout_Item *li;
	   Evas_Object *obj;
	   
	   obj = l->data;
	   li = evas_object_data_get(obj, "e_icon_layout_data");
	   
	   if(li->h > maxh) maxh = li->h;

	   if(x > sd->x + w || x + li->w > sd->x + w)
	    {
	       x = sd->x + sd->xs;
	       y += maxh + sd->ys;
	       maxh = 0;
	    }
	   
	   li->x = x;
	   li->y = y;

	   _e_icon_layout_smart_move_resize_item(li);
	   
	   x += li->w + sd->xs;
	   
	}
       
       sd->vh = y - sd->y;
    }
   else
    {
       for (l = sd->items; l; l = l->next)
	{
	   E_Icon_Layout_Item *li;
	   Evas_Object *obj;
	   
	   obj = l->data;
	   li = evas_object_data_get(obj, "e_icon_layout_data");
	   
	   if(li->w > maxw) maxw = li->w;	   
	   
	   if(y > sd->y + h || y + li->h > sd->y + h)
	    {
	       y = sd->y + sd->ys;
	       x += maxw + sd->xs;
	       maxw = 0;
	    }
	   
	   li->x = x;
	   li->y = y;
	   
	   _e_icon_layout_smart_move_resize_item(li);
	   
	   y += li->h + sd->ys;	   
	}
       
       sd->vw = x - sd->x;
    }
   
   sd->xc = x;
   sd->yc = y;      
   
   sd->changed = 0;
}

static void
_e_icon_layout_smart_move_resize_item(E_Icon_Layout_Item *li)
{				       
   if(li->w == 0 || li->h == 0)    
     evas_object_geometry_get(li->obj, NULL, NULL, &li->w, &li->h);
   
   evas_object_move(li->obj, li->x, li->y);
   evas_object_resize(li->obj, li->w, li->h);      
}
				       
static void
_e_icon_layout_smart_init(void)
{
   if (_e_smart) return;
   _e_smart = evas_smart_new("e_icon_layout",
			     _e_icon_layout_smart_add,
			     _e_icon_layout_smart_del,
			     NULL, NULL, NULL, NULL, NULL,
			     _e_icon_layout_smart_move,
			     _e_icon_layout_smart_resize,
			     _e_icon_layout_smart_show,
			     _e_icon_layout_smart_hide,
			     _e_icon_layout_smart_color_set,
			     _e_icon_layout_smart_clip_set,
			     _e_icon_layout_smart_clip_unset,
			     NULL);
}

static void
_e_icon_layout_smart_show(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   
   evas_object_show(sd->clip);
}

static void
_e_icon_layout_smart_hide(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   
   evas_object_hide(sd->clip);
}
   


static void
_e_icon_layout_smart_add(Evas_Object *obj)
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
   sd->xs = 0;
   sd->ys = 0;
   sd->xc = 0;
   sd->yc = 0;
   sd->fixed = 0;
   sd->clip = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_move(sd->clip, 0, 0);
   evas_object_resize(sd->clip, 0, 0);
   evas_object_color_set(sd->clip, 255, 255, 255, 255);
   evas_object_smart_data_set(obj, sd);
   evas_object_smart_member_add(sd->clip, obj);   
   evas_object_show(sd->clip);
}

static void
_e_icon_layout_smart_del(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   while (sd->items)
    {
       Evas_Object *child;
       
       child = sd->items->data;
       e_icon_layout_unpack(child);
    }
   evas_object_del(sd->clip);
   free(sd);
}

static void
_e_icon_layout_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   
   if (!sd) return;
   if ((x == sd->x) && (y == sd->y)) return;
   if ((x == sd->x) && (y == sd->y)) return;
    {
       Evas_List *l;
       Evas_Coord dx, dy;
       
       if(!sd->clip_frozen)
	 evas_object_move(sd->clip, x, y);
       
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
_e_icon_layout_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if ((w == sd->w) && (h == sd->h)) return;
   evas_object_resize(sd->clip, w, h);
   sd->w = w;
   sd->h = h;
   sd->changed = 1;
   _e_icon_layout_smart_reconfigure(sd);        
}
 
static void
_e_icon_layout_smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_color_set(sd->clip, r, g, b, a);
}

static void
_e_icon_layout_smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_clip_set(sd->clip, clip);
}

static void
_e_icon_layout_smart_clip_unset(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_clip_unset(sd->clip);
}
