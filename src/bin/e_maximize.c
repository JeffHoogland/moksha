/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

void
e_maximize_border_gadman_fit(E_Border *bd, int *x1, int *y1, int *x2, int *y2)
{
   Evas_List *l;
   int cx1, cx2, cy1, cy2;

   cx1 = bd->zone->x;
   if (x1) cx1 = *x1;
   
   cy1 = bd->zone->y;
   if (y1) cy1 = *y1;
   
   cx2 = bd->zone->x + bd->zone->w;
   if (x2) cx2 = *x2;

   cy2 = bd->zone->y + bd->zone->h;
   if (y2) cy2 = *y2;

   /* Find the smallest box */
   for (l = bd->zone->container->gadman->clients; l; l = l->next)
     {
	E_Gadman_Client *gmc;
	double ax, ay;

	gmc = l->data;
	if ((gmc->zone != bd->zone)) continue;

	ax = gmc->ax;
	ay = gmc->ay;

	if (((ax == 0.0) || (ax == 1.0)) &&
	    ((ay == 0.0) || (ay == 1.0)))
	  {
	     /* corner gadget */
	     /* Fake removal from one alignment :) */
	     if (gmc->w > gmc->h)
	       ax = 0.5;
	     else
	       ay = 0.5;
	  }

	if ((ax == 0.0) && (gmc->x + gmc->w) > cx1)
	  cx1 = (gmc->x + gmc->w);
	else if ((ax == 1.0) && (gmc->x < cx2))
	  cx2 = gmc->x;
	else if ((ay == 0.0) && ((gmc->y + gmc->h) > cy1))
	  cy1 = (gmc->y + gmc->h);
	else if ((ay == 1.0) && (gmc->y < cy2))
	  cy2 = gmc->y;
     }

   if (x1) *x1 = cx1;
   if (y1) *y1 = cy1;
   if (x2) *x2 = cx2;
   if (y2) *y2 = cy2;
}

void
e_maximize_border_gadman_fill(E_Border *bd, int *x1, int *y1, int *x2, int *y2)
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
   for (l = bd->zone->container->gadman->clients; l; l = l->next)
     {
	/* expand left */
	E_Gadman_Client *gmc;
	int gx2;

	gmc = l->data;
	if ((gmc->zone != bd->zone)) continue;

	gx2 = gmc->x + gmc->w;
	if ((gx2 > cx1) && (gx2 <= bx) &&
	    E_INTERSECTS(0, gmc->y, bd->zone->w, gmc->h, 0, by, bd->zone->w, bh))
	  {
	     cx1 = gx2;
	  }
     }
   bw += (bx - cx1);
   bx = cx1;
   for (l = bd->zone->container->gadman->clients; l; l = l->next)
     {
	/* expand up */
	E_Gadman_Client *gmc;
	int gy2;

	gmc = l->data;
	if ((gmc->zone != bd->zone)) continue;

	gy2 = gmc->y + gmc->h;
	if ((gy2 > cy1) && (gy2 <= by) &&
	    E_INTERSECTS(gmc->x, 0, gmc->w, bd->zone->h, bx, 0, bw, bd->zone->h))
	  {
	     cy1 = gy2;
	  }
     }
   bh += (by - cy1);
   by = cy1;
   for (l = bd->zone->container->gadman->clients; l; l = l->next)
     {
	/* expand right */
	E_Gadman_Client *gmc;

	gmc = l->data;
	if ((gmc->zone != bd->zone)) continue;

	if ((gmc->x < cx2) && (gmc->x >= (bx + bw)) &&
	    E_INTERSECTS(0, gmc->y, bd->zone->w, gmc->h, 0, by, bd->zone->w, bh))
	  {
	     cx2 = gmc->x;
	  }
     }
   bw = (cx2 - cx1);
   for (l = bd->zone->container->gadman->clients; l; l = l->next)
     {
	/* expand down */
	E_Gadman_Client *gmc;

	gmc = l->data;
	if ((gmc->zone != bd->zone)) continue;

	if ((gmc->y < cy2) && (gmc->y >= (by + bh)) &&
	    E_INTERSECTS(gmc->x, 0, gmc->w, bd->zone->h, bx, 0, bw, bd->zone->h))
	  {
	     cy2 = gmc->y;
	  }
     }
   bh = (cy2 - cy1);

   if (x1) *x1 = cx1;
   if (y1) *y1 = cy1;
   if (x2) *x2 = cx2;
   if (y2) *y2 = cy2;
}
