#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"
#include "e_mod_sft_win.h"

/* local function prototypes */
static void _e_mod_sft_win_cb_free(Sft_Win *swin);
static Eina_Bool _e_mod_sft_win_cb_win_prop(void *data, int type __UNUSED__, void *event);
static Eina_Bool _e_mod_sft_win_cb_zone_resize(void *data, int type __UNUSED__, void *event);
static void _e_mod_sft_win_cb_resize(E_Win *win);
static void _e_mod_sft_win_create_default_buttons(Sft_Win *swin);
static void _e_mod_sft_win_create_extra_buttons(Sft_Win *swin);
static void _e_mod_sft_win_cb_close(void *data, void *data2 __UNUSED__);
static void _e_mod_sft_win_cb_back(void *data, void *data2 __UNUSED__);
static void _e_mod_sft_win_cb_forward(void *data, void *data2 __UNUSED__);
static void _e_mod_sft_win_cb_win_pos(void *data, void *data2 __UNUSED__);
static void _e_mod_sft_win_pos_toggle_top(Sft_Win *swin);
static void _e_mod_sft_win_pos_toggle_left(Sft_Win *swin);
static E_Border *_e_mod_sft_win_border_get(E_Zone *zone, int x, int y);

Sft_Win *
e_mod_sft_win_new(E_Zone *zone) 
{
   Sft_Win *swin;
   Ecore_X_Window_State states[2];

   /* create our new softkey window object */
   swin = E_OBJECT_ALLOC(Sft_Win, SFT_WIN_TYPE, _e_mod_sft_win_cb_free);
   if (!swin) return NULL;

   swin->zone = zone;
   
   /* hook into property change so we can adjust w/ e_scale */
   swin->hdls = 
     eina_list_append(swin->hdls, 
                      ecore_event_handler_add(ECORE_X_EVENT_WINDOW_PROPERTY, 
                                              _e_mod_sft_win_cb_win_prop, swin));

   /* hook into zone resize so we can adjust min width */
   swin->hdls = 
     eina_list_append(swin->hdls, 
                      ecore_event_handler_add(E_EVENT_ZONE_MOVE_RESIZE, 
                                              _e_mod_sft_win_cb_zone_resize, 
                                              swin));

   /* create new window */
   swin->win = e_win_new(zone->container);
   swin->win->data = swin;

   /* set some properties on the window */
   e_win_title_set(swin->win, _("Illume Softkey"));
   e_win_name_class_set(swin->win, "Illume-Softkey", "Illume-Softkey");
   e_win_no_remember_set(swin->win, EINA_TRUE);

   /* hook into window resize so we can resize our objects */
   e_win_resize_callback_set(swin->win, _e_mod_sft_win_cb_resize);

   /* set this window to not show in taskbar or pager */
   states[0] = ECORE_X_WINDOW_STATE_SKIP_TASKBAR;
   states[1] = ECORE_X_WINDOW_STATE_SKIP_PAGER;
   ecore_x_netwm_window_state_set(swin->win->evas_win, states, 2);

   /* set this window to not accept or take focus */
   ecore_x_icccm_hints_set(swin->win->evas_win, 0, 0, 0, 0, 0, 0, 0);

   /* create our base object */
   swin->o_base = edje_object_add(swin->win->evas);
   if (!e_theme_edje_object_set(swin->o_base, 
                                "base/theme/modules/illume-softkey", 
                                "modules/illume-softkey/window")) 
     {
        char buff[PATH_MAX];

        snprintf(buff, sizeof(buff), 
                 "%s/e-module-illume-softkey.edj", _sft_mod_dir);
        edje_object_file_set(swin->o_base, buff, 
                             "modules/illume-softkey/window");
     }
   evas_object_move(swin->o_base, 0, 0);
   evas_object_show(swin->o_base);

   /* create default buttons */
   _e_mod_sft_win_create_default_buttons(swin);

   /* create default buttons */
   _e_mod_sft_win_create_extra_buttons(swin);

   /* set minimum size of this window */
   e_win_size_min_set(swin->win, zone->w, (il_sft_cfg->height * e_scale));

   /* position and resize this window */
   e_win_move_resize(swin->win, zone->x, 
                     (zone->y + zone->h - (il_sft_cfg->height * e_scale)), 
                     zone->w, (il_sft_cfg->height * e_scale));

   /* show the window */
   e_win_show(swin->win);

   e_border_zone_set(swin->win->border, zone);
   swin->win->border->user_skip_winlist = 1;

   swin->win->border->lock_focus_in = 1;
   swin->win->border->lock_focus_out = 1;

   /* set this window to be a dock window. This needs to be done after show 
    * as E will sometimes reset the window type */
   ecore_x_netwm_window_type_set(swin->win->evas_win, ECORE_X_WINDOW_TYPE_DOCK);

   /* tell conformant apps our position and size */
   ecore_x_e_illume_softkey_geometry_set(zone->black_win, 
                                         zone->x, 
                                         (zone->h - (il_sft_cfg->height * e_scale)), 
                                         zone->w, 
                                         (il_sft_cfg->height * e_scale));

   return swin;
}

/* local functions */
static void 
_e_mod_sft_win_cb_free(Sft_Win *swin) 
{
   Ecore_Event_Handler *hdl;
   const Evas_Object *box;

   /* delete the event handlers */
   EINA_LIST_FREE(swin->hdls, hdl)
     ecore_event_handler_del(hdl);

   if ((box = edje_object_part_object_get(swin->o_base, "e.box.buttons"))) 
     {
        Evas_Object *btn;

        /* delete the buttons */
        EINA_LIST_FREE(swin->btns, btn) 
          {
             edje_object_part_box_remove(swin->o_base, "e.box.buttons", btn);
             evas_object_del(btn);
          }
     }
   if ((box = edje_object_part_object_get(swin->o_base, "e.box.extra_buttons"))) 
     {
        Evas_Object *btn;

        /* delete the buttons */
        EINA_LIST_FREE(swin->extra_btns, btn) 
          {
             edje_object_part_box_remove(swin->o_base, 
                                         "e.box.extra_buttons", btn);
             evas_object_del(btn);
          }
     }

   /* delete the objects */
   if (swin->o_base) evas_object_del(swin->o_base);
   swin->o_base = NULL;

   /* delete the window */
   if (swin->win) e_object_del(E_OBJECT(swin->win));
   swin->win = NULL;

   /* tell conformant apps our position and size */
   ecore_x_e_illume_softkey_geometry_set(swin->zone->black_win, 0, 0, 0, 0);

   /* free the allocated object */
   E_FREE(swin);
}

static Eina_Bool
_e_mod_sft_win_cb_win_prop(void *data, int type __UNUSED__, void *event) 
{
   Sft_Win *swin;
   Ecore_X_Event_Window_Property *ev;

   ev = event;

   if (!(swin = data)) return ECORE_CALLBACK_PASS_ON;
   if (ev->win != swin->win->container->manager->root) 
     return ECORE_CALLBACK_PASS_ON;
   if (ev->atom != ATM_ENLIGHTENMENT_SCALE) return ECORE_CALLBACK_PASS_ON;

   /* set minimum size of this window */
   e_win_size_min_set(swin->win, swin->zone->w, (il_sft_cfg->height * e_scale));

   /* NB: Not sure why, but we need to tell this border to fetch icccm 
    * size position hints now :( (NOTE: This was not needed a few days ago) 
    * If we do not do this, than softkey does not change w/ scale anymore */
   swin->win->border->client.icccm.fetch.size_pos_hints = 1;

   /* resize this window */
   e_win_resize(swin->win, swin->zone->w, (il_sft_cfg->height * e_scale));

   /* tell conformant apps our position and size */
   ecore_x_e_illume_softkey_geometry_set(swin->zone->black_win, 
                                         swin->win->x, swin->win->y, 
                                         swin->win->w, 
                                         (il_sft_cfg->height * e_scale));
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_sft_win_cb_zone_resize(void *data, int type __UNUSED__, void *event) 
{
   Sft_Win *swin;
   E_Event_Zone_Move_Resize *ev;

   ev = event;
   if (!(swin = data)) return ECORE_CALLBACK_PASS_ON;
   if (ev->zone != swin->zone) return ECORE_CALLBACK_PASS_ON;

   /* set minimum size of this window */
   e_win_size_min_set(swin->win, ev->zone->w, (il_sft_cfg->height * e_scale));

   return ECORE_CALLBACK_PASS_ON;
}

static void 
_e_mod_sft_win_cb_resize(E_Win *win) 
{
   Sft_Win *swin;
   Evas_Object *btn;
   const Evas_Object *box;
   Eina_List *l;
   int mw, mh;

   if (!(swin = win->data)) return;

   /* adjust button(s) size for e_scale */
   EINA_LIST_FOREACH(swin->btns, l, btn) 
     {
        e_widget_size_min_get(btn, &mw, &mh);
        evas_object_size_hint_min_set(btn, (mw * e_scale), (mh * e_scale));
        evas_object_resize(btn, (mw * e_scale), (mh * e_scale));
     }

   /* adjust box size for content */
   if ((box = edje_object_part_object_get(swin->o_base, "e.box.buttons"))) 
     {
        evas_object_size_hint_min_get((Evas_Object *)box, &mw, &mh);
        evas_object_resize((Evas_Object *)box, mw, mh);
     }

   mw = mh = 0;
   /* adjust button(s) size for e_scale */
   EINA_LIST_FOREACH(swin->extra_btns, l, btn) 
     {
        e_widget_size_min_get(btn, &mw, &mh);
        evas_object_size_hint_min_set(btn, (mw * e_scale), (mh * e_scale));
        evas_object_resize(btn, (mw * e_scale), (mh * e_scale));
     }

   /* adjust box size for content */
   if ((box = edje_object_part_object_get(swin->o_base, "e.box.extra_buttons"))) 
     {
        evas_object_size_hint_min_get((Evas_Object *)box, &mw, &mh);
        evas_object_resize((Evas_Object *)box, mw, mh);
     }

   /* resize the base object */
   if (swin->o_base) evas_object_resize(swin->o_base, win->w, win->h);
}

static void 
_e_mod_sft_win_create_default_buttons(Sft_Win *swin) 
{
   Evas_Object *btn;
   int mw, mh;

   /* create back button */
   btn = e_widget_button_add(swin->win->evas, _("Back"), "go-previous", 
                             _e_mod_sft_win_cb_back, swin, NULL);
   e_widget_size_min_get(btn, &mw, &mh);
   evas_object_size_hint_min_set(btn, (mw * e_scale), (mh * e_scale));

   /* NB: this show is required when packing e_widgets into an edje box else
    * the widgets do not receive any events */
   evas_object_show(btn);

   /* add button to box */
   edje_object_part_box_append(swin->o_base, "e.box.buttons", btn);

   /* add button to our list */
   swin->btns = eina_list_append(swin->btns, btn);

   /* create forward button */
   btn = e_widget_button_add(swin->win->evas, _("Forward"), "go-next", 
                             _e_mod_sft_win_cb_forward, swin, NULL);
   e_widget_size_min_get(btn, &mw, &mh);
   evas_object_size_hint_min_set(btn, (mw * e_scale), (mh * e_scale));

   /* NB: this show is required when packing e_widgets into an edje box else
    * the widgets do not receive any events */
   evas_object_show(btn);

   /* add button to box */
   edje_object_part_box_append(swin->o_base, "e.box.buttons", btn);

   /* add button to our list */
   swin->btns = eina_list_append(swin->btns, btn);
   /* create close button */
   btn = e_widget_button_add(swin->win->evas, _("Close"), "application-exit", 
                             _e_mod_sft_win_cb_close, swin, NULL);
   e_widget_size_min_get(btn, &mw, &mh);
   evas_object_size_hint_min_set(btn, (mw * e_scale), (mh * e_scale));

   /* NB: this show is required when packing e_widgets into an edje box else
    * the widgets do not receive any events */
   evas_object_show(btn);

   /* add button to box */
   edje_object_part_box_append(swin->o_base, "e.box.buttons", btn);

   /* add button to our list */
   swin->btns = eina_list_append(swin->btns, btn);
}

static void 
_e_mod_sft_win_create_extra_buttons(Sft_Win *swin) 
{
   Evas_Object *btn;
   int mw, mh;

   /* create window toggle button */
   btn = e_widget_button_add(swin->win->evas, _("Switch"), "view-refresh", 
                             _e_mod_sft_win_cb_win_pos, swin, NULL);
   e_widget_size_min_get(btn, &mw, &mh);
   evas_object_size_hint_min_set(btn, (mw * e_scale), (mh * e_scale));

   /* NB: this show is required when packing e_widgets into an edje box else
    * the widgets do not receive any events */
   evas_object_show(btn);

   /* add button to box */
   edje_object_part_box_append(swin->o_base, "e.box.extra_buttons", btn);

   /* add button to our list */
   swin->extra_btns = eina_list_append(swin->extra_btns, btn);
}

static void 
_e_mod_sft_win_cb_close(void *data, void *data2 __UNUSED__) 
{
   Sft_Win *swin;

   if (!(swin = data)) return;
   ecore_x_e_illume_close_send(swin->zone->black_win);
}

static void 
_e_mod_sft_win_cb_back(void *data, void *data2 __UNUSED__) 
{
   Sft_Win *swin;

   if (!(swin = data)) return;
   ecore_x_e_illume_focus_back_send(swin->zone->black_win);
}

static void 
_e_mod_sft_win_cb_forward(void *data, void *data2 __UNUSED__) 
{
   Sft_Win *swin;

   if (!(swin = data)) return;
   ecore_x_e_illume_focus_forward_send(swin->zone->black_win);
}

static void 
_e_mod_sft_win_cb_win_pos(void *data, void *data2 __UNUSED__) 
{
   Sft_Win *swin;
   Ecore_X_Illume_Mode mode;

   if (!(swin = data)) return;
   mode = ecore_x_e_illume_mode_get(swin->zone->black_win);
   switch (mode) 
     {
      case ECORE_X_ILLUME_MODE_UNKNOWN:
      case ECORE_X_ILLUME_MODE_SINGLE:
        break;
      case ECORE_X_ILLUME_MODE_DUAL_TOP:
        _e_mod_sft_win_pos_toggle_top(swin);
        break;
      case ECORE_X_ILLUME_MODE_DUAL_LEFT:
        _e_mod_sft_win_pos_toggle_left(swin);
        break;
     }
}

static void 
_e_mod_sft_win_pos_toggle_top(Sft_Win *swin) 
{
   E_Border *t, *b;
   int y, h, tpos, bpos;

   if (!swin) return;
   if (!ecore_x_e_illume_indicator_geometry_get(swin->zone->black_win, 
                                                NULL, &y, NULL, &h)) 
     y = 0;

   if (y > 0) 
     {
        tpos = 0;
        bpos = (y + h);
     }
   else 
     {
        tpos = (y + h);
        bpos = (swin->zone->h / 2);
     }

   t = _e_mod_sft_win_border_get(swin->zone, swin->zone->x, tpos);
   b = _e_mod_sft_win_border_get(swin->zone, swin->zone->x, bpos);

   if (t) e_border_move(t, t->x, bpos);
   if (b) e_border_move(b, b->x, tpos);
}

static void 
_e_mod_sft_win_pos_toggle_left(Sft_Win *swin) 
{
   E_Border *l, *r;
   int h, lpos, rpos;

   if (!swin) return;

   if (!ecore_x_e_illume_indicator_geometry_get(swin->zone->black_win, 
                                                NULL, NULL, NULL, &h)) 
     h = 0;

   lpos = 0;
   rpos = (swin->zone->w / 2);

   l = _e_mod_sft_win_border_get(swin->zone, lpos, h);
   r = _e_mod_sft_win_border_get(swin->zone, rpos, h);

   if (l) e_border_move(l, rpos, l->y);
   if (r) e_border_move(r, lpos, r->y);
}

static E_Border *
_e_mod_sft_win_border_get(E_Zone *zone, int x, int y) 
{
   Eina_List *l;
   E_Border *bd;

   if (!zone) return NULL;
   EINA_LIST_REVERSE_FOREACH(e_border_client_list(), l, bd) 
     {
        if (bd->zone != zone) continue;
        if (!bd->visible) continue;
        if ((bd->x != x) || (bd->y != y)) continue;
        if (bd->client.illume.quickpanel.quickpanel) continue;

        return bd;
     }
   return NULL;
}
