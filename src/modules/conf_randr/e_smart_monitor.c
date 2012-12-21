#include "e.h"
#include "e_mod_main.h"
#include "e_smart_monitor.h"

#define RESIZE_FUZZ 60
#define ROTATE_FUZZ 45

/* local structure */
typedef struct _E_Smart_Data E_Smart_Data;
struct _E_Smart_Data
{
   /* reference to the actual canvas */
   Evas *evas;

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

   /* refresh rate object */
   Evas_Object *o_refresh;

   /* the parent object if we are cloned */
   Evas_Object *parent;

   /* the 'mini' object to represent clone */
   Evas_Object *o_clone;

   /* list of mini's */
   Eina_List *clones;

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

   /* cloned flag */
   Eina_Bool cloned : 1;

   /* layout child geometry on start of move
    * 
    * NB: We save this so that if we 'unclone' we can restore this position */
   Evas_Coord cx, cy, cw, ch;

   /* handler for bg updates */
   Ecore_Event_Handler *bg_update_hdl;

   /* list of randr 'modes' */
   Eina_List *modes;

   /* min & max resolutions */
   struct 
     {
        int w, h;
     } min, max;

   /* original and current values */
   struct
     {
        Evas_Coord x, y, w, h; /* NB: these are virtual coordinates */
        Ecore_X_Randr_Mode_Info *mode;
        Ecore_X_Randr_Orientation orientation;
        int refresh_rate;
        int rotation;
        Eina_Bool enabled : 1;
     } orig, current;

   /* store where user clicked during resize */
   struct 
     {
        Evas_Coord x, y;
     } resize;

   /* reference to the Crtc Info */
   E_Randr_Crtc_Info *crtc;

   /* reference to the RandR Output Info */
   E_Randr_Output_Info *output;

   /* reference to the Layout widget */
   struct 
     {
        Evas_Object *obj; /* the actual layout widget */
        Evas_Coord x, y; /* the layout widget's position */
        Evas_Coord vw, vh; /* the layout widget's virtual size */
     } layout;

   /* reference to the Container */
   E_Container *con;

   /* reference to the Zone number */
   int zone_num;

   /* record what changed */
   E_Smart_Monitor_Changes changes;
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

static void _e_smart_monitor_refresh_rates_fill(Evas_Object *obj);
static double _e_smart_monitor_refresh_rate_get(Ecore_X_Randr_Mode_Info *mode);
static void _e_smart_monitor_modes_fill(E_Smart_Data *sd);
static int _e_smart_monitor_modes_sort(const void *data1, const void *data2);
static void _e_smart_monitor_background_set(E_Smart_Data *sd, Evas_Coord dx, Evas_Coord dy);
static Eina_Bool _e_smart_monitor_background_update(void *data, int type, void *event);
static Ecore_X_Randr_Mode_Info *_e_smart_monitor_resolution_get(E_Smart_Data *sd, Evas_Coord w, Evas_Coord h, Eina_Bool skip_rate_check);
static void _e_smart_monitor_resolution_set(E_Smart_Data *sd, Evas_Coord width, Evas_Coord height);
static int _e_smart_monitor_rotation_get(Ecore_X_Randr_Orientation orient);
static int _e_smart_monitor_rotation_amount_get(E_Smart_Data *sd, Evas_Event_Mouse_Move *ev);
static Ecore_X_Randr_Orientation _e_smart_monitor_orientation_get(int rotation);
static void _e_smart_monitor_pointer_push(Evas_Object *obj, const char *ptr);
static void _e_smart_monitor_pointer_pop(Evas_Object *obj, const char *ptr);
static void _e_smart_monitor_map_apply(Evas_Object *obj, int rotation);

static void _e_smart_monitor_move_event(E_Smart_Data *sd, Evas_Object *mon, void *event);
static void _e_smart_monitor_resize_event(E_Smart_Data *sd, Evas_Object *mon, void *event);
static void _e_smart_monitor_rotate_event(E_Smart_Data *sd, Evas_Object *mon EINA_UNUSED, void *event);

/* local callback prototypes */
static void _e_smart_monitor_cb_refresh_rate_changed(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED);
static void _e_smart_monitor_frame_cb_mouse_move(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event);
static void _e_smart_monitor_frame_cb_resize_in(void *data EINA_UNUSED, Evas_Object *obj, const char *emission EINA_UNUSED, const char *source EINA_UNUSED);
static void _e_smart_monitor_frame_cb_resize_out(void *data EINA_UNUSED, Evas_Object *obj, const char *emission EINA_UNUSED, const char *source EINA_UNUSED);
static void _e_smart_monitor_frame_cb_resize_start(void *data, Evas_Object *obj EINA_UNUSED, const char *emission EINA_UNUSED, const char *source EINA_UNUSED);
static void _e_smart_monitor_frame_cb_resize_stop(void *data, Evas_Object *obj EINA_UNUSED, const char *emission EINA_UNUSED, const char *source EINA_UNUSED);
static void _e_smart_monitor_frame_cb_rotate_in(void *data EINA_UNUSED, Evas_Object *obj, const char *emission EINA_UNUSED, const char *source EINA_UNUSED);
static void _e_smart_monitor_frame_cb_rotate_out(void *data EINA_UNUSED, Evas_Object *obj, const char *emission EINA_UNUSED, const char *source EINA_UNUSED);
static void _e_smart_monitor_frame_cb_rotate_start(void *data, Evas_Object *obj EINA_UNUSED, const char *emission EINA_UNUSED, const char *source EINA_UNUSED);
static void _e_smart_monitor_frame_cb_rotate_stop(void *data, Evas_Object *obj EINA_UNUSED, const char *emission EINA_UNUSED, const char *source EINA_UNUSED);
static void _e_smart_monitor_frame_cb_indicator_in(void *data EINA_UNUSED, Evas_Object *obj, const char *emission EINA_UNUSED, const char *source EINA_UNUSED);
static void _e_smart_monitor_frame_cb_indicator_out(void *data EINA_UNUSED, Evas_Object *obj, const char *emission EINA_UNUSED, const char *source EINA_UNUSED);
static void _e_smart_monitor_frame_cb_indicator_toggle(void *data, Evas_Object *obj EINA_UNUSED, const char *emission EINA_UNUSED, const char *source EINA_UNUSED);

static void _e_smart_monitor_thumb_cb_mouse_in(void *data EINA_UNUSED, Evas *evas EINA_UNUSED, Evas_Object *obj, void *event EINA_UNUSED);
static void _e_smart_monitor_thumb_cb_mouse_out(void *data EINA_UNUSED, Evas *evas EINA_UNUSED, Evas_Object *obj, void *event EINA_UNUSED);
static void _e_smart_monitor_thumb_cb_mouse_down(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj, void *event);
static void _e_smart_monitor_thumb_cb_mouse_up(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj, void *event);

static void _e_smart_monitor_layout_cb_move(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED);

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

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* set this monitor's output reference */
   sd->output = output;
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
e_smart_monitor_crtc_set(Evas_Object *obj, E_Randr_Crtc_Info *crtc)
{
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* set this monitor's crtc reference */
   sd->crtc = crtc;
}

void 
e_smart_monitor_layout_set(Evas_Object *obj, Evas_Object *layout)
{
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* set this monitor's layout reference */
   sd->layout.obj = layout;

   /* get out if this is not a valid layout */
   if (!layout) return;

   /* get the layout's virtual size */
   e_layout_virtual_size_get(layout, &sd->layout.vw, &sd->layout.vh);

   /* setup callback to be notified when this layout moves */
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
e_smart_monitor_setup(Evas_Object *obj)
{
   Evas_Coord mw = 0, mh = 0;
   E_Zone *zone;
   E_Desk *desk;
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* fill the list of 'modes' for this monitor
    * 
    * NB: This clears old modes and also sets the min & max resolutions */
   _e_smart_monitor_modes_fill(sd);

   /* check if enabled */
   sd->orig.enabled = EINA_FALSE;
   if ((sd->crtc) && (sd->crtc->current_mode))
     sd->orig.enabled = EINA_TRUE;

   /* if we have a crtc, get the x/y location of it and current refresh rate
    * 
    * NB: Used to determine the proper container */
   if (sd->crtc)
     {
        /* set original geometry */
        sd->orig.x = sd->crtc->geometry.x;
        sd->orig.y = sd->crtc->geometry.y;

        if (!sd->crtc->current_mode)
          {
             sd->crtc->current_mode = eina_list_last_data_get(sd->modes);

             /* set original mode */
             sd->orig.mode = sd->crtc->current_mode;

             if (!sd->orig.mode)
               {
                  sd->orig.w = 640;
                  sd->orig.h = 480;
                  sd->orig.refresh_rate = 60;
               }
             else
               {
                  sd->orig.w = sd->orig.mode->width;
                  sd->orig.h = sd->orig.mode->height;

                  /* set original refresh rate */
                  sd->orig.refresh_rate = 
                    _e_smart_monitor_refresh_rate_get(sd->orig.mode);
               }
          }
        else
          {
             /* set original mode */
             sd->orig.mode = sd->crtc->current_mode;

             sd->orig.w = sd->orig.mode->width;
             sd->orig.h = sd->orig.mode->height;

             /* set original refresh rate */
             sd->orig.refresh_rate = 
               _e_smart_monitor_refresh_rate_get(sd->orig.mode);
          }

        /* set the original orientation */
        sd->orig.orientation = sd->crtc->current_orientation;
     }

   /* set the original rotation */
   sd->orig.rotation = _e_smart_monitor_rotation_get(sd->orig.orientation);

   /* get the current zone at this crtc coordinate */
   sd->con = e_container_current_get(e_manager_current_get());
   if (!(zone = e_container_zone_at_point_get(sd->con, sd->orig.x, sd->orig.y)))
     zone = e_util_zone_current_get(e_manager_current_get());

   /* set references to the container & zone number
    * 
    * NB: Used later if background gets updated */
   sd->zone_num = zone->num;

   /* with the min & max resolutions, we can now set the thumbnail size.
    * get largest resolution and convert to largest canvas size */
   if (sd->layout.obj)
     e_layout_coord_virtual_to_canvas(sd->layout.obj, 
                                      sd->max.w, sd->max.h, &mw, &mh);

   /* set thumbnail size based on largest canvas size */
   mh = (mw * mh) / mw;
   if (sd->o_thumb) e_livethumb_vsize_set(sd->o_thumb, mw, mh);

   /* try to get the desktop at these coordinates. fallback to current */
   if (!(desk = e_desk_at_xy_get(zone, sd->orig.x, sd->orig.y)))
     desk = e_desk_current_get(zone);

   /* set the background image */
   _e_smart_monitor_background_set(sd, desk->x, desk->y);

   /* if we have an output, set the monitor name */
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

   /* set the resolution name */
   _e_smart_monitor_resolution_set(sd, sd->orig.w, sd->orig.h);

   /* send enabled/disabled signals */
   if (sd->orig.enabled)
     edje_object_signal_emit(sd->o_frame, "e,state,enabled", "e");
   else
     edje_object_signal_emit(sd->o_frame, "e,state,disabled", "e");

   /* check if rotation is supported */
   if (sd->crtc)
     {
        /* if no rotation is supported, disable rotate in frame */
        if (sd->crtc->orientations <= ECORE_X_RANDR_ORIENTATION_ROT_0)
          edje_object_signal_emit(sd->o_frame, "e,state,rotate_disabled", "e");
     }

   /* set the 'current' values to be equal to the original ones */
   sd->current.x = sd->orig.x;
   sd->current.y = sd->orig.y;
   sd->current.w = sd->orig.w;
   sd->current.h = sd->orig.h;
   sd->current.orientation = sd->orig.orientation;
   sd->current.rotation = sd->orig.rotation;
   sd->current.mode = sd->orig.mode;
   sd->current.refresh_rate = sd->orig.refresh_rate;
   sd->current.enabled = sd->orig.enabled;

   /* fill in list of refresh rates
    * 
    * NB: This has to be done after the 'current' refresh rate is calculated 
    * above or else the radio widgets do not get properly selected */
   _e_smart_monitor_refresh_rates_fill(obj);
}

E_Smart_Monitor_Changes 
e_smart_monitor_changes_get(Evas_Object *obj)
{
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) 
     return E_SMART_MONITOR_CHANGED_NONE;

   /* return the changes for this monitor */
   return sd->changes;
}

void 
e_smart_monitor_changes_reset(Evas_Object *obj)
{
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* reset the changes variable */
   sd->changes = E_SMART_MONITOR_CHANGED_NONE;

   /* update the original values to match current state */
   sd->orig.x = sd->current.x;
   sd->orig.y = sd->current.y;
   sd->orig.w = sd->current.w;
   sd->orig.h = sd->current.h;
   sd->orig.mode = sd->current.mode;
   sd->orig.orientation = sd->current.orientation;
   sd->orig.refresh_rate = sd->current.refresh_rate;
   sd->orig.rotation = sd->current.rotation;
   sd->orig.enabled = sd->current.enabled;
}

void 
e_smart_monitor_changes_apply(Evas_Object *obj)
{
   E_Smart_Data *sd;
   Eina_Bool reset = EINA_FALSE;
   Ecore_X_Window root;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   root = sd->con->manager->root;

   if (sd->changes & E_SMART_MONITOR_CHANGED_ENABLED)
     {
        if (sd->current.enabled)
          {
             if (sd->crtc)
               {
                  Ecore_X_Randr_Output *outputs;
                  Evas_Coord mx, my;
                  int noutputs = -1;

                  mx = sd->current.x;
                  my = sd->current.y;

                  noutputs = eina_list_count(sd->crtc->outputs);
                  if (noutputs < 1)
                    {
                       outputs = calloc(1, sizeof(Ecore_X_Randr_Output));
                       outputs[0] = sd->output->xid;
                       noutputs = 1;
                    }
                  else
                    {
                       int i = 0;

                       outputs = 
                         calloc(noutputs, sizeof(Ecore_X_Randr_Output));
                       for (i = 0; i < noutputs; i++)
                         {
                            E_Randr_Output_Info *ero;

                            ero = eina_list_nth(sd->crtc->outputs, i);
                            outputs[i] = ero->xid;
                         }
                    }

                  ecore_x_randr_crtc_settings_set(root, sd->crtc->xid, 
                                                  outputs, 
                                                  noutputs, mx, my,
                                                  sd->current.mode->xid, 
                                                  sd->current.orientation);
                  free(outputs);
               }
          }
        else
          ecore_x_randr_crtc_settings_set(root, sd->crtc->xid, 
                                          NULL, 0, 0, 0, 0, 
                                          ECORE_X_RANDR_ORIENTATION_ROT_0);

        reset = EINA_TRUE;
     }

   if (sd->changes & E_SMART_MONITOR_CHANGED_POSITION)
     {
        if (sd->crtc)
          {
             Evas_Coord mx, my;
             Evas_Coord cx, cy;

             mx = sd->current.x;
             my = sd->current.y;

             ecore_x_randr_crtc_pos_get(root, sd->crtc->xid, &cx, &cy);
             if ((cx != mx) || (cy != my))
               {
                  ecore_x_randr_crtc_pos_set(root, sd->crtc->xid, mx, my);
                  reset = EINA_TRUE;
               }
          }
     }

   if (sd->changes & E_SMART_MONITOR_CHANGED_ROTATION)
     {
        if (sd->crtc)
          {
             Ecore_X_Randr_Orientation orient;

             orient = sd->current.orientation;
             if ((sd->crtc) && (orient != sd->crtc->current_orientation))
               {
                  ecore_x_randr_crtc_orientation_set(root, 
                                                     sd->crtc->xid, orient);
                  reset = EINA_TRUE;
               }
          }
     }

   if ((sd->changes & E_SMART_MONITOR_CHANGED_REFRESH) || 
       (sd->changes & E_SMART_MONITOR_CHANGED_RESOLUTION))
     {
        if (sd->crtc)
          {
             Ecore_X_Randr_Output *outputs = NULL;
             int noutputs = -1;

             if (sd->output) outputs = &sd->output->xid;

             if ((sd->crtc) && (sd->crtc->outputs))
               noutputs = eina_list_count(sd->crtc->outputs);

             ecore_x_randr_crtc_mode_set(root, sd->crtc->xid, 
                                         outputs, noutputs, 
                                         sd->current.mode->xid);
             reset = EINA_TRUE;
          }
     }

   if (reset) ecore_x_randr_screen_reset(root);
}

void 
e_smart_monitor_current_geometry_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y, Evas_Coord *w, Evas_Coord *h)
{
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   if (x) *x = sd->current.x;
   if (y) *y = sd->current.y;
   if (w) *w = sd->current.w;
   if (h) *h = sd->current.h;
}

Ecore_X_Randr_Orientation 
e_smart_monitor_current_orientation_get(Evas_Object *obj)
{
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) 
     return ECORE_X_RANDR_ORIENTATION_ROT_0;

   /* return the current orientation */
   return sd->current.orientation;
}

Ecore_X_Randr_Mode_Info *
e_smart_monitor_current_mode_get(Evas_Object *obj)
{
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) 
     return NULL;

   /* return the current mode */
   return sd->current.mode;
}

Eina_Bool 
e_smart_monitor_current_enabled_get(Evas_Object *obj)
{
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) 
     return EINA_FALSE;

   /* return the current enabled mode */
   return sd->current.enabled;
}

void 
e_smart_monitor_clone_add(Evas_Object *obj, Evas_Object *mon)
{
   E_Smart_Data *osd, *msd;
   Evas_Object *o;
   Evas_Coord mw = 0, mh = 0;

   /* try to get the objects smart data */
   if (!(osd = evas_object_smart_data_get(obj))) return;

   /* try to get the objects smart data */
   if (!(msd = evas_object_smart_data_get(mon))) return;

   /* set cloned flag */
   msd->cloned = EINA_TRUE;

   /* set cloned parent */
   msd->parent = obj;

   /* grab size of monitor's frame */
   evas_object_geometry_get(msd->o_frame, NULL, NULL, &mw, &mh);

   /* hide this monitor */
   if (msd->visible) evas_object_hide(mon);

   /* use 1/4 of the size 
    * 
    * FIXME: NB: This should be fixed to use the same aspect ratio as the 
    * swallowed monitor */
   mw *= 0.25;
   mh *= 0.25;

   /* create mini representation of this monitor */
   msd->o_clone = edje_object_add(osd->evas);
   e_theme_edje_object_set(msd->o_clone, "base/theme/widgets", 
                           "e/conf/randr/main/mini");

   evas_object_data_set(msd->o_clone, "smart_data", msd);

   edje_object_part_unswallow(msd->o_frame, msd->o_thumb);
   evas_object_hide(msd->o_thumb);

   /* swallow the background */
   edje_object_part_swallow(msd->o_clone, "e.swallow.preview", msd->o_thumb);
   evas_object_show(msd->o_thumb);

   if ((msd->current.orientation == ECORE_X_RANDR_ORIENTATION_ROT_0) || 
       (msd->current.orientation == ECORE_X_RANDR_ORIENTATION_ROT_180))
     {
        /* set minimum size */
        evas_object_size_hint_min_set(msd->o_clone, mw, mh);
     }
   else
     {
        /* set minimum size */
        evas_object_size_hint_min_set(msd->o_clone, mh, mw);
     }

   /* resize the mini monitor */
   evas_object_resize(msd->o_clone, mw, mh);

   /* show the mini monitor */
   evas_object_show(msd->o_clone);

   /* add to list of cloned minis */
   osd->clones = eina_list_append(osd->clones, msd->o_clone);

   /* add this clone to the monitor */
   edje_object_part_box_append(osd->o_frame, "e.box.clone", msd->o_clone);

   /* adjust clone box size */
   if ((o = (Evas_Object *)
        edje_object_part_object_get(osd->o_frame, "e.box.clone")))
     {
        evas_object_size_hint_min_get(o, &mw, &mh);
        if (mw < 1) mw = 1;
        if (mh < 1) mh = 1;
        evas_object_resize(o, mw, mh);
     }

   /* apply existing rotation to mini */
   _e_smart_monitor_map_apply(msd->o_clone, msd->current.rotation);
}

void 
e_smart_monitor_clone_del(Evas_Object *obj, Evas_Object *mon)
{
   E_Smart_Data *osd, *msd;
   Evas_Object *o;
   Evas_Coord x = 0, y = 0, w = 0, h = 0;

   /* try to get the objects smart data */
   if (!(osd = evas_object_smart_data_get(obj))) return;

   /* try to get the objects smart data */
   if (!(msd = evas_object_smart_data_get(mon))) return;

   /* remove this monitor from the clone box */
   edje_object_part_box_remove(osd->o_frame, "e.box.clone", msd->o_clone);

   edje_object_part_unswallow(msd->o_clone, msd->o_thumb);
   evas_object_hide(msd->o_thumb);

   /* delete the mini */
   evas_object_del(msd->o_clone);

   /* swallow the background */
   evas_object_show(msd->o_thumb);
   edje_object_part_swallow(msd->o_frame, "e.swallow.preview", msd->o_thumb);

   /* adjust clone box size */
   if ((o = (Evas_Object *)
        edje_object_part_object_get(osd->o_frame, "e.box.clone")))
     {
        Evas_Coord mw = 0, mh = 0;

        evas_object_size_hint_min_get(o, &mw, &mh);
        if (mw < 1) mw = 1;
        if (mh < 1) mh = 1;
        evas_object_resize(o, mw, mh);
     }

   /* show the monitor */
   evas_object_show(mon);

   /* set cloned flag */
   msd->cloned = EINA_FALSE;

   /* set parent object */
   msd->parent = NULL;

   x = msd->cx;
   y = msd->cy;
   w = msd->cw;
   h = msd->ch;

   /* safety check for valid values.
    * 
    * NB: Needed in the case that we have no previous setup, we are in a clone 
    * situation (from X), and we were not manually moved */
   if ((msd->cw == 0) || (msd->ch == 0))
     {
        e_layout_child_geometry_get(mon, &x, &y, &w, &h);
        msd->current.x = x;
        msd->current.y = y;
     }

   /* restore to starting size */
   e_layout_child_resize(mon, w, h);

   /* restore to starting position */
   e_layout_child_move(mon, x, y);
}

void 
e_smart_monitor_drop_zone_set(Evas_Object *obj, Eina_Bool can_drop)
{
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* if we can drop here, emit signal to turn on hilighting, else 
    * emit signal to turn it off */
   if (can_drop)
     edje_object_signal_emit(sd->o_frame, "e,state,drop,on", "e");
   else
     edje_object_signal_emit(sd->o_frame, "e,state,drop,off", "e");
}

/* local functions */
static void 
_e_smart_add(Evas_Object *obj)
{
   E_Smart_Data *sd;

   /* try to allocate the smart data structure */
   if (!(sd = E_NEW(E_Smart_Data, 1))) return;

   /* grab the canvas */
   sd->evas = evas_object_evas_get(obj);

   /* create the base object */
   sd->o_base = edje_object_add(sd->evas);
   e_theme_edje_object_set(sd->o_base, "base/theme/widgets", 
                           "e/conf/randr/main/monitor");
   evas_object_smart_member_add(sd->o_base, obj);

   /* create monitor 'frame' */
   sd->o_frame = edje_object_add(sd->evas);
   e_theme_edje_object_set(sd->o_frame, "base/theme/widgets", 
                           "e/conf/randr/main/frame");
   edje_object_part_swallow(sd->o_base, "e.swallow.frame", sd->o_frame);

   evas_object_event_callback_add(sd->o_frame, EVAS_CALLBACK_MOUSE_MOVE, 
                                  _e_smart_monitor_frame_cb_mouse_move, obj);

   /* create the preview */
   sd->o_thumb = e_livethumb_add(sd->evas);
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
   sd->o_stand = edje_object_add(sd->evas);
   e_theme_edje_object_set(sd->o_stand, "base/theme/widgets", 
                           "e/conf/randr/main/stand");
   edje_object_part_swallow(sd->o_base, "e.swallow.stand", sd->o_stand);

   /* add callbacks for resize signals */
   edje_object_signal_callback_add(sd->o_frame, "e,action,resize,in", "e", 
                                   _e_smart_monitor_frame_cb_resize_in, NULL);
   edje_object_signal_callback_add(sd->o_frame, "e,action,resize,out", "e", 
                                   _e_smart_monitor_frame_cb_resize_out, NULL);
   edje_object_signal_callback_add(sd->o_frame, "e,action,resize,start", "e", 
                                   _e_smart_monitor_frame_cb_resize_start, obj);
   edje_object_signal_callback_add(sd->o_frame, "e,action,resize,stop", "e", 
                                   _e_smart_monitor_frame_cb_resize_stop, obj);

   /* add callbacks for rotate signals */
   edje_object_signal_callback_add(sd->o_frame, "e,action,rotate,in", "e", 
                                   _e_smart_monitor_frame_cb_rotate_in, NULL);
   edje_object_signal_callback_add(sd->o_frame, "e,action,rotate,out", "e", 
                                   _e_smart_monitor_frame_cb_rotate_out, NULL);
   edje_object_signal_callback_add(sd->o_frame, "e,action,rotate,start", "e", 
                                   _e_smart_monitor_frame_cb_rotate_start, obj);
   edje_object_signal_callback_add(sd->o_frame, "e,action,rotate,stop", "e", 
                                   _e_smart_monitor_frame_cb_rotate_stop, obj);

   /* add callbacks for indicator signals */
   edje_object_signal_callback_add(sd->o_frame, "e,action,indicator,in", "e", 
                                   _e_smart_monitor_frame_cb_indicator_in, sd);
   edje_object_signal_callback_add(sd->o_frame, "e,action,indicator,out", "e", 
                                   _e_smart_monitor_frame_cb_indicator_out, sd);
   edje_object_signal_callback_add(sd->o_frame, 
                                   "e,action,indicator,toggle", "e", 
                                   _e_smart_monitor_frame_cb_indicator_toggle, 
                                   obj);

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
   Eina_List *l;
   Evas_Object *mclone;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* delete any existing clones */
   EINA_LIST_FOREACH(sd->clones, l, mclone)
     evas_object_del(mclone);

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
                                        "e,action,resize,start", "e", 
                                        _e_smart_monitor_frame_cb_resize_start);
        edje_object_signal_callback_del(sd->o_frame, 
                                        "e,action,resize,stop", "e", 
                                        _e_smart_monitor_frame_cb_resize_stop);
        edje_object_signal_callback_del(sd->o_frame, 
                                        "e,action,rotate,in", "e", 
                                        _e_smart_monitor_frame_cb_rotate_in);
        edje_object_signal_callback_del(sd->o_frame, 
                                        "e,action,rotate,out", "e", 
                                        _e_smart_monitor_frame_cb_rotate_out);
        edje_object_signal_callback_del(sd->o_frame, 
                                        "e,action,rotate,start", "e", 
                                        _e_smart_monitor_frame_cb_rotate_start);
        edje_object_signal_callback_del(sd->o_frame, 
                                        "e,action,rotate,stop", "e", 
                                        _e_smart_monitor_frame_cb_rotate_stop);
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
   Eina_List *l;
   Evas_Object *mclone;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   if ((sd->x == x) && (sd->y == y)) return;

   sd->x = x;
   sd->y = y;

   /* move the base object */
   if (sd->o_base) evas_object_move(sd->o_base, x, y);

   /* if we are not visible, no need to update map */
   if (!sd->visible) return;

   /* apply any existing rotation */
   _e_smart_monitor_map_apply(sd->o_frame, sd->current.rotation);

   /* loop the clones and update their rotation */
   EINA_LIST_FOREACH(sd->clones, l, mclone)
     {
        E_Smart_Data *msd;

        /* try to get the clones smart data */
        if (!(msd = evas_object_data_get(mclone, "smart_data")))
          continue;

        /* apply existing rotation to mini */
        _e_smart_monitor_map_apply(mclone, msd->current.rotation);
     }
}

static void 
_e_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Smart_Data *sd;
   Eina_List *l;
   Evas_Object *mclone;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   if ((sd->w == h) && (sd->w == h)) return;

   sd->w = w;
   sd->h = h;

   /* resize the base object */
   if (sd->o_base) evas_object_resize(sd->o_base, w, h);

   /* if we are not visible, no need to update map */
   if (!sd->visible) return;

   /* apply any existing rotation */
   _e_smart_monitor_map_apply(sd->o_frame, sd->current.rotation);

   /* loop the clones and update their rotation */
   EINA_LIST_FOREACH(sd->clones, l, mclone)
     {
        E_Smart_Data *msd;

        /* try to get the clones smart data */
        if (!(msd = evas_object_data_get(mclone, "smart_data")))
          continue;

        /* apply existing rotation to mini */
        _e_smart_monitor_map_apply(mclone, msd->current.rotation);
     }
}

static void 
_e_smart_show(Evas_Object *obj)
{
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* if it is already visible, get out */
//   if (sd->visible) return;

   /* show the stand */
   if (sd->o_stand) evas_object_show(sd->o_stand);

   /* show the frame */
   if (sd->o_frame) evas_object_show(sd->o_frame);

   /* show the base object */
   if (sd->o_base) evas_object_show(sd->o_base);

   /* set visibility flag */
   sd->visible = EINA_TRUE;

   /* apply any existing rotation */
   _e_smart_monitor_map_apply(sd->o_frame, sd->current.rotation);
}

static void 
_e_smart_hide(Evas_Object *obj)
{
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* if it is not visible, we have nothing to do */
//   if (!sd->visible) return;

   /* hide the stand */
   if (sd->o_stand) evas_object_hide(sd->o_stand);

   /* hide the frame */
   if (sd->o_frame) evas_object_hide(sd->o_frame);

   /* hide the base object */
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
_e_smart_monitor_refresh_rates_fill(Evas_Object *obj)
{
   E_Smart_Data *sd;
   E_Radio_Group *rg = NULL;
   Eina_List *m = NULL;
   Ecore_X_Randr_Mode_Info *mode = NULL;
   Evas_Coord mw = 0, mh = 0;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   if (sd->o_refresh)
     {
        /* remove the old refresh list */
        edje_object_part_unswallow(sd->o_frame, sd->o_refresh);
        evas_object_del(sd->o_refresh);
     }

   /* create new refresh list widget */
   sd->o_refresh = e_widget_list_add(sd->evas, 0, 0);

   /* loop the modes and find the current one */
   EINA_LIST_FOREACH(sd->modes, m, mode)
     {
        if (!sd->current.mode) continue;

        /* compare mode names */
        if (!strcmp(mode->name, sd->current.mode->name))
          {
             if ((mode->hTotal) && (mode->vTotal))
               {
                  Evas_Object *ow;
                  double rate = 0.0;
                  char buff[1024];

                  /* create radio group for rates */
                  if (!rg) rg = e_widget_radio_group_new(&sd->current.refresh_rate);

                  /* calculate rate */
                  rate = _e_smart_monitor_refresh_rate_get(mode);
                  snprintf(buff, sizeof(buff), "%.1fHz", rate);

                  /* create the radio widget */
                  ow = e_widget_radio_add(sd->evas, buff, (int)rate, rg);

                  /* hook into changed signal */
                  evas_object_smart_callback_add(ow, "changed", 
                                                 _e_smart_monitor_cb_refresh_rate_changed, obj);

                  /* add this radio to the list */
                  e_widget_list_object_append(sd->o_refresh, ow, 1, 0, 0.5);
               }
          }
     }

   /* calculate minimum size for refresh list and set it */
   e_widget_size_min_get(sd->o_refresh, &mw, &mh);
   edje_extern_object_min_size_set(sd->o_refresh, mw, mh);

   /* swallow refresh list into frame and show it */
   edje_object_part_swallow(sd->o_frame, "e.swallow.refresh", sd->o_refresh);
   evas_object_show(sd->o_refresh);
}

static double 
_e_smart_monitor_refresh_rate_get(Ecore_X_Randr_Mode_Info *mode)
{
   double rate = 0.0;

   if (!mode) return 0.0;

   /* calculate rate */
   if ((mode->hTotal) && (mode->vTotal))
     rate = ((float)mode->dotClock / 
             ((float)mode->hTotal * (float)mode->vTotal));

   return rate;
}

static void 
_e_smart_monitor_modes_fill(E_Smart_Data *sd)
{
   /* clear out any old modes */
   if (sd->modes) eina_list_free(sd->modes);

   /* if we have an assigned monitor, copy the modes from that */
   if ((sd->output) && (sd->output->monitor))
     sd->modes = eina_list_clone(sd->output->monitor->modes);
   else if (sd->crtc)
     sd->modes = eina_list_clone(sd->crtc->outputs_common_modes);

   /* sort the mode list (smallest to largest) */
   if (sd->modes)
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
   if ((bg = e_bg_file_get(sd->con->num, sd->zone_num, dx, dy)))
     {
        Evas_Object *o;

        /* try to get the livethumb, if not then create an object */
        if (!(o = e_livethumb_thumb_get(sd->o_thumb)))
          o = edje_object_add(e_livethumb_evas_get(sd->o_thumb));

        /* tell the object to use this edje file & group */
        edje_object_file_set(o, bg, "e/desktop/background");

        /* tell the thumbnail to use this object for preview */
        e_livethumb_thumb_set(sd->o_thumb, o);
        eina_stringshare_del(bg);
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

   /* check this bg event happened on our container */
   if (((ev->container < 0) || (ev->container == (int)sd->con->num)) && 
       ((ev->zone < 0) || (ev->zone == sd->zone_num)))
     {
        /* check this bg event happened on our desktop */
        if (((ev->desk_x < 0) || (ev->desk_x == sd->current.x)) && 
            ((ev->desk_y < 0) || (ev->desk_y == sd->current.y)))
          {
             /* set the livethumb preview to the background of this desktop */
             _e_smart_monitor_background_set(sd, ev->desk_x, ev->desk_y);
          }
     }

   return ECORE_CALLBACK_PASS_ON;
}

static void 
_e_smart_monitor_resolution_set(E_Smart_Data *sd, Evas_Coord width, Evas_Coord height)
{
   char buff[1024];

   if (!sd) return;

   snprintf(buff, sizeof(buff), "%d x %d", width, height);

   /* set the frame's resolution text */
   edje_object_part_text_set(sd->o_frame, "e.text.resolution", buff);
}

static Ecore_X_Randr_Mode_Info *
_e_smart_monitor_resolution_get(E_Smart_Data *sd, Evas_Coord w, Evas_Coord h, Eina_Bool skip_rate_check)
{
   Ecore_X_Randr_Mode_Info *mode = NULL;
   Eina_List *l;

   if (!sd) return NULL;

   /* find the closest resolution to this size */
   if (!skip_rate_check)
     {
        EINA_LIST_REVERSE_FOREACH(sd->modes, l, mode)
          {
             if ((((int)mode->width - RESIZE_FUZZ) <= w) || 
                 (((int)mode->width + RESIZE_FUZZ) <= w))
               {
                  if ((((int)mode->height - RESIZE_FUZZ) <= h) || 
                      (((int)mode->height + RESIZE_FUZZ) <= h))
                    {
                       if ((mode->hTotal) && (mode->vTotal))
                         {
                            double rate = 0.0;

                            /* get the refresh rate for this mode */
                            rate = _e_smart_monitor_refresh_rate_get(mode);

                            /* compare this mode rate to current rate */
                            if (((int)rate == sd->current.refresh_rate))
                              return mode;
                         }
                    }
               }
          }
     }

   /* if we got here, then we found no mode which matches the current 
    * refresh rate and size. Search again, ignoring refresh rate */
   EINA_LIST_REVERSE_FOREACH(sd->modes, l, mode)
     {
        if ((((int)mode->width - RESIZE_FUZZ) <= w) || 
            (((int)mode->width + RESIZE_FUZZ) <= w))
          {
             if ((((int)mode->height - RESIZE_FUZZ) <= h) || 
                 (((int)mode->height + RESIZE_FUZZ) <= h))
               return mode;
          }
     }

   return NULL;
}

static int 
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

static int 
_e_smart_monitor_rotation_amount_get(E_Smart_Data *sd, Evas_Event_Mouse_Move *ev)
{
   Evas_Coord cx = 0, cy = 0;
   double a = 0.0, b = 0.0, c = 0.0, r = 0.0;
   double ax = 0.0, ay = 0.0, bx = 0.0, by = 0.0;
   double dotprod = 0.0;

   /* return a single rotation amount based on 
    * mouse movement in both directions */

   /* if there was no movement, return 0
    * 
    * NB: This smells quite odd to me. How can we get a mouse_move event 
    * (and end up in here) when the coordinates say otherwise ??
    * Must be a synthetic event and we are not interested in those */
   if ((ev->cur.canvas.x == ev->prev.canvas.x) && 
       (ev->cur.canvas.y == ev->prev.canvas.y))
     return 0;

   /* if the mouse is moved outside the monitor then get out
    * 
    * NB: This could be coded into one giant OR statement, but I am feeling 
    * lazy today ;) */
   if ((ev->cur.canvas.x > (sd->x + sd->w))) return 0;
   else if ((ev->cur.canvas.x < sd->x)) return 0;
   if ((ev->cur.canvas.y > (sd->y + sd->h))) return 0;
   else if ((ev->cur.canvas.y < sd->y)) return 0;

   /* get center point
    * 
    * NB: This COULD be used to provide a greater amount of rotation 
    * depending on distance of movement from center */
   cx = (sd->x + (sd->w / 2));
   cy = (sd->y + (sd->h / 2));

   ax = ((sd->x + sd->w) - cx);
   ay = (sd->y - cy);

   bx = (ev->cur.canvas.x - cx);
   by = (ev->cur.canvas.y - cy);

   /* calculate degrees of rotation
    * 
    * NB: A HUGE Thank You to Daniel for the help here !! */
   a = sqrt((ax * ax) + (ay * ay));
   b = sqrt((bx * bx) + (by * by));
   if ((a < 1) || (b < 1)) return 0;

   c = sqrt((ev->cur.canvas.x - (sd->x + sd->w)) * 
            (ev->cur.canvas.x - (sd->x + sd->w)) +
            (ev->cur.canvas.y - sd->y) * 
            (ev->cur.canvas.y - sd->y));

   r = acos(((a * a) + (b * b) - (c * c)) / (2 * (a * b)));
   r = r * 180 / M_PI;

   dotprod = ((ay * bx) + (-ax * by));
   if (dotprod > 0) r = 360 - r;

   return r;
}

static Ecore_X_Randr_Orientation 
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
_e_smart_monitor_pointer_push(Evas_Object *obj, const char *ptr)
{
   Evas_Object *ow;
   E_Win *win;

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

   /* try to find the E_Win for this object */
   if (!(ow = evas_object_name_find(evas_object_evas_get(obj), "E_Win")))
     return;
   if (!(win = evas_object_data_get(ow, "E_Win"))) return;

   /* tell E to reset the pointer */
   e_pointer_type_pop(win->pointer, obj, ptr);
}

static void 
_e_smart_monitor_map_apply(Evas_Object *obj, int rotation)
{
   Evas_Coord fx = 0, fy = 0, fw = 0, fh = 0;
   static Evas_Map *map = NULL;

   /* get the geometry of the frame */
   evas_object_geometry_get(obj, &fx, &fy, &fw, &fh);

   /* create a new evas map */
   if (!map) map = evas_map_new(4);

   /* set map properties */
   evas_map_smooth_set(map, EINA_TRUE);
   evas_map_alpha_set(map, EINA_TRUE);
   evas_map_util_points_populate_from_object_full(map, obj, rotation);

   /* apply current rotation */
   evas_map_util_rotate(map, rotation, (fx + (fw / 2)), (fy + (fh / 2)));

   /* tell object to use this map */
   evas_object_map_set(obj, map);
   evas_object_map_enable_set(obj, EINA_TRUE);
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

   /* if this monitor is cloned into another one, then do not process 
    * any mouse move events */
   if (sd->cloned) return;

   ev = event;

   /* calculate the difference in mouse movement */
   dx = (ev->cur.output.x - ev->prev.output.x);
   dy = (ev->cur.output.y - ev->prev.output.y);

   /* get monitors current geometry */
   e_layout_child_geometry_get(mon, &mx, &my, &mw, &mh);

   /* convert these coords to virtual space */
   e_layout_coord_canvas_to_virtual(sd->layout.obj, (sd->layout.x + dx), 
                                    (sd->layout.y + dy), &nx, &ny);

   /* factor monitor size into mouse movement */
   nx += mx;
   ny += my;

   if ((sd->current.orientation == ECORE_X_RANDR_ORIENTATION_ROT_0) || 
       (sd->current.orientation == ECORE_X_RANDR_ORIENTATION_ROT_180))
     {
        /* constrain to the layout bounds */
        if (nx < 0) nx = 0;
        if (ny < 0) ny = 0;
     }
   else if ((sd->current.orientation == ECORE_X_RANDR_ORIENTATION_ROT_90) || 
            (sd->current.orientation == ECORE_X_RANDR_ORIENTATION_ROT_270))
     {
        int sx = 0, sy = 0;

        sx = -(((mh - mw) / 2) + sd->layout.x);
        sy = -(((mw - mh) / 2) - sd->layout.y);

        /* constrain to the layout bounds */
        if (nx < sx) nx = sx;
        if (ny < sy) ny = sy;
     }

   /* constrain to the layout bounds */
   if ((nx + mw) > sd->layout.vw)
     nx = (sd->layout.vw - mw);
   if ((ny + mh) > sd->layout.vh)
     ny = (sd->layout.vh - mh);

   if ((nx != mx) || (ny != my))
     {
        /* actually move the monitor */
        e_layout_child_move(mon, nx, ny);

        /* send monitor moving signal */
        evas_object_smart_callback_call(mon, "monitor_moving", NULL);
     }
}

static void 
_e_smart_monitor_resize_event(E_Smart_Data *sd, Evas_Object *mon, void *event)
{
   Evas_Event_Mouse_Move *ev;
   Evas_Coord dx = 0, dy = 0;
   Evas_Coord mw = 0, mh = 0;
   Evas_Coord nw = 0, nh = 0;
   Ecore_X_Randr_Mode_Info *mode = NULL;

   /* check for valid smart data */
   if (!sd) return;

   /* if this monitor is cloned into another one, then do not process 
    * any mouse move events */
   if (sd->cloned) return;

   ev = event;

   /* factor in drag resistance to movement and if we have not moved the 
    * mouse enough, then get out */
   dx = (sd->resize.x - ev->cur.canvas.x);
   dy = (sd->resize.y - ev->cur.canvas.y);
   if (((dx * dx) + (dy * dy)) < 
       (e_config->drag_resist * e_config->drag_resist)) 
     return;

   /* TODO: NB: Hmmm, want to badly enable the below code to stop 
    * extra processing when the mouse moves outside of the monitor, 
    * HOWEVER when enabled it creates a torrent of problems for 
    * resizing with snap mode :( */

   /* don't process mouse movements that are outside the object
    * 
    * NB: We add a few pixels of 'fuzziness' here to help with monitor resize 
    * hit detection when at the lowest resolution */
   /* if (!E_INSIDE(ev->cur.canvas.x, ev->cur.canvas.y,  */
   /*               (sd->x - 10), (sd->y - 10), (sd->w + 10), (sd->h + 10))) */
   /*   return; */

   /* calculate resize difference based on mouse movement */
   dx = (ev->cur.canvas.x - ev->prev.canvas.x);
   dy = (ev->cur.canvas.y - ev->prev.canvas.y);

   /* TODO: FIXME: Handle case where monitor is rotated */

   /* convert monitor size to canvas coords */
   e_layout_coord_virtual_to_canvas(sd->layout.obj, sd->current.w, 
                                    sd->current.h, &mw, &mh);

   /* factor in the resize difference and convert to virtual coords */
   e_layout_coord_canvas_to_virtual(sd->layout.obj, 
                                    (mw + dx), (mh + dy), &nw, &nh);

   /* don't process mouse movements that are outside the object
    * 
    * NB: This is a different 'hack' to the E_INSIDE code above */
   if (dx < 0)
     {
        /* while trying to resize smaller, if the mouse is outside the 
         * monitor then get out */
        if ((ev->cur.canvas.x > (sd->x + sd->w))) return;

        /* make sure the min requested width is not smaller than the 
         * smallest resolution */
        if (nw < sd->min.w) nw = sd->min.w;
     }
   if (dy < 0)
     {
        /* while trying to resize smaller, if the mouse is outside the 
         * monitor then get out */
        if ((ev->cur.canvas.y > (sd->y + sd->h))) return;

        /* make sure the min requested height is not smaller than the 
         * smallest resolution */
        if (nh < sd->min.h) nh = sd->min.h;
     }

   if (dx > 0)
     {
        /* while trying to resize larger, if the mouse is outside the 
         * monitor then get out */
        if (ev->cur.canvas.x < sd->x) return;

        /* make sure the max requested width is not larger than the 
         * largest resolution */
        if (nw > sd->max.w) nw = sd->max.w;
     }
   if (dy > 0)
     {
        /* while trying to resize larger, if the mouse is outside the 
         * monitor then get out */
        if (ev->cur.canvas.y < sd->y) return;

        /* make sure the max requested height is not larger than the 
         * largest resolution */
        if (nh > sd->max.h) nh = sd->max.h;
     }

   /* check if size already matches, if so we have nothing to do */
   if ((nw == sd->current.w) && (nh == sd->current.h)) return;

   /* TODO: Make both types of resizing here (freeform and snap) a 
    * checkbox option on the dialog maybe ?? */

// ************************* BEGIN FREEFORM RESIZING ************************
#if 0
   /* actually resize the monitor (freeform) */
   e_layout_child_resize(mon, nw, nh);
#endif
// ************************* END FREEFORM RESIZING **************************

   /* reset current size to match */
   sd->current.w = nw;
   sd->current.h = nh;

// ************************* BEGIN SNAP RESIZING ************************

   /* find the next nearest resolution to this new size */
   if ((mode = _e_smart_monitor_resolution_get(sd, nw, nh, EINA_TRUE)))
     {
        if (sd->current.mode != mode)
          {
             /* reset current mode to match */
             sd->current.mode = mode;

             /* actually resize the monitor (snap) */
             e_layout_child_resize(mon, mode->width, mode->height);

             /* reset current size to match */
             sd->current.w = mode->width;
             sd->current.h = mode->height;

             /* set the resolution text */
             _e_smart_monitor_resolution_set(sd, sd->current.mode->width, 
                                            sd->current.mode->height);
          }
     }

// ************************* END SNAP RESIZING **************************
}

static void 
_e_smart_monitor_rotate_event(E_Smart_Data *sd, Evas_Object *mon EINA_UNUSED, void *event)
{
   Evas_Event_Mouse_Move *ev;
   int rotation = 0;

   /* check for valid smart data */
   if (!sd) return;

   /* if this monitor is cloned into another one, then do not process 
    * any mouse move events */
   if (sd->cloned) return;

   ev = event;

   /* get amount of rotation from the mouse event */
   rotation = _e_smart_monitor_rotation_amount_get(sd, ev);

   /* if we have no rotation to map, get out */
   if (rotation == 0) return;

   /* factor in any existing rotation */
   rotation += sd->current.rotation;

   /* update rotation value */
   sd->current.rotation = rotation;

   /* apply existing rotation */
   _e_smart_monitor_map_apply(sd->o_frame, sd->current.rotation);

   /* NB: The 'snapping' of this rotation (in relation to other monitors) 
    * occurs in the randr widget so we will just 
    * raise a signal here to tell it that we rotated */

   /* NB: For now, don't send this signal here. We will send it when the 
    * user is finished rotating */

   /* send monitor rotated signal */
   /* evas_object_smart_callback_call(mon, "monitor_rotated", NULL); */
}

/* local callbacks */
static void 
_e_smart_monitor_cb_refresh_rate_changed(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Evas_Object *mon;
   E_Smart_Data *sd;
   Ecore_X_Randr_Mode_Info *mode = NULL;
   Eina_List *l = NULL;

   if (!(mon = data)) return;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(mon))) return;

   /* loop the modes and find the current one */
   EINA_LIST_FOREACH(sd->modes, l, mode)
     {
        /* compare mode names */
        if (!strcmp(mode->name, sd->current.mode->name))
          {
             int rate = 0;

             /* get the refresh rate for this mode */
             rate = (int)_e_smart_monitor_refresh_rate_get(mode);

             /* compare to the currently requested refresh rate */
             if (rate == sd->current.refresh_rate)
               {
                  /* set new mode */
                  sd->current.mode = mode;

                  break;
               }
          }
     }

   /* update changes */
   if (sd->orig.refresh_rate != sd->current.refresh_rate)
     sd->changes |= E_SMART_MONITOR_CHANGED_REFRESH;
   else
     sd->changes &= ~(E_SMART_MONITOR_CHANGED_REFRESH);

   /* send monitor changed signal */
   evas_object_smart_callback_call(mon, "monitor_changed", NULL);
}

static void 
_e_smart_monitor_frame_cb_mouse_move(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event)
{
   Evas_Object *mon;
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(mon = data)) return;
   if (!(sd = evas_object_smart_data_get(mon))) return;

   /* call appropriate functions based on current action */
   if (sd->moving)
     _e_smart_monitor_move_event(sd, mon, event);
   else if (sd->resizing)
     _e_smart_monitor_resize_event(sd, mon, event);
   else if (sd->rotating)
     _e_smart_monitor_rotate_event(sd, mon, event);
}

static void 
_e_smart_monitor_frame_cb_resize_in(void *data EINA_UNUSED, Evas_Object *obj, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   /* try to set the pointer to indicate we can be resized */
   _e_smart_monitor_pointer_push(obj, "resize_br");
}

static void 
_e_smart_monitor_frame_cb_resize_out(void *data EINA_UNUSED, Evas_Object *obj, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   /* try to reset the pointer back to default */
   _e_smart_monitor_pointer_pop(obj, "resize_br");
}

static void 
_e_smart_monitor_frame_cb_resize_start(void *data, Evas_Object *obj EINA_UNUSED, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   Evas_Object *mon;
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(mon = data)) return;
   if (!(sd = evas_object_smart_data_get(mon))) return;

   /* record mouse position for drag resistance */
   evas_pointer_canvas_xy_get(sd->evas, &sd->resize.x, &sd->resize.y);

   /* get monitor geometry */
   e_layout_child_geometry_get(mon, NULL, NULL, 
                               &sd->current.w, &sd->current.h);

   /* raise this monitor */
   e_layout_child_raise(mon);

   /* set resizing flag */
   sd->resizing = EINA_TRUE;
}

static void 
_e_smart_monitor_frame_cb_resize_stop(void *data, Evas_Object *obj EINA_UNUSED, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   Evas_Object *mon;
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(mon = data)) return;
   if (!(sd = evas_object_smart_data_get(mon))) return;

   /* set resizing flag */
   sd->resizing = EINA_FALSE;

   /* update the changes flag */
   if (sd->orig.mode)
     {
        if (sd->orig.mode->xid != sd->current.mode->xid)
          sd->changes |= E_SMART_MONITOR_CHANGED_RESOLUTION;
        else
          sd->changes &= ~(E_SMART_MONITOR_CHANGED_RESOLUTION);
     }
   else
     sd->changes |= E_SMART_MONITOR_CHANGED_RESOLUTION;

   /* NB: The 'snapping' of this resize (in relation to other monitors) 
    * occurs in the randr widget so we will just 
    * raise a signal here to tell it that we resized */

   /* send monitor resized signal */
   evas_object_smart_callback_call(mon, "monitor_resized", NULL);
}

static void 
_e_smart_monitor_frame_cb_rotate_in(void *data EINA_UNUSED, Evas_Object *obj, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   /* try to set the pointer to indicate rotation */
   _e_smart_monitor_pointer_push(obj, "rotate");
}

static void 
_e_smart_monitor_frame_cb_rotate_out(void *data EINA_UNUSED, Evas_Object *obj, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   /* try to set the pointer to indicate rotation */
   _e_smart_monitor_pointer_pop(obj, "rotate");
}

static void 
_e_smart_monitor_frame_cb_rotate_start(void *data, Evas_Object *obj EINA_UNUSED, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   Evas_Object *mon;
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(mon = data)) return;
   if (!(sd = evas_object_smart_data_get(mon))) return;

   /* raise this monitor */
   e_layout_child_raise(mon);

   /* set rotating flag */
   sd->rotating = EINA_TRUE;
}

static void 
_e_smart_monitor_frame_cb_rotate_stop(void *data, Evas_Object *obj EINA_UNUSED, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   Evas_Object *mon;
   E_Smart_Data *sd;
   Ecore_X_Randr_Orientation orient;
   Evas_Coord nw = 0, nh = 0;
   int rot = 0;

   /* try to get the objects smart data */
   if (!(mon = data)) return;
   if (!(sd = evas_object_smart_data_get(mon))) return;

   /* set rotating flag */
   sd->rotating = EINA_FALSE;

   /* get the orientation that this monitor would be in */
   orient = _e_smart_monitor_orientation_get(sd->current.rotation);

   /* if the orientation matches, then no changes have occured and we can 
    * get out of here */
   if (sd->current.orientation == orient) return;

   /* grab the current geometry */
   e_layout_child_geometry_get(mon, NULL, NULL, &nw, &nh);

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
   rot = _e_smart_monitor_rotation_get(orient);
   if (rot != sd->current.rotation)
     {
        /* update rotation value */
        sd->current.rotation = rot;

        /* apply existing rotation */
        _e_smart_monitor_map_apply(sd->o_frame, sd->current.rotation);
     }

   /* snap the monitor to this rotation */

   /* check orientation */
   if ((orient == ECORE_X_RANDR_ORIENTATION_ROT_90) || 
       (orient == ECORE_X_RANDR_ORIENTATION_ROT_270))
     {
        if ((sd->current.orientation != ECORE_X_RANDR_ORIENTATION_ROT_90) || 
            (sd->current.orientation != ECORE_X_RANDR_ORIENTATION_ROT_270))
          {
             Evas_Coord nx = 0, ny = 0;
             int sx = 0, sy = 0;

             /* resize monitor object based on rotation */
             e_layout_child_resize(mon, nh, nw);

             /* set the resolution text */
             _e_smart_monitor_resolution_set(sd, sd->current.mode->height, 
                                            sd->current.mode->width);

             /* grab the current geometry */
             e_layout_child_geometry_get(mon, &nx, &ny, &nw, &nh);

             sx = ((nh - nw) / 2);
             sy = ((nw - nh) / 2);

             nx -= (sx + sd->layout.x);
             ny -= (sy - sd->layout.y);

             /* NB: Hmmm, should this also raise a moved signal ?? */
             e_layout_child_move(mon, nx, ny);
          }
     }
   else if ((orient == ECORE_X_RANDR_ORIENTATION_ROT_0) || 
            (orient == ECORE_X_RANDR_ORIENTATION_ROT_180))
     {
        if ((sd->current.orientation != ECORE_X_RANDR_ORIENTATION_ROT_0) || 
            (sd->current.orientation != ECORE_X_RANDR_ORIENTATION_ROT_180))
          {
             /* resize monitor object based on rotation */
             e_layout_child_resize(mon, nh, nw);

             /* set the resolution text */
             _e_smart_monitor_resolution_set(sd, sd->current.mode->width, 
                                            sd->current.mode->height);

             /* NB: Hmmm, should this also raise a moved signal ?? */
             e_layout_child_move(mon, sd->current.x, sd->current.y);
          }
     }

   /* update current orientation */
   sd->current.orientation = orient;

   /* update the changes flag */
   if (sd->orig.orientation != sd->current.orientation)
     sd->changes |= E_SMART_MONITOR_CHANGED_ROTATION;
   else
     sd->changes &= ~(E_SMART_MONITOR_CHANGED_ROTATION);

   /* NB: The 'snapping' of this rotation occurs in the randr widget 
    * so we will just raise a signal here to tell it that we rotated */

   /* send monitor rotated signal */
   evas_object_smart_callback_call(mon, "monitor_rotated", NULL);
}

static void 
_e_smart_monitor_frame_cb_indicator_in(void *data EINA_UNUSED, Evas_Object *obj, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   /* try to set the pointer to indicate we can be clicked */
   _e_smart_monitor_pointer_push(obj, "plus");
}

static void 
_e_smart_monitor_frame_cb_indicator_out(void *data EINA_UNUSED, Evas_Object *obj, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   /* try to reset the pointer back to default */
   _e_smart_monitor_pointer_pop(obj, "plus");
}

static void 
_e_smart_monitor_frame_cb_indicator_toggle(void *data, Evas_Object *obj EINA_UNUSED, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   Evas_Object *mon;
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(mon = data)) return;
   if (!(sd = evas_object_smart_data_get(mon))) return;

   if (sd->current.enabled)
     {
        /* if we Were enabled, switch to disabled and tell the edj object */
        sd->current.enabled = EINA_FALSE;
        edje_object_signal_emit(sd->o_frame, "e,state,disabled", "e");
     }
   else
     {
        /* if we Were disabled, switch to enabled and tell the edj object */
        sd->current.enabled = EINA_TRUE;
        edje_object_signal_emit(sd->o_frame, "e,state,enabled", "e");
     }

   /* update the changes flag */
   if (sd->orig.enabled != sd->current.enabled)
     sd->changes |= E_SMART_MONITOR_CHANGED_ENABLED;
   else
     sd->changes &= ~(E_SMART_MONITOR_CHANGED_ENABLED);

   /* NB: The 'enabling' of this monitor occurs in the randr widget 
    * so we will just raise a signal here to tell it that we toggled */

   /* send monitor changed signal */
   evas_object_smart_callback_call(mon, "monitor_changed", NULL);
}

static void 
_e_smart_monitor_thumb_cb_mouse_in(void *data EINA_UNUSED, Evas *evas EINA_UNUSED, Evas_Object *obj, void *event EINA_UNUSED)
{
   /* try to set the pointer to indicate we can be clicked */
   _e_smart_monitor_pointer_push(obj, "hand");
}

static void 
_e_smart_monitor_thumb_cb_mouse_out(void *data EINA_UNUSED, Evas *evas EINA_UNUSED, Evas_Object *obj, void *event EINA_UNUSED)
{
   /* try to reset the pointer back to default */
   _e_smart_monitor_pointer_pop(obj, "hand");
}

static void 
_e_smart_monitor_thumb_cb_mouse_down(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj, void *event)
{
   Evas_Event_Mouse_Down *ev;

   ev = event;
   if (ev->button == 1)
     {
        Evas_Object *mon;
        E_Smart_Data *sd;

        /* try to get the objects smart data */
        if (!(mon = data)) return;
        if (!(sd = evas_object_smart_data_get(mon))) return;

        /* if this event is not on a cloned monitor */
        if (!sd->cloned)
          {
             /* try to set the mouse pointer to indicate moving */
             _e_smart_monitor_pointer_push(obj, "move");

             /* get the current geometry of this monitor and record it */
             e_layout_child_geometry_get(mon, &sd->cx, &sd->cy, 
                                         &sd->cw, &sd->ch);

             /* set moving flag */
             sd->moving = EINA_TRUE;

             /* raise this monitor */
             e_layout_child_raise(mon);
          }
     }
}

static void 
_e_smart_monitor_thumb_cb_mouse_up(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj, void *event)
{
   Evas_Event_Mouse_Up *ev;

   ev = event;
   if (ev->button == 1)
     {
        Evas_Object *mon;
        E_Smart_Data *sd;

        /* try to get the objects smart data */
        if (!(mon = data)) return;
        if (!(sd = evas_object_smart_data_get(mon))) return;

        /* check if this is a cloned monitor */
        if (sd->cloned)
          {
             /* un-clone this monitor */
             e_smart_monitor_clone_del(sd->parent, mon);

             /* done here. exit the function */
             return;
          }

        /* try to set the mouse pointer to indicate moving is done */
        _e_smart_monitor_pointer_pop(obj, "move");

        /* set moving state */
        sd->moving = EINA_FALSE;

        if ((sd->current.orientation == ECORE_X_RANDR_ORIENTATION_ROT_0) || 
            (sd->current.orientation == ECORE_X_RANDR_ORIENTATION_ROT_180))
          {
             Evas_Coord nx = 0, ny = 0;

             /* get current geometry */
             e_layout_child_geometry_get(mon, &nx, &ny, NULL, NULL);

             /* check if geometry has actually been changed */
             if ((sd->current.x != nx) || (sd->current.y != ny))
               {
                  /* update current geometry */
                  sd->current.x = nx;
                  sd->current.y = ny;
               }
          }

        /* update the changes flag */
        if ((sd->orig.x != sd->current.x) || (sd->orig.y != sd->current.y))
          sd->changes |= E_SMART_MONITOR_CHANGED_POSITION;
        else
          sd->changes &= ~(E_SMART_MONITOR_CHANGED_POSITION);

        /* NB: The 'snapping' of this movement occurs in the randr widget 
         * so we will just raise a signal here to tell it that we moved */

        /* send monitor moved signal */
        evas_object_smart_callback_call(mon, "monitor_moved", NULL);
     }
}

static void 
_e_smart_monitor_layout_cb_move(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   E_Smart_Data *sd;

   if (!(sd = data)) return;

   /* get the layout's geometry and store it in our smart data structure */
   evas_object_geometry_get(sd->layout.obj, 
                            &sd->layout.x, &sd->layout.y, NULL, NULL);
}
