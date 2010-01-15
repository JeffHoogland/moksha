#include "e.h"
#include "e_mod_main.h"
#include "e_mod_win.h"

/* local function prototypes */
static void _e_mod_win_cb_free(Il_Ind_Win *iwin);
static void _e_mod_win_cb_resize(E_Win *win);
static void _e_mod_win_cb_min_size_request(void *data, E_Gadcon *gc, Evas_Coord w, Evas_Coord h);
static void _e_mod_win_cb_size_request(void *data, E_Gadcon *gc, Evas_Coord w, Evas_Coord h);
static Evas_Object *_e_mod_win_cb_frame_request(void *data, E_Gadcon_Client *gcc, const char *style);
static void _e_mod_win_cb_menu_items_append(void *data, E_Gadcon_Client *gcc, E_Menu *mn);
static void _e_mod_win_cb_menu_append(Il_Ind_Win *iwin, E_Menu *mn);
static void _e_mod_win_cb_menu_pre(void *data, E_Menu *mn);
static void _e_mod_win_cb_menu_contents(void *data, E_Menu *mn, E_Menu_Item *mi);
static void _e_mod_win_cb_menu_post(void *data, E_Menu *mn);
static void _e_mod_win_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event);
static void _e_mod_win_cb_mouse_up(void *data, Evas *evas, Evas_Object *obj, void *event);
static void _e_mod_win_cb_mouse_move(void *data, Evas *evas, Evas_Object *obj, void *event);
static void _e_mod_win_cb_mouse_wheel(void *data, Evas *evas, Evas_Object *obj, void *event);

/* local variables */
static int my = 0;

Il_Ind_Win *
e_mod_win_new(E_Zone *zone) 
{
   Il_Ind_Win *iwin;
   Evas *evas;
   Ecore_X_Window_State states[2];
   Ecore_X_Illume_Mode mode;

   iwin = E_OBJECT_ALLOC(Il_Ind_Win, IL_IND_WIN_TYPE, _e_mod_win_cb_free);
   if (!iwin) return NULL;

   iwin->dragging = 0;
   iwin->win = e_win_new(zone->container);
   iwin->win->data = iwin;
   states[0] = ECORE_X_WINDOW_STATE_SKIP_TASKBAR;
   states[1] = ECORE_X_WINDOW_STATE_SKIP_PAGER;
   e_win_title_set(iwin->win, _("Illume Indicator"));
   e_win_name_class_set(iwin->win, "Illume-Indicator", "Illume-Indicator");
   e_win_resize_callback_set(iwin->win, _e_mod_win_cb_resize);
   ecore_x_icccm_hints_set(iwin->win->evas_win, 0, 0, 0, 0, 0, 0, 0);
   ecore_x_netwm_window_state_set(iwin->win->evas_win, states, 2);
   ecore_x_netwm_window_type_set(iwin->win->evas_win, ECORE_X_WINDOW_TYPE_DOCK);

   evas = e_win_evas_get(iwin->win);

   iwin->o_event = evas_object_rectangle_add(evas);
   evas_object_color_set(iwin->o_event, 0, 0, 0, 0);
   evas_object_move(iwin->o_event, 0, 0);
   evas_object_event_callback_add(iwin->o_event, EVAS_CALLBACK_MOUSE_DOWN, 
                                  _e_mod_win_cb_mouse_down, iwin);
   evas_object_event_callback_add(iwin->o_event, EVAS_CALLBACK_MOUSE_UP, 
                                  _e_mod_win_cb_mouse_up, iwin);
   evas_object_event_callback_add(iwin->o_event, EVAS_CALLBACK_MOUSE_MOVE, 
                                  _e_mod_win_cb_mouse_move, iwin);
   evas_object_event_callback_add(iwin->o_event, EVAS_CALLBACK_MOUSE_WHEEL, 
                                  _e_mod_win_cb_mouse_wheel, iwin);
   evas_object_show(iwin->o_event);

   iwin->o_base = edje_object_add(evas);
   if (!e_theme_edje_object_set(iwin->o_base, 
                                "base/theme/modules/illume-indicator", 
                                "modules/illume-indicator/shelf")) 
     {
        char buff[PATH_MAX];

        snprintf(buff, sizeof(buff), "%s/e-module-illume-indicator.edj", 
                 mod_dir);
        edje_object_file_set(iwin->o_base, buff, 
                             "modules/illume-indicator/shelf");
     }
   evas_object_move(iwin->o_base, 0, 0);
   evas_object_show(iwin->o_base);

   iwin->gadcon = e_gadcon_swallowed_new("illume-indicator", zone->id, 
                                         iwin->o_base, "e.swallow.content");

   edje_extern_object_min_size_set(iwin->gadcon->o_container, zone->w, 32);
   edje_object_part_swallow(iwin->o_base, "e.swallow.container", 
                            iwin->gadcon->o_container);
   e_gadcon_min_size_request_callback_set(iwin->gadcon, 
                                          _e_mod_win_cb_min_size_request, iwin);
   e_gadcon_size_request_callback_set(iwin->gadcon, 
                                      _e_mod_win_cb_size_request, iwin);
   e_gadcon_frame_request_callback_set(iwin->gadcon, 
                                       _e_mod_win_cb_frame_request, iwin);
   e_gadcon_orient(iwin->gadcon, E_GADCON_ORIENT_FLOAT);
   e_gadcon_zone_set(iwin->gadcon, zone);
   e_gadcon_ecore_evas_set(iwin->gadcon, iwin->win->ecore_evas);
   e_gadcon_util_menu_attach_func_set(iwin->gadcon, 
                                      _e_mod_win_cb_menu_items_append, iwin);
   e_gadcon_populate(iwin->gadcon);

   e_win_size_min_set(iwin->win, zone->w, 32);
   e_win_move_resize(iwin->win, zone->x, zone->y, zone->w, 32);
   e_win_show(iwin->win);
   e_border_zone_set(iwin->win->border, zone);
//   e_win_placed_set(iwin->win, 1);
//   iwin->win->border->lock_user_location = 1;

   mode = ecore_x_e_illume_mode_get(zone->black_win);
   if (mode < ECORE_X_ILLUME_MODE_DUAL_TOP)
     iwin->win->border->client.illume.drag.locked = 1;
   else if (mode == ECORE_X_ILLUME_MODE_DUAL_TOP)
     iwin->win->border->client.illume.drag.locked = 0;
   else if (mode == ECORE_X_ILLUME_MODE_DUAL_LEFT)
     iwin->win->border->client.illume.drag.locked = 1;

   ecore_x_e_illume_top_shelf_geometry_set(ecore_x_window_root_first_get(), 
                                           zone->x, zone->y, zone->w, 32);

   return iwin;
}

/* local functions */
static void 
_e_mod_win_cb_free(Il_Ind_Win *iwin) 
{
   if (iwin->menu) 
     {
        e_menu_post_deactivate_callback_set(iwin->menu, NULL, NULL);
        e_object_del(E_OBJECT(iwin->menu));
        iwin->menu = NULL;
     }

   if (iwin->o_base) evas_object_del(iwin->o_base);
   if (iwin->o_event) evas_object_del(iwin->o_event);

   e_object_del(E_OBJECT(iwin->gadcon));
   iwin->gadcon = NULL;

   e_object_del(E_OBJECT(iwin->win));
   iwin->win = NULL;

   E_FREE(iwin);
}

static void 
_e_mod_win_cb_resize(E_Win *win) 
{
   Il_Ind_Win *iwin;

   if (!(iwin = win->data)) return;
   evas_object_resize(iwin->o_event, win->w, win->h);
   evas_object_resize(iwin->o_base, win->w, win->h);
}

static void 
_e_mod_win_cb_min_size_request(void *data, E_Gadcon *gc, Evas_Coord w, Evas_Coord h) 
{
   Il_Ind_Win *iwin;

   if (!(iwin = data)) return;
   if (gc != iwin->gadcon) return;
   if (h < 32) h = 32;
   edje_extern_object_min_size_set(iwin->gadcon->o_container, w, h);
   edje_object_part_swallow(iwin->o_base, "e.swallow.content", 
                            iwin->gadcon->o_container);
   evas_object_resize(iwin->o_base, iwin->win->w, iwin->win->h);
}

static void 
_e_mod_win_cb_size_request(void *data, E_Gadcon *gc, Evas_Coord w, Evas_Coord h) 
{
   return;
}

static Evas_Object *
_e_mod_win_cb_frame_request(void *data, E_Gadcon_Client *gcc, const char *style) 
{
   return NULL;
}

static void 
_e_mod_win_cb_menu_items_append(void *data, E_Gadcon_Client *gcc, E_Menu *mn) 
{
   Il_Ind_Win *iwin;

   if (!(iwin = data)) return;
   _e_mod_win_cb_menu_append(iwin, mn);
}

static void 
_e_mod_win_cb_menu_append(Il_Ind_Win *iwin, E_Menu *mn) 
{
   E_Menu *sm;
   E_Menu_Item *mi;

   sm = e_menu_new();
   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Illume Indicator"));
   e_util_menu_item_theme_icon_set(mi, "preferences-desktop-shelf");
   e_menu_pre_activate_callback_set(sm, _e_mod_win_cb_menu_pre, iwin);
   e_object_data_set(E_OBJECT(mi), iwin);
   e_menu_item_submenu_set(mi, sm);
}

static void 
_e_mod_win_cb_menu_pre(void *data, E_Menu *mn) 
{
   Il_Ind_Win *iwin;
   E_Menu_Item *mi;

   if (!(iwin = data)) return;
   e_menu_pre_activate_callback_set(mn, NULL, NULL);

   mi = e_menu_item_new(mn);
   e_menu_item_label_set(mi, _("Set Contents"));
   e_util_menu_item_theme_icon_set(mi, "preferences-desktop-shelf");
   e_menu_item_callback_set(mi, _e_mod_win_cb_menu_contents, iwin);
}

static void 
_e_mod_win_cb_menu_contents(void *data, E_Menu *mn, E_Menu_Item *mi) 
{
   Il_Ind_Win *iwin;

   if (!(iwin = data)) return;
   if (!iwin->gadcon->config_dialog) 
     e_int_gadcon_config_shelf(iwin->gadcon);
}

static void 
_e_mod_win_cb_menu_post(void *data, E_Menu *mn) 
{
   Il_Ind_Win *iwin;

   if (!(iwin = data)) return;
   if (!iwin->menu) return;
   e_object_del(E_OBJECT(iwin->menu));
   iwin->menu = NULL;
}

static void 
_e_mod_win_cb_mouse_down(void *data, Evas *evas, Evas_Object *obj, void *event) 
{
   Il_Ind_Win *iwin;
   Evas_Event_Mouse_Down *ev;

   if (!(iwin = data)) return;
   ev = event;
   if (ev->button == 1)
     {
        if (iwin->win->border->client.illume.drag.locked) return;
        ecore_x_e_illume_drag_start_send(iwin->win->border->client.win);
        ecore_x_pointer_last_xy_get(NULL, &my);
        iwin->dragging = 1;
     }
   else if (ev->button == 3) 
     {
        E_Menu *mn;
        E_Zone *zone;
        int x, y;

        mn = e_menu_new();
        e_menu_post_deactivate_callback_set(mn, _e_mod_win_cb_menu_post, iwin);
        iwin->menu = mn;

        _e_mod_win_cb_menu_append(iwin, mn);
        zone = e_util_zone_current_get(e_manager_current_get());
        e_gadcon_canvas_zone_geometry_get(iwin->gadcon, &x, &y, NULL, NULL);
        e_menu_activate_mouse(mn, zone, x + ev->output.x, y + ev->output.y, 
                              1, 1, E_MENU_POP_DIRECTION_AUTO, ev->timestamp);
     }
}

static void 
_e_mod_win_cb_mouse_up(void *data, Evas *evas, Evas_Object *obj, void *event) 
{
   Il_Ind_Win *iwin;
   Evas_Event_Mouse_Up *ev;
   E_Border *bd;

   if (!(iwin = data)) return;
   ev = event;
   if (ev->button != 1) return;
   bd = iwin->win->border;
   if (bd->client.illume.drag.locked) return;
   iwin->dragging = 0;
   ecore_x_e_illume_drag_end_send(bd->client.win);
   my = 0;
}

static void 
_e_mod_win_cb_mouse_move(void *data, Evas *evas, Evas_Object *obj, void *event) 
{
   Il_Ind_Win *iwin;
   Evas_Event_Mouse_Move *ev;
   E_Border *bd;
   int dy, ny, py;

   if (!(iwin = data)) return;
   ev = event;
   bd = iwin->win->border;
   if (bd->client.illume.drag.locked) return;
   if (!iwin->dragging) return;
   if ((bd->y + bd->h + ev->cur.output.y) >= (bd->zone->h)) return;

   ecore_x_pointer_last_xy_get(NULL, &py);
   dy = ((bd->zone->h - bd->h) / 8);

   if (ev->cur.output.y > ev->prev.output.y) 
     {
        if ((py - my) < dy) return;
     }
   else if (ev->cur.output.y < ev->prev.output.y)
     {
        if ((my - py) < dy) return;
     }
   else return;

   if (py > my) 
     ny = bd->y + dy;
   else if (py < my) 
     ny = bd->y - dy;
   else return;

   if (bd->y != ny) 
     {
        e_border_move(bd, bd->zone->x, ny);
        my = py;
     }
}

static void 
_e_mod_win_cb_mouse_wheel(void *data, Evas *evas, Evas_Object *obj, void *event) 
{
   Il_Ind_Win *iwin;
   Evas_Event_Mouse_Wheel *ev;
   Ecore_X_Illume_Quickpanel_State state;

   if (!(iwin = data)) return;
   ev = event;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return;
   if (ev->direction != 0) return;
   if (ev->z > 0) 
     state = ECORE_X_ILLUME_QUICKPANEL_STATE_ON;
   else if (ev->z < 0) 
     state = ECORE_X_ILLUME_QUICKPANEL_STATE_OFF;
   ecore_x_e_illume_quickpanel_state_send(ecore_x_window_root_first_get(), state);
}
