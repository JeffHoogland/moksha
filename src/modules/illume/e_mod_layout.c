#include "e.h"
#include "e_mod_main.h"
#include "e_mod_layout.h"
#include "e_kbd.h"
#include "e_cfg.h"

/* FIXME: THIS CODE IS UGLY. MUST FIX!!!!! */

#define POST_NONE 0
#define POST_HIDE 1
#define POST_CLOSE 2

typedef struct _Effect Effect;

struct _Effect
{
   E_Border *border;
   Ecore_Animator *animator;
   double time_start, in;
   int direction;
   int post;
};

/* internal calls */
static int _e_mod_layout_cb_effect_animator(void *data);
static void _e_mod_layout_effect_slide_out(E_Border *bd, double in, int post);
static void _e_mod_layout_effect_slide_in(E_Border *bd, double in, int post);
static void _e_mod_layout_cb_hook_post_fetch(void *data, E_Border *bd);
static void _e_mod_layout_post_border_assign(E_Border *bd, int not_new);
static void _e_mod_layout_cb_hook_post_border_assign(void *data, E_Border *bd);
static void _e_mod_layout_cb_hook_end(void *data, E_Border *bd);
static int _cb_event_border_add(void *data, int type, void *event);
static int _cb_event_border_remove(void *data, int type, void *event);
static int _cb_event_border_focus_in(void *data, int type, void *event);
static int _cb_event_border_focus_out(void *data, int type, void *event);
static int _cb_event_border_show(void *data, int type, void *event);
static int _cb_event_border_hide(void *data, int type, void *event);
static int _cb_event_zone_move_resize(void *data, int type, void *event);

/* state */
static E_Border_Hook *hook1 = NULL;
static E_Border_Hook *hook2 = NULL;
static E_Border_Hook *hook3 = NULL;
static Eina_List *handlers = NULL;
static Eina_List *effects = NULL;

/* mainwin is the main big window currently focused/visible */
static E_Border *mainwin = NULL;
static E_Border *focuswin = NULL;
static E_Border *dockwin = NULL;
static int dockwin_use = 0;

/* called from the module core */
void
_e_mod_layout_init(E_Module *m)
{
   hook1 = e_border_hook_add(E_BORDER_HOOK_EVAL_POST_FETCH,
			     _e_mod_layout_cb_hook_post_fetch, NULL);
   hook2 = e_border_hook_add(E_BORDER_HOOK_EVAL_POST_BORDER_ASSIGN,
			     _e_mod_layout_cb_hook_post_border_assign, NULL);
   hook3 = e_border_hook_add(E_BORDER_HOOK_EVAL_END,
			     _e_mod_layout_cb_hook_end, NULL);
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
   
     {
	E_Zone *zone;
	unsigned int area[4];
	int wx = 0, wy = 0, ww = 0, wh = 0;
	int wx2 = 0, wy2 = 0, ww2 = 0, wh2 = 0;
	Ecore_X_Atom *supported, *supported_new;
	int i, supported_num;
	
	zone = e_util_zone_current_get(e_manager_current_get());
	e_slipshelf_safe_app_region_get(zone, &wx, &wy, &ww, &wh);
	e_kbd_safe_app_region_get(zone, &wx2, &wy2, &ww2, &wh2);
	E_RECTS_CLIP_TO_RECT(wx, wy, ww, wh, wx2, wy2, ww2, wh2);
	area[0] = wx;
	area[1] = wy;
	area[2] = ww;
	area[3] = wh;
	ecore_x_netwm_desk_workareas_set(zone->container->manager->root,
					 area, 1);
	
	if (ecore_x_netwm_supported_get(zone->container->manager->root,
					&supported, &supported_num))
	  {
	     int have_it = 0;
	     
	     for (i = 0; i < supported_num; i++)
	       {
		  if (supported[i] == ECORE_X_ATOM_NET_WORKAREA)
		    {
		       have_it = 1;
		       break;
		    }
	       }
	     if (!have_it)
	       {
		  supported_new = malloc(sizeof(Ecore_X_Atom) * (supported_num + 1));
		  if (supported_new)
		    {
		       memcpy(supported_new, supported, sizeof(Ecore_X_Atom) * supported_num);
		       supported_new[supported_num] = ECORE_X_ATOM_NET_WORKAREA;
		       supported_num++;
		       ecore_x_netwm_supported_set(zone->container->manager->root,
						   supported_new, supported_num);
		       free(supported_new);
		    }
	       }
	     free(supported);
	  }
	else
	  {
	     Ecore_X_Atom atom;
	     
	     atom = ECORE_X_ATOM_NET_WORKAREA;
	     ecore_x_netwm_supported_set(zone->container->manager->root,
					 &atom, 1);
	  }
     }
}

void
_e_mod_layout_shutdown(void)
{
   if (hook1)
     {
	e_border_hook_del(hook1);
	hook1 = NULL;
     }
   if (hook2)
     {
	e_border_hook_del(hook2);
	hook2 = NULL;
     }
   if (hook3)
     {
	e_border_hook_del(hook3);
	hook3 = NULL;
     }
}

void
_e_mod_layout_border_hide(E_Border *bd)
{
   _e_mod_layout_effect_slide_out(bd, (double)illume_cfg->sliding.layout.duration / 1000.0, POST_HIDE);
}

void
_e_mod_layout_border_close(E_Border *bd)
{
   _e_mod_layout_effect_slide_out(bd, (double)illume_cfg->sliding.layout.duration / 1000.0, POST_CLOSE);
}

void
_e_mod_layout_border_show(E_Border *bd)
{
   _e_mod_layout_effect_slide_in(bd, (double)illume_cfg->sliding.layout.duration / 1000.0, POST_NONE);
   e_desk_show(bd->desk);
   e_border_uniconify(bd);
   e_border_show(bd);
   e_border_raise(bd);
   e_border_focus_set(bd, 1, 1);
}

void
_e_mod_layout_apply_all(void)
{
   Eina_List *l, *borders;

   borders = e_border_client_list();
   for (l = borders; l; l = l->next)
     {
	E_Border *bd;
	
	bd = l->data;
	if (e_object_is_del(E_OBJECT(bd))) continue;
	_e_mod_layout_post_border_assign(bd, 1);
     }
}

/* internal calls */
static int
_e_mod_layout_cb_effect_animator(void *data)
{
   Effect *ef;
   double t, p;
   
   ef = data;
   t = ecore_loop_time_get() - ef->time_start;
   if (ef->in > 0.0)
     {
	t /= ef->in;
	if (t > 1.0) t = 1.0;
     }
   else
     {
	t = 1.0;
     }
   
   p = 1.0 - t;
   p = 1.0 - (p * p * p * p);

   if (ef->direction == 1)
     e_border_fx_offset(ef->border, 0, (-ef->border->zone->h * (1.0 - p)));
   else
     e_border_fx_offset(ef->border, 0, (-ef->border->zone->h * p));
   
   if (t >= 1.0)
     {
	if (ef->post == POST_HIDE)
	  {
	     e_border_iconify(ef->border);
//	     e_border_focus_set(ef->border, 0, 1);
//	     printf("hide - focus out\n");
	  }
	else if (ef->post == POST_CLOSE)
	  {
//	     e_border_focus_set(ef->border, 0, 1);
//	     printf("close - focus out\n");
//	     e_border_iconify(ef->border);
	     e_border_act_close_begin(ef->border);
	  }
	else
	  {
//	     printf("show - focus in\n");
//	     e_border_focus_set(ef->border, 1, 1);
	  }
	e_border_fx_offset(ef->border, 0, 0);
	effects = eina_list_remove(effects, ef);
	free(ef);
	return 0;
     }
   return 1;
}

static void
_e_mod_layout_effect_slide_out(E_Border *bd, double in, int post)
{
   Effect *ef;
   
   ef = E_NEW(Effect, 1);
   ef->border = bd;
   ef->animator = ecore_animator_add(_e_mod_layout_cb_effect_animator, ef);
   ef->time_start = ecore_loop_time_get();
   ef->in = in;
   ef->direction = 0;
   ef->post = post;
   effects = eina_list_append(effects, ef);
   
   e_border_fx_offset(ef->border, 0, 0);
   if (in <= 0.0)
     {
	ef->animator = ecore_animator_del(ef->animator);
	ef->animator = NULL;
	_e_mod_layout_cb_effect_animator(ef);
	return;
     }
}

static void
_e_mod_layout_effect_slide_in(E_Border *bd, double in, int post)
{
   Effect *ef;
   
   ef = E_NEW(Effect, 1);
   ef->border = bd;
   ef->animator = ecore_animator_add(_e_mod_layout_cb_effect_animator, ef);
   ef->time_start = ecore_loop_time_get();
   ef->in = in;
   ef->direction = 1;
   ef->post = post;
   effects = eina_list_append(effects, ef);

   if (ef->border->iconic)
     e_border_uniconify(ef->border);
   e_border_focus_set(bd, 1, 1);
   e_border_fx_offset(ef->border, 0, -ef->border->zone->h);
   if (in <= 0.0)
     {
	ef->animator = ecore_animator_del(ef->animator);
	ef->animator = NULL;
	_e_mod_layout_cb_effect_animator(ef);
	return;
     }
}

static Ecore_Timer *_dockwin_hide_timer = NULL;

static int
_e_mod_layout_cb_docwin_hide(void *data)
{
   _dockwin_hide_timer = NULL;
   if (!dockwin) return 0;
   e_border_hide(dockwin, 2);
   return 0;
}

static void
_e_mod_layout_dockwin_show(void)
{
//   if (dockwin_use) return;
   dockwin_use = 1;
   if (_dockwin_hide_timer)
     {
	ecore_timer_del(_dockwin_hide_timer);
	_dockwin_hide_timer = NULL;
     }
   if (!dockwin) return;
   e_border_show(dockwin);
}

static void
_e_mod_layout_dockwin_hide(void)
{
//   if (!dockwin_use) return;
   dockwin_use = 0;
   if (_dockwin_hide_timer) ecore_timer_del(_dockwin_hide_timer);
   _dockwin_hide_timer = ecore_timer_add(0.2, _e_mod_layout_cb_docwin_hide, NULL);
/*   
   if (!dockwin) return;
   e_border_hide(dockwin, 2);
 */
}

static int
_is_dialog(E_Border *bd)
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

static void
_e_mod_layout_cb_hook_post_fetch(void *data, E_Border *bd)
{
   if (bd->stolen) return;
   if (bd->new_client)
     {
	if (bd->remember)
	  {
	     if (bd->bordername)
	       {
		  evas_stringshare_del(bd->bordername);
		  bd->bordername = NULL;
		  bd->client.border.changed = 1;
	       }
	     e_remember_unuse(bd->remember);
	     bd->remember = NULL;
	  }
	if (!_is_dialog(bd))
	  {
	     if (bd->bordername) evas_stringshare_del(bd->bordername);
	     bd->bordername = evas_stringshare_add("borderless");
	     bd->client.border.changed = 1;
	  }
        bd->client.e.state.centered = 0;
     }
}

static void
_e_mod_layout_post_border_assign(E_Border *bd, int not_new)
{
   int wx = 0, wy = 0, ww = 0, wh = 0;
   int wx2 = 0, wy2 = 0, ww2 = 0, wh2 = 0;
   int center = 0;
   int isdialog = 0;
   int pbx, pby, pbw, pbh;
   
   if (bd->stolen) return;
   /* FIXME: make some modification based on policy */
   if ((bd->new_client) && (not_new)) return;
   pbx = bd->x;
   pby = bd->y;
   pbw = bd->w;
   pbh = bd->h;
   isdialog = _is_dialog(bd);
   e_slipshelf_safe_app_region_get(bd->zone, &wx, &wy, &ww, &wh);
   e_kbd_safe_app_region_get(bd->zone, &wx2, &wy2, &ww2, &wh2);
   E_RECTS_CLIP_TO_RECT(wx, wy, ww, wh, wx2, wy2, ww2, wh2);
   bd->client.e.state.centered = 0;
   if ((bd->client.netwm.type == ECORE_X_WINDOW_TYPE_DOCK) ||
       (bd->client.qtopia.soft_menu))
     {
	  {
	     unsigned int area[4];
	     
	     dockwin = bd;
	     dockwin->lock_focus_out = 1;
	     area[0] = wx;
	     area[1] = wy;
	     area[2] = ww;
	     area[3] = wh - bd->h;
	     ecore_x_netwm_desk_workareas_set(bd->zone->container->manager->root,
					      area, 1);
//	     wy = wy + wh - bd->h;
//	     wh = bd->h;
	     if (bd->new_client)
	       {
		  _e_mod_layout_dockwin_show();
		  _e_mod_layout_dockwin_hide();
	       }
	  }
     }
   else if (isdialog)
     {
	if (dockwin)
	  {
	     wh -= dockwin->h;
	  }
//	if (ww > (bd->client.icccm.min_w + bd->client_inset.l + bd->client_inset.r))
//	  ww = bd->client.icccm.min_w + bd->client_inset.l + bd->client_inset.r;
	bd->w = ww;
	if (bd->client.h < bd->client.icccm.min_h)
	  bd->h = bd->client.icccm.min_h + 
	  bd->client_inset.t + bd->client_inset.b;
	if (bd->h > wh) bd->h = wh;
	bd->client.w = bd->w - bd->client_inset.l - bd->client_inset.r;
	bd->client.h = bd->h - bd->client_inset.t - bd->client_inset.b;
	bd->changes.size = 1;
//	if (bd->client.icccm.max_w < bd->client.w)
//	  bd->client.icccm.max_w = bd->client.w;
//	if (bd->client.icccm.base_w > bd->client.w)
//	  bd->client.icccm.base_w = bd->client.w;
//	if (bd->client.icccm.base_h > (wh - bd->client_inset.t - bd->client_inset.b))
//	  bd->client.icccm.base_h = wh - bd->client_inset.t - bd->client_inset.b;
//	if (bd->client.icccm.min_w > bd->client.w)
//	  bd->client.icccm.min_w = bd->client.w;
//	if (bd->client.icccm.min_h > (wh - bd->client_inset.t - bd->client_inset.b))
//	  bd->client.icccm.min_h = wh - bd->client_inset.t - bd->client_inset.b;
	if (bd->new_client)
	  {
	     _e_mod_layout_effect_slide_in(bd, (double)illume_cfg->sliding.layout.duration / 1000.0, POST_NONE);
	  }
     }
   else
     {
	if ((dockwin) && (dockwin->client.qtopia.soft_menu) && 
	    (bd->client.qtopia.soft_menus))
	  {
	     wh -= dockwin->h;
	  }
	if (bd->new_client)
	  {
	     _e_mod_layout_effect_slide_in(bd, (double)illume_cfg->sliding.layout.duration / 1000.0, POST_NONE);
	  }
     }

   if (bd == dockwin)
     {
	bd->x = 0;
	bd->y = wy + wh - bd->h;
	bd->w = ww;
	bd->h = bd->h;
	
	if ((pbx != bd->x) || (pby != bd->y)  ||
	    (pbw != bd->w) || (pbh != bd->h))
	  {
	     if (bd->internal_ecore_evas)
	       ecore_evas_managed_move(bd->internal_ecore_evas,
				       bd->x + bd->fx.x + bd->client_inset.l,
				       bd->y + bd->fx.y + bd->client_inset.t);
	     ecore_x_icccm_move_resize_send(bd->client.win,
					    bd->x + bd->fx.x + bd->client_inset.l,
					    bd->y + bd->fx.y + bd->client_inset.t,
					    bd->client.w,
					    bd->client.h);
	     bd->changed = 1;
	     bd->changes.pos = 1;
	     bd->changes.size = 1;
	  }
	
	bd->lock_border = 1;
	
	bd->lock_client_location = 1;
	bd->lock_client_size = 1;
//	bd->lock_client_stacking = 1;
	bd->lock_client_desk = 1;
	bd->lock_client_sticky = 1;
	bd->lock_client_shade = 1;
	bd->lock_client_maximize = 1;
	
	bd->lock_user_location = 1;
	bd->lock_user_size = 1;
//	bd->lock_user_stacking = 1;
	bd->lock_user_desk = 1;
	bd->lock_user_sticky = 1;
	bd->lock_user_shade = 1;
	bd->lock_user_maximize = 1;
     }
   else if (isdialog)
     {
//	if (bd->new_client)
	  {
	     bd->client.e.state.centered = 0;
	     if (bd->new_client)
	       {
		  bd->x = wx + ((ww - bd->w) / 2);
		  bd->y = wy + ((wh - bd->h) / 2);
	       }
	     if ((pbx != bd->x) || (pby != bd->y)  ||
		 (pbw != bd->w) || (pbh != bd->h))
	       {
		  if (bd->internal_ecore_evas)
		    ecore_evas_managed_move(bd->internal_ecore_evas,
					    bd->x + bd->fx.x + bd->client_inset.l,
					    bd->y + bd->fx.y + bd->client_inset.t);
		  ecore_x_icccm_move_resize_send(bd->client.win,
						 bd->x + bd->fx.x + bd->client_inset.l,
						 bd->y + bd->fx.y + bd->client_inset.t,
						 bd->client.w,
						 bd->client.h);
		  bd->changed = 1;
		  bd->changes.pos = 1;
	       }
	     if (bd->remember)
	       {
		  e_remember_unuse(bd->remember);
		  bd->remember = NULL;
	       }
	  }
	bd->placed = 1;
	
	bd->lock_border = 1;
	
	bd->lock_client_location = 1;
//	bd->lock_client_size = 1;
	bd->lock_client_desk = 1;
	bd->lock_client_sticky = 1;
	bd->lock_client_shade = 1;
	bd->lock_client_maximize = 1;
	
	bd->lock_user_location = 1;
	bd->lock_user_size = 1;
//	bd->lock_user_desk = 1;
//	bd->lock_user_sticky = 1;
	bd->lock_user_shade = 1;
	bd->lock_user_maximize = 1;
     }
   else
     {
	bd->placed = 1;
	if (bd->focused)
	  {
	     if ((bd->need_fullscreen) || (bd->fullscreen)) e_kbd_fullscreen_set(bd->zone, 1);
	     else e_kbd_fullscreen_set(bd->zone, 0);
	  }
	if (!((bd->need_fullscreen) || (bd->fullscreen)))
	  {
	     e_kbd_fullscreen_set(bd->zone, 0);
	     bd->x = wx;
	     bd->y = wy;
	     bd->w = ww;
	     bd->h = wh;
	     bd->client.w = bd->w;
	     bd->client.h = bd->h;
	     if ((pbx != bd->x) || (pby != bd->y)  ||
		 (pbw != bd->w) || (pbh != bd->h))
	       {
		  if (bd->internal_ecore_evas)
		    ecore_evas_managed_move(bd->internal_ecore_evas,
					    bd->x + bd->fx.x + bd->client_inset.l,
					    bd->y + bd->fx.y + bd->client_inset.t);
		  ecore_x_icccm_move_resize_send(bd->client.win,
						 bd->x + bd->fx.x + bd->client_inset.l,
						 bd->y + bd->fx.y + bd->client_inset.t,
						 bd->client.w,
						 bd->client.h);
		  bd->changed = 1;
		  bd->changes.pos = 1;
		  bd->changes.size = 1;
	       }
	  }
	else
	  {
	     bd->x = wx2;
	     bd->y = wy2;
	     bd->w = ww2;
	     bd->h = wh2;
	     bd->client.w = bd->w;
	     bd->client.h = bd->h;
	     if ((pbx != bd->x) || (pby != bd->y)  ||
		 (pbw != bd->w) || (pbh != bd->h))
	       {
		  if (bd->internal_ecore_evas)
		    ecore_evas_managed_move(bd->internal_ecore_evas,
					    bd->x + bd->fx.x + bd->client_inset.l,
					    bd->y + bd->fx.y + bd->client_inset.t);
		  ecore_x_icccm_move_resize_send(bd->client.win,
						 bd->x + bd->fx.x + bd->client_inset.l,
						 bd->y + bd->fx.y + bd->client_inset.t,
						 bd->client.w,
						 bd->client.h);
		  bd->changed = 1;
		  bd->changes.pos = 1;
		  bd->changes.size = 1;
	       }
	  }
	if (bd->remember)
	  {
	     e_remember_unuse(bd->remember);
	     bd->remember = NULL;
	  }
	bd->lock_border = 1;
	
	bd->lock_client_location = 1;
	bd->lock_client_size = 1;
//	bd->lock_client_stacking = 1;
	bd->lock_client_desk = 1;
	bd->lock_client_sticky = 1;
	bd->lock_client_shade = 1;
	bd->lock_client_maximize = 1;
	
	bd->lock_user_location = 1;
	bd->lock_user_size = 1;
//	bd->lock_user_stacking = 1;
//	bd->lock_user_desk = 1;
	bd->lock_user_sticky = 1;
//	bd->lock_user_shade = 1;
//	bd->lock_user_maximize = 1;
	
	bd->client.icccm.base_w = 1;
	bd->client.icccm.base_h = 1;
	bd->client.icccm.min_w = 1;
	bd->client.icccm.min_h = 1;
	bd->client.icccm.max_w = 32767;
	bd->client.icccm.max_h = 32767;
	bd->client.icccm.min_aspect = 0.0;
	bd->client.icccm.max_aspect = 0.0;
     }
}

static void
_e_mod_layout_cb_hook_post_border_assign(void *data, E_Border *bd)
{
   _e_mod_layout_post_border_assign(bd, 0);
}

static void
_e_mod_layout_cb_hook_end(void *data, E_Border *bd)
{
   if (bd->stolen) return;
   if ((bd == dockwin) && (!dockwin_use) && (dockwin->visible))
     _e_mod_layout_dockwin_hide();
}

static int
_cb_event_border_add(void *data, int type, void *event)
{
   E_Event_Border_Add *ev;
   
   ev = event;
   if (ev->border->stolen) return 1;
   return 1;
}

static int
_cb_event_border_remove(void *data, int type, void *event)
{
   E_Event_Border_Remove *ev;
   Eina_List *l, *pl;
   
   ev = event;
   if (ev->border->stolen) return 1;
   if (ev->border == dockwin)
     {
	unsigned int area[4];
	int wx = 0, wy = 0, ww = 0, wh = 0;
	int wx2 = 0, wy2 = 0, ww2 = 0, wh2 = 0;
	
	dockwin = NULL;
	dockwin_use = 0;
	e_slipshelf_safe_app_region_get(ev->border->zone, &wx, &wy, &ww, &wh);
	e_kbd_safe_app_region_get(ev->border->zone, &wx2, &wy2, &ww2, &wh2);
	E_RECTS_CLIP_TO_RECT(wx, wy, ww, wh, wx2, wy2, ww2, wh2);
	area[0] = wx;
	area[1] = wy;
	area[2] = ww;
	area[3] = wh;
	ecore_x_netwm_desk_workareas_set(ev->border->zone->container->manager->root,
					 area, 1);
     }
   for (l = effects; l;)
     {
	Effect *ef;
	
	ef = l->data;
	pl = l;
	l = l->next;
	if (ef->border == ev->border)
	  {
	     effects = eina_list_remove_list(effects, pl);
	     ecore_animator_del(ef->animator);
	     free(ef);
	  }
     }
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
   if (bd != dockwin)
     {
	if (bd->client.qtopia.soft_menus)
	  {
	     if ((dockwin) && (!dockwin_use) && (dockwin->client.qtopia.soft_menu))
	       _e_mod_layout_dockwin_show();
	  }
	else
	  {
	     if ((dockwin) && (dockwin_use) && (dockwin->client.qtopia.soft_menu))
	       _e_mod_layout_dockwin_hide();
	  }
     }
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
   if (bd != dockwin)
     {
	if (bd->client.qtopia.soft_menus)
	  {
	     if ((dockwin) && (dockwin_use) && (dockwin->client.qtopia.soft_menu))
	       _e_mod_layout_dockwin_hide();
	  }
     }
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
   _e_mod_layout_apply_all();
   return 1;
}

