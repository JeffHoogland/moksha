#include "e.h"
#include "e_mod_main.h"
#include "e_smart_monitor.h"

#define RESIZE_FUZZ 60

/* local structure */
typedef struct _E_Smart_Data E_Smart_Data;
struct _E_Smart_Data
{
   /* canvas variable */
   Evas *evas;

   /* geometry */
   int x, y, w, h;

   struct 
     {
        Evas_Coord mode_width, mode_height;
     } min, max;

   struct 
     {
        /* reference to the grid we are packed into */
        Evas_Object *obj;
        Evas_Coord x, y, w, h;
     } grid;

   /* virtual size of the grid */
   Evas_Coord vw, vh;

   /* test object */
   /* Evas_Object *o_bg; */

   /* base object */
   Evas_Object *o_base;

   /* frame object */
   Evas_Object *o_frame;

   /* stand object */
   Evas_Object *o_stand;

   /* background thumbnail */
   Evas_Object *o_thumb;

   /* crtc config */
   Ecore_X_Randr_Crtc crtc;

   /* crtc geometry */
   Evas_Coord cx, cy, cw, ch;

   /* output config */
   Ecore_X_Randr_Output output;

   /* container number */
   unsigned int con_num;

   /* zone number */
   unsigned int zone_num;

   /* event handler for background image updates */
   Ecore_Event_Handler *bg_update_hdl;

   /* list of modes */
   Eina_List *modes;

   /* visibility flag */
   Eina_Bool visible : 1;

   /* resizing flag */
   Eina_Bool resizing : 1;

   /* coordinates where the user clicked to start resizing */
   Evas_Coord rx, ry;

   /* rotating flag */
   Eina_Bool rotating : 1;
};

/* smart function prototypes */
static void _e_smart_add(Evas_Object *obj);
static void _e_smart_del(Evas_Object *obj);
static void _e_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y);
static void _e_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h);
static void _e_smart_show(Evas_Object *obj);
static void _e_smart_hide(Evas_Object *obj);
static void _e_smart_clip_set(Evas_Object *obj, Evas_Object *clip);
static void _e_smart_clip_unset(Evas_Object *obj);

/* local function prototypes */
static void _e_smart_monitor_modes_fill(E_Smart_Data *sd);
static int _e_smart_monitor_modes_sort(const void *data1, const void *data2);
static void _e_smart_monitor_background_set(E_Smart_Data *sd, int dx, int dy);
static Eina_Bool _e_smart_monitor_background_update(void *data, int type EINA_UNUSED, void *event);
static void _e_smart_monitor_position_set(E_Smart_Data *sd, Evas_Coord x, Evas_Coord y);
static void _e_smart_monitor_resolution_set(E_Smart_Data *sd, Evas_Coord w, Evas_Coord h);
static void _e_smart_monitor_pointer_push(Evas_Object *obj, const char *ptr);
static void _e_smart_monitor_pointer_pop(Evas_Object *obj, const char *ptr);

static inline void _e_smart_monitor_coord_virtual_to_canvas(E_Smart_Data *sd, double vx, double vy, double *cx, double *cy);
static inline void _e_smart_monitor_coord_canvas_to_virtual(E_Smart_Data *sd, double cx, double cy, double *vx, double *vy);
static Ecore_X_Randr_Mode_Info *_e_smart_monitor_mode_find(E_Smart_Data *sd, Evas_Coord w, Evas_Coord h, Eina_Bool skip_refresh);

static void _e_smart_monitor_thumb_cb_mouse_in(void *data EINA_UNUSED, Evas *evas EINA_UNUSED, Evas_Object *obj, void *event EINA_UNUSED);
static void _e_smart_monitor_thumb_cb_mouse_out(void *data EINA_UNUSED, Evas *evas EINA_UNUSED, Evas_Object *obj, void *event EINA_UNUSED);
static void _e_smart_monitor_thumb_cb_mouse_up(void *data EINA_UNUSED, Evas *evas EINA_UNUSED, Evas_Object *obj, void *event);
static void _e_smart_monitor_thumb_cb_mouse_down(void *data EINA_UNUSED, Evas *evas EINA_UNUSED, Evas_Object *obj, void *event);

static void _e_smart_monitor_frame_cb_mouse_move(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event);
static void _e_smart_monitor_frame_cb_resize_in(void *data EINA_UNUSED, Evas_Object *obj, const char *emission EINA_UNUSED, const char *source EINA_UNUSED);
static void _e_smart_monitor_frame_cb_resize_out(void *data EINA_UNUSED, Evas_Object *obj, const char *emission EINA_UNUSED, const char *source EINA_UNUSED);
static void _e_smart_monitor_frame_cb_rotate_in(void *data EINA_UNUSED, Evas_Object *obj, const char *emission EINA_UNUSED, const char *source EINA_UNUSED);
static void _e_smart_monitor_frame_cb_rotate_out(void *data EINA_UNUSED, Evas_Object *obj, const char *emission EINA_UNUSED, const char *source EINA_UNUSED);
static void _e_smart_monitor_frame_cb_indicator_in(void *data EINA_UNUSED, Evas_Object *obj, const char *emission EINA_UNUSED, const char *source EINA_UNUSED);
static void _e_smart_monitor_frame_cb_indicator_out(void *data EINA_UNUSED, Evas_Object *obj, const char *emission EINA_UNUSED, const char *source EINA_UNUSED);

static void _e_smart_monitor_frame_cb_resize_start(void *data, Evas_Object *obj EINA_UNUSED, const char *emission EINA_UNUSED, const char *source EINA_UNUSED);
static void _e_smart_monitor_frame_cb_resize_stop(void *data, Evas_Object *obj EINA_UNUSED, const char *emission EINA_UNUSED, const char *source EINA_UNUSED);
static void _e_smart_monitor_frame_cb_rotate_start(void *data, Evas_Object *obj EINA_UNUSED, const char *emission EINA_UNUSED, const char *source EINA_UNUSED);
static void _e_smart_monitor_frame_cb_rotate_stop(void *data, Evas_Object *obj EINA_UNUSED, const char *emission EINA_UNUSED, const char *source EINA_UNUSED);

static void _e_smart_monitor_resize_event(E_Smart_Data *sd, Evas_Object *mon, void *event);
static void _e_smart_monitor_rotate_event(E_Smart_Data *sd, Evas_Object *mon, void *event);

/* external functions exposed by this widget */
Evas_Object *
e_smart_monitor_add(Evas *evas)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

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
e_smart_monitor_crtc_set(Evas_Object *obj, Ecore_X_Randr_Crtc crtc, Evas_Coord cx, Evas_Coord cy, Evas_Coord cw, Evas_Coord ch)
{
   E_Smart_Data *sd;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* set the crtc config */
   sd->crtc = crtc;

   /* record the crtc geometry */
   sd->cx = cx;
   sd->cy = cy;
   sd->cw = cw;
   sd->ch = ch;

   /* set monitor position text */
   _e_smart_monitor_position_set(sd, sd->cx, sd->cy);

   /* set monitor resolution text */
   _e_smart_monitor_resolution_set(sd, sd->cw, sd->ch);
}

void 
e_smart_monitor_output_set(Evas_Object *obj, Ecore_X_Randr_Output output)
{
   E_Smart_Data *sd;
   Ecore_X_Randr_Mode_Info *mode;
   Ecore_X_Window root = 0;
   char *name = NULL;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* set the output config */
   sd->output = output;

   /* since we now have the output, let's be preemptive and fill in modes */
   _e_smart_monitor_modes_fill(sd);
   if (!sd->modes) return;

   /* get the largest mode */
   mode = eina_list_last_data_get(sd->modes);
   sd->max.mode_width = mode->width;
   sd->max.mode_height = mode->height;

   /* get the root window */
   root = ecore_x_window_root_first_get();

   /* get output name */
   if (!(name = ecore_x_randr_output_name_get(root, sd->output, NULL)))
     {
        unsigned char *edid = NULL;
        unsigned long edid_length = 0;

        /* get the edid for this output */
        if ((edid = ecore_x_randr_output_edid_get(0, sd->output, &edid_length)))
          {
             /* get output name */
             name = ecore_x_randr_edid_display_name_get(edid, edid_length);

             /* free any memory allocated from ecore_x_randr */
             free(edid);
          }
     }

   /* set monitor name */
   edje_object_part_text_set(sd->o_frame, "e.text.name", name);

   /* free any memory allocated from ecore_x_randr */
   free(name);

   /* get the smallest mode */
   mode = eina_list_nth(sd->modes, 0);
   sd->min.mode_width = mode->width;
   sd->min.mode_height = mode->height;
}

void 
e_smart_monitor_grid_set(Evas_Object *obj, Evas_Object *grid, Evas_Coord gx, Evas_Coord gy, Evas_Coord gw, Evas_Coord gh)
{
   E_Smart_Data *sd;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   sd->grid.obj = grid;
   sd->grid.x = gx;
   sd->grid.y = gy;
   sd->grid.w = gw;
   sd->grid.h = gh;
}

void 
e_smart_monitor_virtual_size_set(Evas_Object *obj, Evas_Coord vw, Evas_Coord vh)
{
   E_Smart_Data *sd;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   sd->vw = vw;
   sd->vh = vh;
}

void 
e_smart_monitor_background_set(Evas_Object *obj, Evas_Coord dx, Evas_Coord dy)
{
   E_Smart_Data *sd;
   E_Manager *man;
   E_Container *con;
   E_Zone *zone;
   E_Desk *desk;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* get the current manager */
   man = e_manager_current_get();

   /* get the current container */
   con = e_container_current_get(man);
   sd->con_num = con->num;

   /* get the zone number */
   if (!(zone = e_container_zone_at_point_get(con, dx, dy)))
     zone = e_util_zone_current_get(man);
   sd->zone_num = zone->num;

   /* get the desk */
   if (!(desk = e_desk_at_xy_get(zone, sd->cx, sd->cy)))
     desk = e_desk_current_get(zone);

   /* set the background image */
   _e_smart_monitor_background_set(sd, desk->x, desk->y);
}

/* smart functions */
static void 
_e_smart_add(Evas_Object *obj)
{
   E_Smart_Data *sd;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to allocate the smart data structure */
   if (!(sd = E_NEW(E_Smart_Data, 1))) return;

   /* grab the canvas */
   sd->evas = evas_object_evas_get(obj);

   /* create the bg test object */
   /* sd->o_bg = evas_object_rectangle_add(sd->evas); */
   /* evas_object_color_set(sd->o_bg, 255, 0, 0, 255); */
   /* evas_object_smart_member_add(sd->o_bg, obj); */

   /* create the base object */
   sd->o_base = edje_object_add(sd->evas);
   e_theme_edje_object_set(sd->o_base, "base/theme/widgets", 
                           "e/conf/randr/main/monitor");
   evas_object_smart_member_add(sd->o_base, obj);

   /* create the frame object */
   sd->o_frame = edje_object_add(sd->evas);
   e_theme_edje_object_set(sd->o_frame, "base/theme/widgets", 
                           "e/conf/randr/main/frame");
   edje_object_part_swallow(sd->o_base, "e.swallow.frame", sd->o_frame);

   /* add callbacks for frame events */
   evas_object_event_callback_add(sd->o_frame, EVAS_CALLBACK_MOUSE_MOVE, 
                                  _e_smart_monitor_frame_cb_mouse_move, obj);

   edje_object_signal_callback_add(sd->o_frame, "e,action,resize,in", "e", 
                                   _e_smart_monitor_frame_cb_resize_in, NULL);
   edje_object_signal_callback_add(sd->o_frame, "e,action,resize,out", "e", 
                                   _e_smart_monitor_frame_cb_resize_out, NULL);
   edje_object_signal_callback_add(sd->o_frame, "e,action,rotate,in", "e", 
                                   _e_smart_monitor_frame_cb_rotate_in, NULL);
   edje_object_signal_callback_add(sd->o_frame, "e,action,rotate,out", "e", 
                                   _e_smart_monitor_frame_cb_rotate_out, NULL);
   edje_object_signal_callback_add(sd->o_frame, "e,action,indicator,in", "e", 
                                   _e_smart_monitor_frame_cb_indicator_in, NULL);
   edje_object_signal_callback_add(sd->o_frame, "e,action,indicator,out", "e", 
                                   _e_smart_monitor_frame_cb_indicator_out, NULL);

   edje_object_signal_callback_add(sd->o_frame, "e,action,resize,start", "e", 
                                   _e_smart_monitor_frame_cb_resize_start, obj);
   edje_object_signal_callback_add(sd->o_frame, "e,action,resize,stop", "e", 
                                   _e_smart_monitor_frame_cb_resize_stop, obj);
   edje_object_signal_callback_add(sd->o_frame, "e,action,rotate,start", "e", 
                                   _e_smart_monitor_frame_cb_rotate_start, obj);
   edje_object_signal_callback_add(sd->o_frame, "e,action,rotate,stop", "e", 
                                   _e_smart_monitor_frame_cb_rotate_stop, obj);

   /* create the stand */
   sd->o_stand = edje_object_add(sd->evas);
   e_theme_edje_object_set(sd->o_stand, "base/theme/widgets", 
                           "e/conf/randr/main/stand");
   edje_object_part_swallow(sd->o_base, "e.swallow.stand", sd->o_stand);

   /* create the background preview */
   sd->o_thumb = e_livethumb_add(sd->evas);
   edje_object_part_swallow(sd->o_frame, "e.swallow.preview", sd->o_thumb);

   /* add callbacks for thumbnail events */
   evas_object_event_callback_add(sd->o_thumb, EVAS_CALLBACK_MOUSE_IN, 
                                  _e_smart_monitor_thumb_cb_mouse_in, NULL);
   evas_object_event_callback_add(sd->o_thumb, EVAS_CALLBACK_MOUSE_OUT, 
                                  _e_smart_monitor_thumb_cb_mouse_out, NULL);
   evas_object_event_callback_add(sd->o_thumb, EVAS_CALLBACK_MOUSE_UP, 
                                  _e_smart_monitor_thumb_cb_mouse_up, NULL);
   evas_object_event_callback_add(sd->o_thumb, EVAS_CALLBACK_MOUSE_DOWN, 
                                  _e_smart_monitor_thumb_cb_mouse_down, NULL);

   /* setup event handler for bg image updates */
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
   Ecore_X_Randr_Mode_Info *mode;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* delete the bg update handler */
   ecore_event_handler_del(sd->bg_update_hdl);

   if (sd->o_thumb)
     {
        /* delete the event callbacks */
        evas_object_event_callback_del(sd->o_thumb, EVAS_CALLBACK_MOUSE_IN, 
                                       _e_smart_monitor_thumb_cb_mouse_in);
        evas_object_event_callback_del(sd->o_thumb, EVAS_CALLBACK_MOUSE_OUT, 
                                       _e_smart_monitor_thumb_cb_mouse_out);
        evas_object_event_callback_del(sd->o_thumb, EVAS_CALLBACK_MOUSE_UP, 
                                       _e_smart_monitor_thumb_cb_mouse_up);
        evas_object_event_callback_del(sd->o_thumb, EVAS_CALLBACK_MOUSE_DOWN, 
                                       _e_smart_monitor_thumb_cb_mouse_down);

        /* delete the object */
        evas_object_del(sd->o_thumb);
     }

   evas_object_del(sd->o_stand);

   if (sd->o_frame)
     {
        /* delete the event callbacks */
        evas_object_event_callback_del(sd->o_frame, EVAS_CALLBACK_MOUSE_MOVE, 
                                       _e_smart_monitor_frame_cb_mouse_move);

        edje_object_signal_callback_del(sd->o_frame, "e,action,resize,in", "e", 
                                        _e_smart_monitor_frame_cb_resize_in);
        edje_object_signal_callback_del(sd->o_frame, "e,action,resize,out", "e", 
                                        _e_smart_monitor_frame_cb_resize_out);
        edje_object_signal_callback_del(sd->o_frame, "e,action,rotate,in", "e", 
                                        _e_smart_monitor_frame_cb_rotate_in);
        edje_object_signal_callback_del(sd->o_frame, "e,action,rotate,out", "e", 
                                        _e_smart_monitor_frame_cb_rotate_out);
        edje_object_signal_callback_del(sd->o_frame, "e,action,indicator,in", "e", 
                                        _e_smart_monitor_frame_cb_indicator_in);
        edje_object_signal_callback_del(sd->o_frame, "e,action,indicator,out", "e", 
                                        _e_smart_monitor_frame_cb_indicator_out);

        edje_object_signal_callback_del(sd->o_frame, "e,action,resize,start", "e", 
                                        _e_smart_monitor_frame_cb_resize_start);
        edje_object_signal_callback_del(sd->o_frame, "e,action,resize,stop", "e", 
                                        _e_smart_monitor_frame_cb_resize_stop);
        edje_object_signal_callback_del(sd->o_frame, "e,action,rotate,start", "e", 
                                        _e_smart_monitor_frame_cb_rotate_start);
        edje_object_signal_callback_del(sd->o_frame, "e,action,rotate,stop", "e", 
                                        _e_smart_monitor_frame_cb_rotate_stop);

        /* delete the object */
        evas_object_del(sd->o_frame);
     }

   evas_object_del(sd->o_base);
   /* evas_object_del(sd->o_bg); */

   /* free the list of modes */
   EINA_LIST_FREE(sd->modes, mode)
     ecore_x_randr_mode_info_free(mode);

   /* try to free the allocated structure */
   E_FREE(sd);

   /* set the objects smart data to null */
   evas_object_smart_data_set(obj, NULL);
}

static void 
_e_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   E_Smart_Data *sd;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* if there is no position change, then get out */
   if ((sd->x == x) && (sd->y == y)) return;

   sd->x = x;
   sd->y = y;

   /* evas_object_move(sd->o_bg, x, y); */
   evas_object_move(sd->o_base, x, y);
}

static void 
_e_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Smart_Data *sd;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* if there is no size change, then get out */
   if ((sd->w == w) && (sd->h == h)) return;

   sd->w = w;
   sd->h = h;

   /* set livethumb thumbnail size */
   if (!sd->resizing) e_livethumb_vsize_set(sd->o_thumb, sd->w, sd->h);

   evas_object_resize(sd->o_base, w, h);
   /* evas_object_resize(sd->o_bg, w, h + 30); */
}

static void 
_e_smart_show(Evas_Object *obj)
{
   E_Smart_Data *sd;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* if we are already visible, then nothing to do */
   if (sd->visible) return;

   evas_object_show(sd->o_base);
   /* evas_object_show(sd->o_bg); */

   /* set visibility flag */
   sd->visible = EINA_TRUE;
}

static void 
_e_smart_hide(Evas_Object *obj)
{
   E_Smart_Data *sd;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* if we are already hidden, then nothing to do */
   if (!sd->visible) return;

   evas_object_hide(sd->o_base);
   /* evas_object_hide(sd->o_bg); */

   /* set visibility flag */
   sd->visible = EINA_FALSE;
}

static void 
_e_smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
   E_Smart_Data *sd;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   evas_object_clip_set(sd->o_base, clip);
   /* evas_object_clip_set(sd->o_bg, clip); */
}

static void 
_e_smart_clip_unset(Evas_Object *obj)
{
   E_Smart_Data *sd;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   evas_object_clip_unset(sd->o_base);
   /* evas_object_clip_unset(sd->o_bg); */
}

/* local functions */
static void 
_e_smart_monitor_modes_fill(E_Smart_Data *sd)
{
   Ecore_X_Window root = 0;
   Ecore_X_Randr_Mode *modes;
   int num = 0, i = 0;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* safety check */
   if (!sd) return;

   /* try to get the root window */
   root = ecore_x_window_root_first_get();

   /* try to get the modes for this output from ecore_x_randr */
   modes = ecore_x_randr_output_modes_get(root, sd->output, &num, NULL);
   if (!modes) return;

   /* loop the returned modes */
   for (i = 0; i < num; i++)
     {
        Ecore_X_Randr_Mode_Info *mode;

        /* try to get the mode info */
        if (!(mode = ecore_x_randr_mode_info_get(root, modes[i])))
          continue;

        /* append the mode info to our list of modes */
        sd->modes = eina_list_append(sd->modes, mode);
     }

   /* free any memory allocated from ecore_x_randr */
   free(modes);

   /* sort the list of modes (smallest to largest) */
   if (sd->modes)
     sd->modes = eina_list_sort(sd->modes, 0, _e_smart_monitor_modes_sort);
}

static int 
_e_smart_monitor_modes_sort(const void *data1, const void *data2)
{
   const Ecore_X_Randr_Mode_Info *m1, *m2 = NULL;

//   LOGFN(__FILE__, __LINE__, __FUNCTION__);

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
_e_smart_monitor_background_set(E_Smart_Data *sd, int dx, int dy)
{
   const char *bg = NULL;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* check for valid smart data */
   if (!sd) return;

   /* try to get the background file for this desktop */
   if ((bg = e_bg_file_get(sd->con_num, sd->zone_num, dx, dy)))
     {
        Evas_Object *o;

        /* try to get the livethumb object, create if needed */
        if (!(o = e_livethumb_thumb_get(sd->o_thumb)))
          o = edje_object_add(e_livethumb_evas_get(sd->o_thumb));

        /* tell the object to use this edje file & group */
        edje_object_file_set(o, bg, "e/desktop/background");

        /* tell the livethumb to use this object */
        e_livethumb_thumb_set(sd->o_thumb, o);
     }
}

static Eina_Bool 
_e_smart_monitor_background_update(void *data, int type EINA_UNUSED, void *event)
{
   E_Smart_Data *sd;
   E_Event_Bg_Update *ev;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the smart data */
   if (!(sd = data)) return ECORE_CALLBACK_PASS_ON;

   ev = event;

   /* check this bg event happened on our container */
   if (((ev->container < 0) || (ev->container == (int)sd->con_num)) && 
       ((ev->zone < 0) || (ev->zone == (int)sd->zone_num)))
     {
        /* check this bg event happened on our desktop */
        if (((ev->desk_x < 0) || (ev->desk_x == sd->cx)) && 
            ((ev->desk_y < 0) || (ev->desk_y == sd->cy)))
          {
             /* set the livethumb preview to the background of this desktop */
             _e_smart_monitor_background_set(sd, ev->desk_x, ev->desk_y);
          }
     }

   return ECORE_CALLBACK_PASS_ON;
}

static void 
_e_smart_monitor_position_set(E_Smart_Data *sd, Evas_Coord x, Evas_Coord y)
{
   char buff[1024];

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   snprintf(buff, sizeof(buff), "%d + %d", x, y);
   edje_object_part_text_set(sd->o_frame, "e.text.position", buff);
}

static void 
_e_smart_monitor_resolution_set(E_Smart_Data *sd, Evas_Coord w, Evas_Coord h)
{
   char buff[1024];

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   snprintf(buff, sizeof(buff), "%d x %d", w, h);
   edje_object_part_text_set(sd->o_frame, "e.text.resolution", buff);
}

static void 
_e_smart_monitor_pointer_push(Evas_Object *obj, const char *ptr)
{
   Evas_Object *ow;
   E_Win *win;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to find the E_Win for this object */
   if (!(ow = evas_object_name_find(evas_object_evas_get(obj), "E_Win")))
     return;
   if (!(win = evas_object_data_get(ow, "E_Win"))) return;

   /* tell E to set the pointer type */
   e_pointer_type_push(win->pointer, obj, ptr);
}

static void 
_e_smart_monitor_pointer_pop(Evas_Object *obj, const char *ptr)
{
   Evas_Object *ow;
   E_Win *win;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to find the E_Win for this object */
   if (!(ow = evas_object_name_find(evas_object_evas_get(obj), "E_Win")))
     return;
   if (!(win = evas_object_data_get(ow, "E_Win"))) return;

   /* tell E to unset the pointer type */
   e_pointer_type_pop(win->pointer, obj, ptr);
}

static inline void 
_e_smart_monitor_coord_virtual_to_canvas(E_Smart_Data *sd, double vx, double vy, double *cx, double *cy)
{
   if (cx) *cx = (vx * ((double)(sd->grid.w) / sd->vw)) + sd->grid.x;
   if (cy) *cy = (vy * ((double)(sd->grid.h) / sd->vh)) + sd->grid.y;
}

static inline void 
_e_smart_monitor_coord_canvas_to_virtual(E_Smart_Data *sd, double cx, double cy, double *vx, double *vy)
{
   if (vx) *vx = ((cx - sd->grid.x) * sd->vw) / (double)sd->grid.w;
   if (vy) *vy = ((cy - sd->grid.y) * sd->vh) / (double)sd->grid.h;
}

static Ecore_X_Randr_Mode_Info *
_e_smart_monitor_mode_find(E_Smart_Data *sd, Evas_Coord w, Evas_Coord h, Eina_Bool skip_refresh)
{
   Ecore_X_Randr_Mode_Info *mode = NULL;
   Eina_List *l = NULL;

   /* are we skipping refresh rate check ? */
   if (!skip_refresh)
     {
        /* loop the modes */
        EINA_LIST_REVERSE_FOREACH(sd->modes, l, mode)
          {
             if ((((int)mode->width - RESIZE_FUZZ) <= w) || 
                 (((int)mode->width + RESIZE_FUZZ) <= w))
               {
                  if ((((int)mode->height - RESIZE_FUZZ) <= h) || 
                      (((int)mode->height + RESIZE_FUZZ) <= h))
                    {
                       /* TODO: Compare refresh rates */
                       return mode;
                    }
               }
          }
     }

   /* if we got here, then we found no mode which matches the current 
    * refresh rate and size. Search again, ignoring refresh rate */

   /* loop the modes */
   EINA_LIST_REVERSE_FOREACH(sd->modes, l, mode)
     {
        if ((((int)mode->width - RESIZE_FUZZ) <= w) || 
            (((int)mode->width + RESIZE_FUZZ) <= w))
          {
             if ((((int)mode->height - RESIZE_FUZZ) <= h) || 
                 (((int)mode->height + RESIZE_FUZZ) <= h))
               {
                  return mode;
               }
          }
     }

   return NULL;
}

static void 
_e_smart_monitor_thumb_cb_mouse_in(void *data EINA_UNUSED, Evas *evas EINA_UNUSED, Evas_Object *obj, void *event EINA_UNUSED)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* set the mouse pointer to indicate we can be clicked */
   _e_smart_monitor_pointer_push(obj, "hand");
}

static void 
_e_smart_monitor_thumb_cb_mouse_out(void *data EINA_UNUSED, Evas *evas EINA_UNUSED, Evas_Object *obj, void *event EINA_UNUSED)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* set the mouse pointer back to default */
   _e_smart_monitor_pointer_pop(obj, "hand");
}

static void 
_e_smart_monitor_thumb_cb_mouse_up(void *data EINA_UNUSED, Evas *evas EINA_UNUSED, Evas_Object *obj, void *event)
{
   Evas_Event_Mouse_Up *ev;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   ev = event;
   if (ev->button != 1) return;

   /* reset mouse pointer */
   _e_smart_monitor_pointer_pop(obj, "move");
}

static void 
_e_smart_monitor_thumb_cb_mouse_down(void *data EINA_UNUSED, Evas *evas EINA_UNUSED, Evas_Object *obj, void *event)
{
   Evas_Event_Mouse_Down *ev;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   ev = event;
   if (ev->button != 1) return;

   /* reset mouse pointer */
   _e_smart_monitor_pointer_push(obj, "move");
}

static void 
_e_smart_monitor_frame_cb_mouse_move(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event)
{
   Evas_Object *mon;
   E_Smart_Data *sd;

   /* try to get the monitor object */
   if (!(mon = data)) return;

   /* try to get the monitor smart data */
   if (!(sd = evas_object_smart_data_get(mon))) return;

   /* call appropriate function based on current action */
   if (sd->resizing) _e_smart_monitor_resize_event(sd, mon, event);
   if (sd->rotating) _e_smart_monitor_rotate_event(sd, mon, event);
}

static void 
_e_smart_monitor_frame_cb_resize_in(void *data EINA_UNUSED, Evas_Object *obj, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* set the mouse pointer to indicate we can be resized */
   _e_smart_monitor_pointer_push(obj, "resize_br");
}

static void 
_e_smart_monitor_frame_cb_resize_out(void *data EINA_UNUSED, Evas_Object *obj, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* set the mouse pointer back to default */
   _e_smart_monitor_pointer_pop(obj, "resize_br");
}

static void 
_e_smart_monitor_frame_cb_rotate_in(void *data EINA_UNUSED, Evas_Object *obj, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* set the mouse pointer to indicate we can be rotated */
   _e_smart_monitor_pointer_push(obj, "rotate");
}

static void 
_e_smart_monitor_frame_cb_rotate_out(void *data EINA_UNUSED, Evas_Object *obj, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* set the mouse pointer back to default */
   _e_smart_monitor_pointer_pop(obj, "rotate");
}

static void 
_e_smart_monitor_frame_cb_indicator_in(void *data EINA_UNUSED, Evas_Object *obj, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* set the mouse pointer to indicate we can be toggled */
   _e_smart_monitor_pointer_push(obj, "plus");
}

static void 
_e_smart_monitor_frame_cb_indicator_out(void *data EINA_UNUSED, Evas_Object *obj, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* set the mouse pointer back to default */
   _e_smart_monitor_pointer_pop(obj, "plus");
}

static void 
_e_smart_monitor_frame_cb_resize_start(void *data, Evas_Object *obj EINA_UNUSED, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   Evas_Object *mon;
   E_Smart_Data *sd;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the monitor object */
   if (!(mon = data)) return;

   /* try to get the monitor smart data */
   if (!(sd = evas_object_smart_data_get(mon))) return;

   /* record current position of mouse */
   evas_pointer_canvas_xy_get(sd->evas, &sd->rx, &sd->ry);

   /* record current size of monitor */
   evas_object_grid_pack_get(sd->grid.obj, mon, NULL, NULL, &sd->cw, &sd->ch);

   /* set resizing flag */
   sd->resizing = EINA_TRUE;
}

static void 
_e_smart_monitor_frame_cb_resize_stop(void *data, Evas_Object *obj EINA_UNUSED, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   Evas_Object *mon;
   E_Smart_Data *sd;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the monitor object */
   if (!(mon = data)) return;

   /* try to get the monitor smart data */
   if (!(sd = evas_object_smart_data_get(mon))) return;

   /* record current size of monitor */
   evas_object_grid_pack_get(sd->grid.obj, mon, NULL, NULL, &sd->cw, &sd->ch);

   /* set resizing flag */
   sd->resizing = EINA_FALSE;
}

static void 
_e_smart_monitor_frame_cb_rotate_start(void *data, Evas_Object *obj EINA_UNUSED, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   Evas_Object *mon;
   E_Smart_Data *sd;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the monitor object */
   if (!(mon = data)) return;

   /* try to get the monitor smart data */
   if (!(sd = evas_object_smart_data_get(mon))) return;

   /* record current size of monitor */
   /* evas_object_grid_pack_get(sd->grid.obj, mon, NULL, NULL, &sd->cw, &sd->ch); */

   /* set resizing flag */
   sd->rotating = EINA_TRUE;
}

static void 
_e_smart_monitor_frame_cb_rotate_stop(void *data, Evas_Object *obj EINA_UNUSED, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   Evas_Object *mon;
   E_Smart_Data *sd;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the monitor object */
   if (!(mon = data)) return;

   /* try to get the monitor smart data */
   if (!(sd = evas_object_smart_data_get(mon))) return;

   /* record current size of monitor */
   /* evas_object_grid_pack_get(sd->grid.obj, mon, NULL, NULL, &sd->cw, &sd->ch); */

   /* set resizing flag */
   sd->rotating = EINA_FALSE;
}

static void 
_e_smart_monitor_resize_event(E_Smart_Data *sd, Evas_Object *mon, void *event)
{
   Evas_Event_Mouse_Move *ev;
   Evas_Coord dx = 0, dy = 0;
   double mw = 0, mh = 0;
   double nw = 0, nh = 0;
   Ecore_X_Randr_Mode_Info *mode = NULL;

//   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   ev = event;

   /* check for valid mouse movement */
   if (((ev->cur.canvas.x - ev->prev.canvas.x) == 0) && 
       ((ev->cur.canvas.y - ev->prev.canvas.y) == 0))
     return;

   /* calculate difference in mouse movement */
   dx = (sd->rx - ev->cur.canvas.x);
   dy = (sd->ry - ev->cur.canvas.y);

   /* factor in drag resistance to measure movement */
   if (((dx * dx) + (dy * dy)) < 
       (e_config->drag_resist * e_config->drag_resist))
     return;

   dx = (ev->cur.canvas.x - ev->prev.canvas.x);
   dy = (ev->cur.canvas.y - ev->prev.canvas.y);
   if ((dx == 0) && (dy == 0)) return;

   /* convert monitor size to canvas size */
   _e_smart_monitor_coord_virtual_to_canvas(sd, sd->cw, sd->ch, &mw, &mh);

   /* factor in resize difference and convert to virtual */
   _e_smart_monitor_coord_canvas_to_virtual(sd, (mw + dx), (mh + dy), &nw, &nh);

   /* check new size against min & max modes */
   if (nw < sd->min.mode_width) nw = sd->min.mode_width;
   if (nw > sd->max.mode_width) nw = sd->max.mode_width;
   if (nh < sd->min.mode_height) nh = sd->min.mode_height;
   if (nh > sd->max.mode_height) nh = sd->max.mode_height;

   /* update current size values */
   sd->cw = nw;
   sd->ch = nh;

   /* try to find a mode that matches this new size */
   if ((mode = _e_smart_monitor_mode_find(sd, nw, nh, EINA_FALSE)))
     {
        /* update monitor size in the grid */
        evas_object_grid_pack(sd->grid.obj, mon, 
                              sd->cx, sd->cy, mode->width, mode->height);

        /* update resolution text */
        _e_smart_monitor_resolution_set(sd, mode->width, mode->height);
     }
}

static void 
_e_smart_monitor_rotate_event(E_Smart_Data *sd, Evas_Object *mon, void *event)
{
   Evas_Event_Mouse_Move *ev;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   ev = event;
}
