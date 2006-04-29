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

struct _m_zone
{
   int x1;
   int y1;
   int x2;
   int y2;
   int area;
};

#define OBSTACLE(_x1, _y1, _x2, _y2) \
   { \
      r = E_NEW(E_Maximize_Rect, 1); \
      r->x1 = (_x1); r->y1 = (_y1); r->x2 = (_x2); r->y2 = (_y2); \
      rects = evas_list_append(rects, r); \
   }

static void _e_maximize_border_rects_fill(E_Border *bd, Evas_List *list, int *x1, int *y1, int *x2, int *y2);
static int _e_mzone_gadman_split(Evas_List **add_splits_to, struct _m_zone *mzone, E_Gadman_Client *gmc);
static int _e_mzone_shelf_split(Evas_List **add_splits_to, struct _m_zone *mzone, E_Shelf *es);
static int _e_mzone_cb_square_reverse_sort(void *e1, void *e2);
static int _e_mzone_cb_width_reverse_sort(void *e1, void *e2);
static int _e_mzone_cb_height_reverse_sort(void *e1, void *e2); // not used yet

EAPI void
e_maximize_border_gadman_fit(E_Border *bd, int *x1, int *y1, int *x2, int *y2)
{
   Evas_List *l, *ll;
   Evas_List *mzones = NULL;
   Evas_List *tmp_mzones = NULL;
   struct _m_zone *mzone = NULL;

   mzone = (struct _m_zone *)malloc(sizeof(struct _m_zone));
   if (mzone == NULL)
     return;
   
   mzone->x1 = bd->zone->x;
   if (x1) mzone->x1 = *x1;
   
   mzone->y1 = bd->zone->y;
   if (y1) mzone->y1 = *y1;
   
   mzone->x2 = bd->zone->x + bd->zone->w;
   if (x2) mzone->x2 = *x2;
   
   mzone->y2 = bd->zone->y + bd->zone->h;
   if (y2) mzone->y2 = *y2;
   
   mzones = evas_list_append(mzones, mzone);
   
   for (l = bd->zone->container->gadman->clients; l; l = l->next)
     {
	E_Gadman_Client *gmc;
	
	gmc = l->data;
	if ((gmc->zone != bd->zone) || 
	    (gmc->policy & E_GADMAN_POLICY_ALLOW_OVERLAP)) continue;
	
	tmp_mzones = mzones;
	mzones = NULL;
	
	for (ll = tmp_mzones; ll; ll = ll->next)
	  {
	     int res;
	     mzone = ll->data;
	     
	     res = _e_mzone_gadman_split(&mzones, mzone, gmc);
	     if (res == 0)
	       {
		  mzones = evas_list_append(mzones, mzone);
		  ll->data = NULL;
	       }
	     else if (res == 2)
	       ;
	     else if (res == -1)
	       ; /* mem problems. Let ignor them yet */
	     
	     if (ll->data != NULL)
	       {
		  free(ll->data);
		  ll->data = NULL;
	       }
	  }
	evas_list_free(tmp_mzones);
	tmp_mzones = NULL;
     }

   for (l = e_shelf_list(); l; l = l->next)
     {
	E_Shelf *es;
	
	es = l->data;
	if (es->zone != bd->zone) continue;
	printf("ES\n");
	for (ll = tmp_mzones; ll; ll = ll->next)
	  {
	     int res;
	     mzone = ll->data;
	     
	     res = _e_mzone_shelf_split(&mzones, mzone, es);
	     if (res == 0)
	       {
		  mzones = evas_list_append(mzones, mzone);
		  ll->data = NULL;
	       }
	     else if (res == 2)
	       ;
	     else if (res == -1)
	       ; /* mem problems. Let ignor them yet */
	     
	     if (ll->data != NULL)
	       {
		  free(ll->data);
		  ll->data = NULL;
	       }
	  }
	evas_list_free(tmp_mzones);
	tmp_mzones = NULL;
     }
   
   for (l = mzones; l; l = l->next)
     {
	mzone = l->data;
	mzone->area = (mzone->x2 - mzone->x1) * (mzone->y2 - mzone->y1);
     }
   
   tmp_mzones = evas_list_sort(mzones, evas_list_count(mzones), _e_mzone_cb_square_reverse_sort);
   mzones = NULL;
   
   mzones = evas_list_append(mzones, tmp_mzones->data);
   
   for (l = tmp_mzones->next; l; l = l->next)
     {
	if ( ((struct _m_zone *)l->data)->area ==
	     ((struct _m_zone *)mzones->data)->area)
	  mzones = evas_list_append(mzones, l->data);
	else
	  free(l->data);
     }
   tmp_mzones = evas_list_free(tmp_mzones);
   tmp_mzones = NULL;
   
   if (mzones != NULL && mzones->next == NULL)
     {
	mzone = mzones->data;
	*x1 = mzone->x1;
	*y1 = mzone->y1;
	*x2 = mzone->x2;
	*y2 = mzone->y2;
     }
   else if (mzones != NULL && mzones->next != NULL)
     {
	Evas_List *wl = NULL;
	
	/* The use of *_width_reverse_sort or *_height_reverse_sort depends
	 * on the preferences of the user - what window he/she would like to
	 * have: (i) maximized verticaly or (ii) horisontaly.
	 */
	wl = evas_list_sort(mzones, evas_list_count(mzones), _e_mzone_cb_width_reverse_sort);
	mzones = NULL;
	
	mzone = wl->data;
	/* mzone = hl->data; */
	*x1 = mzone->x1;
	*y1 = mzone->y1;
	*x2 = mzone->x2;
	*y2 = mzone->y2;
	
	/* evas_list_free( wl );
	 evas_list_free( hl );
	 */ 
	mzones = wl;
     }
   
   for (l = mzones; l ; l = l->next)
     if (l->data != NULL)
       free(l->data);
   mzones = evas_list_free(mzones);
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
	     TOP,
	     RIGHT,
	     BOTTOM,
	     LEFT
	} edge = TOP;

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
		  else if ((bd2->x + bd2->h) == (bd2->zone->x + bd2->zone->h))
		    edge = BOTTOM;
	       }
	     else
	       {
		  if ((bd2->x + bd2->w) == (bd2->zone->x + bd2->zone->w))
		    edge = RIGHT;
		  else if ((bd2->y == bd2->zone->y))
		    edge = LEFT;
	       }
	  }
	else
	  {
	     if (bd2->y == bd2->zone->y)
	       edge = TOP;
	     else if ((bd2->x + bd2->w) == (bd2->zone->x + bd2->zone->w))
	       edge = RIGHT;
	     else if ((bd2->x + bd2->h) == (bd2->zone->x + bd2->zone->h))
	       edge = BOTTOM;
	     else if ((bd2->y == bd2->zone->y))
	       edge = LEFT;
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

   for (l = bd->zone->container->gadman->clients; l; l = l->next)
     {
	E_Gadman_Client *gmc;

	gmc = l->data;
	if ((gmc->zone != bd->zone)) continue;
	OBSTACLE(gmc->x, gmc->y, gmc->x + gmc->w, gmc->y + gmc->h);
     }
   for (l = e_shelf_list(); l; l = l->next)
     {
	E_Shelf *es;

	es = l->data;
	if (es->zone != bd->zone) continue;
	printf("OBS: %i %i %ix%i\n", es->x, es->y, es->w, es->h);
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

static int
_e_mzone_gadman_split(Evas_List **add_splits_to, struct _m_zone *mzone, E_Gadman_Client *gmc)
{
   int ii;
   int mzone_splitted = 0;
   struct _m_zone *mzone_split = NULL;
   
   if (mzone == NULL || gmc == NULL)
     return -1;
   
   if ((mzone->x2 - mzone->x1) <= 0 || (mzone->y2 - mzone->y1) <= 0)
     return 1;
   
   if (gmc->x > mzone->x1 && gmc->y > mzone->y1 && 
       gmc->x + gmc->w < mzone->x2 && gmc->y + gmc->h < mzone->y2)
     {
	mzone_splitted = 1;
	for (ii = 0; ii < 8; ii ++)
	  {
	     mzone_split = (struct _m_zone *)malloc(sizeof(struct _m_zone));
	     if (mzone_split == NULL)
	       return -1;
	     
	     switch(ii)
	       {
		case 0:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = gmc->y;
		  break;
		case 1:
		  mzone_split->x1 = gmc->x + gmc->w;
		  mzone_split->y1 = gmc->y;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = gmc->y + gmc->h;
		  break;
		case 2:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = gmc->y + gmc->h;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = mzone->y2;
		  break;
		case 3:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = gmc->y;
		  mzone_split->x2 = gmc->x;
		  mzone_split->y2 = gmc->y + gmc->h;
		  break;
		case 4:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = gmc->x;
		  mzone_split->y2 = mzone->y2;
		  break;
		case 5:
		  mzone_split->x1 = gmc->x;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = gmc->x + gmc->w;
		  mzone_split->y2 = gmc->y;
		  break;
		case 6:
		  mzone_split->x1 = gmc->x + gmc->w;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = mzone->y2;
		  break;
		case 7:
		  mzone_split->x1 = gmc->x;
		  mzone_split->y1 = gmc->y + gmc->h;
		  mzone_split->x2 = gmc->x + gmc->w;
		  mzone_split->y2 = mzone->y2;
		  break;
	       }
	     *add_splits_to = evas_list_append(*add_splits_to, mzone_split);
	  }
     } // if
   else if (gmc->x + gmc->w > mzone->x1 && gmc->x + gmc->w < mzone->x2 &&
	    gmc->y + gmc->h > mzone->y1 && gmc->y + gmc->h < mzone->y2 &&
	    gmc->x <= mzone->x1 && gmc->y <= mzone->y1)
     {
	mzone_splitted = 1;
	for (ii = 0; ii < 4; ii ++)
	  {
	     mzone_split = (struct _m_zone *)malloc(sizeof(struct _m_zone));
	     if (mzone_split == NULL)
	       return -1;
	     
	     switch(ii)
	       {
		case 0:
		  mzone_split->x1 = gmc->x + gmc->w;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = gmc->y + gmc->h;
		  break;
		case 1:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = gmc->y + gmc->h;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = mzone->y2;
		  break;
		case 2:
		  mzone_split->x1 = gmc->x + gmc->w;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = mzone->y2;
		  break;
		case 3:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = gmc->y + gmc->h;
		  mzone_split->x2 = gmc->x + gmc->w;
		  mzone_split->y2 = mzone->y2;
		  break;
	       }
	     *add_splits_to = evas_list_append(*add_splits_to, mzone_split);
	  }
     }
   else if (gmc->x > mzone->x1 && gmc->y <= mzone->y1 &&
	    gmc->x + gmc->w < mzone->x2 &&
	    gmc->y + gmc->h > mzone->y1 && gmc->y + gmc->h < mzone->y2)
     {
	mzone_splitted = 1;
	for (ii = 0; ii < 6; ii ++)
	  {
	     mzone_split = (struct _m_zone *)malloc(sizeof(struct _m_zone));
	     if (mzone_split == NULL)
	       return -1;
	     
	     switch(ii)
	       {
		case 0:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = gmc->x;
		  mzone_split->y2 = gmc->y + gmc->h;
		  break;
		case 1:
		  mzone_split->x1 = gmc->x + gmc->w;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = gmc->y + gmc->h;
		  break;
		case 2:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = gmc->y + gmc->h;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = mzone->y2;
		  break;
		case 3:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = gmc->x;
		  mzone_split->y2 = mzone->y2;
		  break;
		case 4:
		  mzone_split->x1 = gmc->x;
		  mzone_split->y1 = gmc->y + gmc->h;
		  mzone_split->x2 = gmc->x + gmc->w;
		  mzone_split->y2 = mzone->y2;
		  break;
		case 5:
		  mzone_split->x1 = gmc->x + gmc->w;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = mzone->y2;
		  break;
	       }
	     *add_splits_to = evas_list_append(*add_splits_to, mzone_split);
	  }
     }
   else if (gmc->x > mzone->x1 && gmc->x < mzone->x2 &&
	    gmc->y + gmc->h > mzone->y1 && gmc->y + gmc->h < mzone->y2 &&
	    gmc->y <= mzone->y1 &&
	    gmc->x + gmc->w >= mzone->x2)
     {
	mzone_splitted = 1;
	for (ii = 0; ii < 4; ii ++)
	  {
	     mzone_split = (struct _m_zone *)malloc(sizeof(struct _m_zone));
	     if (mzone_split == NULL)
	       return -1;
	     
	     switch(ii)
	       {
		case 0:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = gmc->x;
		  mzone_split->y2 = gmc->y + gmc->h;
		  break;
		case 1:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = gmc->y + gmc->h;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = mzone->y2;
		  break;
		case 2:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = gmc->x;
		  mzone_split->y2 = mzone->y2;
		  break;
		case 3:
		  mzone_split->x1 = gmc->x;
		  mzone_split->y1 = gmc->y + gmc->h;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = mzone->y2;
		  break;
	       }
	     *add_splits_to = evas_list_append(*add_splits_to, mzone_split);
	  }
     }
   else if (gmc->x > mzone->x1 && gmc->x < mzone->x2 &&
	    gmc->y > mzone->y1 &&
	    gmc->y + gmc->h < mzone->y2 &&
	    gmc->x + gmc->w >= mzone->x2)
     {
	mzone_splitted = 1;
	for (ii = 0; ii < 6; ii ++)
	  {
	     mzone_split = (struct _m_zone *)malloc(sizeof(struct _m_zone));
	     if (mzone_split == NULL)
	       return -1;
	     
	     switch(ii)
	       {
		case 0:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = gmc->y;
		  break;
		case 1:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = gmc->y;
		  mzone_split->x2 = gmc->x;
		  mzone_split->y2 = gmc->y + gmc->h;
		  break;
		case 2:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = gmc->y + gmc->h;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = mzone->y2;
		  break;
		case 3:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = gmc->x;
		  mzone_split->y2 = mzone->y2;
		  break;
		case 4:
		  mzone_split->x1 = gmc->x;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = gmc->y;
		  break;
		case 5:
		  mzone_split->x1 = gmc->x;
		  mzone_split->y1 = gmc->y + gmc->h;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = mzone->y2;
		  break;
	       }
	     *add_splits_to = evas_list_append(*add_splits_to, mzone_split);
	  }
     }
   else if (gmc->x > mzone->x1 && gmc->x < mzone->x2 &&
	    gmc->y > mzone->y1 && gmc->y < mzone->y2 &&
	    gmc->x + gmc->w >= mzone->x2 &&
	    gmc->y + gmc->h >= mzone->y2)
     {
	mzone_splitted = 1;
	for (ii = 0; ii < 4; ii ++)
	  {
	     mzone_split = (struct _m_zone *)malloc(sizeof(struct _m_zone));
	     if (mzone_split == NULL)
	       return -1;
	     
	     switch(ii)
	       {
		case 0:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = gmc->y;
		  break;
		case 1:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = gmc->y;
		  mzone_split->x2 = gmc->x;
		  mzone_split->y2 = mzone->y2;
		  break;
		case 2:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = gmc->x;
		  mzone_split->y2 = mzone->y2;
		  break;
		case 3:
		  mzone_split->x1 = gmc->x;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = gmc->y;
		  break;
	       }
	     *add_splits_to = evas_list_append(*add_splits_to, mzone_split);
	  }
     }
   else if (gmc->x > mzone->x1 &&
	    gmc->y > mzone->y1 && gmc->y < mzone->y2 &&
	    gmc->x + gmc->w < mzone->x2 &&
	    gmc->y + gmc->h >= mzone->y2)
     {
	mzone_splitted = 1;
	for (ii = 0; ii < 6; ii ++)
	  {
	     mzone_split = (struct _m_zone *)malloc(sizeof(struct _m_zone));
	     if (mzone_split == NULL)
	       return -1;
	     
	     switch(ii)
	       {
		case 0:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = gmc->y;
		  break;
		case 1:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = gmc->y;
		  mzone_split->x2 = gmc->x;
		  mzone_split->y2 = mzone->y2;
		  break;
		case 2:
		  mzone_split->x1 = gmc->x + gmc->w;
		  mzone_split->y1 = gmc->y;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = mzone->y2;
		  break;
		case 3:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = gmc->x;
		  mzone_split->y2 = mzone->y2;
		  break;
		case 4:
		  mzone_split->x1 = gmc->x;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = gmc->x + gmc->w;
		  mzone_split->y2 = gmc->y;
		  break;
		case 5:
		  mzone_split->x1 = gmc->x + gmc->w;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = mzone->y2;
		  break;
	       }
	     *add_splits_to = evas_list_append(*add_splits_to, mzone_split);
	  }
     }
   else if (gmc->x <= mzone->x1 &&
	    gmc->y > mzone->y1 && gmc->y < mzone->y2 &&
	    gmc->x + gmc->w > mzone->x1 && gmc->x + gmc->w < mzone->x2 &&
	    gmc->y + gmc->h >= mzone->y2)
     {
	mzone_splitted = 1;
	for (ii = 0; ii < 4; ii ++)
	  {
	     mzone_split = (struct _m_zone *)malloc(sizeof(struct _m_zone));
	     if (mzone_split == NULL)
	       return -1;
	     
	     switch(ii)
	       {
		case 0:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = gmc->y;
		  break;
		case 1:
		  mzone_split->x1 = gmc->x + gmc->w;
		  mzone_split->y1 = gmc->y;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = mzone->y2;
		  break;
		case 2:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = gmc->x + gmc->w;
		  mzone_split->y2 = gmc->y;
		  break;
		case 3:
		  mzone_split->x1 = gmc->x + gmc->w;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = mzone->y2;
		  break;
	       }
	     *add_splits_to = evas_list_append(*add_splits_to, mzone_split);
	  }
     }
   else if (gmc->x <= mzone->x1 &&
	    gmc->y > mzone->y1 &&
	    gmc->y + gmc->h < mzone->y2 &&
	    gmc->x + gmc->w > mzone->x1 && gmc->x + gmc->w < mzone->x2)
     {
	mzone_splitted = 1;
	for (ii = 0; ii < 6; ii ++)
	  {
	     mzone_split = (struct _m_zone *)malloc(sizeof(struct _m_zone));
	     if (mzone_split == NULL)
	       return -1;
	     
	     switch(ii)
	       {
		case 0:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = gmc->y;
		  break;
		case 1:
		  mzone_split->x1 = gmc->x + gmc->w;
		  mzone_split->y1 = gmc->y;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = gmc->y + gmc->h;
		  break;
		case 2:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = gmc->y + gmc->h;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = mzone->y2;
		  break;
		case 3:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = gmc->x + gmc->w;
		  mzone_split->y2 = gmc->y;
		  break;
		case 4:
		  mzone_split->x1 = gmc->x + gmc->w;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = mzone->y2;
		  break;
		case 5:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = gmc->y + gmc->h;
		  mzone_split->x2 = gmc->x + gmc->w;
		  mzone_split->y2 = mzone->y2;
		  break;
	       }
	     *add_splits_to = evas_list_append(*add_splits_to, mzone_split);
	  }
     }
   else if (gmc->x <= mzone->x1 && gmc->y <= mzone->y1 &&
	    gmc->x + gmc->w >= mzone->x2 &&
	    gmc->y + gmc->h > mzone->y1 &&
	    gmc->y + gmc->h < mzone->y2)
     {
	mzone_splitted = 1;
	mzone_split = (struct _m_zone *)malloc(sizeof(struct _m_zone));
	if (mzone_split == NULL)
	  return -1;
	
	mzone_split->x1 = mzone->x1;
	mzone_split->y1 = gmc->y + gmc->h;
	mzone_split->x2 = mzone->x2;
	mzone_split->y2 = mzone->y2;
	
	*add_splits_to = evas_list_append(*add_splits_to, mzone_split);
     }
   else if (gmc->x <= mzone->x1 &&
	    gmc->x + gmc->w >= mzone->x2 &&
	    gmc->y > mzone->y1 && gmc->y < mzone->y2)
     {
	mzone_splitted = 1;
	for (ii = 0; ii < 2; ii ++)
	  {
	     mzone_split = (struct _m_zone *)malloc(sizeof(struct _m_zone));
	     if (mzone_split == NULL)
	       return -1;
	     
	     switch(ii)
	       {
		case 0:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = gmc->y;
		  break;
		case 1:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = gmc->y + gmc->h;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = mzone->y2;
		  break;
	       }
	     if (*add_splits_to == NULL)
	       *add_splits_to = evas_list_append(*add_splits_to, mzone_split);
	     else
	       evas_list_append(*add_splits_to, mzone_split);
	  }
     }
   else if (gmc->x <= mzone->x1 &&
	    gmc->x + gmc->w >= mzone->x2 &&
	    gmc->y > mzone->y1 && gmc->y < mzone->y2 &&
	    gmc->y + gmc->h >= mzone->y2)
     {
	mzone_splitted = 1;
	mzone_split = (struct _m_zone *)malloc(sizeof(struct _m_zone));
	if (mzone_split == NULL)
	  return -1;
	
	mzone_split->x1 = mzone->x1;
	mzone_split->y1 = mzone->y1;
	mzone_split->x2 = mzone->x2;
	mzone_split->y2 = gmc->y;
	
	*add_splits_to = evas_list_append(*add_splits_to, mzone_split);
     }
   else if (gmc->x <= mzone->x1 &&
	    gmc->y <= mzone->y1 &&
	    gmc->y + gmc->h >= mzone->y2 &&
	    gmc->x + gmc->w > mzone->x1 && gmc->x + gmc->w < mzone->x2)
     {
	mzone_splitted = 1;
	mzone_split = (struct _m_zone *)malloc(sizeof(struct _m_zone));
	if (mzone_split == NULL)
	  return -1;
	
	mzone_split->x1 = gmc->x + gmc->w;
	mzone_split->y1 = mzone->y1;
	mzone_split->x2 = mzone->x2;
	mzone_split->y2 = mzone->y2;
	
	*add_splits_to = evas_list_append(*add_splits_to, mzone_split);
     }
   else if (gmc->x > mzone->x1 &&
	    gmc->x + gmc->w < mzone->x2 &&
	    gmc->y <= mzone->y1 &&
	    gmc->y + gmc->h >= mzone->y2)
     {
	mzone_splitted = 1;
	for (ii = 0; ii < 2; ii ++)
	  {
	     mzone_split = (struct _m_zone *)malloc(sizeof(struct _m_zone));
	     if (mzone_split == NULL)
	       return -1;
	     
	     switch(ii)
	       {
		case 0:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = gmc->x;
		  mzone_split->y2 = mzone->y2;
		  break;
		case 1:
		  mzone_split->x1 = gmc->x + gmc->w;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = mzone->y2;
		  break;
	       }
	     *add_splits_to = evas_list_append(*add_splits_to, mzone_split);
	  }
     }
   else if (gmc->x > mzone->x1 && gmc->x < mzone->x2 &&
	    gmc->y <= mzone->y1 &&
	    gmc->x + gmc->w >= mzone->x2 &&
	    gmc->y + gmc->h >= mzone->y2)
     {
	mzone_splitted = 1;
	mzone_split = (struct _m_zone *)malloc(sizeof(struct _m_zone));
	if (mzone_split == NULL)
	  return -1;
	
	mzone_split->x1 = mzone->x1;
	mzone_split->y1 = mzone->y1;
	mzone_split->x2 = gmc->x;
	mzone_split->y2 = mzone->y2;
	
	*add_splits_to = evas_list_append(*add_splits_to, mzone_split);
     }
   else if (gmc->x <= mzone->x1 && gmc->y <= mzone->y1 &&
	    gmc->x + gmc->w >= mzone->x2 && gmc->y + gmc->h >= mzone->y2)
     {
	mzone_splitted = 2;
     }
   
   return mzone_splitted;
}

static int
_e_mzone_shelf_split(Evas_List **add_splits_to, struct _m_zone *mzone, E_Shelf *es)
{
   int ii;
   int mzone_splitted = 0;
   struct _m_zone *mzone_split = NULL;
   
   if (mzone == NULL || es == NULL)
     return -1;
   
   if ((mzone->x2 - mzone->x1) <= 0 || (mzone->y2 - mzone->y1) <= 0)
     return 1;
   
   if (es->x > mzone->x1 && es->y > mzone->y1 && 
       es->x + es->w < mzone->x2 && es->y + es->h < mzone->y2)
     {
	mzone_splitted = 1;
	for (ii = 0; ii < 8; ii ++)
	  {
	     mzone_split = (struct _m_zone *)malloc(sizeof(struct _m_zone));
	     if (mzone_split == NULL)
	       return -1;
	     
	     switch(ii)
	       {
		case 0:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = es->y;
		  break;
		case 1:
		  mzone_split->x1 = es->x + es->w;
		  mzone_split->y1 = es->y;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = es->y + es->h;
		  break;
		case 2:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = es->y + es->h;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = mzone->y2;
		  break;
		case 3:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = es->y;
		  mzone_split->x2 = es->x;
		  mzone_split->y2 = es->y + es->h;
		  break;
		case 4:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = es->x;
		  mzone_split->y2 = mzone->y2;
		  break;
		case 5:
		  mzone_split->x1 = es->x;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = es->x + es->w;
		  mzone_split->y2 = es->y;
		  break;
		case 6:
		  mzone_split->x1 = es->x + es->w;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = mzone->y2;
		  break;
		case 7:
		  mzone_split->x1 = es->x;
		  mzone_split->y1 = es->y + es->h;
		  mzone_split->x2 = es->x + es->w;
		  mzone_split->y2 = mzone->y2;
		  break;
	       }
	     *add_splits_to = evas_list_append(*add_splits_to, mzone_split);
	  }
     } // if
   else if (es->x + es->w > mzone->x1 && es->x + es->w < mzone->x2 &&
	    es->y + es->h > mzone->y1 && es->y + es->h < mzone->y2 &&
	    es->x <= mzone->x1 && es->y <= mzone->y1)
     {
	mzone_splitted = 1;
	for (ii = 0; ii < 4; ii ++)
	  {
	     mzone_split = (struct _m_zone *)malloc(sizeof(struct _m_zone));
	     if (mzone_split == NULL)
	       return -1;
	     
	     switch(ii)
	       {
		case 0:
		  mzone_split->x1 = es->x + es->w;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = es->y + es->h;
		  break;
		case 1:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = es->y + es->h;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = mzone->y2;
		  break;
		case 2:
		  mzone_split->x1 = es->x + es->w;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = mzone->y2;
		  break;
		case 3:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = es->y + es->h;
		  mzone_split->x2 = es->x + es->w;
		  mzone_split->y2 = mzone->y2;
		  break;
	       }
	     *add_splits_to = evas_list_append(*add_splits_to, mzone_split);
	  }
     }
   else if (es->x > mzone->x1 && es->y <= mzone->y1 &&
	    es->x + es->w < mzone->x2 &&
	    es->y + es->h > mzone->y1 && es->y + es->h < mzone->y2)
     {
	mzone_splitted = 1;
	for (ii = 0; ii < 6; ii ++)
	  {
	     mzone_split = (struct _m_zone *)malloc(sizeof(struct _m_zone));
	     if (mzone_split == NULL)
	       return -1;
	     
	     switch(ii)
	       {
		case 0:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = es->x;
		  mzone_split->y2 = es->y + es->h;
		  break;
		case 1:
		  mzone_split->x1 = es->x + es->w;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = es->y + es->h;
		  break;
		case 2:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = es->y + es->h;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = mzone->y2;
		  break;
		case 3:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = es->x;
		  mzone_split->y2 = mzone->y2;
		  break;
		case 4:
		  mzone_split->x1 = es->x;
		  mzone_split->y1 = es->y + es->h;
		  mzone_split->x2 = es->x + es->w;
		  mzone_split->y2 = mzone->y2;
		  break;
		case 5:
		  mzone_split->x1 = es->x + es->w;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = mzone->y2;
		  break;
	       }
	     *add_splits_to = evas_list_append(*add_splits_to, mzone_split);
	  }
     }
   else if (es->x > mzone->x1 && es->x < mzone->x2 &&
	    es->y + es->h > mzone->y1 && es->y + es->h < mzone->y2 &&
	    es->y <= mzone->y1 &&
	    es->x + es->w >= mzone->x2)
     {
	mzone_splitted = 1;
	for (ii = 0; ii < 4; ii ++)
	  {
	     mzone_split = (struct _m_zone *)malloc(sizeof(struct _m_zone));
	     if (mzone_split == NULL)
	       return -1;
	     
	     switch(ii)
	       {
		case 0:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = es->x;
		  mzone_split->y2 = es->y + es->h;
		  break;
		case 1:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = es->y + es->h;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = mzone->y2;
		  break;
		case 2:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = es->x;
		  mzone_split->y2 = mzone->y2;
		  break;
		case 3:
		  mzone_split->x1 = es->x;
		  mzone_split->y1 = es->y + es->h;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = mzone->y2;
		  break;
	       }
	     *add_splits_to = evas_list_append(*add_splits_to, mzone_split);
	  }
     }
   else if (es->x > mzone->x1 && es->x < mzone->x2 &&
	    es->y > mzone->y1 &&
	    es->y + es->h < mzone->y2 &&
	    es->x + es->w >= mzone->x2)
     {
	mzone_splitted = 1;
	for (ii = 0; ii < 6; ii ++)
	  {
	     mzone_split = (struct _m_zone *)malloc(sizeof(struct _m_zone));
	     if (mzone_split == NULL)
	       return -1;
	     
	     switch(ii)
	       {
		case 0:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = es->y;
		  break;
		case 1:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = es->y;
		  mzone_split->x2 = es->x;
		  mzone_split->y2 = es->y + es->h;
		  break;
		case 2:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = es->y + es->h;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = mzone->y2;
		  break;
		case 3:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = es->x;
		  mzone_split->y2 = mzone->y2;
		  break;
		case 4:
		  mzone_split->x1 = es->x;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = es->y;
		  break;
		case 5:
		  mzone_split->x1 = es->x;
		  mzone_split->y1 = es->y + es->h;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = mzone->y2;
		  break;
	       }
	     *add_splits_to = evas_list_append(*add_splits_to, mzone_split);
	  }
     }
   else if (es->x > mzone->x1 && es->x < mzone->x2 &&
	    es->y > mzone->y1 && es->y < mzone->y2 &&
	    es->x + es->w >= mzone->x2 &&
	    es->y + es->h >= mzone->y2)
     {
	mzone_splitted = 1;
	for (ii = 0; ii < 4; ii ++)
	  {
	     mzone_split = (struct _m_zone *)malloc(sizeof(struct _m_zone));
	     if (mzone_split == NULL)
	       return -1;
	     
	     switch(ii)
	       {
		case 0:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = es->y;
		  break;
		case 1:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = es->y;
		  mzone_split->x2 = es->x;
		  mzone_split->y2 = mzone->y2;
		  break;
		case 2:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = es->x;
		  mzone_split->y2 = mzone->y2;
		  break;
		case 3:
		  mzone_split->x1 = es->x;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = es->y;
		  break;
	       }
	     *add_splits_to = evas_list_append(*add_splits_to, mzone_split);
	  }
     }
   else if (es->x > mzone->x1 &&
	    es->y > mzone->y1 && es->y < mzone->y2 &&
	    es->x + es->w < mzone->x2 &&
	    es->y + es->h >= mzone->y2)
     {
	mzone_splitted = 1;
	for (ii = 0; ii < 6; ii ++)
	  {
	     mzone_split = (struct _m_zone *)malloc(sizeof(struct _m_zone));
	     if (mzone_split == NULL)
	       return -1;
	     
	     switch(ii)
	       {
		case 0:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = es->y;
		  break;
		case 1:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = es->y;
		  mzone_split->x2 = es->x;
		  mzone_split->y2 = mzone->y2;
		  break;
		case 2:
		  mzone_split->x1 = es->x + es->w;
		  mzone_split->y1 = es->y;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = mzone->y2;
		  break;
		case 3:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = es->x;
		  mzone_split->y2 = mzone->y2;
		  break;
		case 4:
		  mzone_split->x1 = es->x;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = es->x + es->w;
		  mzone_split->y2 = es->y;
		  break;
		case 5:
		  mzone_split->x1 = es->x + es->w;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = mzone->y2;
		  break;
	       }
	     *add_splits_to = evas_list_append(*add_splits_to, mzone_split);
	  }
     }
   else if (es->x <= mzone->x1 &&
	    es->y > mzone->y1 && es->y < mzone->y2 &&
	    es->x + es->w > mzone->x1 && es->x + es->w < mzone->x2 &&
	    es->y + es->h >= mzone->y2)
     {
	mzone_splitted = 1;
	for (ii = 0; ii < 4; ii ++)
	  {
	     mzone_split = (struct _m_zone *)malloc(sizeof(struct _m_zone));
	     if (mzone_split == NULL)
	       return -1;
	     
	     switch(ii)
	       {
		case 0:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = es->y;
		  break;
		case 1:
		  mzone_split->x1 = es->x + es->w;
		  mzone_split->y1 = es->y;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = mzone->y2;
		  break;
		case 2:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = es->x + es->w;
		  mzone_split->y2 = es->y;
		  break;
		case 3:
		  mzone_split->x1 = es->x + es->w;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = mzone->y2;
		  break;
	       }
	     *add_splits_to = evas_list_append(*add_splits_to, mzone_split);
	  }
     }
   else if (es->x <= mzone->x1 &&
	    es->y > mzone->y1 &&
	    es->y + es->h < mzone->y2 &&
	    es->x + es->w > mzone->x1 && es->x + es->w < mzone->x2)
     {
	mzone_splitted = 1;
	for (ii = 0; ii < 6; ii ++)
	  {
	     mzone_split = (struct _m_zone *)malloc(sizeof(struct _m_zone));
	     if (mzone_split == NULL)
	       return -1;
	     
	     switch(ii)
	       {
		case 0:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = es->y;
		  break;
		case 1:
		  mzone_split->x1 = es->x + es->w;
		  mzone_split->y1 = es->y;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = es->y + es->h;
		  break;
		case 2:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = es->y + es->h;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = mzone->y2;
		  break;
		case 3:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = es->x + es->w;
		  mzone_split->y2 = es->y;
		  break;
		case 4:
		  mzone_split->x1 = es->x + es->w;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = mzone->y2;
		  break;
		case 5:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = es->y + es->h;
		  mzone_split->x2 = es->x + es->w;
		  mzone_split->y2 = mzone->y2;
		  break;
	       }
	     *add_splits_to = evas_list_append(*add_splits_to, mzone_split);
	  }
     }
   else if (es->x <= mzone->x1 && es->y <= mzone->y1 &&
	    es->x + es->w >= mzone->x2 &&
	    es->y + es->h > mzone->y1 &&
	    es->y + es->h < mzone->y2)
     {
	mzone_splitted = 1;
	mzone_split = (struct _m_zone *)malloc(sizeof(struct _m_zone));
	if (mzone_split == NULL)
	  return -1;
	
	mzone_split->x1 = mzone->x1;
	mzone_split->y1 = es->y + es->h;
	mzone_split->x2 = mzone->x2;
	mzone_split->y2 = mzone->y2;
	
	*add_splits_to = evas_list_append(*add_splits_to, mzone_split);
     }
   else if (es->x <= mzone->x1 &&
	    es->x + es->w >= mzone->x2 &&
	    es->y > mzone->y1 && es->y < mzone->y2)
     {
	mzone_splitted = 1;
	for (ii = 0; ii < 2; ii ++)
	  {
	     mzone_split = (struct _m_zone *)malloc(sizeof(struct _m_zone));
	     if (mzone_split == NULL)
	       return -1;
	     
	     switch(ii)
	       {
		case 0:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = es->y;
		  break;
		case 1:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = es->y + es->h;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = mzone->y2;
		  break;
	       }
	     if (*add_splits_to == NULL)
	       *add_splits_to = evas_list_append(*add_splits_to, mzone_split);
	     else
	       evas_list_append(*add_splits_to, mzone_split);
	  }
     }
   else if (es->x <= mzone->x1 &&
	    es->x + es->w >= mzone->x2 &&
	    es->y > mzone->y1 && es->y < mzone->y2 &&
	    es->y + es->h >= mzone->y2)
     {
	mzone_splitted = 1;
	mzone_split = (struct _m_zone *)malloc(sizeof(struct _m_zone));
	if (mzone_split == NULL)
	  return -1;
	
	mzone_split->x1 = mzone->x1;
	mzone_split->y1 = mzone->y1;
	mzone_split->x2 = mzone->x2;
	mzone_split->y2 = es->y;
	
	*add_splits_to = evas_list_append(*add_splits_to, mzone_split);
     }
   else if (es->x <= mzone->x1 &&
	    es->y <= mzone->y1 &&
	    es->y + es->h >= mzone->y2 &&
	    es->x + es->w > mzone->x1 && es->x + es->w < mzone->x2)
     {
	mzone_splitted = 1;
	mzone_split = (struct _m_zone *)malloc(sizeof(struct _m_zone));
	if (mzone_split == NULL)
	  return -1;
	
	mzone_split->x1 = es->x + es->w;
	mzone_split->y1 = mzone->y1;
	mzone_split->x2 = mzone->x2;
	mzone_split->y2 = mzone->y2;
	
	*add_splits_to = evas_list_append(*add_splits_to, mzone_split);
     }
   else if (es->x > mzone->x1 &&
	    es->x + es->w < mzone->x2 &&
	    es->y <= mzone->y1 &&
	    es->y + es->h >= mzone->y2)
     {
	mzone_splitted = 1;
	for (ii = 0; ii < 2; ii ++)
	  {
	     mzone_split = (struct _m_zone *)malloc(sizeof(struct _m_zone));
	     if (mzone_split == NULL)
	       return -1;
	     
	     switch(ii)
	       {
		case 0:
		  mzone_split->x1 = mzone->x1;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = es->x;
		  mzone_split->y2 = mzone->y2;
		  break;
		case 1:
		  mzone_split->x1 = es->x + es->w;
		  mzone_split->y1 = mzone->y1;
		  mzone_split->x2 = mzone->x2;
		  mzone_split->y2 = mzone->y2;
		  break;
	       }
	     *add_splits_to = evas_list_append(*add_splits_to, mzone_split);
	  }
     }
   else if (es->x > mzone->x1 && es->x < mzone->x2 &&
	    es->y <= mzone->y1 &&
	    es->x + es->w >= mzone->x2 &&
	    es->y + es->h >= mzone->y2)
     {
	mzone_splitted = 1;
	mzone_split = (struct _m_zone *)malloc(sizeof(struct _m_zone));
	if (mzone_split == NULL)
	  return -1;
	
	mzone_split->x1 = mzone->x1;
	mzone_split->y1 = mzone->y1;
	mzone_split->x2 = es->x;
	mzone_split->y2 = mzone->y2;
	
	*add_splits_to = evas_list_append(*add_splits_to, mzone_split);
     }
   else if (es->x <= mzone->x1 && es->y <= mzone->y1 &&
	    es->x + es->w >= mzone->x2 && es->y + es->h >= mzone->y2)
     {
	mzone_splitted = 2;
     }
   
   return mzone_splitted;
}

static int
_e_mzone_cb_square_reverse_sort(void *e1, void *e2)
{
  struct _m_zone *mz1 = (struct _m_zone *)e1;
  struct _m_zone *mz2 = (struct _m_zone *)e2;

  if (e1 == NULL) return 1;
  if (e2 == NULL) return -1;

  if (mz1->area > mz2->area)
    return -1;
  else if (mz1->area <  mz2->area)
    return 1;

  return 0;
}

static int
_e_mzone_cb_width_reverse_sort(void *e1, void *e2)
{
  int w1, w2;
  struct _m_zone *mz1 = (struct _m_zone *)e1;
  struct _m_zone *mz2 = (struct _m_zone *)e2;

  if (e1 == NULL) return 1;
  if (e2 == NULL) return -1;
  
  w1 = mz1->x2 - mz1->x1;
  w2 = mz2->x2 - mz2->x1;

  if (w1 > w2)
    return -1;
  else if (w1 < w2)
    return 1;

  return 0;
}

static int
_e_mzone_cb_height_reverse_sort(void *e1, void *e2)
{
  int h1, h2;
  struct _m_zone *mz1 = (struct _m_zone *)e1;
  struct _m_zone *mz2 = (struct _m_zone *)e2;
  
  if (e1 == NULL) return 1;
  if (e2 == NULL) return -1;

  h1 = mz1->y2 - mz1->y1;
  h2 = mz2->y2 - mz2->y1;

  if (h1 > h2)
    return -1;
  else if (h1 < h2)
    return 1;

  return 0;
}

