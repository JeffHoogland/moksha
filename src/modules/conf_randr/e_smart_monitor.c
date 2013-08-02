#include "e.h"
#include "e_mod_main.h"
#include "e_smart_monitor.h"

#define RESIZE_FUZZ 80
#define ROTATE_FUZZ 45

//#define BG_DBG 1

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

#ifdef BG_DBG
   /* test object */
   Evas_Object *o_bg;
#endif

   /* base object */
   Evas_Object *o_base;

   /* frame object */
   Evas_Object *o_frame;

   /* stand object */
   Evas_Object *o_stand;

   /* background thumbnail */
   Evas_Object *o_thumb;

   /* refresh rate object */
   Evas_Object *o_refresh;

   struct 
     {
        Ecore_X_Randr_Crtc id;
        Evas_Coord x, y, w, h;
        Ecore_X_Randr_Orientation orient;
        Ecore_X_Randr_Mode mode;
        double refresh_rate;
        Eina_Bool enabled : 1;
     } crtc;

   /* output config */
   Ecore_X_Randr_Output output;
   Eina_Bool primary : 1;

   struct 
     {
        /* reference to the grid we are packed into */
        Evas_Object *obj;

        Evas_Coord x, y, w, h;

        /* virtual size of the grid */
        Evas_Coord vw, vh;
     } grid;

   /* container number */
   unsigned int con_num;

   /* zone number */
   unsigned int zone_num;

   /* event handler for background image updates */
   Ecore_Event_Handler *bg_update_hdl;

   /* list of modes */
   Eina_List *modes;

   /* coordinates where the user clicked to start resizing */
   Evas_Coord rx, ry;

   /* coordinates where the user clicked to start moving */
   Evas_Coord mx, my;

   struct 
     {
        /* current geometry */
        Evas_Coord x, y, w, h;

        /* current orientation */
        Ecore_X_Randr_Orientation orient;

        /* current mode */
        Ecore_X_Randr_Mode mode;

        /* current rotation */
        int rotation;

        /* current refresh rate */
        int refresh_rate;

        /* current enabled */
        Eina_Bool enabled : 1;
     } current;

   struct 
     {
        Evas_Coord x, y, w, h;
     } prev;

   /* visibility flag */
   Eina_Bool visible : 1;

   /* resizing flag */
   Eina_Bool resizing : 1;

   /* rotating flag */
   Eina_Bool rotating : 1;

   /* moving flag */
   Eina_Bool moving : 1;

   /* cloned flag */
   Eina_Bool cloned : 1;

   /* parent monitor we are cloned into */
   Evas_Object *parent;

   /* mini representation when cloned */
   Evas_Object *o_clone;

   /* record what changed */
   E_Smart_Monitor_Changes changes;
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

static inline void _e_smart_monitor_coord_virtual_to_canvas(E_Smart_Data *sd, Evas_Coord vx, Evas_Coord vy, Evas_Coord *cx, Evas_Coord *cy);
static inline void _e_smart_monitor_coord_canvas_to_virtual(E_Smart_Data *sd, Evas_Coord cx, Evas_Coord cy, Evas_Coord *vx, Evas_Coord *vy);
static Ecore_X_Randr_Mode_Info *_e_smart_monitor_mode_find(E_Smart_Data *sd, Evas_Coord w, Evas_Coord h, Eina_Bool skip_refresh);
static inline double _e_smart_monitor_mode_refresh_rate_get(Ecore_X_Randr_Mode_Info *mode);
static void _e_smart_monitor_mode_refresh_rates_fill(Evas_Object *obj);

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
static void _e_smart_monitor_frame_cb_indicator_toggle(void *data, Evas_Object *obj EINA_UNUSED, const char *emission EINA_UNUSED, const char *source EINA_UNUSED);

static void _e_smart_monitor_refresh_rate_cb_changed(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED);

static void _e_smart_monitor_resize_event(E_Smart_Data *sd, Evas_Object *mon, void *event);
static void _e_smart_monitor_rotate_event(E_Smart_Data *sd, Evas_Object *mon EINA_UNUSED, void *event);
static void _e_smart_monitor_move_event(E_Smart_Data *sd, Evas_Object *mon, void *event);

static int _e_smart_monitor_rotation_amount_get(E_Smart_Data *sd, Evas_Event_Mouse_Move *ev);
static inline int _e_smart_monitor_rotation_get(Ecore_X_Randr_Orientation orient);
static inline Ecore_X_Randr_Orientation _e_smart_monitor_orientation_get(int rotation);

static void _e_smart_monitor_frame_map_apply(Evas_Object *o_frame, int rotation);
static void _e_smart_monitor_thumb_map_apply(Evas_Object *o_thumb, int rotation);

static Ecore_X_Randr_Crtc _e_smart_monitor_crtc_find(Ecore_X_Randr_Output output);

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
   Ecore_X_Randr_Orientation orients = ECORE_X_RANDR_ORIENTATION_ROT_0;
   Ecore_X_Window root = 0;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* set the crtc config */
   sd->crtc.id = crtc;

   /* record the crtc geometry */
   sd->crtc.x = cx;
   sd->crtc.y = cy;
   sd->crtc.w = cw;
   sd->crtc.h = ch;

   sd->current.x = cx;
   sd->current.y = cy;
   sd->current.w = cw;
   sd->current.h = ch;

   /* get the root window */
   root = ecore_x_window_root_first_get();

   Ecore_X_Randr_Crtc_Info *crtc_info;

   if ((crtc_info = ecore_x_randr_crtc_info_get(root, crtc)))
     {
        /* get current orientation */
        sd->crtc.orient = crtc_info->rotation;

        /* get possible orientations for this crtc */
        orients = crtc_info->rotations;

        /* check if orientation is possible and disable if not */
        if (orients <= ECORE_X_RANDR_ORIENTATION_ROT_0)
          edje_object_signal_emit(sd->o_frame, "e,state,rotate,disabled", "e");

        /* get current mode */
        sd->crtc.mode = crtc_info->mode;

        /* free any memory allocated from ecore_x_randr */
        ecore_x_randr_crtc_info_free(crtc_info);
     }

   /* check crtc current mode to determine if enabled */
   if (sd->crtc.mode != 0)
     {
        Ecore_X_Randr_Mode_Info *mode;

        /* try to get current refresh rate for this mode */
        if ((mode = ecore_x_randr_mode_info_get(root, sd->crtc.mode)))
          {
             /* record current refresh rate */
             sd->crtc.refresh_rate = 
               _e_smart_monitor_mode_refresh_rate_get(mode);

             /* free any memory allocated from ecore_x_randr */
             free(mode);
          }
     }

   /* default refresh rate to 60 if not set */
   if (!sd->crtc.refresh_rate) sd->crtc.refresh_rate = 60.0;

   /* fill in current values */
   sd->current.mode = sd->crtc.mode;
   sd->current.orient = sd->crtc.orient;

   sd->crtc.enabled = sd->current.enabled = 
     ((sd->crtc.mode != 0) ? EINA_TRUE : EINA_FALSE);
   if (!sd->current.enabled)
     edje_object_signal_emit(sd->o_frame, "e,state,disabled", "e");

   /* get the degree of rotation */
   sd->current.rotation = _e_smart_monitor_rotation_get(sd->current.orient);

   /* record starting refresh rate */
   sd->current.refresh_rate = (int)sd->crtc.refresh_rate;
}

Ecore_X_Randr_Crtc 
e_smart_monitor_crtc_get(Evas_Object *obj)
{
   E_Smart_Data *sd;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return 0;

   return sd->crtc.id;
}

void 
e_smart_monitor_output_set(Evas_Object *obj, Ecore_X_Randr_Output output)
{
   E_Smart_Data *sd;
   Ecore_X_Randr_Mode_Info *mode;
   Ecore_X_Window root = 0;
   Ecore_X_Randr_Output primary = 0;
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

   /* get the primary output */
   primary = ecore_x_randr_primary_output_get(root);

   /* get output name */
   if (!(name = ecore_x_randr_output_name_get(root, sd->output, NULL)))
     {
        unsigned char *edid = NULL;
        unsigned long edid_length = 0;

        /* get the edid for this output */
        if ((edid = 
             ecore_x_randr_output_edid_get(root, sd->output, &edid_length)))
          {
             /* get output name */
             name = ecore_x_randr_edid_display_name_get(edid, edid_length);

             /* free any memory allocated from ecore_x_randr */
             free(edid);
          }
     }

   /* set if it's primary */
   sd->primary = (output == primary);
   if (sd->primary)
     edje_object_signal_emit(sd->o_frame, "e,state,primary,on", "e"); 
   else
     edje_object_signal_emit(sd->o_frame, "e,state,primary,off", "e");

   /* set monitor name */
   edje_object_part_text_set(sd->o_frame, "e.text.name", name);

   /* free any memory allocated from ecore_x_randr */
   free(name);

   /* get the smallest mode */
   mode = eina_list_nth(sd->modes, 0);
   sd->min.mode_width = mode->width;
   sd->min.mode_height = mode->height;

   /* fill in the refresh rate list
    * 
    * NB: This needs to be done After crtc_set has been called */
   _e_smart_monitor_mode_refresh_rates_fill(obj);
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

   /* set monitor position text */
   _e_smart_monitor_position_set(sd, sd->current.x, sd->current.y);

   evas_object_grid_pack(sd->grid.obj, obj, sd->current.x, 
                         sd->current.y, sd->current.w, sd->current.h);
}

void 
e_smart_monitor_grid_virtual_size_set(Evas_Object *obj, Evas_Coord vw, Evas_Coord vh)
{
   E_Smart_Data *sd;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   sd->grid.vw = vw;
   sd->grid.vh = vh;
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
   if (!(desk = e_desk_at_xy_get(zone, sd->crtc.x, sd->crtc.y)))
     desk = e_desk_current_get(zone);

   /* set the background image */
   _e_smart_monitor_background_set(sd, desk->x, desk->y);
}

void 
e_smart_monitor_current_geometry_set(Evas_Object *obj, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h)
{
   E_Smart_Data *sd;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   sd->current.x = x;
   sd->current.y = y;
   sd->current.w = w;
   sd->current.h = h;

   /* set monitor position text */
   _e_smart_monitor_position_set(sd, x, y);

   /* set monitor resolution text */
   _e_smart_monitor_resolution_set(sd, w, h);

   evas_object_grid_pack(sd->grid.obj, obj, x, y, w, h);
}

void 
e_smart_monitor_current_geometry_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y, Evas_Coord *w, Evas_Coord *h)
{
   E_Smart_Data *sd;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   if (x) *x = sd->current.x;
   if (y) *y = sd->current.y;
   if (w) *w = sd->current.w;
   if (h) *h = sd->current.h;
}

void 
e_smart_monitor_previous_geometry_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y, Evas_Coord *w, Evas_Coord *h)
{
   E_Smart_Data *sd;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   if (x) *x = sd->prev.x;
   if (y) *y = sd->prev.y;
   if (w) *w = sd->prev.w;
   if (h) *h = sd->prev.h;
}

void 
e_smart_monitor_clone_set(Evas_Object *obj, Evas_Object *parent)
{
   E_Smart_Data *sd, *psd = NULL;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* try to get the objects smart data */
   if (parent)
     if (!(psd = evas_object_smart_data_get(parent))) return;

   sd->cloned = ((parent != NULL) ? EINA_TRUE : EINA_FALSE);
   sd->parent = parent;

   if (sd->cloned)
     {
        Evas_Coord fw = 0, fh = 0;
        Evas_Object *box;

        evas_object_hide(obj);

        /* check if parent is larger */
        if ((psd->current.w > sd->current.w) || 
            (psd->current.h > sd->current.h))
          {
             /* NB: X RandR does not allow clones of different size or mode.
              * They both must match. Because of that, if the parent is 
              * larger, then we need to resize the parent down and set the mode */

             /* set the parent mode to this mode */
             if ((psd->current.mode != sd->current.mode) || 
                 (psd->current.refresh_rate != sd->current.refresh_rate) || 
                 (psd->current.w != sd->current.w) || 
                 (psd->current.h != sd->current.h))
               {
                  psd->current.w = sd->current.w;
                  psd->current.h = sd->current.h;
                  psd->current.refresh_rate = sd->current.refresh_rate;
                  psd->current.mode = sd->current.mode;
                  psd->changes |= E_SMART_MONITOR_CHANGED_MODE;
               }

             if (psd->current.orient != sd->current.orient)
               {
                  psd->current.orient = sd->current.orient;
                  psd->changes |= E_SMART_MONITOR_CHANGED_ORIENTATION;
               }

             _e_smart_monitor_resolution_set(psd, psd->current.w, psd->current.h);

             evas_object_grid_pack(psd->grid.obj, parent, 
                                   psd->current.x, psd->current.y, 
                                   psd->current.w, psd->current.h);
          }
        else if ((sd->current.w > psd->current.w) || 
                 (sd->current.h > psd->current.h))
          {
             if ((sd->current.mode != psd->current.mode) || 
                 (sd->current.refresh_rate != psd->current.refresh_rate) || 
                 (sd->current.w != psd->current.w) || 
                 (sd->current.h != psd->current.h))
               {
                  sd->current.w = psd->current.w;
                  sd->current.h = psd->current.h;
                  sd->current.refresh_rate = psd->current.refresh_rate;
                  sd->current.mode = psd->current.mode;
                  sd->changes |= E_SMART_MONITOR_CHANGED_MODE;
               }

             if (sd->current.orient != psd->current.orient)
               {
                  sd->current.orient = psd->current.orient;
                  sd->changes |= E_SMART_MONITOR_CHANGED_ORIENTATION;
               }

             _e_smart_monitor_resolution_set(sd, sd->current.w, sd->current.h);

             evas_object_grid_pack(sd->grid.obj, obj, 
                                   sd->current.x, sd->current.y, 
                                   sd->current.w, sd->current.h);
          }
        else
          {
             /* sizes are equal */
             sd->current.mode = psd->current.mode;
             sd->current.refresh_rate = psd->current.refresh_rate;
             sd->current.orient = psd->current.orient;
             sd->changes |= E_SMART_MONITOR_CHANGED_MODE;
          }

        _e_smart_monitor_coord_virtual_to_canvas(sd, sd->current.w, sd->current.h, &fw, &fh);
        if (fw < 1) fw = (sd->current.w / 10);
        if (fh < 1) fh = (sd->current.h / 10);

        fw *= 0.25;
        fh *= 0.25;
        if (fw < 32) fw = 32;
        if (fh < 25) fh = 25;

        /* create clone on the parent's canvas */
        sd->o_clone = edje_object_add(psd->evas);
        e_theme_edje_object_set(sd->o_clone, "base/theme/widgets", 
                                "e/conf/randr/main/mini");
        evas_object_smart_member_add(sd->o_clone, obj);

        /* remove the thumbnail from monitor frame */
        edje_object_part_unswallow(sd->o_frame, sd->o_thumb);

        /* put the thumbnail into the clone */
        edje_object_part_swallow(sd->o_clone, "e.swallow.preview", sd->o_thumb);

        /* based on orientation, size the clone accordingly */
        if ((sd->current.orient == ECORE_X_RANDR_ORIENTATION_ROT_0) || 
            (sd->current.orient == ECORE_X_RANDR_ORIENTATION_ROT_180))
          {
             evas_object_size_hint_min_set(sd->o_clone, fw, fh);
             evas_object_resize(sd->o_clone, fw, fh);
          }
        else
          {
             evas_object_size_hint_min_set(sd->o_clone, fh, fw);
             evas_object_resize(sd->o_clone, fh, fw);
          }

        evas_object_move(sd->o_clone, 0, 0);
        evas_object_show(sd->o_clone);

        /* pack the clone into the parent box */
        edje_object_part_box_append(psd->o_frame, "e.box.clone", sd->o_clone);

        /* calculate min size for the box */
        box = (Evas_Object *)edje_object_part_object_get(psd->o_frame, "e.box.clone");
        evas_object_size_hint_min_get(box, &fw, &fh);
        if (fw < 1) fw = 1;
        if (fh < 1) fh = 1;
        evas_object_resize(box, fw, fh);

        if ((psd->current.x != 0) || (psd->current.y != 0))
          {
             psd->current.x = 0;
             psd->current.y = 0;

             evas_object_grid_pack(psd->grid.obj, parent, 
                                   psd->current.x, psd->current.y, 
                                   psd->current.w, psd->current.h);
          }
     }
   else
     {
        if (sd->o_clone)
          {
             evas_object_smart_member_del(sd->o_clone);

             edje_object_part_unswallow(sd->o_clone, sd->o_thumb);
             edje_object_part_swallow(sd->o_frame, "e.swallow.preview", sd->o_thumb);
             evas_object_del(sd->o_clone);
          }

        sd->parent = NULL;
        sd->o_clone = NULL;

        /* set monitor position text */
        _e_smart_monitor_resolution_set(sd, sd->current.w, sd->current.h);

        /* repack monitor into original position (before cloning) */
        evas_object_grid_pack(sd->grid.obj, obj, 
                              sd->current.x, sd->current.y, 
                              sd->current.w, sd->current.h);

        evas_object_show(obj);
     }

   /* update changes */
   if ((sd->crtc.x != sd->current.x) || (sd->crtc.y != sd->current.y))
     sd->changes |= E_SMART_MONITOR_CHANGED_POSITION;
   else
     sd->changes &= ~(E_SMART_MONITOR_CHANGED_POSITION);

   if ((sd->crtc.mode != sd->current.mode))
     sd->changes |= E_SMART_MONITOR_CHANGED_MODE;
   else
     sd->changes &= ~(E_SMART_MONITOR_CHANGED_MODE);

   evas_object_smart_callback_call(obj, "monitor_changed", NULL);

   if (!psd) return;

   if ((psd->crtc.x != psd->current.x) || (psd->crtc.y != psd->current.y))
     psd->changes |= E_SMART_MONITOR_CHANGED_POSITION;
   else
     psd->changes &= ~(E_SMART_MONITOR_CHANGED_POSITION);

   if (psd->crtc.mode != sd->current.mode)
     psd->changes |= E_SMART_MONITOR_CHANGED_MODE;
   else
     psd->changes &= ~(E_SMART_MONITOR_CHANGED_MODE);

   evas_object_smart_callback_call(parent, "monitor_changed", NULL);
}

Evas_Object *
e_smart_monitor_clone_parent_get(Evas_Object *obj)
{
   E_Smart_Data *sd;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return NULL;

   return sd->parent;
}

E_Smart_Monitor_Changes 
e_smart_monitor_changes_get(Evas_Object *obj)
{
   E_Smart_Data *sd;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) 
     return E_SMART_MONITOR_CHANGED_NONE;

   return sd->changes;
}

Eina_Bool 
e_smart_monitor_changes_apply(Evas_Object *obj)
{
   E_Smart_Data *sd;
   Ecore_X_Window root = 0;
   Ecore_X_Randr_Output *outputs, primary = 0;
   int noutputs = 0;
   Ecore_X_Randr_Mode_Info *mode_info;
   Ecore_X_Randr_Mode mode;
   Evas_Coord cx, cy, cw, ch;
   Ecore_X_Randr_Orientation orient;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return EINA_FALSE;

   primary = (Ecore_X_Randr_Output)e_randr_cfg->primary;
   sd->primary = (sd->output == primary);

   if (sd->primary)
     edje_object_signal_emit(sd->o_frame, "e,state,primary,on", "e");
   else
     edje_object_signal_emit(sd->o_frame, "e,state,primary,off", "e");

   /* if we have no changes to apply, get out */
   if (sd->changes <= E_SMART_MONITOR_CHANGED_NONE) return EINA_FALSE;

   /* grab the root window */
   root = ecore_x_window_root_first_get();

   /* get the outputs for this crtc */
   outputs = ecore_x_randr_crtc_outputs_get(root, sd->crtc.id, &noutputs);
   if (noutputs < 1)
     {
        free(outputs);

        if ((outputs = malloc(sizeof(Ecore_X_Randr_Output))))
          {
             outputs[0] = sd->output;
             noutputs = 1;
          }
     }

   /* if this monitor gets re-enabled, we need to set a mode */
   if ((sd->current.enabled) && (!sd->current.mode))
     {
        Ecore_X_Randr_Mode_Info *info;

        info = _e_smart_monitor_mode_find(sd, sd->current.w, 
                                          sd->current.h, EINA_FALSE);
        if (info) 
          sd->current.mode = info->xid;
     }

   /* if this monitor gets re-enabled, we need to assign a crtc */
   if ((sd->current.enabled) && (!sd->crtc.id))
     {
        /* find a crtc */
        sd->crtc.id = _e_smart_monitor_crtc_find(sd->output);
     }

   /* record current values */
   mode = sd->current.mode;
   if (!sd->current.enabled)
     {
        mode = 0;
        noutputs = 0;
        if (outputs) free(outputs);
        outputs = NULL;
     }

   cx = sd->current.x;
   cy = sd->current.y;
   cw = sd->current.w;
   ch = sd->current.h;
   orient = sd->current.orient;

   /* if we are cloned, we need to use the parent values */
   if ((sd->cloned) && (sd->parent))
     {
        E_Smart_Data *psd;

        if ((psd = evas_object_smart_data_get(sd->parent)))
          {
             cx = psd->current.x;
             cy = psd->current.y;
             cw = psd->current.w;
             ch = psd->current.h;
             mode = psd->current.mode;
             orient = psd->current.orient;

             ecore_x_randr_crtc_clone_set(root, psd->crtc.id, sd->crtc.id);
          }
     }
   else
     {
        /* try to apply the settings */
        if (!ecore_x_randr_crtc_settings_set(root, sd->crtc.id, outputs, 
                                             noutputs, cx, cy, mode, orient))
          printf("FAILED TO APPLY MONITOR SETTINGS !!!\n");
     }

   /* free any allocated memory from ecore_x_randr */
   if (outputs) free(outputs);
   outputs = NULL;

   /* update crtc values to match current values */
   sd->crtc.x = cx;
   sd->crtc.y = cy;
   sd->crtc.w = cw;
   sd->crtc.h = ch;
   sd->crtc.mode = mode;
   sd->crtc.orient = orient;
   sd->crtc.enabled = sd->current.enabled;

   if ((sd->crtc.mode) && 
       (mode_info = ecore_x_randr_mode_info_get(root, sd->crtc.mode)))
     {
        sd->crtc.refresh_rate = 
          _e_smart_monitor_mode_refresh_rate_get(mode_info);
        ecore_x_randr_mode_info_free(mode_info);
     }
   else
     sd->crtc.refresh_rate = 60.0;

   /* reset changes */
   sd->changes = E_SMART_MONITOR_CHANGED_NONE;

   return EINA_TRUE;
}

const char *
e_smart_monitor_name_get(Evas_Object *obj)
{
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return NULL;

   /* get output name */
   return edje_object_part_text_get(sd->o_frame, "e.text.name");
}

Ecore_X_Randr_Output 
e_smart_monitor_output_get(Evas_Object *obj)
{
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return 0;
   return sd->output;
}

void 
e_smart_monitor_indicator_available_set(Evas_Object *obj, Eina_Bool available)
{
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;
   if (available)
     edje_object_signal_emit(sd->o_frame, "e,state,indicator,enabled", "e");
   else
     edje_object_signal_emit(sd->o_frame, "e,state,indicator,disabled", "e");
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

#ifdef BG_DBG
   /* create the bg test object */
   sd->o_bg = evas_object_rectangle_add(sd->evas);
   evas_object_color_set(sd->o_bg, 255, 0, 0, 128);
   evas_object_smart_member_add(sd->o_bg, obj);
#endif

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
   edje_object_signal_callback_add(sd->o_frame, "e,action,indicator,toggle", "e", 
                                   _e_smart_monitor_frame_cb_indicator_toggle, 
                                   obj);

   /* create the background preview */
   sd->o_thumb = e_livethumb_add(sd->evas);
   edje_object_part_swallow(sd->o_frame, "e.swallow.preview", sd->o_thumb);

   /* add callbacks for thumbnail events */
   evas_object_event_callback_add(sd->o_thumb, EVAS_CALLBACK_MOUSE_IN, 
                                  _e_smart_monitor_thumb_cb_mouse_in, NULL);
   evas_object_event_callback_add(sd->o_thumb, EVAS_CALLBACK_MOUSE_OUT, 
                                  _e_smart_monitor_thumb_cb_mouse_out, NULL);
   evas_object_event_callback_add(sd->o_thumb, EVAS_CALLBACK_MOUSE_UP, 
                                  _e_smart_monitor_thumb_cb_mouse_up, obj);
   evas_object_event_callback_add(sd->o_thumb, EVAS_CALLBACK_MOUSE_DOWN, 
                                  _e_smart_monitor_thumb_cb_mouse_down, obj);

   /* create the stand */
   sd->o_stand = edje_object_add(sd->evas);
   e_theme_edje_object_set(sd->o_stand, "base/theme/widgets", 
                           "e/conf/randr/main/stand");
   edje_object_part_swallow(sd->o_base, "e.swallow.stand", sd->o_stand);

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

   /* delete the refresh rate object */
   if (sd->o_refresh) evas_object_del(sd->o_refresh);

   if (sd->o_clone) evas_object_del(sd->o_clone);

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

   /* delete the stand */
   if (sd->o_stand) evas_object_del(sd->o_stand);

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
        edje_object_signal_callback_del(sd->o_frame, "e,action,indicator,toggle", "e", 
                                        _e_smart_monitor_frame_cb_indicator_toggle);

        /* delete the object */
        evas_object_del(sd->o_frame);
     }

   _e_smart_monitor_pointer_pop(obj, NULL);

   /* delete the base object */
   evas_object_del(sd->o_base);

#ifdef BG_DBG
   evas_object_del(sd->o_bg);
#endif

   /* free the list of modes */
   EINA_LIST_FREE(sd->modes, mode)
     if (mode) ecore_x_randr_mode_info_free(mode);

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

   evas_object_move(sd->o_base, x, y);
#ifdef BG_DBG
   evas_object_move(sd->o_bg, x, y);
#endif
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

   evas_object_resize(sd->o_base, w, h);
#ifdef BG_DBG
   evas_object_resize(sd->o_bg, w, h + 30);
#endif

   /* set livethumb thumbnail size */
   if ((!sd->resizing) && (!sd->rotating) && (!sd->moving))
     {
        Evas_Coord mw = 0, mh = 0;

        _e_smart_monitor_coord_virtual_to_canvas(sd, sd->max.mode_width, 
                                                 sd->max.mode_height, 
                                                 &mw, &mh);
        e_livethumb_vsize_set(sd->o_thumb, mw, mh);
     }
}

static void 
_e_smart_show(Evas_Object *obj)
{
   E_Smart_Data *sd;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* if we are already visible, then nothing to do */
//   if (sd->visible) return;

   if (!sd->cloned)
     {
        evas_object_show(sd->o_stand);
        evas_object_show(sd->o_frame);
        evas_object_show(sd->o_base);
#ifdef BG_DBG
        evas_object_show(sd->o_bg);
#endif

        if (!sd->current.enabled)
          edje_object_signal_emit(sd->o_frame, "e,state,disabled", "e");

        /* set visibility flag */
        sd->visible = EINA_TRUE;
     }
   else
     {
        if (sd->o_clone) evas_object_show(sd->o_clone);
        sd->visible = EINA_FALSE;
     }
}

static void 
_e_smart_hide(Evas_Object *obj)
{
   E_Smart_Data *sd;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* if we are already hidden, then nothing to do */
//   if (!sd->visible) return;

   if (sd->cloned)
     {
        evas_object_hide(sd->o_stand);
        evas_object_hide(sd->o_frame);
        evas_object_hide(sd->o_base);
#ifdef BG_DBG
        evas_object_hide(sd->o_bg);
#endif
     }
   else if (sd->o_clone)
     evas_object_hide(sd->o_clone);

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
#ifdef BG_DBG
   evas_object_clip_set(sd->o_bg, clip);
#endif
}

static void 
_e_smart_clip_unset(Evas_Object *obj)
{
   E_Smart_Data *sd;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   evas_object_clip_unset(sd->o_base);
#ifdef BG_DBG
   evas_object_clip_unset(sd->o_bg);
#endif
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
        if (((ev->desk_x < 0) || (ev->desk_x == sd->crtc.x)) && 
            ((ev->desk_y < 0) || (ev->desk_y == sd->crtc.y)))
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
_e_smart_monitor_coord_virtual_to_canvas(E_Smart_Data *sd, Evas_Coord vx, Evas_Coord vy, Evas_Coord *cx, Evas_Coord *cy)
{
   if (cx) *cx = sd->grid.x + (vx * ((double)sd->grid.w / sd->grid.vw));
   if (cy) *cy = sd->grid.y + (vy * ((double)sd->grid.h / sd->grid.vh));
}

static inline void 
_e_smart_monitor_coord_canvas_to_virtual(E_Smart_Data *sd, Evas_Coord cx, Evas_Coord cy, Evas_Coord *vx, Evas_Coord *vy)
{
   if ((sd->grid.w) && (vx)) 
     *vx = ((cx - sd->grid.x) * sd->grid.vw) / sd->grid.w;
   if ((sd->grid.h) && (vy)) 
     *vy = ((cy - sd->grid.y) * sd->grid.vh) / sd->grid.h;
}

static Ecore_X_Randr_Mode_Info *
_e_smart_monitor_mode_find(E_Smart_Data *sd, Evas_Coord w, Evas_Coord h, Eina_Bool skip_refresh)
{
   Ecore_X_Randr_Mode_Info *mode = NULL;
   Eina_List *l = NULL;

   /* loop the modes */
   EINA_LIST_REVERSE_FOREACH(sd->modes, l, mode)
     {
        if ((((int)mode->width - RESIZE_FUZZ) <= w) || 
            (((int)mode->width + RESIZE_FUZZ) <= w))
          {
             if ((((int)mode->height - RESIZE_FUZZ) <= h) || 
                 (((int)mode->height + RESIZE_FUZZ) <= h))
               {
                  if (!skip_refresh)
                    {
                       double rate = 0.0;

                       /* get the refresh rate for this mode */
                       rate = _e_smart_monitor_mode_refresh_rate_get(mode);

                       /* compare mode rate to "current" rate */
                       if ((int)rate == sd->current.refresh_rate)
                         return mode;
                    }
                  else
                    return mode;
               }
          }
     }

   return NULL;
}

static inline double 
_e_smart_monitor_mode_refresh_rate_get(Ecore_X_Randr_Mode_Info *mode)
{
   double rate = 0.0;

   if (mode)
     {
        if ((mode->hTotal) && (mode->vTotal))
          rate = ((float)mode->dotClock / 
                  ((float)mode->hTotal * (float)mode->vTotal));
     }

   return rate;
}

static void 
_e_smart_monitor_mode_refresh_rates_fill(Evas_Object *obj)
{
   E_Smart_Data *sd;
   Ecore_X_Window root = 0;
   Eina_List *m = NULL;
   Ecore_X_Randr_Mode_Info *cmode = NULL, *mode = NULL;
   E_Radio_Group *rg = NULL;
   Evas_Coord mw = 0, mh = 0;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* try to get the root window */
   root = ecore_x_window_root_first_get();

   /* try to get current mode info */
   if (!(cmode = ecore_x_randr_mode_info_get(root, sd->current.mode)))
     return;

   /* remove the old refresh rate list */
   if (sd->o_refresh)
     {
        edje_object_part_unswallow(sd->o_frame, sd->o_refresh);
        evas_object_del(sd->o_refresh);
     }

   /* create new refresh rate list */
   sd->o_refresh = e_widget_list_add(sd->evas, 0, 0);

   /* loop the modes and find the current one */
   EINA_LIST_FOREACH(sd->modes, m, mode)
     {
        /* compare mode names */
        if (!strcmp(cmode->name, mode->name))
          {
             Evas_Object *ow;
             double rate = 0.0;
             char buff[1024];

             /* create new radio group if needed */
             if (!rg) rg = e_widget_radio_group_new(&sd->current.refresh_rate);

             /* get the refresh rate for this mode */
             rate = _e_smart_monitor_mode_refresh_rate_get(mode);
             snprintf(buff, sizeof(buff), "%.1fHz", rate);

             /* create radio widget */
             ow = e_widget_radio_add(sd->evas, buff, (int)rate, rg);

             /* hook changed signal */
             evas_object_smart_callback_add(ow, "changed", 
                                            _e_smart_monitor_refresh_rate_cb_changed, obj);

             /* add this radio to the list */
             e_widget_list_object_append(sd->o_refresh, ow, 1, 0, 0.5);
          }
     }

   /* free any memory allocated from ecore_x_randr */
   if (cmode) ecore_x_randr_mode_info_free(cmode);

   /* calculate min size for refresh list and set */
   e_widget_size_min_get(sd->o_refresh, &mw, &mh);
   edje_extern_object_min_size_set(sd->o_refresh, mw, mh);

   /* swallow refresh list */
   edje_object_part_swallow(sd->o_frame, "e.swallow.refresh", sd->o_refresh);
   evas_object_show(sd->o_refresh);
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
_e_smart_monitor_thumb_cb_mouse_up(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj, void *event)
{
   Evas_Event_Mouse_Up *ev;
   Evas_Object *mon, *below;
   E_Smart_Data *sd;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   ev = event;
   if (ev->button != 1) return;

   /* try to get the monitor object */
   if (!(mon = data)) return;

   /* try to get the monitor smart data */
   if (!(sd = evas_object_smart_data_get(mon))) return;

   if (sd->cloned)
     {
        /* reset moving flag */
        sd->moving = EINA_FALSE;

        /* unclone this monitor */
        e_smart_monitor_clone_set(mon, NULL);

        return;
     }

   /* if we are not moving, then there is nothing to do in this routine */
   if (!sd->moving) return;

   /* reset moving flag */
   sd->moving = EINA_FALSE;

   /* reset mouse pointer */
   _e_smart_monitor_pointer_pop(obj, "move");

   if ((sd->current.x == sd->prev.x) && 
       (sd->current.y == sd->prev.y)) 
     return;

   /* any objects below this monitor ? */
   if ((below = evas_object_below_get(mon)))
     {
        const char *type;

        /* compare object type and make sure it's a monitor */
        if (!(type = evas_object_type_get(below))) return;
        if (!strcmp(type, "smart_monitor"))
          {
             E_Smart_Data *osd;
             Evas_Coord fx, fy, fw, fh;

             /* try to get the monitor smart data */
             if (!(osd = evas_object_smart_data_get(below))) return;
             if (!osd->visible) return;

             /* grab geometry of this frame */
             evas_object_geometry_get(osd->o_frame, &fx, &fy, &fw, &fh);

             /* determine if monitor is being dragged over another one */
             if (E_INSIDE(sd->x, sd->y, fx, fy, (fw / 2), (fh / 2)))
               {
                  e_smart_monitor_clone_set(mon, below);
                  edje_object_signal_emit(osd->o_frame, "e,state,drop,off", "e");
                  evas_object_smart_callback_call(mon, "monitor_moved", NULL);
                  return;
               }
             else if (E_INSIDE((sd->x + sd->w), sd->y, 
                               (fx + (fw / 2)), fy, fw, (fh / 2)))
               {
                  e_smart_monitor_clone_set(mon, below);
                  edje_object_signal_emit(osd->o_frame, "e,state,drop,off", "e");
                  evas_object_smart_callback_call(mon, "monitor_moved", NULL);
                  return;
               }
          }
     }

   /* take current object position, translate to virtual */
   _e_smart_monitor_coord_canvas_to_virtual(sd, sd->x, sd->y, 
                                            &sd->current.x, &sd->current.y);

   /* repack into the grid with updated position */
   evas_object_grid_pack(sd->grid.obj, mon, sd->current.x, sd->current.y, 
                         sd->current.w, sd->current.h);

   /* set monitor position text */
   _e_smart_monitor_position_set(sd, sd->current.x, sd->current.y);

   /* update changes */
   if ((sd->crtc.x != sd->current.x) || (sd->crtc.y != sd->current.y))
     sd->changes |= E_SMART_MONITOR_CHANGED_POSITION;
   else
     sd->changes &= ~(E_SMART_MONITOR_CHANGED_POSITION);

   evas_object_smart_callback_call(mon, "monitor_moved", NULL);
}

static void 
_e_smart_monitor_thumb_cb_mouse_down(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj, void *event)
{
   Evas_Event_Mouse_Down *ev;
   Evas_Object *mon;
   E_Smart_Data *sd;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   ev = event;
   if (ev->button != 1) return;

   /* try to get the monitor object */
   if (!(mon = data)) return;

   /* try to get the monitor smart data */
   if (!(sd = evas_object_smart_data_get(mon))) return;

   /* set mouse pointer */
   _e_smart_monitor_pointer_push(obj, "move");

   /* set moving flag */
   sd->moving = EINA_TRUE;

   /* record the clicked position */
   sd->mx = ev->canvas.x;
   sd->my = ev->canvas.y;

   /* record current size of monitor */
   evas_object_grid_pack_get(sd->grid.obj, mon, 
                             &sd->prev.x, &sd->prev.y, 
                             &sd->prev.w, &sd->prev.h);

   /* raise the monitor */
   evas_object_raise(mon);
}

static void 
_e_smart_monitor_frame_cb_mouse_move(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event)
{
   Evas_Object *mon;
   E_Smart_Data *sd;

//   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the monitor object */
   if (!(mon = data)) return;

   /* try to get the monitor smart data */
   if (!(sd = evas_object_smart_data_get(mon))) return;

   /* if the monitor is disabled, get out */
   /* if (!sd->current.enabled) return; */

   /* call appropriate function based on current action */
   if (sd->resizing) 
     _e_smart_monitor_resize_event(sd, mon, event);
   else if (sd->rotating) 
     _e_smart_monitor_rotate_event(sd, mon, event);
   else if (sd->moving)
     _e_smart_monitor_move_event(sd, mon, event);
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
   evas_object_grid_pack_get(sd->grid.obj, mon, 
                             &sd->current.x, &sd->current.y, 
                             &sd->current.w, &sd->current.h);

   sd->prev.x = sd->current.x;
   sd->prev.y = sd->current.y;
   sd->prev.w = sd->current.w;
   sd->prev.h = sd->current.h;

   /* set resizing flag */
   sd->resizing = EINA_TRUE;

   /* raise the monitor */
   evas_object_raise(mon);
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
   evas_object_grid_pack_get(sd->grid.obj, mon, NULL, NULL, 
                             &sd->current.w, &sd->current.h);

   /* set resizing flag */
   sd->resizing = EINA_FALSE;

   /* update changes */
   if ((sd->crtc.mode != sd->current.mode))
     sd->changes |= E_SMART_MONITOR_CHANGED_MODE;
   else
     sd->changes &= ~(E_SMART_MONITOR_CHANGED_MODE);

   evas_object_smart_callback_call(mon, "monitor_resized", NULL);
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

   /* reset the degree of rotation */
   sd->current.rotation = 0;

   /* record current size of monitor */
   evas_object_grid_pack_get(sd->grid.obj, mon, 
                             &sd->current.x, &sd->current.y, 
                             &sd->current.w, &sd->current.h);

   sd->prev.x = sd->current.x;
   sd->prev.y = sd->current.y;
   sd->prev.w = sd->current.w;
   sd->prev.h = sd->current.h;

   /* set resizing flag */
   sd->rotating = EINA_TRUE;
}

static void 
_e_smart_monitor_frame_cb_rotate_stop(void *data, Evas_Object *obj EINA_UNUSED, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   Evas_Object *mon;
   E_Smart_Data *sd;
   int rotation = 0, rot = 0;
   Ecore_X_Randr_Orientation orient = 0;
   Evas_Coord nx = 0, ny = 0, nw = 0, nh = 0;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the monitor object */
   if (!(mon = data)) return;

   /* try to get the monitor smart data */
   if (!(sd = evas_object_smart_data_get(mon))) return;

   /* set rotating flag */
   sd->rotating = EINA_FALSE;

   /* get the degrees of rotation based on this orient
    * 
    * NB: I know this seems redundant but it is needed however. The 
    * above orientation_get call will return the proper orientation 
    * for the amount which the user has rotated. Because of this, we need 
    * to take that orient and get the proper rotation angle.
    * 
    * EX: User manually rotates to 80 degrees. We take that 80 and 
    * factor in some fuziness to get 90 degrees. We need to take that 90 
    * and return an 'orientation' */
   rotation = _e_smart_monitor_rotation_get(sd->current.orient);

   /* get current orientation based on rotation */
   orient = _e_smart_monitor_orientation_get(sd->current.rotation + rotation);

   rot = _e_smart_monitor_rotation_get(orient);

   /* if we just flipped axis, we can remove map and get out */
   if (((sd->current.rotation + rotation) % 180) == 0)
     {
        /* remove the map from the frame so that controls revert to normal */
        evas_object_map_set(sd->o_frame, NULL);
        evas_object_map_enable_set(sd->o_frame, EINA_FALSE);

        /* apply rotation map */
        _e_smart_monitor_thumb_map_apply(sd->o_thumb, rot);

        /* update the orientation */
        sd->current.orient = orient;

        goto ret;
     }

   /* remove the map */
   evas_object_map_set(sd->o_frame, NULL);
   evas_object_map_enable_set(sd->o_frame, EINA_FALSE);

   nx = sd->current.x;
   ny = sd->current.y;

   /* calculate new size based on orientation */
   if ((orient == ECORE_X_RANDR_ORIENTATION_ROT_0) || 
       (orient == ECORE_X_RANDR_ORIENTATION_ROT_180))
     {
        if ((sd->current.orient == ECORE_X_RANDR_ORIENTATION_ROT_0) || 
            (sd->current.orient == ECORE_X_RANDR_ORIENTATION_ROT_180))
          {
             nw = sd->current.w;
             nh = sd->current.h;
          }
        else if ((sd->current.orient == ECORE_X_RANDR_ORIENTATION_ROT_90) || 
                 (sd->current.orient == ECORE_X_RANDR_ORIENTATION_ROT_270))
          {
             nw = sd->current.h;
             nh = sd->current.w;
          }
     }
   else if ((orient == ECORE_X_RANDR_ORIENTATION_ROT_90) || 
            (orient == ECORE_X_RANDR_ORIENTATION_ROT_270))
     {
        if ((sd->current.orient == ECORE_X_RANDR_ORIENTATION_ROT_90) || 
            (sd->current.orient == ECORE_X_RANDR_ORIENTATION_ROT_270))
          {
             nw = sd->current.w;
             nh = sd->current.h;
          }
        else if ((sd->current.orient == ECORE_X_RANDR_ORIENTATION_ROT_0) || 
                 (sd->current.orient == ECORE_X_RANDR_ORIENTATION_ROT_180))
          {
             nw = sd->current.h;
             nh = sd->current.w;
          }
     }

   /* make sure new size and position are within the grid */
   if ((nx + nw) > sd->grid.vw) nx = (sd->grid.vw - nw);
   if ((ny + nh) > sd->grid.vh) ny = (sd->grid.vh - nh);

   /* repack monitor into grid at new location & size */
   evas_object_grid_pack(sd->grid.obj, mon, nx, ny, nw, nh);

   /* update resolution text */
   _e_smart_monitor_resolution_set(sd, nw, nh);

   /* apply rotation map */
   _e_smart_monitor_thumb_map_apply(sd->o_thumb, rot);

   /* update current orientation */
   sd->current.orient = orient;

   /* update geometry */
   sd->current.x = nx;
   sd->current.y = ny;
   sd->current.w = nw;
   sd->current.h = nh;

ret:
   /* update changes */
   if ((sd->crtc.orient != sd->current.orient))
     sd->changes |= E_SMART_MONITOR_CHANGED_ORIENTATION;
   else
     sd->changes &= ~(E_SMART_MONITOR_CHANGED_ORIENTATION);

   evas_object_smart_callback_call(mon, "monitor_resized", NULL);
}

static void 
_e_smart_monitor_frame_cb_indicator_toggle(void *data, Evas_Object *obj EINA_UNUSED, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   Evas_Object *mon;
   E_Smart_Data *sd;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(mon = data)) return;

   /* try to get the monitor smart data */
   if (!(sd = evas_object_smart_data_get(mon))) return;

   /* check current enabled value */
   if (sd->current.enabled)
     {
        /* toggle value */
        sd->current.enabled = EINA_FALSE;

        /* tell frame edje object it is now disabled */
        edje_object_signal_emit(sd->o_frame, "e,state,disabled", "e");
     }
   else
     {
        /* toggle value */
        sd->current.enabled = EINA_TRUE;

        /* tell frame edje object it is now disabled */
        edje_object_signal_emit(sd->o_frame, "e,state,enabled", "e");
     }

   /* update changes */
   if ((sd->crtc.enabled != sd->current.enabled))
     sd->changes |= E_SMART_MONITOR_CHANGED_ENABLED;
   else
     sd->changes &= ~(E_SMART_MONITOR_CHANGED_ENABLED);

   evas_object_smart_callback_call(mon, "monitor_changed", NULL);
}

static void 
_e_smart_monitor_refresh_rate_cb_changed(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Evas_Object *mon;
   E_Smart_Data *sd;
   Ecore_X_Window root = 0;
   Eina_List *m = NULL;
   Ecore_X_Randr_Mode_Info *cmode = NULL, *mode = NULL;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   if (!(mon = data)) return;

   /* try to get the monitor smart data */
   if (!(sd = evas_object_smart_data_get(mon))) return;

   /* try to get the root window */
   root = ecore_x_window_root_first_get();

   /* try to get current mode info */
   if (!(cmode = ecore_x_randr_mode_info_get(root, sd->current.mode)))
     return;

   /* loop the modes and find the current one */
   EINA_LIST_FOREACH(sd->modes, m, mode)
     {
        /* compare mode names */
        if (!strcmp(cmode->name, mode->name))
          {
             int rate = 0;

             /* get the refresh rate for this mode */
             rate = (int)_e_smart_monitor_mode_refresh_rate_get(mode);

             /* compare refresh rates */
             if (rate == sd->current.refresh_rate)
               {
                  /* set new mode */
                  sd->current.mode = mode->xid;
                  break;
               }
          }
     }

   /* free any memory allocated from ecore_x_randr */
   if (cmode) ecore_x_randr_mode_info_free(cmode);

   /* update changes */
   if ((sd->crtc.mode != sd->current.mode))
     sd->changes |= E_SMART_MONITOR_CHANGED_MODE;
   else
     sd->changes &= ~(E_SMART_MONITOR_CHANGED_MODE);

   evas_object_smart_callback_call(mon, "monitor_changed", NULL);
}

static void 
_e_smart_monitor_resize_event(E_Smart_Data *sd, Evas_Object *mon, void *event)
{
   Evas_Event_Mouse_Move *ev;
   Evas_Coord dx = 0, dy = 0;
   Evas_Coord cw = 0, ch = 0;
   Evas_Coord nw = 0, nh = 0;
   Ecore_X_Randr_Mode_Info *mode = NULL;

//   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   ev = event;

   /* check for valid mouse movement
    * 
    * NB: This smells quite odd to me. How can we get a mouse_move event 
    * (and end up in here) when the coordinates say otherwise ??
    * Must be a synthetic event and we are not interested in those */
   if ((ev->cur.canvas.x == ev->prev.canvas.x) && 
       (ev->cur.canvas.y == ev->prev.canvas.y))
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

   /* convert monitor size to canvas size */
   _e_smart_monitor_coord_virtual_to_canvas(sd, sd->current.w, sd->current.h, 
                                            &cw, &ch);

   /* factor in resize difference and convert to virtual */
   _e_smart_monitor_coord_canvas_to_virtual(sd, (cw + dx), (ch + dy), 
                                            &nw, &nh);

   /* update current size values */
   sd->current.w = nw;
   sd->current.h = nh;

   /* based on orientation, try to find a valid mode */
   if ((sd->current.orient == ECORE_X_RANDR_ORIENTATION_ROT_0) || 
       (sd->current.orient == ECORE_X_RANDR_ORIENTATION_ROT_180))
     mode = _e_smart_monitor_mode_find(sd, sd->current.w, 
                                       sd->current.h, EINA_TRUE);
   else
     mode = _e_smart_monitor_mode_find(sd, sd->current.h, 
                                       sd->current.w, EINA_TRUE);

   if (mode)
     {
        Evas_Coord mw = 0, mh = 0;

        mw = mode->width;
        mh = mode->height;

        /* if we are rotated, we need to swap sizes */
        if ((sd->current.orient == ECORE_X_RANDR_ORIENTATION_ROT_90) || 
            (sd->current.orient == ECORE_X_RANDR_ORIENTATION_ROT_270))
          {
             mw = mode->height;
             mh = mode->width;
          }

        /* update current mode */
        sd->current.mode = mode->xid;

        /* update refresh rate */
        sd->current.refresh_rate = 
          (int)_e_smart_monitor_mode_refresh_rate_get(mode);

        /* if ((sd->current.x + mw) > sd->grid.vw) */
        /*   sd->current.x = (sd->grid.vw - mw); */

        /* if ((sd->current.h + mh) > sd->grid.vh) */
        /*   sd->current.y = (sd->grid.vh - mh); */

        /* update monitor size in the grid */
        evas_object_grid_pack(sd->grid.obj, mon, 
                              sd->current.x, sd->current.y, mw, mh);

        /* update resolution text */
        _e_smart_monitor_resolution_set(sd, mw, mh);
     }
}

static void 
_e_smart_monitor_rotate_event(E_Smart_Data *sd, Evas_Object *mon EINA_UNUSED, void *event)
{
   Evas_Event_Mouse_Move *ev;
   int rotation = 0;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   ev = event;

   /* get the amount of rotation from the mouse event */
   rotation = _e_smart_monitor_rotation_amount_get(sd, ev);

   /* if we have no rotation to map, get out */
   if (rotation == 0) return;

   /* factor in any existing rotation */
   rotation %= 360;

   /* update current rotation value */
   sd->current.rotation = rotation;

   /* apply rotation map */
   _e_smart_monitor_frame_map_apply(sd->o_frame, sd->current.rotation);
}

static void 
_e_smart_monitor_move_event(E_Smart_Data *sd, Evas_Object *mon, void *event)
{
   Evas_Event_Mouse_Move *ev;
   Evas_Coord dx = 0, dy = 0;
   Evas_Coord nx = 0, ny = 0;
   Evas_Coord px = 0, py = 0;
   Evas_Object *obj;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   ev = event;

   /* skip synthetic events */
   if ((ev->cur.output.x == ev->prev.output.x) && 
       (ev->cur.output.y == ev->prev.output.y))
     return;

   /* calculate difference in movement */
   dx = (ev->cur.output.x - ev->prev.output.x);
   dy = (ev->cur.output.y - ev->prev.output.y);

   nx = (sd->x + dx);
   ny = (sd->y + dy);

   /* make sure movement is restricted to be within the grid */
   if ((nx < sd->grid.x) || (ny < sd->grid.y))  return;
   if (((nx + sd->w) > (sd->grid.x + sd->grid.w)) || 
       ((ny + sd->h) > (sd->grid.y + sd->grid.h)))
     return;

   /* move the monitor */
   evas_object_move(mon, nx, ny);

   /* take current object position, translate to virtual */
   _e_smart_monitor_coord_canvas_to_virtual(sd, nx, ny, &px, &py);

   sd->current.x = px;
   sd->current.y = py;

   /* set monitor position text */
   _e_smart_monitor_position_set(sd, px, py);

   /* any objects below this monitor ? */
   if ((obj = evas_object_below_get(mon)))
     {
        const char *type;

        /* compare object type and make sure it's a monitor */
        if (!(type = evas_object_type_get(obj))) return;
        if (!strcmp(type, "smart_monitor"))
          {
             E_Smart_Data *osd;
             Evas_Coord fx, fy, fw, fh;

             /* try to get the monitor smart data */
             if (!(osd = evas_object_smart_data_get(obj))) return;
             if (!osd->visible) return;

             /* grab geometry of this frame */
             evas_object_geometry_get(osd->o_frame, &fx, &fy, &fw, &fh);

             /* determine if monitor is being dragged over another one */
             if (E_INSIDE(nx, ny, fx, fy, (fw / 2), (fh / 2)))
               edje_object_signal_emit(osd->o_frame, "e,state,drop,on", "e");
             else if (E_INSIDE((nx + sd->w), ny, 
                               (fx + (fw / 2)), fy, fw, (fh / 2)))
               edje_object_signal_emit(osd->o_frame, "e,state,drop,on", "e");
             else
               edje_object_signal_emit(osd->o_frame, "e,state,drop,off", "e");
          }
     }
}

static int 
_e_smart_monitor_rotation_amount_get(E_Smart_Data *sd, Evas_Event_Mouse_Move *ev)
{
   Evas_Coord cx = 0, cy = 0;
   Evas_Coord fx = 0, fy = 0, fw = 0, fh = 0;
   double a = 0.0, b = 0.0, c = 0.0, r = 0.0;
   double ax = 0.0, ay = 0.0, bx = 0.0, by = 0.0;
   double dotprod = 0.0;
   double mx = 0.0, my = 0.0;

   /* return a single rotation amount based on 
    * mouse movement in both directions */

   /* if there was no movement, return 0
    * 
    * NB: This smells quite odd to me. How can we get a mouse_move event 
    * (and end up in here) when the coordinates say otherwise ??
    * Must be a synthetic event and we are not interested in those */
   if ((ev->cur.output.x == ev->prev.output.x) && 
       (ev->cur.output.y == ev->prev.output.y))
     return 0;

   /* get the geometry of the frame */
   evas_object_geometry_get(sd->o_frame, &fx, &fy, &fw, &fh);

   /* get center point
    * 
    * NB: This COULD be used to provide a greater amount of rotation 
    * depending on distance of movement from center */
   cx = (fx + (fw / 2));
   cy = (fy + (fh / 2));

   mx = ev->cur.output.x;
   my = ev->cur.output.y;

   ax = ((fx + fw) - cx);
   ay = (fy - cy);

   bx = (mx - cx);
   by = (my - cy);

   /* calculate degrees of rotation
    * 
    * NB: A HUGE Thank You to Daniel for the help here !! */
   a = sqrt((ax * ax) + (ay * ay));
   b = sqrt((bx * bx) + (by * by));

   c = sqrt((mx - (fx + fw)) * 
            (mx - (fx + fw)) +
            (my - fy) * 
            (my - fy));

   r = acos(((a * a) + (b * b) - (c * c)) / (2 * (a * b)));
   r = r * 180 / M_PI;

   dotprod = ((ay * bx) + (-ax * by));
   if (dotprod > 0) r = 360 - r;

   return r;
}

static inline int 
_e_smart_monitor_rotation_get(Ecore_X_Randr_Orientation orient)
{
   /* return numerical rotation degree based on orientation */
   switch (orient)
     {
      case ECORE_X_RANDR_ORIENTATION_ROT_90:
        return 90;
      case ECORE_X_RANDR_ORIENTATION_ROT_180:
        return 180;
      case ECORE_X_RANDR_ORIENTATION_ROT_270:
        return 270;
      case ECORE_X_RANDR_ORIENTATION_ROT_0:
      default:
        return 0;
     }
}

static inline Ecore_X_Randr_Orientation 
_e_smart_monitor_orientation_get(int rotation)
{
   rotation %= 360;

   /* find the closest orientation based on rotation within fuziness */
   if (((rotation - ROTATE_FUZZ) <= 0) ||
       ((rotation + ROTATE_FUZZ) <= 0))
     return ECORE_X_RANDR_ORIENTATION_ROT_0;
   else if (((rotation - ROTATE_FUZZ) <= 90) ||
            ((rotation + ROTATE_FUZZ) <= 90))
     return ECORE_X_RANDR_ORIENTATION_ROT_90;
   else if (((rotation - ROTATE_FUZZ) <= 180) ||
            ((rotation + ROTATE_FUZZ) <=180))
     return ECORE_X_RANDR_ORIENTATION_ROT_180;
   else if (((rotation - ROTATE_FUZZ) <= 270) ||
            ((rotation + ROTATE_FUZZ) <= 270))
     return ECORE_X_RANDR_ORIENTATION_ROT_270;
   else if (((rotation - ROTATE_FUZZ) < 360) ||
            ((rotation + ROTATE_FUZZ) < 360))
     return ECORE_X_RANDR_ORIENTATION_ROT_0;

   /* return a default */
   return ECORE_X_RANDR_ORIENTATION_ROT_0;
}

static void 
_e_smart_monitor_frame_map_apply(Evas_Object *o_frame, int rotation)
{
   Evas_Coord fx = 0, fy = 0, fw = 0, fh = 0;
   static Evas_Map *map = NULL;

   /* create a new map if needed */
   if (!map) 
     {
        map = evas_map_new(4);
        evas_map_smooth_set(map, EINA_TRUE);
        evas_map_alpha_set(map, EINA_TRUE);
     }

   /* get the frame geometry */
   evas_object_geometry_get(o_frame, &fx, &fy, &fw, &fh);

   /* setup map */
   evas_map_util_points_populate_from_geometry(map, fx, fy, fw, fh, rotation);

   /* apply current rotation */
   evas_map_util_rotate(map, rotation, (fx + (fw / 2)), (fy + (fh / 2)));

   /* tell the frame to use this map */
   evas_object_map_set(o_frame, map);
   evas_object_map_enable_set(o_frame, EINA_TRUE);
}

static void 
_e_smart_monitor_thumb_map_apply(Evas_Object *o_thumb, int rotation)
{
   Evas_Coord fx = 0, fy = 0, fw = 0, fh = 0;
   static Evas_Map *map = NULL;

   /* create a new map if needed */
   if (!map) 
     {
        map = evas_map_new(4);
        evas_map_smooth_set(map, EINA_TRUE);
        evas_map_alpha_set(map, EINA_TRUE);
     }

   /* get the frame geometry */
   evas_object_geometry_get(o_thumb, &fx, &fy, &fw, &fh);

   /* setup map */
   evas_map_util_points_populate_from_geometry(map, fx, fy, fw, fh, rotation);

   /* apply current rotation */
   evas_map_util_rotate(map, rotation, (fx + (fw / 2)), (fy + (fh / 2)));

   /* tell the frame to use this map */
   evas_object_map_set(o_thumb, map);
   evas_object_map_enable_set(o_thumb, EINA_TRUE);
}

static Ecore_X_Randr_Crtc 
_e_smart_monitor_crtc_find(Ecore_X_Randr_Output output)
{
   Ecore_X_Randr_Crtc ret = 0;
   Ecore_X_Window root = 0;
   Ecore_X_Randr_Crtc *crtcs;
   int ncrtcs = 0;

   /* get root window */
   root = ecore_x_window_root_first_get();

   /* get possible crtcs for this output */
   if ((crtcs = ecore_x_randr_output_possible_crtcs_get(root, output, &ncrtcs)))
     {
        Ecore_X_Randr_Output *outputs;
        int i = 0, noutputs = 0;

        for (i = 0; i < ncrtcs; i++)
          {
             int j = 0;

             /* get any outputs on this crtc */
             if (!(outputs = 
                   ecore_x_randr_crtc_outputs_get(root, crtcs[i], &noutputs)))
               ret = crtcs[i];
             else if (noutputs == 0)
               ret = crtcs[i];
             else
               {
                  /* loop the outputs */
                  for (j = 0; j < noutputs; j++)
                    {
                       /* check if it is this output */
                       if (outputs[j] == output)
                         {
                            ret = crtcs[i];
                            break;
                         }
                    }
               }

             free(outputs);
             if (ret) break;
          }

        free(crtcs);
     }

   return ret;
}
