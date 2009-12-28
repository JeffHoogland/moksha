#include "e.h"
#include "e_mod_layout.h"
#include "e_mod_config.h"
#include "e_mod_layout_illume.h"
#include "e_mod_border.h"

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
static Eina_List *handlers = NULL;

void
e_mod_layout_init(E_Module *m)
{
   Eina_List *l;

   hook1 = e_border_hook_add(E_BORDER_HOOK_EVAL_POST_FETCH,
			     _e_mod_layout_cb_hook_post_fetch, NULL);
   hook2 = e_border_hook_add(E_BORDER_HOOK_EVAL_POST_BORDER_ASSIGN,
			     _e_mod_layout_cb_hook_post_border_assign, NULL);
   hook3 = e_border_hook_add(E_BORDER_HOOK_CONTAINER_LAYOUT,
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
   hook1 = NULL;
   hook2 = NULL;
   hook3 = NULL;
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

   /* 
   if (!((bd->need_fullscreen) || (bd->fullscreen)))
     {
        bd->x = zx; bd->y = zy; bd->w = zw; bd->h = zh;
        bd->client.w = bd->w; bd->client.h = bd->h;
        if ((pbx != bd->x) || (pby != bd->y)  ||
            (pbw != bd->w) || (pbh != bd->h))
          {
             bd->changed = 1;
             bd->changes.pos = 1;
             bd->changes.size = 1;
          }
     }
   else
     {
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
     }
    */
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
        E_Border *bd;
        E_Zone *zone;
        int lock = 1;

        if (ev->data.l[0] == ECORE_X_ATOM_E_ILLUME_MODE_SINGLE)
          il_cfg->policy.mode.dual = 0;
        else if (ev->data.l[0] == ECORE_X_ATOM_E_ILLUME_MODE_DUAL)
          il_cfg->policy.mode.dual = 1;
        else /* unknown */
          il_cfg->policy.mode.dual = 0;
        e_config_save_queue();

        if (ev->data.l[0] == ECORE_X_ATOM_E_ILLUME_MODE_DUAL) 
          {
             if (il_cfg->policy.mode.side == 0) lock = 0;
          }

        zone = e_zone_current_get(e_container_current_get(e_manager_current_get()));
        bd = e_mod_border_top_shelf_get(zone);
        if (bd) 
          ecore_x_e_illume_drag_locked_set(bd->client.win, lock);
        bd = e_mod_border_bottom_panel_get(zone);
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
   else if (ev->message_type == ECORE_X_ATOM_E_ILLUME_QUICKPANEL_STATE) 
     {
        if (ev->data.l[0] == ECORE_X_ATOM_E_ILLUME_QUICKPANEL_OFF) 
          {
             printf("Quickpanel Off\n");
          }
        else if (ev->data.l[0] == ECORE_X_ATOM_E_ILLUME_QUICKPANEL_ON) 
          {
             printf("Quickpanel On\n");
          }
     }
   return 1;
}
