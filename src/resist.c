#include "e.h"

void
e_resist_border(E_Border *b)
{
   int resist = 1;
   int desk_resist = 32;
   int win_resist = 12;
   int ok = 0;
   int dx, dy, d;
   int resist_x = 0, resist_y = 0;
   Evas_List l, rects = NULL;
   E_Rect *r;
   E_CFG_INT(cfg_resist, "settings", "/move/resist", 1);
   E_CFG_INT(cfg_desk_resist, "settings", "/move/resist/desk", 32);
   E_CFG_INT(cfg_win_resist, "settings", "/move/resist/win", 12);
   
   E_CONFIG_INT_GET(cfg_resist, resist);
   E_CONFIG_INT_GET(cfg_desk_resist, desk_resist);
   E_CONFIG_INT_GET(cfg_win_resist, win_resist);
   if (!resist)
     {
	b->current.x = b->current.requested.x;
	b->current.y = b->current.requested.y;
	return;
     }
   if (!b->desk) return;
   dx = b->current.requested.x - b->previous.requested.x;
   dy = b->current.requested.y - b->previous.requested.y;
   /* edges of screen */
#define OBSTACLE(_x, _y, _w, _h, _resist) \
{ \
r = NEW(E_Rect, 1); \
r->x = _x; r->y = _y; r->w = _w; r->h = _h; r->v1 = _resist; \
rects = evas_list_append(rects, r); \
}
   OBSTACLE(-1000000, -1000000, 2000000 + b->desk->real.w, 1000000, desk_resist); 
   OBSTACLE(-1000000, -1000000, 1000000, 2000000 + b->desk->real.h, desk_resist); 
   OBSTACLE(-1000000, b->desk->real.h, 2000000 + b->desk->real.w, 1000000, desk_resist); 
   OBSTACLE(b->desk->real.w, -1000000, 1000000, 2000000 + b->desk->real.h, desk_resist); 
   /* can add code here to add more fake obstacles with custom resist values */
   /* here if need be - ie xinerama middle between screens and panels etc. */
  
   for (l = b->desk->windows; l; l = l->next)
     {
	E_Border *bd;
	
	bd = l->data;
	if (bd != b)
	  {
	     r = NEW(struct _E_Rect, 1);
	     r->x = bd->current.x;
	     r->y = bd->current.y;
	     r->w = bd->current.w;
	     r->h = bd->current.h;
	     r->v1 = win_resist;
	     rects = evas_list_append(rects, r);
	  }
     }
   for (l = rects; l; l = l->next)
     {
	r = l->data;
	if (SPANS_COMMON(r->y, r->h, b->current.requested.y, b->current.h))
	  {
	     if (dx > 0)
	       {
		  /* moving right - check left edge of windows against right */
		  d = r->x - (b->current.requested.x + b->current.w);
		  if ((d < 0) && (d >= - r->v1)) 
		    {
		       if (resist_x > d) resist_x = d;
		    }
	       }
	     else if (dx < 0)
	       {
		  /* moving left - check right edge of windows against left */
		  d = b->current.requested.x - (r->x + r->w);
		  if ((d < 0) && (d >= - r->v1))
		    {
		       if (resist_x > d) resist_x = -d;
		    }
	       }
	  }
	if (SPANS_COMMON(r->x, r->w, b->current.requested.x, b->current.w))
	  {
	     if (dy > 0)
	       {
		  /* moving down - check top edge of windows against bottom */
		  d = r->y - (b->current.requested.y + b->current.h);
		  if ((d < 0) && (d >=2 - r->v1))
		    {
		       if (resist_y > d) resist_y = d;
		    }
	       }
	     else if (dy < 0)
	       {
		  /* moving up - check bottom edge of windows against top */
		  d = b->current.requested.y - (r->y + r->h);
		  if ((d < 0) && (d >= - r->v1))
		    {
		       if (resist_y > d) resist_y = -d;
		    }
	       }
	  }
     }
   if (rects)
     {
	for (l = rects; l; l = l->next)
	  {
	     FREE(l->data);
	  }	     
	evas_list_free(rects);
     }
   if (dx != 0) 
     {
	if ((b->previous.requested.x != b->previous.x) &&
	    (((b->previous.requested.dx < 0) && (b->current.requested.dx > 0)) ||
	     ((b->previous.requested.dx > 0) && (b->current.requested.dx < 0))))
	  b->current.requested.x = b->current.x;
	else
	  b->current.x = b->current.requested.x + resist_x;
     }
   if (dy != 0) 
     {
	if ((b->previous.requested.y != b->previous.y) &&
	    (((b->previous.requested.dy < 0) && (b->current.requested.dy > 0)) ||
	     ((b->previous.requested.dy > 0) && (b->current.requested.dy < 0))))
	  b->current.requested.y = b->current.y;
	else
	  b->current.y = b->current.requested.y + resist_y;
     }
}
