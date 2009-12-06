#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"
#include "e_mod_layout.h"
#include "e_mod_layout_illume.h"
#include "e_kbd.h"

/* local function prototypes */
static void _border_calc_position(E_Zone *z, E_Border *bd, int *x, int *y, int *w, int *h);

/* local variables */
static int shelfsize = 0;
static int kbdsize = 0;
static int panelsize = 0;

static void
_border_add(E_Border *bd)
{ // handle a border being added
   e_border_raise(bd);
   e_border_focus_set(bd, 1, 1);
}

static void
_border_del(E_Border *bd)
{ // handle a border being deleted

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
             e_border_move_resize(bd, z->x, z->y, z->w, shelfsize);
             e_border_stick(bd);
          }
        else if (illume_border_is_bottom_panel(bd))
          {
             e_border_move_resize(bd, z->x, z->y + z->h - panelsize, z->w, panelsize);
             e_border_stick(bd);
          }
        else if (illume_border_is_keyboard(bd))
          {
             e_border_move_resize(bd, z->x, z->y + z->h - kbdsize, z->w, kbdsize);
          }
        else if (illume_border_is_home(bd))
          {
             e_border_move_resize(bd, z->x, z->y + shelfsize, z->w, 
                                  z->h - shelfsize - panelsize);
          }
        else if (illume_border_is_dialog(bd))
          {
             if (mh > z->h) mh = z->h;
             e_border_move_resize(bd, z->x, z->y + ((z->h - mh) / 2), z->w, mh);
          }
        else
          {
             e_border_move_resize(bd, z->x, z->y + shelfsize, z->w, 
                                  z->h - shelfsize - panelsize);
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

/* local functions */
static void 
_border_calc_position(E_Zone *z, E_Border *bd, int *x, int *y, int *w, int *h) 
{
   if ((!z) || (!bd)) return;
   if (x) *x = z->x;
   if (y) *y = (z->y + shelfsize);
   if (w) *w = z->w;
   if (illume_border_is_conformant(bd)) 
     {
        if (h) *h = (z->h - shelfsize);
     }
   else 
     {
        if (h) *h = (z->h - shelfsize - panelsize);
     }
   if (il_cfg->policy.mode.dual) 
     {
        /* we ignore config dialogs as we want them fullscreen in dual mode */
        if (strstr(bd->client.icccm.class, "config")) return;
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
