#include "e.h"

/* E_Zone is a child object of E_Container. There is one zone per screen
 * in a xinerama setup. Each zone has one or more desktops.
 */

static void _e_zone_free(E_Zone *zone);
static void _e_zone_cb_bg_mouse_down(void        *data,
                                     Evas        *evas,
                                     Evas_Object *obj,
                                     void        *event_info);
static void _e_zone_cb_bg_mouse_up(void        *data,
                                   Evas        *evas,
                                   Evas_Object *obj,
                                   void        *event_info);
static void      _e_zone_event_zone_desk_count_set_free(void *data,
                                                        void *ev);
static Eina_Bool _e_zone_cb_mouse_in(void *data,
                                     int   type,
                                     void *event);
static Eina_Bool _e_zone_cb_mouse_out(void *data,
                                      int   type,
                                      void *event);
static Eina_Bool _e_zone_cb_mouse_down(void *data,
                                       int   type,
                                       void *event);
static Eina_Bool _e_zone_cb_mouse_up(void *data,
                                     int   type,
                                     void *event);
static Eina_Bool _e_zone_cb_mouse_move(void *data,
                                       int   type,
                                       void *event);
static Eina_Bool _e_zone_cb_desk_after_show(void *data,
                                            int   type,
                                            void *event);
static Eina_Bool   _e_zone_cb_edge_timer(void *data);
static void        _e_zone_event_move_resize_free(void *data,
                                                  void *ev);
static void        _e_zone_event_add_free(void *data,
                                          void *ev);
static void        _e_zone_event_del_free(void *data,
                                          void *ev);
static void        _e_zone_object_del_attach(void *o);
static E_Zone_Edge _e_zone_detect_edge(E_Zone        *zone,
                                       Ecore_X_Window win);
static void        _e_zone_edge_move_resize(E_Zone *zone);

EAPI int E_EVENT_ZONE_DESK_COUNT_SET = 0;
EAPI int E_EVENT_POINTER_WARP = 0;
EAPI int E_EVENT_ZONE_MOVE_RESIZE = 0;
EAPI int E_EVENT_ZONE_ADD = 0;
EAPI int E_EVENT_ZONE_DEL = 0;
EAPI int E_EVENT_ZONE_EDGE_IN = 0;
EAPI int E_EVENT_ZONE_EDGE_OUT = 0;
EAPI int E_EVENT_ZONE_EDGE_MOVE = 0;

#define E_ZONE_FLIP_LEFT(zone)  (((e_config->desk_flip_wrap && ((zone)->desk_x_count > 1)) || ((zone)->desk_x_current > 0)) && (zone)->edge.left)
#define E_ZONE_FLIP_RIGHT(zone) (((e_config->desk_flip_wrap && ((zone)->desk_x_count > 1)) || (((zone)->desk_x_current + 1) < (zone)->desk_x_count)) && (zone)->edge.right)
#define E_ZONE_FLIP_UP(zone)    (((e_config->desk_flip_wrap && ((zone)->desk_y_count > 1)) || ((zone)->desk_y_current > 0)) && (zone)->edge.top)
#define E_ZONE_FLIP_DOWN(zone)  (((e_config->desk_flip_wrap && ((zone)->desk_y_count > 1)) || (((zone)->desk_y_current + 1) < (zone)->desk_y_count)) && (zone)->edge.bottom)

#define E_ZONE_CORNER_RATIO 0.025;

EINTERN int
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

EINTERN int
e_zone_shutdown(void)
{
   return 1;
}

static void
_e_zone_black_new(E_Zone *zone)
{
   Evas_Object *o;
   char name[256];

   if (zone->black_ecore_evas) return;
   zone->black_ecore_evas =
     e_canvas_new(e_config->evas_engine_zone, zone->container->win,
                  zone->x, zone->y, 1, 1, 1, 1, &(zone->black_win));
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
}

static void
_e_zone_black_free(E_Zone *zone)
{
   if (!zone->black_ecore_evas) return;
   e_canvas_del(zone->black_ecore_evas);
   ecore_evas_free(zone->black_ecore_evas);
   zone->black_ecore_evas = NULL;
   zone->black_win = 0;
}

static void
_e_zone_black_get(E_Zone *zone)
{
   zone->black_need++;
   if (!zone->black_ecore_evas) return;
   if (zone->black_need == 1)
     {
        ecore_evas_move(zone->black_ecore_evas, zone->x, zone->y);
        ecore_evas_resize(zone->black_ecore_evas, zone->w, zone->h);
     }
}

static void
_e_zone_black_unget(E_Zone *zone)
{
   zone->black_need--;
   if (!zone->black_ecore_evas) return;
   if (zone->black_need == 0)
     {
        ecore_evas_move(zone->black_ecore_evas, zone->x, zone->y);
        ecore_evas_resize(zone->black_ecore_evas, 1, 1);
     }
}

EAPI E_Zone *
e_zone_new(E_Container *con,
           int          num,
           int          id,
           int          x,
           int          y,
           int          w,
           int          h)
{
   E_Zone *zone;
   Evas_Object *o;
   E_Event_Zone_Add *ev;
   char name[40];

   zone = E_OBJECT_ALLOC(E_Zone, E_ZONE_TYPE, _e_zone_free);
   if (!zone) return NULL;

   zone->container = con;

   zone->x = x;
   zone->y = y;
   zone->w = w;
   zone->h = h;
   zone->num = num;
   zone->id = id;
   e_zone_useful_geometry_dirty(zone);

   zone->handlers =
     eina_list_append(zone->handlers,
                      ecore_event_handler_add(ECORE_X_EVENT_MOUSE_IN,
                                              _e_zone_cb_mouse_in, zone));
   zone->handlers =
     eina_list_append(zone->handlers,
                      ecore_event_handler_add(ECORE_X_EVENT_MOUSE_OUT,
                                              _e_zone_cb_mouse_out, zone));
   zone->handlers =
     eina_list_append(zone->handlers,
                      ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_DOWN,
                                              _e_zone_cb_mouse_down, zone));
   zone->handlers =
     eina_list_append(zone->handlers,
                      ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_UP,
                                              _e_zone_cb_mouse_up, zone));
   zone->handlers =
     eina_list_append(zone->handlers,
                      ecore_event_handler_add(ECORE_EVENT_MOUSE_MOVE,
                                              _e_zone_cb_mouse_move, zone));
   zone->handlers =
     eina_list_append(zone->handlers,
                      ecore_event_handler_add(E_EVENT_DESK_AFTER_SHOW,
                                              _e_zone_cb_desk_after_show, zone));

   snprintf(name, sizeof(name), "Zone %d", zone->num);
   zone->name = eina_stringshare_add(name);

   con->zones = eina_list_append(con->zones, zone);

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
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_UP, _e_zone_cb_bg_mouse_up, zone);

   /* TODO: config the ecore_evas type. */

   zone->desk_x_count = 0;
   zone->desk_y_count = 0;
   zone->desk_x_current = 0;
   zone->desk_y_current = 0;
   e_zone_desk_count_set(zone, e_config->zone_desks_x_count,
                         e_config->zone_desks_y_count);

   e_object_del_attach_func_set(E_OBJECT(zone), _e_zone_object_del_attach);

   _e_zone_black_new(zone);

   ev = E_NEW(E_Event_Zone_Add, 1);
   ev->zone = zone;
   e_object_ref(E_OBJECT(ev->zone));
   ecore_event_add(E_EVENT_ZONE_ADD, ev, _e_zone_event_add_free, NULL);

   return zone;
}

EAPI void
e_zone_name_set(E_Zone     *zone,
                const char *name)
{
   E_OBJECT_CHECK(zone);
   E_OBJECT_TYPE_CHECK(zone, E_ZONE_TYPE);

   if (zone->name) eina_stringshare_del(zone->name);
   zone->name = eina_stringshare_add(name);
}

EAPI void
e_zone_move(E_Zone *zone,
            int     x,
            int     y)
{
   E_Event_Zone_Move_Resize *ev;

   E_OBJECT_CHECK(zone);
   E_OBJECT_TYPE_CHECK(zone, E_ZONE_TYPE);

   if ((x == zone->x) && (y == zone->y)) return;
   zone->x = x;
   zone->y = y;
   evas_object_move(zone->bg_object, x, y);
   if (zone->bg_scrollframe)
     evas_object_move(zone->bg_scrollframe, x, y);
   evas_object_move(zone->bg_event_object, x, y);
   evas_object_move(zone->bg_clip_object, x, y);

   ev = E_NEW(E_Event_Zone_Move_Resize, 1);
   ev->zone = zone;
   e_object_ref(E_OBJECT(ev->zone));
   ecore_event_add(E_EVENT_ZONE_MOVE_RESIZE, ev, _e_zone_event_move_resize_free, NULL);

   _e_zone_edge_move_resize(zone);
   e_zone_bg_reconfigure(zone);
   if (zone->black_need > 0)
     {
        ecore_evas_move(zone->black_ecore_evas, zone->x, zone->y);
        ecore_evas_resize(zone->black_ecore_evas, zone->w, zone->h);
     }
}

EAPI void
e_zone_resize(E_Zone *zone,
              int     w,
              int     h)
{
   E_Event_Zone_Move_Resize *ev;

   E_OBJECT_CHECK(zone);
   E_OBJECT_TYPE_CHECK(zone, E_ZONE_TYPE);

   if ((w == zone->w) && (h == zone->h)) return;
   zone->w = w;
   zone->h = h;
   evas_object_resize(zone->bg_object, w, h);
   if (zone->bg_scrollframe)
     evas_object_resize(zone->bg_scrollframe, w, h);
   evas_object_resize(zone->bg_event_object, w, h);
   evas_object_resize(zone->bg_clip_object, w, h);

   ev = E_NEW(E_Event_Zone_Move_Resize, 1);
   ev->zone = zone;
   e_object_ref(E_OBJECT(ev->zone));
   ecore_event_add(E_EVENT_ZONE_MOVE_RESIZE, ev,
                   _e_zone_event_move_resize_free, NULL);

   _e_zone_edge_move_resize(zone);
   e_zone_bg_reconfigure(zone);
   if (zone->black_need > 0)
     {
        ecore_evas_move(zone->black_ecore_evas, zone->x, zone->y);
        ecore_evas_resize(zone->black_ecore_evas, zone->w, zone->h);
     }
}

EAPI void
e_zone_move_resize(E_Zone *zone,
                   int     x,
                   int     y,
                   int     w,
                   int     h)
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
   if (zone->bg_scrollframe)
     evas_object_move(zone->bg_scrollframe, x, y);
   evas_object_move(zone->bg_event_object, x, y);
   evas_object_move(zone->bg_clip_object, x, y);
   evas_object_resize(zone->bg_object, w, h);
   if (zone->bg_scrollframe)
     evas_object_resize(zone->bg_scrollframe, w, h);
   evas_object_resize(zone->bg_event_object, w, h);
   evas_object_resize(zone->bg_clip_object, w, h);

   ev = E_NEW(E_Event_Zone_Move_Resize, 1);
   ev->zone = zone;
   e_object_ref(E_OBJECT(ev->zone));
   ecore_event_add(E_EVENT_ZONE_MOVE_RESIZE, ev,
                   _e_zone_event_move_resize_free, NULL);

   _e_zone_edge_move_resize(zone);

   e_zone_bg_reconfigure(zone);
   if (zone->black_need > 0)
     {
        ecore_evas_move(zone->black_ecore_evas, zone->x, zone->y);
        ecore_evas_resize(zone->black_ecore_evas, zone->w, zone->h);
     }
}

EAPI void
e_zone_fullscreen_set(E_Zone *zone,
                      int     on)
{
   E_OBJECT_CHECK(zone);
   E_OBJECT_TYPE_CHECK(zone, E_ZONE_TYPE);

   if ((!zone->fullscreen) && (on))
     {
        _e_zone_black_get(zone);
        ecore_evas_show(zone->black_ecore_evas);
        e_container_window_raise(zone->container, zone->black_win, 150);
        zone->fullscreen = 1;
     }
   else if ((zone->fullscreen) && (!on))
     {
        ecore_evas_hide(zone->black_ecore_evas);
        zone->fullscreen = 0;
        _e_zone_black_unget(zone);
     }
}

EAPI E_Zone *
e_zone_current_get(E_Container *con)
{
   Eina_List *l = NULL;
   E_Zone *zone;

   E_OBJECT_CHECK_RETURN(con, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(con, E_CONTAINER_TYPE, NULL);
   if (!starting)
     {
        int x, y;

        ecore_x_pointer_xy_get(con->win, &x, &y);
        EINA_LIST_FOREACH(con->zones, l, zone)
          {
             if (E_INSIDE(x, y, zone->x, zone->y, zone->w, zone->h))
               return zone;
          }
     }
   if (!con->zones) return NULL;
   return (E_Zone *)eina_list_data_get(con->zones);
}

EAPI void
e_zone_bg_reconfigure(E_Zone *zone)
{
   E_OBJECT_CHECK(zone);
   E_OBJECT_TYPE_CHECK(zone, E_ZONE_TYPE);

   e_bg_zone_update(zone, E_BG_TRANSITION_CHANGE);
}

EAPI void
e_zone_flip_coords_handle(E_Zone *zone,
                          int     x,
                          int     y)
{
   E_Event_Zone_Edge *zev;
   E_Binding_Edge *bind;
   E_Zone_Edge edge;
   Eina_List *l;
   E_Shelf *es;
   int ok = 0;
   int one_row = 1;
   int one_col = 1;

   E_OBJECT_CHECK(zone);
   E_OBJECT_TYPE_CHECK(zone, E_ZONE_TYPE);

   if (!e_config->edge_flip_dragging || zone->flip.switching) return;
   /* if we have only 1 row we can flip up/down even if we have xinerama */
   if (eina_list_count(zone->container->zones) > 1)
     {
        Eina_List *zones;
        E_Zone *next_zone;
        int cx, cy;

        zones = zone->container->zones;
        next_zone = (E_Zone *)eina_list_data_get(zones);
        cx = next_zone->x;
        cy = next_zone->y;
        zones = eina_list_next(zones);
        EINA_LIST_FOREACH(eina_list_next(zones), zones, next_zone)
          {
             if (next_zone->x != cx) one_col = 0;
             if (next_zone->y != cy) one_row = 0;
          }
     }
   if (eina_list_count(zone->container->manager->containers) > 1)
     goto noflip;
   if (!E_INSIDE(x, y, zone->x, zone->y, zone->w, zone->h))
     goto noflip;
   if ((one_row) && (y == 0))
     edge = E_ZONE_EDGE_TOP;
   else if ((one_col) && (x == (zone->w - 1)))
     edge = E_ZONE_EDGE_RIGHT;
   else if ((one_row) && (y == (zone->h - 1)))
     edge = E_ZONE_EDGE_BOTTOM;
   else if ((one_col) && (x == 0))
     edge = E_ZONE_EDGE_LEFT;
   else
     {
noflip:

        if (zone->flip.es)
          e_shelf_toggle(zone->flip.es, 0);
        zone->flip.es = NULL;
        return;
     }
   EINA_LIST_FOREACH(e_shelf_list(), l, es)
     {
        if (es->zone != zone) continue;
        switch(es->gadcon->orient)
          {
           case E_GADCON_ORIENT_TOP:
           case E_GADCON_ORIENT_CORNER_TL:
           case E_GADCON_ORIENT_CORNER_TR:
             if (edge == E_ZONE_EDGE_TOP) ok = 1;
             break;

           case E_GADCON_ORIENT_BOTTOM:
           case E_GADCON_ORIENT_CORNER_BL:
           case E_GADCON_ORIENT_CORNER_BR:
             if (edge == E_ZONE_EDGE_BOTTOM) ok = 1;
             break;

           case E_GADCON_ORIENT_LEFT:
           case E_GADCON_ORIENT_CORNER_LT:
           case E_GADCON_ORIENT_CORNER_LB:
             if (edge == E_ZONE_EDGE_LEFT) ok = 1;
             break;

           case E_GADCON_ORIENT_RIGHT:
           case E_GADCON_ORIENT_CORNER_RT:
           case E_GADCON_ORIENT_CORNER_RB:
             if (edge == E_ZONE_EDGE_RIGHT) ok = 1;
             break;

           default:
             ok = 0;
             break;
          }

        if (!ok) continue;
        if (!E_INSIDE(x, y, es->x, es->y, es->w, es->h))
          continue;

        if (zone->flip.es)
          e_shelf_toggle(zone->flip.es, 0);

        zone->flip.es = es;
        e_shelf_toggle(es, 1);
     }
   switch(edge)
     {
      case E_ZONE_EDGE_LEFT:
        if (E_ZONE_FLIP_LEFT(zone)) ok = 1;
        break;

      case E_ZONE_EDGE_TOP:
        if (E_ZONE_FLIP_UP(zone)) ok = 1;
        break;

      case E_ZONE_EDGE_RIGHT:
        if (E_ZONE_FLIP_RIGHT(zone)) ok = 1;
        break;

      case E_ZONE_EDGE_BOTTOM:
        if (E_ZONE_FLIP_DOWN(zone)) ok = 1;
        break;

      default:
        ok = 0;
        break;
     }
   if (!ok) return;
   bind = e_bindings_edge_get("desk_flip_in_direction", edge, 0);
   if (bind)
     {
        zev = E_NEW(E_Event_Zone_Edge, 1);
        zev->zone = zone;
        zev->x = x;
        zev->y = y;
        zev->edge = edge;
        zone->flip.ev = zev;
        zone->flip.bind = bind;
        zone->flip.switching = 1;
        bind->timer = ecore_timer_add(((double)bind->delay), _e_zone_cb_edge_timer, zone);
     }
}

EAPI void
e_zone_desk_count_set(E_Zone *zone,
                      int     x_count,
                      int     y_count)
{
   E_Desk **new_desks;
   E_Desk *desk, *new_desk;
   E_Border *bd;
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
e_zone_desk_count_get(E_Zone *zone,
                      int    *x_count,
                      int    *y_count)
{
   E_OBJECT_CHECK(zone);
   E_OBJECT_TYPE_CHECK(zone, E_ZONE_TYPE);

   if (x_count) *x_count = zone->desk_x_count;
   if (y_count) *y_count = zone->desk_y_count;
}

EAPI void
e_zone_desk_flip_by(E_Zone *zone,
                    int     dx,
                    int     dy)
{
   E_OBJECT_CHECK(zone);
   E_OBJECT_TYPE_CHECK(zone, E_ZONE_TYPE);

   dx = zone->desk_x_current + dx;
   dy = zone->desk_y_current + dy;
   e_zone_desk_flip_to(zone, dx, dy);
}

EAPI void
e_zone_desk_flip_to(E_Zone *zone,
                    int     x,
                    int     y)
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
        else if (x >= zone->desk_x_count)
          x = zone->desk_x_count - 1;
        if (y < 0) y = 0;
        else if (y >= zone->desk_y_count)
          y = zone->desk_y_count - 1;
     }
   desk = e_desk_at_xy_get(zone, x, y);
   if (desk) e_desk_show(desk);
}

EAPI void
e_zone_desk_linear_flip_by(E_Zone *zone,
                           int     dx)
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
e_zone_desk_linear_flip_to(E_Zone *zone,
                           int     x)
{
   int y;

   E_OBJECT_CHECK(zone);
   E_OBJECT_TYPE_CHECK(zone, E_ZONE_TYPE);

   y = x / zone->desk_x_count;
   x = x - (y * zone->desk_x_count);
   e_zone_desk_flip_to(zone, x, y);
}

EAPI void
e_zone_edge_enable(void)
{
   Eina_List *l, *ll, *lll;
   E_Manager *man;
   E_Container *con;
   E_Zone *zone;

   EINA_LIST_FOREACH(e_manager_list(), l, man)
     {
        EINA_LIST_FOREACH(man->containers, ll, con)
          {
             EINA_LIST_FOREACH(con->zones, lll, zone)
               {
                  if (zone->edge.left) ecore_x_window_show(zone->edge.left);
                  if (zone->edge.right) ecore_x_window_show(zone->edge.right);
                  if (zone->edge.top) ecore_x_window_show(zone->edge.top);
                  if (zone->edge.bottom) ecore_x_window_show(zone->edge.bottom);
                  if (zone->corner.left_top) ecore_x_window_show(zone->corner.left_top);
                  if (zone->corner.top_left) ecore_x_window_show(zone->corner.top_left);
                  if (zone->corner.top_right) ecore_x_window_show(zone->corner.top_right);
                  if (zone->corner.right_top) ecore_x_window_show(zone->corner.right_top);
                  if (zone->corner.right_bottom) ecore_x_window_show(zone->corner.right_bottom);
                  if (zone->corner.bottom_right) ecore_x_window_show(zone->corner.bottom_right);
                  if (zone->corner.bottom_left) ecore_x_window_show(zone->corner.bottom_left);
                  if (zone->corner.left_bottom) ecore_x_window_show(zone->corner.left_bottom);
               }
          }
     }
}

EAPI void
e_zone_edge_disable(void)
{
   Eina_List *l, *ll, *lll;
   E_Manager *man;
   E_Container *con;
   E_Zone *zone;

   EINA_LIST_FOREACH(e_manager_list(), l, man)
     {
        EINA_LIST_FOREACH(man->containers, ll, con)
          {
             EINA_LIST_FOREACH(con->zones, lll, zone)
               {
                  if (zone->edge.left) ecore_x_window_hide(zone->edge.left);
                  if (zone->edge.right) ecore_x_window_hide(zone->edge.right);
                  if (zone->edge.top) ecore_x_window_hide(zone->edge.top);
                  if (zone->edge.bottom) ecore_x_window_hide(zone->edge.bottom);
                  if (zone->corner.left_top) ecore_x_window_hide(zone->corner.left_top);
                  if (zone->corner.top_left) ecore_x_window_hide(zone->corner.top_left);
                  if (zone->corner.top_right) ecore_x_window_hide(zone->corner.top_right);
                  if (zone->corner.right_top) ecore_x_window_hide(zone->corner.right_top);
                  if (zone->corner.right_bottom) ecore_x_window_hide(zone->corner.right_bottom);
                  if (zone->corner.bottom_right) ecore_x_window_hide(zone->corner.bottom_right);
                  if (zone->corner.bottom_left) ecore_x_window_hide(zone->corner.bottom_left);
                  if (zone->corner.left_bottom) ecore_x_window_hide(zone->corner.left_bottom);
               }
          }
     }
}

EAPI Eina_Bool
e_zone_exists_direction(E_Zone *zone, E_Zone_Edge edge)
{
   Eina_List *l;
   E_Zone *z2;
   
   EINA_LIST_FOREACH(zone->container->zones, l, z2)
     {
        if (zone == z2) continue;
        
        switch (edge)
          {
           case E_ZONE_EDGE_TOP_LEFT:
             if (((E_SPANS_COMMON(0, zone->x + zone->w, z2->x, z2->w)) &&
                  (z2->y < zone->y)) ||
                 ((E_SPANS_COMMON(0, zone->y + zone->h, z2->y, z2->h)) &&
                  (z2->x < zone->x)))
                return EINA_TRUE;
             break;
           case E_ZONE_EDGE_TOP:
             if ((E_SPANS_COMMON(zone->x, zone->w, z2->x, z2->w)) &&
                 (z2->y < zone->y))
                return EINA_TRUE;
             break;
           case E_ZONE_EDGE_TOP_RIGHT:
             if (((E_SPANS_COMMON(zone->x, 99999, z2->x, z2->w)) &&
                  (z2->y < zone->y)) ||
                 ((E_SPANS_COMMON(0, zone->y + zone->h, z2->y, z2->h)) &&
                  (z2->x >= (zone->x + zone->w))))
                return EINA_TRUE;
             break;
             
           case E_ZONE_EDGE_LEFT:
             if ((E_SPANS_COMMON(zone->y, zone->h, z2->y, z2->h)) &&
                 (z2->x < zone->x))
                return EINA_TRUE;
             break;
             
           case E_ZONE_EDGE_RIGHT:
             if ((E_SPANS_COMMON(zone->y, zone->h, z2->y, z2->h)) &&
                 (z2->x >= (zone->x + zone->w)))
                return EINA_TRUE;
             break;
             
           case E_ZONE_EDGE_BOTTOM_LEFT:
             if (((E_SPANS_COMMON(0, zone->x + zone->w, z2->x, z2->w)) &&
                  (z2->y >= (zone->y + zone->h))) ||
                 ((E_SPANS_COMMON(zone->y, 99999, z2->y, z2->h)) &&
                  (z2->x < zone->x)))
                return EINA_TRUE;
             break;
           case E_ZONE_EDGE_BOTTOM:
             if ((E_SPANS_COMMON(zone->x, zone->w, z2->x, z2->w)) &&
                 (z2->y >= (zone->y + zone->h)))
                return EINA_TRUE;
             break;
           case E_ZONE_EDGE_BOTTOM_RIGHT:
             if (((E_SPANS_COMMON(zone->x, 99999, z2->x, z2->w)) &&
                  (z2->y >= (zone->y + zone->h))) ||
                 ((E_SPANS_COMMON(zone->y, 99999, z2->y, z2->h)) &&
                  (z2->x < zone->x)))
                return EINA_TRUE;
             break;
             
           default:
             break;
          }
     }
      
   return EINA_FALSE;
}

EAPI void
e_zone_edge_new(E_Zone_Edge edge)
{
   Eina_List *l, *ll, *lll;
   E_Manager *man;
   E_Container *con;
   E_Zone *zone;
   int cw, ch;

   // explicitly disallow edge bindings when we have more than 1 root
   // window (ie pure old multihead) since we don't know which direction
   // other root windows are in
   if (eina_list_count(e_manager_list()) > 1) return;
   EINA_LIST_FOREACH(e_manager_list(), l, man)
     {
        EINA_LIST_FOREACH(man->containers, ll, con)
          {
             EINA_LIST_FOREACH(con->zones, lll, zone)
               {
                  // don't allow bindings on edges that are on the boundary
                  // between zones
                  if (e_zone_exists_direction(zone, edge)) continue;
                  cw = zone->w * E_ZONE_CORNER_RATIO;
                  ch = zone->h * E_ZONE_CORNER_RATIO;
                  switch (edge)
                    {
                     case E_ZONE_EDGE_NONE:
                       /* noop */
                       break;

                     case E_ZONE_EDGE_LEFT:
                       if (!zone->edge.left)
                         {
                            zone->edge.left = ecore_x_window_input_new(con->win,
                                                                       zone->x, zone->y + ch, 1,
                                                                       zone->h - 2 * ch);
                            ecore_x_window_show(zone->edge.left);
                         }
                       break;

                     case E_ZONE_EDGE_RIGHT:
                       if (!zone->edge.right)
                         {
                            zone->edge.right = ecore_x_window_input_new(con->win,
                                                                        zone->x + zone->w - 1,
                                                                        zone->y + ch, 1, zone->h - 2 * ch);
                            ecore_x_window_show(zone->edge.right);
                         }
                       break;

                     case E_ZONE_EDGE_TOP:
                       if (!zone->edge.top)
                         {
                            zone->edge.top = ecore_x_window_input_new(con->win,
                                                                      zone->x + 1 + cw, zone->y,
                                                                      zone->w - 2 * cw - 2, 1);
                            ecore_x_window_show(zone->edge.top);
                         }
                       break;

                     case E_ZONE_EDGE_BOTTOM:
                       if (!zone->edge.bottom)
                         {
                            zone->edge.bottom = ecore_x_window_input_new(con->win,
                                                                         zone->x + 1 + cw, zone->y + zone->h - 1,
                                                                         zone->w - 2 - 2 * cw, 1);
                            ecore_x_window_show(zone->edge.bottom);
                         }
                       break;

                     case E_ZONE_EDGE_TOP_LEFT:
                       if (!zone->corner.left_top)
                         {
                            zone->corner.left_top = ecore_x_window_input_new(con->win,
                                                                             zone->x, zone->y, 1, ch);
                            ecore_x_window_show(zone->corner.left_top);
                         }
                       if (!zone->corner.top_left)
                         {
                            zone->corner.top_left = ecore_x_window_input_new(con->win,
                                                                             zone->x + 1, zone->y, cw, 1);
                            ecore_x_window_show(zone->corner.top_left);
                         }
                       break;

                     case E_ZONE_EDGE_TOP_RIGHT:
                       if (!zone->corner.top_right)
                         {
                            zone->corner.top_right = ecore_x_window_input_new(con->win,
                                                                              zone->x + zone->w - cw - 2,
                                                                              zone->y, cw, 1);
                            ecore_x_window_show(zone->corner.top_right);
                         }
                       if (!zone->corner.right_top)
                         {
                            zone->corner.right_top = ecore_x_window_input_new(con->win,
                                                                              zone->x + zone->w - 1,
                                                                              zone->y, 1, ch);
                            ecore_x_window_show(zone->corner.right_top);
                         }
                       break;

                     case E_ZONE_EDGE_BOTTOM_RIGHT:
                       if (!zone->corner.right_bottom)
                         {
                            zone->corner.right_bottom = ecore_x_window_input_new(con->win,
                                                                                 zone->x + zone->w - 1,
                                                                                 zone->y + zone->h - ch, 1, ch);
                            ecore_x_window_show(zone->corner.right_bottom);
                         }
                       if (!zone->corner.bottom_right)
                         {
                            zone->corner.bottom_right = ecore_x_window_input_new(con->win,
                                                                                 zone->x + zone->w - cw - 2,
                                                                                 zone->y + zone->h - 1, cw, 1);

                            ecore_x_window_show(zone->corner.bottom_right);
                         }
                       break;

                     case E_ZONE_EDGE_BOTTOM_LEFT:
                       if (!zone->corner.bottom_left)
                         {
                            zone->corner.bottom_left = ecore_x_window_input_new(con->win,
                                                                                zone->x + 1,
                                                                                zone->y + zone->h - 1, cw, 1);
                            ecore_x_window_show(zone->corner.bottom_left);
                         }
                       if (!zone->corner.left_bottom)
                         {
                            zone->corner.left_bottom = ecore_x_window_input_new(con->win,
                                                                                zone->x, zone->y + zone->h - ch,
                                                                                1, ch);
                            ecore_x_window_show(zone->corner.left_bottom);
                         }
                       break;
                    }
                  if (e_config->fullscreen_flip)
                    e_zone_edge_win_layer_set(zone, 250);
                  else
                    e_zone_edge_win_layer_set(zone, 200);
               }
          }
     }
}

EAPI void
e_zone_edge_free(E_Zone_Edge edge)
{
   Eina_List *l, *ll, *lll;
   E_Manager *man;
   E_Container *con;
   E_Zone *zone;

   EINA_LIST_FOREACH(e_manager_list(), l, man)
     {
        EINA_LIST_FOREACH(man->containers, ll, con)
          {
             EINA_LIST_FOREACH(con->zones, lll, zone)
               {
                  switch (edge)
                    {
                     case E_ZONE_EDGE_NONE:
                       /* noop */
                       break;

                     case E_ZONE_EDGE_LEFT:
                       if (zone->edge.left) ecore_x_window_free(zone->edge.left);
                       zone->edge.left = 0;
                       break;

                     case E_ZONE_EDGE_RIGHT:
                       if (zone->edge.right) ecore_x_window_free(zone->edge.right);
                       zone->edge.right = 0;
                       break;

                     case E_ZONE_EDGE_TOP:
                       if (zone->edge.top) ecore_x_window_free(zone->edge.top);
                       zone->edge.top = 0;
                       break;

                     case E_ZONE_EDGE_BOTTOM:
                       if (zone->edge.bottom) ecore_x_window_free(zone->edge.bottom);
                       zone->edge.bottom = 0;
                       break;

                     case E_ZONE_EDGE_TOP_LEFT:
                       if (zone->corner.left_top) ecore_x_window_free(zone->corner.left_top);
                       if (zone->corner.top_left) ecore_x_window_free(zone->corner.top_left);
                       zone->corner.left_top = 0;
                       zone->corner.top_left = 0;
                       break;

                     case E_ZONE_EDGE_TOP_RIGHT:
                       if (zone->corner.top_right) ecore_x_window_free(zone->corner.top_right);
                       if (zone->corner.right_top) ecore_x_window_free(zone->corner.right_top);
                       zone->corner.top_right = 0;
                       zone->corner.right_top = 0;
                       break;

                     case E_ZONE_EDGE_BOTTOM_RIGHT:
                       if (zone->corner.right_bottom) ecore_x_window_free(zone->corner.right_bottom);
                       if (zone->corner.bottom_right) ecore_x_window_free(zone->corner.bottom_right);
                       zone->corner.right_bottom = 0;
                       zone->corner.bottom_right = 0;
                       break;

                     case E_ZONE_EDGE_BOTTOM_LEFT:
                       if (zone->corner.bottom_left) ecore_x_window_free(zone->corner.bottom_left);
                       if (zone->corner.left_bottom) ecore_x_window_free(zone->corner.left_bottom);
                       zone->corner.bottom_left = 0;
                       zone->corner.left_bottom = 0;
                       break;
                    }
               }
          }
     }
}

EAPI void
e_zone_edge_win_layer_set(E_Zone *zone,
                          int     layer)
{
   if (zone->corner.left_bottom) e_container_window_raise(zone->container, zone->corner.left_bottom, layer);
   if (zone->corner.left_top) e_container_window_raise(zone->container, zone->corner.left_top, layer);
   if (zone->corner.top_left) e_container_window_raise(zone->container, zone->corner.top_left, layer);
   if (zone->corner.top_right) e_container_window_raise(zone->container, zone->corner.top_right, layer);
   if (zone->corner.right_top) e_container_window_raise(zone->container, zone->corner.right_top, layer);
   if (zone->corner.right_bottom) e_container_window_raise(zone->container, zone->corner.right_bottom, layer);
   if (zone->corner.bottom_right) e_container_window_raise(zone->container, zone->corner.bottom_right, layer);
   if (zone->corner.bottom_left) e_container_window_raise(zone->container, zone->corner.bottom_left, layer);

   if (zone->edge.left) e_container_window_raise(zone->container, zone->edge.left, layer);
   if (zone->edge.right) e_container_window_raise(zone->container, zone->edge.right, layer);
   if (zone->edge.top) e_container_window_raise(zone->container, zone->edge.top, layer);
   if (zone->edge.bottom) e_container_window_raise(zone->container, zone->edge.bottom, layer);
}

static void
_e_zone_useful_geometry_calc(E_Zone *zone)
{
   const Eina_List *l;
   const E_Shelf *shelf;
   int x0, x1, y0, y1;

   x0 = 0;
   y0 = 0;
   x1 = zone->w;
   y1 = zone->h;
   EINA_LIST_FOREACH(e_shelf_list(), l, shelf)
     {
        E_Config_Shelf_Desk *sd;
        E_Gadcon_Orient orient;
        Eina_List *ll;
        int skip_shelf = 0;

        if (shelf->zone != zone)
          continue;

        if (shelf->cfg)
          {
             if (shelf->cfg->overlap)
               continue;

             if (shelf->cfg->autohide)
               continue;
             orient = shelf->cfg->orient;

             if (shelf->cfg->desk_show_mode)
               {
                  skip_shelf = 1;
                  EINA_LIST_FOREACH(shelf->cfg->desk_list, ll, sd)
                    {
                       if (!sd) continue;
                       if ((sd->x == zone->desk_x_current) && (sd->y == zone->desk_y_current))
                         {
                            skip_shelf = 0;
                            break;
                         }
                    }
                  if (skip_shelf)
                    continue;
               }
          }
        else
          orient = shelf->gadcon->orient;

        switch (orient)
          {
            /* these are non-edje orientations */
            case E_GADCON_ORIENT_FLOAT:
            case E_GADCON_ORIENT_HORIZ:
            case E_GADCON_ORIENT_VERT:
              break;

            case E_GADCON_ORIENT_TOP:
            case E_GADCON_ORIENT_CORNER_TL:
            case E_GADCON_ORIENT_CORNER_TR:
              if (y0 < shelf->h)
                y0 = shelf->h;
              break;

            case E_GADCON_ORIENT_BOTTOM:
            case E_GADCON_ORIENT_CORNER_BL:
            case E_GADCON_ORIENT_CORNER_BR:
              if (y1 > zone->h - shelf->h)
                y1 = zone->h - shelf->h;
              break;
              break;

            case E_GADCON_ORIENT_LEFT:
            case E_GADCON_ORIENT_CORNER_LT:
            case E_GADCON_ORIENT_CORNER_LB:
              if (x0 < shelf->w)
                x0 = shelf->w;
              break;

            case E_GADCON_ORIENT_RIGHT:
            case E_GADCON_ORIENT_CORNER_RT:
            case E_GADCON_ORIENT_CORNER_RB:
              if (x1 > zone->w - shelf->w)
                x1 = zone->w - shelf->w;
              break;
          }
     }

   zone->useful_geometry.x = zone->x + x0;
   zone->useful_geometry.y = zone->y + y0;
   zone->useful_geometry.w = x1 - x0;
   zone->useful_geometry.h = y1 - y0;
   zone->useful_geometry.dirty = 0;
}

/**
 * Get (or calculate) the useful (or free, without any shelves) area.
 */
EAPI void
e_zone_useful_geometry_get(E_Zone *zone,
                           int    *x,
                           int    *y,
                           int    *w,
                           int    *h)
{
   E_OBJECT_CHECK(zone);
   E_OBJECT_TYPE_CHECK(zone, E_ZONE_TYPE);
   Eina_List *l;
   E_Shelf *shelf;

   if (!zone->useful_geometry.dirty)
     {
        EINA_LIST_FOREACH(e_shelf_list(), l, shelf)
          {
             if (!shelf->cfg) continue;
             if (shelf->cfg->desk_show_mode)
               {
                  _e_zone_useful_geometry_calc(zone);
                  break;
               }
          }
     }
   else
     _e_zone_useful_geometry_calc(zone);

   if (x) *x = zone->useful_geometry.x;
   if (y) *y = zone->useful_geometry.y;
   if (w) *w = zone->useful_geometry.w;
   if (h) *h = zone->useful_geometry.h;
}

/**
 * Mark as dirty so e_zone_useful_geometry_get() will need to recalculate.
 *
 * Call this function when shelves are added or important properties changed.
 */
EAPI void
e_zone_useful_geometry_dirty(E_Zone *zone)
{
   E_Event_Zone_Move_Resize *ev;

   E_OBJECT_CHECK(zone);
   E_OBJECT_TYPE_CHECK(zone, E_ZONE_TYPE);

   ev = E_NEW(E_Event_Zone_Move_Resize, 1);
   ev->zone = zone;
   e_object_ref(E_OBJECT(ev->zone));
   ecore_event_add(E_EVENT_ZONE_MOVE_RESIZE, ev, _e_zone_event_move_resize_free, NULL);

   zone->useful_geometry.dirty = 1;
   zone->useful_geometry.x = -1;
   zone->useful_geometry.y = -1;
   zone->useful_geometry.w = -1;
   zone->useful_geometry.h = -1;
}

/* local subsystem functions */
static void
_e_zone_free(E_Zone *zone)
{
   E_Container *con;
   Ecore_Animator *anim;
   void *data;
   int x, y;

   /* Delete the edge windows if they exist */
   if (zone->edge.top) ecore_x_window_free(zone->edge.top);
   if (zone->edge.bottom) ecore_x_window_free(zone->edge.bottom);
   if (zone->edge.left) ecore_x_window_free(zone->edge.left);
   if (zone->edge.right) ecore_x_window_free(zone->edge.right);
   if (zone->corner.left_bottom) ecore_x_window_free(zone->corner.left_bottom);
   if (zone->corner.left_top) ecore_x_window_free(zone->corner.left_top);
   if (zone->corner.top_left) ecore_x_window_free(zone->corner.top_left);
   if (zone->corner.top_right) ecore_x_window_free(zone->corner.top_right);
   if (zone->corner.right_top) ecore_x_window_free(zone->corner.right_top);
   if (zone->corner.right_bottom) ecore_x_window_free(zone->corner.right_bottom);
   if (zone->corner.bottom_right) ecore_x_window_free(zone->corner.bottom_right);
   if (zone->corner.bottom_left) ecore_x_window_free(zone->corner.bottom_left);

   /* Delete the object event callbacks */
   evas_object_event_callback_del(zone->bg_event_object,
                                  EVAS_CALLBACK_MOUSE_DOWN,
                                  _e_zone_cb_bg_mouse_down);
   evas_object_event_callback_del(zone->bg_event_object,
                                  EVAS_CALLBACK_MOUSE_UP,
                                  _e_zone_cb_bg_mouse_up);

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
   E_FREE_LIST(zone->handlers, ecore_event_handler_del);

   con = zone->container;
   if (zone->name) eina_stringshare_del(zone->name);
   con->zones = eina_list_remove(con->zones, zone);
   anim = evas_object_data_get(zone->bg_object, "switch_animator");
   if (anim) ecore_animator_del(anim);
   data = evas_object_data_get(zone->bg_object, "switch_animator_params");
   if (data) E_FREE(data);
   evas_object_del(zone->bg_event_object);
   evas_object_del(zone->bg_clip_object);
   evas_object_del(zone->bg_object);
   evas_object_del(zone->bg_scrollframe);
   if (zone->prev_bg_object) evas_object_del(zone->prev_bg_object);
   if (zone->transition_object) evas_object_del(zone->transition_object);

   /* free desks */
   for (x = 0; x < zone->desk_x_count; x++)
     {
        for (y = 0; y < zone->desk_y_count; y++)
          e_object_del(E_OBJECT(zone->desks[x + (y * zone->desk_x_count)]));
     }
   free(zone->desks);

   _e_zone_black_free(zone);

   free(zone);
}

static void
_e_zone_cb_bg_mouse_down(void            *data,
                         Evas *evas       __UNUSED__,
                         Evas_Object *obj __UNUSED__,
                         void *event_info __UNUSED__)
{
   E_Zone *zone;

   zone = data;
   if (e_menu_grab_window_get()) return;

   if (!zone->cur_mouse_action)
     {
        if (ecore_event_current_type_get() == ECORE_EVENT_MOUSE_BUTTON_DOWN)
          {
             Ecore_Event_Mouse_Button *ev2;

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
_e_zone_cb_bg_mouse_up(void            *data,
                       Evas *evas       __UNUSED__,
                       Evas_Object *obj __UNUSED__,
                       void *event_info __UNUSED__)
{
   E_Zone *zone;

   zone = data;
   if (zone->cur_mouse_action)
     {
        if (ecore_event_current_type_get() == ECORE_EVENT_MOUSE_BUTTON_UP)
          {
             Ecore_Event_Mouse_Button *ev2;

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
        if (ecore_event_current_type_get() == ECORE_EVENT_MOUSE_BUTTON_UP)
          {
             Ecore_Event_Mouse_Button *ev2;

             ev2 = ecore_event_current_event_get();
             e_bindings_mouse_up_event_handle(E_BINDING_CONTEXT_ZONE,
                                              E_OBJECT(zone), ev2);
          }
     }
}

static void
_e_zone_event_zone_desk_count_set_free(void *data __UNUSED__,
                                       void      *ev)
{
   E_Event_Zone_Desk_Count_Set *e;

   e = ev;
   e_object_unref(E_OBJECT(e->zone));
   free(e);
}

static Eina_Bool
_e_zone_cb_mouse_in(void    *data,
                    int type __UNUSED__,
                    void    *event)
{
   Ecore_X_Event_Mouse_In *ev;
   E_Event_Zone_Edge *zev;
   E_Zone_Edge edge;
   E_Zone *zone;

   ev = event;
   zone = data;

   edge = _e_zone_detect_edge(zone, ev->win);
   if (edge == E_ZONE_EDGE_NONE) return ECORE_CALLBACK_PASS_ON;

   zev = E_NEW(E_Event_Zone_Edge, 1);
   zev->zone = zone;
   zev->edge = edge;
   zev->x = ev->root.x;
   zev->y = ev->root.y;
   zev->modifiers = ev->modifiers;
   ecore_event_add(E_EVENT_ZONE_EDGE_IN, zev, NULL, NULL);
   e_bindings_edge_in_event_handle(E_BINDING_CONTEXT_ZONE, E_OBJECT(zone), zev);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_zone_cb_mouse_out(void    *data,
                     int type __UNUSED__,
                     void    *event)
{
   Ecore_X_Event_Mouse_Out *ev;
   E_Event_Zone_Edge *zev;
   E_Zone_Edge edge;
   E_Zone *zone;

   ev = event;
   zone = data;

   edge = _e_zone_detect_edge(zone, ev->win);
   if (edge == E_ZONE_EDGE_NONE) return ECORE_CALLBACK_PASS_ON;

   zev = E_NEW(E_Event_Zone_Edge, 1);
   zev->zone = zone;
   zev->edge = edge;
   zev->x = ev->root.x;
   zev->y = ev->root.y;
   zev->modifiers = ev->modifiers;
   ecore_event_add(E_EVENT_ZONE_EDGE_OUT, zev, NULL, NULL);
   e_bindings_edge_out_event_handle(E_BINDING_CONTEXT_ZONE, E_OBJECT(zone), zev);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_zone_cb_mouse_down(void    *data,
                      int type __UNUSED__,
                      void    *event)
{
   Ecore_Event_Mouse_Button *ev;
   E_Event_Zone_Edge *zev;
   E_Zone_Edge edge;
   E_Zone *zone;

   ev = event;
   zone = data;

   edge = _e_zone_detect_edge(zone, ev->window);
   if (edge == E_ZONE_EDGE_NONE) return ECORE_CALLBACK_PASS_ON;

   zev = E_NEW(E_Event_Zone_Edge, 1);
   zev->zone = zone;
   zev->edge = edge;
   zev->x = ev->root.x;
   zev->y = ev->root.y;
   zev->modifiers = ev->modifiers;
   ecore_event_add(E_EVENT_ZONE_EDGE_OUT, zev, NULL, NULL);
   e_bindings_edge_down_event_handle(E_BINDING_CONTEXT_ZONE, E_OBJECT(zone), zev);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_zone_cb_mouse_up(void    *data,
                    int type __UNUSED__,
                    void    *event)
{
   Ecore_Event_Mouse_Button *ev;
   E_Event_Zone_Edge *zev;
   E_Zone_Edge edge;
   E_Zone *zone;

   ev = event;
   zone = data;

   edge = _e_zone_detect_edge(zone, ev->window);
   if (edge == E_ZONE_EDGE_NONE) return ECORE_CALLBACK_PASS_ON;

   zev = E_NEW(E_Event_Zone_Edge, 1);
   zev->zone = zone;
   zev->edge = edge;
   zev->x = ev->root.x;
   zev->y = ev->root.y;
   zev->modifiers = ev->modifiers;
   ecore_event_add(E_EVENT_ZONE_EDGE_OUT, zev, NULL, NULL);
   e_bindings_edge_up_event_handle(E_BINDING_CONTEXT_ZONE, E_OBJECT(zone), zev);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_zone_cb_mouse_move(void    *data,
                      int type __UNUSED__,
                      void    *event)
{
   Ecore_Event_Mouse_Move *ev;
   E_Event_Zone_Edge *zev;
   E_Zone_Edge edge;
   E_Zone *zone;

   ev = event;
   zone = data;

   edge = _e_zone_detect_edge(zone, ev->window);
   if (edge == E_ZONE_EDGE_NONE) return ECORE_CALLBACK_PASS_ON;

   zev = E_NEW(E_Event_Zone_Edge, 1);
   zev->zone = zone;
   zev->edge = edge;
   zev->x = ev->root.x;
   zev->y = ev->root.y;
   zev->modifiers = ev->modifiers;
   ecore_event_add(E_EVENT_ZONE_EDGE_MOVE, zev, NULL, NULL);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_zone_cb_desk_after_show(void    *data,
                           int type __UNUSED__,
                           void    *event)
{
   E_Event_Desk_Show *ev;
   E_Zone *zone;

   ev = event;
   zone = data;
   if (ev->desk->zone != zone) return ECORE_CALLBACK_PASS_ON;

   zone->flip.switching = 0;
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_zone_cb_edge_timer(void *data)
{
   E_Zone *zone;
   E_Action *act;

   zone = data;
   act = e_action_find(zone->flip.bind->action);
   if (!act)
     {
        E_FREE(zone->flip.ev);
        return ECORE_CALLBACK_CANCEL;
     }

   if (act->func.go_edge)
     act->func.go_edge(E_OBJECT(zone), zone->flip.bind->params, zone->flip.ev);
   else if (act->func.go)
     act->func.go(E_OBJECT(zone), zone->flip.bind->params);

   zone->flip.bind->timer = NULL;

   E_FREE(zone->flip.ev);
   return ECORE_CALLBACK_RENEW;
}

static void
_e_zone_event_move_resize_free(void *data __UNUSED__,
                               void      *ev)
{
   E_Event_Zone_Move_Resize *e;

   e = ev;
   e_object_unref(E_OBJECT(e->zone));
   free(e);
}

static void
_e_zone_event_add_free(void *data __UNUSED__,
                       void      *ev)
{
   E_Event_Zone_Add *e;

   e = ev;
   e_object_unref(E_OBJECT(e->zone));
   free(e);
}

static void
_e_zone_event_del_free(void *data __UNUSED__,
                       void      *ev)
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

static E_Zone_Edge
_e_zone_detect_edge(E_Zone        *zone,
                    Ecore_X_Window win)
{
   E_Zone_Edge edge = E_ZONE_EDGE_NONE;

   if (win == zone->edge.left)
     edge = E_ZONE_EDGE_LEFT;
   else if (win == zone->edge.top)
     edge = E_ZONE_EDGE_TOP;
   else if (win == zone->edge.right)
     edge = E_ZONE_EDGE_RIGHT;
   else if (win == zone->edge.bottom)
     edge = E_ZONE_EDGE_BOTTOM;
   else if ((win == zone->corner.left_top) ||
            (win == zone->corner.top_left))
     edge = E_ZONE_EDGE_TOP_LEFT;
   else if ((win == zone->corner.right_top) ||
            (win == zone->corner.top_right))
     edge = E_ZONE_EDGE_TOP_RIGHT;
   else if ((win == zone->corner.right_bottom) ||
            (win == zone->corner.bottom_right))
     edge = E_ZONE_EDGE_BOTTOM_RIGHT;
   else if ((win == zone->corner.left_bottom) ||
            (win == zone->corner.bottom_left))
     edge = E_ZONE_EDGE_BOTTOM_LEFT;
   return edge;
}

static void
_e_zone_edge_move_resize(E_Zone *zone)
{
   int cw;
   int ch;

   cw = zone->w * E_ZONE_CORNER_RATIO;
   ch = zone->h * E_ZONE_CORNER_RATIO;

   if (zone->corner.left_bottom)
     ecore_x_window_move_resize(zone->corner.left_bottom,
                                zone->x, zone->y + zone->h - ch, 1, ch);
   if (zone->edge.left)
     ecore_x_window_move_resize(zone->edge.left,
                                zone->x, zone->y + ch, 1, zone->h - 2 * ch);
   if (zone->corner.left_top)
     ecore_x_window_move_resize(zone->corner.left_top,
                                zone->x, zone->y, 1, ch);

   if (zone->corner.top_left)
     ecore_x_window_move_resize(zone->corner.top_left,
                                zone->x + 1, zone->y, cw, 1);
   if (zone->edge.top)
     ecore_x_window_move_resize(zone->edge.top,
                                zone->x + 1 + cw, zone->y, zone->w - 2 * cw - 2, 1);
   if (zone->corner.top_right)
     ecore_x_window_move_resize(zone->corner.top_right,
                                zone->x + zone->w - cw - 2, zone->y, cw, 1);

   if (zone->corner.right_top)
     ecore_x_window_move_resize(zone->corner.right_top,
                                zone->x + zone->w - 1, zone->y, 1, ch);
   if (zone->edge.right)
     ecore_x_window_move_resize(zone->edge.right,
                                zone->x + zone->w - 1, zone->y + ch, 1, zone->h - 2 * ch);
   if (zone->corner.right_bottom)
     ecore_x_window_move_resize(zone->corner.right_bottom,
                                zone->x + zone->w - 1, zone->y + zone->h - ch, 1, ch);

   if (zone->corner.bottom_right)
     ecore_x_window_move_resize(zone->corner.bottom_right,
                                zone->x + 1, zone->y + zone->h - 1, cw, 1);
   if (zone->edge.bottom)
     ecore_x_window_move_resize(zone->edge.bottom,
                                zone->x + 1 + cw, zone->y + zone->h - 1, zone->w - 2 - 2 * cw, 1);
   if (zone->corner.bottom_left)
     ecore_x_window_move_resize(zone->corner.bottom_left,
                                zone->x + zone->w - cw - 2, zone->y + zone->h - 1, cw, 1);
}

