/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* E_Desk is a child object of E_Zone. A desk is essentially a background
 * and an associated set of client windows. Each zone can have an arbitrary
 * number of desktops.
 */

static void _e_desk_free(E_Desk *desk);
static int desk_count;

int
e_desk_init(void)
{
   desk_count = 0;
   return 1;
}

int
e_desk_shutdown(void)
{
   return 1;
}

E_Desk *
e_desk_new(E_Zone *zone, int x, int y)
{
   E_Desk      *desk;
   char		name[40];
   
   E_OBJECT_CHECK_RETURN(zone, NULL);
   
   desk = E_OBJECT_ALLOC(E_Desk, _e_desk_free);
   if (!desk) return NULL;

   desk->clients = NULL;
   desk->zone = zone;
   desk->num = ++desk_count;
   snprintf(name, sizeof(name), "Desktop %d, %d", x, y);
   desk->name = strdup(name);
   return desk;
}

void
e_desk_name_set(E_Desk *desk, const char *name)
{
   E_OBJECT_CHECK(desk);
   if (desk->name)
      free(desk->name);
   desk->name = strdup(name);
}

void
e_desk_show(E_Desk *desk)
{
   Evas_List   *l;
   int          x, y;
   
   E_OBJECT_CHECK(desk);
   if (desk->visible) return;
   
   for (l = desk->zone->container->clients; l; l = l->next)
     {
	E_Border *bd = l->data;

	if (bd->desk->zone == desk->zone && !bd->iconic)
	  {
	     if (bd->desk == desk || bd->sticky)
	       {
		  e_border_show(bd);
	       }
	     else
	       {
		  e_border_hide(bd);
	       }
	  }
     }
   
   for (x = 0; x < desk->zone->desk_x_count; x++)
     {
	for (y = 0; y < desk->zone->desk_y_count; y++)
	  {
	     E_Desk *next;
	     next = e_desk_at_xy_get(desk->zone,x, y);
	     next->visible = 0;
	     if (next == desk)
	       {
		  desk->zone->desk_x_current = x;
		  desk->zone->desk_y_current = y;
	       }
	  }
     }
   desk->visible = 1;
}

void
e_desk_row_add(E_Zone *zone)
{
   e_zone_desk_count_set(zone, zone->desk_x_count, zone->desk_y_count + 1);
}

void
e_desk_row_remove(E_Zone *zone)
{
   e_zone_desk_count_set(zone, zone->desk_x_count, zone->desk_y_count - 1);
}

void
e_desk_col_add(E_Zone *zone)
{
   e_zone_desk_count_set(zone, zone->desk_x_count + 1, zone->desk_y_count);
}

void
e_desk_col_remove(E_Zone *zone)
{
   e_zone_desk_count_set(zone, zone->desk_x_count - 1, zone->desk_y_count);
}

E_Desk *
e_desk_current_get(E_Zone *zone)
{
   E_OBJECT_CHECK_RETURN(zone, NULL);
  
   return e_desk_at_xy_get(zone, zone->desk_x_current, zone->desk_y_current);
}

E_Desk *
e_desk_at_xy_get(E_Zone *zone, int x, int y)
{
   E_OBJECT_CHECK_RETURN(zone, NULL);

   if (x >= zone->desk_x_count || y >= zone->desk_y_count)
     return NULL;

   return (E_Desk *) zone->desks[x + (y * zone->desk_x_count)];
}

void
e_desk_next(E_Zone *zone)
{
   Evas_List   *l;
   E_Desk      *desk;
   int          x, y;

   E_OBJECT_CHECK(zone);
   
   if (zone->desk_x_count < 2 && zone->desk_y_count < 2)
      return;
   
   x = zone->desk_x_current;
   y = zone->desk_y_current;

   x++;
   if (x == zone->desk_x_count)
     {
	x = 0;
	y++;
	if (y == zone->desk_y_count)
	  y = 0;
     }
   
   e_desk_show(e_desk_at_xy_get(zone, x, y));
}

void
e_desk_prev(E_Zone *zone)
{
   Evas_List   *l;
   E_Desk      *desk;
   int          x, y;

   E_OBJECT_CHECK(zone);

   if (zone->desk_x_count < 2 && zone->desk_y_count < 2)
     return;

   x = zone->desk_x_current;
   y = zone->desk_y_current;

   x--;
   if (x < 0)
     {
	x = zone->desk_x_count - 1;
	y--;
	if (y < 0)
	  y = zone->desk_y_count - 1;
	
     }

   e_desk_show(e_desk_at_xy_get(zone, x, y));
}

static void
_e_desk_free(E_Desk *desk)
{
   E_Zone *zone = desk->zone;
   if (desk->name)
     free(desk->name);
//   zone->desks = evas_list_remove(zone->desks, desk);
   free(desk);
}

