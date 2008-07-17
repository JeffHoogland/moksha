/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* local subsystem functions */
static void _e_win_free(E_Win *win);
static void _e_win_del(void *obj);
static void _e_win_prop_update(E_Win *win);
static void _e_win_state_update(E_Win *win);
static void _e_win_cb_move(Ecore_Evas *ee);
static void _e_win_cb_resize(Ecore_Evas *ee);
static void _e_win_cb_delete(Ecore_Evas *ee);

/* local subsystem globals */
static Evas_List *wins = NULL;

/* externally accessible functions */
EAPI int
e_win_init(void)
{
   return 1;
}

EAPI int
e_win_shutdown(void)
{
/*   
   while (wins)
     {
	e_object_del(E_OBJECT(wins->data));
     }
*/
   return 1;
}

EAPI E_Win *
e_win_new(E_Container *con)
{
   E_Win *win;
   Evas_Object *obj;
   
   win = E_OBJECT_ALLOC(E_Win, E_WIN_TYPE, _e_win_free);
   if (!win) return NULL;
   e_object_del_func_set(E_OBJECT(win), _e_win_del);
   win->container = con;
   win->engine = e_canvas_engine_decide(e_config->evas_engine_win);
   win->ecore_evas = e_canvas_new(e_config->evas_engine_win, con->manager->root,
				  0, 0, 1, 1, 1, 0,
				  &(win->evas_win), NULL);
   e_canvas_add(win->ecore_evas);
   ecore_evas_data_set(win->ecore_evas, "E_Win", win);
   ecore_evas_callback_move_set(win->ecore_evas, _e_win_cb_move);
   ecore_evas_callback_resize_set(win->ecore_evas, _e_win_cb_resize);
   ecore_evas_callback_delete_request_set(win->ecore_evas, _e_win_cb_delete);
   win->evas = ecore_evas_get(win->ecore_evas);
   ecore_evas_name_class_set(win->ecore_evas, "E", "_e_internal_window");
   ecore_evas_title_set(win->ecore_evas, "E");
   obj = evas_object_rectangle_add(win->evas);
   evas_object_name_set(obj, "E_Win");
   evas_object_data_set(obj, "E_Win", win);
   win->x = 0;
   win->y = 0;
   win->w = 1;
   win->h = 1;
   win->placed = 0;
   win->min_w = 0;
   win->min_h = 0;
   win->max_w = 9999;
   win->max_h = 9999;
   win->base_w = 0;
   win->base_h = 0;
   win->step_x = 1;
   win->step_y = 1;
   win->min_aspect = 0.0;
   win->max_aspect = 0.0;
   wins = evas_list_append(wins, win);
   
   win->pointer = e_pointer_window_new(win->evas_win, 0);
   return win;
}

EAPI void
e_win_show(E_Win *win)
{
   E_OBJECT_CHECK(win);
   E_OBJECT_TYPE_CHECK(win, E_WIN_TYPE);
   if (!win->border)
     {
	_e_win_prop_update(win);
	ecore_evas_lower(win->ecore_evas);
	win->border = e_border_new(win->container, win->evas_win, 1, 1);
	if (!win->placed)
	  win->border->re_manage = 0;
	win->border->internal = 1;
	win->border->internal_ecore_evas = win->ecore_evas;
	if (win->state.no_remember) win->border->internal_no_remember = 1;
     }
   _e_win_prop_update(win);
   e_border_show(win->border);
   ecore_evas_show(win->ecore_evas);
}

EAPI void
e_win_hide(E_Win *win)
{
   E_OBJECT_CHECK(win);
   E_OBJECT_TYPE_CHECK(win, E_WIN_TYPE);
   if (win->border) e_border_hide(win->border, 1);
}

EAPI void
e_win_move(E_Win *win, int x, int y)
{
   E_OBJECT_CHECK(win);
   E_OBJECT_TYPE_CHECK(win, E_WIN_TYPE);
   if (win->border)
     e_border_move(win->border,
		   x - win->border->client_inset.l,
		   y - win->border->client_inset.t);
   else
     ecore_evas_move(win->ecore_evas, x, y);
}

EAPI void
e_win_resize(E_Win *win, int w, int h)
{
   E_OBJECT_CHECK(win);
   E_OBJECT_TYPE_CHECK(win, E_WIN_TYPE);
   if (win->border)
     e_border_resize(win->border,
		     w + win->border->client_inset.l + win->border->client_inset.r, 
		     h + win->border->client_inset.t + win->border->client_inset.b);
   else
     ecore_evas_resize(win->ecore_evas, w, h);
}

EAPI void
e_win_move_resize(E_Win *win, int x, int y, int w, int h)
{
   E_OBJECT_CHECK(win);
   E_OBJECT_TYPE_CHECK(win, E_WIN_TYPE);
   if (win->border)
     e_border_move_resize(win->border,
			  x - win->border->client_inset.l,
			  y - win->border->client_inset.t,
			  w + win->border->client_inset.l + win->border->client_inset.r,
			  h + win->border->client_inset.t + win->border->client_inset.b);
   else
     ecore_evas_move_resize(win->ecore_evas, x, y, w, h);
}

EAPI void
e_win_raise(E_Win *win)
{
   E_OBJECT_CHECK(win);
   E_OBJECT_TYPE_CHECK(win, E_WIN_TYPE);
   if (win->border)
     e_border_raise(win->border);
}

EAPI void
e_win_lower(E_Win *win)
{
   E_OBJECT_CHECK(win);
   E_OBJECT_TYPE_CHECK(win, E_WIN_TYPE);
   if (win->border)
     e_border_lower(win->border);
}

EAPI void
e_win_placed_set(E_Win *win, int placed)
{
   E_OBJECT_CHECK(win);
   E_OBJECT_TYPE_CHECK(win, E_WIN_TYPE);
   win->placed = placed;
   if (win->border)
     _e_win_prop_update(win);
}

EAPI Evas *
e_win_evas_get(E_Win *win)
{
   E_OBJECT_CHECK_RETURN(win, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(win, E_WIN_TYPE, NULL);
   return win->evas;
}

EAPI void
e_win_move_callback_set(E_Win *win, void (*func) (E_Win *win))
{
   E_OBJECT_CHECK(win);
   E_OBJECT_TYPE_CHECK(win, E_WIN_TYPE);
   win->cb_move = func;
}

EAPI void
e_win_resize_callback_set(E_Win *win, void (*func) (E_Win *win))
{
   E_OBJECT_CHECK(win);
   E_OBJECT_TYPE_CHECK(win, E_WIN_TYPE);
   win->cb_resize = func;
}

EAPI void
e_win_delete_callback_set(E_Win *win, void (*func) (E_Win *win))
{
   E_OBJECT_CHECK(win);
   E_OBJECT_TYPE_CHECK(win, E_WIN_TYPE);
   win->cb_delete = func;
}

EAPI void
e_win_shaped_set(E_Win *win, int shaped)
{
   E_OBJECT_CHECK(win);
   E_OBJECT_TYPE_CHECK(win, E_WIN_TYPE);
   ecore_evas_shaped_set(win->ecore_evas, shaped);
}

EAPI void
e_win_avoid_damage_set(E_Win *win, int avoid)
{
   E_OBJECT_CHECK(win);
   E_OBJECT_TYPE_CHECK(win, E_WIN_TYPE);
   ecore_evas_avoid_damage_set(win->ecore_evas, avoid);
}

EAPI void
e_win_borderless_set(E_Win *win, int borderless)
{
   E_OBJECT_CHECK(win);
   E_OBJECT_TYPE_CHECK(win, E_WIN_TYPE);
   ecore_evas_borderless_set(win->ecore_evas, borderless);
}

EAPI void
e_win_layer_set(E_Win *win, int layer)
{
   E_OBJECT_CHECK(win);
   E_OBJECT_TYPE_CHECK(win, E_WIN_TYPE);
   ecore_evas_layer_set(win->ecore_evas, layer);
}

EAPI void
e_win_sticky_set(E_Win *win, int sticky)
{
   E_OBJECT_CHECK(win);
   E_OBJECT_TYPE_CHECK(win, E_WIN_TYPE);
   ecore_evas_sticky_set(win->ecore_evas, sticky);
}

EAPI void
e_win_size_min_set(E_Win *win, int w, int h)
{
   E_OBJECT_CHECK(win);
   E_OBJECT_TYPE_CHECK(win, E_WIN_TYPE);
   win->min_w = w;
   win->min_h = h;   
   if (win->border)
     _e_win_prop_update(win);
}

EAPI void
e_win_size_max_set(E_Win *win, int w, int h)
{
   E_OBJECT_CHECK(win);
   E_OBJECT_TYPE_CHECK(win, E_WIN_TYPE);
   win->max_w = w;
   win->max_h = h;   
   if (win->border)
     _e_win_prop_update(win);
}

EAPI void
e_win_size_base_set(E_Win *win, int w, int h)
{
   E_OBJECT_CHECK(win);
   E_OBJECT_TYPE_CHECK(win, E_WIN_TYPE);
   win->base_w = w;
   win->base_h = h;   
   if (win->border)
     _e_win_prop_update(win);
}

EAPI void
e_win_step_set(E_Win *win, int x, int y)
{
   E_OBJECT_CHECK(win);
   E_OBJECT_TYPE_CHECK(win, E_WIN_TYPE);
   win->step_x = x;
   win->step_y = y;
   if (win->border)
     _e_win_prop_update(win);
}

EAPI void
e_win_name_class_set(E_Win *win, const char *name, const char *class)
{
   E_OBJECT_CHECK(win);
   E_OBJECT_TYPE_CHECK(win, E_WIN_TYPE);
   ecore_evas_name_class_set(win->ecore_evas, name, class);
}

EAPI void
e_win_title_set(E_Win *win, const char *title)
{
   E_OBJECT_CHECK(win);
   E_OBJECT_TYPE_CHECK(win, E_WIN_TYPE);
   ecore_evas_title_set(win->ecore_evas, title);
}

EAPI void
e_win_centered_set(E_Win *win, int centered)
{
   E_OBJECT_CHECK(win);
   E_OBJECT_TYPE_CHECK(win, E_WIN_TYPE);
   if ((win->state.centered) && (!centered))
     {
	win->state.centered = 0;
	_e_win_state_update(win);
     }
   else if ((!win->state.centered) && (centered))
     {
	win->state.centered = 1;
	_e_win_state_update(win);
     }
   if ((win->border) && (centered))
     {
	/* The window is visible, move it to the right spot */
	e_border_move(win->border,
		      win->border->zone->x + (win->border->zone->w - win->border->w) / 2,
		      win->border->zone->y + (win->border->zone->h - win->border->h) / 2);
     }
}

EAPI void
e_win_dialog_set(E_Win *win, int dialog)
{
   E_OBJECT_CHECK(win);
   E_OBJECT_TYPE_CHECK(win, E_WIN_TYPE);
   if ((win->state.dialog) && (!dialog))
     {
	win->state.dialog = 0;
	_e_win_prop_update(win);
     }
   else if ((!win->state.dialog) && (dialog))
     {
	win->state.dialog = 1;
	_e_win_prop_update(win);
     }
}

EAPI void
e_win_no_remember_set(E_Win *win, int no_remember)
{
   E_OBJECT_CHECK(win);
   E_OBJECT_TYPE_CHECK(win, E_WIN_TYPE);
   win->state.no_remember = no_remember;
}

EAPI E_Win *
e_win_evas_object_win_get(Evas_Object *obj)
{
   Evas *evas;
   Evas_Object *wobj;
   E_Win *win;
   
   if (!obj) return NULL;
   evas = evas_object_evas_get(obj);
   wobj = evas_object_name_find(evas, "E_Win");
   if (!wobj) return NULL;
   win = evas_object_data_get(wobj, "E_Win");
   return win;
}

EAPI void 
e_win_border_icon_set(E_Win *win, const char *icon) 
{
   E_Border *border;
   
   border = win->border;
   if (!border) return;
   if (border->internal_icon)
     {
	evas_stringshare_del(border->internal_icon);
	border->internal_icon = NULL;
     }
   if (icon)
     border->internal_icon = evas_stringshare_add(icon);
}

EAPI void 
e_win_border_icon_key_set(E_Win *win, const char *key) 
{
   E_Border *border;
   
   border = win->border;
   if (!border) return;
   if (border->internal_icon_key) 
     {
	evas_stringshare_del(border->internal_icon_key);
	border->internal_icon_key = NULL;
     }
   if (key)
     border->internal_icon_key = evas_stringshare_add(key);
}

/* local subsystem functions */
static void
_e_win_free(E_Win *win)
{
   e_object_del(E_OBJECT(win->pointer));
   e_canvas_del(win->ecore_evas);
   ecore_evas_free(win->ecore_evas);
   if (win->border)
     {
	e_border_hide(win->border, 1);
	e_object_del(E_OBJECT(win->border));
     }
   wins = evas_list_remove(wins, win);
   free(win);
}

static void
_e_win_del(void *obj)
{
   E_Win *win;
   
   win = obj;
   if (win->border) e_border_hide(win->border, 1);
}

static void
_e_win_prop_update(E_Win *win)
{
   ecore_x_icccm_size_pos_hints_set(win->evas_win,
				    win->placed, ECORE_X_GRAVITY_NW,
				    win->min_w, win->min_h,
				    win->max_w, win->max_h,
				    win->base_w, win->base_h,
				    win->step_x, win->step_y,
				    win->min_aspect, win->max_aspect);
   if (win->state.dialog)
     {
	ecore_x_icccm_transient_for_set(win->evas_win, win->container->manager->root);
	ecore_x_netwm_window_type_set(win->evas_win, ECORE_X_WINDOW_TYPE_DIALOG);
     }
   else
     {
	ecore_x_icccm_transient_for_unset(win->evas_win);
	ecore_x_netwm_window_type_set(win->evas_win, ECORE_X_WINDOW_TYPE_NORMAL);
     }
}

static void
_e_win_state_update(E_Win *win)
{
   Ecore_X_Atom state[1];
   int num = 0;

   if (win->state.centered)
     state[num++] = E_ATOM_WINDOW_STATE_CENTERED;

   if (num)
     ecore_x_window_prop_card32_set(win->evas_win, E_ATOM_WINDOW_STATE, state, num);
   else
     ecore_x_window_prop_property_del(win->evas_win, E_ATOM_WINDOW_STATE);
}

static void
_e_win_cb_move(Ecore_Evas *ee)
{
   E_Win *win;
   
   win = ecore_evas_data_get(ee, "E_Win");
   if (!win) return;
   ecore_evas_geometry_get(win->ecore_evas, &win->x, &win->y, &win->w, &win->h);   
   if (win->cb_move) win->cb_move(win);
}

static void
_e_win_cb_resize(Ecore_Evas *ee)
{
   E_Win *win;
   
   win = ecore_evas_data_get(ee, "E_Win");
   if (!win) return;
   ecore_evas_geometry_get(win->ecore_evas, &win->x, &win->y, &win->w, &win->h);
   if (win->cb_resize) win->cb_resize(win);
}

static void
_e_win_cb_delete(Ecore_Evas *ee)
{
   E_Win *win;
   
   win = ecore_evas_data_get(ee, "E_Win");
   if (!win) return;
   if (win->cb_delete) win->cb_delete(win);
}
