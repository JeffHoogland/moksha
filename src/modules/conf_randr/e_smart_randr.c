#include "e.h"
#include "e_mod_main.h"
#include "e_smart_randr.h"
#include "e_smart_monitor.h"

#define SNAP_FUZZ 100

/*
 * TODO:
 * 
 * Add Poller for Output Change events to listen for hotplug (4 seconds)
 * 
 */

/* local structures */
typedef struct _E_Smart_Data E_Smart_Data;
struct _E_Smart_Data
{
   /* base object */
   Evas_Object *o_base;

   /* grid object */
   Evas_Object *o_grid;

   /* virtual size */
   Evas_Coord vw, vh;

   /* visible flag */
   Eina_Bool visible : 1;

   /* list of monitors */
   Eina_List *monitors;
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

static void _e_smart_randr_grid_cb_move(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED);
static void _e_smart_randr_grid_cb_resize(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED);

static Evas_Object *_e_smart_randr_monitor_crtc_find(E_Smart_Data *sd, Ecore_X_Randr_Crtc crtc);
static void _e_smart_randr_monitor_cb_changed(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED);
static void _e_smart_randr_monitor_cb_moved(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED);
static void _e_smart_randr_monitor_cb_resized(void *data, Evas_Object *obj, void *event EINA_UNUSED);

static void _e_smart_randr_monitor_position_update(E_Smart_Data *sd, Evas_Object *obj, Evas_Object *skip);
static void _e_smart_randr_monitor_position_normalize(E_Smart_Data *sd);

static void _e_smart_randr_monitor_preferred_mode_size_get(Ecore_X_Randr_Output output, Evas_Coord *mw, Evas_Coord *mh);
static Evas_Object *_e_smart_randr_clone_find(Ecore_X_Window root, Ecore_X_Randr_Output output, Ecore_X_Randr_Crtc crtc, Evas_Coord cx, Evas_Coord cy, E_Smart_Data *sd);
static Ecore_X_Randr_Crtc _e_smart_randr_crtc_find(Ecore_X_Randr_Output output);

/* external functions exposed by this widget */
Evas_Object *
e_smart_randr_add(Evas *evas)
{
   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   static Evas_Smart *smart = NULL;
   static const Evas_Smart_Class sc = 
     {
        "smart_randr", EVAS_SMART_CLASS_VERSION, 
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
e_smart_randr_virtual_size_calc(Evas_Object *obj)
{
   E_Smart_Data *sd;
   Ecore_X_Window root = 0;
   Ecore_X_Randr_Output *routputs;
   Evas_Coord vw = 0, vh = 0;
   int noutputs = 0;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* NB: The old code here used to get the modes from the e_randr_cfg.
    * I changed it to get directly from Xrandr because of attempts to 
    * run this in Xephyr. Getting the information from e_randr_cfg was not 
    * practical in those cases */

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* grab the root window */
   root = ecore_x_window_root_first_get();

   /* get list of outputs */
   if ((routputs = ecore_x_randr_outputs_get(root, &noutputs)))
     {
        int j = 0;
        intptr_t *o;
        Eina_List *outputs = NULL;

        for (j = 0; j < noutputs; j++)
          {
             Ecore_X_Randr_Connection_Status status = 
               ECORE_X_RANDR_CONNECTION_STATUS_UNKNOWN;

             status = 
               ecore_x_randr_output_connection_status_get(root, routputs[j]);
             if (status != ECORE_X_RANDR_CONNECTION_STATUS_CONNECTED) continue;

             outputs = 
               eina_list_append(outputs, (intptr_t *)(long)routputs[j]);
          }

        /* loop the outputs and get the largest mode */
        EINA_LIST_FREE(outputs, o)
          {
             Ecore_X_Randr_Output output;
             Ecore_X_Randr_Mode *modes;
             Evas_Coord mw = 0, mh = 0;
             int nmode = 0;

             output = (int)(long)o;

             /* try to get the list of modes for this output */
             modes = 
               ecore_x_randr_output_modes_get(root, output, 
                                              &nmode, NULL);
             if (!modes) continue;

             /* get the size of the largest mode */
             ecore_x_randr_mode_size_get(root, modes[0], &mw, &mh);

             vw += MAX(mw, mh);
             vh += MAX(mw, mh);

             /* free any allocated memory from ecore_x_randr */
             free(modes);
          }

        free(routputs);
     }

   if ((vw == 0) && (vh == 0))
     {
        /* by default, set virtual size to the current screen size */
        ecore_x_randr_screen_current_size_get(root, &vw, &vh, NULL, NULL);
     }

   sd->vw = vw;
   sd->vh = vh;

   /* set the grid size */
   evas_object_grid_size_set(sd->o_grid, vw, vh);
}

void 
e_smart_randr_monitors_create(Evas_Object *obj)
{
   E_Smart_Data *sd;
   Evas *evas;
   Ecore_X_Window root = 0;
   Evas_Coord gx = 0, gy = 0, gw = 0, gh = 0;
   Ecore_X_Randr_Output *outputs;
   Evas_Object *mon;
   Eina_List *l = NULL;
   int noutputs = 0, count = 0;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* NB: The old code here used to get the outputs from the e_randr_cfg.
    * I changed it to get directly from Xrandr because of attempts to 
    * run this in Xephyr. Getting the information from e_randr_cfg was not 
    * practical in those cases */

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* grab the canvas of the grid object */
   evas = evas_object_evas_get(sd->o_grid);

   /* get the geometry of the grid */
   evas_object_geometry_get(sd->o_grid, &gx, &gy, &gw, &gh);

   /* grab the root window */
   root = ecore_x_window_root_first_get();

   /* get a list of outputs from X */
   if ((outputs = ecore_x_randr_outputs_get(root, &noutputs)))
     {
        int i = 0;
        Evas_Coord nx = 0;
        Eina_List *connected = NULL;

        /* loop these outputs */
        for (i = 0; i < noutputs; i++)
          {
             Ecore_X_Randr_Crtc crtc = 0;
             Evas_Coord mw = 0, mh = 0, crtcx = 0, crtcy = 0;
             Evas_Coord cx = 0, cy = 0, cw = 0, ch = 0;
             Ecore_X_Randr_Connection_Status status = 
               ECORE_X_RANDR_CONNECTION_STATUS_UNKNOWN;
             Ecore_X_Randr_Mode mode = 0;

             /* ask X if this output is connected */
             status = 
               ecore_x_randr_output_connection_status_get(root, outputs[i]);

             /* if it's not connected, skip it */
             if (status != ECORE_X_RANDR_CONNECTION_STATUS_CONNECTED)
               continue;

             connected = 
               eina_list_append(connected, (intptr_t *)(long)outputs[i]);

             /* for each output, try to create a monitor */
             if (!(mon = e_smart_monitor_add(evas)))
               continue;

             /* hook into monitor changed callback */
             evas_object_smart_callback_add(mon, "monitor_changed", 
                                            _e_smart_randr_monitor_cb_changed, obj);
             evas_object_smart_callback_add(mon, "monitor_moved", 
                                            _e_smart_randr_monitor_cb_moved, obj);
             evas_object_smart_callback_add(mon, "monitor_resized", 
                                            _e_smart_randr_monitor_cb_resized, obj);

             /* add this monitor to our list */
             sd->monitors = eina_list_append(sd->monitors, mon);

             /* tell monitor what the grid's virtual size is */
             e_smart_monitor_grid_virtual_size_set(mon, sd->vw, sd->vh);

             /* tell monitor what the grid is and it's geometry */
             e_smart_monitor_grid_set(mon, sd->o_grid, gx, gy, gw, gh);

             /* try to get the crtc of this output. If it does not have one 
              * we will try to find a usable one */
             if (!(crtc = ecore_x_randr_output_crtc_get(root, outputs[i])))
               crtc = _e_smart_randr_crtc_find(outputs[i]);

             /* get the geometry for this crtc */
             ecore_x_randr_crtc_geometry_get(root, crtc, 
                                             &cx, &cy, &cw, &ch);

             e_smart_monitor_crtc_set(mon, crtc, cx, cy, cw, ch);

             mode = ecore_x_randr_crtc_mode_get(root, crtc);

             /* if this crtc has no mode, or it's size is 0, 
              * then it's disabled */
             if ((!mode) || ((cw == 0) && (ch == 0)))
               {
                  /* get the size of the preferred mode for this output */
                  _e_smart_randr_monitor_preferred_mode_size_get(outputs[i], 
                                                                 &mw, &mh);
                  if ((mw == 0) && (mh == 0))
                    ecore_x_randr_crtc_size_get(root, crtc, &mw, &mh);

                  /* safety */
                  if ((mw == 0) && (mh == 0))
                    {
                       mw = 640;
                       mh = 480;
                    }

                  /* tell monitor what it's current position is
                   * NB: This also packs into the grid */
                  e_smart_monitor_current_geometry_set(mon, nx, 0, mw, mh);

                  /* tell monitor to set the background preview */
                  e_smart_monitor_background_set(mon, nx, 0);

                  crtcx = nx;
                  crtcy = 0;
                  nx += mw;
               }
             else
               {
                  /* tell monitor what it's current position is
                   * NB: This also packs into the grid */
                  e_smart_monitor_current_geometry_set(mon, cx, cy, 
                                                       cw, ch);

                  /* tell monitor to set the background preview */
                  e_smart_monitor_background_set(mon, cx, cy);

                  crtcx = cx;
                  crtcy = cy;
                  nx += cw;
               }

             /* tell monitor what output it uses */
             e_smart_monitor_output_set(mon, outputs[i]);

             if (eina_list_count(connected) > 0)
               {
                  intptr_t *o;
                  Evas_Object *pmon;

                  EINA_LIST_FOREACH(connected, l, o)
                    {
                       Ecore_X_Randr_Output out;

                       out = (int)(long)o;
                       if (out == outputs[i]) continue;

                       pmon = 
                         _e_smart_randr_clone_find(root, out, crtc, 
                                                   crtcx, crtcy, sd);

                       if (pmon) 
                         {
                            e_smart_monitor_clone_set(mon, pmon);
                            break;
                         }
                    }
               }
          }

        free(outputs);
     }

   /* check if we have only one monitor. If so, we will disable the 
    * indicator toggle so dumb people cannot turn off their only monitor */
   count = eina_list_count(sd->monitors);
   EINA_LIST_FOREACH(sd->monitors, l, mon)
     {
        if (count > 1)
          e_smart_monitor_indicator_available_set(mon, EINA_TRUE);
        else
          e_smart_monitor_indicator_available_set(mon, EINA_FALSE);
     }
}

void 
e_smart_randr_min_size_get(Evas_Object *obj, Evas_Coord *mw, Evas_Coord *mh)
{
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   if (mw) *mw = (sd->vw / 10);
   if (mh) *mh = (sd->vh / 10);
}

Eina_Bool 
e_smart_randr_changed_get(Evas_Object *obj)
{
   E_Smart_Data *sd;
   Eina_Bool changed = EINA_FALSE;
   Eina_List *l = NULL;
   Evas_Object *mon;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return EINA_FALSE;

   EINA_LIST_FOREACH(sd->monitors, l, mon)
     {
        E_Smart_Monitor_Changes changes = E_SMART_MONITOR_CHANGED_NONE;

        changes = e_smart_monitor_changes_get(mon);
        if (changes > E_SMART_MONITOR_CHANGED_NONE)
          {
             changed = EINA_TRUE;
             break;
          }
     }

   return changed;
}

void 
e_smart_randr_changes_apply(Evas_Object *obj)
{
   E_Smart_Data *sd;
   Eina_List *l = NULL;
   Evas_Object *mon;
   Eina_Bool need_reset = EINA_FALSE;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* tell each monitor to apply it's changes */
   EINA_LIST_FOREACH(sd->monitors, l, mon)
     if (e_smart_monitor_changes_apply(mon))
       need_reset = EINA_TRUE;

   if (need_reset) 
     ecore_x_randr_screen_reset(ecore_x_window_root_first_get());
}

Eina_List *
e_smart_randr_monitors_get(Evas_Object *obj)
{
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return NULL;
   return sd->monitors;
}

/* local functions */
static void 
_e_smart_add(Evas_Object *obj)
{
   E_Smart_Data *sd;
   Evas *evas;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to allocate the smart data structure */
   if (!(sd = E_NEW(E_Smart_Data, 1))) return;

   /* grab the canvas */
   evas = evas_object_evas_get(obj);

   sd->o_base = edje_object_add(evas);
   e_theme_edje_object_set(sd->o_base, "base/theme/widgets", 
                           "e/conf/randr/main");
   evas_object_smart_member_add(sd->o_base, obj);

   /* create the virtual grid */
   sd->o_grid = evas_object_grid_add(evas);
   edje_object_part_swallow(sd->o_base, "e.swallow.content", sd->o_grid);

   /* setup grid move callback */
   evas_object_event_callback_add(sd->o_grid, EVAS_CALLBACK_MOVE, 
                                  _e_smart_randr_grid_cb_move, sd);
   evas_object_event_callback_add(sd->o_grid, EVAS_CALLBACK_RESIZE, 
                                  _e_smart_randr_grid_cb_resize, sd);

   /* set the object's smart data */
   evas_object_smart_data_set(obj, sd);
}

static void 
_e_smart_del(Evas_Object *obj)
{
   E_Smart_Data *sd;
   Evas_Object *mon;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* free the monitors */
   EINA_LIST_FREE(sd->monitors, mon)
     {
        evas_object_smart_callback_del(mon, "monitor_changed", 
                                       _e_smart_randr_monitor_cb_changed);
        evas_object_smart_callback_del(mon, "monitor_moved", 
                                       _e_smart_randr_monitor_cb_moved);
        evas_object_smart_callback_del(mon, "monitor_resized", 
                                       _e_smart_randr_monitor_cb_resized);
        evas_object_del(mon);
     }

   /* remove grid move callback */
   evas_object_event_callback_del(sd->o_grid, EVAS_CALLBACK_MOVE, 
                                  _e_smart_randr_grid_cb_move);
   evas_object_event_callback_del(sd->o_grid, EVAS_CALLBACK_RESIZE, 
                                  _e_smart_randr_grid_cb_resize);

   /* delete the grid object */
   evas_object_del(sd->o_grid);

   /* delete the base object */
   evas_object_del(sd->o_base);

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

   /* move the base object */
   evas_object_move(sd->o_base, x, y);
}

static void 
_e_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Smart_Data *sd;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* resize the base object */
   evas_object_resize(sd->o_base, w, h);
}

static void 
_e_smart_show(Evas_Object *obj)
{
   E_Smart_Data *sd;
   Eina_List *l = NULL;
   Evas_Object *mon;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* if it is already visible, get out */
   if (sd->visible) return;

   /* show the base object */
   evas_object_show(sd->o_base);

   /* show any monitors */
   EINA_LIST_FOREACH(sd->monitors, l, mon)
     evas_object_show(mon);

   /* set visibility flag */
   sd->visible = EINA_TRUE;
}

static void 
_e_smart_hide(Evas_Object *obj)
{
   E_Smart_Data *sd;
   Eina_List *l = NULL;
   Evas_Object *mon;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* if it is not visible, we have nothing to do */
   if (!sd->visible) return;

   /* hide any monitors */
   EINA_LIST_FOREACH(sd->monitors, l, mon)
     evas_object_hide(mon);

   /* hide the base object */
   evas_object_hide(sd->o_base);

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
   evas_object_clip_set(sd->o_base, clip);
}

static void 
_e_smart_clip_unset(Evas_Object *obj)
{
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* unset the clip */
   evas_object_clip_unset(sd->o_base);
}

static void 
_e_smart_randr_grid_cb_move(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   E_Smart_Data *sd;
   Evas_Coord gx = 0, gy = 0, gw = 0, gh = 0;
   Eina_List *l = NULL;
   Evas_Object *mon;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the smart data */
   if (!(sd = data)) return;

   /* get the grid geometry */
   evas_object_geometry_get(sd->o_grid, &gx, &gy, &gw, &gh);

   /* loop the monitors and update grid geometry */
   EINA_LIST_FOREACH(sd->monitors, l, mon)
     e_smart_monitor_grid_set(mon, sd->o_grid, gx, gy, gw, gh);
}

static void 
_e_smart_randr_grid_cb_resize(void *data, Evas *evas EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   E_Smart_Data *sd;
   Evas_Coord gx = 0, gy = 0, gw = 0, gh = 0;
   Eina_List *l = NULL;
   Evas_Object *mon;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the smart data */
   if (!(sd = data)) return;

   /* get the grid geometry */
   evas_object_geometry_get(sd->o_grid, &gx, &gy, &gw, &gh);

   /* loop the monitors and update grid geometry */
   EINA_LIST_FOREACH(sd->monitors, l, mon)
     e_smart_monitor_grid_set(mon, sd->o_grid, gx, gy, gw, gh);
}

static Evas_Object *
_e_smart_randr_monitor_crtc_find(E_Smart_Data *sd, Ecore_X_Randr_Crtc crtc)
{
   Eina_List *l = NULL;
   Evas_Object *mon;

   if ((!sd) || (!crtc)) return NULL;

   EINA_LIST_FOREACH(sd->monitors, l, mon)
     {
        Ecore_X_Randr_Crtc mcrtc;

        if ((mcrtc = e_smart_monitor_crtc_get(mon)))
          if ((mcrtc == crtc)) return mon;
     }

   return NULL;
}

static void 
_e_smart_randr_monitor_cb_changed(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Evas_Object *randr;

   if (!(randr = data)) return;

   /* tell main dialog that something changed and to enable apply button */
   evas_object_smart_callback_call(randr, "randr_changed", NULL);
}

static void 
_e_smart_randr_monitor_cb_moved(void *data, Evas_Object *obj, void *event EINA_UNUSED)
{
   E_Smart_Data *sd;
   Evas_Object *randr;

   if (!(randr = data)) return;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(randr))) return;

   /* normalize output positions so that upper left corner of all 
    * outputs is at 0,0 */
   _e_smart_randr_monitor_position_normalize(sd);

   /* move any monitors which are adjacent to this one to their new 
    * positions due to the resize, specifying this resized monitor as 
    * the one to skip */
   _e_smart_randr_monitor_position_update(sd, obj, obj);

   /* tell main dialog that something changed and to enable apply button */
   evas_object_smart_callback_call(randr, "randr_changed", NULL);
}

static void 
_e_smart_randr_monitor_cb_resized(void *data, Evas_Object *obj, void *event EINA_UNUSED)
{
   E_Smart_Data *sd;
   Evas_Object *randr;

   if (!(randr = data)) return;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(randr))) return;

   /* move any monitors which are adjacent to this one to their new 
    * positions due to the resize, specifying this resized monitor as 
    * the one to skip */
   _e_smart_randr_monitor_position_update(sd, obj, obj);

   /* tell main dialog that something changed and to enable apply button */
   evas_object_smart_callback_call(randr, "randr_changed", NULL);
}

static void 
_e_smart_randr_monitor_position_update(E_Smart_Data *sd, Evas_Object *obj, Evas_Object *skip)
{
   Eina_List *l = NULL;
   Evas_Object *mon;
   Eina_Rectangle o, op;

   /* get the current geometry of the monitor we were passed in */
   e_smart_monitor_current_geometry_get(obj, &o.x, &o.y, &o.w, &o.h);

   e_smart_monitor_previous_geometry_get(obj, &op.x, &op.y, &op.w, &op.h);

   /* loop the list of monitors */
   EINA_LIST_FOREACH(sd->monitors, l, mon)
     {
        Eina_Rectangle m;

        /* if this monitor is the one we want to skip, than skip it */
        if (((skip) && (mon == skip)) || (mon == obj))
          continue;

        /* get the current geometry of this monitor */
        e_smart_monitor_current_geometry_get(mon, &m.x, &m.y, &m.w, &m.h);

        /* check if this monitor is adjacent to the original one, 
         * if it is, then we need to move it */

        /* check for any monitors that are on this X axis
         * (within a certain threshold of distance) */
        if ((m.x >= (op.x + (op.w / 3))) && 
            (((m.x <= ((op.x + op.w) + SNAP_FUZZ)) || 
              (m.x <= ((op.x + op.w) - SNAP_FUZZ)))))
          {
             /* don't move the monitor IF this movement would place it 
              * outside the virual grid */
             if (((o.x + o.w) + m.w) <= sd->vw)
               e_smart_monitor_current_geometry_set(mon, (o.x + o.w),
                                                    m.y, m.w, m.h);
          }
        else if ((m.y >= (op.y + (op.h / 3))) && 
                 (((m.y <= ((op.y + op.h) + SNAP_FUZZ)) || 
                   (m.y <= ((op.y + op.h) - SNAP_FUZZ)))))
          {
             /* don't move the monitor IF this movement would place it 
              * outside the virual grid */
             if (((o.y + o.h) + m.h) <= sd->vh)
               e_smart_monitor_current_geometry_set(mon, m.x, (o.y + o.h), 
                                                    m.w, m.h);
          }

        /* handle move case for obj */
        else if ((o.x >= (m.x + (m.w / 3))) && 
                 (((o.x <= ((m.x + m.w) + SNAP_FUZZ)) || 
                   (o.x <= ((m.x + m.w) - SNAP_FUZZ)))))
          {
             /* don't move the monitor IF this movement would place it 
              * outside the virual grid */
             if (((m.x + m.w) + o.w) <= sd->vw)
               e_smart_monitor_current_geometry_set(obj, (m.x + m.w),
                                                    o.y, o.w, o.h);
          }
        else if ((o.y >= (m.y + (m.h / 3))) && 
                 (((o.y <= ((m.y + op.h) + SNAP_FUZZ)) || 
                   (o.y <= ((m.y + op.h) - SNAP_FUZZ)))))
          {
             /* don't move the monitor IF this movement would place it 
              * outside the virual grid */
             if (((m.y + m.h) + o.h) <= sd->vh)
               e_smart_monitor_current_geometry_set(obj, o.x, (m.y + m.h), 
                                                    o.w, o.h);
          }

     }
}

static void 
_e_smart_randr_monitor_position_normalize(E_Smart_Data *sd)
{
   Evas_Object *mon;
   Eina_List *l = NULL;
   Evas_Coord minx = 0, miny = 0;

   minx = 32768;
   miny = 32768;

   EINA_LIST_FOREACH(sd->monitors, l, mon)
     {
        Evas_Coord mx = 0, my = 0;

        /* get the geometry for this monitor */
        e_smart_monitor_current_geometry_get(mon, &mx, &my, NULL, NULL);
        if (mx < minx) minx = mx;
        if (my < miny) miny = my;
     }

   if ((minx) || (miny))
     {
        EINA_LIST_FOREACH(sd->monitors, l, mon)
          {
             Evas_Coord mx = 0, my = 0, mw = 0, mh = 0;

             /* get the geometry for this monitor */
             e_smart_monitor_current_geometry_get(mon, &mx, &my, &mw, &mh);

             mx -= minx;
             my -= miny;

             /* move monitor to new position */
             e_smart_monitor_current_geometry_set(mon, mx, my, mw, mh);
          }
     }
}

static void 
_e_smart_randr_monitor_preferred_mode_size_get(Ecore_X_Randr_Output output, Evas_Coord *mw, Evas_Coord *mh)
{
   Ecore_X_Window root = 0;
   Ecore_X_Randr_Mode *modes;
   int n = 0, p = 0;

   if (mw) *mw = 0;
   if (mh) *mh = 0;

   if (!output) return;

   root = ecore_x_window_root_first_get();

   if (!(modes = ecore_x_randr_output_modes_get(root, output, &n, &p)))
     return;

   if ((n > 0) && (p > 0))
     ecore_x_randr_mode_size_get(root, modes[p - 1], mw, mh);
   else if (n > 0)
     ecore_x_randr_mode_size_get(root, modes[0], mw, mh);

   free(modes);
}

static Evas_Object *
_e_smart_randr_clone_find(Ecore_X_Window root, Ecore_X_Randr_Output output, Ecore_X_Randr_Crtc crtc, Evas_Coord cx, Evas_Coord cy, E_Smart_Data *sd)
{
   Evas_Object *mon = NULL;
   Ecore_X_Randr_Crtc pcrtc = 0;
   Evas_Coord px = 0, py = 0;

   /* get the crtc of the previous output */
   pcrtc = ecore_x_randr_output_crtc_get(root, output);
   if (!pcrtc) return NULL;

   /* get this crtcs geom */
   ecore_x_randr_crtc_geometry_get(root, pcrtc, 
                                   &px, &py, NULL, NULL);

   if ((px == cx) && (py == cy))
     {
        Ecore_X_Randr_Orientation orient = 
          ECORE_X_RANDR_ORIENTATION_ROT_0;
        Ecore_X_Randr_Orientation porient = 
          ECORE_X_RANDR_ORIENTATION_ROT_0;

        porient = 
          ecore_x_randr_crtc_orientation_get(root, pcrtc);

        orient = 
          ecore_x_randr_crtc_orientation_get(root, crtc);

        if (porient == orient)
          {
             Ecore_X_Randr_Mode mode = 0, pmode = 0;
             Evas_Coord pw = 0, ph = 0;
             Evas_Coord w = 0, h = 0;

             pmode = 
               ecore_x_randr_crtc_mode_get(root, pcrtc);
             ecore_x_randr_mode_size_get(root, pmode, &pw, &ph);

             mode = 
               ecore_x_randr_crtc_mode_get(root, crtc);
             ecore_x_randr_mode_size_get(root, mode, &w, &h);

             if ((pmode == mode) || ((pw == w) && (ph == h)))
               mon = _e_smart_randr_monitor_crtc_find(sd, pcrtc);
          }
     }

   return mon;
}

static Ecore_X_Randr_Crtc 
_e_smart_randr_crtc_find(Ecore_X_Randr_Output output)
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
