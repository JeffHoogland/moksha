#include "E_Illume.h"
#include "e_mod_main.h"
#include "e_quickpanel.h"

/* local function prototypes */
static void _e_quickpanel_hide(E_Quickpanel *qp);
static void _e_quickpanel_slide(E_Quickpanel *qp, int visible, double len);
static void _e_quickpanel_border_show(E_Border *bd);
static void _e_quickpanel_border_hide(E_Border *bd);
static E_Quickpanel *_e_quickpanel_by_border_get(E_Border *bd);
static void _e_quickpanel_cb_free(E_Quickpanel *qp);
static int _e_quickpanel_cb_delay_hide(void *data);
static int _e_quickpanel_cb_animate(void *data);
static int _e_quickpanel_cb_border_add(void *data, int type, void *event);
static int _e_quickpanel_cb_client_message(void *data, int type, void *event);
static int _e_quickpanel_cb_mouse_down(void *data, int type, void *event);
static void _e_quickpanel_cb_post_fetch(void *data, void *data2);

/* local variables */
static Eina_List *handlers = NULL, *hooks = NULL;
static Ecore_X_Window input_win = 0;

/* public functions */
int 
e_quickpanel_init(void) 
{
   handlers = 
     eina_list_append(handlers, 
                      ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE, 
                                              _e_quickpanel_cb_client_message, 
                                              NULL));
   handlers = 
     eina_list_append(handlers, 
                      ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_DOWN, 
                                              _e_quickpanel_cb_mouse_down, 
                                              NULL));
   handlers = 
     eina_list_append(handlers, 
                      ecore_event_handler_add(E_EVENT_BORDER_ADD, 
                                              _e_quickpanel_cb_border_add, 
                                              NULL));
   hooks = 
     eina_list_append(hooks, 
                      e_border_hook_add(E_BORDER_HOOK_EVAL_PRE_POST_FETCH, 
                                        _e_quickpanel_cb_post_fetch, NULL));
   return 1;
}

int 
e_quickpanel_shutdown(void) 
{
   Ecore_Event_Handler *handler;
   E_Border_Hook *hook;

   EINA_LIST_FREE(handlers, handler)
     ecore_event_handler_del(handler);
   EINA_LIST_FREE(hooks, hook)
     e_border_hook_del(hook);
   return 1;
}

E_Quickpanel *
e_quickpanel_new(E_Zone *zone) 
{
   E_Quickpanel *qp;

   qp = E_OBJECT_ALLOC(E_Quickpanel, E_QUICKPANEL_TYPE, _e_quickpanel_cb_free);
   if (!qp) return NULL;
   qp->zone = zone;
   return qp;
}

void 
e_quickpanel_show(E_Quickpanel *qp) 
{
   if (qp->timer) ecore_timer_del(qp->timer);
   qp->timer = NULL;
   if ((qp->visible) || (!qp->borders)) return;
   e_illume_border_top_shelf_size_get(qp->zone, NULL, &qp->top_height);

   if (!input_win) 
     {
        input_win = ecore_x_window_input_new(qp->zone->container->win, 
                                             qp->zone->x, qp->zone->y, 
                                             qp->zone->w, qp->zone->h);
        ecore_x_window_show(input_win);
        if (!e_grabinput_get(input_win, 1, input_win)) 
          {
             ecore_x_window_free(input_win);
             input_win = 0;
             return;
          }
     }

   if (il_cfg->sliding.quickpanel.duration <= 0) 
     {
        Eina_List *l;
        E_Border *bd;
        int ny = 0;

        ny = qp->top_height;
        if (qp->borders) 
          {
             EINA_LIST_FOREACH(qp->borders, l, bd) 
               {
                  if (!bd->visible) _e_quickpanel_border_show(bd);
                  e_border_fx_offset(bd, 0, ny);
                  ny += bd->h;
               }
          }
        qp->visible = 1;
     }
   else
     _e_quickpanel_slide(qp, 1, 
                         (double)il_cfg->sliding.quickpanel.duration / 1000.0);
}

void 
e_quickpanel_hide(E_Quickpanel *qp) 
{
   if (!qp->visible) return;
   if (!qp->timer)
     qp->timer = ecore_timer_add(0.2, _e_quickpanel_cb_delay_hide, qp);
}

E_Quickpanel *
e_quickpanel_by_zone_get(E_Zone *zone) 
{
   Eina_List *l;
   E_Quickpanel *qp;

   if (!zone) return NULL;
   EINA_LIST_FOREACH(quickpanels, l, qp)
     if (qp->zone == zone) return qp;
   return NULL;
}

void 
e_quickpanel_position_update(E_Quickpanel *qp) 
{
   Eina_List *l;
   E_Border *bd;
   int ty = 0;

   if (!qp) return;
   e_quickpanel_hide(qp);
   e_illume_border_top_shelf_pos_get(qp->zone, NULL, &ty);
   if (qp->borders) 
     {
        EINA_LIST_FOREACH(qp->borders, l, bd) 
          e_border_move(bd, qp->zone->x, ty);
     }
}

/* local functions */
static void 
_e_quickpanel_hide(E_Quickpanel *qp) 
{
   if (!qp) return;
   if (qp->timer) ecore_timer_del(qp->timer);
   qp->timer = NULL;
   if (!qp->visible) return;
   e_illume_border_top_shelf_size_get(qp->zone, NULL, &qp->top_height);
   if (input_win) 
     {
        ecore_x_window_free(input_win);
        e_grabinput_release(input_win, input_win);
        input_win = 0;
     }
   if (il_cfg->sliding.quickpanel.duration <= 0) 
     {
        Eina_List *l;
        E_Border *bd;

        if (qp->borders) 
          {
             EINA_LIST_REVERSE_FOREACH(qp->borders, l, bd) 
               {
                  e_border_fx_offset(bd, 0, 0);
                  if (bd->visible) _e_quickpanel_border_hide(bd);
               }
          }
        qp->visible = 0;
     }
   else
     _e_quickpanel_slide(qp, 0, 
                         (double)il_cfg->sliding.quickpanel.duration / 1000.0);
}

static void 
_e_quickpanel_slide(E_Quickpanel *qp, int visible, double len) 
{
   if (!qp) return;
   qp->start = ecore_loop_time_get();
   qp->len = len;
   qp->adjust_start = qp->adjust;
   qp->adjust_end = 0;
   if (visible) qp->adjust_end = qp->h;
   if (!qp->animator)
     qp->animator = ecore_animator_add(_e_quickpanel_cb_animate, qp);
}

static void 
_e_quickpanel_border_show(E_Border *bd) 
{
   unsigned int visible = 1;

   if (!bd) return;
   e_container_border_lower(bd);
   e_container_shape_show(bd->shape);
   if (!bd->need_reparent) ecore_x_window_show(bd->client.win);
   e_hints_window_visible_set(bd);
   bd->visible = 1;
   bd->changes.visible = 1;
   ecore_x_window_prop_card32_set(bd->client.win, E_ATOM_MAPPED, &visible, 1);
   ecore_x_window_prop_card32_set(bd->client.win, E_ATOM_MANAGED, &visible, 1);
}

static void 
_e_quickpanel_border_hide(E_Border *bd) 
{
   unsigned int visible = 0;

   if (!bd) return;
   e_container_shape_hide(bd->shape);
   e_hints_window_hidden_set(bd);
   bd->visible = 0;
   bd->changes.visible = 1;
   ecore_x_window_prop_card32_set(bd->client.win, E_ATOM_MAPPED, &visible, 1);
}

static E_Quickpanel *
_e_quickpanel_by_border_get(E_Border *bd) 
{
   Eina_List *l, *ll;
   E_Border *b;
   E_Quickpanel *qp;

   if ((!bd) || (!bd->stolen)) return NULL;
   if (!quickpanels) return NULL;
   EINA_LIST_FOREACH(quickpanels, l, qp)
     {
        if (!qp->borders) continue;
        EINA_LIST_FOREACH(qp->borders, ll, b)
          if (b == bd) return qp;
     }
   return NULL;
}

static void 
_e_quickpanel_cb_free(E_Quickpanel *qp) 
{
   E_Border *bd;

   if (!qp) return;
   if (qp->animator) ecore_animator_del(qp->animator);
   qp->animator = NULL;
   if (qp->timer) ecore_timer_del(qp->timer);
   qp->timer = NULL;
   EINA_LIST_FREE(qp->borders, bd)
     bd->stolen = 0;
   E_FREE(qp);
}

static int 
_e_quickpanel_cb_delay_hide(void *data) 
{
   E_Quickpanel *qp;

   if (!(qp = data)) return 0;
   _e_quickpanel_hide(qp);
   return 0;
}

static int 
_e_quickpanel_cb_animate(void *data) 
{
   E_Quickpanel *qp;
   double t, v = 1.0;

   if (!(qp = data)) return 0;
   t = ecore_loop_time_get() - qp->start;
   if (t > qp->len) t = qp->len;
   if (qp->len > 0.0) 
     {
        v = (t / qp->len);
        v = (1.0 - v);
        v = (v * v * v * v);
        v = (1.0 - v);
     }
   else 
     t = qp->len;

   qp->adjust = (qp->adjust_end * v) + (qp->adjust_start * (1.0 - v));
   if (qp->borders) 
     {
        Eina_List *l;
        E_Border *bd;
        int pbh = 0;

        pbh = qp->top_height - qp->h;
        EINA_LIST_FOREACH(qp->borders, l, bd) 
          {
             if (e_object_is_del(E_OBJECT(bd))) continue;
             if (bd->fx.y != (qp->adjust + pbh))
               e_border_fx_offset(bd, 0, (qp->adjust + pbh));
             pbh += bd->h;
             if (!qp->visible) 
               {
                  if (bd->fx.y > 0) 
                    if (!bd->visible) _e_quickpanel_border_show(bd);
               }
             else 
               {
                  if (bd->fx.y <= 10)
                    if (bd->visible) _e_quickpanel_border_hide(bd);
               }
          }
     }
   if (t == qp->len) 
     {
        qp->animator = NULL;
        if (qp->visible) qp->visible = 0;
        else qp->visible = 1;
        return 0;
     }
   return 1;
}

static int 
_e_quickpanel_cb_border_add(void *data, int type, void *event) 
{
   E_Event_Border_Add *ev;
   E_Quickpanel *qp;
   int ty;

   ev = event;
   if (!ev->border->client.illume.quickpanel.quickpanel) return 1;
   if (!(qp = e_quickpanel_by_zone_get(ev->border->zone))) return 1;
   _e_quickpanel_border_hide(ev->border);
   e_illume_border_top_shelf_pos_get(qp->zone, NULL, &ty);
   if ((ev->border->x != qp->zone->x) || (ev->border->y != ty)) 
     e_border_move(ev->border, qp->zone->x, ty);
   if (ev->border->zone != qp->zone)
     e_border_zone_set(ev->border, qp->zone);
   qp->h += ev->border->h;
   qp->borders = eina_list_append(qp->borders, ev->border);
   return 1;
}

static int 
_e_quickpanel_cb_client_message(void *data, int type, void *event) 
{
   Ecore_X_Event_Client_Message *ev;

   ev = event;
   if (ev->message_type == ECORE_X_ATOM_E_ILLUME_QUICKPANEL_STATE) 
     {
        E_Zone *zone;

        if (zone = e_util_zone_window_find(ev->win)) 
          {
             E_Quickpanel *qp;

             if (qp = e_quickpanel_by_zone_get(zone)) 
               {
                  if (ev->data.l[0] == ECORE_X_ATOM_E_ILLUME_QUICKPANEL_OFF)
                    e_quickpanel_hide(qp);
                  else
                    e_quickpanel_show(qp);
               }
          }
     }
   else if (ev->message_type == ECORE_X_ATOM_E_ILLUME_QUICKPANEL_POSITION_UPDATE) 
     {
        E_Border *bd;
        E_Quickpanel *qp;

        if (!(bd = e_border_find_by_client_window(ev->win))) return 1;
        if (!(qp = e_quickpanel_by_zone_get(bd->zone))) return 1;
        e_quickpanel_position_update(qp);
     }
   else if (ev->message_type == ECORE_X_ATOM_E_ILLUME_QUICKPANEL_ZONE_REQUEST) 
     {
        E_Border *bd;
        E_Zone *zone;
        Ecore_X_Window z;
        int ty;

        if (!(bd = e_border_find_by_client_window(ev->data.l[1]))) return 1;
        z = ecore_x_e_illume_quickpanel_zone_get(bd->client.win);
        if (!(zone = e_util_zone_window_find(z))) return 1;
        _e_quickpanel_border_hide(bd);
        e_illume_border_top_shelf_pos_get(zone, NULL, &ty);
        e_border_move(bd, zone->x, ty);
        e_border_zone_set(bd, zone);
     }
   return 1;
}

static int 
_e_quickpanel_cb_mouse_down(void *data, int type, void *event) 
{
   Ecore_Event_Mouse_Button *ev;
   Eina_List *l;
   E_Quickpanel *qp;

   ev = event;
   if (ev->event_window != input_win) return 1;
   if (!quickpanels) return 1;
   EINA_LIST_FOREACH(quickpanels, l, qp)
     if (qp->visible) 
       ecore_x_e_illume_quickpanel_state_send(qp->zone->black_win, 
                                              ECORE_X_ILLUME_QUICKPANEL_STATE_OFF);
   return 1;
}

static void 
_e_quickpanel_cb_post_fetch(void *data, void *data2) 
{
   E_Border *bd;

   if (!(bd = data2)) return;
   if ((!bd->new_client) || (!bd->client.illume.quickpanel.quickpanel)) return;
   if (_e_quickpanel_by_border_get(bd)) return;
   bd->stolen = 1;
   if (bd->remember) 
     {
        if (bd->bordername) 
          {
             eina_stringshare_del(bd->bordername);
             bd->bordername = NULL;
          }
        e_remember_unuse(bd->remember);
        bd->remember = NULL;
     }
   eina_stringshare_replace(&bd->bordername, "borderless");
   bd->client.border.changed = 1;
}
