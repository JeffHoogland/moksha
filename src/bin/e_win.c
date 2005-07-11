/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* local subsystem functions */
static void _e_win_free(E_Win *win);
static void _e_win_prop_update(E_Win *win);
static void _e_win_cb_move(Ecore_Evas *ee);
static void _e_win_cb_resize(Ecore_Evas *ee);
static void _e_win_cb_delete(Ecore_Evas *ee);

/* local subsystem globals */
static Evas_List *wins = NULL;

/* externally accessible functions */
int
e_win_init(void)
{    
}

int
e_win_shutdown(void)
{
/*   
   while (wins)
     {
	e_object_del(E_OBJECT(wins->data));
     }
*/
}

E_Win *
e_win_new(E_Container *con)
{
   E_Win *win;
   
   win = E_OBJECT_ALLOC(E_Win, E_WIN_TYPE, _e_win_free);
   if (!win) return NULL;
   win->container = con;
   if (e_canvas_engine_decide(e_config->evas_engine_errors) ==
       E_EVAS_ENGINE_GL_X11)
     {
	win->ecore_evas = ecore_evas_gl_x11_new(NULL, con->manager->root,
						0, 0, 1, 1);
	win->evas_win = ecore_evas_gl_x11_window_get(win->ecore_evas);
     }
   else
     {
	win->ecore_evas = ecore_evas_software_x11_new(NULL, con->manager->root,
						      0, 0, 1, 1);
	win->evas_win = ecore_evas_software_x11_window_get(win->ecore_evas);
     }
   ecore_evas_data_set(win->ecore_evas, "E_Win", win);
   ecore_evas_callback_move_set(win->ecore_evas, _e_win_cb_move);
   ecore_evas_callback_resize_set(win->ecore_evas, _e_win_cb_resize);
   ecore_evas_callback_delete_request_set(win->ecore_evas, _e_win_cb_delete);
   win->evas = ecore_evas_get(win->ecore_evas);
   e_canvas_add(win->ecore_evas);
   ecore_evas_name_class_set(win->ecore_evas, "E", "_e_internal_window");
   ecore_evas_title_set(win->ecore_evas, "E");
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
   return win;
}

void
e_win_show(E_Win *win)
{
   E_OBJECT_CHECK(win);
   E_OBJECT_TYPE_CHECK(win, E_WIN_TYPE);
   if (!win->border)
     {
	_e_win_prop_update(win);
	win->border = e_border_new(win->container, win->evas_win, 1);
	if (!win->placed)
	  win->border->re_manage = 0;
	win->border->internal = 1;
     }
   e_border_show(win->border);
   ecore_evas_show(win->ecore_evas);
}

void
e_win_hide(E_Win *win)
{
   E_OBJECT_CHECK(win);
   E_OBJECT_TYPE_CHECK(win, E_WIN_TYPE);
   if (win->border) e_border_hide(win->border, 1);
}

void
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

void
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

void
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

void
e_win_raise(E_Win *win)
{
   E_OBJECT_CHECK(win);
   E_OBJECT_TYPE_CHECK(win, E_WIN_TYPE);
   if (win->border)
     e_border_raise(win->border);
}

void
e_win_lower(E_Win *win)
{
   E_OBJECT_CHECK(win);
   E_OBJECT_TYPE_CHECK(win, E_WIN_TYPE);
   if (win->border)
     e_border_lower(win->border);
}

void
e_win_placed_set(E_Win *win, int placed)
{
   E_OBJECT_CHECK(win);
   E_OBJECT_TYPE_CHECK(win, E_WIN_TYPE);
   win->placed = placed;
   if (win->border)
     _e_win_prop_update(win);
}

Evas *
e_win_evas_get(E_Win *win)
{
   E_OBJECT_CHECK_RETURN(win, NULL);
   E_OBJECT_TYPE_CHECK_RETURN(win, E_WIN_TYPE, NULL);
   return win->evas;
}

void
e_win_move_callback_set(E_Win *win, void (*func) (E_Win *win))
{
   win->cb_move = func;
}

void
e_win_resize_callback_set(E_Win *win, void (*func) (E_Win *win))
{
   win->cb_resize = func;
}

void
e_win_delete_callback_set(E_Win *win, void (*func) (E_Win *win))
{
   win->cb_delete = func;
}

void
e_win_shaped_set(E_Win *win, int shaped)
{
   ecore_evas_shaped_set(win->ecore_evas, shaped);
}

void
e_win_avoid_damage_set(E_Win *win, int avoid)
{
   ecore_evas_avoid_damage_set(win->ecore_evas, avoid);
}

void
e_win_borderless_set(E_Win *win, int borderless)
{
   ecore_evas_borderless_set(win->ecore_evas, borderless);
}

void
e_win_size_min_set(E_Win *win, int w, int h)
{
   win->min_w = w;
   win->min_h = h;   
   if (win->border)
     _e_win_prop_update(win);
}

void
e_win_size_max_set(E_Win *win, int w, int h)
{
   win->max_w = w;
   win->max_h = h;   
   if (win->border)
     _e_win_prop_update(win);
}

void
e_win_size_base_set(E_Win *win, int w, int h)
{
   win->base_w = w;
   win->base_h = h;   
   if (win->border)
     _e_win_prop_update(win);
}

void
e_win_step_set(E_Win *win, int x, int y)
{
   win->step_x = x;
   win->step_y = y;
   if (win->border)
     _e_win_prop_update(win);
}

void
e_win_name_class_set(E_Win *win, char *name, char *class)
{
   ecore_evas_name_class_set(win->ecore_evas, name, class);
}

void
e_win_title_set(E_Win *win, char *title)
{
   ecore_evas_title_set(win->ecore_evas, title);
}

/* local subsystem functions */
static void
_e_win_free(E_Win *win)
{
   e_canvas_del(win->ecore_evas);
   ecore_evas_free(win->ecore_evas);
   if (win->border) e_object_del(E_OBJECT(win->border));
   wins = evas_list_remove(wins, win);
   free(win);
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
}

static void
_e_win_cb_move(Ecore_Evas *ee)
{
   E_Win *win;
   
   win = ecore_evas_data_get(ee, "E_Win");
   if (!win) return;
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
