#include "e.h"
#include "e_mod_main.h"
#include "e_mod_ind_win.h"

/* local function prototypes */
static void _e_mod_ind_win_cb_free(Il_Ind_Win *iwin);
static void _e_mod_ind_win_cb_hook_eval_end(void *data, void *data2);
static void _e_mod_ind_win_cb_resize(E_Win *win);
static void _e_mod_ind_win_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event);
static void _e_mod_ind_win_cb_mouse_up(void *data, Evas *evas, Evas_Object *obj, void *event);
static void _e_mod_ind_win_cb_mouse_move(void *data, Evas *evas, Evas_Object *obj, void *event);
static void _e_mod_ind_win_cb_menu_items_append(void *data, E_Gadcon_Client *gcc, E_Menu *mn);
static void _e_mod_ind_win_cb_menu_append(Il_Ind_Win *iwin, E_Menu *mn);
static void _e_mod_ind_win_cb_menu_pre(void *data, E_Menu *mn);
static void _e_mod_ind_win_cb_menu_contents(void *data, E_Menu *mn, E_Menu_Item *mi);
static void _e_mod_ind_win_cb_menu_post(void *data, E_Menu *mn);
static void _e_mod_ind_win_cb_min_size_request(void *data, E_Gadcon *gc, Evas_Coord w, Evas_Coord h);
static void _e_mod_ind_win_cb_size_request(void *data, E_Gadcon *gc, Evas_Coord w, Evas_Coord h);
static Evas_Object *_e_mod_ind_win_cb_frame_request(void *data, E_Gadcon_Client *gcc, const char *style);

Il_Ind_Win *
e_mod_ind_win_new(E_Zone *zone) 
{
   Evas *evas;
   Il_Ind_Win *iwin;
   Ecore_X_Window_State states[2];
   Ecore_X_Illume_Mode mode;

   /* allocate our new indicator window object */
   iwin = E_OBJECT_ALLOC(Il_Ind_Win, IL_IND_WIN_TYPE, _e_mod_ind_win_cb_free);
   if (!iwin) return NULL;
   iwin->zone = zone;

   /* we hook into eval end so we can set this indicator on the right zone */
   iwin->hook = e_border_hook_add(E_BORDER_HOOK_EVAL_END, 
                                  _e_mod_ind_win_cb_hook_eval_end, iwin);

   /* create the new indicator window */
   iwin->win = e_win_new(zone->container);
   iwin->win->data = iwin;
   e_win_title_set(iwin->win, _("Illume Indicator"));
   e_win_name_class_set(iwin->win, "Illume-Indicator", "Illume-Indicator");
   e_win_resize_callback_set(iwin->win, _e_mod_ind_win_cb_resize);

   /* set this window to not show in taskbar and pager */
   states[0] = ECORE_X_WINDOW_STATE_SKIP_TASKBAR;
   states[1] = ECORE_X_WINDOW_STATE_SKIP_PAGER;
   ecore_x_netwm_window_state_set(iwin->win->evas_win, states, 2);

   /* set this window to be a 'dock' window */
   ecore_x_netwm_window_type_set(iwin->win->evas_win, ECORE_X_WINDOW_TYPE_DOCK);

   /* set this window to not accept or take focus */
   ecore_x_icccm_hints_set(iwin->win->evas_win, 0, 0, 0, 0, 0, 0, 0);

   evas = e_win_evas_get(iwin->win);

   /* create our event rectangle */
   iwin->o_event = evas_object_rectangle_add(evas);
   evas_object_color_set(iwin->o_event, 0, 0, 0, 0);
   evas_object_move(iwin->o_event, 0, 0);
   evas_object_event_callback_add(iwin->o_event, EVAS_CALLBACK_MOUSE_DOWN, 
                                  _e_mod_ind_win_cb_mouse_down, iwin);
   evas_object_event_callback_add(iwin->o_event, EVAS_CALLBACK_MOUSE_UP, 
                                  _e_mod_ind_win_cb_mouse_up, iwin);
   evas_object_event_callback_add(iwin->o_event, EVAS_CALLBACK_MOUSE_MOVE, 
                                  _e_mod_ind_win_cb_mouse_move, iwin);
   evas_object_show(iwin->o_event);

   /* create our base object */
   iwin->o_base = edje_object_add(evas);
   if (!e_theme_edje_object_set(iwin->o_base, 
                                "base/theme/modules/illume-indicator", 
                                "modules/illume-indicator/shelf")) 
     {
        char buff[PATH_MAX];

        memset(buff, 0, sizeof(buff));
        snprintf(buff, sizeof(buff), "%s/e-module-illume-indicator.edj", 
                 _ind_mod_dir);
        edje_object_file_set(iwin->o_base, buff, 
                             "modules/illume-indicator/shelf");
        memset(buff, 0, sizeof(buff));
     }
   evas_object_move(iwin->o_base, 0, 0);
   evas_object_show(iwin->o_base);

   /* create our gadget container */
   iwin->gadcon = e_gadcon_swallowed_new("illume-indicator", zone->id, 
                                         iwin->o_base, "e.swallow.content");

   edje_extern_object_min_size_set(iwin->gadcon->o_container, zone->w, 32);
   edje_object_part_swallow(iwin->o_base, "e.swallow.container", 
                            iwin->gadcon->o_container);
   e_gadcon_min_size_request_callback_set(iwin->gadcon, 
                                          _e_mod_ind_win_cb_min_size_request, iwin);
   e_gadcon_size_request_callback_set(iwin->gadcon, 
                                      _e_mod_ind_win_cb_size_request, iwin);
   e_gadcon_frame_request_callback_set(iwin->gadcon, 
                                       _e_mod_ind_win_cb_frame_request, iwin);
   e_gadcon_orient(iwin->gadcon, E_GADCON_ORIENT_FLOAT);
   e_gadcon_zone_set(iwin->gadcon, zone);
   e_gadcon_ecore_evas_set(iwin->gadcon, iwin->win->ecore_evas);
   e_gadcon_util_menu_attach_func_set(iwin->gadcon, 
                                      _e_mod_ind_win_cb_menu_items_append, iwin);
   e_gadcon_populate(iwin->gadcon);

   /* set minimum size of window */
   e_win_size_min_set(iwin->win, zone->w, (32 * e_scale));

   /* position and resize the window */
   e_win_move_resize(iwin->win, zone->x, zone->y, zone->w, (32 * e_scale));

   /* show the window */
   e_win_show(iwin->win);

   /* get the current illume mode and lock dragging if we need to */
   mode = ecore_x_e_illume_mode_get(zone->black_win);
   if (mode == ECORE_X_ILLUME_MODE_DUAL_TOP)
     iwin->win->border->client.illume.drag.locked = 0;
   else
     iwin->win->border->client.illume.drag.locked = 1;

   /* tell illume conformant apps our position and size */
   ecore_x_e_illume_top_shelf_geometry_set(ecore_x_window_root_first_get(), 
                                           zone->x, zone->y, zone->w, 
                                           (32 * e_scale));
   return iwin;
}

/* local functions */
static void 
_e_mod_ind_win_cb_free(Il_Ind_Win *iwin) 
{
   /* delete the menu */
   if (iwin->menu) 
     {
        e_menu_post_deactivate_callback_set(iwin->menu, NULL, NULL);
        e_object_del(E_OBJECT(iwin->menu));
     }
   iwin->menu = NULL;

   /* delete the border hook */
   if (iwin->hook) e_border_hook_del(iwin->hook);
   iwin->hook = NULL;

   /* delete the objects */
   if (iwin->o_event) evas_object_del(iwin->o_event);
   if (iwin->o_base) evas_object_del(iwin->o_base);

   /* delete the gadget container */
   e_object_del(E_OBJECT(iwin->gadcon));
   iwin->gadcon = NULL;

   /* delete the window */
   e_object_del(E_OBJECT(iwin->win));
   iwin->win = NULL;

   /* free the object */
   E_FREE(iwin);
}

static void 
_e_mod_ind_win_cb_hook_eval_end(void *data, void *data2) 
{
   Il_Ind_Win *iwin;
   E_Border *bd;

   if (!(iwin = data)) return;
   if (!(bd = data2)) return;
   if (bd != iwin->win->border) return;
   if (bd->zone != iwin->zone) 
     {
        bd->x = iwin->zone->x;
        bd->y = iwin->zone->y;
        bd->changes.pos = 1;
        bd->changed = 1;
        e_border_zone_set(bd, iwin->zone);
     }
   bd->lock_user_location = 1;
}

static void 
_e_mod_ind_win_cb_resize(E_Win *win) 
{
   Il_Ind_Win *iwin;

   if (!(iwin = win->data)) return;
   evas_object_resize(iwin->o_event, win->w, win->h);
   evas_object_resize(iwin->o_base, win->w, win->h);
}

static void 
_e_mod_ind_win_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event) 
{
   Il_Ind_Win *iwin;
   Evas_Event_Mouse_Down *ev;

   if (!(iwin = data)) return;
   ev = event;
   if (ev->button == 1) 
     {
        iwin->mouse_down = 1;

        /* make sure we are not drag locked */
        if (iwin->win->border->client.illume.drag.locked) return;

        iwin->drag.start = 1;
        iwin->drag.dnd = 0;

        /* grab the mouse position */
        ecore_x_pointer_last_xy_get(NULL, &iwin->drag.y);

        /* change the pointer to indicate we are dragging */
        if (iwin->win->border->pointer) 
             e_pointer_type_push(iwin->win->border->pointer, 
                                 iwin->win->border, "move");
     }
   else if (ev->button == 3) 
     {
        E_Menu *mn;
        int x, y;

        /* create the menu */
        mn = e_menu_new();
        e_menu_post_deactivate_callback_set(mn, _e_mod_ind_win_cb_menu_post, 
                                            iwin);
        iwin->menu = mn;

        /* append menu items and show it */
        _e_mod_ind_win_cb_menu_append(iwin, mn);
        e_gadcon_canvas_zone_geometry_get(iwin->gadcon, &x, &y, NULL, NULL);
        e_menu_activate_mouse(mn, iwin->zone, 
                              x + ev->output.x, y + ev->output.y, 
                              1, 1, E_MENU_POP_DIRECTION_AUTO, ev->timestamp);
     }
}

static void 
_e_mod_ind_win_cb_mouse_up(void *data, Evas *evas, Evas_Object *obj, void *event) 
{
   Il_Ind_Win *iwin;
   Evas_Event_Mouse_Up *ev;
   Ecore_X_Window xwin;

   ev = event;
   if (ev->button != 1) return;
   if (!(iwin = data)) return;
   xwin = iwin->win->border->zone->black_win;
   if ((!iwin->drag.dnd) && (iwin->mouse_down == 1)) 
     {
        /* show the quickpanel */
        ecore_x_e_illume_quickpanel_state_send
          (xwin, ECORE_X_ILLUME_QUICKPANEL_STATE_ON);
     }
   else if (iwin->drag.dnd) 
     {
        E_Border *bd;

        bd = iwin->win->border;

        /* reset mouse pointer */
        if (bd->pointer) e_pointer_type_pop(bd->pointer, bd, "move");

        /* tell edj that we are done moving */
        edje_object_signal_emit(iwin->o_base, "e,action,move,stop", "e");

        /* send msg that we are done dragging */
        ecore_x_e_illume_drag_end_send(bd->client.win);

        /* update quickpanel position */
        ecore_x_e_illume_quickpanel_position_update_send(bd->client.win);
     }
   iwin->drag.start = 0;
   iwin->drag.dnd = 0;
   iwin->drag.y = 0;
   iwin->mouse_down = 0;
}

static void 
_e_mod_ind_win_cb_mouse_move(void *data, Evas *evas, Evas_Object *obj, void *event) 
{
   Il_Ind_Win *iwin;
   Evas_Event_Mouse_Move *ev;
   E_Border *bd;
   int dy, ny, py;

   if (!(iwin = data)) return;

   ev = event;
   bd = iwin->win->border;

   if (iwin->drag.start) 
     {
        iwin->drag.dnd = 1;
        iwin->drag.start = 0;

        /* tell edj that we are gonna start moving */
        edje_object_signal_emit(iwin->o_base, "e,action,move,start", "e");

        /* send msg that we are gonna start dragging */
        ecore_x_e_illume_drag_start_send(bd->client.win);
     }

   /* if we are not dragging, leave */
   if (!iwin->drag.dnd) return;

   /* make sure we are not gonna drag past the zone boundary */
   if ((bd->y + bd->h + ev->cur.output.y) >= bd->zone->h) return;

   /* grab mouse position */
   ecore_x_pointer_last_xy_get(NULL, &py);

   dy = ((bd->zone->h - bd->h) / 8);

   if (ev->cur.output.y > ev->prev.output.y) 
     {
        if ((py - iwin->drag.y) < dy) return;
     }
   else if (ev->cur.output.y < ev->prev.output.y)
     {
        if ((iwin->drag.y - py) < dy) return;
     }
   else return;

   if (py > iwin->drag.y) 
     ny = bd->y + dy;
   else if (py < iwin->drag.y) 
     ny = bd->y - dy;
   else return;

   if (bd->y != ny) 
     {
        bd->x = bd->zone->x;
        bd->y = ny;
        bd->changes.pos = 1;
        bd->changed = 1;
        iwin->drag.y = py;
     }
}

static void 
_e_mod_ind_win_cb_menu_items_append(void *data, E_Gadcon_Client *gcc, E_Menu *mn) 
{
   Il_Ind_Win *iwin;

   if (!(iwin = data)) return;
   _e_mod_ind_win_cb_menu_append(iwin, mn);
}

static void 
_e_mod_ind_win_cb_menu_append(Il_Ind_Win *iwin, E_Menu *mn) 
{
   E_Menu *sm;
   E_Menu_Item *mi;

   sm = e_menu_new();
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Illume Indicator"));
   e_util_menu_item_theme_icon_set(mi, "preferences-desktop-shelf");
   e_menu_pre_activate_callback_set(sm, _e_mod_ind_win_cb_menu_pre, iwin);
   e_object_data_set(E_OBJECT(mi), iwin);
   e_menu_item_submenu_set(mi, sm);
}

static void 
_e_mod_ind_win_cb_menu_pre(void *data, E_Menu *mn) 
{
   Il_Ind_Win *iwin;
   E_Menu_Item *mi;

   if (!(iwin = data)) return;
   e_menu_pre_activate_callback_set(mn, NULL, NULL);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Set Contents"));
   e_util_menu_item_theme_icon_set(mi, "preferences-desktop-shelf");
   e_menu_item_callback_set(mi, _e_mod_ind_win_cb_menu_contents, iwin);
}

static void 
_e_mod_ind_win_cb_menu_contents(void *data, E_Menu *mn, E_Menu_Item *mi) 
{
   Il_Ind_Win *iwin;

   if (!(iwin = data)) return;
   if (!iwin->gadcon->config_dialog) 
     e_int_gadcon_config_shelf(iwin->gadcon);
}

static void 
_e_mod_ind_win_cb_menu_post(void *data, E_Menu *mn) 
{
   Il_Ind_Win *iwin;

   if (!(iwin = data)) return;
   if (!iwin->menu) return;
   e_object_del(E_OBJECT(iwin->menu));
   iwin->menu = NULL;
}

static void 
_e_mod_ind_win_cb_min_size_request(void *data, E_Gadcon *gc, Evas_Coord w, Evas_Coord h) 
{
   Il_Ind_Win *iwin;

   if (!(iwin = data)) return;
   if (gc != iwin->gadcon) return;
   if (h < (32 * e_scale)) h = (32 * e_scale);
   edje_extern_object_min_size_set(iwin->gadcon->o_container, w, h);
   edje_object_part_swallow(iwin->o_base, "e.swallow.content", 
                            iwin->gadcon->o_container);
   evas_object_resize(iwin->o_base, iwin->win->w, iwin->win->h);
}

static void 
_e_mod_ind_win_cb_size_request(void *data, E_Gadcon *gc, Evas_Coord w, Evas_Coord h) 
{
   return;
}

static Evas_Object *
_e_mod_ind_win_cb_frame_request(void *data, E_Gadcon_Client *gcc, const char *style) 
{
   return NULL;
}
