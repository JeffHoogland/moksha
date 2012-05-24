#include "e.h"
#include "e_mod_main.h"

typedef struct
{
   E_Zone         *zone;
   Ecore_X_Window  win;
   Ecore_Timer    *timer;
   int             x, y, dx, dy;
   unsigned int    dt;
   Eina_Bool       down : 1;
} Cover;

static Eina_List *covers = NULL;
static Eina_List *handlers = NULL;

static Ecore_X_Window
_mouse_win_in_get(Cover *cov, int x, int y)
{
   Eina_List *l;
   Ecore_X_Window *skip, inwin;
   Cover *cov2;
   int i;
   
   skip = alloca(sizeof(Ecore_X_Window) * eina_list_count(covers));
   i = 0;
   EINA_LIST_FOREACH(covers, l, cov2)
     {
        skip[i] = cov2->win;
        i++;
     }
   inwin = ecore_x_window_shadow_tree_at_xy_with_skip_get
     (cov->zone->container->manager->root, x, y, skip, i);
   return inwin;
}

static void
_mouse_win_fake_tap(Cover *cov, Ecore_Event_Mouse_Button *ev)
{
   Ecore_X_Window inwin;
   int x, y;
   
   inwin = _mouse_win_in_get(cov, ev->root.x, ev->root.y);
   ecore_x_pointer_xy_get(inwin, &x, &y);
   ecore_x_mouse_in_send(inwin, x, y);
   ecore_x_mouse_move_send(inwin, x, y);
   ecore_x_mouse_down_send(inwin, x, y, 1);
   ecore_x_mouse_up_send(inwin, x, y, 1);
   ecore_x_mouse_out_send(inwin, x, y);
}

static Eina_Bool
_mouse_longpress(void *data)
{
   Cover *cov = data;
   int distance = 40;
   int dx, dy;
   
   cov->timer = NULL;
   dx = cov->x - cov->dx;
   dy = cov->y - cov->dy;
   if (((dx * dx) + (dy * dy)) < (distance * distance))
     {
        E_Border *bd = e_border_focused_get();
        
        cov->down = EINA_FALSE;
        printf("longpress\n");
        if (bd)
          ecore_x_e_illume_access_action_read_send(bd->client.win);
     }
   return EINA_FALSE;
}

static void
_mouse_down(Cover *cov, Ecore_Event_Mouse_Button *ev)
{
   double longtime = 0.5;
   
   cov->dx = ev->x;
   cov->dy = ev->y;
   cov->x = ev->x;
   cov->y = ev->y;
   cov->dt = ev->timestamp;
   cov->down = EINA_TRUE;
   cov->timer = ecore_timer_add(longtime, _mouse_longpress, cov);
}

static void
_mouse_up(Cover *cov, Ecore_Event_Mouse_Button *ev)
{
   double timeout = 0.15;
   int distance = 40;
   int dx, dy;
   E_Border *bd = e_border_focused_get();
   
   if (cov->timer)
     {
        ecore_timer_del(cov->timer);
        cov->timer = NULL;
     }
   if (!cov->down) return;
   dx = ev->x - cov->dx;
   dy = ev->y - cov->dy;
   if (((dx * dx) + (dy * dy)) < (distance * distance))
     {
        if ((ev->timestamp - cov->dt) > (timeout * 1000))
          {
             printf("tap\n");
             _mouse_win_fake_tap(cov, ev);
          }
        else if (ev->double_click)
          {
             printf("double click\n");
             if (bd)
               ecore_x_e_illume_access_action_activate_send(bd->client.win);
          }
     }
   else
     {
        if (abs(dx) > abs(dy)) // left or right
          {
             if (dx > 0) // right
               {
                  printf("right\n");
                  if (bd)
                    ecore_x_e_illume_access_action_read_next_send(bd->client.win);
               }
             else // left
               {
                  printf("left\n");
                  if (bd)
                    ecore_x_e_illume_access_action_read_prev_send(bd->client.win);
               }
          }
        else // up or down
          {
             if (dy > 0) // down
               {
                  printf("down\n");
                  if (bd)
                    ecore_x_e_illume_access_action_next_send(bd->client.win);
               }
             else // up
               {
                  printf("up\n");
                  if (bd)
                    ecore_x_e_illume_access_action_prev_send(bd->client.win);
               }
          }
     }
   cov->down = EINA_FALSE;
}

static void
_mouse_move(Cover *cov, Ecore_Event_Mouse_Move *ev)
{
   if (!cov->down) return;
   cov->x = ev->x;
   cov->y = ev->y;
}

static void
_mouse_wheel(Cover *cov __UNUSED__, Ecore_Event_Mouse_Wheel *ev __UNUSED__)
{
   // XXX: fake wheel?
}

static Eina_Bool
_cb_mouse_down(void    *data __UNUSED__,
               int      type __UNUSED__,
               void    *event)
{
   Ecore_Event_Mouse_Button *ev = event;
   Eina_List *l;
   Cover *cov;
   
   EINA_LIST_FOREACH(covers, l, cov)
     {
        if (ev->window == cov->win)
          {
             if ((ev->buttons == 1) && (ev->multi.device == 0))
               _mouse_down(cov, ev);
             return ECORE_CALLBACK_PASS_ON;
          }
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_cb_mouse_up(void    *data __UNUSED__,
             int      type __UNUSED__,
             void    *event)
{
   Ecore_Event_Mouse_Button *ev = event;
   Eina_List *l;
   Cover *cov;
   
   EINA_LIST_FOREACH(covers, l, cov)
     {
        if (ev->window == cov->win)
          {
             if ((ev->buttons == 1) && (ev->multi.device == 0))
               _mouse_up(cov, ev);
             return ECORE_CALLBACK_PASS_ON;
          }
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_cb_mouse_move(void    *data __UNUSED__,
               int      type __UNUSED__,
               void    *event)
{
   Ecore_Event_Mouse_Move *ev = event;
   Eina_List *l;
   Cover *cov;
   
   EINA_LIST_FOREACH(covers, l, cov)
     {
        if (ev->window == cov->win)
          {
             if (ev->multi.device == 0)
               _mouse_move(cov, ev);
             return ECORE_CALLBACK_PASS_ON;
          }
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_cb_mouse_wheel(void    *data __UNUSED__,
                int      type __UNUSED__,
                void    *event)
{
   Ecore_Event_Mouse_Wheel *ev = event;
   Eina_List *l;
   Cover *cov;
   
   EINA_LIST_FOREACH(covers, l, cov)
     {
        if (ev->window == cov->win)
          {
             _mouse_wheel(cov, ev);
             return ECORE_CALLBACK_PASS_ON;
          }
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Cover *
_cover_new(E_Zone *zone)
{
   Cover *cov;
   
   cov = E_NEW(Cover, 1);
   if (!cov) return NULL;
   cov->zone = zone;
   cov->win = ecore_x_window_input_new(zone->container->manager->root,
                                       zone->container->x + zone->x,
                                       zone->container->y + zone->y,
                                       zone->w, zone->h);
   ecore_x_window_ignore_set(cov->win, 1);
   ecore_x_window_configure(cov->win,
                            ECORE_X_WINDOW_CONFIGURE_MASK_SIBLING |
                            ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE,
                            0, 0, 0, 0, 0,
                            zone->container->layers[8].win,
                            ECORE_X_WINDOW_STACK_ABOVE);
   ecore_x_window_show(cov->win);
   return cov;
}

static void
_covers_init(void)
{
   Eina_List *l, *l2, *l3;
   E_Manager *man;

   EINA_LIST_FOREACH(e_manager_list(), l, man)
     {
        E_Container *con;
     
        EINA_LIST_FOREACH(man->containers, l2, con)
          {
             E_Zone *zone;
             
             EINA_LIST_FOREACH(con->zones, l3, zone)
               {
                  Cover *cov = _cover_new(zone);
                  if (cov) covers = eina_list_append(covers, cov);
               }
          }
     }
}

static void
_covers_shutdown(void)
{
   Cover *cov;
   
   EINA_LIST_FREE(covers, cov)
     {
        ecore_x_window_ignore_set(cov->win, 0);
        ecore_x_window_free(cov->win);
        if (cov->timer)
          {
             ecore_timer_del(cov->timer);
             cov->timer = NULL;
          }
        free(cov);
     }
}

static Eina_Bool
_cb_zone_add(void    *data __UNUSED__,
             int      type __UNUSED__,
             void    *event __UNUSED__)
{
   _covers_shutdown();
   _covers_init();
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_cb_zone_del(void    *data __UNUSED__,
             int      type __UNUSED__,
             void    *event __UNUSED__)
{
   _covers_shutdown();
   _covers_init();
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_cb_zone_move_resize(void    *data __UNUSED__,
                     int      type __UNUSED__,
                     void    *event __UNUSED__)
{
   _covers_shutdown();
   _covers_init();
   return ECORE_CALLBACK_PASS_ON;
}

static void
_events_init(void)
{
   handlers = eina_list_append
     (handlers, ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_DOWN,
                                        _cb_mouse_down, NULL));
   handlers = eina_list_append
     (handlers, ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_UP,
                                        _cb_mouse_up, NULL));
   handlers = eina_list_append
     (handlers, ecore_event_handler_add(ECORE_EVENT_MOUSE_MOVE,
                                        _cb_mouse_move, NULL));
   handlers = eina_list_append
     (handlers, ecore_event_handler_add(ECORE_EVENT_MOUSE_WHEEL,
                                        _cb_mouse_wheel, NULL));
   handlers = eina_list_append
     (handlers, ecore_event_handler_add(E_EVENT_ZONE_ADD,
                                        _cb_zone_add, NULL));
   handlers = eina_list_append
     (handlers, ecore_event_handler_add(E_EVENT_ZONE_DEL,
                                        _cb_zone_del, NULL));
   handlers = eina_list_append
     (handlers, ecore_event_handler_add(E_EVENT_ZONE_MOVE_RESIZE,
                                        _cb_zone_move_resize, NULL));
}

static void
_events_shutdown(void)
{
   E_FREE_LIST(handlers, ecore_event_handler_del);
}

/***************************************************************************/
/* module setup */
EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION, "Access"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   _events_init();
   _covers_init();
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   _covers_shutdown();
   _events_shutdown();
   return 1;
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   return 1;
}
