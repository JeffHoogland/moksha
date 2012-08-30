#include "e.h"
#include "e_mod_main.h"
#include "e_mod_physics.h"
#include <EPhysics.h>

typedef struct _E_Physics     E_Physics;
typedef struct _E_Physics_Win E_Physics_Win;
typedef struct _E_Physics_Shelf E_Physics_Shelf;

struct _E_Physics
{
   EPhysics_World *world;
   Ecore_X_Window  win;
   E_Manager      *man;
   Eina_Inlist    *wins;
   Eina_Inlist    *shelves;
};

struct _E_Physics_Shelf
{
   EINA_INLIST;

   E_Physics *p;
   E_Shelf *es;
   EPhysics_Body *body;
};

struct _E_Physics_Win
{
   EINA_INLIST;

   E_Physics           *p;     // parent physics
   Ecore_X_Window       win;  // raw window - for menus etc.
   E_Border            *bd;  // if its a border - later
   E_Popup             *pop;  // if its a popup - later
   E_Menu              *menu;  // if it is a menu - later
   EPhysics_Body       *body; // physics body
   int                  x, y, w, h;  // geometry
   int                 ix, iy; // impulse geometry (next move coord */
   struct
   {
      int x, y, w, h; // hidden geometry (used when its unmapped and re-instated on map)
   } hidden;
   int                  border;  // border width

   E_Border_Hook       *begin; // begin move
   E_Border_Hook       *end; // end move
   E_Maximize           maximize;

   unsigned int         stopped; /* number of frames window has been stopped for */
   unsigned int         started; /* number of frames window has been moving for */
   double               impulse_x, impulse_y; /* x,y for impulse vector */

   Eina_Bool            visible : 1;  // is visible
   Eina_Bool            inhash : 1;  // is in the windows hash
   Eina_Bool            show_ready : 1;  // is this window ready for its first show
   Eina_Bool            moving : 1;
   Eina_Bool            impulse : 1;
};

static Eina_List *handlers = NULL;
static Eina_List *physicists = NULL;
static Eina_Hash *borders = NULL;

//////////////////////////////////////////////////////////////////////////
#undef DBG
#if 0
#define DBG(f, x ...) printf(f, ##x)
#else
#define DBG(f, x ...)
#endif

static Eina_Bool _e_mod_physics_bd_fullscreen(void *data __UNUSED__, int type, void *event);
static void _e_mod_physics_bd_move_intercept_cb(E_Border *bd, int x, int y);
static void _e_mod_physics_win_update_cb(E_Physics_Win *pw, EPhysics_Body *body, void *event_info __UNUSED__);
static void _e_mod_physics_win_del(E_Physics_Win *pw);
static void _e_mod_physics_win_show(E_Physics_Win *pw);
static void _e_mod_physics_win_hide(E_Physics_Win *pw);
static void _e_mod_physics_win_configure(E_Physics_Win *pw,
                                         int x,
                                         int y,
                                         int w,
                                         int h,
                                         int border);
static void _e_mod_physics_shelf_new(E_Physics *p, E_Shelf *es);
/* TODO
static void
_e_mod_physics_fps_update(E_Physics *p)
{
   if (_comp_mod->conf->fps_show)
     {
        if (!p->fps_bg)
          {
             p->fps_bg = evas_object_rectangle_add(p->evas);
             evas_object_color_set(p->fps_bg, 0, 0, 0, 128);
             evas_object_layer_set(p->fps_bg, EVAS_LAYER_MAX);
             evas_object_show(p->fps_bg);

             p->fps_fg = evas_object_text_add(p->evas);
             evas_object_text_font_set(p->fps_fg, "Sans", 10);
             evas_object_text_text_set(p->fps_fg, "???");
             evas_object_color_set(p->fps_fg, 255, 255, 255, 255);
             evas_object_layer_set(p->fps_fg, EVAS_LAYER_MAX);
             evas_object_show(p->fps_fg);
          }
     }
   else
     {
        if (p->fps_fg)
          {
             evas_object_del(p->fps_fg);
             p->fps_fg = NULL;
          }
        if (p->fps_bg)
          {
             evas_object_del(p->fps_bg);
             p->fps_bg = NULL;
          }
     }
}
*/


static void
_e_mod_physics_move_end(void *p_w, void *b_d)
{
   E_Physics_Win *pw = p_w;
   /* wrong border */
   if (b_d != pw->bd) return;
   DBG("PHYS: DRAG END %d\n", pw->win);
   pw->impulse = pw->moving = EINA_FALSE;
}

static void
_e_mod_physics_move_begin(void *p_w, void *b_d)
{
   E_Physics_Win *pw = p_w;
   /* wrong border */
   if (b_d != pw->bd) return;
   DBG("PHYS: DRAG BEGIN %d\n", pw->win);
   pw->moving = EINA_TRUE;
   pw->started = 0;
   /* hacks! */
   if ((!pw->impulse_x) && (!pw->impulse_y)) return;
   _e_mod_physics_win_hide(pw);
   _e_mod_physics_win_show(pw);
   pw->show_ready = 0;
   _e_mod_physics_win_configure(pw, pw->bd->x, pw->bd->y, pw->bd->w, pw->bd->h, pw->border);
   pw->impulse_x = pw->impulse_y = 0;
}

static E_Physics *
_e_mod_physics_find(Ecore_X_Window root)
{
   Eina_List *l;
   E_Physics *p;

   // fixme: use hash if physicists list > 4
   EINA_LIST_FOREACH (physicists, l, p)
     {
        if (p->man->root == root) return p;
     }
   return NULL;
}

static E_Physics_Win *
_e_mod_physics_win_find(Ecore_X_Window win)
{
   return eina_hash_find(borders, e_util_winid_str_get(win));
}

static void
_e_mod_physics_win_show(E_Physics_Win *pw)
{
   if (pw->visible) return;
   DBG("PHYS: SHOW %d\n", pw->win);
   pw->visible = 1;
   pw->body = ephysics_body_box_add(pw->p->world);
   ephysics_body_friction_set(pw->body, 1.0);
   ephysics_body_rotation_on_z_axis_enable_set(pw->body, EINA_FALSE);
   ephysics_body_event_callback_add(pw->body, EPHYSICS_CALLBACK_BODY_UPDATE, (EPhysics_Body_Event_Cb)_e_mod_physics_win_update_cb, pw);
   pw->begin = e_border_hook_add(E_BORDER_HOOK_MOVE_BEGIN, _e_mod_physics_move_begin, pw);
   pw->end = e_border_hook_add(E_BORDER_HOOK_MOVE_END, _e_mod_physics_move_end, pw);
   e_border_move_intercept_cb_set(pw->bd, _e_mod_physics_bd_move_intercept_cb);
}

static void
_e_mod_physics_bd_move_intercept_cb(E_Border *bd, int x, int y)
{
   E_Physics_Win *pw;

   pw = _e_mod_physics_win_find(bd->client.win);
   if ((bd->x == x) && (bd->y == y))
     {
        DBG("PHYS: STOPPED %d\n", pw->win);
        if (pw->moving) pw->started = 0;
        pw->show_ready = 0;
        _e_mod_physics_win_configure(pw, x, y, pw->bd->w, pw->bd->h, pw->border);
        return;
     }
   if (!pw) return;
   DBG("PHYS: MOVE %d - (%d,%d) ->(WANT) (%d,%d)\n", pw->win, bd->x, bd->y, x, y);
   if ((!pw->body) || (!pw->moving) || (pw->started < _physics_mod->conf->delay))
     {
        DBG("PHYS: MOVE THROUGH\n");
        bd->x = x, bd->y = y;
        if (pw->moving) pw->started++;
        pw->show_ready = 0;
        _e_mod_physics_win_configure(pw, x, y, pw->bd->w, pw->bd->h, pw->border);
        return;
     }
   if (!pw->impulse)
     {
        DBG("PHYS: IMPULSE APPLY\n");
        pw->impulse_x = (x - bd->x) * 5;
        pw->impulse_y = (bd->y - y) * 5;
        ephysics_body_central_impulse_apply(pw->body, pw->impulse_x, pw->impulse_y);
        bd->x = x, bd->y = y;
        pw->impulse = EINA_TRUE;
        return;
     }
   if ((pw->ix == x) && (pw->iy == y))
     bd->x = x, bd->y = y;
   else
     {
        if (E_INSIDE(bd->x, bd->y, pw->p->man->x, pw->p->man->y, pw->p->man->w, pw->p->man->h) &&
            E_INSIDE(bd->x + bd->w, bd->y, pw->p->man->x, pw->p->man->y, pw->p->man->w, pw->p->man->h) &&
            E_INSIDE(bd->x, bd->y + bd->h, pw->p->man->x, pw->p->man->y, pw->p->man->w, pw->p->man->h) &&
            E_INSIDE(bd->x + bd->w, bd->y + bd->h, pw->p->man->x, pw->p->man->y, pw->p->man->w, pw->p->man->h))
          DBG("REJECTED!\n");
        else if (pw->moving)
          {
             DBG("UPDATE\n");
             bd->x = x, bd->y = y;
             pw->show_ready = 0;
             _e_mod_physics_win_configure(pw, x, y, pw->bd->w, pw->bd->h, pw->border);
          }
     }
}

static E_Physics_Win *
_e_mod_physics_win_add(E_Physics *p, E_Border *bd)
{
   Ecore_X_Window_Attributes att;
   E_Physics_Win *pw;

   memset((&att), 0, sizeof(Ecore_X_Window_Attributes));
   if (!ecore_x_window_attributes_get(bd->client.win, &att))
     return NULL;
   /* don't physics on input grab windows or tooltips */
   if (att.input_only || att.override) return NULL;

   pw = calloc(1, sizeof(E_Physics_Win));
   if (!pw) return NULL;
   pw->win = bd->client.win;
   pw->p = p;
   pw->bd = bd;
   eina_hash_add(borders, e_util_winid_str_get(pw->bd->client.win), pw);
   if (pw->bd->visible)
     _e_mod_physics_win_show(pw);
   DBG("PHYS: WIN %d ADD\n", bd->client.win);
   p->wins = eina_inlist_append(p->wins, EINA_INLIST_GET(pw));
   return pw;
}

static void
_e_mod_physics_win_del(E_Physics_Win *pw)
{
   eina_hash_del(borders, e_util_winid_str_get(pw->bd->client.win), pw);
   e_border_move_intercept_cb_set(pw->bd, NULL);
   pw->bd = NULL;
   pw->p->wins = eina_inlist_remove(pw->p->wins, EINA_INLIST_GET(pw));
   if (pw->body) ephysics_body_del(pw->body);
   memset(pw, 0, sizeof(E_Physics_Win));
   free(pw);
}

static void
_e_mod_physics_win_hide(E_Physics_Win *pw)
{
   if (!pw->visible) return;
   DBG("PHYS: HIDE %d\n", pw->win);
   pw->show_ready = pw->visible = 0;
   ephysics_body_del(pw->body);
   pw->body = NULL;
   e_border_hook_del(pw->begin);
   e_border_hook_del(pw->end);
   pw->begin = pw->end = NULL;
   e_border_move_intercept_cb_set(pw->bd, NULL);
}

#if 0
static void
_e_mod_physics_win_raise_above(E_Physics_Win *pw,
                               E_Physics_Win *cw2)
{
   DBG("  [0x%x] abv [0x%x]\n", pw->win, cw2->win);
   pw->p->wins_invalid = 1;
   pw->p->wins = eina_inlist_remove(pw->p->wins, EINA_INLIST_GET(pw));
   pw->p->wins = eina_inlist_append_relative(pw->p->wins,
                                             EINA_INLIST_GET(pw),
                                             EINA_INLIST_GET(cw2));
   evas_object_stack_above(pw->shobj, cw2->shobj);
   if (pw->bd)
     {
        Eina_List *l;
        E_Border *tmp;

        EINA_LIST_FOREACH (pw->bd->client.e.state.video_child, l, tmp)
          {
             E_Physics_Win *tcw;

             tcw = eina_hash_find(borders, e_util_winid_str_get(tmp->client.win));
             if (!tcw) continue;

             evas_object_stack_below(tcw->shobj, pw->shobj);
          }
     }

   _e_mod_physics_win_render_queue(pw);
   pw->pending_count++;
   e_manager_comp_event_src_config_send
     (pw->p->man, (E_Manager_Comp_Source *)pw,
     _e_mod_physics_cb_pending_after, pw->p);
}

static void
_e_mod_physics_win_raise(E_Physics_Win *pw)
{
   DBG("  [0x%x] rai\n", pw->win);
   pw->p->wins_invalid = 1;
   pw->p->wins = eina_inlist_remove(pw->p->wins, EINA_INLIST_GET(pw));
   pw->p->wins = eina_inlist_append(pw->p->wins, EINA_INLIST_GET(pw));
   evas_object_raise(pw->shobj);
   if (pw->bd)
     {
        Eina_List *l;
        E_Border *tmp;

        EINA_LIST_FOREACH (pw->bd->client.e.state.video_child, l, tmp)
          {
             E_Physics_Win *tcw;

             tcw = eina_hash_find(borders, e_util_winid_str_get(tmp->client.win));
             if (!tcw) continue;

             evas_object_stack_below(tcw->shobj, pw->shobj);
          }
     }

   _e_mod_physics_win_render_queue(pw);
   pw->pending_count++;
   e_manager_comp_event_src_config_send
     (pw->p->man, (E_Manager_Comp_Source *)pw,
     _e_mod_physics_cb_pending_after, pw->p);
}

static void
_e_mod_physics_win_lower(E_Physics_Win *pw)
{
   DBG("  [0x%x] low\n", pw->win);
   pw->p->wins_invalid = 1;
   pw->p->wins = eina_inlist_remove(pw->p->wins, EINA_INLIST_GET(pw));
   pw->p->wins = eina_inlist_prepend(pw->p->wins, EINA_INLIST_GET(pw));
   evas_object_lower(pw->shobj);
   if (pw->bd)
     {
        Eina_List *l;
        E_Border *tmp;

        EINA_LIST_FOREACH (pw->bd->client.e.state.video_child, l, tmp)
          {
             E_Physics_Win *tcw;

             tcw = eina_hash_find(borders, e_util_winid_str_get(tmp->client.win));
             if (!tcw) continue;

             evas_object_stack_below(tcw->shobj, pw->shobj);
          }
     }

   _e_mod_physics_win_render_queue(pw);
   pw->pending_count++;
   e_manager_comp_event_src_config_send
     (pw->p->man, (E_Manager_Comp_Source *)pw,
     _e_mod_physics_cb_pending_after, pw->p);
}
#endif

static void
_e_mod_physics_win_mass_set(E_Physics_Win *pw)
{
   double mass;
   E_Border *bd = pw->bd;
   if (!bd) return;
   if (bd->remember)
     {
        if ((bd->remember->apply & E_REMEMBER_APPLY_POS) || 
            (bd->remember->prop.lock_client_location) || 
            (bd->remember->prop.lock_user_location))
          ephysics_body_mass_set(pw->body, 50000);
        return;
     }
   mass = _physics_mod->conf->max_mass * (((double)pw->w / (double)pw->p->man->w) + ((double)pw->h / (double)pw->p->man->h) / 2.);
   DBG("PHYS: WIN %d MASS %g\n", pw->win, mass);
   ephysics_body_mass_set(pw->body, mass);
}

static void
_e_mod_physics_win_configure(E_Physics_Win *pw,
                             int x,
                             int y,
                             int w,
                             int h,
                             int border)
{
   //DBG("PHYS: CONFIG %d\n", pw->win);
   if (!pw->visible)
     {
        pw->hidden.x = x;
        pw->hidden.y = y;
        pw->border = border;
     }
   else
     {
        if (!((x == pw->x) && (y == pw->y)))
          {
             pw->x = x;
             pw->y = y;
          }
        pw->hidden.x = x;
        pw->hidden.y = y;
     }
   pw->maximize = pw->bd->maximized;
   pw->hidden.w = w;
   pw->hidden.h = h;
   if (!((w == pw->w) && (h == pw->h)))
     {
        pw->w = w;
        pw->h = h;
     }
   if (pw->border != border)
     {
        pw->border = border;
     }
   if ((!pw->show_ready) && pw->body)
     {
        _e_mod_physics_win_mass_set(pw);
        ephysics_body_geometry_set(pw->body, x, y, w, border + h);
        pw->show_ready = 1;
     }
}

static E_Physics_Shelf *
_e_mod_physics_shelf_find(E_Physics *p, E_Shelf *es)
{
   E_Physics_Shelf *eps;
   EINA_INLIST_FOREACH(p->shelves, eps)
     if (eps->es == es) return eps;
   return NULL;
}

static void
_e_mod_physics_shelf_free(E_Physics *p, E_Shelf *es)
{
   E_Physics_Shelf *eps;

   eps = _e_mod_physics_shelf_find(p, es);
   if (!eps) return;
   if (eps->body) ephysics_body_del(eps->body);
   p->shelves = eina_inlist_remove(p->shelves, EINA_INLIST_GET(eps));
   free(eps);
}

static void
_e_mod_physics_shelf_new(E_Physics *p, E_Shelf *es)
{
   EPhysics_Body *eb;
   E_Physics_Shelf *eps;

   eps = E_NEW(E_Physics_Shelf, 1);
   eps->p = p;
   eps->es = es;
   if (!_physics_mod->conf->ignore_shelves)
     {
        if (_physics_mod->conf->shelf.disable_move)
          {
             eps->body = eb = ephysics_body_box_add(p->world);
             ephysics_body_evas_object_set(eb, es->o_base, EINA_TRUE);
             ephysics_body_linear_movement_enable_set(eb, EINA_FALSE, EINA_FALSE);
             ephysics_body_mass_set(eb, 50000);
          }
        else
          {
             eps->body = eb = ephysics_body_box_add(p->world);
             ephysics_body_evas_object_set(eb, es->o_base, EINA_TRUE);
             if (es->cfg->overlap || es->cfg->autohide)
               ephysics_body_mass_set(eb, 0);
             else
               ephysics_body_mass_set(eb, 50000);
             if (es->cfg->popup && (!_physics_mod->conf->shelf.disable_rotate))
               ephysics_body_rotation_on_z_axis_enable_set(eb, EINA_FALSE);
          }
     }
   p->shelves = eina_inlist_append(p->shelves, EINA_INLIST_GET(eps));
}

//////////////////////////////////////////////////////////////////////////


static Eina_Bool
_e_mod_physics_bd_property(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Border_Property *ev = event;
   E_Physics_Win *pw = _e_mod_physics_win_find(ev->border->client.win);
   if (pw->maximize == ev->border->maximized) return ECORE_CALLBACK_PASS_ON;

   if (ev->border->maximized)
     _e_mod_physics_win_del(pw);
   else
     {
        E_Physics *p = _e_mod_physics_find(ev->border->zone->container->manager->root);
        pw = _e_mod_physics_win_add(p, ev->border);
        if (!pw) return ECORE_CALLBACK_PASS_ON;
        _e_mod_physics_win_configure(pw, pw->bd->x, pw->bd->y,
                                     pw->bd->w, pw->bd->h,
                                     ecore_x_window_border_width_get(pw->bd->client.win));
     }

   pw->maximize = ev->border->maximized;
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_physics_bd_fullscreen(void *data __UNUSED__, int type, void *event)
{
   E_Event_Border_Fullscreen *ev = event;
   E_Physics_Win *pw = _e_mod_physics_win_find(ev->border->client.win);
   if (type == E_EVENT_BORDER_FULLSCREEN)
     {
        if (pw)
          _e_mod_physics_win_del(pw);
     }
   else
     {
        E_Physics *p = _e_mod_physics_find(ev->border->zone->container->manager->root);
        if (!pw)
          {
             pw = _e_mod_physics_win_add(p, ev->border);
             if (!pw) return ECORE_CALLBACK_PASS_ON;
             _e_mod_physics_win_configure(pw, pw->bd->x, pw->bd->y,
                                          pw->bd->w, pw->bd->h,
                                          ecore_x_window_border_width_get(pw->bd->client.win));
          }
     }
   return ECORE_CALLBACK_PASS_ON;
}


static Eina_Bool
_e_mod_physics_bd_add(void *data __UNUSED__,
                      int type   __UNUSED__,
                      void *event)
{
   E_Event_Border_Add *ev = event;
   DBG("PHYS: NEW %d\n", ev->border->client.win);
   E_Physics *p = _e_mod_physics_find(ev->border->zone->container->manager->root);
   E_Physics_Win *pw = _e_mod_physics_win_find(ev->border->client.win);
   if (pw) return ECORE_CALLBACK_PASS_ON;
   pw = _e_mod_physics_win_add(p, ev->border);
   if (pw) _e_mod_physics_win_configure(pw, ev->border->x, ev->border->y, ev->border->w, ev->border->h,
                                        ecore_x_window_border_width_get(ev->border->client.win));
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_physics_bd_del(void *data __UNUSED__,
                      int type   __UNUSED__,
                      void *event)
{
   E_Event_Border_Add *ev = event;
   DBG("PHYS: DEL %d\n", ev->border->client.win);
   E_Physics_Win *pw = _e_mod_physics_win_find(ev->border->client.win);
   if (!pw) return ECORE_CALLBACK_PASS_ON;
   _e_mod_physics_win_del(pw);
   return ECORE_CALLBACK_PASS_ON;
}

/*
static Eina_Bool
_e_mod_physics_reparent(void *data __UNUSED__,
                        int type   __UNUSED__,
                        void *event)
{
   Ecore_X_Event_Window_Reparent *ev = event;
   E_Physics_Win *pw = _e_mod_physics_win_find(ev->win);
   if (!pw) return ECORE_CALLBACK_PASS_ON;
   if (ev->parent != pw->p->man->root)
     _e_mod_physics_win_del(pw);
   return ECORE_CALLBACK_PASS_ON;
}
*/
static Eina_Bool
_e_mod_physics_configure(void *data __UNUSED__,
                         int type   __UNUSED__,
                         void *event)
{
   Ecore_X_Event_Window_Configure *ev = event;
   E_Physics_Win *pw = _e_mod_physics_win_find(ev->win);
   if (!pw) return ECORE_CALLBACK_PASS_ON;
/* TODO: stacking 
   if (ev->abovewin == 0)
     {
        if (EINA_INLIST_GET(pw)->prev) _e_mod_physics_win_lower(pw);
     }
   else
     {
        E_Physics_Win *cw2 = _e_mod_physics_win_find(ev->abovewin);

        if (cw2)
          {
             E_Physics_Win *cw3 = (E_Physics_Win *)(EINA_INLIST_GET(pw)->prev);

             if (cw3 != cw2) _e_mod_physics_win_raise_above(pw, cw2);
          }
     }
*/
   if (!((pw->x == ev->x) && (pw->y == ev->y) &&
         (pw->w == ev->w) && (pw->h == ev->h) &&
         (pw->border == ev->border)))
     {
        _e_mod_physics_win_configure(pw, ev->x, ev->y, ev->w, ev->h, ev->border);
     }
   return ECORE_CALLBACK_PASS_ON;
}
#if 0
static Eina_Bool
_e_mod_physics_stack(void *data __UNUSED__,
                     int type   __UNUSED__,
                     void *event)
{
   Ecore_X_Event_Window_Stack *ev = event;
   E_Physics_Win *pw = _e_mod_physics_win_find(ev->win);
   if (!pw) return ECORE_CALLBACK_PASS_ON;
   /* TODO
   if (ev->detail == ECORE_X_WINDOW_STACK_ABOVE) _e_mod_physics_win_raise(pw);
   else _e_mod_physics_win_lower(pw);
   */
   return ECORE_CALLBACK_PASS_ON;
}
#endif
static Eina_Bool
_e_mod_physics_randr(void *data       __UNUSED__,
                     int type         __UNUSED__,
                     __UNUSED__ void *event __UNUSED__)
{
   Eina_List *l;
   E_Physics *p;

   EINA_LIST_FOREACH(physicists, l, p)
     ephysics_world_render_geometry_set(p->world, 0, 0, p->man->w, p->man->h);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_physics_bd_resize(void *data __UNUSED__,
                       int type   __UNUSED__,
                       void *event)
{
   E_Event_Border_Move *ev = event;
   E_Physics_Win *pw = _e_mod_physics_win_find(ev->border->client.win);
   if (!pw) return ECORE_CALLBACK_PASS_ON;
   if (!pw->visible) return ECORE_CALLBACK_PASS_ON;
   pw->show_ready = 0;
   _e_mod_physics_win_configure(pw, pw->x, pw->y, ev->border->w, ev->border->h, pw->border);
   pw->show_ready = 1;
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_physics_shelf_add(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Shelf *ev = event;
   E_Physics *p;
   Eina_List *l;

   EINA_LIST_FOREACH(physicists, l, p)
     if (p->man == ev->shelf->zone->container->manager)
       {
          _e_mod_physics_shelf_new(p, ev->shelf);
          break;
       }
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_e_mod_physics_shelf_del(void *data __UNUSED__, int type __UNUSED__, void *event)
{
   E_Event_Shelf *ev = event;
   E_Physics *p;
   Eina_List *l;

   EINA_LIST_FOREACH(physicists, l, p)
     if (p->man == ev->shelf->zone->container->manager)
       {
          _e_mod_physics_shelf_free(p, ev->shelf);
          break;
       }
   return ECORE_CALLBACK_RENEW;

   
}

static Eina_Bool
_e_mod_physics_remember_update(void *data __UNUSED__, int type   __UNUSED__, void *event)
{
   E_Event_Remember_Update *ev = event;
   E_Border *bd = ev->border;
   E_Physics_Win *pw = _e_mod_physics_win_find(bd->client.win);
   if (!pw) return ECORE_CALLBACK_PASS_ON;
    _e_mod_physics_win_mass_set(pw);
   return ECORE_CALLBACK_PASS_ON;
}
/*
static Eina_Bool
_e_mod_physics_bd_move(void *data __UNUSED__,
                       int type   __UNUSED__,
                       void *event)
{
   E_Event_Border_Move *ev = event;
   E_Physics_Win *pw = _e_mod_physics_win_find(ev->border->client.win);
   if (!pw) return ECORE_CALLBACK_PASS_ON;
   if (!pw->visible) return ECORE_CALLBACK_PASS_ON;
   _e_mod_physics_win_configure(pw, pw->x, pw->y, ev->border->w, ev->border->h, pw->border);
  
   return ECORE_CALLBACK_PASS_ON;
}
*/
static Eina_Bool
_e_mod_physics_bd_show(void *data __UNUSED__,
                       int type   __UNUSED__,
                       void *event)
{
   E_Event_Border_Hide *ev = event;
   E_Physics_Win *pw = _e_mod_physics_win_find(ev->border->client.win);
   if (!pw) return ECORE_CALLBACK_PASS_ON;
   _e_mod_physics_win_show(pw);
   _e_mod_physics_win_configure(pw, pw->bd->x, pw->bd->y, pw->bd->w, pw->bd->h, pw->border);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_physics_bd_hide(void *data __UNUSED__,
                       int type   __UNUSED__,
                       void *event)
{
   E_Event_Border_Hide *ev = event;
   E_Physics_Win *pw = _e_mod_physics_win_find(ev->border->client.win);
   if (!pw) return ECORE_CALLBACK_PASS_ON;
   _e_mod_physics_win_hide(pw);
   return ECORE_CALLBACK_PASS_ON;
}
#if 0
static Eina_Bool
_e_mod_physics_bd_iconify(void *data __UNUSED__,
                          int type   __UNUSED__,
                          void *event)
{
   E_Event_Border_Iconify *ev = event;
   E_Physics_Win *pw = _e_mod_physics_win_find(ev->border->client.win);
   if (!pw) return ECORE_CALLBACK_PASS_ON;
   // fimxe: special iconfiy anim
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_physics_bd_uniconify(void *data __UNUSED__,
                            int type   __UNUSED__,
                            void *event)
{
   E_Event_Border_Uniconify *ev = event;
   E_Physics_Win *pw = _e_mod_physics_win_find(ev->border->client.win);
   if (!pw) return ECORE_CALLBACK_PASS_ON;
   // fimxe: special uniconfiy anim
   return ECORE_CALLBACK_PASS_ON;
}
#endif

static E_Physics *
_e_mod_physics_add(E_Manager *man)
{
   E_Physics *p;
   EPhysics_Body *bound;
   Eina_List *l;
   E_Border *bd;
   E_Shelf *es;

   p = calloc(1, sizeof(E_Physics));
   if (!p) return NULL;
   p->man = man;
   p->world = ephysics_world_new();
   /* TODO: world per zone */
   DBG("PHYS: world++ || %dx%d\n", man->w, man->h);
   ephysics_world_render_geometry_set(p->world, 0, 0, man->w, man->h);
   ephysics_world_gravity_set(p->world, 0, _physics_mod->conf->gravity);

   bound = ephysics_body_left_boundary_add(p->world);
   ephysics_body_restitution_set(bound, 1);
   ephysics_body_friction_set(bound, 3);
   bound = ephysics_body_right_boundary_add(p->world);
   ephysics_body_restitution_set(bound, 1);
   ephysics_body_friction_set(bound, 3);
   bound = ephysics_body_top_boundary_add(p->world);
   ephysics_body_restitution_set(bound, 1);
   ephysics_body_friction_set(bound, 3);
   bound = ephysics_body_bottom_boundary_add(p->world);
   ephysics_body_restitution_set(bound, 1);
   ephysics_body_friction_set(bound, 3);

   EINA_LIST_FOREACH(e_border_client_list(), l, bd)
     {
        E_Physics_Win *pw;
        int border;

        pw = _e_mod_physics_win_add(p, bd);
        if (!pw) continue;
        border = ecore_x_window_border_width_get(bd->client.win);
        _e_mod_physics_win_configure(pw, bd->x, bd->y, bd->w, bd->h, border);
     }
   l = e_shelf_list_all();
   EINA_LIST_FREE(l, es)
     _e_mod_physics_shelf_new(p, es);

   return p;
}

static void
_e_mod_physics_del(E_Physics *p)
{
   E_Physics_Win *pw;
   Eina_Inlist *l;
   E_Physics_Shelf *eps;

   while (p->wins)
     {
        pw = (E_Physics_Win *)(p->wins);
        _e_mod_physics_win_del(pw);
     }
   if (p->world)
     ephysics_world_del(p->world);
   EINA_INLIST_FOREACH_SAFE(p->shelves, l, eps)
     {
        free(eps);
     }
   free(p);
}

static void
_e_mod_physics_win_update_cb(E_Physics_Win *pw, EPhysics_Body *body, void *event_info __UNUSED__)
{
   //DBG("PHYS: TICKER %d\n", pw->win);
   if (pw->moving && (pw->started < _physics_mod->conf->delay))
     {
        pw->show_ready = 0;
        _e_mod_physics_win_configure(pw, pw->x, pw->y, pw->w, pw->h, pw->border);
        return;
     }
   ephysics_body_geometry_get(body, &pw->ix, &pw->iy, NULL, NULL);
   e_border_move(pw->bd, pw->ix, pw->iy);
}

//////////////////////////////////////////////////////////////////////////

void
e_mod_physics_mass_update(void)
{
   Eina_List *l;
   E_Physics *p;
   E_Physics_Win *pw;
   EINA_LIST_FOREACH(physicists, l, p)
     EINA_INLIST_FOREACH(p->wins, pw)
       _e_mod_physics_win_mass_set(pw);
}

Eina_Bool
e_mod_physics_init(void)
{
   Eina_List *l;
   E_Manager *man;

   borders = eina_hash_string_superfast_new(NULL);

//   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_REPARENT, _e_mod_physics_reparent, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_CONFIGURE, _e_mod_physics_configure, NULL));
   //handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_STACK, _e_mod_physics_stack, NULL));

   if (_physics_mod->conf->ignore_maximized)
     handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_PROPERTY, _e_mod_physics_bd_property, NULL));
   if (_physics_mod->conf->ignore_fullscreen)
     {
        handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_FULLSCREEN, _e_mod_physics_bd_fullscreen, NULL));
        handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_UNFULLSCREEN, _e_mod_physics_bd_fullscreen, NULL));
     }
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_CONTAINER_RESIZE, _e_mod_physics_randr, NULL));

   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_ADD, _e_mod_physics_bd_add, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_REMOVE, _e_mod_physics_bd_del, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_SHOW, _e_mod_physics_bd_show, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_HIDE, _e_mod_physics_bd_hide, NULL));
//   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_MOVE, _e_mod_physics_bd_move, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_RESIZE, _e_mod_physics_bd_resize, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_REMEMBER_UPDATE, _e_mod_physics_remember_update, NULL));

   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_SHELF_ADD, _e_mod_physics_shelf_add, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_SHELF_DEL, _e_mod_physics_shelf_del, NULL));

#if 0
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_ICONIFY, _e_mod_physics_bd_iconify, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(E_EVENT_BORDER_UNICONIFY, _e_mod_physics_bd_uniconify, NULL));
#endif
   EINA_LIST_FOREACH (e_manager_list(), l, man)
     {
        E_Physics *p;

        p = _e_mod_physics_add(man);
        if (p) physicists = eina_list_append(physicists, p);
     }

   ecore_x_sync();

   return 1;
}

void
e_mod_physics_shutdown(void)
{
   E_Physics *p;

   EINA_LIST_FREE (physicists, p)
     _e_mod_physics_del(p);

   E_FREE_LIST(handlers, ecore_event_handler_del);

   if (borders) eina_hash_free(borders);
   borders = NULL;
}

