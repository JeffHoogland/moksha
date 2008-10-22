/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

EAPI void
e_place_zone_region_smart_cleanup(E_Zone *zone)
{
   E_Desk *desk;
   Eina_List *borders = NULL;
   E_Border_List *bl;
   E_Border *border;

   E_OBJECT_CHECK(zone);
   desk = e_desk_current_get(zone);
   bl = e_container_border_list_first(desk->zone->container);
   while ((border = e_container_border_list_next(bl)))
     {
	/* Build a list of windows on this desktop and not iconified. */
	if ((border->desk == desk) && (!border->iconic) &&
	    (!border->lock_user_location))
	  {
	     int area;
	     Eina_List *ll;

	     /* Ordering windows largest to smallest gives better results */
	     area = border->w * border->h;
	     for (ll = borders; ll; ll = ll->next)
	       {
		  int testarea;
		  E_Border *bd;

		  bd = ll->data;
		  testarea = bd->w * bd->h;
		  /* Insert the border if larger than the current border */
		  if (area >= testarea)
		    {
		       borders = eina_list_prepend_relative(borders, border, bd);
		       break;
		    }
	       }
	     /* Looped over all borders without placing, so place at end */
	     if (!ll) borders = eina_list_append(borders, border);
	  }
     }
   e_container_border_list_free(bl);

   /* Loop over the borders moving each one using the smart placement */
   while (borders)
     {
	int new_x, new_y;

	border = borders->data;
	e_place_zone_region_smart(zone, borders, border->x, border->y,
				  border->w, border->h, &new_x, &new_y);
	e_border_move(border, new_x, new_y);
	borders = eina_list_remove(borders, border);
     }
}

static int
_e_place_cb_sort_cmp(const void *v1, const void *v2)
{
   return (*((int *)v1)) - (*((int *)v2));
}

static int
_e_place_coverage_border_add(E_Zone *zone, Eina_List *skiplist, int ar, int x, int y, int w, int h)
{
   Eina_List *ll;
   E_Border_List *bl;
   E_Border *bd;
   int x2, y2, w2, h2;
   int ok;
   int iw, ih;
   int x0, x00, y0, y00;

   bl = e_container_border_list_first(zone->container);
   while ((bd = e_container_border_list_next(bl)))
     {
	ok = 1;
	x2 = (bd->x - zone->x); y2 = (bd->y - zone->y); w2 = bd->w; h2 = bd->h;
	for (ll = skiplist; ll; ll = ll->next)
	  {
	     if (ll->data == bd)
	       {
		  ok = 0;
		  break;
	       }
	  }
	if ((ok) && (bd->visible) && E_INTERSECTS(x, y, w, h, x2, y2, w2, h2))
	  {
	     x0 = x;
	     if (x < x2) x0 = x2;
	     x00 = (x + w);
	     if ((x2 + w2) < (x + w)) x00 = (x2 + w2);
	     y0 = y;
	     if (y < y2) y0 = y2;
	     y00 = (y + h);
	     if ((y2 + h2) < (y + h)) y00 = (y2 + h2);
	     iw = x00 - x0;
	     ih = y00 - y0;
	     ar += (iw * ih);
	  }
     }
   e_container_border_list_free(bl);
   return ar;
}

static int
_e_place_coverage_shelf_add(E_Zone *zone, int ar, int x, int y, int w, int h)
{
   Eina_List *l;
   E_Shelf *es;
   int x2, y2, w2, h2;

   for (l = e_shelf_list(); l; l = l->next)
     {

	es = l->data;
	if (es->zone != zone) continue;
	x2 = es->x; y2 = es->y; w2 = es->w; h2 = es->h;
	if (E_INTERSECTS(x, y, w, h, x2, y2, w2, h2))
	  return 0x7fffffff;
     }
   return ar;
}

EAPI int
e_place_zone_region_smart(E_Zone *zone, Eina_List *skiplist, int x, int y, int w, int h, int *rx, int *ry)
{
   int                 a_w = 0, a_h = 0, a_alloc_w = 0, a_alloc_h = 0;
   int                *a_x = NULL, *a_y = NULL;
   int                 zw, zh;
   char               *u_x = NULL, *u_y = NULL;
   Eina_List          *ll;
   E_Border_List      *bl;
   E_Border           *bd;

   *rx = x;
   *ry = y;
#if 0
   /* DISABLE placement entirely for speed testing */
   return 1;
#endif

   if ((w <= 0) || (h <= 0))
     {
	printf("EEEK! trying to place 0x0 window!!!!\n");
	return 1;
     }

   /* FIXME: this NEEDS optimizing */
   a_w = 2;
   a_h = 2;
   a_x = E_NEW(int, 2);
   a_y = E_NEW(int, 2);
   a_alloc_w = 2;
   a_alloc_h = 2;

   x -= zone->x;
   y -= zone->y;
   zw = zone->w;
   zh = zone->h;

   u_x = calloc(zw + 1, sizeof(char));
   u_y = calloc(zh + 1, sizeof(char));

   a_x[0] = 0;
   a_x[1] = zw;
   a_y[0] = 0;
   a_y[1] = zh;

   u_x[0] = 1;
   u_x[zw] = 1;
   u_y[0] = 1;
   u_y[zh] = 1;

   if (e_config->window_placement_policy == E_WINDOW_PLACEMENT_SMART)
     {
	Eina_List *l;

	for (l = e_shelf_list(); l; l = l->next)
	  {
	     E_Shelf *es;
	     int bx, by, bw, bh;

	     es = l->data;
	     if (es->zone != zone) continue;

	     bx = es->x;
	     by = es->y;
	     bw = es->w;
	     bh = es->h;
	     if (!E_INTERSECTS(bx, by, bw, bh, 0, 0, zw, zh)) continue;

	     if (bx < 0)
	       {
		  bw += bx;
		  bx = 0;
	       }
	     if ((bx + bw) > zw) bw = zw - bx;
	     if (bx >= zw) continue;
	     if (by < 0)
	       {
		  bh += by;
		  by = 0;
	       }
	     if ((by + bh) > zh) bh = zh - by;
	     if (by >= zh) continue;
	     if (!u_x[bx])
	       {
		  a_w++;
		  if (a_w > a_alloc_w)
		    {
		       a_alloc_w += 32;
		       E_REALLOC(a_x, int, a_alloc_w);
		    }
		  a_x[a_w - 1] = bx;
		  u_x[bx] = 1;
	       }
	     if (!u_x[bx + bw])
	       {
		  a_w++;
		  if (a_w > a_alloc_w)
		    {
		       a_alloc_w += 32;
		       E_REALLOC(a_x, int, a_alloc_w);
		    }
		  a_x[a_w - 1] = bx + bw;
		  u_x[bx + bw] = 1;
	       }
	     if (!u_y[by])
	       {
		  a_h++;
		  if (a_h > a_alloc_h)
		    {
		       a_alloc_h += 32;
		       E_REALLOC(a_y, int, a_alloc_h);
		    }
		  a_y[a_h - 1] = by;
		  u_y[by] = 1;
	       }
	     if (!u_y[by + bh])
	       {
		  a_h++;
		  if (a_h > a_alloc_h)
		    {
		       a_alloc_h += 32;
		       E_REALLOC(a_y, int, a_alloc_h);
		    }
		  a_y[a_h - 1] = by + bh;
		  u_y[by + bh] = 1;
	       }
	  }
     }

   bl = e_container_border_list_first(zone->container);
   while ((bd = e_container_border_list_next(bl)))
     {
	int ok;
	int bx, by, bw, bh;

	ok = 1;
	for (ll = skiplist; ll; ll = ll->next)
	  {
	     if (ll->data == bd)
	       {
		  ok = 0;
		  break;
	       }
	  }
	if ((!ok) || (!bd->visible)) continue;

	bx = bd->x - zone->x;
	by = bd->y - zone->y;
	bw = bd->w;
	bh = bd->h;

	if (E_INTERSECTS(bx, by, bw, bh, 0, 0, zw, zh))
	  {
	     if (bx < 0)
	       {
		  bw += bx;
		  bx = 0;
	       }
	     if ((bx + bw) > zw) bw = zw - bx;
	     if (bx >= zw) continue;
	     if (by < 0)
	       {
		  bh += by;
		  by = 0;
	       }
	     if ((by + bh) > zh) bh = zh - by;
	     if (by >= zh) continue;
	     if (!u_x[bx])
	       {
		  a_w++;
		  if (a_w > a_alloc_w)
		    {
		       a_alloc_w += 32;
		       E_REALLOC(a_x, int, a_alloc_w);
		    }
		  a_x[a_w - 1] = bx;
		  u_x[bx] = 1;
	       }
	     if (!u_x[bx + bw])
	       {
		  a_w++;
		  if (a_w > a_alloc_w)
		    {
		       a_alloc_w += 32;
		       E_REALLOC(a_x, int, a_alloc_w);
		    }
		  a_x[a_w - 1] = bx + bw;
		  u_x[bx + bw] = 1;
	       }
	     if (!u_y[by])
	       {
		  a_h++;
		  if (a_h > a_alloc_h)
		    {
		       a_alloc_h += 32;
		       E_REALLOC(a_y, int, a_alloc_h);
		    }
		  a_y[a_h - 1] = by;
		  u_y[by] = 1;
	       }
	     if (!u_y[by + bh])
	       {
		  a_h++;
		  if (a_h > a_alloc_h)
		    {
		       a_alloc_h += 32;
		       E_REALLOC(a_y, int, a_alloc_h);
		    }
		  a_y[a_h - 1] = by + bh;
		  u_y[by + bh] = 1;
	       }
	  }
     }
   qsort(a_x, a_w, sizeof(int), _e_place_cb_sort_cmp);
   qsort(a_y, a_h, sizeof(int), _e_place_cb_sort_cmp);
   e_container_border_list_free(bl);
   free(u_x);
   free(u_y);

   {
      int                 i, j;
      int                 area = 0x7fffffff;

      for (j = 0; j < a_h - 1; j++)
	{
	   for (i = 0; i < a_w - 1; i++)
	     {
		if ((a_x[i] <= (zw - w)) &&
		    (a_y[j] <= (zh - h)))
		  {
		     int ar = 0;

		     ar = _e_place_coverage_border_add(zone, skiplist, ar,
						       a_x[i], a_y[j],
						       w, h);
		     if (e_config->window_placement_policy == E_WINDOW_PLACEMENT_SMART)
		       ar = _e_place_coverage_shelf_add(zone, ar,
							a_x[i], a_y[j],
							w, h);
		     if (ar < area)
		       {
			  area = ar;
			  *rx = a_x[i];
			  *ry = a_y[j];
			  if (ar == 0) goto done;
		       }
		  }
		if ((a_x[i + 1] - w > 0) && (a_y[j] <= (zh - h)))
		  {
		     int ar = 0;

		     ar = _e_place_coverage_border_add(zone, skiplist, ar,
						       a_x[i + 1] - w, a_y[j],
						       w, h);
		     if (e_config->window_placement_policy == E_WINDOW_PLACEMENT_SMART)
		       ar = _e_place_coverage_shelf_add(zone, ar,
							a_x[i + 1] - w, a_y[j],
							w, h);
		     if (ar < area)
		       {
			  area = ar;
			  *rx = a_x[i + 1] - w;
			  *ry = a_y[j];
			  if (ar == 0) goto done;
		       }
		  }
		if ((a_x[i + 1] - w > 0) && (a_y[j + 1] - h > 0))
		  {
		     int ar = 0;

		     ar = _e_place_coverage_border_add(zone, skiplist, ar,
						       a_x[i + 1] - w, a_y[j + 1] - h,
						       w, h);
		     if (e_config->window_placement_policy == E_WINDOW_PLACEMENT_SMART)
		       ar = _e_place_coverage_shelf_add(zone, ar,
							a_x[i + 1] - w, a_y[j + 1] - h,
							w, h);
		     if (ar < area)
		       {
			  area = ar;
			  *rx = a_x[i + 1] - w;
			  *ry = a_y[j + 1] - h;
			  if (ar == 0) goto done;
		       }
		  }
		if ((a_x[i] <= (zw - w)) && (a_y[j + 1] - h > 0))
		  {
		     int ar = 0;

		     ar = _e_place_coverage_border_add(zone, skiplist, ar,
						       a_x[i], a_y[j + 1] - h,
						       w, h);
		     if (e_config->window_placement_policy == E_WINDOW_PLACEMENT_SMART)
		       ar = _e_place_coverage_shelf_add(zone, ar,
							a_x[i], a_y[j + 1] - h,
							w, h);
		     if (ar < area)
		       {
			  area = ar;
			  *rx = a_x[i];
			  *ry = a_y[j + 1] - h;
			  if (ar == 0) goto done;
		       }
		  }
	     }
	}
   }
 done:
   E_FREE(a_x);
   E_FREE(a_y);

   if ((*rx + w) > zone->w) *rx = zone->w - w;
   if (*rx < 0) *rx = 0;
   if ((*ry + h) > zone->h) *ry = zone->h - h;
   if (*ry < 0) *ry = 0;

//   printf("0 - PLACE %i %i | %ix%i\n", *rx, *ry, w, h);

   *rx += zone->x;
   *ry += zone->y;
   return 1;
}

EAPI int
e_place_zone_cursor(E_Zone *zone, int x, int y, int w, int h, int it, int *rx, int *ry)
{
   int cursor_x = 0, cursor_y = 0;
   int zone_right, zone_bottom;

   E_OBJECT_CHECK_RETURN(zone, 0);

   ecore_x_pointer_xy_get(zone->container->win, &cursor_x, &cursor_y);
   *rx = cursor_x - (w >> 1);
   *ry = cursor_y - (it >> 1);

   if (*rx < zone->x)
     *rx = zone->x;

   if (*ry < zone->y)
     *ry = zone->y;

   zone_right = zone->x + zone->w;
   zone_bottom = zone->y + zone->h;

   if ((*rx + w) > zone_right)
     *rx = zone_right - w;

   if ((*ry + h) > zone_bottom)
     *ry = zone_bottom - h;

   return 1;
}

EAPI int
e_place_zone_manual(E_Zone *zone, int w, int h, int *rx, int *ry)
{
   int cursor_x = 0, cursor_y = 0;

   E_OBJECT_CHECK_RETURN(zone, 0);

   ecore_x_pointer_xy_get(zone->container->win, &cursor_x, &cursor_y);
   if (rx) *rx = cursor_x - (w >> 1);
   if (ry) *ry = cursor_y - (h >> 1);

   return 1;
}
