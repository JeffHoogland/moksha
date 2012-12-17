#include "e.h"
#include "e_mod_main.h"
#include "e_smart_monitor.h"

/* local structure */
typedef struct _E_Smart_Data E_Smart_Data;
struct _E_Smart_Data
{
   /* object geometry */
   Evas_Coord x, y, w, h;

   /* base object */
   Evas_Object *o_base;

   /* frame object */
   Evas_Object *o_frame;

   /* stand object */
   Evas_Object *o_stand;

   /* thumbnail object */
   Evas_Object *o_thumb;

   /* changed flag */
   Eina_Bool changed : 1;

   /* visible flag */
   Eina_Bool visible : 1;

   /* enabled flag */
   Eina_Bool enabled : 1;

   /* moving flag */
   Eina_Bool moving : 1;

   /* resizing flag */
   Eina_Bool resizing : 1;

   /* rotating flag */
   Eina_Bool rotating : 1;

   /* handler for bg updates */
   Ecore_Event_Handler *bg_update_hdl;

   /* list of randr 'modes' */
   Eina_List *modes;

   /* min & max resolutions */
   struct 
     {
        int w, h;
     } min, max;

   /* reference to the RandR Output Info */
   E_Randr_Output_Info *output;

   /* reference to the Layout widget */
   struct 
     {
        Evas_Object *obj; /* the actual layout widget */
        Evas_Coord x, y; /* the layout widget's position */
        Evas_Coord vw, vh; /* the layout widget's virtual size */
     } layout;

   /* Evas_Object *o_layout; */

   /* reference to the Container number */
   int con_num;

   /* reference to the Zone number */
   int zone_num;
};

/* local function prototypes */
static void _e_smart_add(Evas_Object *obj);
static void _e_smart_del(Evas_Object *obj);
static void _e_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _e_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static void _e_smart_show(Evas_Object *obj);
static void _e_smart_hide(Evas_Object *obj);
static void _e_smart_clip_set(Evas_Object *obj, Evas_Object *clip);
static void _e_smart_clip_unset(Evas_Object *obj);

static void _e_smart_monitor_modes_fill(E_Smart_Data *sd);
static int _e_smart_monitor_modes_sort(const void *data1, const void *data2);
static void _e_smart_monitor_background_set(E_Smart_Data *sd, Evas_Coord dx, Evas_Coord dy);
static Eina_Bool _e_smart_monitor_background_update(void *data, int type, void *event);

static void _e_smart_monitor_move_event(E_Smart_Data *sd, Evas_Object *mon, void *event);

/* local callback prototypes */
static void _e_smart_monitor_frame_cb_mouse_move(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event);
static void _e_smart_monitor_frame_cb_resize_in(void *data __UNUSED__, Evas_Object *obj, const char *emission __UNUSED__, const char *source __UNUSED__);
static void _e_smart_monitor_frame_cb_resize_out(void *data __UNUSED__, Evas_Object *obj, const char *emission __UNUSED__, const char *source __UNUSED__);
static void _e_smart_monitor_frame_cb_rotate_in(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__);
static void _e_smart_monitor_frame_cb_rotate_out(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__);
static void _e_smart_monitor_frame_cb_indicator_in(void *data __UNUSED__, Evas_Object *obj, const char *emission __UNUSED__, const char *source __UNUSED__);
static void _e_smart_monitor_frame_cb_indicator_out(void *data __UNUSED__, Evas_Object *obj, const char *emission __UNUSED__, const char *source __UNUSED__);
static void _e_smart_monitor_frame_cb_indicator_toggle(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__);

static void _e_smart_monitor_thumb_cb_mouse_in(void *data __UNUSED__, Evas *evas __UNUSED__, Evas_Object *obj, void *event __UNUSED__);
static void _e_smart_monitor_thumb_cb_mouse_out(void *data __UNUSED__, Evas *evas __UNUSED__, Evas_Object *obj, void *event __UNUSED__);
static void _e_smart_monitor_thumb_cb_mouse_down(void *data, Evas *evas __UNUSED__, Evas_Object *obj, void *event);
static void _e_smart_monitor_thumb_cb_mouse_up(void *data, Evas *evas __UNUSED__, Evas_Object *obj, void *event);

static void _e_smart_monitor_layout_cb_move(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event __UNUSED__);

/* external functions exposed by this widget */
Evas_Object *
e_smart_monitor_add(Evas *evas)
{
   static Evas_Smart *smart = NULL;
   static const Evas_Smart_Class sc = 
     {
        "smart_monitor", EVAS_SMART_CLASS_VERSION, 
        _e_smart_add, _e_smart_del, _e_smart_move, _e_smart_resize, 
        _e_smart_show, _e_smart_hide, NULL, 
        _e_smart_clip_set, _e_smart_clip_unset, 
        NULL, NULL, NULL, NULL, NULL, NULL, NULL
     };

   /* if we have never created the smart class, do it now */
   if (!smart)
     if (!(smart = evas_smart_class_new(&sc)))
       return NULL;

   /* return a newly created smart randr widget */
   return evas_object_smart_add(evas, smart);
}

void 
e_smart_monitor_output_set(Evas_Object *obj, E_Randr_Output_Info *output)
{
   E_Smart_Data *sd;
   Evas_Coord mw = 0, mh = 0;
   Evas_Coord cx = 0, cy = 0;
   Evas_Coord fw = 0, fh = 0;
   E_Container *con;
   E_Zone *zone;
   E_Desk *desk;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* set this monitor's output reference */
   sd->output = output;

   /* fill the list of 'modes' for this monitor
    * 
    * NB: This clears old modes and also sets the min & max resolutions */
   _e_smart_monitor_modes_fill(sd);

   if (sd->layout.obj)
     {
        /* with the min & max resolutions, we can now set the thumbnail size.
         * get largest resolution and convert to largest canvas size */
        e_layout_coord_virtual_to_canvas(sd->layout.obj, 
                                         sd->max.w, sd->max.h, &mw, &mh);
     }

   /* set thumbnail size based on largest canvas size */
   mh = (mw * mh) / mw;
   if (sd->o_thumb) 
     e_livethumb_vsize_set(sd->o_thumb, mw, mh);

   /* if we have a crtc, get the x/y location of it
    * 
    * NB: Used to determine the proper container */
   if ((sd->output) && (sd->output->crtc))
     {
        cx = sd->output->crtc->geometry.x;
        cy = sd->output->crtc->geometry.y;
     }
   else
     {
        /* FIXME: NB: TODO: Handle case of output not having crtc */
     }

   /* get the current desktop at this crtc coordinate */
   con = e_container_current_get(e_manager_current_get());
   zone = e_container_zone_at_point_get(con, cx, cy);
   if (!(desk = e_desk_at_xy_get(zone, cx, cy)))
     desk = e_desk_current_get(zone);

   /* set references to the container & zone number
    * 
    * NB: Used later if background gets updated */
   sd->con_num = con->num;
   sd->zone_num = zone->num;

   /* set the background image */
   _e_smart_monitor_background_set(sd, desk->x, desk->y);

   /* if we have an output, lets set the monitor name */
   if (sd->output) 
     {
        E_Randr_Monitor_Info *monitor = NULL;
        const char *name = NULL;

        if ((monitor = sd->output->monitor))
          name = ecore_x_randr_edid_display_name_get(monitor->edid, 
                                                     monitor->edid_length);
        if (!name) name = sd->output->name;
        edje_object_part_text_set(sd->o_frame, "e.text.name", name);
     }

   /* if we have an output, lets set the resolution name */
   if ((sd->output) && (sd->output->crtc))
     {
        Ecore_X_Randr_Mode_Info *mode;

        if ((mode = sd->output->crtc->current_mode))
          {
             char buff[1024];

             snprintf(buff, sizeof(buff), "%d x %d", 
                      mode->width, mode->height);
             edje_object_part_text_set(sd->o_frame, "e.text.resolution", buff);
          }
     }

   /* check if enabled */
   if ((sd->output) && (sd->output->crtc))
     {
        if (sd->output->crtc->current_mode)
          sd->enabled = EINA_TRUE;
     }

   /* send enabled/disabled signals */
   if (sd->enabled)
     edje_object_signal_emit(sd->o_frame, "e,state,enabled", "e");
   else
     edje_object_signal_emit(sd->o_frame, "e,state,disabled", "e");

   /* with the background all setup, calculate the smallest frame width */
   edje_object_size_min_get(sd->o_frame, &fw, &fh);

   if (sd->layout.obj)
     {
        /* get smallest resolution and convert to smallest canvas size */
        e_layout_coord_virtual_to_canvas(sd->layout.obj, 
                                         sd->min.w, sd->min.h, &mw, &mh);
     }

   /* if min resolution is smaller than the frame, then set the 
    * objects min size to frame size */
   if (mw < fw) mw = fw;
   if (mh < fh) mh = fh;
   evas_object_size_hint_min_set(obj, mw, mh);
}

E_Randr_Output_Info *
e_smart_monitor_output_get(Evas_Object *obj)
{
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return NULL;

   /* return the monitor's referenced output */
   return sd->output;
}

void 
e_smart_monitor_layout_set(Evas_Object *obj, Evas_Object *layout)
{
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* set this monitor's layout reference */
   sd->layout.obj = layout;

   /* get the layout's virtual size */
   e_layout_virtual_size_get(layout, &sd->layout.vw, &sd->layout.vh);

   evas_object_event_callback_add(layout, EVAS_CALLBACK_MOVE, 
                                  _e_smart_monitor_layout_cb_move, sd);
}

Evas_Object *
e_smart_monitor_layout_get(Evas_Object *obj)
{
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return NULL;

   /* return the monitor's referenced layout widget */
   return sd->layout.obj;
}

void 
e_smart_monitor_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   if (!obj) return;
   e_layout_child_move(obj, x, y);
}

void 
e_smart_monitor_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   if (!obj) return;
   e_layout_child_resize(obj, w, h);
}

/* local functions */
static void 
_e_smart_add(Evas_Object *obj)
{
   E_Smart_Data *sd;
   Evas *evas;

   /* try to allocate the smart data structure */
   if (!(sd = E_NEW(E_Smart_Data, 1))) return;

   /* grab the canvas */
   evas = evas_object_evas_get(obj);

   /* create the base object */
   sd->o_base = edje_object_add(evas);
   e_theme_edje_object_set(sd->o_base, "base/theme/widgets", 
                           "e/conf/randr/main/monitor");
   evas_object_smart_member_add(sd->o_base, obj);

   /* create monitor 'frame' */
   sd->o_frame = edje_object_add(evas);
   e_theme_edje_object_set(sd->o_frame, "base/theme/widgets", 
                           "e/conf/randr/main/frame");
   edje_object_part_swallow(sd->o_base, "e.swallow.frame", sd->o_frame);
   evas_object_event_callback_add(sd->o_frame, EVAS_CALLBACK_MOUSE_MOVE, 
                                  _e_smart_monitor_frame_cb_mouse_move, obj);

   /* create the preview */
   sd->o_thumb = e_livethumb_add(evas);
   edje_object_part_swallow(sd->o_frame, "e.swallow.preview", sd->o_thumb);
   evas_object_event_callback_add(sd->o_thumb, EVAS_CALLBACK_MOUSE_IN, 
                                  _e_smart_monitor_thumb_cb_mouse_in, NULL);
   evas_object_event_callback_add(sd->o_thumb, EVAS_CALLBACK_MOUSE_OUT, 
                                  _e_smart_monitor_thumb_cb_mouse_out, NULL);
   evas_object_event_callback_add(sd->o_thumb, EVAS_CALLBACK_MOUSE_UP, 
                                  _e_smart_monitor_thumb_cb_mouse_up, obj);
   evas_object_event_callback_add(sd->o_thumb, EVAS_CALLBACK_MOUSE_DOWN, 
                                  _e_smart_monitor_thumb_cb_mouse_down, obj);

   /* create monitor stand */
   sd->o_stand = edje_object_add(evas);
   e_theme_edje_object_set(sd->o_stand, "base/theme/widgets", 
                           "e/conf/randr/main/stand");
   edje_object_part_swallow(sd->o_base, "e.swallow.stand", sd->o_stand);

   /* add callbacks for resize signals */
   edje_object_signal_callback_add(sd->o_frame, "e,action,resize,in", "e", 
                                   _e_smart_monitor_frame_cb_resize_in, NULL);
   edje_object_signal_callback_add(sd->o_frame, "e,action,resize,out", "e", 
                                   _e_smart_monitor_frame_cb_resize_out, NULL);

   /* add callbacks for rotate signals */
   edje_object_signal_callback_add(sd->o_frame, "e,action,rotate,in", "e", 
                                   _e_smart_monitor_frame_cb_rotate_in, sd);
   edje_object_signal_callback_add(sd->o_frame, "e,action,rotate,out", "e", 
                                   _e_smart_monitor_frame_cb_rotate_out, sd);

   /* add callbacks for indicator signals */
   edje_object_signal_callback_add(sd->o_frame, "e,action,indicator,in", "e", 
                                   _e_smart_monitor_frame_cb_indicator_in, sd);
   edje_object_signal_callback_add(sd->o_frame, "e,action,indicator,out", "e", 
                                   _e_smart_monitor_frame_cb_indicator_out, sd);
   edje_object_signal_callback_add(sd->o_frame, 
                                   "e,action,indicator,toggle", "e", 
                                   _e_smart_monitor_frame_cb_indicator_toggle, 
                                   obj);

   /* NB: Not entirely sure this is needed */
   /* evas_object_stack_below(sd->o_stand, sd->o_frame); */

   /* add handler for bg updates */
   sd->bg_update_hdl = 
     ecore_event_handler_add(E_EVENT_BG_UPDATE, 
                             _e_smart_monitor_background_update, sd);

   /* set the objects smart data */
   evas_object_smart_data_set(obj, sd);
}

static void 
_e_smart_del(Evas_Object *obj)
{
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* delete the list of modes */
   if (sd->modes) eina_list_free(sd->modes);

   /* delete the bg update handler */
   if (sd->bg_update_hdl) ecore_event_handler_del(sd->bg_update_hdl);

   /* delete the stand object */
   if (sd->o_stand) evas_object_del(sd->o_stand);

   /* delete the preview */
   if (sd->o_thumb) 
     {
        evas_object_event_callback_del(sd->o_thumb, EVAS_CALLBACK_MOUSE_IN, 
                                       _e_smart_monitor_thumb_cb_mouse_in);
        evas_object_event_callback_del(sd->o_thumb, EVAS_CALLBACK_MOUSE_OUT, 
                                       _e_smart_monitor_thumb_cb_mouse_out);
        evas_object_event_callback_del(sd->o_thumb, EVAS_CALLBACK_MOUSE_UP, 
                                       _e_smart_monitor_thumb_cb_mouse_up);
        evas_object_event_callback_del(sd->o_thumb, EVAS_CALLBACK_MOUSE_DOWN, 
                                       _e_smart_monitor_thumb_cb_mouse_down);
        evas_object_del(sd->o_thumb);
     }

   /* delete the frame object */
   if (sd->o_frame) 
     {
        edje_object_signal_callback_del(sd->o_frame, 
                                        "e,action,resize,in", "e", 
                                        _e_smart_monitor_frame_cb_resize_in);
        edje_object_signal_callback_del(sd->o_frame, 
                                        "e,action,resize,out", "e", 
                                        _e_smart_monitor_frame_cb_resize_out);
        edje_object_signal_callback_del(sd->o_frame, 
                                        "e,action,rotate,in", "e", 
                                        _e_smart_monitor_frame_cb_rotate_in);
        edje_object_signal_callback_del(sd->o_frame, 
                                        "e,action,rotate,out", "e", 
                                        _e_smart_monitor_frame_cb_rotate_out);
        edje_object_signal_callback_del(sd->o_frame, 
                                        "e,action,indicator,in", "e", 
                                        _e_smart_monitor_frame_cb_indicator_in);
        edje_object_signal_callback_del(sd->o_frame, 
                                        "e,action,indicator,out", "e", 
                                        _e_smart_monitor_frame_cb_indicator_out);
        edje_object_signal_callback_del(sd->o_frame, 
                                        "e,action,indicator,toggle", "e", 
                                        _e_smart_monitor_frame_cb_indicator_toggle);
        evas_object_event_callback_del(sd->o_frame, EVAS_CALLBACK_MOUSE_MOVE, 
                                       _e_smart_monitor_frame_cb_mouse_move);
        evas_object_del(sd->o_frame);
     }

   /* delete the base object */
   if (sd->o_base) evas_object_del(sd->o_base);

   /* try to free the allocated structure */
   E_FREE(sd);

   /* set the objects smart data to null */
   evas_object_smart_data_set(obj, NULL);
}

static void 
_e_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   if ((sd->x == x) && (sd->y == y)) return;

   sd->x = x;
   sd->y = y;

   /* move the base object */
   if (sd->o_base) evas_object_move(sd->o_base, x, y);
}

static void 
_e_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   if ((sd->w == h) && (sd->w == h)) return;

   sd->w = w;
   sd->h = h;

   /* resize the base object */
   if (sd->o_base) evas_object_resize(sd->o_base, w, h);
}

static void 
_e_smart_show(Evas_Object *obj)
{
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* if it is already visible, get out */
   if (sd->visible) return;

   /* show the grid */
   if (sd->o_base) evas_object_show(sd->o_base);

   /* set visibility flag */
   sd->visible = EINA_TRUE;
}

static void 
_e_smart_hide(Evas_Object *obj)
{
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* if it is not visible, we have nothing to do */
   if (!sd->visible) return;

   /* hide the grid */
   if (sd->o_base) evas_object_hide(sd->o_base);

   /* set visibility flag */
   sd->visible = EINA_FALSE;
}

static void 
_e_smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* set the clip */
   if (sd->o_stand) evas_object_clip_set(sd->o_stand, clip);
   if (sd->o_frame) evas_object_clip_set(sd->o_frame, clip);
   if (sd->o_base) evas_object_clip_set(sd->o_base, clip);
}

static void 
_e_smart_clip_unset(Evas_Object *obj)
{
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* unset the clip */
   if (sd->o_stand) evas_object_clip_unset(sd->o_stand);
   if (sd->o_frame) evas_object_clip_unset(sd->o_frame);
   if (sd->o_base) evas_object_clip_unset(sd->o_base);
}

static void 
_e_smart_monitor_modes_fill(E_Smart_Data *sd)
{
   /* clear out any old modes */
   if (sd->modes) eina_list_free(sd->modes);

   /* make sure we have a valid output */
   if (!sd->output) return;

   /* if we have an assigned monitor, copy the modes from that */
   if (sd->output->monitor)
     sd->modes = eina_list_clone(sd->output->monitor->modes);

   /* FIXME: NB: TODO: Test/check for Common Modes */

   /* sort the mode list (smallest to largest) */
   sd->modes = eina_list_sort(sd->modes, 0, _e_smart_monitor_modes_sort);

   /* try to determine the min & max resolutions */
   if (sd->modes)
     {
        Ecore_X_Randr_Mode_Info *mode = NULL;

        /* try to get smallest resolution */
        if ((mode = eina_list_nth(sd->modes, 0)))
          {
             sd->min.w = mode->width;
             sd->min.h = mode->height;
          }

        /* try to get largest resolution */
        if ((mode = eina_list_last_data_get(sd->modes)))
          {
             sd->max.w = mode->width;
             sd->max.h = mode->height;
          }
     }
}

static int 
_e_smart_monitor_modes_sort(const void *data1, const void *data2)
{
   const Ecore_X_Randr_Mode_Info *m1, *m2 = NULL;

   if (!(m1 = data1)) return 1;
   if (!(m2 = data2)) return -1;

   /* second one compares to previous to determine position */
   if (m2->width < m1->width) return 1;
   if (m2->width > m1->width) return -1;

   /* width are same, compare heights */
   if ((m2->width == m1->width))
     {
        if (m2->height < m1->height) return 1;
        if (m2->height > m1->height) return -1;
     }

   return 1;
}

static void 
_e_smart_monitor_background_set(E_Smart_Data *sd, Evas_Coord dx, Evas_Coord dy)
{
   const char *bg = NULL;

   if (!sd->o_thumb) return;

   /* try to get the bg file for this desktop */
   if ((bg = e_bg_file_get(sd->con_num, sd->zone_num, dx, dy)))
     {
        Evas_Object *o;

        /* try to get the livethumb, if not then create an object */
        if (!(o = e_livethumb_thumb_get(sd->o_thumb)))
          o = edje_object_add(e_livethumb_evas_get(sd->o_thumb));

        /* tell the object to use this edje file & group */
        edje_object_file_set(o, bg, "e/desktop/background");

        /* tell the thumbnail to use this object for preview */
        e_livethumb_thumb_set(sd->o_thumb, o);
     }
}

static Eina_Bool 
_e_smart_monitor_background_update(void *data, int type, void *event)
{
   E_Smart_Data *sd;
   E_Event_Bg_Update *ev;

   if (type != E_EVENT_BG_UPDATE) return ECORE_CALLBACK_PASS_ON;
   if (!(sd = data)) return ECORE_CALLBACK_PASS_ON;

   ev = event;

   if (((ev->container < 0) || (ev->container == sd->con_num)) && 
       ((ev->zone < 0) || (ev->zone == sd->zone_num)))
     {
        Evas_Coord cx = 0, cy = 0;

        /* if we have a crtc, get the x/y location of it */
        if ((sd->output) && (sd->output->crtc))
          {
             cx = sd->output->crtc->geometry.x;
             cy = sd->output->crtc->geometry.y;
          }
        else
          {
             /* FIXME: NB: TODO: Handle case of output not having crtc */
          }

        if (((ev->desk_x < 0) || (ev->desk_x == cx)) && 
            ((ev->desk_y < 0) || (ev->desk_y == cy)))
          _e_smart_monitor_background_set(sd, ev->desk_x, ev->desk_y);
     }

   return ECORE_CALLBACK_PASS_ON;
}

static void 
_e_smart_monitor_move_event(E_Smart_Data *sd, Evas_Object *mon, void *event)
{
   Evas_Event_Mouse_Move *ev;
   Evas_Coord dx = 0, dy = 0;
   Evas_Coord nx = 0, ny = 0;
   Evas_Coord mx = 0, my = 0;
   Evas_Coord mw = 0, mh = 0;

   /* check for valid smart data */
   if (!sd) return;

   ev = event;

   /* calculate the difference in mouse movement */
   dx = (ev->cur.canvas.x - ev->prev.canvas.x);
   dy = (ev->cur.canvas.y - ev->prev.canvas.y);

   /* convert these coords to virtual space */
   e_layout_coord_canvas_to_virtual(sd->layout.obj, (sd->layout.x + dx), 
                                    (sd->layout.y + dy), &nx, &ny);

   /* get monitors current geometry */
   e_layout_child_geometry_get(mon, &mx, &my, &mw, &mh);
   nx += mx;
   ny += my;

   /* constrain to the layout bounds */
   if (nx < sd->layout.x) 
     nx = sd->layout.x;
   else if (nx > (sd->layout.x + sd->layout.vw) - mw)
     nx = (sd->layout.x + sd->layout.vw) - mw;

   if (ny < sd->layout.y) 
     ny = sd->layout.y;
   else if (ny > (sd->layout.y + sd->layout.vh) - mh)
     ny = (sd->layout.y + sd->layout.vh) - mh;

   /* actually do the move */
   if ((nx != mx) || (ny != my))
     {
        e_layout_child_move(mon, nx, ny);
        evas_object_smart_callback_call(mon, "monitor_moved", NULL);
     }

   /* TODO: handle changes */
}

/* local callbacks */
static void 
_e_smart_monitor_frame_cb_mouse_move(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event)
{
   Evas_Object *mon;
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(mon = data)) return;
   if (!(sd = evas_object_smart_data_get(mon))) return;

   /* TODO: handle moving, resize, rotating */
   if (sd->moving)
     _e_smart_monitor_move_event(sd, mon, event);
}

static void 
_e_smart_monitor_frame_cb_resize_in(void *data __UNUSED__, Evas_Object *obj, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   E_Manager *man;

   if ((man = e_manager_current_get()))
     e_pointer_type_push(man->pointer, obj, "resize_br");
}

static void 
_e_smart_monitor_frame_cb_resize_out(void *data __UNUSED__, Evas_Object *obj, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   E_Manager *man;

   if ((man = e_manager_current_get()))
     e_pointer_type_pop(man->pointer, obj, "resize_br");
}

static void 
_e_smart_monitor_frame_cb_rotate_in(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   E_Smart_Data *sd;
   Ecore_Evas *ee;
   Ecore_X_Window win;
   Ecore_X_Cursor cur;

   if (!(sd = data)) return;

   /* changing cursors for rotate is done this way because e_pointer 
    * does not support all available X cursors */
   ee = ecore_evas_ecore_evas_get(evas_object_evas_get(sd->o_frame));
   win = (Ecore_X_Window)ecore_evas_window_get(ee);

   cur = ecore_x_cursor_shape_get(ECORE_X_CURSOR_EXCHANGE);
   ecore_x_window_cursor_set(win, cur);
   ecore_x_cursor_free(cur);
}

static void 
_e_smart_monitor_frame_cb_rotate_out(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   E_Smart_Data *sd;
   Ecore_Evas *ee;
   Ecore_X_Window win;

   if (!(sd = data)) return;

   /* reset cursor back to default */
   ee = ecore_evas_ecore_evas_get(evas_object_evas_get(sd->o_frame));
   win = (Ecore_X_Window)ecore_evas_window_get(ee);
   ecore_x_window_cursor_set(win, 0);
}

static void 
_e_smart_monitor_frame_cb_indicator_in(void *data __UNUSED__, Evas_Object *obj, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   E_Manager *man;

   if ((man = e_manager_current_get()))
     e_pointer_type_push(man->pointer, obj, "hand");
}

static void 
_e_smart_monitor_frame_cb_indicator_out(void *data __UNUSED__, Evas_Object *obj, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   E_Manager *man;

   if ((man = e_manager_current_get()))
     e_pointer_type_pop(man->pointer, obj, "hand");
}

static void 
_e_smart_monitor_frame_cb_indicator_toggle(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Evas_Object *mon;
   E_Smart_Data *sd;

   if (!(mon = data)) return;
   if (!(sd = evas_object_smart_data_get(mon))) return;

   if (sd->enabled)
     {
        sd->enabled = EINA_FALSE;
        edje_object_signal_emit(sd->o_frame, "e,state,disabled", "e");
     }
   else
     {
        sd->enabled = EINA_TRUE;
        edje_object_signal_emit(sd->o_frame, "e,state,enabled", "e");
     }

   /* TODO: Record changes */

   evas_object_smart_callback_call(mon, "monitor_toggled", NULL);
}

static void 
_e_smart_monitor_thumb_cb_mouse_in(void *data __UNUSED__, Evas *evas __UNUSED__, Evas_Object *obj, void *event __UNUSED__)
{
   E_Manager *man;

   if ((man = e_manager_current_get()))
     e_pointer_type_push(man->pointer, obj, "hand");
}

static void 
_e_smart_monitor_thumb_cb_mouse_out(void *data __UNUSED__, Evas *evas __UNUSED__, Evas_Object *obj, void *event __UNUSED__)
{
   E_Manager *man;

   if ((man = e_manager_current_get()))
     e_pointer_type_pop(man->pointer, obj, "hand");
}

static void 
_e_smart_monitor_thumb_cb_mouse_down(void *data, Evas *evas __UNUSED__, Evas_Object *obj, void *event)
{
   Evas_Event_Mouse_Down *ev;

   ev = event;
   if (ev->button == 1)
     {
        E_Manager *man;
        Evas_Object *mon;
        E_Smart_Data *sd;

        if ((man = e_manager_current_get()))
          e_pointer_type_push(man->pointer, obj, "move");

        if (!(mon = data)) return;
        if (!(sd = evas_object_smart_data_get(mon))) return;

        /* set moving state */
        sd->moving = EINA_TRUE;

        /* raise this monitor */
        e_layout_child_raise(mon);
     }
}

static void 
_e_smart_monitor_thumb_cb_mouse_up(void *data, Evas *evas __UNUSED__, Evas_Object *obj, void *event)
{
   Evas_Event_Mouse_Up *ev;

   ev = event;
   if (ev->button == 1)
     {
        E_Manager *man;
        Evas_Object *mon;
        E_Smart_Data *sd;

        if ((man = e_manager_current_get()))
          e_pointer_type_pop(man->pointer, obj, "move");

        if (!(mon = data)) return;
        if (!(sd = evas_object_smart_data_get(mon))) return;

        /* set moving state */
        sd->moving = EINA_FALSE;

        /* send monitor moved signal */
        evas_object_smart_callback_call(mon, "monitor_moved", NULL);
     }
}

static void 
_e_smart_monitor_layout_cb_move(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event __UNUSED__)
{
   E_Smart_Data *sd;

   if (!(sd = data)) return;

   /* get the layout's geometry */
   evas_object_geometry_get(sd->layout.obj, 
                            &sd->layout.x, &sd->layout.y, NULL, NULL);
}
