#include "e.h"
#include "e_mod_main.h"
#include "e_mod_layout.h"
#include "e_mod_layout_illume.h"

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
   E_Manager *man;

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
   EINA_LIST_FREE(handlers, handle) ecore_event_handler_del(handle);
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

   if (bd->client.icccm.transient_for != 0)
     isdialog = 1;
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
   if ((bd->client.vkbd.vkbd) || /* explicit hint that its a virtual keyboard */
       /* legacy */
       ( /* trap the matchbox qwerty and multitap kbd's */
         (((bd->client.icccm.title) && (!strcmp(bd->client.icccm.title, "Keyboard"))) ||
          ((bd->client.icccm.name) && ((!strcmp(bd->client.icccm.name, "multitap-pad")))))
         && (bd->client.netwm.state.skip_taskbar)
         && (bd->client.netwm.state.skip_pager)
         )
       )
     return 1;
   return 0;
}

// get window info - is it a bottom app panel window (eg qtopia softmenu)
Eina_Bool
illume_border_is_bottom_panel(E_Border *bd)
{
   if ((bd->client.netwm.type == ECORE_X_WINDOW_TYPE_DOCK) ||
       (bd->client.qtopia.soft_menu))
     return 1;
   return 0;
}

// get window info - is it a top shelf window
Eina_Bool
illume_border_is_top_shelf(E_Border *bd)
{
   if ((bd->client.icccm.name) &&
       (strstr(bd->client.icccm.name, "xfce4-panel")))
     return 1;
   // FIXME: detect
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
   if (((bd->client.icccm.name) && 
        (strstr(bd->client.icccm.name, "Illume-Home"))) || 
       ((bd->client.icccm.title) && 
        (strstr(bd->client.icccm.title, "Home"))))
     return 1;
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
        if (bd->client.icccm.base_w > bd->client.icccm.min_h)
          *mh = bd->client.icccm.base_h;
        else
          *mh = bd->client.icccm.min_h;
     }
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
