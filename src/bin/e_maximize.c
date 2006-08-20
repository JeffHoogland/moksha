/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* FIXME: this code is quite bad.. formatting, correctness, list management - 
 * overly long and complex... */

typedef struct _E_Maximize_Rect E_Maximize_Rect;

struct _E_Maximize_Rect
{
   int x1, y1, x2, y2;
};

#define OBSTACLE(_x1, _y1, _x2, _y2) \
   { \
      r = E_NEW(E_Maximize_Rect, 1); \
      r->x1 = (_x1); r->y1 = (_y1); r->x2 = (_x2); r->y2 = (_y2); \
      rects = evas_list_append(rects, r); \
   }

static void _e_maximize_border_rects_fill(E_Border *bd, Evas_List *list, int *x1, int *y1, int *x2, int *y2);

EAPI void
e_maximize_border_gadman_fit(E_Border *bd, int *x1, int *y1, int *x2, int *y2)
{
   return e_maximize_border_gadman_fill(bd, x1, y1, x2, y2);
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
e_maximize_border_gadman_fill(E_Border *bd, int *x1, int *y1, int *x2, int *y2)
{
   Evas_List *l, *rects = NULL;
   E_Maximize_Rect *r;

   for (l = e_shelf_list(); l; l = l->next)
     {
	E_Shelf *es;

	es = l->data;
	if (es->zone != bd->zone) continue;
	OBSTACLE(es->x, es->y, es->x + es->w, es->y + es->h);
     }
   if (rects)
     {
	_e_maximize_border_rects_fill(bd, rects, x1, y1, x2, y2);
	for (l = rects; l; l = l->next)
	  free(l->data);
	evas_list_free(rects);
     }
}

EAPI void
e_maximize_border_border_fill(E_Border *bd, int *x1, int *y1, int *x2, int *y2)
{
   Evas_List *l, *rects = NULL;
   E_Border_List *bl;
   E_Maximize_Rect *r;
   E_Border *bd2;

   bl = e_container_border_list_first(bd->zone->container);
   while ((bd2 = e_container_border_list_next(bl)))
     {
	if ((bd2->zone != bd->zone) ||
	    (bd == bd2))
	  continue;
	OBSTACLE(bd2->x, bd2->y, bd2->x + bd2->w, bd2->y + bd2->h);
     }
   e_container_border_list_free(bl);
   if (rects)
     {
	_e_maximize_border_rects_fill(bd, rects, x1, y1, x2, y2);
	for (l = rects; l; l = l->next)
	  free(l->data);
	evas_list_free(rects);
     }
}

static void
_e_maximize_border_rects_fill(E_Border *bd, Evas_List *rects, int *x1, int *y1, int *x2, int *y2)
{
   Evas_List *l;
   int bx, by, bw, bh;
   int cx1, cx2, cy1, cy2;

   cx1 = bd->zone->x;
   if (x1) cx1 = *x1;
   
   cy1 = bd->zone->y;
   if (y1) cy1 = *y1;
   
   cx2 = bd->zone->x + bd->zone->w;
   if (x2) cx2 = *x2;

   cy2 = bd->zone->y + bd->zone->h;
   if (y2) cy2 = *y2;

   /* Loop four times. We expand left, up, right, down. */
   /* FIXME: The right order? */
   bx = bd->x;
   by = bd->y;
   bw = bd->w;
   bh = bd->h;
   for (l = rects; l; l = l->next)
     {
	/* expand left */
	E_Maximize_Rect *rect;

	rect = l->data;
	if ((rect->x2 > cx1) && (rect->x2 <= bx) &&
	    E_INTERSECTS(0, rect->y1, bd->zone->w, (rect->y2 - rect->y1), 0, by, bd->zone->w, bh))
	  {
	     cx1 = rect->x2;
	  }
     }
   bw += (bx - cx1);
   bx = cx1;
   for (l = rects; l; l = l->next)
     {
	/* expand up */
	E_Maximize_Rect *rect;

	rect = l->data;
	if ((rect->y2 > cy1) && (rect->y2 <= by) &&
	    E_INTERSECTS(rect->x1, 0, (rect->x2 - rect->x1), bd->zone->h, bx, 0, bw, bd->zone->h))
	  {
	     cy1 = rect->y2;
	  }
     }
   bh += (by - cy1);
   by = cy1;
   for (l = rects; l; l = l->next)
     {
	/* expand right */
	E_Maximize_Rect *rect;

	rect = l->data;
	if ((rect->x1 < cx2) && (rect->x1 >= (bx + bw)) &&
	    E_INTERSECTS(0, rect->y1, bd->zone->w, (rect->y2 - rect->y1), 0, by, bd->zone->w, bh))
	  {
	     cx2 = rect->x1;
	  }
     }
   bw = (cx2 - cx1);
   for (l = rects; l; l = l->next)
     {
	/* expand down */
	E_Maximize_Rect *rect;

	rect = l->data;
	if ((rect->y1 < cy2) && (rect->y1 >= (by + bh)) &&
	    E_INTERSECTS(rect->x1, 0, (rect->x2 - rect->x1), bd->zone->h, bx, 0, bw, bd->zone->h))
	  {
	     cy2 = rect->y1;
	  }
     }
   bh = (cy2 - cy1);

   if (x1) *x1 = cx1;
   if (y1) *y1 = cy1;
   if (x2) *x2 = cx2;
   if (y2) *y2 = cy2;
}
