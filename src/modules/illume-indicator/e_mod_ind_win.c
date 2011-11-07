#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"
#include "e_mod_ind_win.h"

/* local function prototypes */
static void _e_mod_ind_win_cb_free(Ind_Win *iwin);
static Eina_Bool _e_mod_ind_win_cb_win_prop(void *data, int type __UNUSED__, void *event);
static Eina_Bool _e_mod_ind_win_cb_zone_resize(void *data, int type __UNUSED__, void *event);
static void _e_mod_ind_win_cb_resize(E_Win *win);
static void _e_mod_ind_win_cb_mouse_down(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event);
static void _e_mod_ind_win_cb_mouse_up(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event);
static void _e_mod_ind_win_cb_mouse_move(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event);
static void _e_mod_ind_win_cb_min_size_request(void *data, E_Gadcon *gc, Evas_Coord w, Evas_Coord h);
static void _e_mod_ind_win_cb_size_request(void *data __UNUSED__, E_Gadcon *gc __UNUSED__, Evas_Coord w __UNUSED__, Evas_Coord h __UNUSED__);
static Evas_Object *_e_mod_ind_win_cb_frame_request(void *data __UNUSED__, E_Gadcon_Client *gcc __UNUSED__, const char *style __UNUSED__);
static void _e_mod_ind_win_cb_menu_items_append(void *data, E_Gadcon_Client *gcc __UNUSED__, E_Menu *mn);
static void _e_mod_ind_win_cb_menu_append(Ind_Win *iwin, E_Menu *mn);
static void _e_mod_ind_win_cb_menu_pre(void *data, E_Menu *mn);
static void _e_mod_ind_win_cb_menu_post(void *data, E_Menu *mn __UNUSED__);
static void _e_mod_ind_win_cb_menu_contents(void *data, E_Menu *mn __UNUSED__, E_Menu_Item *mi __UNUSED__);
static void _e_mod_ind_win_cb_menu_edit(void *data, E_Menu *mn __UNUSED__, E_Menu_Item *mi __UNUSED__);
static Eina_Bool _e_mod_ind_win_cb_border_hide(void *data, int type __UNUSED__, void *event);
static Eina_Bool _e_mod_ind_win_cb_border_show(void *data, int type __UNUSED__, void *event);

Ind_Win *
e_mod_ind_win_new(E_Zone *zone) 
{
   Ind_Win *iwin;
   Ecore_X_Window_State states[2];
   Evas_Coord h = 0;

   /* create our new indicator window object */
   iwin = E_OBJECT_ALLOC(Ind_Win, IND_WIN_TYPE, _e_mod_ind_win_cb_free);
   if (!iwin) return NULL;

   h = (il_ind_cfg->height * e_scale);
   iwin->zone = zone;

   /* create new window */
   iwin->win = e_win_new(zone->container);
   iwin->win->data = iwin;

   /* set some properties on the window */
   e_win_title_set(iwin->win, _("Illume Indicator"));
   e_win_name_class_set(iwin->win, "Illume-Indicator", "Illume-Indicator");
   e_win_no_remember_set(iwin->win, EINA_TRUE);

   /* hook into window resize so we can resize our objects */
   e_win_resize_callback_set(iwin->win, _e_mod_ind_win_cb_resize);

   /* set this window to not show in taskbar or pager */
   states[0] = ECORE_X_WINDOW_STATE_SKIP_TASKBAR;
   states[1] = ECORE_X_WINDOW_STATE_SKIP_PAGER;
   ecore_x_netwm_window_state_set(iwin->win->evas_win, states, 2);

   /* set this window to not accept or take focus */
   ecore_x_icccm_hints_set(iwin->win->evas_win, 0, 0, 0, 0, 0, 0, 0);

   /* create the popup */
   iwin->popup = e_popup_new(zone, 0, 0, zone->w, h);
   e_popup_name_set(iwin->popup, "indicator");
   e_popup_layer_set(iwin->popup, 200);

   /* create our event rectangle */
   iwin->o_event = evas_object_rectangle_add(iwin->win->evas);
   evas_object_color_set(iwin->o_event, 0, 0, 0, 0);
   evas_object_event_callback_add(iwin->o_event, EVAS_CALLBACK_MOUSE_DOWN, 
                                  _e_mod_ind_win_cb_mouse_down, iwin);
   evas_object_event_callback_add(iwin->o_event, EVAS_CALLBACK_MOUSE_UP, 
                                  _e_mod_ind_win_cb_mouse_up, iwin);
   evas_object_event_callback_add(iwin->o_event, EVAS_CALLBACK_MOUSE_MOVE, 
                                  _e_mod_ind_win_cb_mouse_move, iwin);
   evas_object_move(iwin->o_event, 0, 0);
   evas_object_show(iwin->o_event);

   /* create our base object */
   iwin->o_base = edje_object_add(iwin->win->evas);
   if (!e_theme_edje_object_set(iwin->o_base, 
                                "base/theme/modules/illume-indicator", 
                                "modules/illume-indicator/window")) 
     {
        char buff[PATH_MAX];

        snprintf(buff, sizeof(buff), 
                 "%s/e-module-illume-indicator.edj", _ind_mod_dir);
        edje_object_file_set(iwin->o_base, buff, 
                             "modules/illume-indicator/window");
     }
   evas_object_move(iwin->o_base, 0, 0);
   evas_object_show(iwin->o_base);

   e_popup_edje_bg_object_set(iwin->popup, iwin->o_base);

   /* create our gadget container */
   iwin->gadcon = e_gadcon_swallowed_new("illume-indicator", zone->id, 
                                         iwin->o_base, "e.swallow.content");
   edje_extern_object_min_size_set(iwin->gadcon->o_container, zone->w, h);
   e_gadcon_min_size_request_callback_set(iwin->gadcon, 
                                          _e_mod_ind_win_cb_min_size_request, 
                                          iwin);
   e_gadcon_size_request_callback_set(iwin->gadcon, 
                                      _e_mod_ind_win_cb_size_request, iwin);
   e_gadcon_frame_request_callback_set(iwin->gadcon, 
                                       _e_mod_ind_win_cb_frame_request, iwin);
   e_gadcon_orient(iwin->gadcon, E_GADCON_ORIENT_FLOAT);
   e_gadcon_zone_set(iwin->gadcon, zone);
   e_gadcon_ecore_evas_set(iwin->gadcon, iwin->win->ecore_evas);

   e_gadcon_util_menu_attach_func_set(iwin->gadcon, 
                                      _e_mod_ind_win_cb_menu_items_append, 
                                      iwin);
   e_gadcon_populate(iwin->gadcon);

   /* hook into property change so we can adjust w/ e_scale */
   iwin->hdls = 
     eina_list_append(iwin->hdls, 
                      ecore_event_handler_add(ECORE_X_EVENT_WINDOW_PROPERTY, 
                                              _e_mod_ind_win_cb_win_prop, iwin));

   /* hook into zone resize so we can set minimum window width when zone 
    * size changes */
   iwin->hdls = 
     eina_list_append(iwin->hdls, 
                      ecore_event_handler_add(E_EVENT_ZONE_MOVE_RESIZE, 
                                              _e_mod_ind_win_cb_zone_resize, 
                                              iwin));

   iwin->hdls = 
     eina_list_append(iwin->hdls, 
                      ecore_event_handler_add(E_EVENT_BORDER_HIDE, 
                                              _e_mod_ind_win_cb_border_hide, 
                                              iwin));

   iwin->hdls = 
     eina_list_append(iwin->hdls, 
                      ecore_event_handler_add(E_EVENT_BORDER_SHOW, 
                                              _e_mod_ind_win_cb_border_show, 
                                              iwin));

   /* set minimum size of this window & popup */
   e_win_size_min_set(iwin->win, zone->w, h);
   ecore_evas_size_min_set(iwin->popup->ecore_evas, zone->w, h);

   /* position and resize this window */
   e_win_move_resize(iwin->win, zone->x, zone->y, zone->w, h);
   e_popup_move_resize(iwin->popup, zone->x, zone->y, zone->w, h);

   /* show the window */
   e_win_show(iwin->win);
   e_popup_show(iwin->popup);

   /* set this window on proper zone */
   e_border_zone_set(iwin->win->border, zone);
   iwin->win->border->user_skip_winlist = 1;
   iwin->win->border->lock_focus_in = 1;
   iwin->win->border->lock_focus_out = 1;

   /* set this window to be a dock window. This needs to be done after show 
    * as E will sometimes reset the window type */
   ecore_x_netwm_window_type_set(iwin->win->evas_win, ECORE_X_WINDOW_TYPE_DOCK);

   /* tell conformant apps our position and size */
   ecore_x_e_illume_indicator_geometry_set(zone->black_win, zone->x, zone->y, 
                                           zone->w, h);

   return iwin;
}

/* local function prototypes */
static void 
_e_mod_ind_win_cb_free(Ind_Win *iwin) 
{
   Ecore_Event_Handler *hdl;

   /* delete the handlers */
   EINA_LIST_FREE(iwin->hdls, hdl)
     ecore_event_handler_del(hdl);

   /* delete the menu */
   if (iwin->menu) 
     {
        e_menu_post_deactivate_callback_set(iwin->menu, NULL, NULL);
        e_object_del(E_OBJECT(iwin->menu));
     }
   iwin->menu = NULL;

   /* delete the gadget container */
   if (iwin->gadcon) e_object_del(E_OBJECT(iwin->gadcon));
   iwin->gadcon = NULL;

   /* delete the objects */
   if (iwin->o_base) evas_object_del(iwin->o_base);
   iwin->o_base = NULL;
   if (iwin->o_event) evas_object_del(iwin->o_event);
   iwin->o_event = NULL;

   /* tell conformant apps our position and size */
   ecore_x_e_illume_indicator_geometry_set(iwin->zone->black_win, 0, 0, 0, 0);

   if (iwin->popup) e_object_del(E_OBJECT(iwin->popup));
   iwin->popup = NULL;

   /* delete the window */
   if (iwin->win) e_object_del(E_OBJECT(iwin->win));
   iwin->win = NULL;

   /* free the allocated object */
   E_FREE(iwin);
}

static Eina_Bool
_e_mod_ind_win_cb_win_prop(void *data, int type __UNUSED__, void *event) 
{
   Ind_Win *iwin;
   Ecore_X_Event_Window_Property *ev;
   Evas_Coord h = 0;

   ev = event;

   if (!(iwin = data)) return ECORE_CALLBACK_PASS_ON;
   if (ev->win != iwin->win->container->manager->root) 
     return ECORE_CALLBACK_PASS_ON;
   if (ev->atom != ATM_ENLIGHTENMENT_SCALE) return ECORE_CALLBACK_PASS_ON;

   h = (il_ind_cfg->height * e_scale);

   /* set minimum size of this window */
   e_win_size_min_set(iwin->win, iwin->zone->w, h);
   ecore_evas_size_min_set(iwin->popup->ecore_evas, iwin->zone->w, h);

   /* NB: Not sure why, but we need to tell this border to fetch icccm 
    * size position hints now :( (NOTE: This was not needed a few days ago) 
    * If we do not do this, than indicator does not change w/ scale anymore */
   iwin->win->border->client.icccm.fetch.size_pos_hints = 1;

   /* resize this window */
   e_win_resize(iwin->win, iwin->zone->w, h);
   e_popup_resize(iwin->popup, iwin->zone->w, h);

   /* tell conformant apps our position and size */
   ecore_x_e_illume_indicator_geometry_set(iwin->zone->black_win, 
                                           iwin->win->x, iwin->win->y, 
                                           iwin->win->w, h);

   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_ind_win_cb_zone_resize(void *data, int type __UNUSED__, void *event) 
{
   Ind_Win *iwin;
   E_Event_Zone_Move_Resize *ev;
   Evas_Coord h = 0;

   ev = event;
   if (!(iwin = data)) return ECORE_CALLBACK_PASS_ON;
   if (ev->zone != iwin->zone) return ECORE_CALLBACK_PASS_ON;

   h = (il_ind_cfg->height * e_scale);

   /* set minimum size of this window to match zone size */
   e_win_size_min_set(iwin->win, ev->zone->w, h);
   ecore_evas_size_min_set(iwin->popup->ecore_evas, ev->zone->w, h);

   return ECORE_CALLBACK_PASS_ON;
}

static void 
_e_mod_ind_win_cb_resize(E_Win *win) 
{
   Ind_Win *iwin;

   if (!(iwin = win->data)) return;
   if (iwin->popup) e_popup_resize(iwin->popup, win->w, win->h);
   if (iwin->o_event) evas_object_resize(iwin->o_event, win->w, win->h);
   if (iwin->o_base) evas_object_resize(iwin->o_base, win->w, win->h);
   if (iwin->gadcon->o_container)
     edje_extern_object_min_size_set(iwin->gadcon->o_container, 
                                     win->w, win->h);
}

static void 
_e_mod_ind_win_cb_mouse_down(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event) 
{
   Ind_Win *iwin;
   Evas_Event_Mouse_Down *ev;

   ev = event;
   if (!(iwin = data)) return;
   if (ev->button == 1) 
     {
        iwin->mouse_down = 1;

        /* make sure we can drag */
        if (iwin->win->border->client.illume.drag.locked) return;

        iwin->drag.start = 1;
        iwin->drag.dnd = 0;
        iwin->drag.y = ev->output.y;
        iwin->drag.by = iwin->win->border->y;
     }
   else if (ev->button == 3) 
     {
        int x, y;

        /* create our popup menu */
        iwin->menu = e_menu_new();
        e_menu_post_deactivate_callback_set(iwin->menu, 
                                            _e_mod_ind_win_cb_menu_post, iwin);

        /* append items to our menu */
        _e_mod_ind_win_cb_menu_append(iwin, iwin->menu);

        /* show menu */
        e_gadcon_canvas_zone_geometry_get(iwin->gadcon, &x, &y, NULL, NULL);
        e_menu_activate_mouse(iwin->menu, iwin->zone, x + ev->output.x, 
                              y + ev->output.y, 1, 1, 
                              E_MENU_POP_DIRECTION_AUTO, ev->timestamp);
     }
}

static void 
_e_mod_ind_win_cb_mouse_up(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event) 
{
   Ind_Win *iwin;
   Evas_Event_Mouse_Up *ev;

   ev = event;
   if (!(iwin = data)) return;

   if (ev->button != 1) return;

   /* if we are not dragging, send message to toggle quickpanel state */
   if ((!iwin->drag.dnd) && (iwin->mouse_down == 1)) 
     {
        Ecore_X_Window xwin;

        xwin = iwin->win->border->zone->black_win;
        ecore_x_e_illume_quickpanel_state_toggle(xwin);
     }
   else if (iwin->drag.dnd) 
     {
        E_Border *bd;

        bd = iwin->win->border;

        /* reset mouse pointer */
        if (bd->pointer) 
          e_pointer_type_pop(bd->pointer, bd, "move");

        /* tell edj we are done moving */
        edje_object_signal_emit(iwin->o_base, "e,action,move,stop", "e");

        /* send message that we are done dragging */
        ecore_x_e_illume_drag_end_send(bd->client.win);

        /* update quickpanel position if needed */
        if (bd->y != iwin->drag.by) 
          ecore_x_e_illume_quickpanel_position_update_send(bd->client.win);

        /* tell conformant apps our position and size */
        ecore_x_e_illume_indicator_geometry_set(iwin->zone->black_win, 
                                                bd->x, bd->y, 
                                                bd->w, bd->h);
     }
   iwin->drag.start = 0;
   iwin->drag.dnd = 0;
   iwin->drag.y = 0;
   iwin->drag.by = 0;
   iwin->mouse_down = 0;
}

static void 
_e_mod_ind_win_cb_mouse_move(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event) 
{
   Ind_Win *iwin;
   Evas_Event_Mouse_Move *ev;
   E_Border *bd;

   ev = event;
   if (!(iwin = data)) return;
   bd = iwin->win->border;
   if (iwin->drag.start) 
     {
        iwin->drag.dnd = 1;
        iwin->drag.start = 0;

        /* change mouse pointer to indicate we are dragging */
        if (iwin->win->border->pointer) 
          e_pointer_type_push(iwin->win->border->pointer, 
                              iwin->win->border, "move");

        /* tell edj we are going to start moving */
        edje_object_signal_emit(iwin->o_base, "e,action,move,start", "e");

        /* tell quickpanel to hide because we are going to drag */
        ecore_x_e_illume_quickpanel_state_send(bd->zone->black_win, 
                                              ECORE_X_ILLUME_QUICKPANEL_STATE_OFF);

        /* send message that we are going to start dragging */
        ecore_x_e_illume_drag_start_send(bd->client.win);
     }

   /* make sure we are dragging */
   if (iwin->drag.dnd) 
     {
        int dy, py, ny;

        /* get current mouse position */
        py = ev->cur.output.y;

        /* do moves in 'chunks' of screen size */
        dy = ((bd->zone->h - bd->h) / 8);

        /* are we moving up or down ? */
        if (ev->cur.output.y > ev->prev.output.y) 
          {
             /* moving down */
             if ((py - iwin->drag.y) < dy) return;
          }
        else if (ev->cur.output.y < ev->prev.output.y) 
          {
             /* moving up */
             if ((iwin->drag.y - py) < dy) return;
          }
        else return;

        if (py > iwin->drag.y) 
          ny = bd->y + dy;
        else if (py < iwin->drag.y) 
          ny = bd->y - dy;
        else return;

        /* make sure we don't drag off the screen */
        if (ny < iwin->zone->y) 
          ny = iwin->zone->y;
        else if ((ny + bd->h) > (iwin->zone->y + iwin->zone->h)) 
          return;

        /* move the border if we need to */
        if (bd->y != ny) 
          {
             bd->y = ny;
             bd->changes.pos = 1;
             bd->changed = 1;
             e_popup_move(iwin->popup, iwin->popup->x, ny);
          }
     }
}

static void 
_e_mod_ind_win_cb_min_size_request(void *data, E_Gadcon *gc, Evas_Coord w, Evas_Coord h) 
{
   Ind_Win *iwin;

   if (!(iwin = data)) return;
   if (gc != iwin->gadcon) return;
   if (h < iwin->win->h) h = iwin->win->h;
   edje_extern_object_min_size_set(iwin->gadcon->o_container, w, h);
}

static void 
_e_mod_ind_win_cb_size_request(void *data __UNUSED__, E_Gadcon *gc __UNUSED__, Evas_Coord w __UNUSED__, Evas_Coord h __UNUSED__) 
{
   return;
}

static Evas_Object *
_e_mod_ind_win_cb_frame_request(void *data __UNUSED__, E_Gadcon_Client *gcc __UNUSED__, const char *style __UNUSED__) 
{
   return NULL;
}

static void 
_e_mod_ind_win_cb_menu_items_append(void *data, E_Gadcon_Client *gcc __UNUSED__, E_Menu *mn) 
{
   Ind_Win *iwin;

   if (!(iwin = data)) return;
   _e_mod_ind_win_cb_menu_append(iwin, mn);
}

static void 
_e_mod_ind_win_cb_menu_append(Ind_Win *iwin, E_Menu *mn) 
{
   E_Menu *subm;
   E_Menu_Item *mi;
   char buff[PATH_MAX];

   snprintf(buff, sizeof(buff), 
            "%s/e-module-illume-indicator.edj", _ind_mod_dir);

   subm = e_menu_new();
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Illume Indicator"));
   e_menu_item_icon_edje_set(mi, buff, "icon");
   e_menu_pre_activate_callback_set(subm, _e_mod_ind_win_cb_menu_pre, iwin);
   e_menu_item_submenu_set(mi, subm);
}

static void 
_e_mod_ind_win_cb_menu_pre(void *data, E_Menu *mn) 
{
   Ind_Win *iwin;
   E_Menu_Item *mi;

   if (!(iwin = data)) return;
   e_menu_pre_activate_callback_set(mn, NULL, NULL);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Set Contents"));
   e_util_menu_item_theme_icon_set(mi, "preferences-desktop-shelf");
   e_menu_item_callback_set(mi, _e_mod_ind_win_cb_menu_contents, iwin);

   mi = e_menu_item_new(mn);
   if (iwin->gadcon->editing) 
     e_menu_item_label_set(mi, _("End Move/Resize Items"));
   else
     e_menu_item_label_set(mi, _("Begin Move/Resize Items"));

   e_util_menu_item_theme_icon_set(mi, "transform-scale");
   e_menu_item_callback_set(mi, _e_mod_ind_win_cb_menu_edit, iwin);
}

static void 
_e_mod_ind_win_cb_menu_post(void *data, E_Menu *mn __UNUSED__) 
{
   Ind_Win *iwin;

   if (!(iwin = data)) return;
   if (!iwin->menu) return;
   e_object_del(E_OBJECT(iwin->menu));
   iwin->menu = NULL;
}

static void 
_e_mod_ind_win_cb_menu_contents(void *data, E_Menu *mn __UNUSED__, E_Menu_Item *mi __UNUSED__) 
{
   Ind_Win *iwin;

   if (!(iwin = data)) return;
   if (!iwin->gadcon->config_dialog) 
     e_int_gadcon_config_shelf(iwin->gadcon);
   else 
     {
        e_win_show(iwin->gadcon->config_dialog->dia->win);
        e_win_raise(iwin->gadcon->config_dialog->dia->win);
     }
}

static void 
_e_mod_ind_win_cb_menu_edit(void *data, E_Menu *mn __UNUSED__, E_Menu_Item *mi __UNUSED__) 
{
   Ind_Win *iwin;

   if (!(iwin = data)) return;
   if (iwin->gadcon->editing) 
     e_gadcon_edit_end(iwin->gadcon);
   else
     e_gadcon_edit_begin(iwin->gadcon);
}

static Eina_Bool 
_e_mod_ind_win_cb_border_hide(void *data, int type __UNUSED__, void *event) 
{
   Ind_Win *iwin;
   E_Event_Border_Hide *ev;

   if (!(iwin = data)) return ECORE_CALLBACK_PASS_ON;
   ev = event;
   if (ev->border != iwin->win->border) return ECORE_CALLBACK_PASS_ON;
   e_popup_hide(iwin->popup);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool 
_e_mod_ind_win_cb_border_show(void *data, int type __UNUSED__, void *event) 
{
   Ind_Win *iwin;
   E_Event_Border_Show *ev;

   if (!(iwin = data)) return ECORE_CALLBACK_PASS_ON;
   ev = event;
   if (ev->border != iwin->win->border) return ECORE_CALLBACK_PASS_ON;
   e_popup_show(iwin->popup);
   return ECORE_CALLBACK_PASS_ON;
}
