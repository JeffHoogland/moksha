#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"
#include "e_mod_layout.h"
#include "e_mod_layout_illume.h"
#include "e_kbd.h"

/* local variables */
static int shelfsize = 0;
static int kbdsize = 0;
static int panelsize = 0;

static void
_border_add(E_Border *bd)
{ // handle a border being added
   if (illume_border_is_top_shelf(bd)) return;
   if (illume_border_is_bottom_panel(bd)) return;
   if (illume_border_is_keyboard(bd)) return;
   if ((bd->need_fullscreen) || (bd->fullscreen)) 
     {
        E_Border *b;

        b = illume_border_top_shelf_get();
        if (b) e_border_fx_offset(b, 0, -shelfsize);
        b = illume_border_bottom_panel_get();
        if (b) e_border_fx_offset(b, 0, -panelsize);
     }

   e_border_raise(bd);
   e_border_focus_set(bd, 1, 1);
}

static void
_border_del(E_Border *bd)
{ // handle a border being deleted
   if (illume_border_is_top_shelf(bd)) return;
   if (illume_border_is_bottom_panel(bd)) return;
   if (illume_border_is_keyboard(bd)) return;
   if ((bd->need_fullscreen) || (bd->fullscreen)) 
     {
        E_Border *b;

        b = illume_border_top_shelf_get();
        if (b) e_border_fx_offset(b, 0, 0);
        b = illume_border_bottom_panel_get();
        if (b) e_border_fx_offset(b, 0, 0);
     }
}

static void
_border_focus_in(E_Border *bd)
{ // do something if focus enters a window

}

static void
_border_focus_out(E_Border *bd)
{ // do something if the focus exits a window

}

static void
_zone_layout(E_Zone *z)
{ // borders are in re-layout stage. this is where you move/resize them
   Eina_List *l, *borders;
   E_Border *bd;

   // phase 1. loop through borders to figure out sizes of things
   borders = e_border_client_list();
   EINA_LIST_FOREACH(borders, l, bd)
     {
        int mw, mh;

        if (bd->zone != z) continue; // skip other zones
        if (bd->new_client) continue; // skip new clients 
        if (!bd->visible) continue; // skip invisible

        illume_border_min_get(bd, &mw, &mh);
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

   // phase 2. actually apply the sizing, positioning and layering too
   borders = e_border_client_list();
   EINA_LIST_FOREACH(borders, l, bd)
     {
        int mw, mh;

        if (bd->zone != z) continue; // skip other zones
        if (bd->new_client) continue; // skip new clients 
        if (!bd->visible) continue; // skip invisible

        illume_border_min_get(bd, &mw, &mh);
        if (illume_border_is_top_shelf(bd))
          {
             if ((bd->w != z->w) || (bd->h != shelfsize))
               e_border_resize(bd, z->w, shelfsize);
             if ((bd->x != z->x) || (bd->y != z->y))
               e_border_fx_offset(bd, z->x, z->y);
             e_border_stick(bd);
          }
        else if (illume_border_is_bottom_panel(bd))
          {
             if ((bd->w != z->w) || (bd->h != panelsize))
               e_border_resize(bd, z->w, panelsize);
             if ((bd->x != z->x) || (bd->y != (z->y + z->h - panelsize)))
               e_border_fx_offset(bd, z->x, (z->y + z->h - panelsize));
             e_border_stick(bd);
          }
        else if (illume_border_is_keyboard(bd))
          {
             if ((bd->w != z->w) || (bd->h != kbdsize))
               e_border_resize(bd, z->w, kbdsize);
             if ((bd->x != z->x) || (bd->y != (z->y + z->h - kbdsize)))
               e_border_fx_offset(bd, z->x, (z->y + z->h - kbdsize));
             e_border_stick(bd);
          }
        else if (illume_border_is_dialog(bd))
          {
             if (mh > z->h) mh = z->h;
             if ((bd->w != z->w) || (bd->h != mh))
               e_border_resize(bd, z->w, mh);
             if ((bd->x != z->x) || (bd->y != (z->y + ((z->h - mh) / 2))))
               e_border_fx_offset(bd, z->x, (z->y + ((z->h - mh) / 2)));
          }
        else if ((bd->need_fullscreen) || (bd->fullscreen)) 
          e_border_fullscreen(bd, E_FULLSCREEN_RESIZE);
        else
          {
             if (!il_cfg->policy.mode.dual) 
               {
                  if (illume_border_is_conformant(bd)) 
                    {
                       if ((bd->w != z->w) || (bd->h != z->h))
                         e_border_resize(bd, z->w, z->h);
                       if ((bd->x != z->x) || (bd->y != z->y))
                         e_border_fx_offset(bd, z->x, z->y);
                    }
                  else 
                    {
                       if ((bd->w != z->w) || 
                           (bd->h != (z->h - shelfsize - panelsize)))
                         e_border_resize(bd, z->w, (z->h - shelfsize - panelsize));
                       if ((bd->x != z->x) || (bd->y != (z->y + shelfsize)))
                         e_border_fx_offset(bd, z->x, (z->y + shelfsize));
                    }
               }
             else 
               {
                  E_Border *b;
                  int bx, by, bw, bh;

                  bx = z->x;
                  bw = z->w;
                  by = (z->y + shelfsize);
                  bh = (z->h - shelfsize - panelsize);

                  /* in dual mode */
                  if (il_cfg->policy.mode.side == 0) /* top/bottom */
                    {
                       bh = ((z->h - shelfsize - panelsize) / 2);
                       b = illume_border_at_xy_get(bx, by);
                       if ((b) && (bd != b)) 
                         by = by + bh;
                       else if (b) 
                         by = bd->fx.y;
                       if (illume_border_is_conformant(bd)) 
                         {
                            by = z->y;
                            bh = (z->h / 2);
                            if ((b) && (bd != b)) 
                              {
                                 by = b->fx.y + b->h;
                                 bh = (z->h - (b->fx.y + b->h));
                              }
                            else if (b) 
                              by = bd->fx.y;
                         }
                    }
                  else if (il_cfg->policy.mode.side == 1) /* left/right */
                    {
                       bw = (z->w / 2);
                       b = illume_border_at_xy_get(bx, by);
                       if ((b) && (bd != b)) 
                         bx = bx + bw;
                       else if (b) 
                         bx = bd->x;
                       if (illume_border_is_conformant(bd)) 
                         {
                            bx = z->x;
                            by = z->y;
                            bw = (z->w / 2);
                            bh = z->h;
                            if ((b) && (bd != b)) 
                              bx = b->fx.x + b->w;
                         }
                    }
                  if ((bd->w != bw) || (bd->h != bh))
                    e_border_resize(bd, bw, bh);
                  if ((bd->x != bx) || (bd->y != by))
                    e_border_fx_offset(bd, bx, by);
               }
          }
     }
}

static void
_zone_move_resize(E_Zone *z)
{ // the zone was moved or resized - re-configure all windows in this zone
   _zone_layout(z);
}

static void 
_border_activate(E_Border *bd) 
{
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
                  if (!bd->lock_user_iconify) e_border_uniconify(bd);
               }
             if (!bd->lock_user_stacking) e_border_raise(bd);
             if (!bd->lock_focus_out) e_border_focus_set(bd, 1, 1);
          }
     }
}

//////////////////////////////////////////////////////////////////////////////
const Illume_Layout_Mode laymode =
{
   "illume", "Illume", // FIXME: translatable?
     _border_add,
     _border_del,
     _border_focus_in,
     _border_focus_out,
     _zone_layout,
     _zone_move_resize, 
     _border_activate
};

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
