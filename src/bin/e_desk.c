/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* E_Desk is a child object of E_Zone. A desk is essentially a background
 * and an associated set of client windows. Each zone can have an arbitrary
 * number of desktops.
 */

static void _e_desk_free(E_Desk *desk);
static void _e_border_event_desk_show_free(void *data, void *ev);

int E_EVENT_DESK_SHOW = 0;

int
e_desk_init(void)
{
   E_EVENT_DESK_SHOW = ecore_event_type_new();
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
   Evas_Object *o;
   char		name[40];
   
   E_OBJECT_CHECK_RETURN(zone, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(zone, E_ZONE_TYPE, NULL);
   
   desk = E_OBJECT_ALLOC(E_Desk, E_DESK_TYPE, _e_desk_free);
   if (!desk) return NULL;

   desk->zone = zone;
   desk->x = x;
   desk->y = y;
   snprintf(name, sizeof(name), _("Desktop %d, %d"), x, y);
   desk->name = strdup(name);

   o = evas_object_rectangle_add(zone->container->bg_evas);
   desk->bg_black_object = o;
   evas_object_layer_set(o, 100);
   evas_object_move(o, zone->x, zone->y);
   evas_object_resize(o, zone->w, zone->h);
   evas_object_color_set(o, 0, 0, 0, 255);
   snprintf(name, sizeof(name), "desktop/background/%d.%d", desk->x, desk->y);
   evas_object_name_set(o, name);
   evas_object_data_set(o, "e_desk", desk);

   return desk;
}

void
e_desk_name_set(E_Desk *desk, const char *name)
{
   E_OBJECT_CHECK(desk);
   E_OBJECT_TYPE_CHECK(desk, E_DESK_TYPE);
   E_FREE(desk->name);
   desk->name = strdup(name);
}

void
e_desk_show(E_Desk *desk)
{
   E_Border_List     *bl;
   int                x, y;
   E_Event_Desk_Show *ev;
   E_Border *bd;

   E_OBJECT_CHECK(desk);
   E_OBJECT_TYPE_CHECK(desk, E_DESK_TYPE);
   if (desk->visible) return;
   
   bl = e_container_border_list_first(desk->zone->container);
   while ((bd = e_container_border_list_next(bl)))
     {
	if ((bd->desk->zone == desk->zone) && (!bd->iconic))
	  {
	     if ((bd->desk == desk) || (bd->sticky))
	       e_border_show(bd);
	     else if (bd->moving)
	       e_border_desk_set(bd, desk);
	     else
	       e_border_hide(bd, 1);
	  }
     }
   e_container_border_list_free(bl);
   
   for (x = 0; x < desk->zone->desk_x_count; x++)
     {
	for (y = 0; y < desk->zone->desk_y_count; y++)
	  {
	     E_Desk *desk2;

	     desk2 = e_desk_at_xy_get(desk->zone,x, y);
	     desk2->visible = 0;
	     evas_object_hide(desk2->bg_black_object);
	     if (desk2 == desk)
	       {
		  desk->zone->desk_x_current = x;
		  desk->zone->desk_y_current = y;
	       }
	  }
     }
   if (desk->black)
     evas_object_show(desk->bg_black_object);
   desk->visible = 1;

   ev = E_NEW(E_Event_Desk_Show, 1);
   ev->desk = desk;
   e_object_ref(E_OBJECT(desk));
   ecore_event_add(E_EVENT_DESK_SHOW, ev, _e_border_event_desk_show_free, NULL);
}

void
e_desk_black_set(E_Desk *desk, int set)
{
   if ((!desk->black) && (set))
     {
	evas_object_show(desk->bg_black_object);
	desk->black = 1;
     }
   else if ((desk->black) && (!set))
     {
	evas_object_hide(desk->bg_black_object);
	desk->black = 0;
     }
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
   E_OBJECT_TYPE_CHECK_RETURN(zone, E_ZONE_TYPE, NULL);
   return e_desk_at_xy_get(zone, zone->desk_x_current, zone->desk_y_current);
}

E_Desk *
e_desk_at_xy_get(E_Zone *zone, int x, int y)
{
   E_OBJECT_CHECK_RETURN(zone, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(zone, E_ZONE_TYPE, NULL);

   if ((x >= zone->desk_x_count) || (y >= zone->desk_y_count))
     return NULL;

   return zone->desks[x + (y * zone->desk_x_count)];
}

E_Desk *
e_desk_at_pos_get(E_Zone *zone, int pos)
{
   int x, y;

   E_OBJECT_CHECK_RETURN(zone, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(zone, E_ZONE_TYPE, NULL);

   y = pos / zone->desk_x_count;
   x = pos - (y * zone->desk_x_count);

   if ((x >= zone->desk_x_count) || (y >= zone->desk_y_count))
     return NULL;

   return zone->desks[x + (y * zone->desk_x_count)];
}

void
e_desk_xy_get(E_Desk *desk, int *x, int *y)
{
   E_OBJECT_CHECK(desk);
   E_OBJECT_TYPE_CHECK(desk, E_DESK_TYPE);

   if (x) *x = desk->x;
   if (y) *y = desk->y;
}

void
e_desk_next(E_Zone *zone)
{
   int          x, y;

   E_OBJECT_CHECK(zone);
   E_OBJECT_TYPE_CHECK(zone, E_ZONE_TYPE);
   
   if ((zone->desk_x_count < 2) && 
       (zone->desk_y_count < 2))
     return;
   
   x = zone->desk_x_current;
   y = zone->desk_y_current;
   
   x++;
   if (x >= zone->desk_x_count)
     {
	x = 0;
	y++;
	if (y >= zone->desk_y_count) y = 0;
     }
   
   e_desk_show(e_desk_at_xy_get(zone, x, y));
}

void
e_desk_prev(E_Zone *zone)
{
   int          x, y;

   E_OBJECT_CHECK(zone);
   E_OBJECT_TYPE_CHECK(zone, E_ZONE_TYPE);

   if ((zone->desk_x_count < 2) && 
       (zone->desk_y_count < 2))
     return;

   x = zone->desk_x_current;
   y = zone->desk_y_current;

   x--;
   if (x < 0)
     {
	x = zone->desk_x_count - 1;
	y--;
	if (y < 0) y = zone->desk_y_count - 1;
     }
   e_desk_show(e_desk_at_xy_get(zone, x, y));
}

static void
_e_desk_free(E_Desk *desk)
{
   E_FREE(desk->name);
   free(desk);
}

static void
_e_border_event_desk_show_free(void *data, void *event)
{
   E_Event_Desk_Show *ev;

   ev = event;
   e_object_unref(E_OBJECT(ev->desk));
   free(ev);
}

