#include "e.h"
#include "e_mod_border.h"
#include "e_mod_config.h"
#include "e_kbd.h"

//////////////////////////////////////////////////////////////////////////////
// :: Convenience routines to make it easy to write layout logic code ::

// activate a window - meant for main app and home app windows
void
e_mod_border_activate(E_Border *bd)
{
   e_desk_show(bd->desk);
   e_border_uniconify(bd);
   e_border_raise(bd);
   e_border_show(bd);
   e_border_focus_set(bd, 1, 1);
}

// activate a window that isnt meant to get the focus - like panels, kbd etc.
void
e_mod_border_show(E_Border *bd)
{
   e_desk_show(bd->desk);
   e_border_uniconify(bd);
   e_border_raise(bd);
   e_border_show(bd);
}

// get a window away from being visile (but maintain it)
void
e_mod_border_deactivate(E_Border *bd)
{
   e_border_iconify(bd);
}

// get window info - is this one a dialog?
Eina_Bool
e_mod_border_is_dialog(E_Border *bd)
{
   int isdialog = 0, i;

   if (bd->client.icccm.transient_for != 0) isdialog = 1;
   if (bd->client.netwm.type == ECORE_X_WINDOW_TYPE_DIALOG)
     {
	isdialog = 1;
	if (bd->client.netwm.extra_types)
	  {
	     for (i = 0; i < bd->client.netwm.extra_types_num; i++)
	       {
		  if (bd->client.netwm.extra_types[i] == 
		      ECORE_X_WINDOW_TYPE_UNKNOWN) continue;
		  if ((bd->client.netwm.extra_types[i] !=
		       ECORE_X_WINDOW_TYPE_DIALOG) &&
		      (bd->client.netwm.extra_types[i] !=
		       ECORE_X_WINDOW_TYPE_SPLASH))
		    {
		       return 0;
		    }
	       }
	  }
     }
   return isdialog;
}

// get window info - is this a vkbd window
Eina_Bool
e_mod_border_is_keyboard(E_Border *bd)
{
   if (bd->client.vkbd.vkbd) return 1;
   if (il_cfg->policy.vkbd.match.title) 
     {
        if ((bd->client.icccm.title) && 
            (!strcmp(bd->client.icccm.title, il_cfg->policy.vkbd.title)))
          return 1;
     }
   if (il_cfg->policy.vkbd.match.name) 
     {
        if ((bd->client.icccm.name) && 
            (!strcmp(bd->client.icccm.name, il_cfg->policy.vkbd.name)))
          return 1;
     }
   if (il_cfg->policy.vkbd.match.class) 
     {
        if ((bd->client.icccm.class) && 
            (!strcmp(bd->client.icccm.class, il_cfg->policy.vkbd.class)))
          return 1;
     }
   if ((bd->client.icccm.name) && 
       ((!strcmp(bd->client.icccm.name, "multitap-pad")))
       && (bd->client.netwm.state.skip_taskbar)
       && (bd->client.netwm.state.skip_pager)) 
     return 1;
   return 0;
}

// get window info - is it a bottom app panel window (eg qtopia softmenu)
Eina_Bool
e_mod_border_is_bottom_panel(E_Border *bd)
{
   if (il_cfg->policy.softkey.match.title) 
     {
        if ((bd->client.icccm.title) && 
            (!strcmp(bd->client.icccm.title, il_cfg->policy.softkey.title)))
          return 1;
     }
   if (il_cfg->policy.softkey.match.name) 
     {
        if ((bd->client.icccm.name) && 
            (!strcmp(bd->client.icccm.name, il_cfg->policy.softkey.name)))
          return 1;
     }
   if (il_cfg->policy.softkey.match.class) 
     {
        if ((bd->client.icccm.class) && 
            (!strcmp(bd->client.icccm.class, il_cfg->policy.softkey.class)))
          return 1;
     }
   if (((bd->client.netwm.type == ECORE_X_WINDOW_TYPE_DOCK) ||
        (bd->client.qtopia.soft_menu)))
     return 1;
   return 0;
}

// get window info - is it a top shelf window
Eina_Bool
e_mod_border_is_top_shelf(E_Border *bd)
{
   if (il_cfg->policy.indicator.match.title) 
     {
        if ((bd->client.icccm.title) && 
            (!strcmp(bd->client.icccm.title, il_cfg->policy.indicator.title)))
          return 1;
     }
   if (il_cfg->policy.indicator.match.name) 
     {
        if ((bd->client.icccm.name) && 
            (!strcmp(bd->client.icccm.name, il_cfg->policy.indicator.name)))
          return 1;
     }
   if (il_cfg->policy.indicator.match.class) 
     {
        if ((bd->client.icccm.class) && 
            (!strcmp(bd->client.icccm.class, il_cfg->policy.indicator.class)))
          return 1;
     }
   return 0;
}

// get window info - is it a mini app window
Eina_Bool
e_mod_border_is_mini_app(E_Border *bd)
{
   // FIXME: detect
   return 0;
}

// get window info - is it a notification window
Eina_Bool
e_mod_border_is_notification(E_Border *bd)
{
   // FIXME: detect
   return 0;
}

// get window info - is it a home window
Eina_Bool
e_mod_border_is_home(E_Border *bd)
{
   if (il_cfg->policy.home.match.title) 
     {
        if ((bd->client.icccm.title) && 
            (!strcmp(bd->client.icccm.title, il_cfg->policy.home.title)))
          return 1;
     }
   if (il_cfg->policy.home.match.name) 
     {
        if ((bd->client.icccm.name) && 
            (!strcmp(bd->client.icccm.name, il_cfg->policy.home.name)))
          return 1;
     }
   if (il_cfg->policy.home.match.class) 
     {
        if ((bd->client.icccm.class) && 
            (!strcmp(bd->client.icccm.class, il_cfg->policy.home.class)))
          return 1;
     }
   return 0;
}

// get window info - is it side pane (left) window
Eina_Bool
e_mod_border_is_side_pane_left(E_Border *bd)
{
   // FIXME: detect
   return 0;
}

// get window info - is it side pane (right) window
Eina_Bool
e_mod_border_is_side_pane_right(E_Border *bd)
{
   // FIXME: detect
   return 0;
}

// get window info - is it overlay window (eg expose display of windows etc.)
Eina_Bool
e_mod_border_is_overlay(E_Border *bd)
{
   // FIXME: detect
   return 0;
}

Eina_Bool 
e_mod_border_is_conformant(E_Border *bd) 
{
   if (strstr(bd->client.icccm.class, "config")) return EINA_FALSE;
   return ecore_x_e_illume_conformant_get(bd->client.win);
}

Eina_List *
e_mod_border_valid_borders_get(E_Zone *zone) 
{
   Eina_List *bds, *l, *ret = NULL;
   E_Border *bd;

   bds = e_border_client_list();
   EINA_LIST_FOREACH(bds, l, bd) 
     {
        if (!bd) continue;
        if (bd->zone != zone) continue;
        if (e_mod_border_is_top_shelf(bd)) continue;
        if (e_mod_border_is_bottom_panel(bd)) continue;
        if (e_mod_border_is_keyboard(bd)) continue;
        if (e_mod_border_is_dialog(bd)) continue;
        ret = eina_list_append(ret, bd);
     }
   return ret;
}

E_Border *
e_mod_border_valid_border_get(E_Zone  *zone) 
{
   Eina_List *bds, *l;
   E_Border *bd, *ret = NULL;

   bds = e_border_client_list();
   EINA_LIST_FOREACH(bds, l, bd) 
     {
        if (!bd) continue;
        if (bd->zone != zone) continue;
        if (e_mod_border_is_top_shelf(bd)) continue;
        if (e_mod_border_is_bottom_panel(bd)) continue;
        if (e_mod_border_is_keyboard(bd)) continue;
        if (e_mod_border_is_dialog(bd)) continue;
        ret = bd;
        break;
     }
   return ret;
}

int 
e_mod_border_valid_count_get(E_Zone *zone) 
{
   Eina_List *l;
   int count;

   l = e_mod_border_valid_borders_get(zone);
   count = eina_list_count(l);
   eina_list_free(l);
   return count;
}

E_Border *
e_mod_border_at_xy_get(E_Zone *zone, int x, int y) 
{
   Eina_List *bds, *l;
   E_Border *bd, *b = NULL;

   bds = e_mod_border_valid_borders_get(zone);
   EINA_LIST_FOREACH(bds, l, bd) 
     {
        if (((bd->fx.x == x) && (bd->fx.y == y)) ||
            ((bd->x == x) && (bd->y == y)))
          {
             b = bd;
             break;
          }
     }
   eina_list_free(bds);
   return b;
}

E_Border *
e_mod_border_in_region_get(E_Zone *zone, int x, int y, int w, int h) 
{
   Eina_List *bds, *l;
   E_Border *bd, *b = NULL;

   bds = e_mod_border_valid_borders_get(zone);
   EINA_LIST_FOREACH(bds, l, bd) 
     {
        if (E_INSIDE(bd->x, bd->fx.y, x, y, w, h)) 
          {
             b = bd;
             break;
          }
     }
   eina_list_free(bds);
   return b;
}

E_Border *
e_mod_border_top_shelf_get(E_Zone *zone) 
{
   Eina_List *bds, *l;
   E_Border *bd, *b = NULL;

   bds = e_border_client_list();
   EINA_LIST_FOREACH(bds, l, bd) 
     {
        if (bd->zone != zone) continue;
        if (!e_mod_border_is_top_shelf(bd)) continue;
        b = bd;
        break;
     }
   return b;
}

E_Border *
e_mod_border_bottom_panel_get(E_Zone *zone) 
{
   Eina_List *bds, *l;
   E_Border *bd, *b = NULL;

   bds = e_border_client_list();
   EINA_LIST_FOREACH(bds, l, bd) 
     {
        if (bd->zone != zone) continue;
        if (!e_mod_border_is_bottom_panel(bd)) continue;
        b = bd;
        break;
     }
   return b;
}

void 
e_mod_border_top_shelf_pos_get(E_Zone *zone, int *x, int *y) 
{
   E_Border *bd;

   if (!(bd = e_mod_border_top_shelf_get(zone))) return;
   if (x) *x = bd->x;
   if (y) *y = bd->y;
}

void 
e_mod_border_top_shelf_size_get(E_Zone *zone, int *w, int *h) 
{
   E_Border *bd;

   if (!(bd = e_mod_border_top_shelf_get(zone))) return;
   if (w) *w = bd->w;
   if (h) *h = bd->h;
}

void 
e_mod_border_bottom_panel_pos_get(E_Zone *zone, int *x, int *y) 
{
   E_Border *bd;

   if (!(bd = e_mod_border_bottom_panel_get(zone))) return;
   if (x) *x = bd->x;
   if (y) *y = bd->y;
}

void 
e_mod_border_bottom_panel_size_get(E_Zone *zone, int *w, int *h) 
{
   E_Border *bd;

   if (!(bd = e_mod_border_bottom_panel_get(zone))) return;
   if (w) *w = bd->w;
   if (h) *h = bd->h;
}

void
e_mod_border_slide_to(E_Border *bd, int x, int y, Illume_Anim_Class aclass)
{
   // FIXME: do
   // 1. if an existing slide exists, use is current offset x,y as current border pos, new x,y as new pos and start slide again
}

void
e_mod_border_min_get(E_Border *bd, int *mw, int *mh)
{
   if (mw)
     {
        if (bd->client.icccm.base_w > bd->client.icccm.min_w)
          *mw = bd->client.icccm.base_w;
        else
          *mw = bd->client.icccm.min_w;
     }
   if (mh)
     {
        if (bd->client.icccm.base_h > bd->client.icccm.min_h)
          *mh = bd->client.icccm.base_h;
        else
          *mh = bd->client.icccm.min_h;
     }
}

void 
e_mod_border_max_get(E_Border *bd, int *mw, int *mh) 
{
   if (mw)
     {
        if (bd->client.icccm.base_w > bd->client.icccm.max_w)
          *mw = bd->client.icccm.base_w;
        else
          *mw = bd->client.icccm.max_w;
     }
   if (mh)
     {
        if (bd->client.icccm.base_h > bd->client.icccm.max_h)
          *mh = bd->client.icccm.base_h;
        else
          *mh = bd->client.icccm.max_h;
     }
}

void 
e_mod_border_app1_safe_region_get(E_Zone *zone, int *x, int *y, int *w, int *h) 
{
   int ty, nx, ny, nw, nh;

   if (!zone) return;
   e_kbd_safe_app_region_get(zone, &nx, &ny, &nw, &nh);
   e_mod_border_top_shelf_pos_get(zone, NULL, &ty);
   if (nh >= zone->h) nh = (ny + ty);
   if (x) *x = nx;
   if (y) *y = ny;
   if (w) *w = nw;
   if (h) *h = nh;
}

void 
e_mod_border_app2_safe_region_get(E_Zone *zone, int *x, int *y, int *w, int *h) 
{
   int ty, th, bh;
   int nx, ny, nw, nh;

   if (!zone) return;
   e_kbd_safe_app_region_get(zone, &nx, NULL, &nw, &nh);
   e_mod_border_top_shelf_pos_get(zone, NULL, &ty);
   e_mod_border_top_shelf_size_get(zone, NULL, &th);
   e_mod_border_bottom_panel_size_get(zone, NULL, &bh);
   ny = (ty + th);
   nh = (nh - ny - bh);
   if (x) *x = nx;
   if (y) *y = ny;
   if (w) *w = nw;
   if (h) *h = nh;
}
