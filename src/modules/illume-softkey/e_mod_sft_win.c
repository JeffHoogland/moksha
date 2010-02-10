#include "e.h"
#include "e_mod_main.h"
#include "e_mod_sft_win.h"

/* local function prototypes */
static void _e_mod_sft_win_cb_free(Il_Sft_Win *swin);
static void _e_mod_sft_win_cb_hook_eval_end(void *data, void *data2);
static void _e_mod_sft_win_cb_resize(E_Win *win);
static void _e_mod_sft_win_cb_back_click(void *data, void *data2);
static void _e_mod_sft_win_cb_close_click(void *data, void *data2);

Il_Sft_Win *
e_mod_sft_win_new(E_Zone *zone) 
{
   Evas *evas;
   Il_Sft_Win *swin;
   Ecore_X_Window_State states[2];

   /* allocate our new softkey window object */
   swin = E_OBJECT_ALLOC(Il_Sft_Win, IL_SFT_WIN_TYPE, _e_mod_sft_win_cb_free);
   if (!swin) return NULL;
   swin->zone = zone;

   /* hook into eval end so we can set on the correct zone */
   swin->hook = e_border_hook_add(E_BORDER_HOOK_EVAL_END, 
                                  _e_mod_sft_win_cb_hook_eval_end, swin);

   /* create the new softkey window */
   swin->win = e_win_new(zone->container);
   swin->win->data = swin;
   e_win_title_set(swin->win, _("Illume Softkey"));
   e_win_name_class_set(swin->win, "Illume-Softkey", "Illume-Softkey");
   e_win_resize_callback_set(swin->win, _e_mod_sft_win_cb_resize);

   /* set this window to not show in taskbar or pager */
   states[0] = ECORE_X_WINDOW_STATE_SKIP_TASKBAR;
   states[1] = ECORE_X_WINDOW_STATE_SKIP_PAGER;
   ecore_x_netwm_window_state_set(swin->win->evas_win, states, 2);

   /* set this window to not accept or take focus */
   ecore_x_icccm_hints_set(swin->win->evas_win, 0, 0, 0, 0, 0, 0, 0);

   evas = e_win_evas_get(swin->win);

   /* create our base object */
   swin->o_base = edje_object_add(evas);
   if (!e_theme_edje_object_set(swin->o_base, 
                                "base/theme/modules/illume-softkey", 
                                "modules/illume-softkey/window")) 
     {
        char buff[PATH_MAX];

        memset(buff, 0, sizeof(buff));
        snprintf(buff, sizeof(buff), "%s/e-module-illume-softkey.edj", 
                 _sft_mod_dir);
        edje_object_file_set(swin->o_base, buff, 
                             "modules/illume-softkey/window");
        memset(buff, 0, sizeof(buff));
     }
   evas_object_move(swin->o_base, 0, 0);
   evas_object_show(swin->o_base);

   /* create the box object for packing buttons */
   swin->o_box = e_widget_list_add(evas, 1, 1);
   edje_object_part_swallow(swin->o_base, "e.swallow.buttons", swin->o_box);

   /* create the back button */
   swin->b_back = 
     e_widget_button_add(evas, NULL, "go-previous", 
                         _e_mod_sft_win_cb_back_click, swin, NULL);
   e_widget_list_object_append(swin->o_box, swin->b_back, 1, 0, 0.5);

   /* create the close button */
   swin->b_close = 
     e_widget_button_add(evas, NULL, "window-close", 
                         _e_mod_sft_win_cb_close_click, swin, NULL);
   e_widget_list_object_append(swin->o_box, swin->b_close, 1, 0, 0.5);

   /* set the minimum window size */
   e_win_size_min_set(swin->win, zone->w, (32 * e_scale));

   /* position and resize the window */
   e_win_move_resize(swin->win, zone->x, (zone->y + zone->h - (32 * e_scale)), 
                     zone->w, (32 * e_scale));

   /* show the window */
   e_win_show(swin->win);

   /* set this window to be a 'dock' window */
   ecore_x_netwm_window_type_set(swin->win->evas_win, ECORE_X_WINDOW_TYPE_DOCK);

   /* tell illume conformant apps our position and size */
   ecore_x_e_illume_bottom_panel_geometry_set(ecore_x_window_root_first_get(), 
                                              zone->x, (zone->y + zone->h - (32 * e_scale)), 
                                              zone->w, (32 * e_scale));
   return swin;
}

/* local functions */
static void 
_e_mod_sft_win_cb_free(Il_Sft_Win *swin) 
{
   /* delete the border hook */
   if (swin->hook) e_border_hook_del(swin->hook);
   swin->hook = NULL;

   /* delete the objects */
   if (swin->b_close) evas_object_del(swin->b_close);
   if (swin->b_back) evas_object_del(swin->b_back);
   if (swin->o_box) evas_object_del(swin->o_box);
   if (swin->o_base) evas_object_del(swin->o_base);

   /* delete the window */
   e_object_del(E_OBJECT(swin->win));
   swin->win = NULL;

   /* free the object */
   E_FREE(swin);
}

static void 
_e_mod_sft_win_cb_hook_eval_end(void *data, void *data2) 
{
   Il_Sft_Win *swin;
   E_Border *bd;

   if (!(swin = data)) return;
   if (!(bd = data2)) return;
   if (bd != swin->win->border) return;
   if (bd->zone != swin->zone) 
     {
        bd->x = swin->zone->x;
        bd->y = (swin->zone->h - bd->h);
        bd->changes.pos = 1;
        bd->changed = 1;
        bd->lock_user_location = 1;
        e_border_zone_set(bd, swin->zone);
     }
}

static void 
_e_mod_sft_win_cb_resize(E_Win *win) 
{
   Il_Sft_Win *swin;

   if (!(swin = win->data)) return;
   evas_object_resize(swin->o_base, swin->win->w, swin->win->h);
}

static void 
_e_mod_sft_win_cb_back_click(void *data, void *data2) 
{
   Il_Sft_Win *swin;

   if (!(swin = data)) return;
   ecore_x_e_illume_back_send(swin->zone->black_win);
}

static void 
_e_mod_sft_win_cb_close_click(void *data, void *data2) 
{
   Il_Sft_Win *swin;

   if (!(swin = data)) return;
   ecore_x_e_illume_close_send(swin->zone->black_win);
}
