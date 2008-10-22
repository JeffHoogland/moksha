/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

typedef struct _E_Maximize_Rect E_Maximize_Rect;

struct _E_Maximize_Rect
{
   int x1, y1, x2, y2;
};

#define OBSTACLE(_x1, _y1, _x2, _y2) \
   { \
      r = E_NEW(E_Maximize_Rect, 1); \
      r->x1 = (_x1); r->y1 = (_y1); r->x2 = (_x2); r->y2 = (_y2); \
      rects = eina_list_append(rects, r); \
   }

static void _e_maximize_border_rects_fill(E_Border *bd, Eina_List *list, int *x1, int *y1, int *x2, int *y2, E_Maximize dir);
static void _e_maximize_border_rects_fill_both(E_Border *bd, Eina_List *rects, int *x1, int *y1, int *x2, int *y2); 
static void _e_maximize_border_rects_fill_horiz(E_Border *bd, Eina_List *rects, int *x1, int *x2, int *bx, int *by, int *bw, int *bh);
static void _e_maximize_border_rects_fill_vert(E_Border *bd, Eina_List *rects, int *y1, int *y2, int *bx, int *by, int *bw, int *bh);

EAPI void
e_maximize_border_shelf_fit(E_Border *bd, int *x1, int *y1, int *x2, int *y2, E_Maximize dir)
{
   return e_maximize_border_shelf_fill(bd, x1, y1, x2, y2, dir);
}

EAPI void
e_maximize_border_dock_fit(E_Border *bd, int *x1, int *y1, int *x2, int *y2)
{
   E_Border_List *bl;
   E_Border *bd2;
   int cx1, cx2, cy1, cy2;

   cx1 = bd->zone->x;
   if (x1) cx1 = *x1;
   
   cy1 = bd->zone->y;
   if (y1) cy1 = *y1;
   
   cx2 = bd->zone->x + bd->zone->w;
   if (x2) cx2 = *x2;

   cy2 = bd->zone->y + bd->zone->h;
   if (y2) cy2 = *y2;

   bl = e_container_border_list_first(bd->zone->container);
   while ((bd2 = e_container_border_list_next(bl)))
     {
	enum {
	     NONE,
	     TOP,
	     RIGHT,
	     BOTTOM,
	     LEFT
	} edge = NONE;
	
	if ((bd2->zone != bd->zone) || (bd2 == bd) ||
	    (bd2->client.netwm.type != ECORE_X_WINDOW_TYPE_DOCK))
	  continue;

	if (((bd2->x == bd2->zone->x) || ((bd2->x + bd2->w) == (bd2->zone->x + bd2->zone->w))) &&
	    ((bd2->y == bd2->zone->y) || ((bd2->x + bd2->h) == (bd2->zone->x + bd2->zone->h))))
	  {
	     /* corner */
	     if (bd2->w > bd2->h)
	       {
		  if (bd2->y == bd2->zone->y)
		    edge = TOP;
		  else if ((bd2->y + bd2->h) == (bd2->zone->y + bd2->zone->h))
		    edge = BOTTOM;
	       }
	     else
	       {
		  if ((bd2->x + bd2->w) == (bd2->zone->x + bd2->zone->w))
		    edge = RIGHT;
		  else if (bd2->x == bd2->zone->x)
		    edge = LEFT;
	       }
	  }
	else
	  {
	     if (bd2->y == bd2->zone->y)
	       edge = TOP;
	     else if ((bd2->y + bd2->h) == (bd2->zone->y + bd2->zone->h))
	       edge = BOTTOM;
	     else if (bd2->x == bd2->zone->x)
	       edge = LEFT;
	     else if ((bd2->x + bd2->w) == (bd2->zone->x + bd2->zone->w))
	       edge = RIGHT;
	  }

	switch (edge)
	  {
	   case TOP:
	      if ((bd2->y + bd2->h) > cy1)
		cy1 = (bd2->y + bd2->h);
	      break;
	   case RIGHT:
	      if (bd2->x < cx2)
		cx2 = bd2->x;
	      break;
	   case BOTTOM:
	      if (bd2->y < cy2)
		cy2 = bd2->y;
	      break;
	   case LEFT:
	      if ((bd2->x + bd2->w) > cx1)
		cx1 = (bd2->x + bd2->w);
	      break;
	   case NONE:
	      printf("Crazy people. Dock isn't at the edge.\n");
	      break;
	  }
     }
   e_container_border_list_free(bl);

   if (x1) *x1 = cx1;
   if (y1) *y1 = cy1;
   if (x2) *x2 = cx2;
   if (y2) *y2 = cy2;
}

EAPI void
e_maximize_border_shelf_fill(E_Border *bd, int *x1, int *y1, int *x2, int *y2, E_Maximize dir)
{
   Eina_List *l, *rects = NULL;
   E_Maximize_Rect *r;

   for (l = e_shelf_list(); l; l = l->next)
     {
	E_Shelf *es;
	Eina_List *ll;

	es = l->data;
	if (es->cfg->overlap) continue;
	if (es->zone != bd->zone) continue;
	if (es->cfg->desk_show_mode)
	  {
	     for (ll = es->cfg->desk_list; ll; ll = ll->next)
	       {
		  E_Config_Shelf_Desk *sd;

		  sd = ll->data;
		  if (!sd) continue;
		  if ((sd->x == bd->desk->x) && (sd->y == bd->desk->y))
		    {
		       OBSTACLE(es->x + es->zone->x, es->y + es->zone->y, 
				es->x + es->zone->x + es->w, es->y + es->zone->y + es->h);
		       break;
		    }
	       }
	  }
	else
	  {
	     OBSTACLE(es->x + es->zone->x, es->y + es->zone->y, 
		      es->x + es->zone->x + es->w, es->y + es->zone->y + es->h);
	  }
     }
   if (rects)
     {
	_e_maximize_border_rects_fill(bd, rects, x1, y1, x2, y2, dir);
	for (l = rects; l; l = l->next)
	  free(l->data);
	eina_list_free(rects);
     }
}

EAPI void
e_maximize_border_border_fill(E_Border *bd, int *x1, int *y1, int *x2, int *y2, E_Maximize dir)
{
   Eina_List *l, *rects = NULL;
   E_Border_List *bl;
   E_Maximize_Rect *r;
   E_Border *bd2;

   bl = e_container_border_list_first(bd->zone->container);
   while ((bd2 = e_container_border_list_next(bl)))
     {
	if ((bd2->zone != bd->zone) || (bd == bd2) || (bd2->desk != bd->desk && !bd2->sticky) || (bd2->iconic))
	  continue;
	OBSTACLE(bd2->x, bd2->y, bd2->x + bd2->w, bd2->y + bd2->h);
     }
   e_container_border_list_free(bl);
   if (rects)
     {
	_e_maximize_border_rects_fill(bd, rects, x1, y1, x2, y2, dir);
	for (l = rects; l; l = l->next)
	  free(l->data);
	eina_list_free(rects);
     }
}

static void
_e_maximize_border_rects_fill(E_Border *bd, Eina_List *rects, int *x1, int *y1, int *x2, int *y2, E_Maximize dir)
{
   if ((dir & E_MAXIMIZE_DIRECTION) == E_MAXIMIZE_BOTH)
     {
        _e_maximize_border_rects_fill_both(bd, rects, x1, y1, x2, y2);
     }
   else
     {
        int bx, by, bw, bh;

        bx = bd->x;
        by = bd->y;
        bw = bd->w;
        bh = bd->h;

	if ((dir & E_MAXIMIZE_DIRECTION) == E_MAXIMIZE_HORIZONTAL)
	  _e_maximize_border_rects_fill_horiz(bd, rects, x1, x2, &bx, &by, &bw, &bh);
	else if ((dir & E_MAXIMIZE_DIRECTION) == E_MAXIMIZE_VERTICAL)
	  _e_maximize_border_rects_fill_vert(bd, rects, y1, y2, &bx, &by, &bw, &bh);  
     }
}

static void
_e_maximize_border_rects_fill_both(E_Border *bd, Eina_List *rects, int *x1, int *y1, int *x2, int *y2)
{
   int hx1, hy1, hx2, hy2;
   int vx1, vy1, vx2, vy2;
   int bx, by, bw, bh;

   hx1 = vx1 = bd->zone->x;
   if (x1) hx1 = vx1 = *x1;
   
   hy1 = vy1 = bd->zone->y;
   if (y1) hy1 = vy1 = *y1;
   
   hx2 = vx2 = bd->zone->x + bd->zone->w;
   if (x2) hx2 = vx2 = *x2;

   hy2 = vy2 = bd->zone->y + bd->zone->h;
   if (y2) hy2 = vy2 = *y2;

   /* Init working values, try maximizing horizontally first */
   bx = bd->x;
   by = bd->y;
   bw = bd->w;
   bh = bd->h;
   _e_maximize_border_rects_fill_horiz(bd, rects, &hx1, &hx2, &bx, &by, &bw, &bh);
   _e_maximize_border_rects_fill_vert(bd, rects, &hy1, &hy2, &bx, &by, &bw, &bh);

   /* Reset working values, try maximizing vertically first */
   bx = bd->x;
   by = bd->y;
   bw = bd->w;
   bh = bd->h;
   _e_maximize_border_rects_fill_vert(bd, rects, &vy1, &vy2, &bx, &by, &bw, &bh);
   _e_maximize_border_rects_fill_horiz(bd, rects, &vx1, &vx2, &bx, &by, &bw, &bh);

   /* Use the result set that gives the largest volume */
   if (((hx2 - hx1) * (hy2 - hy1)) > ((vx2 - vx1) * (vy2 - vy1)))
     {
        if (x1) *x1 = hx1;
	if (y1) *y1 = hy1;
	if (x2) *x2 = hx2;
	if (y2) *y2 = hy2;
     }
   else
     {
        if (x1) *x1 = vx1;
	if (y1) *y1 = vy1;
	if (x2) *x2 = vx2;
	if (y2) *y2 = vy2;
     }
}

static void
_e_maximize_border_rects_fill_horiz(E_Border *bd, Eina_List *rects, int *x1, int *x2, int *bx, int *by, int *bw, int *bh)
{
   Eina_List *l;
   int cx1, cx2;

   cx1 = bd->zone->x;
   if (x1) cx1 = *x1;
   
   cx2 = bd->zone->x + bd->zone->w;
   if (x2) cx2 = *x2;

   /* Expand left */
   for (l = rects; l; l = l->next)
     {
	E_Maximize_Rect *rect;

	rect = l->data;
	if ((rect->x2 > cx1) && (rect->x2 <= *bx) &&
	    E_INTERSECTS(0, rect->y1, bd->zone->w, (rect->y2 - rect->y1), 0, *by, bd->zone->w, *bh))
	  {
	     cx1 = rect->x2;
	  }
     }
   *bw += (*bx - cx1);
   *bx = cx1;

   /* Expand right */
   for (l = rects; l; l = l->next)
     {
	E_Maximize_Rect *rect;

	rect = l->data;
	if ((rect->x1 < cx2) && (rect->x1 >= (*bx + *bw)) &&
	    E_INTERSECTS(0, rect->y1, bd->zone->w, (rect->y2 - rect->y1), 0, *by, bd->zone->w, *bh))
	  {
	     cx2 = rect->x1;
	  }
     }
   *bw = (cx2 - cx1);
 
   if (x1) *x1 = cx1;
   if (x2) *x2 = cx2;
}

static void
_e_maximize_border_rects_fill_vert(E_Border *bd, Eina_List *rects, int *y1, int *y2, int *bx, int *by, int *bw, int *bh)
{
   Eina_List *l;
   int cy1, cy2;

   cy1 = bd->zone->y;
   if (y1) cy1 = *y1;
   
   cy2 = bd->zone->y + bd->zone->h;
   if (y2) cy2 = *y2;

   /* Expand up */
   for (l = rects; l; l = l->next)
     {
	E_Maximize_Rect *rect;

	rect = l->data;
	if ((rect->y2 > cy1) && (rect->y2 <= *by) &&
	    E_INTERSECTS(rect->x1, 0, (rect->x2 - rect->x1), bd->zone->h, *bx, 0, *bw, bd->zone->h))
	  {
	     cy1 = rect->y2;
	  }
     }
   *bh += (*by - cy1);
   *by = cy1;

   /* Expand down */
   for (l = rects; l; l = l->next)
     {
	E_Maximize_Rect *rect;

	rect = l->data;
	if ((rect->y1 < cy2) && (rect->y1 >= (*by + *bh)) &&
	    E_INTERSECTS(rect->x1, 0, (rect->x2 - rect->x1), bd->zone->h, *bx, 0, *bw, bd->zone->h))
	  {
	     cy2 = rect->y1;
	  }
     }
   *bh = (cy2 - cy1);

   if (y1) *y1 = cy1;
   if (y2) *y2 = cy2;
}

