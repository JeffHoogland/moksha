#include "e_mod_main.h"

//////////////////////////////////////////////////////////////////////////////
// globals
static Evas_Object *o_img, *o_rend, *win2, *o_events, *o_scroll;
static Evas_Object *o_col1, *o_col2;
static int img_w, img_h,  edit_w, edit_h;

static int window_pad = 64;

typedef enum
{
   TOOL_NONE,
   TOOL_CROP,
   TOOL_MODIFY,
   TOOL_DELETE,
   TOOL_BOX,
   TOOL_LINE
} Tool_Mode;

typedef struct
{
   int r, g, b, a;
} Tool_Color;

static Tool_Mode tool_mode = TOOL_NONE;
static int color_sel = 0;
Tool_Color color[2] =
{
   { 255, 255, 255, 255 },
   {   0,   0,   0, 255 }
};

//
//////////////////////////////////////////////////////////////////////////////

static inline double
to_rad(double deg)
{
   return (deg * M_PI * 2.0) / 360.0;
}

static inline double
to_deg(double rad)
{
   return (rad * 360.0) / (M_PI * 2.0);
}

static inline int
premul(int v, int a)
{
   return (v * a) / 255;
}

//////////////////////////////////////////////////////////////////////////////
// draw/modify handling objects

typedef enum
{
   MODIFY_NONE,
   MODIFY_LINE,
   MODIFY_BOX
} Modify_Mode;

typedef enum
{
   MODIFY_BOX_NONE,
   MODIFY_BOX_MOVE,
   MODIFY_BOX_RESIZE_TL,
   MODIFY_BOX_RESIZE_TR,
   MODIFY_BOX_RESIZE_BL,
   MODIFY_BOX_RESIZE_BR,
   MODIFY_BOX_ROTATE_TL,
   MODIFY_BOX_ROTATE_TR,
   MODIFY_BOX_ROTATE_BL,
   MODIFY_BOX_ROTATE_BR
} Modify_Box_Mode;

static Eina_List   *draw_objects = NULL;
static Evas_Object *o_draw_handles[2] = { NULL, NULL };
static Eina_Bool    modify_down = EINA_FALSE;
static int          modify_down_x = 0;
static int          modify_down_y = 0;
static int          modify_x = 0;
static int          modify_y = 0;
static int          modify_line_x1 = 0;
static int          modify_line_y1 = 0;
static int          modify_line_x2 = 0;
static int          modify_line_y2 = 0;
static int          modify_box_x1 = 0;
static int          modify_box_y1 = 0;
static int          modify_box_x2 = 0;
static int          modify_box_y2 = 0;
static double       modify_box_angle = 0.0;
static int          modify_handle_offx = 0;
static int          modify_handle_offy = 0;
static Modify_Mode  modify_mode = MODIFY_NONE;

static Eina_Bool       modify_box_rotate = EINA_FALSE;
static Modify_Box_Mode modify_box_mode = MODIFY_BOX_NONE;

static void colorsel_set(void);
static void draw_selectable_set(Eina_Bool sel);
static void line_clear(void);
static void line_modify_begin(Evas_Object *o, int x1, int y1, int x2, int y2, int inset);
static void line_modify_coord_set(int x1, int y1, int x2, int y2);
static void line_modify_coord_get(int *x1, int *y1, int *x2, int *y2);
static void line_eval(void);
static void box_clear(void);
static void box_modify_begin(Evas_Object *o, int x1, int y1, int x2, int y2, double ang);
static void box_modify_coord_set(int x1, int y1, int x2, int y2, double ang);
static void box_modify_coord_get(int *x1, int *y1, int *x2, int *y2, double *ang);
static Evas_Object *box_obj_get(void);
static void box_eval(void);

static void
draw_handle_line_update(void)
{
   int x1, y1, x2, y2;

   line_modify_coord_get(&x1, &y1, &x2, &y2);
   evas_object_move(o_draw_handles[0],
                    x1 + modify_handle_offx, y1 + modify_handle_offy);
   evas_object_move(o_draw_handles[1],
                    x2 + modify_handle_offx, y2 + modify_handle_offy);
}

static void
draw_handle_box_update(void)
{
   Evas_Object *o = box_obj_get();
   Evas_Coord x, y, w, h;
   const Evas_Map *m0;
   Evas_Map *m;

   evas_object_geometry_get(o, &x, &y, &w, &h);
   evas_object_geometry_set(o_draw_handles[0], x, y, w, h);
   m0 = evas_object_map_get(o);
   if (m0)
     {
        m = evas_map_dup(m0);
        evas_object_map_set(o_draw_handles[0], m);
        evas_map_free(m);
        evas_object_map_enable_set(o_draw_handles[0], EINA_TRUE);
        evas_object_show(o_draw_handles[0]);
     }
}

static void
_cb_modify_mouse_down(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *info)
{
   Evas_Event_Mouse_Down *ev = info;

   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return;
   if (ev->button == 1)
     {
        modify_down = EINA_TRUE;
        evas_pointer_canvas_xy_get(evas_object_evas_get(win2), &modify_x, &modify_y);
        modify_down_x = modify_x;
        modify_down_y = modify_y;
        if (modify_mode == MODIFY_LINE)
          {
             line_modify_coord_get(&modify_line_x1, &modify_line_y1,
                                   &modify_line_x2, &modify_line_y2);
          }
        else if (modify_mode == MODIFY_BOX)
          {
             box_modify_coord_get(&modify_box_x1, &modify_box_y1,
                                  &modify_box_x2, &modify_box_y2,
                                  &modify_box_angle);
          }
        elm_object_scroll_hold_push(o_scroll);
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
}

static void
_cb_modify_mouse_up(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *info)
{
   Evas_Event_Mouse_Up *ev = info;

   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return;
   if (ev->button == 1)
     {
        int dx, dy;

        if (!modify_down) return;
        modify_down = EINA_FALSE;
        elm_object_scroll_hold_pop(o_scroll);
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
        dx = modify_x - modify_down_x;
        dy = modify_y - modify_down_y;
        if ((modify_box_mode == MODIFY_BOX_MOVE) &&
            (((dx * dx) + (dy * dy)) < (5 * 5)))
          {
             modify_box_rotate = !modify_box_rotate;
             if (modify_box_rotate)
               elm_layout_signal_emit(o_draw_handles[0], "e,state,resize", "e");
             else
               elm_layout_signal_emit(o_draw_handles[0], "e,state,move", "e");
          }
     }
}

static void
rot_point(int *x, int *y, int cx, int cy, double ang)
{
   int dx = *x - cx;
   int dy = *y - cy;
   double len = sqrt((dx * dx) + (dy * dy));
   double a = atan2(dy, dx);
   *x = cx + (cos(a + to_rad(ang)) * len);
   *y = cy + (sin(a + to_rad(ang)) * len);
}

static double
angle_get(int x1, int y1, int x2, int y2, int px, int py, double ang, int nx, int ny)
{
   int cx = (x1 + x2) / 2;
   int cy = (y1 + y2) / 2;
   double ang_pt = 360.0 + to_deg(atan2(py - cy, px - cx)) + ang;
   double ang_new = 360.0 + to_deg(atan2(ny - cy, nx - cx));
   return fmod(360.0 + ang_new - ang_pt, 360.0);
}

static void
_cb_modify_mouse_move(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj, void *info)
{
   Evas_Event_Mouse_Move *ev = info;
   int mx, my;

   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return;
   if (!modify_down) return;
   evas_pointer_canvas_xy_get(evas_object_evas_get(win2), &modify_x, &modify_y);
   mx = modify_x - modify_down_x;
   my = modify_y - modify_down_y;
   if (modify_mode == MODIFY_LINE)
     {
        if (obj == o_draw_handles[0])
          line_modify_coord_set(modify_line_x1 + mx,
                                modify_line_y1 + my,
                                modify_line_x2, modify_line_y2);
        else if (obj == o_draw_handles[1])
          line_modify_coord_set(modify_line_x1, modify_line_y1,
                                modify_line_x2 + mx,
                                modify_line_y2 + my);
        draw_handle_line_update();
        line_eval();
     }
   else if (modify_mode == MODIFY_BOX)
     {
        if (modify_box_mode == MODIFY_BOX_MOVE)
          {
             box_modify_coord_set(modify_box_x1 + mx,
                                  modify_box_y1 + my,
                                  modify_box_x2 + mx,
                                  modify_box_y2 + my,
                                  modify_box_angle);
          }
        else if (modify_box_mode == MODIFY_BOX_RESIZE_TL)
          {
             if (modify_box_rotate)
               {
                  double rot_by =
                    angle_get(modify_box_x1, modify_box_y1,
                              modify_box_x2, modify_box_y2,
                              modify_box_x1, modify_box_y1,
                              modify_box_angle,
                              modify_x, modify_y);
                  box_modify_coord_set(modify_box_x1, modify_box_y1,
                                       modify_box_x2, modify_box_y2,
                                       modify_box_angle + rot_by);
               }
             else
               {
                  rot_point(&mx, &my, 0, 0, -modify_box_angle);
                  box_modify_coord_set(modify_box_x1 + mx,
                                       modify_box_y1 + my,
                                       modify_box_x2 - mx,
                                       modify_box_y2 - my,
                                       modify_box_angle);
               }
          }
        else if (modify_box_mode == MODIFY_BOX_RESIZE_TR)
          {
             if (modify_box_rotate)
               {
                  double rot_by =
                    angle_get(modify_box_x1, modify_box_y1,
                              modify_box_x2, modify_box_y2,
                              modify_box_x2, modify_box_y1,
                              modify_box_angle,
                              modify_x, modify_y);
                  box_modify_coord_set(modify_box_x1, modify_box_y1,
                                       modify_box_x2, modify_box_y2,
                                       modify_box_angle + rot_by);
               }
             else
               {
                  rot_point(&mx, &my, 0, 0, -modify_box_angle);
                  box_modify_coord_set(modify_box_x1 - mx,
                                       modify_box_y1 + my,
                                       modify_box_x2 + mx,
                                       modify_box_y2 - my,
                                       modify_box_angle);
               }
          }
        else if (modify_box_mode == MODIFY_BOX_RESIZE_BL)
          {
             if (modify_box_rotate)
               {
                  double rot_by =
                    angle_get(modify_box_x1, modify_box_y1,
                              modify_box_x2, modify_box_y2,
                              modify_box_x1, modify_box_y2,
                              modify_box_angle,
                              modify_x, modify_y);
                  box_modify_coord_set(modify_box_x1, modify_box_y1,
                                       modify_box_x2, modify_box_y2,
                                       modify_box_angle + rot_by);
               }
             else
               {
                  rot_point(&mx, &my, 0, 0, -modify_box_angle);
                  box_modify_coord_set(modify_box_x1 + mx,
                                       modify_box_y1 - my,
                                       modify_box_x2 - mx,
                                       modify_box_y2 + my,
                                       modify_box_angle);
               }
          }
        else if (modify_box_mode == MODIFY_BOX_RESIZE_BR)
          {
             if (modify_box_rotate)
               {
                  double rot_by =
                    angle_get(modify_box_x1, modify_box_y1,
                              modify_box_x2, modify_box_y2,
                              modify_box_x2, modify_box_y2,
                              modify_box_angle,
                              modify_x, modify_y);
                  box_modify_coord_set(modify_box_x1, modify_box_y1,
                                       modify_box_x2, modify_box_y2,
                                       modify_box_angle + rot_by);
               }
             else
               {
                  rot_point(&mx, &my, 0, 0, -modify_box_angle);
                  box_modify_coord_set(modify_box_x1 - mx,
                                       modify_box_y1 - my,
                                       modify_box_x2 + mx,
                                       modify_box_y2 + my,
                                       modify_box_angle);
               }
          }
        box_eval();
        draw_handle_box_update();
     }
   ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
}

static void
_cb_mod_move(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   modify_box_mode = MODIFY_BOX_MOVE;
}

static void
_cb_mod_resize_tl(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   modify_box_mode = MODIFY_BOX_RESIZE_TL;
}

static void
_cb_mod_resize_tr(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   modify_box_mode = MODIFY_BOX_RESIZE_TR;
}

static void
_cb_mod_resize_bl(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   modify_box_mode = MODIFY_BOX_RESIZE_BL;
}

static void
_cb_mod_resize_br(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   modify_box_mode = MODIFY_BOX_RESIZE_BR;
}

static void
draw_modify_clear(void)
{
   int i;

   modify_mode = MODIFY_NONE;
   modify_box_mode = MODIFY_BOX_NONE;
   modify_box_rotate = EINA_FALSE;
   for (i = 0; i < 2; i++)
     {
        if (!o_draw_handles[i]) continue;
        evas_object_del(o_draw_handles[i]);
        o_draw_handles[i] = NULL;
     }
   line_clear();
   box_clear();
}

static Evas_Object *
draw_handle_add(Evas_Object *parent, const char *group)
{
   Evas_Object *o;
   Evas_Coord minw, minh;
   char path[PATH_MAX];
   char buf[1024];

   o = elm_layout_add(parent);
   snprintf(path, sizeof(path), "%s/shotedit.edj", e_module_dir_get(shot_module));
   snprintf(buf, sizeof(buf), "e/modules/shot/%s", group);
   elm_layout_file_set(o, path, buf);
   edje_object_size_min_calc(elm_layout_edje_get(o), &minw, &minh);
   evas_object_resize(o, minw, minh);
   modify_handle_offx = -(minw / 2);
   modify_handle_offy = -(minh / 2);
   evas_object_show(o);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN,
                                  _cb_modify_mouse_down, NULL);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_UP,
                                  _cb_modify_mouse_up, NULL);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_MOVE,
                                  _cb_modify_mouse_move, NULL);
   return o;
}

static Evas_Object *
draw_boxhandle_add(Evas_Object *parent, const char *group)
{
   Evas_Object *o;
   char path[PATH_MAX];
   char buf[1024];

   o = elm_layout_add(parent);
   snprintf(path, sizeof(path), "%s/shotedit.edj", e_module_dir_get(shot_module));
   snprintf(buf, sizeof(buf), "e/modules/shot/%s", group);
   elm_layout_file_set(o, path, buf);
   evas_object_repeat_events_set(o, EINA_TRUE);
   evas_object_show(o);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN,
                                  _cb_modify_mouse_down, NULL);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_UP,
                                  _cb_modify_mouse_up, NULL);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_MOVE,
                                  _cb_modify_mouse_move, NULL);
   return o;
}

static void
draw_color_rects_update(void)
{
   evas_object_color_set(o_col1,
                         premul(color[0].r, color[0].a),
                         premul(color[0].g, color[0].a),
                         premul(color[0].b, color[0].a),
                         color[0].a);
   evas_object_color_set(o_col2,
                         premul(color[1].r, color[1].a),
                         premul(color[1].g, color[1].a),
                         premul(color[1].b, color[1].a),
                         color[1].a);
}

static void
draw_modify_begin(Evas_Object *o)
{
   if (evas_object_data_get(o, "line"))
     {
        modify_mode = MODIFY_LINE;
        int x1 = (int)(uintptr_t)evas_object_data_get(o, "x1");
        int y1 = (int)(uintptr_t)evas_object_data_get(o, "y1");
        int x2 = (int)(uintptr_t)evas_object_data_get(o, "x2");
        int y2 = (int)(uintptr_t)evas_object_data_get(o, "y2");
        int inset = (int)(uintptr_t)evas_object_data_get(o, "inset");
        modify_line_x1 = x1; modify_line_y1 = y1;
        modify_line_x2 = x2; modify_line_y2 = y2;
        line_modify_begin(o, x1, y1, x2, y2, inset);
        evas_object_raise(o);
        evas_object_stack_below(evas_object_data_get(o, "shadow"), o);
        edje_object_color_class_get(elm_layout_edje_get(o), "color",
                                    &color[0].r, &color[0].g, &color[0].b, &color[0].a,
                                    NULL, NULL, NULL, NULL,
                                    NULL, NULL, NULL, NULL);
        edje_object_color_class_get(elm_layout_edje_get(o), "color2",
                                    &color[1].r, &color[1].g, &color[1].b, &color[1].a,
                                    NULL, NULL, NULL, NULL,
                                    NULL, NULL, NULL, NULL);
        colorsel_set();
        o_draw_handles[0] = draw_handle_add(win2, "tool/line/handle");
        o_draw_handles[1] = draw_handle_add(win2, "tool/line/handle");
        draw_handle_line_update();
     }
   else if (evas_object_data_get(o, "box"))
     {
        modify_mode = MODIFY_BOX;
        modify_box_mode = MODIFY_BOX_NONE;
        modify_box_rotate = EINA_FALSE;
        int x1 = (int)(uintptr_t)evas_object_data_get(o, "x1");
        int y1 = (int)(uintptr_t)evas_object_data_get(o, "y1");
        int x2 = (int)(uintptr_t)evas_object_data_get(o, "x2");
        int y2 = (int)(uintptr_t)evas_object_data_get(o, "y2");
        double ang = (double)(uintptr_t)evas_object_data_get(o, "angle") / 1000000.0;
        modify_box_x1 = x1; modify_box_y1 = y1;
        modify_box_x2 = x2; modify_box_y2 = y2;
        box_modify_begin(o, x1, y1, x2, y2, ang);
        evas_object_raise(o);
        evas_object_stack_below(evas_object_data_get(o, "shadow"), o);
        edje_object_color_class_get(elm_layout_edje_get(o), "color",
                                    &color[0].r, &color[0].g, &color[0].b, &color[0].a,
                                    NULL, NULL, NULL, NULL,
                                    NULL, NULL, NULL, NULL);
        edje_object_color_class_get(elm_layout_edje_get(o), "color2",
                                    &color[1].r, &color[1].g, &color[1].b, &color[1].a,
                                    NULL, NULL, NULL, NULL,
                                    NULL, NULL, NULL, NULL);
        colorsel_set();
        o_draw_handles[0] = draw_boxhandle_add(win2, "tool/box/handle");
        elm_layout_signal_callback_add(o_draw_handles[0],
                                       "action,move,begin", "e",
                                       _cb_mod_move, NULL);
        elm_layout_signal_callback_add(o_draw_handles[0],
                                       "action,resize,tl,begin", "e",
                                       _cb_mod_resize_tl, NULL);
        elm_layout_signal_callback_add(o_draw_handles[0],
                                       "action,resize,tr,begin", "e",
                                       _cb_mod_resize_tr, NULL);
        elm_layout_signal_callback_add(o_draw_handles[0],
                                       "action,resize,bl,begin", "e",
                                       _cb_mod_resize_bl, NULL);
        elm_layout_signal_callback_add(o_draw_handles[0],
                                       "action,resize,br,begin", "e",
                                       _cb_mod_resize_br, NULL);
        if (evas_object_data_get(o, "entry"))
          elm_layout_signal_emit(o_draw_handles[0], "e,state,moveall,off", "e");
        draw_handle_box_update();
     }
   draw_color_rects_update();
}

static void
_cb_draw_none_mouse_down(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *info)
{
   Evas_Event_Mouse_Down *ev = info;

   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return;
   if (ev->button == 1)
     {
        Evas_Object *o = box_obj_get();

        if (o)
          {
             o = evas_object_data_get(o, "entry");
             if (o) elm_object_focus_set(o, EINA_FALSE);
          }
        draw_modify_clear();
     }
}

static void
_cb_draw_mouse_down(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj, void *info)
{
   Evas_Event_Mouse_Down *ev = info;

   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return;
   if (ev->button == 1)
     {
        if (tool_mode == TOOL_MODIFY)
          {
             Evas_Object *o;

             draw_modify_clear();
             draw_modify_begin(obj);
             o = evas_object_data_get(obj, "entry");
             if (o)
               {
                  elm_object_focus_move_policy_automatic_set(o_scroll, EINA_FALSE);
                  elm_object_focus_move_policy_automatic_set(o, EINA_FALSE);
                  elm_object_focus_allow_set(o_scroll, EINA_FALSE);
                  elm_object_focus_set(o_rend, EINA_TRUE);
                  elm_object_focus_set(o, EINA_TRUE);
               }
          }
        else if (tool_mode == TOOL_DELETE)
          {
             Evas_Object *o;

             draw_objects = eina_list_remove(draw_objects, obj);
             o = evas_object_data_get(obj, "shadow");
             if (o) evas_object_del(o);
             evas_object_del(obj);
          }
     }
}

static void
draw_selectable_set(Eina_Bool sel)
{
   Eina_List *l;
   Evas_Object *o;

   EINA_LIST_FOREACH(draw_objects, l, o)
     {
        evas_object_pass_events_set(o, !sel);
     }
}
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// line tool handling
static Eina_Bool    line_mouse_pressed = EINA_FALSE;
static const char  *line_style = NULL;
static Evas_Object *o_line = NULL;
static Evas_Object *o_line_shadow = NULL;
static int          line_x1 = 0;
static int          line_y1 = 0;
static int          line_x2 = 0;
static int          line_y2 = 0;
static int          line_point_inset = 0;
static int          line_shadow_point_inset = 0;
static int          line_shadow_off_x = 0;
static int          line_shadow_off_y = 0;

static void
line_clear(void)
{
   o_line = NULL;
}

static void
line_modify_coord_set(int x1, int y1, int x2, int y2)
{
   line_x1 = x1; line_y1 = y1;
   line_x2 = x2; line_y2 = y2;
}

static void
line_modify_coord_get(int *x1, int *y1, int *x2, int *y2)
{
   *x1 = line_x1; *y1 = line_y1;
   *x2 = line_x2; *y2 = line_y2;
}

static void
line_shadow_off_get(void)
{
   const char *s;

   s = edje_object_data_get(o_line_shadow, "offset_x");
   if (s) line_shadow_off_x = atoi(s);
   else line_shadow_off_x = 0;
   s = edje_object_data_get(o_line_shadow, "offset_y");
   if (s) line_shadow_off_y = atoi(s);
   else line_shadow_off_y = 0;
   line_shadow_off_x = ELM_SCALE_SIZE(line_shadow_off_x);
   line_shadow_off_y = ELM_SCALE_SIZE(line_shadow_off_y);
}

static void
line_modify_begin(Evas_Object *o, int x1, int y1, int x2, int y2, int inset)
{
   o_line = o;
   o_line_shadow = evas_object_data_get(o_line, "shadow");
   line_modify_coord_set(x1, y1, x2, y2);
   line_point_inset = inset;
   line_shadow_point_inset =
     (int)(uintptr_t)evas_object_data_get(o_line_shadow, "inset");
   line_shadow_off_get();
}

static void
line_map_apply(Evas_Object *o, int x1, int y1, int x2, int y2, int offx, int offy, int inset)
{
   Evas_Map *m;
   int dx = x2 - x1;
   int dy = y2 - y1;
   int x, y, w, h;
   int len = sqrt((dx * dx) + (dy * dy));
   double len2;
   // angle from horiz axis + angle == y2 > y1
   double angle = fmod(360.0 + to_deg(atan2(dy, dx)), 360.0);

   // 0                     1
   //  +--------------------+
   //  |\ x1 y1       x2,y2/|
   //  | +----------------+ |
   //  |/ <--- len ------> \<- len2
   //  +--------------------+
   // 3                     2
   len2 = sqrt((inset * inset) + (inset * inset));
   w = len + (inset * 2);
   h = inset * 2;
   evas_object_resize(o, w, h);
   m = evas_map_new(4);
   evas_map_alpha_set(m, EINA_TRUE);
   evas_map_smooth_set(m, EINA_TRUE);
   evas_map_util_points_color_set(m, 255, 255, 255, 255);
   evas_map_util_points_populate_from_object(m ,o);
   x = cos(to_rad(angle - 135.0)) * len2;
   y = sin(to_rad(angle - 135.0)) * len2;
   evas_map_point_coord_set(m, 0, x1 + x + offx, y1 + y + offy, 0);
   x = cos(to_rad(angle - 45.0)) * len2;
   y = sin(to_rad(angle - 45.0)) * len2;
   evas_map_point_coord_set(m, 1, x2 + x + offx, y2 + y + offy, 0);
   x = cos(to_rad(angle + 45.0)) * len2;
   y = sin(to_rad(angle + 45.0)) * len2;
   evas_map_point_coord_set(m, 2, x2 + x + offx, y2 + y + offy, 0);
   x = cos(to_rad(angle + 135.0)) * len2;
   y = sin(to_rad(angle + 135.0)) * len2;
   evas_map_point_coord_set(m, 3, x1 + x + offx, y1 + y + offy, 0);
   evas_object_map_set(o, m);
   evas_map_free(m);
   evas_object_map_enable_set(o, EINA_TRUE);
   evas_object_show(o);
}

static void
line_eval(void)
{
   line_map_apply(o_line, line_x1, line_y1, line_x2, line_y2,
                  0, 0, line_point_inset);
   evas_object_data_set(o_line, "x1", (void *)(uintptr_t)line_x1);
   evas_object_data_set(o_line, "y1", (void *)(uintptr_t)line_y1);
   evas_object_data_set(o_line, "x2", (void *)(uintptr_t)line_x2);
   evas_object_data_set(o_line, "y2", (void *)(uintptr_t)line_y2);
   line_map_apply(o_line_shadow, line_x1, line_y1, line_x2, line_y2,
                  line_shadow_off_x, line_shadow_off_y,
                  line_shadow_point_inset);
}

static void
line_color_set(void)
{
   if (!o_line) return;
   edje_object_color_class_set
     (elm_layout_edje_get(o_line),
      "color", color[0].r, color[0].g, color[0].b, color[0].a,
      0, 0, 0, 0, 0, 0, 0, 0);
   edje_object_color_class_set
     (elm_layout_edje_get(evas_object_data_get(o_line, "shadow")),
      "color", color[0].r, color[0].g, color[0].b, color[0].a,
      0, 0, 0, 0, 0, 0, 0, 0);
   edje_object_color_class_set
     (elm_layout_edje_get(o_line),
      "color2", color[1].r, color[1].g, color[1].b, color[1].a,
      0, 0, 0, 0, 0, 0, 0, 0);
   edje_object_color_class_set
     (elm_layout_edje_get(evas_object_data_get(o_line, "shadow")),
      "color2", color[1].r, color[1].g, color[1].b, color[1].a,
       0, 0, 0, 0, 0, 0, 0, 0);
}

static Evas_Object *
line_obj_add(Evas_Object *parent, const char *style, const char *append, int *inset)
{
   Evas_Object *o;
   Evas_Coord minw, minh;
   char path[PATH_MAX];
   char buf[1024];

   o = elm_layout_add(parent);
   snprintf(path, sizeof(path), "%s/shotedit.edj", e_module_dir_get(shot_module));
   snprintf(buf, sizeof(buf), "e/modules/shot/item/line/%s%s", style, append);
   elm_layout_file_set(o, path, buf);
   evas_object_pass_events_set(o, EINA_TRUE);
   edje_object_size_min_calc(elm_layout_edje_get(o), &minw, &minh);
   *inset = minh / 2;
   evas_object_data_set(o, "line", o);
   evas_object_data_set(o, "inset", (void *)(uintptr_t)(*inset));
   return o;
}

static void
line_down(int x, int y)
{
   line_mouse_pressed = EINA_TRUE;
   line_modify_coord_set(x, y, x, y);

   o_line = line_obj_add(win2, line_style, "", &line_point_inset);
   draw_objects = eina_list_append(draw_objects, o_line);
   evas_object_event_callback_add(o_line, EVAS_CALLBACK_MOUSE_DOWN,
                                  _cb_draw_mouse_down, NULL);

   o_line_shadow = line_obj_add(win2, line_style, "/shadow",
                                &line_shadow_point_inset);
   line_shadow_off_get();

   evas_object_stack_below(o_line_shadow, o_line);
   evas_object_data_set(o_line, "shadow", o_line_shadow);

   line_color_set();
   line_eval();
}

static void
line_up(int x EINA_UNUSED, int y EINA_UNUSED)
{
   line_mouse_pressed = EINA_FALSE;
   o_line = NULL;
}

static void
line_move(int x, int y)
{
   line_x2 = x; line_y2 = y;
   line_eval();
}
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// box tool handling
static Eina_Bool    box_mouse_pressed = EINA_FALSE;
static const char  *box_style = NULL;
static Evas_Object *o_box = NULL;
static Evas_Object *o_box_shadow = NULL;
static int          box_x1 = 0;
static int          box_y1 = 0;
static int          box_x2 = 0;
static int          box_y2 = 0;
static double       box_angle = 0.0;
static int          box_minw = 0;
static int          box_minh = 0;
static int          box_shadow_off_x = 0;
static int          box_shadow_off_y = 0;
static int          box_shadow_minw = 0;
static int          box_shadow_minh = 0;

static void
box_clear(void)
{
   o_box = NULL;
}

static void
box_modify_coord_set(int x1, int y1, int x2, int y2, double ang)
{
   double a;
   static const double stops[] = { 0, 45, 90, 135, 180, 225, 270, 315, 360 };
   static const double hyst = 2.0;
   int i;

   box_x1 = x1; box_y1 = y1;
   box_x2 = x2; box_y2 = y2;
   a = fmod(360.0 + ang, 360.0);
   for (i = 0; i < (int)(sizeof(stops) / sizeof(stops[0])); i++)
     {
        if ((a >= (stops[i] - hyst)) && (a <= (stops[i] + hyst)))
          {
             a = stops[i];
             break;
          }
     }
   box_angle = a;
}

static void
box_modify_coord_get(int *x1, int *y1, int *x2, int *y2, double *ang)
{
   *x1 = box_x1; *y1 = box_y1;
   *x2 = box_x2; *y2 = box_y2;
   *ang = box_angle;
}

static Evas_Object *
box_obj_get(void)
{
   return o_box;
}

static void
box_shadow_off_get(void)
{
   const char *s;

   s = edje_object_data_get(o_box_shadow, "offset_x");
   if (s) box_shadow_off_x = atoi(s);
   else box_shadow_off_x = 0;
   s = edje_object_data_get(o_box_shadow, "offset_y");
   if (s) box_shadow_off_y = atoi(s);
   else box_shadow_off_y = 0;
   box_shadow_off_x = ELM_SCALE_SIZE(box_shadow_off_x);
   box_shadow_off_y = ELM_SCALE_SIZE(box_shadow_off_y);
   box_shadow_minw = (int)(uintptr_t)evas_object_data_get(o_box_shadow, "minw");
   box_shadow_minh = (int)(uintptr_t)evas_object_data_get(o_box_shadow, "minh");
}

static void
box_color_set(void)
{
   Evas_Object *o;

   if (!o_box) return;
   edje_object_color_class_set
     (elm_layout_edje_get(o_box),
      "color", color[0].r, color[0].g, color[0].b, color[0].a, 0, 0, 0, 0, 0, 0, 0, 0);
   edje_object_color_class_set
     (elm_layout_edje_get(evas_object_data_get(o_box, "shadow")),
      "color", color[0].r, color[0].g, color[0].b, color[0].a, 0, 0, 0, 0, 0, 0, 0, 0);
   edje_object_color_class_set
     (elm_layout_edje_get(o_box),
      "color2", color[1].r, color[1].g, color[1].b, color[1].a, 0, 0, 0, 0, 0, 0, 0, 0);
   edje_object_color_class_set
     (elm_layout_edje_get(evas_object_data_get(o_box, "shadow")),
      "color2", color[1].r, color[1].g, color[1].b, color[1].a, 0, 0, 0, 0, 0, 0, 0, 0);
   o = evas_object_data_get(o_box, "entry");
   if (o)
     {
        const char *s;
        char buf[256];

        s = edje_object_data_get(elm_layout_edje_get(o_box), "entry_style");
        if (s)
          {
             int l = strlen(s);

             if (l > 0)
               {
                  char *tmp = malloc(l + 1 + 8 + 8);

                  if (tmp)
                    {
                       strcpy(tmp, s);
                       if (tmp[l - 1] == '\'')
                         {
                            tmp[l - 1] = 0;
                            snprintf(buf, sizeof(buf),
                                     " color=#%02x%02x%02x%02x'",
                                     color[1].r, color[1].g, color[1].b, color[1].a);
                            strcpy(tmp + l - 1, buf);
                            elm_entry_text_style_user_pop(o);
                            elm_entry_text_style_user_push(o, tmp);
                         }
                       free(tmp);
                    }
               }
          }
     }
}

static void
box_modify_begin(Evas_Object *o, int x1, int y1, int x2, int y2, double ang)
{
   o_box = o;
   o_box_shadow = evas_object_data_get(o_box, "shadow");
   box_modify_coord_set(x1, y1, x2, y2, ang);
   box_minw = (int)(uintptr_t)evas_object_data_get(o_box, "minw");
   box_minh = (int)(uintptr_t)evas_object_data_get(o_box, "minh");
   box_angle = (double)(uintptr_t)evas_object_data_get(o_box, "angle") / 1000000.0;
   box_shadow_off_get();
}

static void
box_map_apply(Evas_Object *o, int x1, int y1, int x2, int y2, int minw, int minh, int offx, int offy, double ang)
{
   Evas_Map *m;
   int x, y, w, h, cx, cy;

   if (x2 >= x1)
     {
        x = x1;
        w = x2 - x1;
     }
   else
     {
        x = x2;
        w = x1 - x2;
     }
   if (y2 >= y1)
     {
        y = y1;
        h = y2 - y1;
     }
   else
     {
        y = y2;
        h = y1 - y2;
     }
   x -= minw / 2;
   y -= minh / 2;
   w += minw;
   h += minh;
   cx = (x1 + x2 + offx) / 2;
   cy = (y1 + y2 + offy) / 2;
   evas_object_geometry_set(o, x + offx, y + offy, w, h);
   m = evas_map_new(4);
   evas_map_alpha_set(m, EINA_TRUE);
   evas_map_smooth_set(m, EINA_TRUE);
   evas_map_util_points_color_set(m, 255, 255, 255, 255);
   evas_map_util_points_populate_from_object(m ,o);
   evas_map_util_rotate(m, ang, cx, cy);
   evas_object_map_set(o, m);
   evas_map_free(m);
   evas_object_map_enable_set(o, EINA_TRUE);
   evas_object_show(o);
}

static Evas_Object *
box_obj_add(Evas_Object *parent, const char *style, const char *append, int *minw, int *minh)
{
   Evas_Object *o, *o2;
   char path[PATH_MAX];
   char buf[1024];

   o = elm_layout_add(parent);
   snprintf(path, sizeof(path), "%s/shotedit.edj", e_module_dir_get(shot_module));
   snprintf(buf, sizeof(buf), "e/modules/shot/item/box/%s%s", style, append);
   elm_layout_file_set(o, path, buf);
   if (edje_object_part_exists(elm_layout_edje_get(o), "e.swallow.entry"))
     {
        const char *s;

        o2 = elm_entry_add(parent);
        s = edje_object_data_get(elm_layout_edje_get(o), "entry_style");
        if (s) elm_entry_text_style_user_push(o2, s);
        elm_object_text_set(o2, "Sample Text");
        elm_object_part_content_set(o, "e.swallow.entry", o2);
        evas_object_data_set(o, "entry", o2);
     }
   evas_object_pass_events_set(o, EINA_TRUE);
   evas_object_data_set(o, "box", o);
   edje_object_size_min_calc(elm_layout_edje_get(o), minw, minh);
   evas_object_data_set(o, "minw", (void *)(uintptr_t)*minw);
   evas_object_data_set(o, "minh", (void *)(uintptr_t)*minh);
   return o;
}

static void
box_eval(void)
{
   box_map_apply(o_box, box_x1, box_y1, box_x2, box_y2,
                 box_minw, box_minh, 0, 0, box_angle);
   evas_object_data_set(o_box, "x1", (void *)(uintptr_t)box_x1);
   evas_object_data_set(o_box, "y1", (void *)(uintptr_t)box_y1);
   evas_object_data_set(o_box, "x2", (void *)(uintptr_t)box_x2);
   evas_object_data_set(o_box, "y2", (void *)(uintptr_t)box_y2);
   evas_object_data_set(o_box, "angle", (void *)(uintptr_t)(box_angle * 1000000.0));
   box_map_apply(o_box_shadow, box_x1, box_y1, box_x2, box_y2,
                 box_shadow_minw, box_shadow_minh,
                 box_shadow_off_x, box_shadow_off_y,
                 box_angle);
}

static void
box_down(int x, int y)
{
   box_mouse_pressed = EINA_TRUE;
   box_modify_coord_set(x, y, x, y, 0.0);
   o_box = box_obj_add(win2, box_style, "", &box_minw, &box_minh);
   draw_objects = eina_list_append(draw_objects, o_box);
   evas_object_event_callback_add(o_box, EVAS_CALLBACK_MOUSE_DOWN,
                                  _cb_draw_mouse_down, NULL);
   o_box_shadow = box_obj_add(win2, box_style, "/shadow",
                              &box_shadow_minw, &box_shadow_minh);
   box_shadow_off_get();
   evas_object_stack_below(o_box_shadow, o_box);
   evas_object_data_set(o_box, "shadow", o_box_shadow);
   box_color_set();
   box_eval();
}

static void
box_up(int x EINA_UNUSED, int y EINA_UNUSED)
{
   box_mouse_pressed = EINA_FALSE;
   o_box = NULL;
}

static void
box_move(int x, int y)
{
   box_x2 = x; box_y2 = y;
   box_eval();
}
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// crop handling
typedef enum
{
   CROP_NONE,
   CROP_SCREEN,
   CROP_WINDOW_PAD,
   CROP_WINDOW,
   CROP_AREA
} Crop_Mode;

typedef enum
{
   CROP_ADJUST_NONE,
   CROP_ADJUST_TL,
   CROP_ADJUST_TR,
   CROP_ADJUST_BL,
   CROP_ADJUST_BR
} Crop_Adjust;

typedef struct
{
   int x, y, w, h;
} Crop_Area;

static Crop_Mode    crop_mode = CROP_NONE;
static Crop_Adjust  crop_adjust = CROP_ADJUST_NONE;
static Crop_Area   *crop_screen_areas = NULL;
static Crop_Area   *crop_window_areas = NULL;
static Crop_Area    crop_area = { 0, 0, 0, 0 };
static int          crop_down_x = 0;
static int          crop_down_y = 0;
static int          crop_adjust_x = 0;
static int          crop_adjust_y = 0;
static Eina_Bool    crop_area_changed = EINA_FALSE;
static Eina_Bool    crop_mouse_pressed = EINA_FALSE;
static Evas_Object *o_crop = NULL;

static void crop_down(int x, int y);

static void
crop_resize_begin(void)
{
   Evas_Coord x, y;

   evas_pointer_canvas_xy_get(evas_object_evas_get(o_crop), &x, &y);
   elm_object_scroll_hold_push(o_scroll);
   crop_down(x, y);
}

static void
_cb_crop_resize_tl(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   Evas_Coord x, y;

   if (tool_mode != TOOL_CROP) return;
   crop_adjust = CROP_ADJUST_TL;
   evas_pointer_canvas_xy_get(evas_object_evas_get(o_crop), &x, &y);
   crop_adjust_x = crop_area.x - x;
   crop_adjust_y = crop_area.y - y;
   crop_resize_begin();
}

static void
_cb_crop_resize_tr(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   Evas_Coord x, y;

   if (tool_mode != TOOL_CROP) return;
   crop_adjust = CROP_ADJUST_TR;
   evas_pointer_canvas_xy_get(evas_object_evas_get(o_crop), &x, &y);
   crop_adjust_x = crop_area.x + crop_area.w - 1 - x;
   crop_adjust_y = crop_area.y - y;
   crop_resize_begin();
}

static void
_cb_crop_resize_bl(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   Evas_Coord x, y;

   if (tool_mode != TOOL_CROP) return;
   crop_adjust = CROP_ADJUST_BL;
   evas_pointer_canvas_xy_get(evas_object_evas_get(o_crop), &x, &y);
   crop_adjust_x = crop_area.x - x;
   crop_adjust_y = crop_area.y + crop_area.h - 1 - y;
   crop_resize_begin();
}

static void
_cb_crop_resize_br(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   Evas_Coord x, y;

   if (tool_mode != TOOL_CROP) return;
   crop_adjust = CROP_ADJUST_BR;
   evas_pointer_canvas_xy_get(evas_object_evas_get(o_crop), &x, &y);
   crop_adjust_x = crop_area.x + crop_area.w - 1 - x;
   crop_adjust_y = crop_area.y + crop_area.h - 1 - y;
   crop_resize_begin();
}

static void
crop_create(void)
{
   if (!o_crop)
     {
        Evas_Object *o;
        char path[PATH_MAX];

        o_crop = o = elm_layout_add(win2);
        snprintf(path, sizeof(path), "%s/shotedit.edj", e_module_dir_get(shot_module));
        elm_layout_file_set(o, path, "e/modules/shot/tool/crop");
        evas_object_repeat_events_set(o, EINA_TRUE);
        elm_layout_signal_callback_add(o, "action,resize,tl,begin", "e",
                                       _cb_crop_resize_tl, NULL);
        elm_layout_signal_callback_add(o, "action,resize,tr,begin", "e",
                                       _cb_crop_resize_tr, NULL);
        elm_layout_signal_callback_add(o, "action,resize,bl,begin", "e",
                                       _cb_crop_resize_bl, NULL);
        elm_layout_signal_callback_add(o, "action,resize,br,begin", "e",
                                       _cb_crop_resize_br, NULL);
        evas_object_layer_set(o, 100);
        evas_object_resize(o, img_w, img_h);
        evas_object_show(o);
     }
}

static void
crop_position(void)
{
   if (o_crop)
     {
        Evas_Object *o = elm_layout_edje_get(o_crop);
        edje_object_part_drag_value_set
          (o, "e/drag/rel1",
           crop_area.x,
           crop_area.y);
        edje_object_part_drag_value_set
          (o, "e/drag/rel2",
           crop_area.x + crop_area.w,
           crop_area.y + crop_area.h);
     }
}

static void
crop_export(void)
{
   crop.x = crop_area.x;
   crop.y = crop_area.y;
   crop.w = crop_area.w;
   crop.h = crop_area.h;
}

static void
crop_clear(void)
{
   crop_area.x = crop_area.y = crop_area.w = crop_area.h = 0;
   crop_export();
   if (o_crop)
     {
        evas_object_del(o_crop);
        o_crop = NULL;
     }
}

static void
crop_update(void)
{
   if ((crop_area.w > 5) || (crop_area.h > 5)) crop_create();
   if (o_crop)
     {
        crop_mode = CROP_AREA;
        crop_position();
     }
}

static void
crop_eval(int x1, int y1, int x2, int y2)
{
   if (x1 < x2)
     {
        crop_area.x = x1;
        crop_area.w = x2 - x1 + 1;
     }
   else
     {
        crop_area.x = x2;
        crop_area.w = x1 - x2 + 1;
     }
   if (y1 < y2)
     {
        crop_area.y = y1;
        crop_area.h = y2 - y1 + 1;
     }
   else
     {
        crop_area.y = y2;
        crop_area.h = y1 - y2 + 1;
     }
   E_RECTS_CLIP_TO_RECT(crop_area.x, crop_area.y,
                        crop_area.w, crop_area.h,
                        0, 0, img_w, img_h);
   if (crop_area.w < 1) crop_area.w = 1;
   if (crop_area.h < 1) crop_area.h = 1;
   if (crop_area.x >= img_w) crop_area.x = img_w - 1;
   if (crop_area.y >= img_h) crop_area.y = img_h - 1;
   crop_export();
   crop_update();
}

static void
crop_down(int x, int y)
{
   crop_down_x = x;
   crop_down_y = y;
   crop_area_changed = EINA_FALSE;
   crop_mouse_pressed = EINA_TRUE;
   if (crop_mode == CROP_NONE)
     {
        crop_clear();
        if (crop_adjust == CROP_ADJUST_NONE)
          crop_eval(crop_down_x, crop_down_y, x, y);
     }
}

static Eina_Bool
point_in_area(int x, int y, Crop_Area area)
{
   if ((x >= area.x) && (y >= area.y) &&
       (x < (area.x + area.w)) && (y < (area.y + area.h)))
     return EINA_TRUE;
   return EINA_FALSE;
}

static void
crop_up(int x, int y)
{
   Eina_Bool found = EINA_FALSE;
   int i;

   if (!crop_mouse_pressed) return;
   crop_mouse_pressed = EINA_FALSE;
   if (crop_mode == CROP_NONE)
     { // pick a screen to crop
        if (crop_screen_areas)
          {
             for (i = 0; crop_screen_areas[i].w; i++);
             // only have one screen - no point trying to crop the screen
             if (i < 2) goto try_win;
             for (i = 0; crop_screen_areas[i].w; i++)
               {
                  if (point_in_area(x, y, crop_screen_areas[i]))
                    {
                       crop_area = crop_screen_areas[i];
                       crop_create();
                       crop_export();
                       crop_position();
                       found = EINA_TRUE;
                       crop_mode = CROP_SCREEN;
                       break;
                    }
               }
          }
        else goto try_win;
     }
   else if (crop_mode == CROP_SCREEN)
     { // pick a window plus padding
try_win:
        if (crop_window_areas)
          {
             for (i = 0; crop_window_areas[i].w; i++)
               {
                  if (point_in_area(x, y, crop_window_areas[i]))
                    {
                       crop_area = crop_window_areas[i];
                       crop_area.x -= ELM_SCALE_SIZE(window_pad);
                       crop_area.y -= ELM_SCALE_SIZE(window_pad);
                       crop_area.w += ELM_SCALE_SIZE(window_pad) * 2;
                       crop_area.h += ELM_SCALE_SIZE(window_pad) * 2;
                       E_RECTS_CLIP_TO_RECT(crop_area.x, crop_area.y,
                                            crop_area.w, crop_area.h,
                                            0, 0, img_w, img_h);
                       crop_create();
                       crop_export();
                       crop_position();
                       found = EINA_TRUE;
                       crop_mode = CROP_WINDOW_PAD;
                       break;
                    }
               }
          }
        if (!found) crop_mode = CROP_NONE;
     }
   else if (crop_mode == CROP_WINDOW_PAD)
     { // pick a window
        if (crop_window_areas)
          {
             for (i = 0; crop_window_areas[i].w; i++)
               {
                  if (point_in_area(x, y, crop_window_areas[i]))
                    {
                       crop_area = crop_window_areas[i];
                       E_RECTS_CLIP_TO_RECT(crop_area.x, crop_area.y,
                                            crop_area.w, crop_area.h,
                                            0, 0, img_w, img_h);
                       crop_create();
                       crop_export();
                       crop_position();
                       found = EINA_TRUE;
                       crop_mode = CROP_WINDOW;
                       break;
                    }
               }
          }
        if (!found) crop_mode = CROP_NONE;
     }
   else if (crop_mode == CROP_WINDOW)
     { // got back to none
        crop_mode = CROP_NONE;
     }
   else if (crop_mode == CROP_AREA)
     {
        if (crop_area_changed) found = EINA_TRUE;
        else crop_mode = CROP_NONE;
     }
   if (!found)
     {
        crop_clear();
     }
}

static void
crop_move(int x, int y)
{
   if (!crop_mouse_pressed) return;
   if (crop_adjust == CROP_ADJUST_NONE)
     {
        crop_eval(crop_down_x, crop_down_y, x, y);
        if ((crop_area.w > 5) || (crop_area.h > 5))
          crop_area_changed = EINA_TRUE;
     }
   else
     {
        x += crop_adjust_x;
        y += crop_adjust_y;
        if (crop_adjust == CROP_ADJUST_TL)
          crop_eval(x, y, crop_area.x + crop_area.w - 1, crop_area.y + crop_area.h - 1);
        else if (crop_adjust == CROP_ADJUST_TR)
          crop_eval(crop_area.x, y, x, crop_area.y + crop_area.h - 1);
        else if (crop_adjust == CROP_ADJUST_BL)
          crop_eval(x, crop_area.y, crop_area.x + crop_area.w - 1, y);
        else if (crop_adjust == CROP_ADJUST_BR)
          crop_eval(crop_area.x, crop_area.y, x, y);
        crop_area_changed = EINA_TRUE;
     }
}
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// zoom handling
#define ZOOM_COUNT 16
#define ZOOM_NONE 8
#define ZOOM_DEFAULT 4
static int zoom = ZOOM_DEFAULT;
static int zooms[] = { 125, 143, 167, 200, 250, 333, 500, 750,
                       1000,
                       2000, 3000, 4000, 5000, 6000, 7000, 8000 };

static void
zoom_set(int slot)
{
   zoom = slot;
   if (zoom < 0) zoom = 0;
   else if (zoom >= ZOOM_COUNT) zoom = ZOOM_COUNT - 1;
   edit_w = (img_w * zooms[zoom]) / 1000;
   edit_h = (img_h * zooms[zoom]) / 1000;
   if (zooms[zoom] >= 1000) evas_object_image_smooth_scale_set(o_rend, EINA_FALSE);
   else evas_object_image_smooth_scale_set(o_rend, EINA_TRUE);
   evas_object_size_hint_min_set(o_rend, edit_w, edit_h);
   evas_object_resize(o_rend, edit_w, edit_h);
}

static void
_cb_tool_zoom_plus(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *info EINA_UNUSED)
{
   elm_scroller_gravity_set(o_scroll, 0.5, 0.5);
   zoom_set(zoom + 1);
}

static void
_cb_tool_zoom_reset(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *info EINA_UNUSED)
{
   elm_scroller_gravity_set(o_scroll, 0.5, 0.5);
   zoom_set(ZOOM_NONE);
}

static void
_cb_tool_zoom_minus(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *info EINA_UNUSED)
{
   elm_scroller_gravity_set(o_scroll, 0.5, 0.5);
   zoom_set(zoom - 1);
}

static void
_cb_edit_wheel(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *info)
{
   Evas_Event_Mouse_Wheel *ev = info;

   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return;
   if ((evas_key_modifier_is_set(ev->modifiers, "Control")) &&
       (ev->direction == 0))
     {
        Evas_Coord x, y, w, h, px, py;
        double gx, gy;

        evas_pointer_canvas_xy_get(evas_object_evas_get(win), &px, &py);
        evas_object_geometry_get(o_rend, &x, &y, &w, &h);
        if (px < x) px = x;
        if (py < y) py = y;
        if (px >= (x + w)) px = x + w - 1;
        if (py >= (y + h)) py = y + h - 1;
        if ((w > 0) && (h > 0))
          {
             gx = (double)(px - x) / (double)w;
             gy = (double)(py - y) / (double)h;
             elm_scroller_gravity_set(o_scroll, gx, gy);
          }
        zoom_set(zoom - ev->z);
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
}

static void
_cb_edit_mouse_down(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *info)
{
   Evas_Event_Mouse_Down *ev = info;

   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return;
   if (ev->button == 1)
     {
        if (tool_mode == TOOL_CROP)
          {
             crop_adjust = CROP_ADJUST_NONE;
             elm_object_scroll_hold_push(o_scroll);
             crop_down(ev->canvas.x, ev->canvas.y);
             ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
          }
        else if (tool_mode == TOOL_BOX)
          {
             elm_object_scroll_hold_push(o_scroll);
             box_down(ev->canvas.x, ev->canvas.y);
             ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
          }
        else if (tool_mode == TOOL_LINE)
          {
             elm_object_scroll_hold_push(o_scroll);
             line_down(ev->canvas.x, ev->canvas.y);
             ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
          }
     }
}

static void
_cb_edit_mouse_up(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *info)
{
   Evas_Event_Mouse_Up *ev = info;

   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return;
   if (ev->button == 1)
     {
        if (tool_mode == TOOL_CROP)
          {
             elm_object_scroll_hold_pop(o_scroll);
             crop_up(ev->canvas.x, ev->canvas.y);
             ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
          }
        else if (tool_mode == TOOL_BOX)
          {
             elm_object_scroll_hold_pop(o_scroll);
             box_up(ev->canvas.x, ev->canvas.y);
             ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
          }
        else if (tool_mode == TOOL_LINE)
          {
             elm_object_scroll_hold_pop(o_scroll);
             line_up(ev->canvas.x, ev->canvas.y);
             ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
          }
     }
}

static void
_cb_edit_mouse_move(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *info)
{
   Evas_Event_Mouse_Move *ev = info;

   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return;
   if (tool_mode == TOOL_CROP)
     {
        if (crop_mouse_pressed)
          {
             crop_move(ev->cur.canvas.x, ev->cur.canvas.y);
             ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
          }
     }
   else if (tool_mode == TOOL_BOX)
     {
        if (box_mouse_pressed)
          {
             box_move(ev->cur.canvas.x, ev->cur.canvas.y);
             ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
          }
     }
   else if (tool_mode == TOOL_LINE)
     {
        if (line_mouse_pressed)
          {
             line_move(ev->cur.canvas.x, ev->cur.canvas.y);
             ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
          }
     }
}
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// tool type
typedef struct
{
   Tool_Mode mode;
   const char *style;
} Tool_Info;

static int       _tool_info_count = 0;
static Tool_Info tool_info[100] =  { { 0 } };

static void
_cb_tool_changed(void *data EINA_UNUSED, Evas_Object *obj, void *info EINA_UNUSED)
{
   int val = elm_radio_value_get(obj);

   o_box = o_line = NULL;
   if (tool_info[val].mode != TOOL_CROP)
     draw_modify_clear();
   if ((tool_info[val].mode == TOOL_MODIFY) ||
       (tool_info[val].mode == TOOL_DELETE))
     draw_selectable_set(EINA_TRUE);
   else
     draw_selectable_set(EINA_FALSE);
   tool_mode = tool_info[val].mode;
   line_style = box_style = tool_info[val].style;
}

static void
_cb_color_change(void *data EINA_UNUSED, Evas_Object *obj, void *info EINA_UNUSED)
{
   Evas_Object *o;

   if (color_sel == 0) o = o_col1;
   else o = o_col2;
   elm_colorselector_color_get(obj,
                               &color[color_sel].r, &color[color_sel].g,
                               &color[color_sel].b, &color[color_sel].a);
   evas_object_color_set(o,
                         premul(color[color_sel].r, color[color_sel].a),
                         premul(color[color_sel].g, color[color_sel].a),
                         premul(color[color_sel].b, color[color_sel].a),
                         color[color_sel].a);
   line_color_set();
   box_color_set();
}
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// xxx
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// colorsel stuff
static Evas_Object *o_colorsel = NULL;
static void
colorsel_set(void)
{
   elm_colorselector_color_set(o_colorsel,
                               color[color_sel].r, color[color_sel].g,
                               color[color_sel].b, color[color_sel].a);
}
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// ui setup
static void
_cb_win_del(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *info EINA_UNUSED)
{
   line_mouse_pressed = EINA_FALSE;
   box_mouse_pressed = EINA_FALSE;
   draw_objects = eina_list_free(draw_objects);
   o_draw_handles[0] = NULL;
   o_draw_handles[1] = NULL;
   o_box = NULL;
   o_line = NULL;
   color_sel = 0;
   crop_mode = CROP_NONE;
   crop_adjust = CROP_ADJUST_NONE;
   free(crop_screen_areas);
   crop_screen_areas = NULL;
   free(crop_window_areas);
   crop_window_areas = NULL;
   crop_area.x = crop_area.y = crop_area.w = crop_area.h = 0;
   crop_area_changed = EINA_FALSE;
   crop_mouse_pressed = EINA_FALSE;
   zoom = ZOOM_DEFAULT;
   modify_down = EINA_FALSE;
   win = NULL;
}

static void
_focfix(void *data)
{
   Evas_Object *obj = data;

   elm_object_focus_set(o_rend, EINA_TRUE);
   elm_object_focus_set(obj, EINA_TRUE);
   evas_object_unref(obj);
}

static void
_cb_col_changed(void *data EINA_UNUSED, Evas_Object *obj, void *info EINA_UNUSED)
{
   color_sel = elm_radio_value_get(obj);
   elm_colorselector_color_set(o_colorsel,
                               color[color_sel].r, color[color_sel].g,
                               color[color_sel].b, color[color_sel].a);
}

static void
_cb_win_focus(void *data EINA_UNUSED, Evas_Object *obj, void *info EINA_UNUSED)
{
   if ((tool_mode == TOOL_MODIFY) &&
       (o_box) && (evas_object_data_get(o_box, "entry")))
     {
        // XXXX: this is a hack - hook to window focus....
        evas_object_ref(obj);
        ecore_job_add(_focfix, obj);
     }
}

static Evas_Object *
ui_tool_add(Evas_Object *parent, Evas_Object *tb, Evas_Object *radg, int x, int y, const char *icon, const char *tooltip, int size, const char *style, Tool_Mode mode)
{
   Evas_Object *o, *rad;
   int val = _tool_info_count;
   char path[PATH_MAX];
   char buf[256];

   tool_info[val].mode = mode;
   tool_info[val].style = style;
   _tool_info_count++;

   o = evas_object_rectangle_add(evas_object_evas_get(parent));
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_color_set(o, 0, 0, 0, 0);
   evas_object_size_hint_min_set(o, ELM_SCALE_SIZE(size), ELM_SCALE_SIZE(size));
   elm_table_pack(tb, o, x, y, 1, 1);

   rad = o = elm_radio_add(parent);
   elm_object_tooltip_text_set(o, tooltip);
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_radio_state_value_set(o, val);
   elm_object_style_set(o, "icon");
   if (radg) elm_radio_group_add(o, radg);
   evas_object_smart_callback_add(o, "changed", _cb_tool_changed, NULL);
   elm_table_pack(tb, o, x, y, 1, 1);
   evas_object_show(o);

   o = elm_icon_add(parent);
   snprintf(path, sizeof(path), "%s/shotedit.edj", e_module_dir_get(shot_module));
   snprintf(buf, sizeof(buf), "e/modules/shot/tool/icon/%s", icon);
   elm_layout_file_set(o, path, buf);
   elm_object_content_set(rad, o);
   evas_object_show(o);
   return rad;
}
/* not used right now
static Evas_Object *
ui_icon_button_add(Evas_Object *parent, const char *icon)
{
   Evas_Object *o, *ic;
   char path[PATH_MAX];
   char buf[256];

   ic = o = elm_icon_add(parent);
   snprintf(path, sizeof(path), "%s/shotedit.edj", e_module_dir_get(shot_module));
   snprintf(buf, sizeof(buf), "e/modules/shot/icon/%s", icon);
   elm_layout_file_set(o, path, buf);

   o = elm_button_add(parent);
   elm_object_content_set(o, ic);
   evas_object_show(ic);
   evas_object_show(o);
   return o;
}
*/
static Evas_Object *
ui_icon_button_standard_add(Evas_Object *parent, const char *icon)
{
   Evas_Object *o, *ic;

   ic = o = elm_icon_add(parent);
   elm_icon_standard_set(o, icon);

   o = elm_button_add(parent);
   elm_object_content_set(o, ic);
   evas_object_show(ic);
   evas_object_show(o);
   return o;
}

Evas_Object *
ui_edit(Evas_Object *window, Evas_Object *o_bg, E_Zone *zone,
        E_Border *bd, void *dst, int sx, int sy, int sw, int sh,
        Evas_Object **o_img_ret)
{
   Evas *evas, *evas2;
   Evas_Object *o, *tb, *tb2, *sc, *fr, *bx, *radg, *rad, *bx_ret, *sc2;
   int w, h;
   
   //~ E_Container *con = NULL;
   E_Container *con = e_container_current_get(e_manager_current_get());
   //~ con = zone->container;

   o = win = window;
   evas_object_smart_callback_add(o, "focused", _cb_win_focus, NULL);

   evas = evas_object_evas_get(win);
   evas_object_event_callback_add(win, EVAS_CALLBACK_DEL, _cb_win_del, NULL);

   if ((!zone) && (bd)) zone = bd->zone;
   if (zone)
     {
        int i;

        for (i = ZOOM_NONE; i >= 0; i--)
          {
             w = (sw * zooms[i]) / 1000;
             if (w <= (zone->w / 2))
               {
                  zoom = i;
                  break;
               }
          }
     }
   w = (sw * zooms[zoom]) / 1000;
   if ((zone) && (w > (zone->w / 8))) w = zone->w / 8;
   if (w < ELM_SCALE_SIZE(360)) w = ELM_SCALE_SIZE(360);
   h = (w * sh) / sw;
   if ((zone) && (h > (zone->h / 8))) h = zone->h / 8;
   if (h < ELM_SCALE_SIZE(280)) h = ELM_SCALE_SIZE(280);

   if (!bd)
     {
        const Eina_List *l;
        E_Zone *z;
        int i, count;
        //~ count = eina_list_count(e_comp->zones);
        count = eina_list_count(con->zones);
        if (count > 0)
          {
             crop_screen_areas = calloc(count + 1, sizeof(Crop_Area));
             if (crop_screen_areas)
               {
                  i = 0;
                  EINA_LIST_FOREACH(con->zones, l, z)
                    {
                       crop_screen_areas[i].x = z->x;
                       crop_screen_areas[i].y = z->y;
                       crop_screen_areas[i].w = z->w;
                       crop_screen_areas[i].h = z->h;
                       i++;
                    }
               }
          }
        //~ count = eina_list_count(e_comp->clients);
        count = eina_list_count(e_border_client_list());
        
        if (count > 0)
          {
             crop_window_areas = calloc(count + 1, sizeof(Crop_Area));
             if (crop_window_areas)
               {
                  i = 0;
                  // top to bottom...
                  EINA_LIST_FOREACH(e_border_client_list(), l, bd)
                    {
                       if ((bd->iconic) || (bd->hidden)) continue;
                       crop_window_areas[i].x = bd->x;
                       crop_window_areas[i].y = bd->y;
                       crop_window_areas[i].w = bd->w;
                       crop_window_areas[i].h = bd->h;
                       i++;
                    }
               }
          }
     }
   else
     {
        crop_window_areas = calloc(2, sizeof(Crop_Area));
        crop_window_areas[0].x = bd->x - sx;
        crop_window_areas[0].y = bd->y - sy;
        crop_window_areas[0].w = bd->w;
        crop_window_areas[0].h = bd->h;
     }

   o_crop = NULL;
   crop_mouse_pressed = EINA_FALSE;
   crop_area.x = 0;
   crop_area.y = 0;
   crop_area.w = 0;
   crop_area.h = 0;

   tb = o = elm_table_add(win);
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_part_content_set(o_bg, "e.swallow.content", o);
   evas_object_show(o);

   o = evas_object_rectangle_add(evas);
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_min_set(o, w, h);
   evas_object_pass_events_set(o, EINA_TRUE);
   elm_table_pack(tb, o, 0, 0, 5, 5);

   sc = o = elm_scroller_add(win);
   evas_object_data_set(win, "scroll/main", o);
   elm_object_style_set(o, "noclip");
   elm_scroller_gravity_set(o, 0.5, 0.5);
   elm_object_focus_next_object_set(o, o, ELM_FOCUS_PREVIOUS);
   elm_object_focus_next_object_set(o, o, ELM_FOCUS_NEXT);
   elm_object_focus_next_object_set(o, o, ELM_FOCUS_UP);
   elm_object_focus_next_object_set(o, o, ELM_FOCUS_DOWN);
   elm_object_focus_next_object_set(o, o, ELM_FOCUS_RIGHT);
   elm_object_focus_next_object_set(o, o, ELM_FOCUS_LEFT);
   o_scroll = sc;
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_table_pack(tb, o, 0, 0, 5, 5);
   evas_object_show(o);

   sc2 = o = elm_scroller_add(win);
   evas_object_data_set(win, "scroll/tools", o);
   elm_object_style_set(o, "noclip");
   elm_scroller_content_min_limit(o, EINA_TRUE, EINA_FALSE);
   elm_scroller_gravity_set(o, 0.0, 0.0);
   evas_object_size_hint_weight_set(o, 0.0, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_table_pack(tb, o, 5, 0, 1, 5);
   evas_object_show(o);

   bx_ret = bx = o = elm_box_add(win);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(o, 0.0, EVAS_HINT_EXPAND);
   elm_object_content_set(sc2, o);
   evas_object_show(o);

   fr = o = elm_frame_add(win);
   elm_frame_autocollapse_set(o, EINA_TRUE);
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, 0.0);
   elm_object_text_set(o, _("Tools"));
   elm_box_pack_end(bx, o);
   evas_object_show(o);

   tb2 = o = elm_table_add(win);
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, 0.0);
   elm_object_content_set(fr, o);
   evas_object_show(o);

   _tool_info_count = 0;
   radg
     = ui_tool_add(win, tb2, NULL, 0, 0, "crop",               _("Select crop area"),      40, NULL,             TOOL_CROP);
   o = ui_tool_add(win, tb2, radg, 1, 0, "modify",             _("Modify objects"),        40, NULL,             TOOL_MODIFY);
   o = ui_tool_add(win, tb2, radg, 2, 0, "delete",             _("Delete objects"),        40, NULL,             TOOL_DELETE);

   o = elm_separator_add(win);
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, 0.0);
   elm_separator_horizontal_set(o, EINA_TRUE);
   elm_table_pack(tb2, o, 0, 1, 4, 1);
   evas_object_show(o);

   o = ui_tool_add(win, tb2, radg, 0, 2, "line-arrow",         _("Single arrow line"),     40, "arrow",          TOOL_LINE);
   o = ui_tool_add(win, tb2, radg, 1, 2, "line-arrow2",        _("Double arrow line"),     40, "arrow2",         TOOL_LINE);
   o = ui_tool_add(win, tb2, radg, 2, 2, "line-arrow0",        _("Plain line"),            40, "arrow0",         TOOL_LINE);
   o = ui_tool_add(win, tb2, radg, 3, 2, "box-solid",          _("Solid box"),             40, "solid",          TOOL_BOX);

   o = ui_tool_add(win, tb2, radg, 0, 3, "box-malloc",         _("Malloc"),                40, "malloc",         TOOL_BOX);
   o = ui_tool_add(win, tb2, radg, 1, 3, "box-demalloc",       _("Malloc (evil)"),         40, "demalloc",       TOOL_BOX);
   o = ui_tool_add(win, tb2, radg, 2, 3, "box-finger",         _("Pointing finger"),       40, "finger",         TOOL_BOX);
   o = ui_tool_add(win, tb2, radg, 3, 3, "box-logo",           _("Enlightenment logo"),    40, "logo",           TOOL_BOX);

   o = ui_tool_add(win, tb2, radg, 0, 4, "box-foot",           _("Foot"),                  40, "foot",           TOOL_BOX);
   o = ui_tool_add(win, tb2, radg, 1, 4, "box-walk",           _("Silly walk"),            40, "walk",           TOOL_BOX);
   o = ui_tool_add(win, tb2, radg, 2, 4, "box-outline-box",    _("Box outline"),           40, "outline-box",    TOOL_BOX);
   o = ui_tool_add(win, tb2, radg, 3, 4, "box-outline-circle", _("Circle outline"),        40, "outline-circle", TOOL_BOX);

   o = ui_tool_add(win, tb2, radg, 0, 5, "box-text-empty",     _("Plain text"),            40, "text/empty",     TOOL_BOX);
   o = ui_tool_add(win, tb2, radg, 1, 5, "box-text-plain",     _("Text box"),              40, "text/plain",     TOOL_BOX);
   o = ui_tool_add(win, tb2, radg, 2, 5, "box-text-cloud",     _("Text thought bubble"),   40, "text/cloud",     TOOL_BOX);
   o = ui_tool_add(win, tb2, radg, 3, 5, "box-text-cloud2",    _("Text thought bubble 2"), 40, "text/cloud2",    TOOL_BOX);

   o = ui_tool_add(win, tb2, radg, 0, 6, "box-text-speech",    _("Speech bubble"),         40, "text/speech",    TOOL_BOX);
   o = ui_tool_add(win, tb2, radg, 1, 6, "box-text-speech2",   _("Speech bubble 2"),       40, "text/speech2",   TOOL_BOX);
   o = ui_tool_add(win, tb2, radg, 2, 6, "box-text-kaboom",    _("Kaboom splat"),          40, "text/kaboom",    TOOL_BOX);
   o = ui_tool_add(win, tb2, radg, 3, 6, "box-text-kapow",     _("Pow explode"),           40, "text/kapow",     TOOL_BOX);

   _cb_tool_changed(NULL, radg, NULL);

   fr = o = elm_frame_add(win);
   elm_frame_autocollapse_set(o, EINA_TRUE);
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, 0.0);
   elm_object_text_set(o, _("Color"));
   elm_box_pack_end(bx, o);
   evas_object_show(o);

   tb2 = o = elm_table_add(win);
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, 0.0);
   elm_object_content_set(fr, tb2);
   evas_object_show(o);

   bx = o = elm_box_add(win);
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_box_horizontal_set(o, EINA_TRUE);
   elm_table_pack(tb2, o, 0, 0, 1, 1);
   evas_object_show(o);

   rad = o = elm_radio_add(win);
   radg = rad;
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_radio_state_value_set(o, 0);
   elm_object_style_set(o, "icon");
   evas_object_smart_callback_add(o, "changed", _cb_col_changed, NULL);
   o_col1 = o = evas_object_rectangle_add(evas);
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_min_set(o, ELM_SCALE_SIZE(24), ELM_SCALE_SIZE(24));
   elm_object_content_set(rad, o);
   elm_box_pack_end(bx, rad);
   evas_object_show(rad);

   rad = o = elm_radio_add(win);
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_radio_state_value_set(o, 1);
   elm_object_style_set(o, "icon");
   evas_object_smart_callback_add(o, "changed", _cb_col_changed, NULL);
   elm_radio_group_add(o, radg);
   o_col2 = o = evas_object_rectangle_add(evas);
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_min_set(o, ELM_SCALE_SIZE(24), ELM_SCALE_SIZE(24));
   elm_object_content_set(rad, o);
   elm_box_pack_end(bx, rad);
   evas_object_show(rad);

   draw_color_rects_update();

   o = o_colorsel = elm_colorselector_add(win);
   evas_object_resize(o, ELM_SCALE_SIZE(160), ELM_SCALE_SIZE(160));
   elm_colorselector_palette_clear(o);
   elm_colorselector_palette_color_add(o, 255, 255, 255, 255);
   elm_colorselector_palette_color_add(o, 224, 224, 224, 255);
   elm_colorselector_palette_color_add(o, 192, 192, 192, 255);
   elm_colorselector_palette_color_add(o, 160, 160, 160, 255);
   elm_colorselector_palette_color_add(o, 128, 128, 128, 255);
   elm_colorselector_palette_color_add(o,  96,  96,  96, 255);
   elm_colorselector_palette_color_add(o,  64,  64,  64, 255);
   elm_colorselector_palette_color_add(o,  32,  32,  32, 255);
   elm_colorselector_palette_color_add(o,   0,   0,   0, 255);
   elm_colorselector_palette_color_add(o,  51, 153, 255, 255);
   elm_colorselector_palette_color_add(o, 255, 153,  51, 255);
   elm_colorselector_palette_color_add(o, 255,  51, 153, 255);
   elm_colorselector_palette_color_add(o, 255,   0,   0, 255);
   elm_colorselector_palette_color_add(o, 255, 128,   0, 255);
   elm_colorselector_palette_color_add(o, 255, 255,   0, 255);
   elm_colorselector_palette_color_add(o, 128, 255,   0, 255);
   elm_colorselector_palette_color_add(o,   0, 255,   0, 255);
   elm_colorselector_palette_color_add(o,   0, 255, 128, 255);
   elm_colorselector_palette_color_add(o,   0, 255, 255, 255);
   elm_colorselector_palette_color_add(o,   0, 128, 255, 255);
   elm_colorselector_palette_color_add(o,   0,   0, 255, 255);
   elm_colorselector_palette_color_add(o, 128,   0, 255, 255);
   elm_colorselector_palette_color_add(o, 255,   0, 255, 255);
   elm_colorselector_palette_color_add(o, 255,   0, 128, 255);
   elm_colorselector_color_set(o, 255, 255, 255, 255);
   elm_table_pack(tb2, o, 0, 1, 1, 1);
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, 0.0);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, 0.0);
   evas_object_show(o);
   evas_object_smart_callback_add(o, "changed", _cb_color_change, NULL);

   o = elm_frame_add(win);
   elm_object_style_set(o, "pad_large");
   elm_table_pack(tb, o, 4, 0, 1, 1);

   o = ui_icon_button_standard_add(win, "zoom-in");
   elm_object_style_set(o, "overlay");
   elm_table_pack(tb, o, 3, 1, 1, 1);
   evas_object_smart_callback_add(o, "clicked", _cb_tool_zoom_plus, NULL);

   o = ui_icon_button_standard_add(win, "zoom-original");
   elm_object_style_set(o, "overlay");
   elm_table_pack(tb, o, 3, 2, 1, 1);
   evas_object_smart_callback_add(o, "clicked", _cb_tool_zoom_reset, NULL);

   o = ui_icon_button_standard_add(win, "zoom-out");
   elm_object_style_set(o, "overlay");
   elm_table_pack(tb, o, 3, 3, 1, 1);
   evas_object_smart_callback_add(o, "clicked", _cb_tool_zoom_minus, NULL);
   evas_object_show(o);

   img_w = edit_w = sw;
   img_h = edit_h = sh;
   
   win2 = o = elm_win_add(win, "inlined", ELM_WIN_INLINED_IMAGE);
   evas_object_resize(win2, edit_w, edit_h);
   //~ evas_object_show(o);
   evas2 = evas_object_evas_get(win2);
   evas_font_path_append(evas2, e_module_dir_get(shot_module));

   tb2 = o = elm_table_add(win);
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(o, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_content_set(sc, o);
   evas_object_show(o);

   o_rend = o = elm_win_inlined_image_object_get(win2);
   *o_img_ret = o;
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(o, 0.5, 0.5);
   evas_object_size_hint_min_set(o, edit_w, edit_h);
   elm_table_pack(tb2, o, 0, 0, 1, 1);
   evas_object_show(o);

   o_img = o = evas_object_image_filled_add(evas2);
   evas_object_image_smooth_scale_set(o_img, EINA_FALSE);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN,
                                  _cb_draw_none_mouse_down, NULL);
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_image_colorspace_set(o, EVAS_COLORSPACE_ARGB8888);
   evas_object_image_alpha_set(o, EINA_FALSE);
   evas_object_image_size_set(o, sw, sh);
   evas_object_image_data_copy_set(o, dst);
   evas_object_image_data_update_add(o, 0, 0, sw, sh);
   evas_object_size_hint_min_set(o, img_w, img_h);
   evas_object_size_hint_max_set(o, img_w, img_h);
   elm_win_resize_object_add(win2, o);
   free(dst);
   evas_object_show(o);

   zoom_set(zoom);

   o_events = o = evas_object_rectangle_add(evas2);
   evas_object_size_hint_weight_set(o, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_color_set(o, 0, 0, 0, 0);
   evas_object_repeat_events_set(o, EINA_TRUE);
   elm_win_resize_object_add(win2, o);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_WHEEL, _cb_edit_wheel, NULL);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN, _cb_edit_mouse_down, NULL);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_UP, _cb_edit_mouse_up, NULL);
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_MOVE, _cb_edit_mouse_move, NULL);
   evas_object_show(o);

   return bx_ret;
}

void
ui_edit_prepare(void)
{
   if (o_draw_handles[0]) evas_object_hide(o_draw_handles[0]);
   if (o_draw_handles[1]) evas_object_hide(o_draw_handles[1]);
   evas_object_hide(o_crop);
   elm_win_norender_push(win2);
   elm_win_render(win2);
}

void
ui_edit_crop_screen_set(int x, int y, int w, int h)
{
   crop_area.x = x;
   crop_area.y = y;
   crop_area.w = w;
   crop_area.h = h;
   E_RECTS_CLIP_TO_RECT(crop_area.x, crop_area.y,
                        crop_area.w, crop_area.h,
                        0, 0, img_w, img_h);
   crop_create();
   crop_export();
   crop_position();
   crop_mode = CROP_SCREEN;
}

//
//////////////////////////////////////////////////////////////////////////////
