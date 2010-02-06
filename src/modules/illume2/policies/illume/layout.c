#include "E_Illume.h"
#include "layout.h"

/* local function prototypes */
static void _zone_layout_border_move(E_Border *bd, int x, int y);
static void _zone_layout_border_resize(E_Border *bd, int w, int h);
static void _zone_layout_single(E_Border *bd);
static void _zone_layout_dual(E_Border *bd);
static void _zone_layout_dual_top(E_Border *bd);
static void _zone_layout_dual_top_custom(E_Border *bd);
static void _zone_layout_dual_left(E_Border *bd);

/* local variables */
static int shelfsize = 0;
static int kbdsize = 0;
static int panelsize = 0;

void 
_layout_border_add(E_Border *bd) 
{
   if (!bd) return;
   if ((bd->new_client) || (!bd->visible)) return;
   if ((bd->need_fullscreen) || (bd->fullscreen)) 
     {
        E_Border *b;

        b = e_illume_border_top_shelf_get(bd->zone);
        if (b) e_border_hide(b, 2);
        if (bd->layer != IL_FULLSCREEN_LAYER)
          e_border_layer_set(bd, IL_FULLSCREEN_LAYER);
        bd->lock_user_stacking = 1;
     }
   else if (e_illume_border_is_conformant(bd)) 
     {
        if (bd->layer != IL_CONFORM_LAYER)
          e_border_layer_set(bd, IL_CONFORM_LAYER);
        bd->lock_user_stacking = 1;
     }
   if ((bd->client.icccm.accepts_focus) && (bd->client.icccm.take_focus) 
       && (!bd->lock_focus_out) && (!bd->focused))
     e_border_focus_set(bd, 1, 1);
}

void 
_layout_border_del(E_Border *bd) 
{
   if (!bd) return;

   /* Do something if a border gets removed */
   if ((bd->need_fullscreen) || (bd->fullscreen)) 
     {
        E_Border *b;

        b = e_illume_border_top_shelf_get(bd->zone);
        if (b) e_border_show(b);
     }
}

void 
_layout_border_focus_in(E_Border *bd) 
{
   /* Do something if focus enters a window */
}

void 
_layout_border_focus_out(E_Border *bd) 
{
   /* Do something if focus exits a window */
}

void 
_layout_border_activate(E_Border *bd) 
{
   E_Border *b;

   /* HANDLE A BORDER BEING ACTIVATED */

   if ((!bd) || (bd->stolen)) return;

   if (e_illume_border_is_conformant(bd))
     {
        b = e_illume_border_bottom_panel_get(bd->zone);
        if (b) e_border_hide(b, 2);
     }
   else
     {
        b = e_illume_border_bottom_panel_get(bd->zone);
        if (b) e_border_show(b);
     }
   
   /* only set focus if border accepts it and it's not locked out */
   if (((!bd->client.icccm.accepts_focus) && (!bd->client.icccm.take_focus)) ||
       (bd->lock_focus_out))
     return;

   /* if the border is not focused, check focus settings */
   if ((bd) && (!bd->focused)) 
     {
        if ((e_config->focus_setting == E_FOCUS_NEW_WINDOW) || 
            ((bd->parent) && 
             ((e_config->focus_setting == E_FOCUS_NEW_DIALOG) || 
              ((bd->parent->focused) && 
               (e_config->focus_setting == E_FOCUS_NEW_DIALOG_IF_OWNER_FOCUSED))))) 
          {
             if (bd->iconic) 
               {
                  /* if it's iconic, then uniconify */
                  if (!bd->lock_user_iconify) e_border_uniconify(bd);
               }
             /* if we can, raise the border */
             if (!bd->lock_user_stacking) e_border_raise(bd);
             /* if we can, focus the border */
             if (!bd->lock_focus_out) e_border_focus_set(bd, 1, 1);
          }
     }
}

void 
_layout_border_property_change(E_Border *bd, Ecore_X_Event_Window_Property *event) 
{
   E_Border *ind;

   if (event->atom != ECORE_X_ATOM_NET_WM_STATE) return;
   if ((!bd->client.icccm.name) || (!bd->client.icccm.class) || 
       (bd->stolen)) 
     return;
   if (!(ind = e_illume_border_top_shelf_get(bd->zone))) return;
   if ((bd->fullscreen) || (bd->need_fullscreen)) 
     {
        if (ind) e_border_hide(ind, 2);
     }
   else 
     {
        if (ind) e_border_show(ind);
     }
}

void 
_layout_zone_layout(E_Zone *zone) 
{
   E_Illume_Config_Zone *cfg_zone;
   Eina_List *l;
   E_Border *bd;

   if (!zone) return;

   cfg_zone = e_illume_zone_config_get(zone->id);
   if (!cfg_zone) return;

   EINA_LIST_FOREACH(e_border_client_list(), l, bd) 
     {
        int mh;

        if ((bd->zone != zone) || (bd->new_client) || (!bd->visible)) continue;
        if (e_illume_border_is_top_shelf(bd)) 
          {
             e_illume_border_min_get(bd, NULL, &mh);
             if (shelfsize < mh) shelfsize = mh;
             if (!bd->client.illume.drag.drag) 
               {
                  if ((bd->w != zone->w) || (bd->h != shelfsize))
                    _zone_layout_border_resize(bd, zone->w, shelfsize);
                  if (!cfg_zone->mode.dual) 
                    {
                       if ((bd->x != zone->x) || (bd->y != zone->y)) 
                         {
                            _zone_layout_border_move(bd, zone->x, zone->y);
                            ecore_x_e_illume_quickpanel_position_update_send(bd->client.win);
                         }
                    }
                  else 
                    {
                       if (cfg_zone->mode.side == 0) 
                         {
                            if (bd->x != zone->x) 
                              _zone_layout_border_move(bd, zone->x, bd->y);
                         }
                       else 
                         {
                            if ((bd->x != zone->x) || (bd->y != zone->y)) 
                              {
                                 _zone_layout_border_move(bd, zone->x, zone->y);
                                 ecore_x_e_illume_quickpanel_position_update_send(bd->client.win);
                              }
                         }
                    }
               }
             e_border_stick(bd);
             if (bd->layer != IL_TOP_SHELF_LAYER)
               e_border_layer_set(bd, IL_TOP_SHELF_LAYER);
          }
        else if (e_illume_border_is_bottom_panel(bd)) 
          {
             e_illume_border_min_get(bd, NULL, &mh);
             if (panelsize < mh) panelsize = mh;
             if (!bd->client.illume.drag.drag) 
               {
                  if ((bd->w != zone->w) || (bd->h != panelsize))
                    _zone_layout_border_resize(bd, zone->w, panelsize);
                  if ((bd->x != zone->x) || 
                      (bd->y != (zone->y + zone->h - panelsize))) 
                    _zone_layout_border_move(bd, zone->x, 
                                             (zone->y + zone->h - panelsize));
               }
             e_border_stick(bd);
             if (bd->layer != IL_BOTTOM_PANEL_LAYER)
               e_border_layer_set(bd, IL_BOTTOM_PANEL_LAYER);
          }
        else if (e_illume_border_is_keyboard(bd)) 
          {
             e_illume_border_min_get(bd, NULL, &mh);
             if (kbdsize < mh) kbdsize = mh;
             if ((bd->w != zone->w) || (bd->h != kbdsize))
               _zone_layout_border_resize(bd, zone->w, kbdsize);
             if ((bd->x != zone->x) || 
                 (bd->y != (zone->y + zone->h - kbdsize))) 
               _zone_layout_border_move(bd, zone->x, 
                                        (zone->y + zone->h - kbdsize));
             if (bd->layer != IL_KEYBOARD_LAYER)
               e_border_layer_set(bd, IL_KEYBOARD_LAYER);
          }
        else if (e_illume_border_is_dialog(bd)) 
          {
             int mw, nx, ny;

             e_illume_border_min_get(bd, &mw, &mh);
             if (mw > zone->w) mw = zone->w;
             if (mh > zone->h) mh = zone->h;
             nx = (zone->x + ((zone->w - mw) / 2));
             ny = (zone->y + ((zone->h - mh) / 2));
             if ((bd->w != mw) || (bd->h != mh))
               _zone_layout_border_resize(bd, mw, mh);
             if ((bd->x != nx) || (bd->y != ny))
               _zone_layout_border_move(bd, nx, ny);
             if (bd->layer != IL_DIALOG_LAYER)
               e_border_layer_set(bd, IL_DIALOG_LAYER);
          }
        else if (e_illume_border_is_quickpanel(bd)) 
          {
             e_illume_border_min_get(bd, NULL, &mh);
             if ((bd->w != zone->w) || (bd->h != mh))
               _zone_layout_border_resize(bd, zone->w, mh);
             if (bd->layer != IL_QUICKPANEL_LAYER)
               e_border_layer_set(bd, IL_QUICKPANEL_LAYER);
             bd->lock_user_stacking = 1;
          }
        else 
          {
             if (bd->stolen) continue;
             if (cfg_zone->mode.dual) _zone_layout_dual(bd);
             else _zone_layout_single(bd);
             if (e_illume_border_is_conformant(bd)) 
               {
                  if (bd->layer != IL_CONFORM_LAYER)
                    e_border_layer_set(bd, IL_CONFORM_LAYER);
               }
             else if ((bd->fullscreen) || (bd->need_fullscreen))
               {
                  if (bd->layer != IL_FULLSCREEN_LAYER)
                    e_border_layer_set(bd, IL_FULLSCREEN_LAYER);
               }
             else 
               {
                  if (bd->layer != IL_APP_LAYER)
                    e_border_layer_set(bd, IL_APP_LAYER);
               }
          }
     }
}

void 
_layout_zone_move_resize(E_Zone *zone) 
{
   if (!zone) return;
   _layout_zone_layout(zone);
}

void 
_layout_drag_start(E_Border *bd) 
{
   if (!bd) return;
   ecore_x_e_illume_drag_set(bd->client.win, 1);
   ecore_x_e_illume_drag_set(bd->zone->black_win, 1);
}

void 
_layout_drag_end(E_Border *bd) 
{
   if (!bd) return;
   ecore_x_e_illume_drag_set(bd->client.win, 0);
   ecore_x_e_illume_drag_set(bd->zone->black_win, 0);
}

/* local functions */
static void 
_zone_layout_border_move(E_Border *bd, int x, int y) 
{
   if (!bd) return;
   bd->x = x;
   bd->y = y;
   bd->changes.pos = 1;
   bd->changed = 1;
}

static void 
_zone_layout_border_resize(E_Border *bd, int w, int h) 
{
   if (!bd) return;
   bd->w = w;
   bd->h = h;
   bd->client.w = bd->w - (bd->client_inset.l + bd->client_inset.r);
   bd->client.h = bd->h - (bd->client_inset.t + bd->client_inset.b);
   bd->changes.size = 1;
   bd->changed = 1;
}

static void 
_zone_layout_single(E_Border *bd) 
{
   int kx, ky, kw, kh;
   int ss = 0, ps = 0;
   int nh, ny;

   if (!bd) return;
   e_illume_kbd_safe_app_region_get(bd->zone, &kx, &ky, &kw, &kh);
   if (!((bd->need_fullscreen) || (bd->fullscreen))) 
     {
        if (kh >= bd->zone->h) ps = panelsize;
        ss = shelfsize;
     }
   nh = kh - ss - ps;
   ny = ky + ss;
   if (e_illume_border_is_conformant(bd)) 
     {
        nh = bd->zone->h;
        ny = ky;
     }
   if ((bd->w != kw) || (bd->h != nh))
     _zone_layout_border_resize(bd, kw, nh);
   if ((bd->x != kx) || (bd->y != ny))
     _zone_layout_border_move(bd, kx, ny);
}

static void 
_zone_layout_dual(E_Border *bd) 
{
   E_Illume_Config_Zone *cz;

   if (!bd) return;
   cz = e_illume_zone_config_get(bd->zone->id);
   if (!cz) return;
   if (cz->mode.side == 0) 
     {
        int ty;

        e_illume_border_top_shelf_pos_get(bd->zone, NULL, &ty);
        if (ty <= bd->zone->y)
          _zone_layout_dual_top(bd);
        else
          _zone_layout_dual_top_custom(bd);
     }
   else if (cz->mode.side == 1)
     _zone_layout_dual_left(bd);
}

static void 
_zone_layout_dual_top(E_Border *bd) 
{
   int conform;
   int kx, ky, kw, kh;
   int ps = 0, ss = 0;
   int by, bh;
   E_Border *b;

   if (!bd) return;
   conform = e_illume_border_is_conformant(bd);
   e_illume_kbd_safe_app_region_get(bd->zone, &kx, &ky, &kw, &kh);

   if (!((bd->need_fullscreen) || (bd->fullscreen))) 
     {
        if (kh >= bd->zone->h) ps = panelsize;
        ss = shelfsize;
     }

   by = (ky + ss);
   bh = ((kh - ss - ps) / 2);

   b = e_illume_border_at_xy_get(bd->zone, kx, by);
   if ((b) && (bd != b)) 
     {
        if (e_illume_border_is_home(b)) 
          {
             if (conform) 
               {
                  by = ky;
                  bh = bd->zone->h / 2;
               }
             else if (e_illume_border_is_home(bd))
               by = (b->y + b->h);
          }
        else if (e_illume_border_is_conformant(b)) 
          {
             by = (b->y + b->h);
             if (conform) bh = bd->zone->h / 2;
          }
        else 
          {
             by = (b->y + b->h);
             if (conform) bh = bd->zone->h / 2;
          }
     }
   else if (b) 
     {
        if (bd->client.vkbd.state > ECORE_X_VIRTUAL_KEYBOARD_STATE_OFF) 
          {
             by = (ky + ss);
             bh = ((kh - ss - ps) / 2);
          }
     }

   if ((bd->need_fullscreen) || (bd->fullscreen))
     {
        by = ky;
        bh = kh;
     }

   if ((bd->w != kw) || (bd->h != bh))
     _zone_layout_border_resize(bd, kw, bh);
   if ((bd->x != kx) || (bd->y != by))
     _zone_layout_border_move(bd, kx, by);
}

static void 
_zone_layout_dual_top_custom(E_Border *bd) 
{
   int kx, kw, ky, kh;
   int ax, ay, aw, ah;
   int zx, zy, zw, zh;

   if (!bd) return;

   /* grab the 'safe' region. Safe region is space not occupied by keyboard */
   e_illume_kbd_safe_app_region_get(bd->zone, &kx, &ky, &kw, &kh);

   e_illume_border_app1_safe_region_get(bd->zone, &ax, &ay, &aw, &ah);
   e_illume_border_app2_safe_region_get(bd->zone, &zx, &zy, &zw, &zh);

   /* if there are no other borders, than give this one all available space */
   if (e_illume_border_valid_count_get(bd->zone) < 2) 
     {
        if (ah >= zh) 
          {
             zx = ax;
             zy = ax;
             zw = aw;
             zh = ah;
          }
        if ((bd->w != zw) || (bd->h != zh))
          _zone_layout_border_resize(bd, zw, zh);
        if ((bd->x != zx) || (bd->y != zy))
          _zone_layout_border_move(bd, zx, zy);
     }
   else 
     {
        int bh, by;

        bh = ah;
        by = ay;

        if (bd->client.vkbd.state <= ECORE_X_VIRTUAL_KEYBOARD_STATE_OFF) 
          {
             E_Border *bt;

             /* grab the border at the top */
             bt = e_illume_border_at_xy_get(bd->zone, kx, ay);
             if ((bt) && (bd != bt))
               {
                  E_Border *bb;

                  /* have border @ top, check for border @ bottom */
                  bb = e_illume_border_at_xy_get(bd->zone, kx, zy);
                  if ((bb) && (bd != bb)) 
                    {
                       /* have border @ top & bottom; neither is current */

                       /* if top border is !home, check bottom */
                       if (!e_illume_border_is_home(bt)) 
                         {
                            if (e_illume_border_is_home(bb)) 
                              {
                                 bh = zh;
                                 by = zy;
                              }
                            else 
                              {
                                 /* potential hole */
                                 bh = ah;
                                 by = ay;
                              }
                         }
                       else 
                         {
                            bh = ah;
                            by = ay;
                         }
                    }
                  else if (bb) 
                    {
                       bh = zh;
                       by = bd->y;
                    }
                  else 
                    {
                       bh = zh;
                       by = zy;
                    }
               }
             else if (bt) 
               {
                  bh = ah;
                  by = bd->y;
               }
             else 
               {
                  bh = ah;
                  by = ay;
               }
          }

        if ((bd->fullscreen) || (bd->need_fullscreen))
          {
             bh = kh;
             by = ky;
          }
        if ((bd->w != kw) || (bd->h != bh))
          _zone_layout_border_resize(bd, kw, bh);
        if ((bd->x != kx) || (bd->y != by))
          _zone_layout_border_move(bd, kx, by);
     }
}

static void 
_zone_layout_dual_left(E_Border *bd) 
{
   int conform;
   int kx, ky, kw, kh;
   int ps = 0, ss = 0;
   int by, bh, bx, bw;
   E_Border *b;

   if (!bd) return;

   conform = e_illume_border_is_conformant(bd);
   e_illume_kbd_safe_app_region_get(bd->zone, &kx, &ky, &kw, &kh);

   if (!((bd->need_fullscreen) || (bd->fullscreen))) 
     {
        if (kh >= bd->zone->h) ps = panelsize;
        ss = shelfsize;
     }

   bx = kx;
   by = (ky + ss);
   bw = (kw / 2);
   bh = (kh - ss - ps);

   b = e_illume_border_at_xy_get(bd->zone, kx, by);
   if ((b) && (bd != b)) 
     {
        if (e_illume_border_is_home(b)) 
          {
             if (conform) 
               {
                  by = ky;
                  bh = bd->zone->h;
               }
             else if (e_illume_border_is_home(bd))
               bx = (b->x + b->w);
          }
        else if (e_illume_border_is_conformant(b)) 
          {
             bx = (b->x + b->w);
             if (conform) 
               {
                  by = ky;
                  bh = bd->zone->h;
               }
          }
        else 
          {
             bx = (b->x + b->w);
             if (conform) 
               {
                  by = ky;
                  bh = bd->zone->h;
               }
          }
     }
   else if (b) 
     {
        if (bd->client.vkbd.state > ECORE_X_VIRTUAL_KEYBOARD_STATE_OFF) 
          {
             by = (ky + ss);
             bh = (kh - ss - ps);
          }
     }

   if ((bd->need_fullscreen) || (bd->fullscreen))
     {
        bx = kx;
        by = ky;
        bw = kw;
        bh = kh;
     }

   if ((bd->w != bw) || (bd->h != bh))
     _zone_layout_border_resize(bd, bw, bh);
   if ((bd->x != bx) || (bd->y != by))
     _zone_layout_border_move(bd, bx, by);
}
