#include "e_illume.h"
#include "policy.h"
#include "e.h"

/* local function prototypes */
static void _policy_border_set_focus(E_Border *bd);
static void _policy_border_move(E_Border *bd, int x, int y);
static void _policy_border_resize(E_Border *bd, int w, int h);
static void _policy_border_hide_below(E_Border *bd);
static void _policy_border_show_below(E_Border *bd);
static void _policy_zone_layout_update(E_Zone *zone);
static void _policy_zone_layout_indicator(E_Border *bd, E_Illume_Config_Zone *cz);
static void _policy_zone_layout_quickpanel(E_Border *bd);
static void _policy_zone_layout_softkey(E_Border *bd, E_Illume_Config_Zone *cz);
static void _policy_zone_layout_keyboard(E_Border *bd, E_Illume_Config_Zone *cz);
static void _policy_zone_layout_home_single(E_Border *bd, E_Illume_Config_Zone *cz);
static void _policy_zone_layout_fullscreen(E_Border *bd);
static void _policy_zone_layout_app_single(E_Border *bd, E_Illume_Config_Zone *cz);
static void _policy_zone_layout_app_dual_top(E_Border *bd, E_Illume_Config_Zone *cz);
static void _policy_zone_layout_app_dual_custom(E_Border *bd, E_Illume_Config_Zone *cz);
static void _policy_zone_layout_app_dual_left(E_Border *bd, E_Illume_Config_Zone *cz, Eina_Bool force);
static void _policy_zone_layout_dialog(E_Border *bd, E_Illume_Config_Zone *cz);
static void _policy_zone_layout_splash(E_Border *bd, E_Illume_Config_Zone *cz);
static void _policy_zone_layout_conformant_single(E_Border *bd, E_Illume_Config_Zone *cz);
#if 0
static void _policy_zone_layout_conformant_dual_top(E_Border *bd, E_Illume_Config_Zone *cz);
static void _policy_zone_layout_conformant_dual_custom(E_Border *bd, E_Illume_Config_Zone *cz);
static void _policy_zone_layout_conformant_dual_left(E_Border *bd, E_Illume_Config_Zone *cz);
#endif

static Eina_List *_pol_focus_stack;

/* local functions */
static void
_policy_border_set_focus(E_Border *bd)
{
   if (!bd) return;
   /* printf("_policy_border_set_focus %s\n", e_border_name_get(bd)); */

   /* if focus is locked out then get out */
   if (bd->lock_focus_out) return;

   if ((bd->client.icccm.accepts_focus) || (bd->client.icccm.take_focus))
     {
	/* check E's focus settings */
	if ((e_config->focus_setting == E_FOCUS_NEW_WINDOW) ||
	    ((bd->parent) &&
	     ((e_config->focus_setting == E_FOCUS_NEW_DIALOG) ||
	      ((bd->parent->focused) &&
	       (e_config->focus_setting == E_FOCUS_NEW_DIALOG_IF_OWNER_FOCUSED)))))
	  {
	     /* if the border was hidden due to layout, we need to unhide */
	     if (!bd->visible) e_illume_border_show(bd);

	     if ((bd->iconic) && (!bd->lock_user_iconify))
	       e_border_uniconify(bd);

	     if (!bd->lock_user_stacking) e_border_raise(bd);

	     /* focus the border */
	     e_border_focus_set(bd, 1, 1);
	     /* e_border_raise(bd); */
	     /* hide the border below this one */
	     _policy_border_hide_below(bd);

	     /* NB: since we skip needless border evals when container layout
	      * is called (to save cpu cycles), we need to
	      * signal this border that it's focused so that the edj gets
	      * updated.
	      *
	      * This is potentially useless as THIS policy
	      * makes all windows borderless anyway, but it's in here for
	      * completeness
	      e_border_focus_latest_set(bd);
	      if (bd->bg_object)
	      edje_object_signal_emit(bd->bg_object, "e,state,focused", "e");
	      if (bd->icon_object)
	      edje_object_signal_emit(bd->icon_object, "e,state,focused", "e");
	      e_focus_event_focus_in(bd);
	     */
	  }
     }
}

static void
_policy_border_move(E_Border *bd, int x, int y)
{
   if (!bd) return;

   bd->x = x;
   bd->y = y;
   bd->changes.pos = 1;
   bd->changed = 1;
}

static void
_policy_border_resize(E_Border *bd, int w, int h)
{
   if (!bd) return;

   bd->w = w;
   bd->h = h;
   bd->client.w = (bd->w - (bd->client_inset.l + bd->client_inset.r));
   bd->client.h = (bd->h - (bd->client_inset.t + bd->client_inset.b));
   bd->changes.size = 1;
   bd->changed = 1;
}

static void
_policy_border_hide_below(E_Border *bd)
{
   int pos = 0, i;

   return;

   //	printf("Hide Borders Below: %s %d %d\n",
   //	       bd->client.icccm.name, bd->x, bd->y);

   if (!bd) return;
   /* printf("_policy_border_hide_below %s\n", e_border_name_get(bd)); */
   /* determine layering position */
   if (bd->layer <= 0) pos = 0;
   else if ((bd->layer > 0) && (bd->layer <= 50)) pos = 1;
   else if ((bd->layer > 50) && (bd->layer <= 100)) pos = 2;
   else if ((bd->layer > 100) && (bd->layer <= 150)) pos = 3;
   else if ((bd->layer > 150) && (bd->layer <= 200)) pos = 4;
   else pos = 5;

   /* Find the windows below this one */
   for (i = pos; i >= 2; i--)
     {
	Eina_List *l;
	E_Border *b;

	EINA_LIST_FOREACH(bd->zone->container->layers[i].clients, l, b)
	  {
	     /* skip if it's the same border */
	     if (b == bd) continue;

	     /* skip if it's not on this zone */
	     if (b->zone != bd->zone) continue;

	     /* skip special borders */
	     if (e_illume_border_is_indicator(b)) continue;
	     if (e_illume_border_is_softkey(b)) continue;
	     if (e_illume_border_is_keyboard(b)) continue;
	     if (e_illume_border_is_quickpanel(b)) continue;
	     if (e_illume_border_is_home(b)) continue;

	     if ((bd->fullscreen) || (bd->need_fullscreen))
	       {
		  if (b->visible) e_illume_border_hide(b);
	       }
	     else
	       {
		  /* we need to check x/y position */
		  if (E_CONTAINS(bd->x, bd->y, bd->w, bd->h,
				 b->x, b->y, b->w, b->h))
		    {
		       if (b->visible) e_illume_border_hide(b);
		    }
	       }
	  }
     }
}

static void
_policy_border_show_below(E_Border *bd)
{
   Eina_List *l;
   E_Border *prev;
   int pos = 0, i;

   //	printf("Show Borders Below: %s %d %d\n",
   //	       bd->client.icccm.class, bd->x, bd->y);

   if (!bd) return;
   /* printf("_policy_border_show_below %s\n", e_border_name_get(bd)); */
   if (bd->client.icccm.transient_for)
     {
	if ((prev = e_border_find_by_client_window(bd->client.icccm.transient_for)))
	  {
	     _policy_border_set_focus(prev);
	     return;
	  }
     }

   /* determine layering position */
   if (bd->layer <= 0) pos = 0;
   else if ((bd->layer > 0) && (bd->layer <= 50)) pos = 1;
   else if ((bd->layer > 50) && (bd->layer <= 100)) pos = 2;
   else if ((bd->layer > 100) && (bd->layer <= 150)) pos = 3;
   else if ((bd->layer > 150) && (bd->layer <= 200)) pos = 4;
   else pos = 5;

   /* Find the windows below this one */
   for (i = pos; i >= 2; i--)
     {
	E_Border *b;

	EINA_LIST_REVERSE_FOREACH(bd->zone->container->layers[i].clients, l, b)
	  {
	     /* skip if it's the same border */
	     if (b == bd) continue;

	     /* skip if it's not on this zone */
	     if (b->zone != bd->zone) continue;

	     /* skip special borders */
	     if (e_illume_border_is_indicator(b)) continue;
	     if (e_illume_border_is_softkey(b)) continue;
	     if (e_illume_border_is_keyboard(b)) continue;
	     if (e_illume_border_is_quickpanel(b)) continue;
	     if (e_illume_border_is_home(b)) continue;

	     if ((bd->fullscreen) || (bd->need_fullscreen))
	       {
		  _policy_border_set_focus(b);
		  return;
	       }
	     else
	       {
		  /* need to check x/y position */
		  if (E_CONTAINS(bd->x, bd->y, bd->w, bd->h,
				 b->x, b->y, b->w, b->h))
		    {
		       _policy_border_set_focus(b);
		       return;
		    }
	       }
	  }
     }

   /* if we reach here, then there is a problem with showing a window below
    * this one, so show previous window in stack */
   EINA_LIST_REVERSE_FOREACH(_pol_focus_stack, l, prev)
     {
	if (prev->zone != bd->zone) continue;
	_policy_border_set_focus(prev);
	return;
     }

   /* Fallback to focusing home if all above fails */
   _policy_focus_home(bd->zone);
}

static void
_policy_zone_layout_update(E_Zone *zone)
{
   Eina_List *l;
   E_Border *bd;

   if (!zone) return;

   EINA_LIST_FOREACH(e_border_client_list(), l, bd)
     {
	/* skip borders not on this zone */
	if (bd->zone != zone) continue;

	/* skip special windows */
	if (e_illume_border_is_keyboard(bd)) continue;
	if (e_illume_border_is_quickpanel(bd)) continue;

	/* signal a changed pos here so layout gets updated */
	bd->changes.pos = 1;
	bd->changed = 1;
     }
}

static void
_policy_zone_layout_indicator(E_Border *bd, E_Illume_Config_Zone *cz)
{
   if ((!bd) || (!cz)) return;

   /* grab minimum indicator size */
   e_illume_border_min_get(bd, NULL, &cz->indicator.size);

   /* no point in doing anything here if indicator is hidden */
   if ((!bd->new_client) && (!bd->visible))
     {
	ecore_x_e_illume_indicator_geometry_set(bd->zone->black_win,
						0, 0, 0, 0);
	return;
     }

   /* if we are dragging, then skip it for now */
   if (bd->client.illume.drag.drag)
     {
	/* when dragging indicator, we need to trigger a layout update */
	ecore_x_e_illume_indicator_geometry_set(bd->zone->black_win,
						0, 0, 0, 0);
	_policy_zone_layout_update(bd->zone);
	return;
     }

   //	printf("\tLayout Indicator: %d\n", bd->zone->id);

   /* lock indicator window from dragging if we need to */
   if ((cz->mode.dual == 1) && (cz->mode.side == 0))
     ecore_x_e_illume_drag_locked_set(bd->client.win, 0);
   else
     ecore_x_e_illume_drag_locked_set(bd->client.win, 1);

   if ((bd->w != bd->zone->w) || (bd->h != cz->indicator.size))
     _policy_border_resize(bd, bd->zone->w, cz->indicator.size);

   if (!cz->mode.dual)
     {
	/* move to 0, 0 (relative to zone) if needed */
	if ((bd->x != bd->zone->x) || (bd->y != bd->zone->y))
	  {
	     _policy_border_move(bd, bd->zone->x, bd->zone->y);
	     ecore_x_e_illume_quickpanel_position_update_send(bd->client.win);
	  }
     }
   else
     {
	if (cz->mode.side == 0)
	  {
	     /* top mode...indicator is draggable so just set X.
	      * in this case, the indicator itself should be responsible for
	      * sending the quickpanel position update message when it is
	      * finished dragging */
	     if (bd->x != bd->zone->x)
	       _policy_border_move(bd, bd->zone->x, bd->y);
	  }
	else
	  {
	     /* move to 0, 0 (relative to zone) if needed */
	     if ((bd->x != bd->zone->x) || (bd->y != bd->zone->y))
	       {
		  _policy_border_move(bd, bd->zone->x, bd->zone->y);
		  ecore_x_e_illume_quickpanel_position_update_send(bd->client.win);
	       }
	  }
     }
   ecore_x_e_illume_indicator_geometry_set(bd->zone->black_win,
					   bd->x, bd->y,
					   bd->w, bd->h);

   if (bd->layer != POL_INDICATOR_LAYER)
     e_border_layer_set(bd, POL_INDICATOR_LAYER);
}

#define ZONE_GEOMETRY					\
  int x, y, w, h;					\
  e_zone_useful_geometry_get(bd->zone, &x, &y, &w, &h); \
  x += bd->zone->x;					\
  y += bd->zone->y;					\

static void
_border_geometry_set(E_Border *bd, int x, int y, int w, int h, int layer)
{
   if ((bd->w != w) || (bd->h != h))
     _policy_border_resize(bd, w, h);

   if ((bd->x != x) || (bd->y != y))
     _policy_border_move(bd, x, y);

   if ((int)bd->layer != layer) e_border_layer_set(bd, layer);
}

static void
_policy_zone_layout_quickpanel(E_Border *bd)
{
   int mh;

   if (!bd) return;

   e_illume_border_min_get(bd, NULL, &mh);

   if ((bd->w != bd->zone->w) || (bd->h != mh))
     _policy_border_resize(bd, bd->zone->w, mh);

   if (bd->layer != POL_QUICKPANEL_LAYER)
     e_border_layer_set(bd, POL_QUICKPANEL_LAYER);
}

static void
_policy_zone_layout_softkey(E_Border *bd, E_Illume_Config_Zone *cz)
{
   int ny;

   if ((!bd) || (!cz)) return;
   if (!bd->visible)
     {
	ecore_x_e_illume_softkey_geometry_set(bd->zone->black_win, 0, 0, 0, 0);
	return;
     }

   ZONE_GEOMETRY;

   e_illume_border_min_get(bd, NULL, &cz->softkey.size);

   /* if we are dragging, then skip it for now */
   /* NB: Disabled currently until we confirm that softkey should be draggable */
   //	if (bd->client.illume.drag.drag) return;

   if ((bd->w != w) || (bd->h != cz->softkey.size))
     _policy_border_resize(bd, w, cz->softkey.size);

   ny = ((bd->zone->y + bd->zone->h) - cz->softkey.size);

   /* NB: not sure why yet, but on startup the border->y is reporting
    * that it is already in this position...but it's actually not.
    * So for now, just disable the ny check until this gets sorted out */

   if ((bd->x != x) || (bd->y != ny))
     _policy_border_move(bd, x, ny);

   /* _border_geometry_set(bd, x, ny, nw, nh, bd, POL_CONFORMANT_LAYER); */

   ecore_x_e_illume_softkey_geometry_set(bd->zone->black_win,
					 bd->x, bd->y,
					 bd->w, bd->h);

   if (bd->layer != POL_SOFTKEY_LAYER) e_border_layer_set(bd, POL_SOFTKEY_LAYER);
}



#define MIN_HEIGHT 100

static Eina_Bool
_policy_layout_app_check(E_Border *bd)
{
   if (!bd)
     return EINA_FALSE;

   if (!bd->visible)
     return EINA_FALSE;

   if ((bd->desk != e_desk_current_get(bd->zone)) && (!bd->sticky))
     return EINA_FALSE;

   return EINA_TRUE;
}


static void
_policy_keyboard_restrict(E_Border *bd, int *h)
{
   int kh;

   if (bd->client.vkbd.state > ECORE_X_VIRTUAL_KEYBOARD_STATE_OFF)
     {
	e_illume_keyboard_safe_app_region_get(bd->zone, NULL, NULL, NULL, &kh);
	kh -= bd->zone->h - *h;
	if ((kh < *h) && (kh > MIN_HEIGHT))
	  *h = kh;
     }
}

static void
_policy_indicator_restrict(E_Illume_Config_Zone *cz, int *y, int *h)
{
   if ((cz->indicator.size) && (*y < cz->indicator.size))
     {
	*h -= cz->indicator.size;
	*y = cz->indicator.size;
     }
}

static void
_policy_softkey_restrict(E_Illume_Config_Zone *cz, int *y, int *h)
{
   if ((cz->softkey.size) && ((*y + *h) > cz->softkey.size))
     *h -= (*y + *h) - cz->softkey.size;
}

static void
_policy_zone_layout_keyboard(E_Border *bd, E_Illume_Config_Zone *cz)
{
   int ny, layer;

   if ((!bd) || (!cz) || (!bd->visible)) return;

   ZONE_GEOMETRY;

   e_illume_border_min_get(bd, NULL, &cz->vkbd.size);

   ny = ((bd->zone->y + bd->zone->h) - cz->vkbd.size);

   /* if ((bd->fullscreen) || (bd->need_fullscreen))
    *   layer = POL_FULLSCREEN_LAYER;
    * else */
   layer = POL_KEYBOARD_LAYER;
   
   _border_geometry_set(bd, x, ny, w, cz->vkbd.size, layer);
}

static void
_policy_zone_layout_home_single(E_Border *bd, E_Illume_Config_Zone *cz)
{
   if ((!bd) || (!cz) || (!bd->visible)) return;

   ZONE_GEOMETRY;
   _policy_indicator_restrict(cz, &y, &h);
   _border_geometry_set(bd, x, y, w, h, POL_HOME_LAYER);
}

static void
_policy_zone_layout_fullscreen(E_Border *bd)
{
   if (!_policy_layout_app_check(bd)) return;

   ZONE_GEOMETRY;

   _policy_keyboard_restrict(bd, &h);

   _border_geometry_set(bd, x, y, w, h, POL_FULLSCREEN_LAYER);
}

static void
_policy_zone_layout_app_single(E_Border *bd, E_Illume_Config_Zone *cz)
{
   if (!_policy_layout_app_check(bd)) return;

   ZONE_GEOMETRY;

   _policy_keyboard_restrict(bd, &h);
   _policy_indicator_restrict(cz, &y, &h);
   _policy_softkey_restrict(cz, &y, &h);

   _border_geometry_set(bd, x, y, w, h, POL_APP_LAYER);
}

static void
_policy_zone_layout_app_dual_top(E_Border *bd, E_Illume_Config_Zone *cz)
{
   E_Border *bd2;

   if (!_policy_layout_app_check(bd)) return;

   ZONE_GEOMETRY;

   _policy_keyboard_restrict(bd, &h);
   _policy_indicator_restrict(cz, &y, &h);
   _policy_softkey_restrict(cz, &y, &h);

   bd2 = e_illume_border_at_xy_get(bd->zone, x, y);
   if ((bd2) && (bd2 != bd))
     {
	if ((bd->focused) && (bd->client.vkbd.state > ECORE_X_VIRTUAL_KEYBOARD_STATE_OFF))
	  _border_geometry_set(bd2, x, h/2 + y, w, h/2, POL_APP_LAYER);
	else
	  y += h/2;
     }

   _border_geometry_set(bd, x, y, w, h/2, POL_APP_LAYER);
}

static void
_policy_zone_layout_app_dual_custom(E_Border *bd, E_Illume_Config_Zone *cz)
{
   E_Border *app;
   int iy, ny, nh;

   if (!_policy_layout_app_check(bd)) return;

   ZONE_GEOMETRY;

   e_illume_border_indicator_pos_get(bd->zone, NULL, &iy);

   ny = bd->zone->y;
   nh = iy;

   app = e_illume_border_at_xy_get(bd->zone, bd->zone->x, bd->zone->y);
   if ((app) && (bd != app))
     {
	ny = (iy + cz->indicator.size);
	nh = ((bd->zone->y + bd->zone->h) - ny - cz->softkey.size);
     }

   _border_geometry_set(bd, x, ny, w, nh, POL_APP_LAYER);
}

static void
_policy_zone_layout_app_dual_left(E_Border *bd, E_Illume_Config_Zone *cz, Eina_Bool force)
{
   E_Border *bd2;
   int ky, kh, nx, nw;

   if (!_policy_layout_app_check(bd)) return;

   ZONE_GEOMETRY;

   e_illume_keyboard_safe_app_region_get(bd->zone, NULL, &ky, NULL, &kh);

   if (kh >= bd->zone->h)
     kh = (kh - cz->indicator.size - cz->softkey.size);
   else
     kh = (kh - cz->indicator.size);

   nx = x;
   nw = w / 2;

   if (!force)
     {
	/* see if there is a border already there. if so, place at right */
	bd2 = e_illume_border_at_xy_get(bd->zone, nx, y);
	if ((bd2) && (bd != bd2)) nx = x + nw;
     }

   _border_geometry_set(bd, nx, y, nw, kh, POL_APP_LAYER);
}

static void
_policy_zone_layout_dialog(E_Border *bd, E_Illume_Config_Zone *cz)
{
   E_Border *parent;
   int mw, mh, nx, ny;

   /* if (!_policy_layout_app_check(bd)) return; */
   if ((!bd) || (!cz)) return;
   printf("place dialog %d - %dx%d\n", bd->placed, bd->w, bd->h);

   if (bd->placed)
     return;

   ZONE_GEOMETRY;

   mw = bd->w;
   mh = bd->h;

   if (e_illume_border_is_fixed_size(bd))
     {
	if (mw > w) mw = w;
	if (mh > h) mh = h;
     }
   else
     {
	if (w * 2/3 > bd->w)
	  mw = w * 2/3;

	if (h * 2/3 > bd->h)
	  mh = h * 2/3;
     }

   parent = e_illume_border_parent_get(bd);

   if ((!parent) || (cz->mode.dual == 1))
     {
	nx = (x + ((w - mw) / 2));
	ny = (y + ((h - mh) / 2));
     }
   else
     {
	if (mw > parent->w) mw = parent->w;
	if (mh > parent->h) mh = parent->h;

	nx = (parent->x + ((parent->w - mw) / 2));
	ny = (parent->y + ((parent->h - mh) / 2));
     }

   bd->placed = 1;
   
   _border_geometry_set(bd, nx, ny, mw, mh, POL_DIALOG_LAYER);
   printf("set geom %d %d\n", mw, mh);
}

static void
_policy_zone_layout_splash(E_Border *bd, E_Illume_Config_Zone *cz)
{
   _policy_zone_layout_dialog(bd, cz);

   if (bd->layer != POL_SPLASH_LAYER) e_border_layer_set(bd, POL_SPLASH_LAYER);
}

static void
_policy_zone_layout_conformant_single(E_Border *bd, E_Illume_Config_Zone *cz __UNUSED__)
{
   if (!_policy_layout_app_check(bd)) return;

   _border_geometry_set(bd, bd->zone->x, bd->zone->y,
			bd->zone->w, bd->zone->h,
			POL_CONFORMANT_LAYER);
}

#if 0
typedef struct _App_Desk App_Desk;

struct _App_Desk
{
  E_Desk *desk;
  const char *class;
  Eina_List *borders;
};

static Eina_List *desks = NULL;

#define EINA_LIST_FIND(_list, _item, _match)	\
  {						\
     Eina_List *_l;				\
     EINA_LIST_FOREACH(_list, _l, _item)	\
       if (_match) break;			\
  }
#endif


/* policy functions */
void
_policy_border_add(E_Border *bd)
{
   //	printf("Border added: %s\n", bd->client.icccm.class);

   if (!bd) return;

   /* NB: this call sets an atom on the window that specifices the zone.
    * the logic here is that any new windows created can access the zone
    * window by a 'get' call. This is useful for elementary apps as they
    * normally would not have access to the zone window. Typical use case
    * is for indicator & softkey windows so that they can send messages
    * that apply to their respective zone only. Example: softkey sends close
    * messages (or back messages to cycle focus) that should pertain just
    * to it's current zone */
   ecore_x_e_illume_zone_set(bd->client.win, bd->zone->black_win);

   if (e_illume_border_is_keyboard(bd))
     {
	bd->sticky = 1;
	e_hints_window_sticky_set(bd, 1);
     }
   
   if (e_illume_border_is_home(bd))
     {
	bd->sticky = 1;
	e_hints_window_sticky_set(bd, 1);
     }

   if (e_illume_border_is_indicator(bd))
     {
	bd->sticky = 1;
	e_hints_window_sticky_set(bd, 1);
     }

   if (e_illume_border_is_softkey(bd))
     {
	bd->sticky = 1;
	e_hints_window_sticky_set(bd, 1);
     }

   /* ignore stolen borders. These are typically quickpanel or keyboards */
   if (bd->stolen) return;

   /* if this is a fullscreen window, than we need to hide indicator window */
   /* NB: we could use the e_illume_border_is_fullscreen function here
    * but we save ourselves a function call this way */
   if ((bd->fullscreen) || (bd->need_fullscreen))
     {
	E_Border *ind, *sft;

	if ((ind = e_illume_border_indicator_get(bd->zone)))
	  {
	     if (ind->visible) e_illume_border_hide(ind);
	  }
	if ((sft = e_illume_border_softkey_get(bd->zone)))
	  {
	     if (e_illume_border_is_conformant(bd))
	       {
		  if (sft->visible)
		    e_illume_border_hide(sft);
		  else if (!sft->visible)
		    e_illume_border_show(sft);
	       }
	  }
     }


#if 0
   if (bd->client.icccm.class)
     {
   	Eina_List *l;
   	App_Desk *d;

   	EINA_LIST_FIND(desks, d, (d->class == bd->client.icccm.class));

   	if (!d)
   	  {
   	     d = E_NEW(App_Desk, 1);
   	     d->desk
   	  }

   	d->borders = eina_list_append(d->borders, bd);
   	e_border_desk_set(bd, d->desk);
     }
#endif

   /* Add this border to our focus stack if it can accept or take focus */
   if ((bd->client.icccm.accepts_focus) || (bd->client.icccm.take_focus))
     _pol_focus_stack = eina_list_append(_pol_focus_stack, bd);

   if ((e_illume_border_is_softkey(bd)) || (e_illume_border_is_indicator(bd)))
     _policy_zone_layout_update(bd->zone);
   else
     {
	/* set focus on new border if we can */
	_policy_border_set_focus(bd);
     }
}

void
_policy_border_del(E_Border *bd)
{
   //	printf("Policy Border deleted: %s\n", bd->client.icccm.class);

   if (!bd) return;

   /* if this is a fullscreen window, than we need to show indicator window */
   /* NB: we could use the e_illume_border_is_fullscreen function here
    * but we save ourselves a function call this way */
   if ((bd->fullscreen) || (bd->need_fullscreen))
     {
	E_Border *ind;

	/* try to get the Indicator on this zone */
	if ((ind = e_illume_border_indicator_get(bd->zone)))
	  {
	     if (!ind->visible) e_illume_border_show(ind);
	  }
	_policy_zone_layout_update(bd->zone);
     }

   /* remove from focus stack */
   if ((bd->client.icccm.accepts_focus) || (bd->client.icccm.take_focus))
     _pol_focus_stack = eina_list_remove(_pol_focus_stack, bd);

   if (e_illume_border_is_softkey(bd))
     {
	E_Illume_Config_Zone *cz;

	cz = e_illume_zone_config_get(bd->zone->id);
	cz->softkey.size = 0;
	_policy_zone_layout_update(bd->zone);
     }
   else if (e_illume_border_is_indicator(bd))
     {
	E_Illume_Config_Zone *cz;

	cz = e_illume_zone_config_get(bd->zone->id);
	cz->indicator.size = 0;
	_policy_zone_layout_update(bd->zone);
     }
   /* else
    *   {
    * 	_policy_border_show_below(bd);
    *   } */
}

void
_policy_border_focus_in(E_Border *bd __UNUSED__)
{
   E_Border *ind;

   //	printf("Border focus in: %s\n", bd->client.icccm.name);
   if ((bd->fullscreen) || (bd->need_fullscreen))
     {
	/* try to get the Indicator on this zone */
	if ((ind = e_illume_border_indicator_get(bd->zone)))
	  {
	     /* we have the indicator, show it if needed */
	     if (ind->visible) e_illume_border_hide(ind);
	  }
     }
   else
     {
	/* try to get the Indicator on this zone */
	if ((ind = e_illume_border_indicator_get(bd->zone)))
	  {
	     /* we have the indicator, show it if needed */
	     if (!ind->visible) e_illume_border_show(ind);
	  }
     }
   _policy_zone_layout_update(bd->zone);
}

void
_policy_border_focus_out(E_Border *bd)
{
   //	printf("Border focus out: %s\n", bd->client.icccm.name);

   if (!bd) return;

   /* NB: if we got this focus_out event on a deleted border, we check if
    * it is a transient (child) of another window. If it is, then we
    * transfer focus back to the parent window */
   if (e_object_is_del(E_OBJECT(bd)))
     {
	if (e_illume_border_is_dialog(bd))
	  {
	     E_Border *parent;

	     if ((parent = e_illume_border_parent_get(bd)))
	       _policy_border_set_focus(parent);
	  }
     }
}

void
_policy_border_activate(E_Border *bd)
{
   E_Border *sft;

   //	printf("Border Activate: %s\n", bd->client.icccm.name);

   if (!bd) return;

   /* NB: stolen borders may or may not need focus call...have to test */
   if (bd->stolen) return;

   /* conformant windows hide the softkey */
   if ((sft = e_illume_border_softkey_get(bd->zone)))
     {
	if (e_illume_border_is_conformant(bd))
	  {
	     if (sft->visible) e_illume_border_hide(sft);
	  }
	else
	  {
	     if (!sft->visible) e_illume_border_show(sft);
	  }
     }

   /* NB: We cannot use our set_focus function here because it does,
    * occasionally fall through wrt E's focus policy, so cherry pick the good
    * parts and use here :) */
   if (bd->desk != e_desk_current_get(bd->zone))
     e_desk_show(bd->desk);

   /* if the border is iconified then uniconify if allowed */
   if ((bd->iconic) && (!bd->lock_user_iconify))
     e_border_uniconify(bd);

   /* set very high layer for this window as it needs attention and thus
    * should show above everything */
   e_border_layer_set(bd, POL_ACTIVATE_LAYER);

   /* if we can raise the border do it */
   if (!bd->lock_user_stacking) e_border_raise(bd);

   /* focus the border */
   e_border_focus_set(bd, 1, 1);

   /* NB: since we skip needless border evals when container layout
    * is called (to save cpu cycles), we need to
    * signal this border that it's focused so that the edj gets
    * updated.
    *
    * This is potentially useless as THIS policy
    * makes all windows borderless anyway, but it's in here for
    * completeness
    e_border_focus_latest_set(bd);
    if (bd->bg_object)
    edje_object_signal_emit(bd->bg_object, "e,state,focused", "e");
    if (bd->icon_object)
    edje_object_signal_emit(bd->icon_object, "e,state,focused", "e");
    e_focus_event_focus_in(bd);
   */
}
static Eina_Bool
_policy_border_is_dialog(E_Border *bd)
{
   if (e_illume_border_is_dialog(bd))
     return EINA_TRUE;

   if (bd->client.e.state.centered)
     return EINA_TRUE;

   if (bd->internal)
     {
	if (bd->client.icccm.class)
	  {
	     if (!strncmp(bd->client.icccm.class, "Illume", 6))
	       return EINA_FALSE;
	     if (!strncmp(bd->client.icccm.class, "e_fwin", 6))
	       return EINA_FALSE;
	     if (!strncmp(bd->client.icccm.class, "every", 5))
	       return EINA_FALSE;

	  }
	return EINA_TRUE;
     }

   return EINA_FALSE;
}

void
_policy_border_post_fetch(E_Border *bd)
{
   if (!bd) return;

   /* NB: for this policy we disable all remembers set on a border */
   if (bd->remember) e_remember_del(bd->remember);
   bd->remember = NULL;

   if (_policy_border_is_dialog(bd))
     {
	return;
     }
   else if (e_illume_border_is_fixed_size(bd))
     {
	return;
     }
   else if (!bd->borderless)
     {
	bd->borderless = 1;
	bd->client.border.changed = 1;
     }
}

void
_policy_border_post_assign(E_Border *bd)
{
   if (!bd) return;

   bd->internal_no_remember = 1;

   if (_policy_border_is_dialog(bd) ||
       e_illume_border_is_fixed_size(bd))
     return;

   /* do not allow client to change these properties */
   bd->lock_client_size = 1;
   bd->lock_client_shade = 1;
   bd->lock_client_maximize = 1;
   bd->lock_client_location = 1;
   bd->lock_client_stacking = 1;

   /* do not allow the user to change these properties */
   /* bd->lock_user_location = 1;
    * bd->lock_user_size = 1;
    * bd->lock_user_shade = 1; */

   /* clear any centered states */
   /* NB: this is mainly needed for E's main config dialog */
   bd->client.e.state.centered = 0;

   /* lock the border type so user/client cannot change */
   bd->lock_border = 1;

   /* disable e's placement (and honoring of icccm.request_pos) */
   bd->placed = 1;
}

void
_policy_border_show(E_Border *bd)
{
   if (!bd) return;

   /* printf("_policy_border_show %s\n", e_border_name_get(bd)); */

   if (!bd->client.icccm.name) return;

   //	printf("Border Show: %s\n", bd->client.icccm.class);

   /* trap for special windows so we can ignore hides below them */
   if (e_illume_border_is_indicator(bd)) return;
   if (e_illume_border_is_softkey(bd)) return;
   if (e_illume_border_is_quickpanel(bd)) return;
   if (e_illume_border_is_keyboard(bd)) return;

   _policy_border_hide_below(bd);
}

/* called on E_BORDER_HOOK_CONTAINER_LAYOUT (after e_border/eval0) */
void
_policy_zone_layout(E_Zone *zone)
{
   E_Illume_Config_Zone *cz;
   Eina_List *l;
   E_Border *bd;

   if (!zone) return;

   cz = e_illume_zone_config_get(zone->id);

   EINA_LIST_FOREACH(e_border_client_list(), l, bd)
     {
	if (e_object_is_del(E_OBJECT(bd))) continue;

	if (bd->zone != zone) continue;

	if ((!bd->new_client) && (!bd->changes.pos) && (!bd->changes.size) &&
	    (!bd->changes.visible) && (!bd->pending_move_resize) &&
	    (!bd->need_shape_export) && (!bd->need_shape_merge)) continue;

	if (e_illume_border_is_indicator(bd))
	  _policy_zone_layout_indicator(bd, cz);

	else if (e_illume_border_is_quickpanel(bd))
	  _policy_zone_layout_quickpanel(bd);

	else if (e_illume_border_is_softkey(bd))
	  _policy_zone_layout_softkey(bd, cz);

	else if (e_illume_border_is_keyboard(bd))
	  _policy_zone_layout_keyboard(bd, cz);

	else if (e_illume_border_is_home(bd))
	  _policy_zone_layout_home_single(bd, cz);

	else if ((bd->fullscreen) || (bd->need_fullscreen))
	  _policy_zone_layout_fullscreen(bd);

	else if (e_illume_border_is_splash(bd))
	  _policy_zone_layout_splash(bd, cz);

	else if (_policy_border_is_dialog(bd))
	  _policy_zone_layout_dialog(bd, cz);

	else if (e_illume_border_is_conformant(bd))
	  _policy_zone_layout_conformant_single(bd, cz);

	else if (e_illume_border_is_fixed_size(bd))
	  _policy_zone_layout_dialog(bd, cz);

	else if (bd->internal && bd->client.icccm.class &&
		 (!strcmp(bd->client.icccm.class, "everything-window")))
	  {
	     if (bd->client.vkbd.state == ECORE_X_VIRTUAL_KEYBOARD_STATE_ON)
	       _policy_zone_layout_app_single(bd, cz);
	     /* else
	      *   _policy_zone_layout_app_dual_left(bd, cz, EINA_TRUE); */
	     if (bd->layer != POL_ACTIVATE_LAYER)
	       e_border_layer_set(bd, POL_ACTIVATE_LAYER);
	     
	     /* if (bd->layer != POL_SPLASH_LAYER)
	      *   e_border_layer_set(bd, POL_SPLASH_LAYER); */
	  }
	else if (bd->client.e.state.centered)
	  _policy_zone_layout_dialog(bd, cz);
	else if (!cz->mode.dual)
	  _policy_zone_layout_app_single(bd, cz);
	else
	  {
	     if (cz->mode.side == 0)
	       {
		  int ty;

		  /* grab the indicator position so we can tell if it
		   * is in a custom position or not (user dragged it) */
		  e_illume_border_indicator_pos_get(bd->zone, NULL, &ty);
		  if (ty <= bd->zone->y)
		    _policy_zone_layout_app_dual_top(bd, cz);
		  else
		    _policy_zone_layout_app_dual_custom(bd, cz);
	       }
	     else
	       _policy_zone_layout_app_dual_left(bd, cz, EINA_FALSE);
	  }
     }
}

void
_policy_zone_move_resize(E_Zone *zone)
{
   Eina_List *l;
   E_Border *bd;

   if (!zone) return;

   EINA_LIST_FOREACH(e_border_client_list(), l, bd)
     {
	if (bd->zone != zone) continue;

	bd->changes.pos = 1;
	bd->changed = 1;
     }
}

void
_policy_zone_mode_change(E_Zone *zone, Ecore_X_Atom mode)
{
   E_Illume_Config_Zone *cz;
   /* Eina_List *homes = NULL; */
   E_Border *bd;
   /* int count; */

   if (!zone) return;

   cz = e_illume_zone_config_get(zone->id);

   if (mode == ECORE_X_ATOM_E_ILLUME_MODE_SINGLE)
     cz->mode.dual = 0;
   else
     {
	cz->mode.dual = 1;
	if (mode == ECORE_X_ATOM_E_ILLUME_MODE_DUAL_TOP)
	  cz->mode.side = 0;
	else if (mode == ECORE_X_ATOM_E_ILLUME_MODE_DUAL_LEFT)
	  cz->mode.side = 1;
     }
   e_config_save_queue();

   /* lock indicator window from dragging if we need to */
   bd = e_illume_border_indicator_get(zone);
   if (bd)
     {
	/* only dual-top mode can drag */
	if ((cz->mode.dual == 1) && (cz->mode.side == 0))
	  {
	     /* only set locked if we need to */
	     if (bd->client.illume.drag.locked != 0)
	       ecore_x_e_illume_drag_locked_set(bd->client.win, 0);
	  }
	else
	  {
	     /* only set locked if we need to */
	     if (bd->client.illume.drag.locked != 1)
	       ecore_x_e_illume_drag_locked_set(bd->client.win, 1);
	  }
     }
#if 0 /* split home window? wtf?! go home! */
   if (!(homes = e_illume_border_home_borders_get(zone))) return;

   count = eina_list_count(homes);

   /* create a new home window (if needed) for dual mode */
   if (cz->mode.dual == 1)
     {
	if (count < 2)
	  ecore_x_e_illume_home_new_send(zone->black_win);
     }
   else if (cz->mode.dual == 0)
     {
	/* if we went to single mode, delete any extra home windows */
	if (count >= 2)
	  {
	     E_Border *home;

	     /* try to get a home window on this zone and remove it */
	     if ((home = e_illume_border_home_get(zone)))
	       ecore_x_e_illume_home_del_send(home->client.win);
	  }
     }
#endif

   _policy_zone_layout_update(zone);
}

void
_policy_zone_close(E_Zone *zone)
{
   E_Border *bd;

   if (!zone) return;

   if (!(bd = e_border_focused_get())) return;

   if (bd->zone != zone) return;

   /* close this border */
   e_border_act_close_begin(bd);
}

void
_policy_drag_start(E_Border *bd)
{
   if (!bd) return;

   if (bd->stolen) return;

   ecore_x_e_illume_drag_set(bd->client.win, 1);
   ecore_x_e_illume_drag_set(bd->zone->black_win, 1);
}

void
_policy_drag_end(E_Border *bd)
{
   //	printf("Drag end\n");

   if (!bd) return;

   if (bd->stolen) return;

   /* set property on this border to say we are done dragging */
   ecore_x_e_illume_drag_set(bd->client.win, 0);

   /* set property on zone window that a drag is finished */
   ecore_x_e_illume_drag_set(bd->zone->black_win, 0);
}

void
_policy_focus_back(E_Zone *zone)
{
   Eina_List *l, *fl = NULL;
   E_Border *bd, *fbd;

   if (!zone) return;
   if (eina_list_count(_pol_focus_stack) < 1) return;

   //	printf("Focus back\n");

   EINA_LIST_REVERSE_FOREACH(_pol_focus_stack, l, bd)
     {
	if (bd->zone != zone) continue;
	fl = eina_list_append(fl, bd);
     }

   if (!(fbd = e_border_focused_get())) return;
   if (fbd->parent) return;

   EINA_LIST_REVERSE_FOREACH(fl, l, bd)
     {
	if ((fbd) && (bd == fbd))
	  {
	     E_Border *b;

	     if ((l->next) && (b = l->next->data))
	       {
		  _policy_border_set_focus(b);
		  break;
	       }
	     else
	       {
		  /* we've reached the end of the list. Set focus to first */
		  if ((b = eina_list_nth(fl, 0)))
		    {
		       _policy_border_set_focus(b);
		       break;
		    }
	       }
	  }
     }
   eina_list_free(fl);
}

void
_policy_focus_forward(E_Zone *zone)
{
   Eina_List *l, *fl = NULL;
   E_Border *bd, *fbd;

   if (!zone) return;
   if (eina_list_count(_pol_focus_stack) < 1) return;

   //	printf("Focus forward\n");

   EINA_LIST_FOREACH(_pol_focus_stack, l, bd)
     {
	if (bd->zone != zone) continue;
	fl = eina_list_append(fl, bd);
     }

   if (!(fbd = e_border_focused_get())) return;
   if (fbd->parent) return;

   EINA_LIST_FOREACH(fl, l, bd)
     {
	if ((fbd) && (bd == fbd))
	  {
	     E_Border *b;

	     if ((l->next) && (b = l->next->data))
	       {
		  _policy_border_set_focus(b);
		  break;
	       }
	     else
	       {
		  /* we've reached the end of the list. Set focus to first */
		  if ((b = eina_list_nth(fl, 0)))
		    {
		       _policy_border_set_focus(b);
		       break;
		    }
	       }
	  }
     }
   eina_list_free(fl);
}

void
_policy_focus_home(E_Zone *zone)
{
   E_Border *bd;

   if (!zone) return;
   if (!(bd = e_illume_border_home_get(zone))) return;
   _policy_border_set_focus(bd);
}

void
_policy_property_change(Ecore_X_Event_Window_Property *event)
{
   if (event->atom == ECORE_X_ATOM_NET_WM_STATE)
     {
	E_Border *bd, *ind;

	if (!(bd = e_border_find_by_client_window(event->win))) return;

	/* not interested in stolen or invisible borders */
	if ((bd->stolen) || (!bd->visible)) return;

	/* make sure the border has a name or class */
	/* NB: this check is here because some E borders get State Changes
	 * but do not have a name/class associated with them. Not entirely sure
	 * which ones they are, but I would guess Managers, Containers, or Zones.
	 * At any rate, we're not interested in those types of borders */
	if ((!bd->client.icccm.name) || (!bd->client.icccm.class)) return;

	/* NB: If we have reached this point, then it should be a fullscreen
	 * border that has toggled fullscreen on/off */

	/* try to get the Indicator on this zone */
	if (!(ind = e_illume_border_indicator_get(bd->zone))) return;

	/* if we are fullscreen, hide the indicator...else we show it */
	/* NB: we could use the e_illume_border_is_fullscreen function here
	 * but we save ourselves a function call this way */
	if ((bd->fullscreen) || (bd->need_fullscreen))
	  {
	     if (ind->visible)
	       {
		  e_illume_border_hide(ind);
		  _policy_zone_layout_update(bd->zone);
	       }
	  }
	else
	  {
	     if (!ind->visible)
	       {
		  e_illume_border_show(ind);
		  _policy_zone_layout_update(bd->zone);
	       }
	  }
     }
   else if (event->atom == ECORE_X_ATOM_E_ILLUME_INDICATOR_GEOMETRY)
     {
	Eina_List *l;
	E_Zone *zone;
	E_Border *bd;
	int x, y, w, h;

	if (!(zone = e_util_zone_window_find(event->win))) return;

	if (!(bd = e_illume_border_indicator_get(zone))) return;
	x = bd->x;
	y = bd->y;
	w = bd->w;
	h = bd->h;

	EINA_LIST_FOREACH(e_border_client_list(), l, bd)
	  {
	     if (bd->zone != zone) continue;
	     if (!e_illume_border_is_conformant(bd)) continue;
	     /* set indicator geometry on conformant window */
	     /* NB: This is needed so that conformant apps get told about
	      * the indicator size/position...else they have no way of
	      * knowing that the geometry has been updated */
	     ecore_x_e_illume_indicator_geometry_set(bd->client.win, x, y, w, h);
	  }
     }
   else if (event->atom == ECORE_X_ATOM_E_ILLUME_SOFTKEY_GEOMETRY)
     {
	Eina_List *l;
	E_Zone *zone;
	E_Border *bd;
	int x, y, w, h;

	if (!(zone = e_util_zone_window_find(event->win))) return;

	if (!(bd = e_illume_border_softkey_get(zone))) return;
	x = bd->x;
	y = bd->y;
	w = bd->w;
	h = bd->h;

	EINA_LIST_FOREACH(e_border_client_list(), l, bd)
	  {
	     if (bd->zone != zone) continue;
	     if (!e_illume_border_is_conformant(bd)) continue;
	     /* set softkey geometry on conformant window */
	     /* NB: This is needed so that conformant apps get told about
	      * the softkey size/position...else they have no way of
	      * knowing that the geometry has been updated */
	     ecore_x_e_illume_softkey_geometry_set(bd->client.win, x, y, w, h);
	  }
     }
   else if (event->atom == ECORE_X_ATOM_E_ILLUME_KEYBOARD_GEOMETRY)
     {
	Eina_List *l;
	E_Zone *zone;
	E_Illume_Keyboard *kbd;
	E_Border *bd;
	int x, y, w, h;

	if (!(zone = e_util_zone_window_find(event->win))) return;

	if (!(kbd = e_illume_keyboard_get())) return;
	if (!kbd->border) return;

	x = kbd->border->x;
	w = kbd->border->w;
	h = kbd->border->h;

	/* adjust Y for keyboard visibility because keyboard uses fx_offset */
	y = 0;
	if (kbd->border->fx.y <= 0) y = kbd->border->y;

	/* look for conformant borders */
	EINA_LIST_FOREACH(e_border_client_list(), l, bd)
	  {
	     if (bd->zone != zone) continue;
	     if (!e_illume_border_is_conformant(bd)) continue;
	     /* set keyboard geometry on conformant window */
	     /* NB: This is needed so that conformant apps get told about
	      * the keyboard size/position...else they have no way of
	      * knowing that the geometry has been updated */
	     ecore_x_e_illume_keyboard_geometry_set(bd->client.win, x, y, w, h);
	  }
     }
   else if (event->atom == ATM_ENLIGHTENMENT_SCALE)
     {
	Eina_List *ml;
	E_Manager *man;

	EINA_LIST_FOREACH(e_manager_list(), ml, man)
	  {
	     Eina_List *cl;
	     E_Container *con;

	     if (event->win != man->root) continue;
	     EINA_LIST_FOREACH(man->containers, cl, con)
	       {
		  Eina_List *zl;
		  E_Zone *zone;

		  EINA_LIST_FOREACH(con->zones, zl, zone)
		    _policy_zone_layout_update(zone);
	       }
	  }
     }
}
