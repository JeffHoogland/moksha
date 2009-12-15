#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"
#include "e_mod_layout.h"
#include "e_mod_layout_illume.h"
#include "e_kbd.h"

/* local function prototypes */
static void _border_resize_fx(E_Border *bd, int bx, int by, int bw, int bh);
static void _border_add(E_Border *bd);
static void _border_del(E_Border *bd);
static void _border_focus_in(E_Border *bd);
static void _border_focus_out(E_Border *bd);
static void _border_activate(E_Border *bd);
static void _zone_layout(E_Zone *z);
static void _zone_layout_single(E_Border *bd);
static void _zone_layout_dual(E_Border *bd);
static void _zone_layout_dual_top(E_Border *bd);
static void _zone_layout_dual_left(E_Border *bd);
static void _zone_move_resize(E_Zone *z);

/* local variables */
static int shelfsize = 0;
static int kbdsize = 0;
static int panelsize = 0;

const Illume_Layout_Mode laymode = 
{
   "illume", "Illume", 
     _border_add, _border_del, 
     _border_focus_in, _border_focus_out, 
     _zone_layout, _zone_move_resize, 
     _border_activate
};

/* public functions */
void 
illume_layout_illume_init(void) 
{
   illume_layout_mode_register(&laymode);
}

void 
illume_layout_illume_shutdown(void) 
{
   illume_layout_mode_unregister(&laymode);
}

/* local functions */
static void 
_border_resize_fx(E_Border *bd, int bx, int by, int bw, int bh) 
{
   /* CONVENIENCE FUNCTION TO REMOVE DUPLICATED CODE */

   if ((bd->need_fullscreen) || (bd->fullscreen)) 
     {
        if ((bd->w != bw) || (bd->h != bh)) 
          {
             bd->w = bw;
             bd->h = bh;
             bd->client.w = bw;
             bd->client.h = bh;
             bd->changes.size = 1;
          }
        if ((bd->x != bx) || (bd->y != by)) 
          {
             bd->x = bx;
             bd->y = by;
             bd->changes.pos = 1;
          }
     }
   else 
     {
        if ((bd->w != bw) || (bd->h != bh))
          e_border_resize(bd, bw, bh);
        if ((bd->x != bx) || (bd->y != by))
          e_border_fx_offset(bd, bx, by);
     }
}

static void 
_border_add(E_Border *bd) 
{
   /* HANDLE A BORDER BEING ADDED */

   int conform;

   /* skip new clients and invisible borders */
   if ((bd->new_client) || (!bd->visible)) return;

   /* check if this border is conformant */
   conform = illume_border_is_conformant(bd);

   /* is this a fullscreen border ? */
   if ((bd->need_fullscreen) || (bd->fullscreen)) 
     {
        E_Border *b;

        if (bd->layer != 115) e_border_layer_set(bd, 115);

        /* we lock stacking so that the keyboard does not get put 
         * under the window (if it's needed) */
        bd->lock_user_stacking = 1;

        /* conformant fullscreen borders just hide bottom panel */
        b = illume_border_bottom_panel_get();
        if (b) e_border_fx_offset(b, 0, -panelsize);

        /* for non-conformant fullscreen borders, 
         * we hide top shelf and bottom panel in all cases */
        if (!conform) 
          {
             b = illume_border_top_shelf_get();
             if (b) e_border_fx_offset(b, 0, -shelfsize);
          }
     }
   if (conform) 
     {
        if (bd->layer != 110) e_border_layer_set(bd, 110);
     }

   /* we lock stacking so that the keyboard does not get put 
    * under the window (if it's needed) */
   if (conform) bd->lock_user_stacking = 1;

   /* only set focus if border accepts it and it's not locked out */
   if ((bd->client.icccm.accepts_focus) && (bd->client.icccm.take_focus) 
       && (!bd->lock_focus_out))
     e_border_focus_set(bd, 1, 1);
}

static void 
_border_del(E_Border *bd) 
{
   /* HANDLE A BORDER BEING DELETED */

   if ((bd->need_fullscreen) || (bd->fullscreen)) 
     {
        E_Border *b;

        /* conformant fullscreen borders just get bottom panel shown */
        b = illume_border_bottom_panel_get();
        if (b) e_border_fx_offset(b, 0, 0);

        /* for non-conformant fullscreen borders, 
         * we show top shelf and bottom panel in all cases */
        if (!illume_border_is_conformant(bd)) 
          {
             b = illume_border_top_shelf_get();
             if (b) e_border_fx_offset(b, 0, 0);
          }
     }
}

static void 
_border_focus_in(E_Border *bd) 
{
   /* do something if focus enters a window */
}

static void 
_border_focus_out(E_Border *bd) 
{
   /* do something if focus exits a window */
}

static void 
_border_activate(E_Border *bd) 
{
   /* HANDLE A BORDER BEING ACTIVATED */

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

static void 
_zone_layout(E_Zone *z) 
{
   Eina_List *l, *borders;
   E_Border *bd;

   /* ACTUALLY APPLY THE SIZING, POSITIONING AND LAYERING */

   /* grab list of borders */
   borders = e_border_client_list();
   EINA_LIST_FOREACH(borders, l, bd) 
     {
        int mh;

        /* skip border if: zone not the same, a new client, or invisible */
        if ((bd->zone != z) || (bd->new_client) || (!bd->visible)) continue;

        /* check for special windows to get their size(s) */
        illume_border_min_get(bd, NULL, &mh);
        if (illume_border_is_top_shelf(bd)) 
          {
             if (shelfsize < mh) shelfsize = mh;
          }
        else if (illume_border_is_bottom_panel(bd)) 
          {
             if (panelsize < mh) panelsize = mh;
          }
        else if (illume_border_is_keyboard(bd)) 
          {
             if (kbdsize < mh) kbdsize = mh;
          }
     }

   /* grab list of borders */
   borders = e_border_client_list();
   EINA_LIST_FOREACH(borders, l, bd) 
     {
        /* skip border if: zone not the same, a new client, or invisible */
        if ((bd->zone != z) || (bd->new_client) || (!bd->visible)) continue;

        /* trap 'special' windows as they need special treatment */
        if (illume_border_is_top_shelf(bd)) 
          {
             _border_resize_fx(bd, z->x, z->y, z->w, shelfsize);
             e_border_stick(bd);
             if (bd->layer != 200) e_border_layer_set(bd, 200);
          }
        else if (illume_border_is_bottom_panel(bd)) 
          {
             _border_resize_fx(bd, z->x, (z->y + z->h - panelsize), 
                               z->w, panelsize);
             e_border_stick(bd);
             if (bd->layer != 100) e_border_layer_set(bd, 100);
          }
        else if (illume_border_is_keyboard(bd)) 
          {
             _border_resize_fx(bd, z->x, (z->y + z->h - kbdsize), 
                               z->w, kbdsize);
             e_border_stick(bd);
             if (bd->layer != 150) e_border_layer_set(bd, 150);
          }
        else if (illume_border_is_dialog(bd)) 
          {
             int mw, mh;

             illume_border_min_get(bd, &mw, &mh);
             if (mh > z->h) mh = z->h;
             if (mw > z->w) mw = z->w;
             _border_resize_fx(bd, (z->x + ((z->w - mw) / 2)), 
                               (z->y + ((z->h - mh) / 2)), mw, mh);
             if (bd->layer != 120) e_border_layer_set(bd, 120);
          }
        else 
          {
             /* normal border, handle layout based on policy mode */
             if (il_cfg->policy.mode.dual) _zone_layout_dual(bd);
             else _zone_layout_single(bd);
          }
     }
}

static void 
_zone_layout_single(E_Border *bd) 
{
   int kx, ky, kw, kh, ss, ps;

   /* grab the 'safe' region. Safe region is space not occupied by keyboard */
   e_kbd_safe_app_region_get(bd->zone, &kx, &ky, &kw, &kh);
   if (!illume_border_is_conformant(bd)) 
     {
        if (!((bd->need_fullscreen) || (bd->fullscreen))) 
          {
             if (kh >= bd->zone->h) ps = panelsize;
             ss = shelfsize;
          }
     }
   else 
     kh = bd->zone->h;
   _border_resize_fx(bd, kx, (ky + ss), kw, (kh - ss - ps));
}

static void 
_zone_layout_dual(E_Border *bd) 
{
   if (il_cfg->policy.mode.side == 0) _zone_layout_dual_top(bd);
   else if (il_cfg->policy.mode.side == 1) _zone_layout_dual_left(bd);
}

static void 
_zone_layout_dual_top(E_Border *bd) 
{
   int kx, ky, kw, kh, ss, ps;
   int count, conform;

   /* get count of valid borders */
   count = illume_border_valid_count_get();

   /* fetch if this border is conformant */
   conform = illume_border_is_conformant(bd);

   /* grab the 'safe' region. Safe region is space not occupied by keyboard */
   e_kbd_safe_app_region_get(bd->zone, &kx, &ky, &kw, &kh);
   if (!conform) 
     {
        /* if the border is not conformant and doesn't need fullscreen, then 
         * we account for shelf & panel sizes */
        if (!((bd->need_fullscreen) || (bd->fullscreen))) 
          {
             if (kh >= bd->zone->h) ps = panelsize;
             ss = shelfsize;
          }
     }

   /* if there are no other borders, than give this one all available space */
   if (count < 2) 
     _border_resize_fx(bd, kx, (ky + ss), kw, (kh - ss - ps));
   else 
     {
        E_Border *b;
        int bh, by;

        /* more than one valid border */

        /* grab the border at this location */
        b = illume_border_at_xy_get(kx, shelfsize);

        if ((b) && (bd != b)) 
          {
             /* we have a border there, and it's not the current one */
             if (!illume_border_is_conformant(b)) 
               {
                  /* border in this location is not conformant */
                  bh = ((kh - ss - ps) / 2);
                  by = (b->fx.y + b->h);
               }
             else 
               {
                  /* border there is conformant */
                  if (conform) 
                    {
                       /* if current border is conformant, divide zone in half */
                       bh = ((bd->zone->h - ss) / 2);
                       by = by + bh;
                    }
                  else 
                    {
                       /* current border is not conformant */
                       by = (b->fx.y + b->h);
                       bh = (kh - by - ps);
                    }
               }
          }
        else if (b) 
          {
             /* border at this location and it's the current border */
             by = bd->fx.y;
             if (conform)
               bh = ((kh - ss - ps) / 2);
             else
               bh = ((kh - ss - ps) / 2);
          }
        else 
          {
             /* no border at this location */
             b = illume_border_valid_border_get();
             if (conform) 
               by = ky;
             else 
               by = (ky + ss);
             bh = (kh - b->h);
          }
        _border_resize_fx(bd, kx, by, kw, bh);
     }
}

static void 
_zone_layout_dual_left(E_Border *bd) 
{
   int kx, ky, kw, kh, ss, ps;
   int count, conform;

   /* get count of valid borders */
   count = illume_border_valid_count_get();

   /* fetch if this border is conformant */
   conform = illume_border_is_conformant(bd);

   /* grab the 'safe' region. Safe region is space not occupied by keyboard */
   e_kbd_safe_app_region_get(bd->zone, &kx, &ky, &kw, &kh);
   if (!conform) 
     {
        /* if the border is not conformant and doesn't need fullscreen, then 
         * we account for shelf & panel sizes */
        if (!((bd->need_fullscreen) || (bd->fullscreen))) 
          {
             if (kh >= bd->zone->h) ps = panelsize;
             ss = shelfsize;
          }
     }

   /* if there are no other borders, than give this one all available space */
   if (count < 2) 
     _border_resize_fx(bd, kx, (ky + ss), kw, (kh - ss - ps));
   else 
     {
        E_Border *b;
        int bx, by, bw, bh;

        /* more than one valid border */
        bx = kx;
        by = (ky + ss);
        bw = kw;
        bh = (kh - ss - ps);
        /* grab the border at this location */
        b = illume_border_at_xy_get(kx, shelfsize);

        if ((b) && (bd != b)) 
          {
             /* we have a border there, and it's not the current one */
             if (!illume_border_is_conformant(b)) 
               {
                  /* border in this location is not conformant */
                  bw = (kw / 2);
                  bx = (b->fx.x + b->w);
               }
             else 
               {
                  /* border there is conformant */
                  if (conform) 
                    {
                       /* if current border is conformant, divide zone in half */
                       bw = (bd->zone->w / 2);
                       bx = bx + bw;
                    }
                  else 
                    {
                       /* current border is not conformant */
                       bx = (b->fx.x + b->w);
                       bw = (kw - bx);
                    }
               }
          }
        else if (b) 
          {
             /* border at this location and it's the current border */
             bx = bd->fx.x;
             bw = (kw / 2);
          }
        else 
          {
             /* no border at this location */
             b = illume_border_valid_border_get();
             bx = kx;
             bw = (kw - b->w);
          }
        _border_resize_fx(bd, bx, by, bw, bh);
     }
}

static void 
_zone_move_resize(E_Zone *z) 
{
   /* the zone was moved or resized - re-configure all windows in this zone */
   _zone_layout(z);
}
