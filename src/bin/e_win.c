#include "e.h"

/* local subsystem functions */
static void _e_win_free(E_Win *win);
static void _e_win_del(void *obj);
static void _e_win_prop_update(E_Win *win);
static void _e_win_state_update(E_Win *win);
static void _e_win_cb_move(Ecore_Evas *ee);
static void _e_win_cb_resize(Ecore_Evas *ee);
static void _e_win_cb_delete(Ecore_Evas *ee);
static void _e_win_cb_state(Ecore_Evas *ee);

/* local subsystem globals */
static Eina_List *wins = NULL;

#ifdef HAVE_ELEMENTARY
/* intercept elm_win operations so we talk directly to e_border */

#include <Elementary.h>

typedef struct _Elm_Win_Trap_Ctx
{
   E_Border *border;
   Ecore_X_Window xwin;
   Eina_Bool centered:1;
   Eina_Bool placed:1;
} Elm_Win_Trap_Ctx;

static void *
_elm_win_trap_add(Evas_Object *o __UNUSED__)
{
   Elm_Win_Trap_Ctx *ctx = calloc(1, sizeof(Elm_Win_Trap_Ctx));
   EINA_SAFETY_ON_NULL_RETURN_VAL(ctx, NULL);
   return ctx;
}

static void
_elm_win_trap_del(void *data, Evas_Object *o __UNUSED__)
{
   Elm_Win_Trap_Ctx *ctx = data;
   EINA_SAFETY_ON_NULL_RETURN(ctx);
   if (ctx->border)
     {
        e_border_hide(ctx->border, 1);
        e_object_del(E_OBJECT(ctx->border));
     }
   free(ctx);
}

static Eina_Bool
_elm_win_trap_hide(void *data, Evas_Object *o __UNUSED__)
{
   Elm_Win_Trap_Ctx *ctx = data;
   EINA_SAFETY_ON_NULL_RETURN_VAL(ctx, EINA_TRUE);
   if (!ctx->border) return EINA_TRUE;
   e_border_hide(ctx->border, 1);
   return EINA_FALSE;
}

static Eina_Bool
_elm_win_trap_show(void *data, Evas_Object *o)
{
   Elm_Win_Trap_Ctx *ctx = data;
   EINA_SAFETY_ON_NULL_RETURN_VAL(ctx, EINA_TRUE);
   if (!ctx->border)
     {
        Ecore_X_Window xwin = elm_win_xwindow_get(o);
        E_Container *con = e_util_container_window_find(xwin);
        Evas *e = evas_object_evas_get(o);
        Ecore_Evas *ee = ecore_evas_ecore_evas_get(e);

        if (!con)
          {
             E_Manager *man = e_manager_current_get();
             EINA_SAFETY_ON_NULL_RETURN_VAL(man, EINA_TRUE);
             con = e_container_current_get(man);
             if (!con) con = e_container_number_get(man, 0);
             EINA_SAFETY_ON_NULL_RETURN_VAL(con, EINA_TRUE);
          }

        ctx->xwin = xwin;
        ctx->border = e_border_new(con, xwin, 0, 1);
        EINA_SAFETY_ON_NULL_RETURN_VAL(ctx->border, EINA_TRUE);
        ctx->border->placed = ctx->placed;
        ctx->border->internal = 1;
        ctx->border->internal_ecore_evas = ee;
     }
   if (ctx->centered) e_border_center(ctx->border);
   e_border_show(ctx->border);
   return EINA_FALSE;
}

static Eina_Bool
_elm_win_trap_move(void *data, Evas_Object *o __UNUSED__, int x, int y)
{
   Elm_Win_Trap_Ctx *ctx = data;
   EINA_SAFETY_ON_NULL_RETURN_VAL(ctx, EINA_TRUE);
   ctx->centered = EINA_FALSE;
   ctx->placed = EINA_TRUE;
   if (!ctx->border) return EINA_TRUE;
   e_border_move_without_border(ctx->border, x, y);
   return EINA_FALSE;
}

static Eina_Bool
_elm_win_trap_resize(void *data, Evas_Object *o __UNUSED__, int w, int h)
{
   Elm_Win_Trap_Ctx *ctx = data;
   EINA_SAFETY_ON_NULL_RETURN_VAL(ctx, EINA_TRUE);
   ctx->centered = EINA_FALSE;
   if (!ctx->border) return EINA_TRUE;
   e_border_resize_without_border(ctx->border, w, h);
   return EINA_FALSE;
}

static Eina_Bool
_elm_win_trap_center(void *data, Evas_Object *o __UNUSED__)
{
   Elm_Win_Trap_Ctx *ctx = data;
   EINA_SAFETY_ON_NULL_RETURN_VAL(ctx, EINA_TRUE);
   ctx->centered = EINA_TRUE;
   if (!ctx->border) return EINA_TRUE;
   if (ctx->centered) e_border_center(ctx->border);
   return EINA_FALSE;
}

static Eina_Bool
_elm_win_trap_lower(void *data, Evas_Object *o __UNUSED__)
{
   Elm_Win_Trap_Ctx *ctx = data;
   EINA_SAFETY_ON_NULL_RETURN_VAL(ctx, EINA_TRUE);
   if (!ctx->border) return EINA_TRUE;
   e_border_lower(ctx->border);
   return EINA_FALSE;
}

static Eina_Bool
_elm_win_trap_raise(void *data, Evas_Object *o __UNUSED__)
{
   Elm_Win_Trap_Ctx *ctx = data;
   EINA_SAFETY_ON_NULL_RETURN_VAL(ctx, EINA_TRUE);
   if (!ctx->border) return EINA_TRUE;
   e_border_raise(ctx->border);
   return EINA_FALSE;
}

static const Elm_Win_Trap _elm_win_trap = {
  ELM_WIN_TRAP_VERSION,
  _elm_win_trap_add,
  _elm_win_trap_del,
  _elm_win_trap_hide,
  _elm_win_trap_show,
  _elm_win_trap_move,
  _elm_win_trap_resize,
  _elm_win_trap_center,
  _elm_win_trap_lower,
  _elm_win_trap_raise,
  /* activate */ NULL,
  /* alpha_set */ NULL,
  /* aspect_set */ NULL,
  /* avoid_damage_set */ NULL,
  /* borderless_set */ NULL,
  /* demand_attention_set */ NULL,
  /* focus_skip_set */ NULL,
  /* fullscreen_set */ NULL,
  /* iconified_set */ NULL,
  /* layer_set */ NULL,
  /* manual_render_set */ NULL,
  /* maximized_set */ NULL,
  /* modal_set */ NULL,
  /* name_class_set */ NULL,
  /* object_cursor_set */ NULL,
  /* override_set */ NULL,
  /* rotation_set */ NULL,
  /* rotation_with_resize_set */ NULL,
  /* shaped_set */ NULL,
  /* size_base_set */ NULL,
  /* size_step_set */ NULL,
  /* size_min_set */ NULL,
  /* size_max_set */ NULL,
  /* sticky_set */ NULL,
  /* title_set */ NULL,
  /* urgent_set */ NULL,
  /* withdrawn_set */ NULL
  };
#endif

/* externally accessible functions */
EINTERN int
e_win_init(void)
{
#ifdef HAVE_ELEMENTARY
   if (!elm_win_trap_set(&_elm_win_trap)) return 0;
#endif
   return 1;
}

EINTERN int
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
   e_object_delay_del_set(E_OBJECT(win), e_win_hide);
   win->container = con;
   win->ecore_evas = e_canvas_new(con->manager->root,
                                  0, 0, 1, 1, 1, 0,
                                  &(win->evas_win));
   e_canvas_add(win->ecore_evas);
   ecore_evas_data_set(win->ecore_evas, "E_Win", win);
   ecore_evas_callback_move_set(win->ecore_evas, _e_win_cb_move);
   ecore_evas_callback_resize_set(win->ecore_evas, _e_win_cb_resize);
   ecore_evas_callback_delete_request_set(win->ecore_evas, _e_win_cb_delete);
   ecore_evas_callback_state_change_set(win->ecore_evas, _e_win_cb_state);
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
   wins = eina_list_append(wins, win);

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
// dont need this - special stuff
//        win->border->ignore_first_unmap = 1;
        if (!win->placed)
          win->border->re_manage = 0;
        win->border->internal = 1;
        if (win->ecore_evas)
          win->border->internal_ecore_evas = win->ecore_evas;
        if (win->state.no_remember) win->border->internal_no_remember = 1;
        win->border->internal_no_reopen = win->state.no_reopen;
     }
   _e_win_prop_update(win);
   e_border_show(win->border);
// done now by e_border specially
//   ecore_evas_show(win->ecore_evas);
}

EAPI void
e_win_hide(E_Win *win)
{
   E_OBJECT_CHECK(win);
   E_OBJECT_TYPE_CHECK(win, E_WIN_TYPE);
   if (win->border) e_border_hide(win->border, 1);
}

/**
 * This will move window to position, automatically accounts border decorations.
 *
 * Don't need to account win->border->client_inset, it's done
 * automatically, so it will work fine with new windows that still
 * don't have the border.
 *
 * @parm x horizontal position to place window.
 * @parm y vertical position to place window.
 */
EAPI void
e_win_move(E_Win *win, int x, int y)
{
   E_OBJECT_CHECK(win);
   E_OBJECT_TYPE_CHECK(win, E_WIN_TYPE);
   if (win->border)
     e_border_move_without_border(win->border, x, y);
   else
     ecore_evas_move(win->ecore_evas, x, y);
}

/**
 * This will resize window, automatically accounts border decorations.
 *
 * Don't need to account win->border->client_inset, it's done
 * automatically, so it will work fine with new windows that still
 * don't have the border.
 *
 * @parm w horizontal window size.
 * @parm h vertical window size.
 */
EAPI void
e_win_resize(E_Win *win, int w, int h)
{
   E_OBJECT_CHECK(win);
   E_OBJECT_TYPE_CHECK(win, E_WIN_TYPE);
   if (win->border)
     e_border_resize_without_border(win->border, w, h);
   else
     ecore_evas_resize(win->ecore_evas, w, h);
}

/**
 * This will move and resize window to position, automatically
 * accounts border decorations.
 *
 * Don't need to account win->border->client_inset, it's done
 * automatically, so it will work fine with new windows that still
 * don't have the border.
 *
 * @parm x horizontal position to place window.
 * @parm y vertical position to place window.
 * @parm w horizontal window size.
 * @parm h vertical window size.
 */
EAPI void
e_win_move_resize(E_Win *win, int x, int y, int w, int h)
{
   E_OBJECT_CHECK(win);
   E_OBJECT_TYPE_CHECK(win, E_WIN_TYPE);
   if (win->border)
     e_border_move_resize_without_border(win->border, x, y, w, h);
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
e_win_move_callback_set(E_Win *win, void (*func)(E_Win *win))
{
   E_OBJECT_CHECK(win);
   E_OBJECT_TYPE_CHECK(win, E_WIN_TYPE);
   win->cb_move = func;
}

EAPI void
e_win_resize_callback_set(E_Win *win, void (*func)(E_Win *win))
{
   E_OBJECT_CHECK(win);
   E_OBJECT_TYPE_CHECK(win, E_WIN_TYPE);
   win->cb_resize = func;
}

EAPI void
e_win_delete_callback_set(E_Win *win, void (*func)(E_Win *win))
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
e_win_layer_set(E_Win *win, E_Win_Layer layer)
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
     e_border_center(win->border);
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
        eina_stringshare_del(border->internal_icon);
        border->internal_icon = NULL;
     }
   if (icon)
     border->internal_icon = eina_stringshare_add(icon);
}

EAPI void
e_win_border_icon_key_set(E_Win *win, const char *key)
{
   E_Border *border;

   border = win->border;
   if (!border) return;
   if (border->internal_icon_key)
     {
        eina_stringshare_del(border->internal_icon_key);
        border->internal_icon_key = NULL;
     }
   if (key)
     border->internal_icon_key = eina_stringshare_add(key);
}

/* local subsystem functions */
static void
_e_win_free(E_Win *win)
{
   if (win->pointer)
     e_object_del(E_OBJECT(win->pointer));

   e_canvas_del(win->ecore_evas);
   ecore_evas_free(win->ecore_evas);
   if (win->border)
     {
        e_border_hide(win->border, 1);
        e_object_del(E_OBJECT(win->border));
     }
   wins = eina_list_remove(wins, win);
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

static void
_e_win_cb_state(Ecore_Evas *ee)
{
   E_Win *win;

   win = ecore_evas_data_get(ee, "E_Win");
   if (!win) return;
   if (!win->border) return;
   win->border->changed = win->border->changes.size = 1;
}
