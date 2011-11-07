#include "e_illume_private.h"
#include "e_mod_quickpanel.h"

/* local function prototypes */
static Eina_Bool _e_mod_quickpanel_cb_client_message(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _e_mod_quickpanel_cb_mouse_up(void *data, int type __UNUSED__, void *event);
static Eina_Bool _e_mod_quickpanel_cb_border_add(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _e_mod_quickpanel_cb_border_remove(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _e_mod_quickpanel_cb_border_resize(void *data __UNUSED__, int type __UNUSED__, void *event);
static Eina_Bool _e_mod_quickpanel_cb_border_uniconify(void *data __UNUSED__, int type __UNUSED__, void *event);
static void _e_mod_quickpanel_cb_post_fetch(void *data __UNUSED__, void *data2);
static void _e_mod_quickpanel_cb_free(E_Illume_Quickpanel *qp);
static Eina_Bool _e_mod_quickpanel_cb_delay_hide(void *data);
static void _e_mod_quickpanel_slide(E_Illume_Quickpanel *qp, int visible, double len);
static void _e_mod_quickpanel_hide(E_Illume_Quickpanel *qp);
static Eina_Bool _e_mod_quickpanel_cb_animate(void *data);
static void _e_mod_quickpanel_position_update(E_Illume_Quickpanel *qp);
static void _e_mod_quickpanel_animate_down(E_Illume_Quickpanel *qp);
static void _e_mod_quickpanel_animate_up(E_Illume_Quickpanel *qp);
static void _e_mod_quickpanel_clickwin_show(E_Illume_Quickpanel *qp);
static void _e_mod_quickpanel_clickwin_hide(E_Illume_Quickpanel *qp);

/* local variables */
static Eina_List *_qp_hdls = NULL;
static E_Border_Hook *_qp_hook = NULL;

int 
e_mod_quickpanel_init(void) 
{
   /* add handlers for messages we are interested in */
   _qp_hdls = 
     eina_list_append(_qp_hdls, 
                      ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE, 
                                              _e_mod_quickpanel_cb_client_message, 
                                              NULL));
   _qp_hdls = 
     eina_list_append(_qp_hdls, 
                      ecore_event_handler_add(E_EVENT_BORDER_ADD, 
                                              _e_mod_quickpanel_cb_border_add, 
                                              NULL));
   _qp_hdls = 
     eina_list_append(_qp_hdls, 
                      ecore_event_handler_add(E_EVENT_BORDER_REMOVE, 
                                              _e_mod_quickpanel_cb_border_remove,
                                              NULL));
   _qp_hdls = 
     eina_list_append(_qp_hdls, 
                      ecore_event_handler_add(E_EVENT_BORDER_RESIZE, 
                                              _e_mod_quickpanel_cb_border_resize, 
                                              NULL));
   _qp_hdls = 
     eina_list_append(_qp_hdls, 
                      ecore_event_handler_add(E_EVENT_BORDER_UNICONIFY, 
                                              _e_mod_quickpanel_cb_border_uniconify, 
                                              NULL));

   /* add hook for new borders so we can test for qp borders */
   _qp_hook = e_border_hook_add(E_BORDER_HOOK_EVAL_PRE_POST_FETCH, 
                                _e_mod_quickpanel_cb_post_fetch, NULL);

   return 1;
}

int 
e_mod_quickpanel_shutdown(void) 
{
   Ecore_Event_Handler *hdl;

   /* delete the event handlers */
   EINA_LIST_FREE(_qp_hdls, hdl)
     ecore_event_handler_del(hdl);

   /* delete the border hook */
   if (_qp_hook) e_border_hook_del(_qp_hook);
   _qp_hook = NULL;

   return 1;
}

E_Illume_Quickpanel *
e_mod_quickpanel_new(E_Zone *zone) 
{
   E_Illume_Quickpanel *qp;

   /* try to allocate a new quickpanel object */
   qp = E_OBJECT_ALLOC(E_Illume_Quickpanel, E_ILLUME_QP_TYPE, 
                       _e_mod_quickpanel_cb_free);
   if (!qp) return NULL;

   /* set quickpanel zone */
   qp->zone = zone;
   qp->vert.dir = 0;
   qp->mouse_hdl = ecore_event_handler_add(ECORE_EVENT_MOUSE_BUTTON_UP, 
                                           _e_mod_quickpanel_cb_mouse_up, qp);

   return qp;
}

void 
e_mod_quickpanel_show(E_Illume_Quickpanel *qp) 
{
   E_Illume_Config_Zone *cz;
   int duration;

   if (!qp) return;

   /* delete the animator if it exists */
   if (qp->animator) ecore_animator_del(qp->animator);
   qp->animator = NULL;

   /* delete any existing timer */
   if (qp->timer) ecore_timer_del(qp->timer);
   qp->timer = NULL;

   /* if it's already visible, or has no borders to show, then get out */
   if ((qp->visible) || (!qp->borders)) return;

   duration = _e_illume_cfg->animation.quickpanel.duration;

   /* grab the height of the indicator */
   cz = e_illume_zone_config_get(qp->zone->id);
   qp->vert.isize = cz->indicator.size;

   /* check animation duration */
   if (duration <= 0) 
     {
        Eina_List *l;
        E_Border *bd;
        int ny = 0;

        ny = qp->vert.isize;
        if (qp->vert.dir == 1) ny = 0;

        /* if we are not animating, just show the borders */
        EINA_LIST_FOREACH(qp->borders, l, bd) 
          {
             if (!bd->visible) e_illume_border_show(bd);
             if (qp->vert.dir == 0) 
               {
                  e_border_fx_offset(bd, 0, ny);
                  ny += bd->h;
               }
             else 
               {
                  ny -= bd->h;
                  e_border_fx_offset(bd, 0, ny);
               }
          }
        qp->visible = 1;
        _e_mod_quickpanel_clickwin_show(qp);
     }
   else 
     _e_mod_quickpanel_slide(qp, 1, (double)duration / 1000.0);
}

void 
e_mod_quickpanel_hide(E_Illume_Quickpanel *qp) 
{
   if (!qp->visible) return;
   if (!qp->timer) 
     qp->timer = ecore_timer_add(0.2, _e_mod_quickpanel_cb_delay_hide, qp);
}

/* local functions */
static Eina_Bool
_e_mod_quickpanel_cb_client_message(void *data __UNUSED__, int type __UNUSED__, void *event) 
{
   Ecore_X_Event_Client_Message *ev;

   ev = event;
   if (ev->message_type == ECORE_X_ATOM_E_ILLUME_QUICKPANEL_STATE) 
     {
        E_Zone *zone;

        if ((zone = e_util_zone_window_find(ev->win))) 
          {
             E_Illume_Quickpanel *qp;

             if ((qp = e_illume_quickpanel_by_zone_get(zone))) 
               {
                  if (ev->data.l[0] == (int)ECORE_X_ATOM_E_ILLUME_QUICKPANEL_OFF)
                    _e_mod_quickpanel_hide(qp);
                  else
                    e_mod_quickpanel_show(qp);
               }
          }
     }
   else if (ev->message_type == ECORE_X_ATOM_E_ILLUME_QUICKPANEL_STATE_TOGGLE) 
     {
        E_Zone *zone;

        if ((zone = e_util_zone_window_find(ev->win))) 
          {
             E_Illume_Quickpanel *qp;

             if ((qp = e_illume_quickpanel_by_zone_get(zone))) 
               {
                  if (qp->visible) 
                    _e_mod_quickpanel_hide(qp);
                  else 
                    e_mod_quickpanel_show(qp);
               }
          }
     }
   else if (ev->message_type == ECORE_X_ATOM_E_ILLUME_QUICKPANEL_POSITION_UPDATE) 
     {
        E_Border *bd;
        E_Illume_Quickpanel *qp;

        if (!(bd = e_border_find_by_client_window(ev->win))) 
          return ECORE_CALLBACK_PASS_ON;
        if (!(qp = e_illume_quickpanel_by_zone_get(bd->zone))) 
          return ECORE_CALLBACK_PASS_ON;
        _e_mod_quickpanel_position_update(qp);
     }

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_quickpanel_cb_mouse_up(void *data, int type __UNUSED__, void *event) 
{
   Ecore_Event_Mouse_Button *ev;
   E_Illume_Quickpanel *qp;

   ev = event;
   if (!(qp = data)) return ECORE_CALLBACK_PASS_ON;
   if (ev->event_window != qp->clickwin) return ECORE_CALLBACK_PASS_ON;
   if (qp->visible) e_mod_quickpanel_hide(qp);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_quickpanel_cb_border_add(void *data __UNUSED__, int type __UNUSED__, void *event) 
{
   E_Event_Border_Add *ev;
   E_Illume_Quickpanel *qp;
   E_Zone *zone;
   int iy;

   ev = event;
   if (!ev->border->client.illume.quickpanel.quickpanel) 
     return ECORE_CALLBACK_PASS_ON;

   if (!(zone = ev->border->zone)) return ECORE_CALLBACK_PASS_ON;

   /* if this border should be on a different zone, get requested zone */
   if ((int)zone->num != ev->border->client.illume.quickpanel.zone) 
     {
        E_Container *con;
        int zn = 0;

        /* find this zone */
        if (!(con = e_container_current_get(e_manager_current_get())))
          return ECORE_CALLBACK_PASS_ON;
        zn = ev->border->client.illume.quickpanel.zone;
        zone = e_util_container_zone_number_get(con->num, zn);
        if (!zone) zone = e_util_container_zone_number_get(con->num, 0);
        if (!zone) return ECORE_CALLBACK_PASS_ON;
     }

   if (!(qp = e_illume_quickpanel_by_zone_get(zone))) 
     return ECORE_CALLBACK_PASS_ON;

   /* set position and zone */
   e_illume_border_indicator_pos_get(zone, NULL, &iy);
   if ((ev->border->x != zone->x) || (ev->border->y != iy)) 
     e_border_move(ev->border, zone->x, iy);
   if (ev->border->zone != zone) 
     e_border_zone_set(ev->border, zone);
   
   /* hide this border */
   e_illume_border_hide(ev->border);

   qp->vert.size += ev->border->h;

   /* add this border to QP border collection */
   qp->borders = eina_list_append(qp->borders, ev->border);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_quickpanel_cb_border_remove(void *data __UNUSED__, int type __UNUSED__, void *event) 
{
   E_Event_Border_Remove *ev;
   E_Illume_Quickpanel *qp;
   E_Zone *zone;
   Eina_List *l;
   E_Border *bd;

   ev = event;
   if (!ev->border->client.illume.quickpanel.quickpanel) 
     return ECORE_CALLBACK_PASS_ON;

   if (!(zone = ev->border->zone)) return ECORE_CALLBACK_PASS_ON;

   /* if this border should be on a different zone, get requested zone */
   if ((int)zone->num != ev->border->client.illume.quickpanel.zone) 
     {
        E_Container *con;
        int zn = 0;

        /* find this zone */
        if (!(con = e_container_current_get(e_manager_current_get())))
          return ECORE_CALLBACK_PASS_ON;
        zn = ev->border->client.illume.quickpanel.zone;
        zone = e_util_container_zone_number_get(con->num, zn);
        if (!zone) zone = e_util_container_zone_number_get(con->num, 0);
        if (!zone) return ECORE_CALLBACK_PASS_ON;
     }

   if (!(qp = e_illume_quickpanel_by_zone_get(zone))) 
     return ECORE_CALLBACK_PASS_ON;

   /* add this border to QP border collection */
   if (qp->borders) 
     qp->borders = eina_list_remove(qp->borders, ev->border);

   qp->vert.size = 0;
   EINA_LIST_FOREACH(qp->borders, l, bd)
     qp->vert.size += bd->h;

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_quickpanel_cb_border_resize(void *data __UNUSED__, int type __UNUSED__, void *event) 
{
   E_Event_Border_Resize *ev;
   E_Illume_Quickpanel *qp;
   Eina_List *l;
   E_Border *bd;

   ev = event;
   if (!ev->border->client.illume.quickpanel.quickpanel) 
     return ECORE_CALLBACK_PASS_ON;
   if (!(qp = e_illume_quickpanel_by_zone_get(ev->border->zone))) 
     return ECORE_CALLBACK_PASS_ON;

   qp->vert.size = 0;
   EINA_LIST_FOREACH(qp->borders, l, bd)
     qp->vert.size += bd->h;

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool 
_e_mod_quickpanel_cb_border_uniconify(void *data __UNUSED__, int type __UNUSED__, void *event) 
{
   E_Event_Border_Uniconify *ev;
   E_Illume_Quickpanel *qp;

   ev = event;
   if (!ev->border->client.illume.quickpanel.quickpanel)
     return ECORE_CALLBACK_PASS_ON;
   if (!(qp = e_illume_quickpanel_by_zone_get(ev->border->zone)))
     return ECORE_CALLBACK_PASS_ON;
   e_mod_quickpanel_show(qp);
   return ECORE_CALLBACK_PASS_ON;
}

static void 
_e_mod_quickpanel_cb_post_fetch(void *data __UNUSED__, void *data2) 
{
   E_Border *bd;

   if (!(bd = data2)) return;
   if (!bd->client.illume.quickpanel.quickpanel) return;
   bd->stolen = 1;
}

static void 
_e_mod_quickpanel_cb_free(E_Illume_Quickpanel *qp) 
{
   E_Border *bd;

   /* delete the animator if it exists */
   if (qp->animator) ecore_animator_del(qp->animator);
   qp->animator = NULL;

   /* delete the timer if it exists */
   if (qp->timer) ecore_timer_del(qp->timer);
   qp->timer = NULL;

   /* delete the clickwin */
   if (qp->clickwin) ecore_x_window_free(qp->clickwin);
   qp->clickwin = 0;

   /* delete the mouse handler */
   if (qp->mouse_hdl) ecore_event_handler_del(qp->mouse_hdl);
   qp->mouse_hdl = NULL;

   /* set the borders of this quickpanel to not stolen */
   EINA_LIST_FREE(qp->borders, bd)
     bd->stolen = 0;

   /* free the structure */
   E_FREE(qp);
}

static Eina_Bool
_e_mod_quickpanel_cb_delay_hide(void *data) 
{
   E_Illume_Quickpanel *qp;

   if (!(qp = data)) return ECORE_CALLBACK_CANCEL;
   _e_mod_quickpanel_hide(qp);
   return ECORE_CALLBACK_CANCEL;
}

static void 
_e_mod_quickpanel_slide(E_Illume_Quickpanel *qp, int visible, double len) 
{
   if (!qp) return;
   qp->start = ecore_loop_time_get();
   qp->len = len;
   qp->vert.adjust_start = qp->vert.adjust;
   qp->vert.adjust_end = 0;
   if (qp->vert.dir == 0) 
     {
        if (visible) qp->vert.adjust_end = qp->vert.size;
     }
   else 
     {
        if (visible) qp->vert.adjust_end = -qp->vert.size;
     }

   if (!qp->animator) 
     qp->animator = ecore_animator_add(_e_mod_quickpanel_cb_animate, qp);
}

static void 
_e_mod_quickpanel_hide(E_Illume_Quickpanel *qp) 
{
   int duration;

   if (!qp) return;

   /* delete the animator if it exists */
   if (qp->animator) ecore_animator_del(qp->animator);
   qp->animator = NULL;

   /* delete the timer if it exists */
   if (qp->timer) ecore_timer_del(qp->timer);
   qp->timer = NULL;

   /* if it's not visible, we can't hide it */
   if (!qp->visible) return;

   duration = _e_illume_cfg->animation.quickpanel.duration;

   if (duration <= 0) 
     {
        Eina_List *l;
        E_Border *bd;

        /* if we are not animating, hide the qp borders */
        EINA_LIST_REVERSE_FOREACH(qp->borders, l, bd) 
          {
             e_border_fx_offset(bd, 0, 0);
             if (bd->visible) e_illume_border_hide(bd);
          }
        qp->visible = 0;
        _e_mod_quickpanel_clickwin_hide(qp);
     }
   else
     _e_mod_quickpanel_slide(qp, 0, (double)duration / 1000.0);
}

static Eina_Bool
_e_mod_quickpanel_cb_animate(void *data) 
{
   E_Illume_Quickpanel *qp;
   double t, v = 1.0;

   if (!(qp = data)) return ECORE_CALLBACK_CANCEL;
   t = (ecore_loop_time_get() - qp->start);
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

   qp->vert.adjust = ((qp->vert.adjust_end * v) + 
                      (qp->vert.adjust_start * (1.0 - v)));

   if (qp->vert.dir == 0) _e_mod_quickpanel_animate_down(qp);
   else _e_mod_quickpanel_animate_up(qp);

   if (t == qp->len) 
     {
        qp->animator = NULL;
        if (qp->visible) 
          {
             qp->visible = 0;
             _e_mod_quickpanel_clickwin_hide(qp);
          }
        else 
          {
             qp->visible = 1;
             _e_mod_quickpanel_clickwin_show(qp);
          }
        return ECORE_CALLBACK_CANCEL;
     }

   return ECORE_CALLBACK_RENEW;
}

static void 
_e_mod_quickpanel_position_update(E_Illume_Quickpanel *qp) 
{
   Eina_List *l;
   E_Border *bd;
   int iy = 0;

   if (!qp) return;
   _e_mod_quickpanel_hide(qp);
   if (!qp->zone) return;
   e_illume_border_indicator_pos_get(qp->zone, NULL, &iy);
   EINA_LIST_FOREACH(qp->borders, l, bd) 
     e_border_move(bd, qp->zone->x, iy);

   qp->vert.dir = 0;
   if ((iy + qp->vert.isize + qp->vert.size) > qp->zone->h) qp->vert.dir = 1;
}

static void 
_e_mod_quickpanel_animate_down(E_Illume_Quickpanel *qp) 
{
   Eina_List *l;
   E_Border *bd;
   int pbh = 0;

   if (!qp) return;
   pbh = (qp->vert.isize - qp->vert.size);
   EINA_LIST_FOREACH(qp->borders, l, bd) 
     {
        /* don't adjust borders that are being deleted */
        if (e_object_is_del(E_OBJECT(bd))) continue;
        if (bd->fx.y != (qp->vert.adjust + pbh)) 
          e_border_fx_offset(bd, 0, (qp->vert.adjust + pbh));
        pbh += bd->h;

        if (!qp->visible) 
          {
             if (bd->fx.y > 0) 
               {
                  if (!bd->visible) e_illume_border_show(bd);
               }
          }
        else 
          {
             if (bd->fx.y <= 10) 
               {
                  if (bd->visible) e_illume_border_hide(bd);
               }
          }
     }
}

static void 
_e_mod_quickpanel_animate_up(E_Illume_Quickpanel *qp) 
{
   Eina_List *l;
   E_Border *bd;
   int pbh = 0;

   if (!qp) return;
   pbh = qp->vert.size;
   EINA_LIST_FOREACH(qp->borders, l, bd) 
     {
        /* don't adjust borders that are being deleted */
        if (e_object_is_del(E_OBJECT(bd))) continue;
        pbh -= bd->h;
        if (bd->fx.y != (qp->vert.adjust + pbh)) 
          e_border_fx_offset(bd, 0, (qp->vert.adjust + pbh));

        if (!qp->visible) 
          {
             if (bd->fx.y < 0) 
               {
                  if (!bd->visible) e_illume_border_show(bd);
               }
          }
        else 
          {
             if (bd->fx.y >= -10) 
               {
                  if (bd->visible) e_illume_border_hide(bd);
               }
          }
     }
}

static void 
_e_mod_quickpanel_clickwin_show(E_Illume_Quickpanel *qp) 
{
   E_Border *ind;

   if ((!qp) || (!qp->borders) || (!qp->zone)) return;
   if (!(ind = eina_list_nth(qp->borders, 0))) return;

   if (qp->clickwin) ecore_x_window_free(qp->clickwin);
   qp->clickwin = 0;
   qp->clickwin = ecore_x_window_input_new(qp->zone->container->win, 
                                           qp->zone->x, qp->zone->y, 
                                           qp->zone->w, qp->zone->h);

   ecore_x_window_configure(qp->clickwin, 
                            ECORE_X_WINDOW_CONFIGURE_MASK_SIBLING | 
                            ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE, 
                            qp->zone->x, qp->zone->y, 
                            qp->zone->w, qp->zone->h, 0, 
                            ind->win, ECORE_X_WINDOW_STACK_BELOW);

   ecore_x_window_show(qp->clickwin);
}

static void 
_e_mod_quickpanel_clickwin_hide(E_Illume_Quickpanel *qp) 
{
   if (!qp) return;
   if (qp->clickwin) ecore_x_window_free(qp->clickwin);
   qp->clickwin = 0;
}
