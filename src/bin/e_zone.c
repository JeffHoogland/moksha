/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* E_Zone is a child object of E_Container. There is one zone per screen
 * in a xinerama setup. Each zone has one or more desktops.
 */

static void _e_zone_free(E_Zone *zone);
static void _e_zone_cb_bg_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void _e_zone_cb_bg_mouse_up(void *data, Evas *evas, Evas_Object *obj, void *event_info);
static void _e_zone_cb_bg_mouse_move(void *data, Evas *evas, Evas_Object *obj, void *event_info);

E_Zone *
e_zone_new(E_Container *con, int x, int y, int w, int h)
{
   E_Zone      *zone;
   E_Desk      *desk;

   zone = E_OBJECT_ALLOC(E_Zone, _e_zone_free);
   if (!zone) return NULL;

   printf("NEW ZONE! %d %d %d %d\n", x, y, w, h);

   zone->container = con;
   zone->name = NULL;

   zone->x = x;
   zone->y = y;
   zone->w = w;
   zone->h = h;
   zone->num = evas_list_count(con->zones) + 1;

   e_object_ref(E_OBJECT(con));
   con->zones = evas_list_append(con->zones, zone);
   
   if (1)
     {
	char name[40];
	Evas_Object *o;

	o = evas_object_rectangle_add(con->bg_evas);
	zone->bg_blank_object = o;
	evas_object_layer_set(o, -100);
	evas_object_move(o, x, y);
	evas_object_resize(o, w, h);
	evas_object_color_set(o, 255, 255, 255, 255);
	evas_object_show(o);

	o = edje_object_add(con->bg_evas);
	zone->bg_object = o;
	evas_object_layer_set(o, -1);
	snprintf(name, sizeof(name), "desktop/background/%d", zone->num);
	evas_object_name_set(o, name);
	evas_object_data_set(o, "e_zone", zone);
	evas_object_move(o, x, y);
	evas_object_resize(o, w, h);
	edje_object_file_set(o,
			     e_config->desktop_default_background,
			     "desktop/background");
	evas_object_show(o);

	o = evas_object_rectangle_add(con->bg_evas);
	zone->bg_event_object = o;
	evas_object_move(o, x, y);
	evas_object_resize(o, w, h);
	evas_object_color_set(o, 255, 255, 255, 0);
	evas_object_show(o);
	evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _e_zone_cb_bg_mouse_down, zone);
	evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_UP,   _e_zone_cb_bg_mouse_up, zone);
	evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_MOVE, _e_zone_cb_bg_mouse_move, zone);
     }

   /* Start off with a single desktop */
   desk = e_desk_new(zone);
   e_desk_show(desk);

   return zone;
}

void
e_zone_name_set(E_Zone *zone, const char *name)
{
   E_OBJECT_CHECK(zone);
   if (zone->name)
      free(zone->name);
   zone->name = strdup(name);
}

void
e_zone_move(E_Zone *zone, int x, int y)
{
   E_OBJECT_CHECK(zone);
   
   if ((x == zone->x) && (y == zone->y)) return;
   zone->x = x;
   zone->y = y;
   evas_object_move(zone->bg_blank_object, x, y);
   evas_object_move(zone->bg_object, x, y);
   evas_object_move(zone->bg_event_object, x, y);
}

void
e_zone_resize(E_Zone *zone, int w, int h)
{
   E_OBJECT_CHECK(zone);
   
   if ((w == zone->w) && (h == zone->h)) return;
   zone->w = w;
   zone->h = h;
   evas_object_resize(zone->bg_blank_object, w, h);
   evas_object_resize(zone->bg_object, w, h);
   evas_object_resize(zone->bg_event_object, w, h);
}

void
e_zone_move_resize(E_Zone *zone, int x, int y, int w, int h)
{
   E_OBJECT_CHECK(zone);

   if ((x == zone->x) && (y == zone->y) && (w == zone->w) && (h == zone->h))
     return;

   zone->x = x;
   zone->y = y;
   zone->w = w;
   zone->h = h;
   
   evas_object_move(zone->bg_blank_object, x, y);
   evas_object_move(zone->bg_object, x, y);
   evas_object_move(zone->bg_event_object, x, y);
   evas_object_resize(zone->bg_blank_object, w, h);
   evas_object_resize(zone->bg_object, w, h);
   evas_object_resize(zone->bg_event_object, w, h);
} 

E_Zone *
e_zone_current_get(E_Container *con)
{
   Evas_List *l;

   E_OBJECT_CHECK_RETURN(con, NULL);
   l = con->zones;
   /* FIXME: Should return the zone the pointer is currently in */
   return (E_Zone *)l->data;
}

void
e_zone_bg_reconfigure(E_Zone *zone)
{
   Evas_Object *o;
   
   E_OBJECT_CHECK(zone);
   o = zone->bg_object;
   evas_object_hide(o);
   edje_object_file_set(o,
			e_config->desktop_default_background,
			"desktop/background");
   evas_object_layer_set(o, -1);
   evas_object_show(o);
}

Evas_List *
e_zone_clients_list_get(E_Zone *zone)
{
    E_OBJECT_CHECK_RETURN(zone, NULL);
    return zone->clients;
}
   

static void
_e_zone_free(E_Zone *zone)
{
   E_Container *con = zone->container;
   if (zone->name)
     free(zone->name);
   con->zones = evas_list_remove(con->zones, zone);
   e_object_unref(E_OBJECT(zone->container));
   free(zone);
}

static void
_e_zone_cb_bg_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
   E_Zone *zone;
   Evas_Event_Mouse_Down *ev;
   
   ev = (Evas_Event_Mouse_Down *)event_info;
   zone = data;
   if (e_menu_grab_window_get()) return;
    
   if (ev->button == 1)
     {
	E_Menu *m;
	
	m = e_int_menus_main_new();
	e_menu_activate_mouse(m, zone->container, ev->output.x, ev->output.y, 1, 1,
			      E_MENU_POP_DIRECTION_DOWN);
	e_util_container_fake_mouse_up_all_later(zone->container);
     }
   else if (ev->button == 2)
     {
	E_Menu *m;

	m = e_int_menus_clients_new();
	/* FIXME: this is a bit of a hack... setting m->zone - bad hack */
	m->con = zone->container;
	e_menu_activate_mouse(m, zone->container, ev->output.x, ev->output.y, 1, 1,
			      E_MENU_POP_DIRECTION_DOWN);
	e_util_container_fake_mouse_up_all_later(zone->container);
     }
   else if (ev->button == 3)
     {
	E_Menu *m;
	
	m = e_int_menus_favorite_apps_new(1);
	e_menu_activate_mouse(m, zone->container, ev->output.x, ev->output.y, 1, 1,
			      E_MENU_POP_DIRECTION_DOWN);
	e_util_container_fake_mouse_up_all_later(zone->container);
     }
}

static void
_e_zone_cb_bg_mouse_up(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
   E_Zone *zone;
   Evas_Event_Mouse_Up *ev;
   
   ev = (Evas_Event_Mouse_Up *)event_info;      
   zone = data;
}

static void
_e_zone_cb_bg_mouse_move(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
   E_Zone *zone;
   Evas_Event_Mouse_Move *ev;
   
   ev = (Evas_Event_Mouse_Move *)event_info;   
   zone = data;
}

