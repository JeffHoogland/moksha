/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

void
e_maximize_border_gadman(E_Border *bd, int *x1, int *y1, int *x2, int *y2)
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

	gmc = l->data;
	if ((gmc->zone != bd->zone)) continue;

	if ((gmc->ax == 0.0) && ((gmc->x + gmc->w) > cx1))
	  cx1 = (gmc->x + gmc->w);
	if ((gmc->ax == 1.0) && (gmc->x < cx2))
	  cx2 = gmc->x;
	if ((gmc->ay == 0.0) && ((gmc->y + gmc->h) > cy1))
	  cy1 = (gmc->y + gmc->h);
	if ((gmc->ay == 1.0) && (gmc->y < cy2))
	  cy2 = gmc->y;
     }
   /* FIXME: Try to expand */

   if (x1) *x1 = cx1;
   if (y1) *y1 = cy1;
   if (x2) *x2 = cx2;
   if (y2) *y2 = cy2;
}
