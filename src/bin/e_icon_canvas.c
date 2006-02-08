#include "e.h"

#define TILE_SIZE 512
#define TILE_NUM  20

typedef struct _E_Smart_Data E_Smart_Data;
typedef struct _E_Icon_Canvas_Item E_Icon_Canvas_Item;
typedef struct _E_Icon_Canvas_Tile E_Icon_Canvas_Tile;

struct _E_Smart_Data
{
   Evas_Coord       x, y, w, h;
   Evas_Coord       vw, vh;
   Evas_Coord       xs, ys;
   Evas_Coord       xc, yc;
   Evas_Coord       mw, mh;
   Evas_Object     *clip;
   Evas_Object     *obj;
   int              frozen;
   int              xy_frozen;
   int              fixed;
   unsigned char    changed : 1;
   Evas_List       *items;
   
   E_Icon_Canvas_Tile *tiles[TILE_NUM][TILE_NUM];
   
   struct {
      Evas_Object *obj;
      Evas_Coord   x, y, w, h;
   } viewport;
};

struct _E_Icon_Canvas_Item
{
   E_Smart_Data       *sd;
   Evas_Coord          x, y, w, h;
   Evas_Object        *obj;
   Evas_Object      *(*create)(void *data);
   void              (*destroy)(Evas_Object *obj, void *data);
   void               *data;
   E_Icon_Canvas_Tile *tile;
};

struct _E_Icon_Canvas_Tile
{
   E_Smart_Data  *sd;
   Evas_List     *items;   
   Evas_Coord     x, y, w, h;
   unsigned char  visible :1;
};
   
/* local subsystem functions */
static E_Icon_Canvas_Item *_e_icon_canvas_adopt(E_Smart_Data *sd, Evas_Object *obj);
static void                _e_icon_canvas_disown(Evas_Object *obj);
static void                _e_icon_canvas_item_del_hook(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void                _e_icon_canvas_reconfigure(E_Smart_Data *sd);
static void                _e_icon_canvas_move_resize_item(E_Icon_Canvas_Item *li);
static void                _e_icon_canvas_pack(E_Smart_Data *sd, E_Icon_Canvas_Item *li);
static E_Icon_Canvas_Tile *_e_icon_canvas_tile_get_at_coord(E_Smart_Data *sd, Evas_Coord x, Evas_Coord y);
static E_Icon_Canvas_Tile *_e_icon_canvas_tile_add(E_Smart_Data *sd, Evas_Coord x, Evas_Coord y);
static void                _e_icon_canvas_tile_move(E_Icon_Canvas_Tile *t, Evas_Coord x, Evas_Coord y);
static void                _e_icon_canvas_tile_show(E_Icon_Canvas_Tile *t);
static void                _e_icon_canvas_tile_hide(E_Icon_Canvas_Tile *t);
static void                _e_icon_canvas_tile_pack(E_Icon_Canvas_Tile *t, E_Icon_Canvas_Item *li);
static void                _e_icon_canvas_icon_show(E_Icon_Canvas_Item *li);
static void                _e_icon_canvas_icon_hide(E_Icon_Canvas_Item *li);

static void _e_icon_canvas_smart_init(void);
static void _e_icon_canvas_smart_add(Evas_Object *obj);
static void _e_icon_canvas_smart_show(Evas_Object *obj);
static void _e_icon_canvas_smart_hide(Evas_Object *obj);
static void _e_icon_canvas_smart_del(Evas_Object *obj);
static void _e_icon_canvas_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _e_icon_canvas_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static void _e_icon_canvas_smart_color_set(Evas_Object *obj, int r, int g, int b, int a);
static void _e_icon_canvas_smart_clip_set(Evas_Object *obj, Evas_Object *clip);
static void _e_icon_canvas_smart_clip_unset(Evas_Object *obj);

/* local subsystem globals */
static Evas_Smart *_e_smart = NULL;

/* externally accessible functions */
EAPI Evas_Object *
e_icon_canvas_add(Evas *evas)
{
   _e_icon_canvas_smart_init();
   return evas_object_smart_add(evas, _e_smart);
}

EAPI int
e_icon_canvas_freeze(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   if ((!obj) || !(sd = evas_object_smart_data_get(obj)))
     return -1;
   
   sd->frozen++;
   return sd->frozen;
}

EAPI int
e_icon_canvas_thaw(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   if ((!obj) || !(sd = evas_object_smart_data_get(obj)))
     return -1;
         
   sd->frozen--;
   if (sd->frozen <= 0) _e_icon_canvas_reconfigure(sd);
   return sd->frozen;
}

EAPI void
e_icon_canvas_virtual_size_get(Evas_Object *obj, Evas_Coord *w, Evas_Coord *h)
{
   E_Smart_Data *sd;
   
   if ((!obj) || !(sd = evas_object_smart_data_get(obj)))
     return;
   
   if (w) *w = sd->vw;
   if (h) *h = sd->vh;
}

EAPI void
e_icon_canvas_width_fix(Evas_Object *obj, Evas_Coord w)
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
   if (sd->frozen <= 0) _e_icon_canvas_reconfigure(sd);
}

EAPI void
e_icon_canvas_height_fix(Evas_Object *obj, Evas_Coord h)
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
   if (sd->frozen <= 0) _e_icon_canvas_reconfigure(sd);
}

EAPI void
e_icon_canvas_sort(Evas_Object *obj, int (*func)(void *d1, void *d2))
{
#if 0   
   E_Smart_Data *sd;
   
   if ((!obj) || !(sd = evas_object_smart_data_get(obj)))
     return;
   
   sd->items = evas_list_sort(sd->items, evas_list_count(sd->items), func);
   _e_icon_canvas_reconfigure(sd);
#endif   
}
   
EAPI void
e_icon_canvas_icon_callbacks_set(Evas_Object *child, void (*appear)(Evas_Object *obj, void *data), void (*disappear)(Evas_Object *obj, void *data), void *data)
{
#if 0   
   E_Icon_Canvas_Item *li;
   
   li = evas_object_data_get(child, "e_icon_canvas_data");
   if (!li) return;
   
   li->appear_func = appear;
   li->disappear_func = disappear;
   li->data = data;
#endif   
}

EAPI void
e_icon_canvas_pack_at_location(Evas_Object *obj, Evas_Object *child, Evas_Object *(*create)(void *data), void (*destroy)(Evas_Object *obj, void *data), void *data, Evas_Coord x, Evas_Coord y)
{    
#if 0   
   E_Smart_Data *sd;   
   E_Icon_Canvas_Item *li;
   
   if ((!obj) || !(sd = evas_object_smart_data_get(obj)))
     return;
   
   li = _e_icon_canvas_adopt(sd, child);
   li->create = create;
   li->destroy = destroy;
   li->data = data;
   sd->items = evas_list_append(sd->items, li);
   
   li->x = x;
   li->y = y;   
   
   if(li->x + li->w > sd->vw)
     sd->vw = li->x + li->w;
   if(li->y + li->h > sd->vh)
     sd->vh = li->y + li->h;   
   
   _e_icon_canvas_pack(sd, li);
#endif

   e_icon_canvas_pack(obj, child, create, destroy, data);
   
}

EAPI void
e_icon_canvas_pack(Evas_Object *obj, Evas_Object *child, Evas_Object *(*create)(void *data), void (*destroy)(Evas_Object *obj, void *data), void *data)
{
   E_Smart_Data *sd;
   E_Icon_Canvas_Item *li;
   
   if ((!obj) || !(sd = evas_object_smart_data_get(obj)))
     return;
      
   li = _e_icon_canvas_adopt(sd, child);
   li->create = create;
   li->destroy = destroy;
   li->data = data;
   sd->items = evas_list_append(sd->items, li);

   if (sd->fixed == 0)
    {	   
       if(li->h > sd->mh) sd->mh = li->h;
       if(sd->xc > sd->x + sd->vw || sd->xc + li->w > sd->x + sd->vw)
	 {
	    sd->xc = sd->x + sd->xs;
	    sd->yc += sd->mh + sd->ys;
	    sd->mh = 0;
	 }
       
       li->x = sd->xc;
       li->y = sd->yc;
       	   
       sd->xc += li->w + sd->xs;              
       sd->vh = (sd->yc - sd->y) + li->h;
    }       
   else
     {
	if(li->w > sd->mw) sd->mw = li->w;	   
	
	if(sd->yc > sd->y + sd->vh || sd->yc + li->h > sd->y + sd->vh)
	  {
	     sd->yc = sd->y + sd->ys;
	     sd->xc += sd->mw + sd->xs;
	     sd->mw = 0;
	  }
	
	li->x = sd->xc;
	li->y = sd->yc;
		
	sd->yc += li->h + sd->ys;
	sd->vw = sd->xc - sd->x;	
     }

   _e_icon_canvas_pack(sd, li);
}

EAPI void
e_icon_canvas_child_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Icon_Canvas_Item *li;
   
   li = evas_object_data_get(obj, "e_icon_canvas_data");
   if (!li) return;
   if (w < 0) w = 0;
   if (h < 0) h = 0;
   if ((li->w == w) && (li->h == h)) return;
   li->w = w;
   li->h = h;
   _e_icon_canvas_move_resize_item(li);
}

EAPI void         
e_icon_canvas_child_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   E_Icon_Canvas_Item *li;
   
   li = evas_object_data_get(obj, "e_icon_canvas_data");
   if(x < 0) x = 0;
   if(y < 0) y = 0;
   li->x = x;
   li->y = y;
   _e_icon_canvas_move_resize_item(li);
}

EAPI void
e_icon_canvas_unpack(Evas_Object *obj)
{
   E_Icon_Canvas_Item *li;
   E_Smart_Data *sd;
   
   if(!obj) return;
   li = evas_object_data_get(obj, "e_icon_canvas_data");
   if (!li) return;
   
   sd = li->sd;         
   sd->items = evas_list_remove(sd->items, li);
   _e_icon_canvas_disown(obj);

   if (!li->tile) return;
   if (!li->tile->items) return;   
   li->tile->items = evas_list_remove(li->tile->items, li);
}

EAPI void
e_icon_canvas_spacing_set(Evas_Object *obj, Evas_Coord xs, Evas_Coord ys)
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
   
   if (sd->frozen <= 0) _e_icon_canvas_reconfigure(sd);
}

EAPI void         
e_icon_canvas_redraw_force(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   if ((!obj) || !(sd = evas_object_smart_data_get(obj)))
     return;
   
   sd->changed = 1;
   
   if (sd->frozen <= 0) _e_icon_canvas_reconfigure(sd);   
}

EAPI void
e_icon_canvas_reset(Evas_Object *obj)
{
   E_Smart_Data *sd;
   int i, j;
   
   if ((!obj) || !(sd = evas_object_smart_data_get(obj)))
     return;

   sd->xc = sd->viewport.x + sd->xs;
   sd->yc = sd->viewport.y + sd->ys;
   sd->mw = 0;
   sd->mh = 0;

   while (sd->items)
     {
	E_Icon_Canvas_Item *li;
	
	li = sd->items->data;
	if(li->obj)
	  e_icon_canvas_unpack(li->obj);
	else
	  sd->items = evas_list_remove(sd->items, li);
	//free(li);
     }   
   
   for(i = 0; i < TILE_NUM; i++)
     {
	for(j = 0; j < TILE_NUM; j++)
	  {
	     if(sd->tiles[i][j])
	       {
		  E_Icon_Canvas_Tile *t;
		  
		  t = sd->tiles[i][j];
		  while(t->items)		    
		    t->items = evas_list_remove(t->items, t->items->data);
		  free(sd->tiles[i][j]);
		  sd->tiles[i][j] = NULL;
	       }
	  }
     }
   
   sd->x = sd->viewport.x;
   sd->y = sd->viewport.y;
}

EAPI void
e_icon_canvas_xy_freeze(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   if ((!obj) || !(sd = evas_object_smart_data_get(obj)))
     return;
   
   sd->xy_frozen = 1;
}

EAPI void
e_icon_canvas_xy_thaw(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   if ((!obj) || !(sd = evas_object_smart_data_get(obj)))
     return;
   
   sd->xy_frozen = 0;
}

EAPI void
e_icon_canvas_viewport_set(Evas_Object *obj, Evas_Object *viewport)
{
   E_Smart_Data *sd;

   if ((!obj) || !(sd = evas_object_smart_data_get(obj)))
     return;
   
   if(!viewport) return;
   sd->viewport.obj = viewport;
   evas_object_geometry_get(sd->viewport.obj, &(sd->viewport.x), &(sd->viewport.y),
			    &(sd->viewport.w), &(sd->viewport.h));
   if (sd->frozen <= 0) _e_icon_canvas_reconfigure(sd);
}

EAPI Evas_Object *
e_icon_canvas_viewport_get(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   if ((!obj) || !(sd = evas_object_smart_data_get(obj)))
     return NULL;
   
   return sd->viewport.obj;   
}


/* local subsystem functions */
static E_Icon_Canvas_Item *
_e_icon_canvas_adopt(E_Smart_Data *sd, Evas_Object *obj)
{
   E_Icon_Canvas_Item *li;
   
   li = calloc(1, sizeof(E_Icon_Canvas_Item));
   if (!li) return NULL;
   li->sd = sd;
   li->obj = obj;
   li->create = NULL;
   li->destroy = NULL;
   li->data = NULL;
   evas_object_geometry_get(obj, NULL, NULL, &li->w, &li->h);
   /* defaults */
   li->x = 0;
   li->y = 0;
   evas_object_clip_set(obj, sd->clip);
   evas_object_smart_member_add(obj, li->sd->obj);
   evas_object_show(obj);
   evas_object_data_set(obj, "e_icon_canvas_data", li);
   if ((!evas_object_visible_get(sd->clip)) &&
       (evas_object_visible_get(sd->obj)))
     evas_object_show(sd->clip);
   return li;
}

static void
_e_icon_canvas_disown(Evas_Object *obj)
{
   E_Icon_Canvas_Item *li;
   
   li = evas_object_data_get(obj, "e_icon_canvas_data");
   if (!li) return;
   if (!li->sd->items)
    {
       if (evas_object_visible_get(li->sd->clip))
	 evas_object_hide(li->sd->clip);
    }
   evas_object_smart_member_del(obj);
   evas_object_data_del(obj, "e_icon_canvas_data");
   free(li);
}

static void
_e_icon_canvas_item_del_hook(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   e_icon_canvas_unpack(obj);
}

static void
_e_icon_canvas_reconfigure(E_Smart_Data *sd)
{
   int i, j;

   if(!sd->changed) return;
   
   for(i = 0; i < TILE_NUM; i++)
     {
	for(j = 0; j < TILE_NUM; j++)
	  {
	     if(sd->tiles[i][j])
	       {
		  E_Icon_Canvas_Tile *t;
		  
		  t = sd->tiles[i][j];
		  
		  if(E_INTERSECTS(sd->viewport.x, sd->viewport.y,
				  sd->viewport.w, sd->viewport.h,
				  t->x, t->y, t->w, t->h))
		    {
		       if(t->visible == 1)
			 continue;
		       _e_icon_canvas_tile_show(t);		       
		    }
		  else
		    {
		       if(!t->visible)
			 continue;
		       _e_icon_canvas_tile_hide(t);		       
		    }
	       }
	  }
     }   
   
   sd->changed = 0;
}

E_Icon_Canvas_Tile *
_e_icon_canvas_tile_get_at_coord(E_Smart_Data *sd, Evas_Coord x, Evas_Coord y)
{
   int tx, ty;
   
   if(x == 0)
     tx = 0;
   else
     tx = (int)ceil((double)x / (double)TILE_SIZE) - 1;
   if(y == 0)
     ty = 0;
   else
     ty = (int)ceil((double)y / (double)TILE_SIZE) - 1;
   
   if(tx >= TILE_NUM || ty >= TILE_NUM)
     return NULL;
   
   if(sd->tiles[tx][ty])    
	return sd->tiles[tx][ty];     
   
   return NULL;
}

E_Icon_Canvas_Tile *
_e_icon_canvas_tile_add(E_Smart_Data *sd, Evas_Coord x, Evas_Coord y)
{
   int tx, ty;
   E_Icon_Canvas_Tile *tile;

   if(x < 0 || y < 0)
     return NULL;
   
   if(x == 0)
     tx = 0;
   else
     tx = (int)ceil((double)x / (double)TILE_SIZE) - 1;
   if(y == 0)
     ty = 0;
   else
     ty = (int)ceil((double)y / (double)TILE_SIZE) - 1;
   
   if(tx >= TILE_NUM || ty >= TILE_NUM)
     return NULL;

   tile = E_NEW(E_Icon_Canvas_Tile, 1);
   tile->x = (tx) * TILE_SIZE;
   tile->y = (ty) * TILE_SIZE;
   tile->w = TILE_SIZE;
   tile->h = TILE_SIZE;
   tile->visible = 0;
   tile->sd = sd;   
   tile->items = NULL;
   sd->tiles[tx][ty] = tile;

   if(E_INTERSECTS(sd->viewport.x, sd->viewport.y,
		   sd->viewport.w, sd->viewport.h,
		   tile->x, tile->y, tile->w, tile->h))
     {
	_e_icon_canvas_tile_show(tile);
     }
      
   return tile;
}

static void
_e_icon_canvas_tile_move(E_Icon_Canvas_Tile *t, Evas_Coord x, Evas_Coord y)
{
   Evas_List *l;
   Evas_Coord dx, dy;
   
   dx = x - t->x;
   dy = y - t->y;
   
   t->x = x;
   t->y = y;
   
   for(l = t->items; l; l = l->next)
     {
	E_Icon_Canvas_Item *li;
	
	li = l->data;
	li->x += dx;
	li->y += dy;
	if(li->obj)
	  evas_object_move(li->obj, li->x, li->y);
     }
}

static void
_e_icon_canvas_tile_show(E_Icon_Canvas_Tile *t)
{
   Evas_List *l;
   
   if(t->visible == 1) return;   
   t->visible = 1;
   for(l = t->items; l; l = l->next)
     _e_icon_canvas_icon_show(l->data);
}

static void
_e_icon_canvas_tile_hide(E_Icon_Canvas_Tile *t)
{
   Evas_List *l;
   
   if(t->visible == 0) return;   
   t->visible = 0;
   for(l = t->items; l; l = l->next)     
     _e_icon_canvas_icon_hide(l->data);     
}

void
_e_icon_canvas_tile_pack(E_Icon_Canvas_Tile *t, E_Icon_Canvas_Item *li)
{   
   if(!t)
     return;
   t->items = evas_list_append(t->items, li);
   li->tile = t;
   if(t->visible)     
     _e_icon_canvas_icon_show(li);
   else
     _e_icon_canvas_icon_hide(li);
}

void
_e_icon_canvas_icon_show(E_Icon_Canvas_Item *li)
{
   if(!li->obj && li->create && li->data)
     {
	li->obj = li->create(li->data);
	if(!li->obj) return;
	evas_object_smart_member_add(li->obj, li->sd->obj);
	evas_object_data_set(li->obj, "e_icon_canvas_data", li);
	evas_object_clip_set(li->obj, li->sd->clip);
     }
   evas_object_move(li->obj, li->x, li->y);
   evas_object_show(li->obj);
}

void
_e_icon_canvas_icon_hide(E_Icon_Canvas_Item *li)
{
   if(li->obj && li->destroy)
     li->destroy(li->obj, li->data);
   li->obj = NULL;
}

void
_e_icon_canvas_pack(E_Smart_Data *sd, E_Icon_Canvas_Item *li)
{    
   E_Icon_Canvas_Tile *t;
    
   if((t =_e_icon_canvas_tile_get_at_coord(sd, li->x, li->y)) == NULL)
     t = _e_icon_canvas_tile_add(sd, li->x, li->y);
   _e_icon_canvas_move_resize_item(li);
   _e_icon_canvas_tile_pack(t, li);
}

static void
_e_icon_canvas_move_resize_item(E_Icon_Canvas_Item *li)
{				       

   if(li->w == 0 || li->h == 0)
     {
	evas_object_geometry_get(li->obj, NULL, NULL, &li->w, &li->h);
	evas_object_resize(li->obj, li->w, li->h);
     }
   
   evas_object_move(li->obj, li->x, li->y);   
}
				       
static void
_e_icon_canvas_smart_init(void)
{
   if (_e_smart) return;
   _e_smart = evas_smart_new("e_icon_canvas",
			     _e_icon_canvas_smart_add,
			     _e_icon_canvas_smart_del,
			     NULL, NULL, NULL, NULL, NULL,
			     _e_icon_canvas_smart_move,
			     _e_icon_canvas_smart_resize,
			     _e_icon_canvas_smart_show,
			     _e_icon_canvas_smart_hide,
			     _e_icon_canvas_smart_color_set,
			     _e_icon_canvas_smart_clip_set,
			     _e_icon_canvas_smart_clip_unset,
			     NULL);
}

static void
_e_icon_canvas_smart_show(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);   
   evas_object_show(sd->clip);
}

static void
_e_icon_canvas_smart_hide(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   evas_object_hide(sd->clip);
}
   


static void
_e_icon_canvas_smart_add(Evas_Object *obj)
{
   E_Smart_Data *sd;
   int i, j;
   
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
   sd->mw = 0;
   sd->mh = 0;
   sd->fixed = 0;   
   sd->viewport.obj = NULL;
   for(i = 0; i < TILE_NUM; i++)     
     for(j = 0; j < TILE_NUM; j++)	  
       sd->tiles[i][j] = NULL;	       
   sd->clip = evas_object_rectangle_add(evas_object_evas_get(obj));
   evas_object_move(sd->clip, 0, 0);
   evas_object_resize(sd->clip, 0, 0);
   evas_object_color_set(sd->clip, 255, 255, 255, 255);
   evas_object_smart_data_set(obj, sd);
   evas_object_smart_member_add(sd->clip, obj);   
   evas_object_show(sd->clip);
}

static void
_e_icon_canvas_smart_del(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   while (sd->items)
    {
       E_Icon_Canvas_Item *li;
       
       li = sd->items->data;
       if(li->obj)
	 e_icon_canvas_unpack(li->obj);
       else
	 sd->items = evas_list_remove(sd->items, li);
       //free(li);
    }
   evas_object_del(sd->clip);
   free(sd);
}

static void
_e_icon_canvas_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   E_Smart_Data *sd;
   int i, j;
   Evas_Coord dx, dy;
   
   sd = evas_object_smart_data_get(obj);
   
   if (!sd) return;
   if ((x == sd->x) && (y == sd->y)) return;
   if(sd->viewport.obj)
     evas_object_geometry_get(sd->viewport.obj, 
			      &(sd->viewport.x), &(sd->viewport.y),
			      &(sd->viewport.w), &(sd->viewport.h));

   if (!sd->xy_frozen)
     evas_object_move(sd->clip, x, y);
   
   dx = x - sd->x;
   dy = y - sd->y;   
      
   for(i = 0; i < TILE_NUM; i++)
     {
	for(j = 0; j < TILE_NUM; j++)
	  {
	     if(sd->tiles[i][j])
	       {
		  E_Icon_Canvas_Tile *t;
		  
		  t = sd->tiles[i][j];
		  
		  _e_icon_canvas_tile_move(t, t->x + dx, t->y + dy);
		  
		  if(E_INTERSECTS(sd->viewport.x, sd->viewport.y,
				  sd->viewport.w, sd->viewport.h,
				  t->x, t->y, t->w, t->h))
		    {
		       if(t->visible == 1 && !sd->xy_frozen)
			 continue;
		       _e_icon_canvas_tile_show(t);
		    }
		  else
		    {
		       if(!t->visible)
			 continue;
		       _e_icon_canvas_tile_hide(t);
		    }
	       }
	  }
     }
		          
   sd->x = x;
   sd->y = y;
}

static void
_e_icon_canvas_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Smart_Data *sd;

   if(!obj) return;
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   if ((w == sd->w) && (h == sd->h)) return;
   evas_object_resize(sd->clip, w, h);
   sd->w = w;
   sd->h = h;
   sd->changed = 1;
   if(sd->viewport.obj)
     evas_object_geometry_get(sd->viewport.obj, 
			      &(sd->viewport.x), &(sd->viewport.y),
			      &(sd->viewport.w), &(sd->viewport.h));
   
   sd->changed = 1;
   _e_icon_canvas_reconfigure(sd);   
}
 
static void
_e_icon_canvas_smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_color_set(sd->clip, r, g, b, a);
}

static void
_e_icon_canvas_smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_clip_set(sd->clip, clip);
}

static void
_e_icon_canvas_smart_clip_unset(Evas_Object *obj)
{
   E_Smart_Data *sd;
   
   sd = evas_object_smart_data_get(obj);
   if (!sd) return;
   evas_object_clip_unset(sd->clip);
}
