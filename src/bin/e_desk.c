/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* E_Desk is a child object of E_Zone. A desk is essentially a background
 * and an associated set of client windows. Each zone can have an arbitrary
 * number of desktops.
 */

static void _e_desk_free(E_Desk *desk);

E_Desk *
e_desk_new(E_Zone *zone)
{
   E_Desk      *desk;
   char		name[40];
   
   desk = E_OBJECT_ALLOC(E_Desk, _e_desk_free);
   if (!desk) return NULL;

   desk->clients = NULL;
   desk->zone = zone;
   desk->num = evas_list_count(zone->desks) + 1;
   snprintf(name, sizeof(name), "Desktop %d", desk->num);
   desk->name = strdup(name);
   e_object_ref(E_OBJECT(zone));
   zone->desks = evas_list_append(zone->desks, desk);

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
   
   E_OBJECT_CHECK(desk);
   if (desk->visible) return;
   
   for (l = desk->zone->clients; l; l = l->next)
     {
	E_Border *bd = l->data;

	if (bd->desk == desk)
	  e_border_show(bd);
	else
	  e_border_hide(bd);
     }
   
   for (l = desk->zone->desks; l; l = l->next)
     {
	E_Desk *d = l->data;
	d->visible = 0;
     }
   desk->visible = 1;
}

void
e_desk_remove(E_Desk *desk)
{
   Evas_List *l;
   E_Desk *previous;
   
   E_OBJECT_CHECK(desk);
   if (evas_list_count(desk->zone->desks) < 2)
     return;
   l = evas_list_find(desk->zone->desks, desk);
   l = l->prev;
   if (!l) l = evas_list_last(desk->zone->desks);
   previous = l->data;

   for (l = desk->clients; l; l = l->next)
     {
	E_Border *bd = l->data;
	bd->desk = previous;
     }
   desk->zone->desks = evas_list_remove(desk->zone->desks, desk);
   e_desk_show(previous);
   
   evas_list_free(desk->clients);
   e_object_del(E_OBJECT(desk));
}

E_Desk *
e_desk_current_get(E_Zone *zone)
{
   Evas_List *l;
   
   E_OBJECT_CHECK_RETURN(zone, NULL);
   
   for (l = zone->desks; l; l = l->next)
     {
	E_Desk *desk = l->data;
	if (desk->visible)
	  return desk;
     }

   return NULL;   
}

void
e_desk_next(E_Zone *zone)
{
   Evas_List   *l;
   E_Desk      *desk;

   E_OBJECT_CHECK(zone);
   
   if (evas_list_count(zone->desks) < 2)
      return;
   
   /* Locate the position of the current desktop in the list */
   desk = e_desk_current_get(zone);
   l = evas_list_find_list(zone->desks, desk);
   if (!l) return; /* Couldn't help putting this here */
   
   l = l->next;
   if (!l) l = zone->desks; /* Wraparound */

   /* Show the desktop */
   desk = l->data;
   e_desk_show(desk);
}

void
e_desk_prev(E_Zone *zone)
{
   Evas_List   *l;
   E_Desk      *desk;

   E_OBJECT_CHECK(zone);
   
   if (evas_list_count(zone->desks) < 2)
      return;
   
   /* Locate the position of the current desktop in the list */
   desk = e_desk_current_get(zone);
   l = evas_list_find_list(zone->desks, desk);
   if (!l) return; /* Couldn't help putting this here */
   
   l = l->prev;
   if (!l) l = evas_list_last(zone->desks); /* Wraparound */

   /* Show the desktop */
   desk = l->data;
   e_desk_show(desk);
}

static void
_e_desk_free(E_Desk *desk)
{
   E_Zone *zone = desk->zone;
   if (desk->name)
     free(desk->name);
   zone->desks = evas_list_remove(zone->desks, desk);
   e_object_unref(E_OBJECT(desk->zone));
   free(desk);
}

