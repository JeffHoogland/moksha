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
static void _e_zone_event_zone_desk_count_set_free(void *data, void *ev);
static int  _e_zone_cb_mouse_in(void *data, int type, void *event);
static int  _e_zone_cb_mouse_out(void *data, int type, void *event);
static int  _e_zone_cb_mouse_move(void *data, int type, void *event);
static int  _e_zone_cb_timer(void *data);
static int  _e_zone_cb_desk_show(void *data, int type, void *event);
static void _e_zone_update_flip(E_Zone *zone);
static void _e_zone_update_edge(E_Zone *zone);
static void _e_zone_event_move_resize_free(void *data, void *ev);
static void _e_zone_event_add_free(void *data, void *ev);
static void _e_zone_event_del_free(void *data, void *ev);
static void _e_zone_object_del_attach(void *o);

EAPI int E_EVENT_ZONE_DESK_COUNT_SET = 0;
EAPI int E_EVENT_POINTER_WARP = 0;
EAPI int E_EVENT_ZONE_MOVE_RESIZE = 0;
EAPI int E_EVENT_ZONE_ADD = 0;
EAPI int E_EVENT_ZONE_DEL = 0;
EAPI int E_EVENT_ZONE_EDGE_IN = 0;
EAPI int E_EVENT_ZONE_EDGE_OUT = 0;
EAPI int E_EVENT_ZONE_EDGE_MOVE = 0;

#define E_ZONE_FLIP_LEFT(zone)  ((e_config->desk_flip_wrap && ((zone)->desk_x_count > 1)) || ((zone)->desk_x_current > 0))
#define E_ZONE_FLIP_RIGHT(zone) ((e_config->desk_flip_wrap && ((zone)->desk_x_count > 1)) || (((zone)->desk_x_current + 1) < (zone)->desk_x_count))
#define E_ZONE_FLIP_UP(zone)    ((e_config->desk_flip_wrap && ((zone)->desk_y_count > 1)) || ((zone)->desk_y_current > 0))
#define E_ZONE_FLIP_DOWN(zone)  ((e_config->desk_flip_wrap && ((zone)->desk_y_count > 1)) || (((zone)->desk_y_current + 1) < (zone)->desk_y_count))

EAPI int
e_zone_init(void)
{
   E_EVENT_ZONE_DESK_COUNT_SET = ecore_event_type_new();
   E_EVENT_POINTER_WARP = ecore_event_type_new();
   E_EVENT_ZONE_MOVE_RESIZE = ecore_event_type_new();
   E_EVENT_ZONE_ADD = ecore_event_type_new();
   E_EVENT_ZONE_DEL = ecore_event_type_new();
   E_EVENT_ZONE_EDGE_IN = ecore_event_type_new();
   E_EVENT_ZONE_EDGE_OUT = ecore_event_type_new();
   E_EVENT_ZONE_EDGE_MOVE = ecore_event_type_new();
   return 1;
}

EAPI int
e_zone_shutdown(void)
{
   return 1;
}

EAPI E_Zone *
e_zone_new(E_Container *con, int num, int id, int x, int y, int w, int h)
{
   E_Zone *zone;
   char    name[40];
   Evas_Object *o;
   E_Event_Zone_Add *ev;

   zone = E_OBJECT_ALLOC(E_Zone, E_ZONE_TYPE, _e_zone_free);
   if (!zone) return NULL;

   zone->container = con;

   zone->x = x;
   zone->y = y;
   zone->w = w;
   zone->h = h;
   zone->num = num;
   zone->id = id;

   zone->edge.left = ecore_x_window_input_new(con->win, zone->x, zone->y, 1, zone->h);
   e_container_window_raise(zone->container, zone->edge.left, 999);
   zone->edge.right = ecore_x_window_input_new(con->win, zone->x + zone->w - 1, zone->y, 1, zone->h);
   e_container_window_raise(zone->container, zone->edge.right, 999);
   zone->edge.top = ecore_x_window_input_new(con->win, zone->x + 1, zone->y, zone->w - 2, 1);
   e_container_window_raise(zone->container, zone->edge.top, 999);
   zone->edge.bottom = ecore_x_window_input_new(con->win, zone->x + 1, zone->y + zone->h - 1, zone->w - 2, 1);
   e_container_window_raise(zone->container, zone->edge.bottom, 999);

   zone->handlers = 
     evas_list_append(zone->handlers,
                      ecore_event_handler_add(ECORE_X_EVENT_MOUSE_IN,
                                              _e_zone_cb_mouse_in, zone));
   zone->handlers = 
     evas_list_append(zone->handlers,
                      ecore_event_handler_add(ECORE_X_EVENT_MOUSE_OUT,
                                              _e_zone_cb_mouse_out, zone));
   zone->handlers = 
     evas_list_append(zone->handlers,
                      ecore_event_handler_add(ECORE_X_EVENT_MOUSE_MOVE,
                                              _e_zone_cb_mouse_move, zone));
   zone->handlers = 
     evas_list_append(zone->handlers,
                      ecore_event_handler_add(E_EVENT_DESK_SHOW,
                                              _e_zone_cb_desk_show, zone));

   snprintf(name, sizeof(name), "Zone %d", zone->num);
   zone->name = eina_stringshare_add(name);

   con->zones = evas_list_append(con->zones, zone);

   o = evas_object_rectangle_add(con->bg_evas);
   zone->bg_clip_object = o;
   evas_object_move(o, x, y);
   evas_object_resize(o, w, h);
   evas_object_color_set(o, 255, 255, 255, 255);
   evas_object_show(o);

   o = evas_object_rectangle_add(con->bg_evas);
   zone->bg_event_object = o;
   evas_object_clip_set(o, zone->bg_clip_object);
   evas_object_move(o, x, y);
   evas_object_resize(o, w, h);
   evas_object_color_set(o, 0, 0, 0, 0);
   evas_object_repeat_events_set(o, 1);
   evas_object_show(o);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _e_zone_cb_bg_mouse_down, zone);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_UP,   _e_zone_cb_bg_mouse_up, zone);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_MOVE, _e_zone_cb_bg_mouse_move, zone);

   /* TODO: config the ecore_evas type. */
   zone->black_ecore_evas = 
     e_canvas_new(e_config->evas_engine_zone, zone->container->win,
                  0, 0, zone->w, zone->h, 1, 1, &(zone->black_win), NULL);
   e_canvas_add(zone->black_ecore_evas);
   ecore_evas_layer_set(zone->black_ecore_evas, 6);
   zone->black_evas = ecore_evas_get(zone->black_ecore_evas);

   o = evas_object_rectangle_add(zone->black_evas);
   evas_object_move(o, 0, 0);
   evas_object_resize(o, zone->w, zone->h);
   evas_object_color_set(o, 0, 0, 0, 255);
   evas_object_show(o);

   ecore_evas_name_class_set(zone->black_ecore_evas, "E", "Black_Window");
   snprintf(name, sizeof(name), "Enlightenment Black Zone (%d)", zone->num);
   ecore_evas_title_set(zone->black_ecore_evas, name);

   zone->desk_x_count = 0;
   zone->desk_y_count = 0;
   zone->desk_x_current = 0;
   zone->desk_y_current = 0;
   e_zone_desk_count_set(zone, e_config->zone_desks_x_count,
			 e_config->zone_desks_y_count);

   _e_zone_update_flip(zone);
   _e_zone_update_edge(zone);

   e_object_del_attach_func_set(E_OBJECT(zone), _e_zone_object_del_attach);

   ev = E_NEW(E_Event_Zone_Add, 1);
   ev->zone = zone;
   e_object_ref(E_OBJECT(ev->zone));
   ecore_event_add(E_EVENT_ZONE_ADD, ev, _e_zone_event_add_free, NULL);

   return zone;
}

EAPI void
e_zone_name_set(E_Zone *zone, const char *name)
{
   E_OBJECT_CHECK(zone);
   E_OBJECT_TYPE_CHECK(zone, E_ZONE_TYPE);

   if (zone->name) eina_stringshare_del(zone->name);
   zone->name = eina_stringshare_add(name);
}

EAPI void
e_zone_move(E_Zone *zone, int x, int y)
{
   E_Event_Zone_Move_Resize *ev;

   E_OBJECT_CHECK(zone);
   E_OBJECT_TYPE_CHECK(zone, E_ZONE_TYPE);

   if ((x == zone->x) && (y == zone->y)) return;
   zone->x = x;
   zone->y = y;
   evas_object_move(zone->bg_object, x, y);
   evas_object_move(zone->bg_event_object, x, y);
   evas_object_move(zone->bg_clip_object, x, y);

   ev = E_NEW(E_Event_Zone_Move_Resize, 1);
   ev->zone = zone;
   e_object_ref(E_OBJECT(ev->zone));
   ecore_event_add(E_EVENT_ZONE_MOVE_RESIZE, ev, _e_zone_event_move_resize_free, NULL);

   ecore_x_window_move_resize(zone->edge.left, zone->x, zone->y, 1, zone->h);
   ecore_x_window_move_resize(zone->edge.right, zone->x + zone->w - 1, zone->y, 1, zone->h);
   ecore_x_window_move_resize(zone->edge.top, zone->x + 1, zone->y, zone->w - 2, 1);
   ecore_x_window_move_resize(zone->edge.bottom, zone->x + 1, zone->y + zone->h - 1, zone->w - 2, 1);
}

EAPI void
e_zone_resize(E_Zone *zone, int w, int h)
{
   E_Event_Zone_Move_Resize *ev;

   E_OBJECT_CHECK(zone);
   E_OBJECT_TYPE_CHECK(zone, E_ZONE_TYPE);

   if ((w == zone->w) && (h == zone->h)) return;
   zone->w = w;
   zone->h = h;
   evas_object_resize(zone->bg_object, w, h);
   evas_object_resize(zone->bg_event_object, w, h);
   evas_object_resize(zone->bg_clip_object, w, h);

   ev = E_NEW(E_Event_Zone_Move_Resize, 1);
   ev->zone = zone;
   e_object_ref(E_OBJECT(ev->zone));
   ecore_event_add(E_EVENT_ZONE_MOVE_RESIZE, ev, 
                   _e_zone_event_move_resize_free, NULL);

   ecore_x_window_move_resize(zone->edge.left, zone->x, zone->y, 1, zone->h);
   ecore_x_window_move_resize(zone->edge.right, zone->x + zone->w - 1, 
                              zone->y, 1, zone->h);
   ecore_x_window_move_resize(zone->edge.top, zone->x + 1, zone->y, 
                              zone->w - 2, 1);
   ecore_x_window_move_resize(zone->edge.bottom, zone->x + 1, 
                              zone->y + zone->h - 1, zone->w - 2, 1);
}

EAPI void
e_zone_move_resize(E_Zone *zone, int x, int y, int w, int h)
{
   E_Event_Zone_Move_Resize *ev;

   E_OBJECT_CHECK(zone);
   E_OBJECT_TYPE_CHECK(zone, E_ZONE_TYPE);

   if ((x == zone->x) && (y == zone->y) && (w == zone->w) && (h == zone->h))
     return;

   zone->x = x;
   zone->y = y;
   zone->w = w;
   zone->h = h;

   evas_object_move(zone->bg_object, x, y);
   evas_object_move(zone->bg_event_object, x, y);
   evas_object_move(zone->bg_clip_object, x, y);
   evas_object_resize(zone->bg_object, w, h);
   evas_object_resize(zone->bg_event_object, w, h);
   evas_object_resize(zone->bg_clip_object, w, h);

   ev = E_NEW(E_Event_Zone_Move_Resize, 1);
   ev->zone = zone;
   e_object_ref(E_OBJECT(ev->zone));
   ecore_event_add(E_EVENT_ZONE_MOVE_RESIZE, ev, 
                   _e_zone_event_move_resize_free, NULL);

   ecore_x_window_move_resize(zone->edge.left, zone->x, zone->y, 1, zone->h);
   ecore_x_window_move_resize(zone->edge.right, zone->x + zone->w - 1, 
                              zone->y, 1, zone->h);
   ecore_x_window_move_resize(zone->edge.top, zone->x + 1, zone->y, 
                              zone->w - 2, 1);
   ecore_x_window_move_resize(zone->edge.bottom, zone->x + 1, 
                              zone->y + zone->h - 1, zone->w - 2, 1);
} 

EAPI void
e_zone_fullscreen_set(E_Zone *zone, int on)
{
   E_OBJECT_CHECK(zone);
   E_OBJECT_TYPE_CHECK(zone, E_ZONE_TYPE);

   if ((!zone->fullscreen) && (on))
     {
	ecore_evas_show(zone->black_ecore_evas);
	e_container_window_raise(zone->container, zone->black_win, 150);
	zone->fullscreen = 1;
     }
   else if ((zone->fullscreen) && (!on))
     {
	ecore_evas_hide(zone->black_ecore_evas);
	zone->fullscreen = 0;
     }
}

EAPI E_Zone *
e_zone_current_get(E_Container *con)
{
   Evas_List *l = NULL;

   E_OBJECT_CHECK_RETURN(con, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(con, E_CONTAINER_TYPE, NULL);
   if (!starting)
     {
	int x, y;

	ecore_x_pointer_xy_get(con->win, &x, &y);
	for (l = con->zones; l; l = l->next)
	  {
	     E_Zone *zone;

	     zone = l->data;
	     if (E_INSIDE(x, y, zone->x, zone->y, zone->w, zone->h))
	       return zone;
	  }
     }
   if (!con->zones) return NULL;
   l = con->zones;
   return (E_Zone *)l->data;
}

EAPI void
e_zone_bg_reconfigure(E_Zone *zone)
{
   E_OBJECT_CHECK(zone);
   E_OBJECT_TYPE_CHECK(zone, E_ZONE_TYPE);

   e_bg_zone_update(zone, E_BG_TRANSITION_CHANGE);
}

EAPI void
e_zone_flip_coords_handle(E_Zone *zone, int x, int y)
{
   int one_row = 1;
   int one_col = 1;

   E_OBJECT_CHECK(zone);
   E_OBJECT_TYPE_CHECK(zone, E_ZONE_TYPE);

   if (!e_config->edge_flip_dragging) return;
   /* if we have only 1 row we can flip up/down even if we have xinerama */
   if (evas_list_count(zone->container->zones) > 1)
     {
	Evas_List *zones;
	E_Zone *next_zone;
	int cx, cy;

	zones = zone->container->zones;
	next_zone = (E_Zone *)evas_list_data(zones);
	cx = next_zone->x;
	cy = next_zone->y;
	zones = evas_list_next(zones);
	while (zones)
	  {
	     next_zone = (E_Zone *)zones->data;
	     if (next_zone->x != cx) one_col = 0;
	     if (next_zone->y != cy) one_row = 0;
	     zones = zones->next;
	  }
     }
   if (evas_list_count(zone->container->manager->containers) > 1)
     goto noflip;
   if (!E_INSIDE(x, y, zone->x, zone->y, zone->w, zone->h))
     goto noflip;
   if ((one_row) && (y == 0) && E_ZONE_FLIP_UP(zone))
     {
	/* top */
	if (!zone->flip.timer)
	  zone->flip.timer = ecore_timer_add(e_config->edge_flip_timeout, 
                                             _e_zone_cb_timer, zone);
	zone->flip.direction = E_DIRECTION_UP;
     }
   else if ((one_col) && (x == (zone->w - 1)) && E_ZONE_FLIP_RIGHT(zone))
     {
	/* right */
	if (!zone->flip.timer)
	  zone->flip.timer = ecore_timer_add(e_config->edge_flip_timeout, 
                                             _e_zone_cb_timer, zone);
	zone->flip.direction = E_DIRECTION_RIGHT;
     }
   else if ((one_row) && (y == (zone->h - 1)) && E_ZONE_FLIP_DOWN(zone))
     {
	/* bottom */
	if (!zone->flip.timer)
	  zone->flip.timer = ecore_timer_add(e_config->edge_flip_timeout, 
                                             _e_zone_cb_timer, zone);
	zone->flip.direction = E_DIRECTION_DOWN;
     }
   else if ((one_col) && (x == 0) && E_ZONE_FLIP_LEFT(zone))
     {
	/* left */
	if (!zone->flip.timer)
	  zone->flip.timer = ecore_timer_add(e_config->edge_flip_timeout, 
                                             _e_zone_cb_timer, zone);
	zone->flip.direction = E_DIRECTION_LEFT;
     }
   else
     {
	noflip:
	/* in zone */
	if (zone->flip.timer) ecore_timer_del(zone->flip.timer);
	zone->flip.timer = NULL;
     }
}

EAPI void
e_zone_desk_count_set(E_Zone *zone, int x_count, int y_count)
{
   E_Desk **new_desks;
   E_Desk *desk, *new_desk;
   E_Border  *bd;
   E_Event_Zone_Desk_Count_Set *ev;
   E_Border_List *bl;
   int x, y, xx, yy, moved, nx, ny;

   E_OBJECT_CHECK(zone);
   E_OBJECT_TYPE_CHECK(zone, E_ZONE_TYPE);

   xx = x_count;
   if (xx < 1) xx = 1;
   yy = y_count;
   if (yy < 1) yy = 1;

   /* Orphaned window catcher; in case desk count gets reset */
   moved = 0;
   if (zone->desk_x_current >= xx) moved = 1;
   if (zone->desk_y_current >= yy) moved = 1;
   if (moved)
     {
	nx = zone->desk_x_current;
	ny = zone->desk_y_current;
	if (zone->desk_x_current >= xx) nx = xx - 1;
	if (zone->desk_y_current >= yy) ny = yy - 1;
	e_desk_show(e_desk_at_xy_get(zone, nx, ny));
     }

   new_desks = malloc(xx * yy * sizeof(E_Desk *));
   for (x = 0; x < xx; x++)
     {
	for (y = 0; y < yy; y++)
	  {
	     if ((x < zone->desk_x_count) && (y < zone->desk_y_count))
	       desk = zone->desks[x + (y * zone->desk_x_count)];
	     else
	       desk = e_desk_new(zone, x, y);
	     new_desks[x + (y * xx)] = desk;
	  }
     }

   /* catch windoes that have fallen off the end if we got smaller */
   if (xx < zone->desk_x_count)
     {
	for (y = 0; y < zone->desk_y_count; y++)
	  {
	     new_desk = zone->desks[xx - 1 + (y * zone->desk_x_count)];
	     for (x = xx; x < zone->desk_x_count; x++)
	       {
		  desk = zone->desks[x + (y * zone->desk_x_count)];

		  bl = e_container_border_list_first(zone->container);
		  while ((bd = e_container_border_list_next(bl)))
		    {
		       if (bd->desk == desk)
			 e_border_desk_set(bd, new_desk);
		    }
		  e_container_border_list_free(bl);
		  e_object_del(E_OBJECT(desk));
	       }
	  }
     }
   if (yy < zone->desk_y_count)
     {
	for (x = 0; x < zone->desk_x_count; x++)
	  {
	     new_desk = zone->desks[x + ((yy - 1) * zone->desk_x_count)];
	     for (y = yy; y < zone->desk_y_count; y++)
	       {
		  desk = zone->desks[x + (y * zone->desk_x_count)];

		  bl = e_container_border_list_first(zone->container);
		  while ((bd = e_container_border_list_next(bl)))
		    {
		       if (bd->desk == desk)
			 e_border_desk_set(bd, new_desk);
		    }
		  e_container_border_list_free(bl);
		  e_object_del(E_OBJECT(desk));
	       }
	  }
     }
   if (zone->desks) free(zone->desks);
   zone->desks = new_desks;

   zone->desk_x_count = xx;
   zone->desk_y_count = yy;
   e_config->zone_desks_x_count = xx;
   e_config->zone_desks_y_count = yy;
   e_config_save_queue();

   /* Cannot call desk_current_get until the zone desk counts have been set 
    * or else we end up with a "white background" because desk_current_get will
    * return NULL.
    */
   desk = e_desk_current_get(zone);
   if (desk)
     {
	desk->visible = 0;
	e_desk_show(desk);
     }

   ev = E_NEW(E_Event_Zone_Desk_Count_Set, 1);
   if (!ev) return;
   ev->zone = zone;
   e_object_ref(E_OBJECT(ev->zone));
   ecore_event_add(E_EVENT_ZONE_DESK_COUNT_SET, ev, 
                   _e_zone_event_zone_desk_count_set_free, NULL);
}

EAPI void
e_zone_desk_count_get(E_Zone *zone, int *x_count, int *y_count)
{
   E_OBJECT_CHECK(zone);
   E_OBJECT_TYPE_CHECK(zone, E_ZONE_TYPE);

   if (x_count) *x_count = zone->desk_x_count;
   if (y_count) *y_count = zone->desk_y_count;
}

EAPI void
e_zone_update_flip(E_Zone *zone)
{
   E_OBJECT_CHECK(zone);
   E_OBJECT_TYPE_CHECK(zone, E_ZONE_TYPE);

   _e_zone_update_flip(zone);
   _e_zone_update_edge(zone);
}

EAPI void
e_zone_update_flip_all(void)
{
   Evas_List *l, *ll;
   E_Manager *man;
   E_Container *con;
   E_Zone *zone;

   for (l = e_manager_list(); l; l = l->next)
     {
	man = l->data;
	for (ll = man->containers; ll; ll = ll->next)
	  {
	     con = ll->data;
	     zone = e_zone_current_get(con);
	     e_zone_update_flip(zone);
	  }
     }
}

EAPI void
e_zone_desk_flip_by(E_Zone *zone, int dx, int dy)
{
   E_OBJECT_CHECK(zone);
   E_OBJECT_TYPE_CHECK(zone, E_ZONE_TYPE);

   dx = zone->desk_x_current + dx;
   dy = zone->desk_y_current + dy;
   e_zone_desk_flip_to(zone, dx, dy);
}

EAPI void
e_zone_desk_flip_to(E_Zone *zone, int x, int y)
{
   E_Desk *desk;

   E_OBJECT_CHECK(zone);
   E_OBJECT_TYPE_CHECK(zone, E_ZONE_TYPE);

   if (e_config->desk_flip_wrap)
     {
	x = x % zone->desk_x_count;
	y = y % zone->desk_y_count;
	if (x < 0) x += zone->desk_x_count;
	if (y < 0) y += zone->desk_y_count;
     }
   else
     {
	if (x < 0) x = 0;
	else if (x >= zone->desk_x_count) x = zone->desk_x_count - 1;
	if (y < 0) y = 0;
	else if (y >= zone->desk_y_count) y = zone->desk_y_count - 1;
     }
   desk = e_desk_at_xy_get(zone, x, y);
   if (desk) e_desk_show(desk);
}

EAPI void
e_zone_desk_linear_flip_by(E_Zone *zone, int dx)
{
   E_OBJECT_CHECK(zone);
   E_OBJECT_TYPE_CHECK(zone, E_ZONE_TYPE);

   dx = zone->desk_x_current + 
     (zone->desk_y_current * zone->desk_x_count) + dx;
   dx = dx % (zone->desk_x_count * zone->desk_y_count);
   while (dx < 0)
     dx += (zone->desk_x_count * zone->desk_y_count);
   e_zone_desk_linear_flip_to(zone, dx);
}

EAPI void
e_zone_desk_linear_flip_to(E_Zone *zone, int x)
{
   int y;

   E_OBJECT_CHECK(zone);
   E_OBJECT_TYPE_CHECK(zone, E_ZONE_TYPE);

   y = x / zone->desk_x_count;
   x = x - (y * zone->desk_x_count);
   e_zone_desk_flip_to(zone, x, y);
}

EAPI void
e_zone_flip_win_disable(void)
{
   Evas_List *l, *ll, *lll;
   E_Manager *man;
   E_Container *con;

   for (l = e_manager_list(); l; l = l->next)
     {
	man = l->data;
	for (ll = man->containers; ll; ll = ll->next)
	  {
	     con = ll->data;
	     for (lll = con->zones; lll; lll = lll->next)
	       {
		  E_Zone *zone;

		  zone = lll->data;
		  ecore_x_window_hide(zone->edge.left);
		  ecore_x_window_hide(zone->edge.right);
		  ecore_x_window_hide(zone->edge.top);
		  ecore_x_window_hide(zone->edge.bottom);
	       }
	  }
     }
}

EAPI void
e_zone_flip_win_restore(void)
{
   Evas_List *l, *ll, *lll;
   E_Manager *man;
   E_Container *con;

   for (l = e_manager_list(); l; l = l->next)
     {
	man = l->data;
	for (ll = man->containers; ll; ll = ll->next)
	  {
	     con = ll->data;
	     for (lll = con->zones; lll; lll = lll->next)
	       {
		  E_Zone *zone;

		  zone = lll->data;
		  _e_zone_update_edge(zone);
	       }
	  }
     }
}

EAPI void
e_zone_edge_event_register(E_Zone *zone, E_Zone_Edge edge, int reg)
{
   switch (edge)
     {
      case E_ZONE_EDGE_LEFT:
	 if (reg) zone->show.left++;
	 else zone->show.left--;
	 break;
      case E_ZONE_EDGE_RIGHT:
	 if (reg) zone->show.right++;
	 else zone->show.right--;
	 break;
      case E_ZONE_EDGE_TOP:
	 if (reg) zone->show.top++;
	 else zone->show.top--;
	 break;
      case E_ZONE_EDGE_BOTTOM:
	 if (reg) zone->show.bottom++;
	 else zone->show.bottom--;
	 break;
     }
   _e_zone_update_edge(zone);
}

/* local subsystem functions */
static void
_e_zone_free(E_Zone *zone)
{
   E_Container *con;
   Evas_List *l;
   int x, y;

   /* Delete the edge windows if they exist */
   if (zone->edge.top) ecore_x_window_del(zone->edge.top);
   if (zone->edge.bottom) ecore_x_window_del(zone->edge.bottom);
   if (zone->edge.left) ecore_x_window_del(zone->edge.left);
   if (zone->edge.right) ecore_x_window_del(zone->edge.right);

   /* Delete the object event callbacks */
   evas_object_event_callback_del(zone->bg_event_object, 
				  EVAS_CALLBACK_MOUSE_DOWN, 
				  _e_zone_cb_bg_mouse_down);
   evas_object_event_callback_del(zone->bg_event_object, 
				  EVAS_CALLBACK_MOUSE_UP, 
				  _e_zone_cb_bg_mouse_up);
   evas_object_event_callback_del(zone->bg_event_object, 
				  EVAS_CALLBACK_MOUSE_MOVE, 
				  _e_zone_cb_bg_mouse_move);

   if (zone->black_ecore_evas)
     {
	e_canvas_del(zone->black_ecore_evas);
	ecore_evas_free(zone->black_ecore_evas);
     }
   if (zone->cur_mouse_action)
     {
	e_object_unref(E_OBJECT(zone->cur_mouse_action));
	zone->cur_mouse_action = NULL;
     }
   
   /* remove handlers */
   for (l = zone->handlers; l; l = l->next)
     {
	Ecore_Event_Handler *h;

	h = l->data;
	ecore_event_handler_del(h);
     }
   evas_list_free(zone->handlers);
   zone->handlers = NULL;

   con = zone->container;
   if (zone->name) eina_stringshare_del(zone->name);
   con->zones = evas_list_remove(con->zones, zone);
   evas_object_del(zone->bg_event_object);
   evas_object_del(zone->bg_clip_object);
   evas_object_del(zone->bg_object);
   if (zone->prev_bg_object) evas_object_del(zone->prev_bg_object);
   if (zone->transition_object) evas_object_del(zone->transition_object);

   /* free desks */
   for (x = 0; x < zone->desk_x_count; x++)
     {
	for (y = 0; y < zone->desk_y_count; y++)
	  e_object_del(E_OBJECT(zone->desks[x + (y * zone->desk_x_count)]));
     }
   free(zone->desks);
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

   if (!zone->cur_mouse_action)
     {
	if (ecore_event_current_type_get() == ECORE_X_EVENT_MOUSE_BUTTON_DOWN)
	  {
	     Ecore_X_Event_Mouse_Button_Down *ev2;

	     ev2 = ecore_event_current_event_get();
	     zone->cur_mouse_action =
	       e_bindings_mouse_down_event_handle(E_BINDING_CONTEXT_ZONE,
						  E_OBJECT(zone), ev2);
	     if (zone->cur_mouse_action)
	       {
		  if ((!zone->cur_mouse_action->func.end_mouse) &&
		      (!zone->cur_mouse_action->func.end))
		    zone->cur_mouse_action = NULL;
		  if (zone->cur_mouse_action)
		    e_object_ref(E_OBJECT(zone->cur_mouse_action));
	       }
	  }
     }
}

static void
_e_zone_cb_bg_mouse_up(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
   E_Zone *zone;
   Evas_Event_Mouse_Up *ev;

   ev = (Evas_Event_Mouse_Up *)event_info;      
   zone = data;
   if (zone->cur_mouse_action)
     {
	if (ecore_event_current_type_get() == ECORE_X_EVENT_MOUSE_BUTTON_UP)
	  {
	     Ecore_X_Event_Mouse_Button_Up *ev2;

	     ev2 = ecore_event_current_event_get();
	     if (zone->cur_mouse_action->func.end_mouse)
	       zone->cur_mouse_action->func.end_mouse(E_OBJECT(zone), "", ev2);
	     else if (zone->cur_mouse_action->func.end)
	       zone->cur_mouse_action->func.end(E_OBJECT(zone), "");
	  }
	e_object_unref(E_OBJECT(zone->cur_mouse_action));
	zone->cur_mouse_action = NULL;
     }
   else
     {
	if (ecore_event_current_type_get() == ECORE_X_EVENT_MOUSE_BUTTON_UP)
	  {
	     Ecore_X_Event_Mouse_Button_Up *ev2;

	     ev2 = ecore_event_current_event_get();
	     e_bindings_mouse_up_event_handle(E_BINDING_CONTEXT_ZONE,
					      E_OBJECT(zone), ev2);
	  }
     }
}

static void
_e_zone_cb_bg_mouse_move(void *data, Evas *evas, Evas_Object *obj, void *event_info)
{
   E_Zone *zone;
   Evas_Event_Mouse_Move *ev;

   ev = (Evas_Event_Mouse_Move *)event_info;   
   zone = data;
}

static void
_e_zone_event_zone_desk_count_set_free(void *data, void *ev)
{
   E_Event_Zone_Desk_Count_Set *e;

   e = ev;
   e_object_unref(E_OBJECT(e->zone));
   free(e);
}

static int
_e_zone_cb_mouse_in(void *data, int type, void *event)
{
   Ecore_X_Event_Mouse_In *ev;
   E_Event_Zone_Edge *zev;
   E_Zone *zone;

   ev = event;
   zone = data;

   if (ev->win == zone->edge.top)
     {
	if (zone->flip.top)
	  {
	     if (!zone->flip.timer)
	       zone->flip.timer = ecore_timer_add(e_config->edge_flip_timeout, _e_zone_cb_timer, zone);
	     zone->flip.direction = E_DIRECTION_UP;
	  }

	zev = E_NEW(E_Event_Zone_Edge, 1);
	zev->zone = zone;
	zev->x = ev->x;
	zev->y = ev->y;
	zev->edge = E_ZONE_EDGE_TOP;
	ecore_event_add(E_EVENT_ZONE_EDGE_IN, zev, NULL, NULL);
     }
   else if (ev->win == zone->edge.right)
     {
	if (zone->flip.right)
	  {
	     if (!zone->flip.timer)
	       zone->flip.timer = ecore_timer_add(e_config->edge_flip_timeout, 
                                                  _e_zone_cb_timer, zone);
	     zone->flip.direction = E_DIRECTION_RIGHT;
	  }

	zev = E_NEW(E_Event_Zone_Edge, 1);
	zev->zone = zone;
	zev->x = ev->x;
	zev->y = ev->y;
	zev->edge = E_ZONE_EDGE_RIGHT;
	ecore_event_add(E_EVENT_ZONE_EDGE_IN, zev, NULL, NULL);
     }
   else if (ev->win == zone->edge.bottom)
     {
	if (zone->flip.bottom)
	  {
	     if (!zone->flip.timer)
	       zone->flip.timer = ecore_timer_add(e_config->edge_flip_timeout, 
                                                  _e_zone_cb_timer, zone);
	     zone->flip.direction = E_DIRECTION_DOWN;
	  }

	zev = E_NEW(E_Event_Zone_Edge, 1);
	zev->zone = zone;
	zev->x = ev->x;
	zev->y = ev->y;
	zev->edge = E_ZONE_EDGE_BOTTOM;
	ecore_event_add(E_EVENT_ZONE_EDGE_IN, zev, NULL, NULL);
     }
   else if (ev->win == zone->edge.left)
     {
	if (zone->flip.left)
	  {
	     if (!zone->flip.timer)
	       zone->flip.timer = ecore_timer_add(e_config->edge_flip_timeout, 
                                                  _e_zone_cb_timer, zone);
	     zone->flip.direction = E_DIRECTION_LEFT;
	  }

	zev = E_NEW(E_Event_Zone_Edge, 1);
	zev->zone = zone;
	zev->x = ev->x;
	zev->y = ev->y;
	zev->edge = E_ZONE_EDGE_LEFT;
	ecore_event_add(E_EVENT_ZONE_EDGE_IN, zev, NULL, NULL);
     }
   return 1;
}

static int
_e_zone_cb_mouse_out(void *data, int type, void *event)
{
   Ecore_X_Event_Mouse_Out *ev;
   E_Event_Zone_Edge *zev;
   E_Zone *zone;

   ev = event;
   zone = data;

   if ((ev->win == zone->edge.top) ||
       (ev->win == zone->edge.bottom) ||
       (ev->win == zone->edge.left) ||
       (ev->win == zone->edge.right))
     {
	if (zone->flip.timer)
	  ecore_timer_del(zone->flip.timer);
	zone->flip.timer = NULL;
	zev = E_NEW(E_Event_Zone_Edge, 1);
	zev->zone = zone;
	zev->x = ev->x;
	zev->y = ev->y;
	if (ev->win == zone->edge.top) zev->edge = E_ZONE_EDGE_TOP;
	else if (ev->win == zone->edge.bottom) zev->edge = E_ZONE_EDGE_BOTTOM;
	else if (ev->win == zone->edge.left) zev->edge = E_ZONE_EDGE_LEFT;
	else if (ev->win == zone->edge.right) zev->edge = E_ZONE_EDGE_RIGHT;
	ecore_event_add(E_EVENT_ZONE_EDGE_OUT, zev, NULL, NULL);
     }
   return 1;
}

static int
_e_zone_cb_mouse_move(void *data, int type, void *event)
{
   Ecore_X_Event_Mouse_Move *ev;
   E_Event_Zone_Edge *zev;
   E_Zone *zone;

   ev = event;
   zone = data;

   if ((ev->win == zone->edge.top) ||
       (ev->win == zone->edge.bottom) ||
       (ev->win == zone->edge.left) ||
       (ev->win == zone->edge.right))
     {
	zev = E_NEW(E_Event_Zone_Edge, 1);
	zev->zone = zone;
	zev->x = ev->x;
	zev->y = ev->y;
	if (ev->win == zone->edge.top) zev->edge = E_ZONE_EDGE_TOP;
	else if (ev->win == zone->edge.bottom) zev->edge = E_ZONE_EDGE_BOTTOM;
	else if (ev->win == zone->edge.left) zev->edge = E_ZONE_EDGE_LEFT;
	else if (ev->win == zone->edge.right) zev->edge = E_ZONE_EDGE_RIGHT;
	ecore_event_add(E_EVENT_ZONE_EDGE_MOVE, zev, NULL, NULL);
     }
   return 1;
}

static int
_e_zone_cb_timer(void *data)
{
   E_Zone *zone;
   E_Desk *prev = NULL, *current = NULL;
   E_Event_Pointer_Warp *ev;
   int x, y;

   ev = E_NEW(E_Event_Pointer_Warp, 1);
   if (!ev) return 0;

   zone = data;
   if (zone != e_zone_current_get(zone->container))
     {
	zone->flip.timer = NULL;
	return 0;
     }

   ecore_x_pointer_xy_get(zone->container->win, &x, &y);
   ev->prev.x = x;
   ev->prev.y = y;
   prev = e_desk_at_xy_get(zone, zone->desk_x_current, zone->desk_y_current);

   switch (zone->flip.direction)
     {
      case E_DIRECTION_UP:
	if (E_ZONE_FLIP_UP(zone))
	   {
	      e_zone_desk_flip_by(zone, 0, -1);
	      ecore_x_pointer_warp(zone->container->win, x, zone->h - 2);
	      ev->curr.x = x;
	      ev->curr.y = zone->h - 2;
	   }
	break;
      case E_DIRECTION_RIGHT:
	if (E_ZONE_FLIP_RIGHT(zone))
	  {
	     e_zone_desk_flip_by(zone, 1, 0);
	     ecore_x_pointer_warp(zone->container->win, 2, y);
	     ev->curr.y = y;
	     ev->curr.x = 2;
	  }
	break;
      case E_DIRECTION_DOWN:
	if (E_ZONE_FLIP_DOWN(zone))
	  {
	     e_zone_desk_flip_by(zone, 0, 1);
	     ecore_x_pointer_warp(zone->container->win, x, 2);
	     ev->curr.x = x;
	     ev->curr.y = 2;
	  }
	break;
      case E_DIRECTION_LEFT:
	if (E_ZONE_FLIP_LEFT(zone))
	  {
	     e_zone_desk_flip_by(zone, -1, 0);
	     ecore_x_pointer_warp(zone->container->win, zone->w - 2, y);
	     ev->curr.y = y;
	     ev->curr.x = zone->w - 2;
	  }
	break;
     }

   zone->flip.timer = NULL;

   current = e_desk_current_get(zone);
   if (current)
     ecore_event_add(E_EVENT_POINTER_WARP, ev, NULL, NULL);
   else
     free(ev);

   return 0;
}

static int
_e_zone_cb_desk_show(void *data, int type, void *event)
{
   E_Event_Desk_Show *ev;
   E_Zone *zone;

   ev = event;
   zone = data;
   if (ev->desk->zone != zone) return 1;

   _e_zone_update_flip(zone);
   _e_zone_update_edge(zone);
   return 1;
}

static void
_e_zone_update_flip(E_Zone *zone)
{
   if (zone->flip.left) zone->show.left--;
   zone->flip.left = 0;
   if (zone->flip.right) zone->show.right--;
   zone->flip.right = 0;
   if (zone->flip.top) zone->show.top--;
   zone->flip.top = 0;
   if (zone->flip.bottom) zone->show.bottom--;
   zone->flip.bottom = 0;

   if (e_config->edge_flip_moving)
     {
	/* if we have only 1 row we can flip up/down even if we have xinerama */
	int one_row = 1;
	int one_col = 1;

	if (evas_list_count(zone->container->zones) > 1)
	  {
	     Evas_List *zones;
	     E_Zone *next_zone;
	     int x, y;

	     zones = zone->container->zones;
	     next_zone = (E_Zone *)evas_list_data(zones);
	     x = next_zone->x;
	     y = next_zone->y;
	     zones = evas_list_next(zones);
	     while (zones)
	       {
		  next_zone = (E_Zone *)zones->data;
		  if (next_zone->x != x) one_col = 0;
		  if (next_zone->y != y) one_row = 0;
		  zones = zones->next;
	       }
	  }
	if (evas_list_count(zone->container->manager->containers) > 1)
	  {
	     one_col = 0;
	     one_row = 0;
	  }

	if (one_col && E_ZONE_FLIP_LEFT(zone))
	  {
	     zone->flip.left = 1;
	     zone->show.left++;
	  }

	if (one_col && E_ZONE_FLIP_RIGHT(zone))
	  {
	     zone->flip.right = 1;
	     zone->show.right++;
	  }

	if (one_row && E_ZONE_FLIP_UP(zone))
	  {
	     zone->flip.top = 1;
	     zone->show.top++;
	  }

	if (one_row && E_ZONE_FLIP_DOWN(zone))
	  {
	     zone->flip.bottom = 1;
	     zone->show.bottom++;
	  }
     }
}

static void
_e_zone_update_edge(E_Zone *zone)
{
   if (zone->show.left) ecore_x_window_show(zone->edge.left);
   else ecore_x_window_hide(zone->edge.left);

   if (zone->show.right) ecore_x_window_show(zone->edge.right);
   else ecore_x_window_hide(zone->edge.right);

   if (zone->show.top) ecore_x_window_show(zone->edge.top);
   else ecore_x_window_hide(zone->edge.top);

   if (zone->show.bottom) ecore_x_window_show(zone->edge.bottom);
   else ecore_x_window_hide(zone->edge.bottom);
}

static void
_e_zone_event_move_resize_free(void *data, void *ev)
{
   E_Event_Zone_Move_Resize *e;

   e = ev;
   e_object_unref(E_OBJECT(e->zone));
   free(e);
}

static void
_e_zone_event_add_free(void *data, void *ev)
{
   E_Event_Zone_Add *e;

   e = ev;
   e_object_unref(E_OBJECT(e->zone));
   free(e);
}

static void
_e_zone_event_del_free(void *data, void *ev)
{
   E_Event_Zone_Del *e;

   e = ev;
   e_object_unref(E_OBJECT(e->zone));
   free(e);
}

static void
_e_zone_object_del_attach(void *o)
{
   E_Zone *zone;
   E_Event_Zone_Del *ev;

   if (e_object_is_del(E_OBJECT(o))) return;
   zone = o;
   ev = E_NEW(E_Event_Zone_Del, 1);
   ev->zone = zone;
   e_object_ref(E_OBJECT(ev->zone));
   ecore_event_add(E_EVENT_ZONE_DEL, ev, _e_zone_event_del_free, NULL);
}
