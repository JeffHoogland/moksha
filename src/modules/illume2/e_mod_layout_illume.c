#include "e.h"
#include "e_mod_main.h"
#include "e_mod_layout.h"
#include "e_mod_layout_illume.h"

static void
_border_add(E_Border *bd)
{ // handle a border being added
/*
   printf("Border Add\n");
   if (bd->client.icccm.transient_for)
     printf("Transient For\n");
   else if (bd->client.icccm.client_leader)
     printf("Client Leader\n");
*/
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

   // data for state
   int shelfsize = 0;
   int kbdsize = 0;
   int panelsize = 0;

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
             if (bd->layer != 200) e_border_layer_set(bd, 200);
          }
        else if (illume_border_is_bottom_panel(bd))
          {
             e_border_move_resize(bd, z->x, z->y + z->h - panelsize, z->w, panelsize);
             if (bd->layer != 100) e_border_layer_set(bd, 100);
          }
        else if (illume_border_is_keyboard(bd))
          {
             e_border_move_resize(bd, z->x, z->y + z->h - kbdsize, z->w, kbdsize);
             if (bd->layer != 150) e_border_layer_set(bd, 150);
          }
        else if (illume_border_is_home(bd))
          {
             e_border_move_resize(bd, z->x, z->y + shelfsize, z->w, z->h - shelfsize);
             if (bd->layer != 50) e_border_layer_set(bd, 50);
          }
        else if (illume_border_is_dialog(bd))
          {
             if (mh > z->h) mh = z->h;
             e_border_move_resize(bd, z->x, z->y + ((z->h - mh) / 2), z->w, mh);
             if (bd->layer != 120) e_border_layer_set(bd, 120);
          }
        else
          {
             e_border_move_resize(bd, z->x, z->y + shelfsize, z->w, z->h - shelfsize - kbdsize);
             if (bd->layer != 100) e_border_layer_set(bd, 100);
          }
     }
}

static void
_zone_move_resize(E_Zone *z)
{ // the zone was moved or resized - re-configure all windows in this zone
   _zone_layout(z);
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
     _zone_move_resize
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
