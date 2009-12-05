#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"
#include "e_mod_win.h"

/* local function prototypes */
static void _il_sk_win_cb_free(Il_Sk_Win *win);
static void _il_sk_win_cb_resize(E_Win *win);
static void _il_sk_win_cb_back_click(void *data, void *data2);
static void _il_sk_win_cb_close_click(void *data, void *data2);

int 
e_mod_sk_win_init(void) 
{
   return 1;
}

int 
e_mod_sk_win_shutdown(void) 
{
   return 1;
}

Il_Sk_Win *
e_mod_sk_win_new(void) 
{
   Il_Sk_Win *swin;
   E_Zone *zone;
   Evas *evas;
   Ecore_X_Window_State states[2];
   char buff[PATH_MAX];

   swin = E_OBJECT_ALLOC(Il_Sk_Win, IL_SK_WIN_TYPE, _il_sk_win_cb_free);
   if (!swin) return NULL;

   snprintf(buff, sizeof(buff), "%s/e-module-illume-softkey.edj", 
            il_sk_cfg->mod_dir);

   swin->win = e_win_new(e_util_container_number_get(0));
   states[0] = ECORE_X_WINDOW_STATE_SKIP_TASKBAR;
   states[1] = ECORE_X_WINDOW_STATE_SKIP_PAGER;
   ecore_x_netwm_window_state_set(swin->win->evas_win, states, 2);
   ecore_x_icccm_hints_set(swin->win->evas_win, 0, 0, 0, 0, 0, 0, 0);
   ecore_x_netwm_window_type_set(swin->win->evas_win, ECORE_X_WINDOW_TYPE_DOCK);

   e_win_no_remember_set(swin->win, 1);
   e_win_resize_callback_set(swin->win, _il_sk_win_cb_resize);
   e_win_borderless_set(swin->win, 1);
   swin->win->data = swin;
   e_win_title_set(swin->win, _("Illume Softkey"));
   e_win_name_class_set(swin->win, "Illume-Softkey", "Illume-Softkey");

   evas = e_win_evas_get(swin->win);

   zone = e_util_container_zone_number_get(0, 0);

   swin->o_base = edje_object_add(evas);
   if (!e_theme_edje_object_set(swin->o_base, "base/theme/modules/illume-softkey", 
                                "modules/illume-softkey/window"))
     edje_object_file_set(swin->o_base, buff, "modules/illume-softkey/window");
   evas_object_move(swin->o_base, 0, 0);
   evas_object_resize(swin->o_base, zone->w, 32);
   evas_object_show(swin->o_base);

   swin->o_box = e_widget_list_add(evas, 1, 1);
   edje_object_part_swallow(swin->o_base, "e.swallow.buttons", swin->o_box);

   swin->b_back = e_widget_button_add(evas, NULL, "go-previous", 
                                      _il_sk_win_cb_back_click, swin, NULL);
   e_widget_list_object_append(swin->o_box, swin->b_back, 1, 0, 0.5);

   swin->b_close = e_widget_button_add(evas, NULL, "window-close", 
                                       _il_sk_win_cb_close_click, swin, NULL);
   e_widget_list_object_append(swin->o_box, swin->b_close, 1, 0, 0.5);

   e_win_size_min_set(swin->win, zone->w, 32);
   e_win_show(swin->win);
   e_win_move_resize(swin->win, 0, (zone->h - 32), zone->w, 32);

   return swin;
}

/* local functions */
static void 
_il_sk_win_cb_free(Il_Sk_Win *win) 
{
   if (win->b_close) evas_object_del(win->b_close);
   if (win->b_back) evas_object_del(win->b_back);
   if (win->o_box) evas_object_del(win->o_box);
   if (win->o_base) evas_object_del(win->o_base);
   e_object_del(E_OBJECT(win->win));
   E_FREE(win);
}

static void 
_il_sk_win_cb_resize(E_Win *win) 
{
   Il_Sk_Win *swin;

   if (!(swin = win->data)) return;
   evas_object_resize(swin->o_base, swin->win->w, swin->win->h);
}

static void 
_il_sk_win_cb_back_click(void *data, void *data2) 
{
   Il_Sk_Win *swin;
   E_Border *bd, *fbd;
   Eina_List *focused, *l;

   if (!(swin = data)) return;
   if (!(bd = e_border_focused_get())) return;
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

static void 
_il_sk_win_cb_close_click(void *data, void *data2) 
{
   Il_Sk_Win *swin;
   E_Border *bd;

   if (!(swin = data)) return;
   if (!(bd = e_border_focused_get())) return;
   e_border_act_close_begin(bd);
}
