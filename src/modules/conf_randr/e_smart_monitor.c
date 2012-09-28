#include "e.h"
#include "e_mod_main.h"
#include "e_smart_monitor.h"

#define RESISTANCE_THRESHOLD 5
#define SNAP_FUZZINESS 80

/* local structures */
typedef struct _E_Smart_Data E_Smart_Data;
struct _E_Smart_Data
{
   /* object geometry */
   Evas_Coord x, y, w, h;

   /* visible flag */
   Eina_Bool visible : 1;

   /* changed flag */
   Eina_Bool changed : 1;

   /* resizing flag */
   Eina_Bool resizing : 1;

   /* snapped flag */
   Eina_Bool snapped : 1;

   /* rotating flag */
   Eina_Bool rotating : 1;

   /* connected flag */
   Eina_Bool connected : 1;

   /* layout object (this monitors parent) */
   Evas_Object *o_layout;

   /* base monitor object */
   Evas_Object *o_base;

   /* frame object */
   Evas_Object *o_frame;

   /* monitor stand object */
   Evas_Object *o_stand;

   /* livethumbnail for background image */
   Evas_Object *o_thumb;

   /* crtc information */
   E_Randr_Crtc_Info *crtc;

   /* list of event handlers */
   Eina_List *hdls;

   /* evas_map for rotation */
   Evas_Map *map;

   /* container number (for bg preview) */
   int con;

   /* zone number (for bg preview) */
   int zone;

   /* list of available modes */
   Eina_List *modes;

   /* min & max resolution */
   struct
     {
        int w, h;
     } min, max;
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
static Eina_Bool _e_smart_cb_bg_update(void *data, int type __UNUSED__, void *event);
static void _e_smart_cb_resize_mouse_in(void *data __UNUSED__, Evas_Object *obj, const char *emission __UNUSED__, const char *source __UNUSED__);
static void _e_smart_cb_resize_mouse_out(void *data __UNUSED__, Evas_Object *obj, const char *emission __UNUSED__, const char *source __UNUSED__);
static void _e_smart_cb_resize_start(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__);
static void _e_smart_cb_resize_stop(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__);
static void _e_smart_cb_rotate_mouse_in(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__);
static void _e_smart_cb_rotate_mouse_out(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__);
static void _e_smart_cb_rotate_start(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__);
static void _e_smart_cb_rotate_stop(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__);
static void _e_smart_cb_indicator_mouse_in(void *data __UNUSED__, Evas_Object *obj, const char *emission __UNUSED__, const char *source __UNUSED__);
static void _e_smart_cb_indicator_mouse_out(void *data __UNUSED__, Evas_Object *obj, const char *emission __UNUSED__, const char *source __UNUSED__);
static void _e_smart_cb_indicator_toggle(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__);
static void _e_smart_cb_frame_mouse_move(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event);
static int _e_smart_cb_modes_sort(const void *data1, const void *data2);

static void _e_smart_monitor_rotate(E_Smart_Data *sd, void *event);
static void _e_smart_monitor_resize(E_Smart_Data *sd, Evas_Object *mon, void *event);
static void _e_smart_monitor_snap(Evas_Object *obj, Ecore_X_Randr_Mode_Info *mode);
static Ecore_X_Randr_Mode_Info *_e_smart_monitor_resolution_get(E_Smart_Data *sd, Evas_Coord width, Evas_Coord height);

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

   if (!smart)
     if (!(smart = evas_smart_class_new(&sc))) 
       return NULL;

   return evas_object_smart_add(evas, smart);
}

void 
e_smart_monitor_layout_set(Evas_Object *obj, Evas_Object *layout)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj)))
     return;
   sd->o_layout = layout;
}

void 
e_smart_monitor_crtc_set(Evas_Object *obj, E_Randr_Crtc_Info *crtc)
{
   E_Smart_Data *sd;
   Evas_Object *o;
   const char *bg = NULL;
   E_Container *con;
   E_Desk *desk;
   E_Zone *zone;
   Evas_Coord w, h;
   Eina_List *l;
   E_Randr_Output_Info *output;

   if (!(sd = evas_object_smart_data_get(obj)))
     return;

   sd->crtc = crtc;
   if (!crtc) return;

   EINA_LIST_FOREACH(crtc->outputs, l, output)
     {
        Eina_List *modes = NULL, *m = NULL;
        Ecore_X_Randr_Mode_Info *mode = NULL;
        E_Randr_Monitor_Info *monitor = NULL;
        const char *name = NULL;

        printf("Output: %d %s\n", output->xid, output->name);

        if (output->crtc)
          modes = output->crtc->outputs_common_modes;
        else if (output->monitor)
          modes = output->monitor->modes;

        /* grab a copy of this monitor's modes, 
         * filtering out duplicate resolutions */
        EINA_LIST_FOREACH(modes, m, mode)
          {
             Ecore_X_Randr_Mode_Info *nmode = NULL;

             if ((nmode = eina_list_data_get(m->next)))
               {
                  if (!strcmp(mode->name, nmode->name))
                    continue;
               }

             sd->modes = eina_list_append(sd->modes, mode);
          }

        /* sort the mode list */
        sd->modes = eina_list_sort(sd->modes, 0, _e_smart_cb_modes_sort);

        /* NB: This is just a development printf to list modes.
         * Remove when dialog is complete */
        EINA_LIST_FOREACH(sd->modes, m, mode)
          {
             double rate = 0.0;

             if ((mode->hTotal) && (mode->vTotal))
               rate = ((float)mode->dotClock / 
                       ((float)mode->hTotal * (float)mode->vTotal));

             printf("\tMode: %d %dx%d @ %.1fHz\n", mode->xid, 
                    mode->width, mode->height, rate);
          }

        /* get the min resolution for this monitor */
        mode = eina_list_nth(sd->modes, 0);
        sd->min.w = mode->width;
        sd->min.h = mode->height;

        /* get the max resolution for this monitor */
        mode = eina_list_data_get(eina_list_last(sd->modes));
        sd->max.w = mode->width;
        sd->max.h = mode->height;

        /* tell monitor object we are enabled/disabled */
        if (output->connection_status == 
            ECORE_X_RANDR_CONNECTION_STATUS_CONNECTED)
          {
             sd->connected = EINA_TRUE;
             edje_object_signal_emit(sd->o_base, "e,state,enabled", "e");
             edje_object_signal_emit(sd->o_frame, "e,state,enabled", "e");
          }
        else
          {
             sd->connected = EINA_FALSE;
             edje_object_signal_emit(sd->o_base, "e,state,disabled", "e");
             edje_object_signal_emit(sd->o_frame, "e,state,disabled", "e");
          }

        /* get and display monitor name if available */
        if ((monitor = output->monitor))
          {
             if (monitor->edid)
               {
                  name = 
                    ecore_x_randr_edid_display_name_get(monitor->edid, 
                                                        monitor->edid_length);
               }
          }

        if (!name) name = output->name;
        edje_object_part_text_set(sd->o_frame, "e.text.name", name);
     }

   /* get which desk this is based on monitor geometry */
   con = e_container_current_get(e_manager_current_get());
   zone = 
     e_container_zone_at_point_get(con, crtc->geometry.x, crtc->geometry.y);
   desk = e_desk_at_xy_get(zone, crtc->geometry.x, crtc->geometry.y);
   if (!desk) desk = e_desk_current_get(zone);

   sd->con = con->num;
   sd->zone = zone->num;

   /* get bg file for this screen */
   bg = e_bg_file_get(con->num, zone->num, desk->x, desk->y);

   w = zone->w;
   h = (w * zone->h) / zone->w;
   e_livethumb_vsize_set(sd->o_thumb, w, h);

   o = e_livethumb_thumb_get(sd->o_thumb);
   if (!o) o = edje_object_add(e_livethumb_evas_get(sd->o_thumb));
   edje_object_file_set(o, bg, "e/desktop/background");
   e_livethumb_thumb_set(sd->o_thumb, o);
}

E_Randr_Crtc_Info *
e_smart_monitor_crtc_get(Evas_Object *obj)
{
   E_Smart_Data *sd;

   if (!obj) return NULL;

   if (!(sd = evas_object_smart_data_get(obj)))
     return NULL;

   return sd->crtc;
}

void 
e_smart_monitor_crtc_geometry_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y, Evas_Coord *w, Evas_Coord *h)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj)))
     return;

   if (x) *x = 0;
   if (y) *y = 0;
   if (w) *w = 0;
   if (h) *h = 0;

   if (!sd->crtc) return;

   if (sd->crtc)
     {
        if (x) *x = sd->crtc->geometry.x;
        if (y) *y = sd->crtc->geometry.y;
        if (w) *w = sd->crtc->geometry.w;
        if (h) *h = sd->crtc->geometry.h;
     }
   /* else */
   /*   { */
   /*      if (sd->crtc->monitor) */
   /*        { */
   /*           if (w) *w = sd->crtc->monitor->size_mm.width; */
   /*           if (h) *h = sd->crtc->monitor->size_mm.height; */
   /*        } */
   /*   } */
}

/* local functions */
static void 
_e_smart_add(Evas_Object *obj)
{
   E_Smart_Data *sd;
   Evas *evas;
   /* Evas_Object *o_resize; */

   if (!(sd = calloc(1, sizeof(E_Smart_Data)))) return;

   /* retrieve canvas once for reuse */
   evas = evas_object_evas_get(obj);

   /* create base object */
   sd->o_base = edje_object_add(evas);
   e_theme_edje_object_set(sd->o_base, "base/theme/widgets", 
                           "e/conf/randr/main/monitor");
   evas_object_smart_member_add(sd->o_base, obj);

   /* create monitor frame with preview */
   sd->o_frame = edje_object_add(evas);
   e_theme_edje_object_set(sd->o_frame, "base/theme/widgets", 
                           "e/conf/randr/main/frame");
   edje_object_part_swallow(sd->o_base, "e.swallow.frame", sd->o_frame);
   evas_object_smart_member_add(sd->o_frame, obj);
   evas_object_event_callback_add(sd->o_frame, EVAS_CALLBACK_MOUSE_MOVE, 
                                  _e_smart_cb_frame_mouse_move, obj);

   /* create bg preview */
   sd->o_thumb = e_livethumb_add(evas);
   edje_object_part_swallow(sd->o_frame, "e.swallow.preview", sd->o_thumb);
   evas_object_show(sd->o_thumb);

   /* create monitor stand */
   sd->o_stand = edje_object_add(evas);
   e_theme_edje_object_set(sd->o_stand, "base/theme/widgets", 
                           "e/conf/randr/main/stand");
   evas_object_smart_member_add(sd->o_stand, obj);
   edje_object_part_swallow(sd->o_base, "e.swallow.stand", sd->o_stand);
   evas_object_stack_below(sd->o_stand, sd->o_frame);

   /* add callbacks for 'resize' edje signals */
   edje_object_signal_callback_add(sd->o_frame, "e,action,resize,in", "e", 
                                   _e_smart_cb_resize_mouse_in, NULL);
   edje_object_signal_callback_add(sd->o_frame, "e,action,resize,out", "e", 
                                   _e_smart_cb_resize_mouse_out, NULL);
   edje_object_signal_callback_add(sd->o_frame, "e,action,resize,start", "e", 
                                   _e_smart_cb_resize_start, obj);
   edje_object_signal_callback_add(sd->o_frame, "e,action,resize,stop", "e", 
                                   _e_smart_cb_resize_stop, obj);

   /* add callbacks for 'rotate' edje signals */
   edje_object_signal_callback_add(sd->o_frame, "e,action,rotate,in", "e", 
                                   _e_smart_cb_rotate_mouse_in, sd);
   edje_object_signal_callback_add(sd->o_frame, "e,action,rotate,out", "e", 
                                   _e_smart_cb_rotate_mouse_out, sd);
   edje_object_signal_callback_add(sd->o_frame, "e,action,rotate,start", "e", 
                                   _e_smart_cb_rotate_start, obj);
   edje_object_signal_callback_add(sd->o_frame, "e,action,rotate,stop", "e", 
                                   _e_smart_cb_rotate_stop, obj);

   /* add callback for indicator edje signals */
   edje_object_signal_callback_add(sd->o_frame, "e,action,indicator,in", "e", 
                                   _e_smart_cb_indicator_mouse_in, NULL);
   edje_object_signal_callback_add(sd->o_frame, "e,action,indicator,out", "e", 
                                   _e_smart_cb_indicator_mouse_out, NULL);
   edje_object_signal_callback_add(sd->o_frame, 
                                   "e,action,indicator,toggle", "e", 
                                   _e_smart_cb_indicator_toggle, sd);

   /* create event handlers */
   sd->hdls = 
     eina_list_append(sd->hdls, 
                      ecore_event_handler_add(E_EVENT_BG_UPDATE, 
                                              _e_smart_cb_bg_update, sd));

   evas_object_smart_data_set(obj, sd);
}

static void 
_e_smart_del(Evas_Object *obj)
{
   E_Smart_Data *sd;
   Ecore_Event_Handler *hdl;

   if (!(sd = evas_object_smart_data_get(obj)))
     return;

   /* delete callbacks for 'resize' edje signals */
   edje_object_signal_callback_del(sd->o_frame, "e,action,resize,in", "e", 
                                   _e_smart_cb_resize_mouse_in);
   edje_object_signal_callback_del(sd->o_frame, "e,action,resize,out", "e", 
                                   _e_smart_cb_resize_mouse_out);
   edje_object_signal_callback_del(sd->o_frame, "e,action,resize,start", "e", 
                                   _e_smart_cb_resize_start);
   edje_object_signal_callback_del(sd->o_frame, "e,action,resize,stop", "e", 
                                   _e_smart_cb_resize_stop);

   /* delete callbacks for 'rotate' edje signals */
   edje_object_signal_callback_del(sd->o_frame, "e,action,rotate,in", "e", 
                                   _e_smart_cb_rotate_mouse_in);
   edje_object_signal_callback_del(sd->o_frame, "e,action,rotate,out", "e", 
                                   _e_smart_cb_rotate_mouse_out);
   edje_object_signal_callback_del(sd->o_frame, "e,action,rotate,start", "e", 
                                   _e_smart_cb_rotate_start);
   edje_object_signal_callback_del(sd->o_frame, "e,action,rotate,stop", "e", 
                                   _e_smart_cb_rotate_stop);

   /* delete callback for indicator edje signals */
   edje_object_signal_callback_del(sd->o_frame, "e,action,indicator,in", "e", 
                                   _e_smart_cb_indicator_mouse_in);
   edje_object_signal_callback_del(sd->o_frame, "e,action,indicator,out", "e", 
                                   _e_smart_cb_indicator_mouse_out);
   edje_object_signal_callback_del(sd->o_frame, 
                                   "e,action,indicator,toggle", "e", 
                                   _e_smart_cb_indicator_toggle);

   /* delete event handlers */
   EINA_LIST_FREE(sd->hdls, hdl)
     ecore_event_handler_del(hdl);

   /* delete the list of modes */
   if (sd->modes) eina_list_free(sd->modes);

   /* delete the monitor objects */
   if (sd->o_stand) evas_object_del(sd->o_stand);
   if (sd->o_thumb) evas_object_del(sd->o_thumb);
   if (sd->o_frame) evas_object_del(sd->o_frame);
   if (sd->o_base) evas_object_del(sd->o_base);

   E_FREE(sd);
   evas_object_smart_data_set(obj, NULL);
}

static void 
_e_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj)))
     return;

   if ((sd->x == x) && (sd->y == y)) return;
   sd->x = x;
   sd->y = y;
   evas_object_move(sd->o_base, x, y);
}

static void 
_e_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj)))
     return;

   if ((sd->w == w) && (sd->h == h)) return;
   sd->w = w;
   sd->h = h;
   evas_object_resize(sd->o_base, w, h);
}

static void 
_e_smart_show(Evas_Object *obj)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj)))
     return;

   if (sd->visible) return;
   evas_object_show(sd->o_base);
   sd->visible = EINA_TRUE;
}

static void 
_e_smart_hide(Evas_Object *obj)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj)))
     return;

   if (!sd->visible) return;
   evas_object_hide(sd->o_base);
   sd->visible = EINA_FALSE;
}

static void 
_e_smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj)))
     return;

   evas_object_clip_set(sd->o_stand, clip);
   evas_object_clip_set(sd->o_frame, clip);
   evas_object_clip_set(sd->o_base, clip);
}

static void 
_e_smart_clip_unset(Evas_Object *obj)
{
   E_Smart_Data *sd;

   if (!(sd = evas_object_smart_data_get(obj)))
     return;

   evas_object_clip_unset(sd->o_stand);
   evas_object_clip_unset(sd->o_frame);
   evas_object_clip_unset(sd->o_base);
}

static Eina_Bool 
_e_smart_cb_bg_update(void *data, int type, void *event)
{
   E_Smart_Data *sd;
   E_Event_Bg_Update *ev;

   if (type != E_EVENT_BG_UPDATE) return ECORE_CALLBACK_PASS_ON;
   if (!(sd = data)) return ECORE_CALLBACK_PASS_ON;
   if (!sd->crtc) return ECORE_CALLBACK_PASS_ON;
   ev = event;

   if (((ev->container < 0) || (sd->con == ev->container)) && 
       ((ev->zone < 0) || (sd->zone == ev->zone)))
     {
        if (((ev->desk_x < 0) || (sd->crtc->geometry.x == ev->desk_x)) && 
            ((ev->desk_y < 0) || (sd->crtc->geometry.y == ev->desk_y)))
          {
             Evas_Object *o;
             const char *bg = NULL;

             /* background changed. grab new bg file and set thumbnail */
             bg = e_bg_file_get(sd->con, sd->zone, ev->desk_x, ev->desk_y);

             o = e_livethumb_thumb_get(sd->o_thumb);
             if (!o) o = edje_object_add(e_livethumb_evas_get(sd->o_thumb));
             edje_object_file_set(o, bg, "e/desktop/background");
             e_livethumb_thumb_set(sd->o_thumb, o);
          }
     }

   return ECORE_CALLBACK_PASS_ON;
}

static void 
_e_smart_cb_resize_mouse_in(void *data __UNUSED__, Evas_Object *obj, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   E_Manager *man;

   man = e_manager_current_get();
   e_pointer_type_push(man->pointer, obj, "resize_br");
}

static void 
_e_smart_cb_resize_mouse_out(void *data __UNUSED__, Evas_Object *obj, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   E_Manager *man;

   man = e_manager_current_get();
   e_pointer_type_pop(man->pointer, obj, "resize_br");
}

static void 
_e_smart_cb_resize_start(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Evas_Object *mon;
   E_Smart_Data *sd;

   if (!(mon = data)) return;
   if (!(sd = evas_object_smart_data_get(mon))) return;
   sd->resizing = EINA_TRUE;
   e_layout_child_raise(mon);
}

static void 
_e_smart_cb_resize_stop(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Evas_Object *mon;
   E_Smart_Data *sd;
   Evas_Coord ow, oh;
   Evas_Coord nrw, nrh;
   Ecore_X_Randr_Mode_Info *mode;

   if (!(mon = data)) return;
   if (!(sd = evas_object_smart_data_get(mon))) return;
   sd->resizing = EINA_FALSE;

   /* get the object geometry and convert to virtual space */
   evas_object_geometry_get(mon, NULL, NULL, &ow, &oh);
   e_layout_coord_canvas_to_virtual(sd->o_layout, ow, oh, &nrw, &nrh);

   /* find the closest resolution to this one and snap to it */
   if ((mode = _e_smart_monitor_resolution_get(sd, nrw, nrh)))
     _e_smart_monitor_snap(mon, mode);

   e_layout_child_lower(mon);
}

static void 
_e_smart_cb_rotate_mouse_in(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
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
_e_smart_cb_rotate_mouse_out(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
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
_e_smart_cb_rotate_start(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Evas_Object *mon;
   E_Smart_Data *sd;

   if (!(mon = data)) return;
   if (!(sd = evas_object_smart_data_get(mon))) return;

   sd->rotating = EINA_TRUE;

   e_layout_child_raise(mon);

   /* NB: Dont' free these maps yet.
    * 
    * They will be needed to handle Resize While Rotated */

   /* create frame 'map' for rotation */
   sd->map = evas_map_new(4);
   evas_map_smooth_set(sd->map, EINA_TRUE);
   evas_map_alpha_set(sd->map, EINA_TRUE);
   evas_map_util_points_populate_from_object(sd->map, sd->o_frame);
   evas_object_map_set(sd->o_frame, sd->map);
   evas_object_map_enable_set(sd->o_frame, EINA_TRUE);
}

static void 
_e_smart_cb_rotate_stop(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Evas_Object *mon;
   E_Smart_Data *sd;

   if (!(mon = data)) return;
   if (!(sd = evas_object_smart_data_get(mon))) return;

   sd->rotating = EINA_FALSE;

   e_layout_child_lower(mon);

   if (sd->map) evas_map_free(sd->map);
}

static void 
_e_smart_cb_indicator_mouse_in(void *data __UNUSED__, Evas_Object *obj, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   E_Manager *man;

   man = e_manager_current_get();
   e_pointer_type_push(man->pointer, obj, "hand");
}

static void 
_e_smart_cb_indicator_mouse_out(void *data __UNUSED__, Evas_Object *obj, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   E_Manager *man;

   man = e_manager_current_get();
   e_pointer_type_pop(man->pointer, obj, "hand");
}

static void 
_e_smart_cb_indicator_toggle(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   E_Smart_Data *sd;

   if (!(sd = data)) return;

   if (sd->connected)
     {
        sd->connected = EINA_FALSE;
        edje_object_signal_emit(sd->o_base, "e,state,disabled", "e");
        edje_object_signal_emit(sd->o_frame, "e,state,disabled", "e");
     }
   else
     {
        sd->connected = EINA_TRUE;
        edje_object_signal_emit(sd->o_base, "e,state,enabled", "e");
        edje_object_signal_emit(sd->o_frame, "e,state,enabled", "e");
     }
}

static void 
_e_smart_cb_frame_mouse_move(void *data, Evas *evas __UNUSED__, Evas_Object *obj __UNUSED__, void *event)
{
   Evas_Object *mon;
   E_Smart_Data *sd;

   if (!(mon = data)) return;
   if (!(sd = evas_object_smart_data_get(mon))) return;

   /* if we are not rotating or resizing, then we have nothing to do */
   if ((!sd->rotating) && (!sd->resizing)) return;

   if (sd->rotating) 
     _e_smart_monitor_rotate(sd, event);
   else if (sd->resizing) 
     _e_smart_monitor_resize(sd, mon, event);
}

static int 
_e_smart_cb_modes_sort(const void *data1, const void *data2)
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
_e_smart_monitor_rotate(E_Smart_Data *sd, void *event)
{
   Evas_Event_Mouse_Move *ev;
   Evas_Coord mx;
   Evas_Coord x, y, w, h;
   const Evas_Map *m;
   Evas_Map *m2;

   ev = event;
   mx = (ev->prev.output.x - ev->cur.output.x);

   /* update map for frame object to reflect this rotation */
   evas_object_geometry_get(sd->o_frame, &x, &y, &w, &h);
   m = evas_object_map_get(sd->o_frame);
   m2 = evas_map_dup(m);
   evas_map_util_rotate(m2, mx, x + (w / 2), y + (h / 2));
   evas_object_map_set(sd->o_frame, m2);
   evas_map_free(m2);
}

static void 
_e_smart_monitor_resize(E_Smart_Data *sd, Evas_Object *mon, void *event)
{
   Evas_Event_Mouse_Move *ev;
   Evas_Coord w, h;
   Evas_Coord mx, my;
   Evas_Coord nrw, nrh;

   ev = event;

   /* FIXME: Ideally, this should use the evas_map to get geometry 
    * (for cases where the user has rotated the monitor and is now resizing) */

   /* grab size of monitor object */
   evas_object_geometry_get(mon, NULL, NULL, &w, &h);

   /* calculate resize difference */
   mx = (ev->cur.output.x - ev->prev.output.x);
   my = (ev->cur.output.y - ev->prev.output.y);

   /* determine what resolution this geometry would be */
   e_layout_coord_canvas_to_virtual(sd->o_layout, 
                                    (w + mx), (h + my), &nrw, &nrh);

   /* determine if this new size is below or above the 
    * available resolutions and stop resizing if so */
   if ((nrw < sd->min.w) || (nrh < sd->min.h)) return;
   if ((nrw > sd->max.w) || (nrh > sd->max.h)) return;

   /* graphically resize the monitor */
   evas_object_resize(mon, w + mx, h + my);

   /* tell randr widget we resized this monitor so that it can 
    * update the layout for any monitors around this one */
   evas_object_smart_callback_call(mon, "monitor_resized", NULL);
}

static void 
_e_smart_monitor_snap(Evas_Object *obj, Ecore_X_Randr_Mode_Info *mode)
{
   E_Smart_Data *sd;
   Evas_Coord nw, nh;

   if (!(sd = evas_object_smart_data_get(obj))) return;
   sd->snapped = EINA_TRUE;

   /* get the new canvas size */
   e_layout_coord_virtual_to_canvas(sd->o_layout, 
                                    mode->width, mode->height, &nw, &nh);

   /* graphically resize the monitor */
   evas_object_resize(obj, nw, nh);

   /* tell randr widget we resized this monitor so that it can 
    * update the layout for any monitors around this one */
   evas_object_smart_callback_call(obj, "monitor_resized", NULL);
}

static Ecore_X_Randr_Mode_Info *
_e_smart_monitor_resolution_get(E_Smart_Data *sd, Evas_Coord width, Evas_Coord height)
{
   Ecore_X_Randr_Mode_Info *mode;
   Eina_List *l;

   if (!sd) return NULL;

   EINA_LIST_REVERSE_FOREACH(sd->modes, l, mode)
     {
        if ((((int)mode->width - SNAP_FUZZINESS) <= width) || 
            (((int)mode->width + SNAP_FUZZINESS) <= width))
          {
             if ((((int)mode->height - SNAP_FUZZINESS) <= height) || 
                 (((int)mode->height + SNAP_FUZZINESS) <= height))
               return mode;
          }
     }

   return NULL;
}
