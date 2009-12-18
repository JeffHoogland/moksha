#include "e.h"
#include "e_mod_main.h"
#include "e_mod_layout.h"
#include "e_mod_layout_illume.h"
#include "e_mod_config.h"
#include "e_kbd.h"

// internal calls
static void _e_mod_layout_cb_hook_container_layout(void *data, void *data2);
static void _e_mod_layout_cb_hook_post_fetch(void *data, void *data2);
static void _e_mod_layout_cb_hook_post_border_assign(void *data, void *data2);
static void _e_mod_layout_cb_hook_end(void *data, void *data2);
static int _cb_event_border_add(void *data, int type, void *event);
static int _cb_event_border_remove(void *data, int type, void *event);
static int _cb_event_border_focus_in(void *data, int type, void *event);
static int _cb_event_border_focus_out(void *data, int type, void *event);
static int _cb_event_border_show(void *data, int type, void *event);
static int _cb_event_border_hide(void *data, int type, void *event);
static int _cb_event_zone_move_resize(void *data, int type, void *event);
static int _cb_event_client_message(void *data, int type, void *event);

// state
static E_Border_Hook *hook1 = NULL;
static E_Border_Hook *hook2 = NULL;
static E_Border_Hook *hook3 = NULL;
static E_Border_Hook *hook4 = NULL;
static Eina_List *handlers = NULL;

void
e_mod_layout_init(E_Module *m)
{
   Eina_List *l;

   hook1 = e_border_hook_add(E_BORDER_HOOK_EVAL_POST_FETCH,
			     _e_mod_layout_cb_hook_post_fetch, NULL);
   hook2 = e_border_hook_add(E_BORDER_HOOK_EVAL_POST_BORDER_ASSIGN,
			     _e_mod_layout_cb_hook_post_border_assign, NULL);
   hook3 = e_border_hook_add(E_BORDER_HOOK_EVAL_END,
			     _e_mod_layout_cb_hook_end, NULL);
   hook4 = e_border_hook_add(E_BORDER_HOOK_CONTAINER_LAYOUT,
			     _e_mod_layout_cb_hook_container_layout, NULL);
   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (E_EVENT_BORDER_ADD, _cb_event_border_add, NULL));
   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (E_EVENT_BORDER_REMOVE, _cb_event_border_remove, NULL));
   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (E_EVENT_BORDER_FOCUS_IN, _cb_event_border_focus_in, NULL));
   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (E_EVENT_BORDER_FOCUS_OUT, _cb_event_border_focus_out, NULL));
   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (E_EVENT_BORDER_SHOW, _cb_event_border_show, NULL));
   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (E_EVENT_BORDER_HIDE, _cb_event_border_hide, NULL));
   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (E_EVENT_ZONE_MOVE_RESIZE, _cb_event_zone_move_resize, NULL));
   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (ECORE_X_EVENT_CLIENT_MESSAGE, _cb_event_client_message, NULL));

   illume_layout_illume_init();
}

void
e_mod_layout_shutdown(void)
{
   Ecore_Event_Handler *handle;

   illume_layout_illume_shutdown();

   if (hook1) e_border_hook_del(hook1);
   if (hook2) e_border_hook_del(hook2);
   if (hook3) e_border_hook_del(hook3);
   if (hook4) e_border_hook_del(hook4);
   hook1 = NULL;
   hook2 = NULL;
   hook3 = NULL;
   hook4 = NULL;
   EINA_LIST_FREE(handlers, handle) 
     ecore_event_handler_del(handle);
}

//////////////////////////////////////////////////////////////////////////////
// :: Convenience routines to make it easy to write layout logic code ::

static Eina_List *modes = NULL;
static const Illume_Layout_Mode *mode = NULL;

void
illume_layout_mode_register(const Illume_Layout_Mode *laymode)
{
   if (!modes) mode = laymode;
   modes = eina_list_append(modes, laymode);
}

void
illume_layout_mode_unregister(const Illume_Layout_Mode *laymode)
{ 
   if (mode == laymode) mode = NULL;
   modes = eina_list_remove(modes, laymode);
}

Eina_List *
illume_layout_modes_get(void) 
{
   return modes;
}

//////////////////////////////////////////////////////////////////////////////
// :: Convenience routines to make it easy to write layout logic code ::

// activate a window - meant for main app and home app windows
void
illume_border_activate(E_Border *bd)
{
   e_desk_show(bd->desk);
   e_border_uniconify(bd);
   e_border_raise(bd);
   e_border_show(bd);
   e_border_focus_set(bd, 1, 1);
}

// activate a window that isnt meant to get the focus - like panels, kbd etc.
void
illume_border_show(E_Border *bd)
{
   e_desk_show(bd->desk);
   e_border_uniconify(bd);
   e_border_raise(bd);
   e_border_show(bd);
}

// get a window away from being visile (but maintain it)
void
illume_border_deactivate(E_Border *bd)
{
   e_border_iconify(bd);
}

// get window info - is this one a dialog?
Eina_Bool
illume_border_is_dialog(E_Border *bd)
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
illume_border_is_keyboard(E_Border *bd)
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
illume_border_is_bottom_panel(E_Border *bd)
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
illume_border_is_top_shelf(E_Border *bd)
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
illume_border_is_mini_app(E_Border *bd)
{
   // FIXME: detect
   return 0;
}

// get window info - is it a notification window
Eina_Bool
illume_border_is_notification(E_Border *bd)
{
   // FIXME: detect
   return 0;
}

// get window info - is it a home window
Eina_Bool
illume_border_is_home(E_Border *bd)
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
illume_border_is_side_pane_left(E_Border *bd)
{
   // FIXME: detect
   return 0;
}

// get window info - is it side pane (right) window
Eina_Bool
illume_border_is_side_pane_right(E_Border *bd)
{
   // FIXME: detect
   return 0;
}

// get window info - is it overlay window (eg expose display of windows etc.)
Eina_Bool
illume_border_is_overlay(E_Border *bd)
{
   // FIXME: detect
   return 0;
}

Eina_Bool 
illume_border_is_conformant(E_Border *bd) 
{
   if (strstr(bd->client.icccm.class, "config")) return EINA_FALSE;
   return ecore_x_e_illume_conformant_get(bd->client.win);
}

Eina_List *
illume_border_valid_borders_get(void) 
{
   Eina_List *bds, *l, *ret = NULL;
   E_Border *bd;

   bds = e_border_client_list();
   EINA_LIST_FOREACH(bds, l, bd) 
     {
        if (!bd) continue;
        if (illume_border_is_top_shelf(bd)) continue;
        if (illume_border_is_bottom_panel(bd)) continue;
        if (illume_border_is_keyboard(bd)) continue;
        if (illume_border_is_dialog(bd)) continue;
        ret = eina_list_append(ret, bd);
     }
   return ret;
}

E_Border *
illume_border_valid_border_get(void) 
{
   Eina_List *bds, *l;
   E_Border *bd, *ret = NULL;

   bds = e_border_client_list();
   EINA_LIST_FOREACH(bds, l, bd) 
     {
        if (!bd) continue;
        if (illume_border_is_top_shelf(bd)) continue;
        if (illume_border_is_bottom_panel(bd)) continue;
        if (illume_border_is_keyboard(bd)) continue;
        if (illume_border_is_dialog(bd)) continue;
        ret = bd;
        break;
     }
   return ret;
}

int 
illume_border_valid_count_get(void) 
{
   Eina_List *l;
   int count;

   l = illume_border_valid_borders_get();
   count = eina_list_count(l);
   eina_list_free(l);
   return count;
}

E_Border *
illume_border_at_xy_get(int x, int y) 
{
   Eina_List *bds, *l;
   E_Border *bd, *b = NULL;

   bds = illume_border_valid_borders_get();
   EINA_LIST_FOREACH(bds, l, bd) 
     {
        if ((bd->fx.x == x) && (bd->fx.y == y)) 
          {
             b = bd;
             break;
          }
     }
   eina_list_free(bds);
   return b;
}

E_Border *
illume_border_top_shelf_get(void) 
{
   Eina_List *bds, *l;
   E_Border *bd, *b = NULL;

   bds = e_border_client_list();
   EINA_LIST_FOREACH(bds, l, bd) 
     {
        if (!illume_border_is_top_shelf(bd)) continue;
        b = bd;
        break;
     }
   return b;
}

E_Border *
illume_border_bottom_panel_get(void) 
{
   Eina_List *bds, *l;
   E_Border *bd, *b = NULL;

   bds = e_border_client_list();
   EINA_LIST_FOREACH(bds, l, bd) 
     {
        if (!illume_border_is_bottom_panel(bd)) continue;
        b = bd;
        break;
     }
   return b;
}

void 
illume_border_top_shelf_pos_get(int *x, int *y) 
{
   E_Border *bd;

   if (!(bd = illume_border_top_shelf_get())) return;
   if (x) *x = bd->x;
   if (y) *y = bd->y;
}

void 
illume_border_top_shelf_size_get(int *w, int *h) 
{
   E_Border *bd;

   if (!(bd = illume_border_top_shelf_get())) return;
   if (w) *w = bd->w;
   if (h) *h = bd->h;
}

void 
illume_border_bottom_panel_pos_get(int *x, int *y) 
{
   E_Border *bd;

   if (!(bd = illume_border_bottom_panel_get())) return;
   if (x) *x = bd->x;
   if (y) *y = bd->y;
}

void 
illume_border_bottom_panel_size_get(int *w, int *h) 
{
   E_Border *bd;

   if (!(bd = illume_border_bottom_panel_get())) return;
   if (w) *w = bd->w;
   if (h) *h = bd->h;
}

void
illume_border_slide_to(E_Border *bd, int x, int y, Illume_Anim_Class aclass)
{
   // FIXME: do
   // 1. if an existing slide exists, use is current offset x,y as current border pos, new x,y as new pos and start slide again
}

void
illume_border_min_get(E_Border *bd, int *mw, int *mh)
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
illume_border_app1_safe_region_get(E_Zone *zone, int *x, int *y, int *w, int *h) 
{
   int ty, th;
   int nx, ny, nw, nh;

   if (!zone) return;
   e_kbd_safe_app_region_get(zone, &nx, &ny, &nw, &nh);
   illume_border_top_shelf_pos_get(NULL, &ty);
   illume_border_top_shelf_size_get(NULL, &th);
   nh = (ny + ty);
   if (x) *x = nx;
   if (y) *y = ny;
   if (w) *w = nw;
   if (h) *h = nh;
}

void 
illume_border_app2_safe_region_get(E_Zone *zone, int *x, int *y, int *w, int *h) 
{
   int ty, th, bh;
   int nx, ny, nw, nh;

   if (!zone) return;
   e_kbd_safe_app_region_get(zone, &nx, &ny, &nw, &nh);
   illume_border_top_shelf_pos_get(NULL, &ty);
   illume_border_top_shelf_size_get(NULL, &th);
   illume_border_bottom_panel_size_get(NULL, &bh);
   ny = (ty + th);
   nh = (nh - ny - bh);
   if (x) *x = nx;
   if (y) *y = ny;
   if (w) *w = nw;
   if (h) *h = nh;
}

static void
_e_mod_layout_cb_hook_container_layout(void *data, void *data2)
{
   Eina_List *l;
   E_Zone *zone;
   E_Container *con;

   if (!(con = data2)) return;
   EINA_LIST_FOREACH(con->zones, l, zone)
     {
        if ((mode) && (mode->funcs.zone_layout))
          mode->funcs.zone_layout(zone);
     }
}

static void
_e_mod_layout_cb_hook_post_fetch(void *data, void *data2)
{
   E_Border *bd;

   if (!(bd = data2)) return;
   if (bd->stolen) return;
   if (bd->new_client)
     {
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
        bd->client.e.state.centered = 0;
     }
}

static void
_e_mod_layout_cb_hook_post_border_assign(void *data, void *data2)
{
   E_Border *bd;
   int zx, zy, zw, zh, pbx, pby, pbw, pbh;

   if (!(bd = data2)) return;
   if (bd->stolen) return;

   pbx = bd->x; pby = bd->y; pbw = bd->w; pbh = bd->h;
   zx = bd->zone->x; zy = bd->zone->y; zw = bd->zone->w; zh = bd->zone->h;

   bd->placed = 1;
   bd->client.e.state.centered = 0;
   if (!((bd->need_fullscreen) || (bd->fullscreen)))
     {
/*        
        bd->x = zx; bd->y = zy; bd->w = zw; bd->h = zh;
        bd->client.w = bd->w; bd->client.h = bd->h;
        if ((pbx != bd->x) || (pby != bd->y)  ||
            (pbw != bd->w) || (pbh != bd->h))
          {
             bd->changed = 1;
             bd->changes.pos = 1;
             bd->changes.size = 1;
          }
 */
     }
   else
     {
/*        
        bd->x = zx; bd->y = zy; bd->w = zw; bd->h = zh;
        bd->client.w = bd->w; bd->client.h = bd->h;
        if ((pbx != bd->x) || (pby != bd->y) || 
            (pbw != bd->w) || (pbh != bd->h))
          {
             if (bd->internal_ecore_evas)
               ecore_evas_managed_move(bd->internal_ecore_evas,
                                       bd->x + bd->fx.x + bd->client_inset.l,
                                       bd->y + bd->fx.y + bd->client_inset.t);
             ecore_x_icccm_move_resize_send
               (bd->client.win,
                bd->x + bd->fx.x + bd->client_inset.l,
                bd->y + bd->fx.y + bd->client_inset.t,
                bd->client.w,
                bd->client.h);
             bd->changed = 1;
             bd->changes.pos = 1;
             bd->changes.size = 1;
          }
 */
     }
   if (bd->remember)
     {
        e_remember_unuse(bd->remember);
        bd->remember = NULL;
     }
   bd->lock_border = 1;

   bd->lock_client_location = 1;
   bd->lock_client_size = 1;
   bd->lock_client_desk = 1;
   bd->lock_client_sticky = 1;
   bd->lock_client_shade = 1;
   bd->lock_client_maximize = 1;

   bd->lock_user_location = 1;
   bd->lock_user_size = 1;
   bd->lock_user_sticky = 1;
}

static void
_e_mod_layout_cb_hook_end(void *data, void *data2)
{

}

static int
_cb_event_border_add(void *data, int type, void *event)
{
   E_Event_Border_Add *ev;
   E_Border *bd;

   ev = event;
   if (ev->border->stolen) return 1;
   bd = ev->border;
   if ((mode) && (mode->funcs.border_add))
     mode->funcs.border_add(bd);
   return 1;
}

static int
_cb_event_border_remove(void *data, int type, void *event)
{
   E_Event_Border_Remove *ev;
   E_Border *bd;

   ev = event;
   if (ev->border->stolen) return 1;
   bd = ev->border;
   if ((mode) && (mode->funcs.border_del))
     mode->funcs.border_del(bd);
   return 1;
}

static int
_cb_event_border_focus_in(void *data, int type, void *event)
{
   E_Event_Border_Focus_In *ev;
   E_Border *bd;

   ev = event;
   if (ev->border->stolen) return 1;
   bd = ev->border;
   if ((mode) && (mode->funcs.border_focus_in))
     mode->funcs.border_focus_in(bd);
   return 1;
}

static int
_cb_event_border_focus_out(void *data, int type, void *event)
{
   E_Event_Border_Focus_Out *ev;
   E_Border *bd;

   ev = event;
   if (ev->border->stolen) return 1;
   bd = ev->border;
   if ((mode) && (mode->funcs.border_focus_out))
     mode->funcs.border_focus_out(bd);
   return 1;
}

static int
_cb_event_border_show(void *data, int type, void *event)
{
   E_Event_Border_Show *ev;

   ev = event;
   if (ev->border->stolen) return 1;
   return 1;
}

static int
_cb_event_border_hide(void *data, int type, void *event)
{
   E_Event_Border_Hide *ev;

   ev = event;
   if (ev->border->stolen) return 1;
   return 1;
}

static int
_cb_event_zone_move_resize(void *data, int type, void *event)
{
   E_Event_Zone_Move_Resize *ev;

   ev = event;
   if ((mode) && (mode->funcs.zone_move_resize)) 
     mode->funcs.zone_move_resize(ev->zone);
   return 1;
}

static int 
_cb_event_client_message(void *data, int type, void *event) 
{
   Ecore_X_Event_Client_Message *ev;

   ev = event;
   if (ev->message_type == ECORE_X_ATOM_NET_ACTIVE_WINDOW) 
     {
        E_Border *bd;

        bd = e_border_find_by_client_window(ev->win);
        if ((!bd) || (bd->stolen)) return 1;
        if ((mode) && (mode->funcs.border_activate))
          mode->funcs.border_activate(bd);
     }
   else if (ev->message_type == ECORE_X_ATOM_E_ILLUME_MODE) 
     {
        Ecore_X_Illume_Mode mode;
        E_Border *bd;
        int lock = 1;

        mode = ecore_x_e_illume_mode_get(ev->win);
        if (mode == ECORE_X_ILLUME_MODE_SINGLE) 
          il_cfg->policy.mode.dual = 0;
        else if (mode == ECORE_X_ILLUME_MODE_DUAL)
          il_cfg->policy.mode.dual = 1;
        else /* unknown */
          il_cfg->policy.mode.dual = 0;
        e_config_save_queue();

        if (mode == ECORE_X_ILLUME_MODE_DUAL) 
          {
             if (il_cfg->policy.mode.side == 0) lock = 0;
          }
        bd = illume_border_top_shelf_get();
        if (bd) 
          ecore_x_e_illume_drag_locked_set(bd->client.win, lock);
        bd = illume_border_bottom_panel_get();
        if (bd) 
          ecore_x_e_illume_drag_locked_set(bd->client.win, lock);
     }
   else if (ev->message_type == ECORE_X_ATOM_E_ILLUME_BACK) 
     {
        E_Border *bd, *fbd;
        Eina_List *focused, *l;

        if (!(bd = e_border_focused_get())) return 1;
        focused = e_border_focus_stack_get();
        EINA_LIST_REVERSE_FOREACH(focused, l, fbd) 
          {
             E_Border *fb;

             if (e_object_is_del(E_OBJECT(fbd))) continue;
             if ((!fbd->client.icccm.accepts_focus) && 
                 (!fbd->client.icccm.take_focus)) continue;
             if (fbd->client.netwm.state.skip_taskbar) continue;
             if (fbd == bd) 
               {
                  if (!(fb = focused->next->data)) continue;
                  if (e_object_is_del(E_OBJECT(fb))) continue;
                  if ((!fb->client.icccm.accepts_focus) && 
                      (!fb->client.icccm.take_focus)) continue;
                  if (fb->client.netwm.state.skip_taskbar) continue;
                  e_border_raise(fb);
                  e_border_focus_set(fb, 1, 1);
                  break;
               }
          }
     }
   else if (ev->message_type == ECORE_X_ATOM_E_ILLUME_CLOSE) 
     {
        E_Border *bd;

        if (!(bd = e_border_focused_get())) return 1;
        e_border_act_close_begin(bd);
     }
   else if (ev->message_type == ECORE_X_ATOM_E_ILLUME_DRAG_START) 
     {
        E_Border *bd;

        bd = e_border_find_by_client_window(ev->win);
        if ((!bd) || (bd->stolen)) return 1;
        if ((mode) && (mode->funcs.drag_start))
          mode->funcs.drag_start(bd);
     }
   else if (ev->message_type == ECORE_X_ATOM_E_ILLUME_DRAG_END) 
     {
        E_Border *bd;

        bd = e_border_find_by_client_window(ev->win);
        if ((!bd) || (bd->stolen)) return 1;
        if ((mode) && (mode->funcs.drag_end))
          mode->funcs.drag_end(bd);
     }
   return 1;
}
