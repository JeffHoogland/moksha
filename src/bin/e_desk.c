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

   /* TODO: config the ecore_evas type. */
   desk->black_ecore_evas = ecore_evas_software_x11_new(NULL, zone->container->win,
							0, 0, zone->w, zone->h);
   ecore_evas_software_x11_direct_resize_set(desk->black_ecore_evas, 1);
   ecore_evas_override_set(desk->black_ecore_evas, 1);
   ecore_evas_layer_set(desk->black_ecore_evas, 6);

   desk->black_win = ecore_evas_software_x11_window_get(desk->black_ecore_evas);
   desk->black_evas = ecore_evas_get(desk->black_ecore_evas);

   o = evas_object_rectangle_add(desk->black_evas);
   evas_object_move(o, 0, 0);
   evas_object_resize(o, zone->w, zone->h);
   evas_object_color_set(o, 0, 0, 0, 255);
   ecore_evas_name_class_set(desk->black_ecore_evas, "E", "Black_Window");
   snprintf(name, sizeof(name), "Enlightenment Black Desk (%d, %d)", desk->x, desk->y);
   ecore_evas_title_set(desk->black_ecore_evas, name);

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
   E_Border          *bd;
   int                was_zone = 0;
   int                x, y;
   E_Event_Desk_Show *ev;

   E_OBJECT_CHECK(desk);
   E_OBJECT_TYPE_CHECK(desk, E_DESK_TYPE);
   if (desk->visible) return;

   for (x = 0; x < desk->zone->desk_x_count; x++)
     {
	for (y = 0; y < desk->zone->desk_y_count; y++)
	  {
	     E_Desk *desk2;

	     desk2 = e_desk_at_xy_get(desk->zone,x, y);
	     if (desk2->visible)
	       {
		  desk2->visible = 0;
		  ecore_evas_hide(desk2->black_ecore_evas);
	       }
	  }
     }

   bl = e_container_border_list_first(desk->zone->container);
   if (desk->zone->bg_object) was_zone = 1;
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

   if (e_config->focus_last_focused_per_desktop)
     e_desk_last_focused_focus(desk);
	
   desk->zone->desk_x_current = desk->x;
   desk->zone->desk_y_current = desk->y;
   if (desk->fullscreen)
     {
	ecore_evas_show(desk->black_ecore_evas);
	e_container_window_raise(desk->zone->container, desk->black_win, 150);
	e_border_fullscreen(desk->fullscreen);
     }
   desk->visible = 1;

   if (was_zone)
     e_bg_zone_update(desk->zone, E_BG_TRANSITION_DESK);
   else
     e_bg_zone_update(desk->zone, E_BG_TRANSITION_START);
   
   ev = E_NEW(E_Event_Desk_Show, 1);
   ev->desk = desk;
   e_object_ref(E_OBJECT(desk));
   ecore_event_add(E_EVENT_DESK_SHOW, ev, _e_border_event_desk_show_free, NULL);
   
}

void
e_desk_last_focused_focus(E_Desk *desk)
{
   Evas_List *l;
   E_Border *bd;
   
   for (l = e_border_focus_stack_get(); l; l = l->next)
     {
	bd = l->data;
	if ((!bd->iconic) && (bd->visible) &&
	    (((bd->desk == desk) ||
	      ((bd->sticky) && (bd->zone == desk->zone)))))
	  {
	     /* this was the window last focused in this desktop */
	     e_border_focus_set(bd, 1, 1);
	     break;
	  }
     }
}

void
e_desk_fullscreen_set(E_Desk *desk, E_Border *bd)
{
   if ((!desk->fullscreen) && (bd))
     {
	ecore_evas_show(desk->black_ecore_evas);
	e_container_window_raise(desk->zone->container, desk->black_win, 150);
	desk->fullscreen = bd;
     }
   else if ((desk->fullscreen) && (!bd))
     {
	ecore_evas_hide(desk->black_ecore_evas);
	desk->fullscreen = NULL;
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

