#include "e.h"
#include "e_quickpanel.h"
#include "e_mod_config.h"
#include "e_mod_border.h"

/* local function prototypes */
static void _e_quickpanel_cb_free(E_Quickpanel *qp);
static int _e_quickpanel_cb_client_message(void *data, int type, void *event);
static void _e_quickpanel_cb_border_pre_post_fetch(void *data, void *data2);
static E_Quickpanel *_e_quickpanel_by_border_get(E_Border *bd);
static int _e_quickpanel_border_is_quickpanel(E_Border *bd);
static void _e_quickpanel_slide(E_Quickpanel *qp, int visible, double len);
static int _e_quickpanel_cb_animate(void *data);
static int _e_quickpanel_cb_delay_hide(void *data);
static void _e_quickpanel_hide(E_Quickpanel *qp);
static int _e_quickpanel_cb_sort(const void *b1, const void *b2);

/* local variables */
static Eina_List *qps = NULL, *handlers = NULL, *hooks = NULL;

/* public functions */
int 
e_quickpanel_init(void) 
{
   handlers = 
     eina_list_append(handlers, 
                      ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE, 
                                              _e_quickpanel_cb_client_message, NULL));
   hooks = 
     eina_list_append(hooks, 
                      e_border_hook_add(E_BORDER_HOOK_EVAL_PRE_POST_FETCH, 
                                        _e_quickpanel_cb_border_pre_post_fetch, 
                                        NULL));
   return 1;
}

int 
e_quickpanel_shutdown(void) 
{
   E_Quickpanel *qp;
   Ecore_Event_Handler *handler;
   E_Border_Hook *hook;

   EINA_LIST_FREE(handlers, handler)
     ecore_event_handler_del(handler);
   EINA_LIST_FREE(hooks, hook)
     e_border_hook_del(hook);
   EINA_LIST_FREE(qps, qp)
     e_object_del(E_OBJECT(qp));
   return 1;
}

E_Quickpanel *
e_quickpanel_new(E_Zone *zone) 
{
   E_Quickpanel *qp;

   qp = E_OBJECT_ALLOC(E_Quickpanel, E_QUICKPANEL_TYPE, _e_quickpanel_cb_free);
   if (!qp) return NULL;
   qp->zone = zone;
   qp->visible = 0;
   qps = eina_list_append(qps, qp);
   return qp;
}

void 
e_quickpanel_show(E_Quickpanel *qp) 
{
   if (qp->timer) ecore_timer_del(qp->timer);
   qp->timer = NULL;
   if (qp->visible) return;
   if (il_cfg->sliding.quickpanel.duration <= 0) 
     {
        Eina_List *l;
        E_Border *bd;
        int ny = 0;

        e_mod_border_top_shelf_size_get(qp->zone, NULL, &ny);
        EINA_LIST_FOREACH(qp->borders, l, bd)
          ny += bd->h;

        EINA_LIST_FOREACH(qp->borders, l, bd) 
          {
             e_border_lower(bd);
             e_border_fx_offset(bd, 0, ny);
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
   EINA_LIST_FOREACH(qps, l, qp)
     if (qp->zone == zone) return qp;
   return NULL;
}

void 
e_quickpanel_position_update(E_Quickpanel *qp) 
{
   Eina_List *l;
   E_Border *bd;
   int ty;

   if (!qp) return;
   e_mod_border_top_shelf_pos_get(qp->zone, NULL, &ty);
   EINA_LIST_FOREACH(qp->borders, l, bd) 
     {
        bd->x = qp->zone->x;
        bd->y = (ty - qp->h);
        bd->changed = 1;
        bd->changes.pos = 1;
//        e_border_move(bd, qp->zone->x, (ty - qp->h));
     }
}

/* local functions */
static void 
_e_quickpanel_cb_free(E_Quickpanel *qp) 
{
   E_Border *bd;

   if (qp->animator) ecore_animator_del(qp->animator);
   qp->animator = NULL;
   if (qp->timer) ecore_timer_del(qp->timer);
   qp->timer = NULL;
   EINA_LIST_FREE(qp->borders, bd)
     bd->stolen = 0;
   E_FREE(qp);
}

static int 
_e_quickpanel_cb_client_message(void *data, int type, void *event) 
{
   Ecore_X_Event_Client_Message *ev;
   E_Zone *zone;
   E_Quickpanel *qp;

   ev = event;
   if (ev->win != ecore_x_window_root_first_get()) return 1;
   if (ev->message_type != ECORE_X_ATOM_E_ILLUME_QUICKPANEL_STATE) return 1;
   zone = e_zone_current_get(e_container_current_get(e_manager_current_get()));
   if (!(qp = e_quickpanel_by_zone_get(zone))) return 1;
   if (ev->data.l[0] == ECORE_X_ATOM_E_ILLUME_QUICKPANEL_OFF) 
     e_quickpanel_hide(qp);
   else if (ev->data.l[0] == ECORE_X_ATOM_E_ILLUME_QUICKPANEL_ON) 
     e_quickpanel_show(qp);
   return 1;
}

static void 
_e_quickpanel_cb_border_pre_post_fetch(void *data, void *data2) 
{
   E_Border *bd;
   E_Quickpanel *qp;
   int ty;

   if (!(bd = data2)) return;
   if (!bd->new_client) return;
   if (!_e_quickpanel_border_is_quickpanel(bd)) return;
   if (_e_quickpanel_by_border_get(bd)) return;
   if (!(qp = e_quickpanel_by_zone_get(bd->zone))) return;
   e_mod_border_top_shelf_pos_get(qp->zone, NULL, &ty);
   bd->stolen = 1;
   if (bd->remember) 
     {
        if (bd->bordername) 
          {
             eina_stringshare_del(bd->bordername);
             bd->bordername = NULL;
             bd->client.border.changed = 1;
          }
        e_remember_unuse(bd->remember);
        bd->remember = NULL;
     }
   eina_stringshare_replace(&bd->bordername, "borderless");
   bd->client.border.changed = 1;
   qp->h += bd->h;
   e_border_move(bd, qp->zone->x, (ty - qp->h));
   if (qp->visible) 
     {
        int th;

        e_mod_border_top_shelf_size_get(qp->zone, NULL, &th);
        e_border_fx_offset(bd, 0, (bd->h - th));
     }
   qp->borders = eina_list_append(qp->borders, bd);
   qp->borders = eina_list_sort(qp->borders, -1, _e_quickpanel_cb_sort);
   qp->borders = eina_list_reverse(qp->borders);
}

static E_Quickpanel *
_e_quickpanel_by_border_get(E_Border *bd) 
{
   Eina_List *l, *ll;
   E_Border *b;
   E_Quickpanel *qp;

   if (!bd->stolen) return NULL;
   EINA_LIST_FOREACH(qps, l, qp) 
     EINA_LIST_FOREACH(qp->borders, ll, b)
       if (b == bd) return qp;
   return NULL;
}

static int 
_e_quickpanel_border_is_quickpanel(E_Border *bd) 
{
   return ecore_x_e_illume_quickpanel_get(bd->client.win);
}

static void 
_e_quickpanel_slide(E_Quickpanel *qp, int visible, double len) 
{
   qp->start = ecore_loop_time_get();
   qp->len = len;
   qp->adjust_start = qp->adjust;
   qp->adjust_end = 0;
   if (visible) 
     {
        Eina_List *l;
        E_Border *bd;

        EINA_LIST_FOREACH(qp->borders, l, bd) 
          qp->adjust_end += bd->h;
     }
   else 
     {
        int th;

        e_mod_border_top_shelf_size_get(qp->zone, NULL, &th);
        qp->adjust_end = -th;
     }
   if (!qp->animator)
     qp->animator = ecore_animator_add(_e_quickpanel_cb_animate, qp);
}

static int 
_e_quickpanel_cb_animate(void *data) 
{
   E_Quickpanel *qp;
   Eina_List *l;
   E_Border *bd;
   double t, v;

   if (!(qp = data)) return 0;
   t = ecore_loop_time_get() - qp->start;
   if (t > qp->len) t = qp->len;
   if (qp->len > 0.0) 
     {
        v = t / qp->len;
        v = 1.0 - v;
        v = v * v * v * v;
        v = 1.0 - v;
     }
   else
     {
        t = qp->len;
        v = 1.0;
     }
   qp->adjust = (qp->adjust_end * v) + (qp->adjust_start * (1.0 - v));

   EINA_LIST_FOREACH(qp->borders, l, bd) 
     {
        e_border_lower(bd);
        e_border_fx_offset(bd, 0, (bd->h + qp->adjust));
     }

   if (t == qp->len) 
     {
        qp->animator = NULL;
        if (qp->visible) 
          qp->visible = 0;
        else 
          qp->visible = 1;
        return 0;
     }
   return 1;
}

static int 
_e_quickpanel_cb_delay_hide(void *data) 
{
   E_Quickpanel *qp;

   if (!(qp = data)) return 0;
   _e_quickpanel_hide(qp);
   qp->timer = NULL;
   return 0;
}

static void 
_e_quickpanel_hide(E_Quickpanel *qp) 
{
   if (qp->timer) ecore_timer_del(qp->timer);
   qp->timer = NULL;
   if (!qp->visible) return;
   if (il_cfg->sliding.quickpanel.duration <= 0) 
     {
        Eina_List *l;
        E_Border *bd;

        EINA_LIST_FOREACH(qp->borders, l, bd) 
          {
             e_border_lower(bd);
             e_border_fx_offset(bd, 0, 0);
          }
        qp->visible = 0;
     }
   else
     _e_quickpanel_slide(qp, 0, 
                         (double)il_cfg->sliding.quickpanel.duration / 1000.0);
}

static int 
_e_quickpanel_cb_sort(const void *b1, const void *b2) 
{
   const E_Border *bd1, *bd2;
   int major1, minor1;
   int major2, minor2;

   if (!(bd1 = b1)) return -1;
   if (!(bd2 = b2)) return 1;
   major1 = ecore_x_e_illume_quickpanel_priority_major_get(bd1->client.win);
   major2 = ecore_x_e_illume_quickpanel_priority_major_get(bd2->client.win);
   minor1 = ecore_x_e_illume_quickpanel_priority_minor_get(bd1->client.win);
   minor2 = ecore_x_e_illume_quickpanel_priority_minor_get(bd2->client.win);
   if (major1 < major2) return -1;
   else if (major1 > major2) return 1;
   else 
     {
        if (major1 == major2) 
          {
             if (minor1 < minor2) return -1;
             else if (minor1 > minor2) return 1;
          }
     }
   return 0;
}
