#include "e.h"
#include "e_mod_main.h"
#include "e_smart_randr.h"
#include "e_smart_monitor.h"

/* local structures */
typedef struct _E_Smart_Data E_Smart_Data;
struct _E_Smart_Data
{
   /* scroll object */
   Evas_Object *o_scroll;

   /* layout object */
   Evas_Object *o_layout;

   /* list of monitor objects */
   Eina_List *monitors;

   /* changed flag */
   Eina_Bool changed : 1;

   /* visible flag */
   Eina_Bool visible : 1;
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
static void _e_smart_randr_changed_set(Evas_Object *obj);
static int _e_smart_randr_modes_sort(const void *data1, const void *data2);

static Evas_Object *_e_smart_randr_monitor_find(E_Smart_Data *sd, Ecore_X_Randr_Crtc xid);

static void _e_smart_randr_monitor_adjacent_move(E_Smart_Data *sd, Evas_Object *obj, Evas_Object *skip);

/* local callbacks prototypes for monitors */
static void _e_smart_randr_monitor_cb_moving(void *data, Evas_Object *obj, void *event EINA_UNUSED);
static void _e_smart_randr_monitor_cb_moved(void *data, Evas_Object *obj, void *event EINA_UNUSED);
static void _e_smart_randr_monitor_cb_resized(void *data, Evas_Object *obj, void *event EINA_UNUSED);
static void _e_smart_randr_monitor_cb_rotated(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED);
static void _e_smart_randr_monitor_cb_deleted(void *data EINA_UNUSED, Evas *evas EINA_UNUSED, Evas_Object *obj, void *event EINA_UNUSED);
static void _e_smart_randr_monitor_cb_changed(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED);

/* external functions exposed by this widget */
Evas_Object *
e_smart_randr_add(Evas *evas)
{
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
e_smart_randr_layout_size_get(Evas_Object *obj, Evas_Coord *w, Evas_Coord *h)
{
   E_Smart_Data *sd;
   Eina_List *l;
   E_Randr_Crtc_Info *crtc;
   Evas_Coord mw = 0, mh = 0;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* loop the crtcs, checking for valid output */
   EINA_LIST_FOREACH(E_RANDR_12->crtcs, l, crtc)
     {
        E_Randr_Output_Info *output;
        Eina_List *outputs = NULL, *ll;

        EINA_LIST_FOREACH(crtc->outputs, ll, output)
          outputs = eina_list_append(outputs, output);

        /* if this crtc is disabled, then no output will be assigned to it.
         * 
         * We need to check the possible outputs and assign one */
        if (!crtc->current_mode)
          {
             EINA_LIST_FOREACH(crtc->possible_outputs, ll, output)
               {
                  if (!(eina_list_data_find(outputs, output) == output))
                    {
                       if (!output->crtc) output->crtc = crtc;
                       if (output->crtc != crtc) continue;
                       outputs = eina_list_append(outputs, output);
                    }
               }
          }

        /* loop the outputs on this crtc */
        EINA_LIST_FOREACH(outputs, ll, output)
          {
             Eina_List *modes = NULL;
             Ecore_X_Randr_Mode_Info *mode = NULL;

             /* check for valid monitor */
             if (output->monitor)
               {
                  /* try to get the list of modes */
                  if (!(modes = eina_list_clone(output->monitor->modes)))
                    continue;

                  /* sort the list of modes */
                  modes = eina_list_sort(modes, 0, _e_smart_randr_modes_sort);

                  /* get last mode */
                  if ((mode = eina_list_last_data_get(modes)))
                    {
                       /* grab max mode size and add to return value */
                       mw += mode->width;
                       mh += mode->height;
                    }
               }
          }
     }

   if (w) *w = mw;
   if (h) *h = mh;
}

void 
e_smart_randr_current_size_set(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* set the virtual size of the layout */
   e_layout_virtual_size_set(sd->o_layout, w, h);

   /* resize the layout widget
    * 
    * NB: This is using an arbitrary scale of 1/10th the screen size */
   evas_object_resize(sd->o_layout, (w / 10), (h / 10));
}

void 
e_smart_randr_monitors_create(Evas_Object *obj)
{
   E_Smart_Data *sd;
   Eina_List *l, *ll, *deferred = NULL;
   E_Randr_Crtc_Info *crtc;
   E_Randr_Output_Info *output;
   Evas *evas;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* get the canvas of the layout widget */
   evas = evas_object_evas_get(sd->o_layout);

   /* loop the crtcs, checking for valid output */
   EINA_LIST_FOREACH(E_RANDR_12->crtcs, l, crtc)
     {
        Eina_List *outputs = NULL;

        printf("Checking Crtc: %d\n", crtc->xid);
        printf("\tGeom: %d %d %d %d\n", crtc->geometry.x, 
               crtc->geometry.y, crtc->geometry.w, crtc->geometry.h);

        EINA_LIST_FOREACH(crtc->outputs, ll, output)
          outputs = eina_list_append(outputs, output);

        /* if this crtc is disabled, then no output will be assigned to it.
         * 
         * We need to check the possible outputs and assign one */
        if (!crtc->current_mode)
          {
             /* loop the possible outputs */
             EINA_LIST_FOREACH(crtc->possible_outputs, ll, output)
               {
                  if (!(eina_list_data_find(outputs, output) == output))
                    {
                       E_Randr_Crtc_Info *pcrtc;

                       pcrtc = eina_list_last_data_get(output->possible_crtcs);
                       if (!output->crtc) output->crtc = pcrtc;
                       if ((output->crtc) && 
                           (output->crtc != pcrtc)) continue;
                       printf("\tAssigned Crtc %d To Output: %d\n", 
                              pcrtc->xid, output->xid);
                       outputs = eina_list_append(outputs, output);
                    }
               }
          }

        /* loop the outputs on this crtc */
        EINA_LIST_FOREACH(outputs, ll, output)
          {
             /* printf("\tChecking Output: %d %s\n", output->xid, output->name); */
             /* printf("\tOutput Policy: %d\n", output->policy); */
             /* printf("\tOutput Status: %d\n", output->connection_status); */

             /* if (output->wired_clones) */
             /*   printf("\tHAS WIRED CLONES !!\n"); */

             if (output->connection_status == 
                 ECORE_X_RANDR_CONNECTION_STATUS_CONNECTED)
               {
                  Evas_Object *mon = NULL, *pmon = NULL;
                  E_Randr_Crtc_Info *pcrtc = NULL;

                  /* if we do not have a saved config yet, and the 
                   * policy of this output is 'none' then this could be a 
                   * first run situation. Because ecore_x_randr does not 
                   * tell us proper output policies 
                   * (as in ECORE_X_RANDR_OUTPUT_POLICY_CLONE) due to 
                   * X not setting them, we need to determine manually 
                   * if we are in a cloned situation here, and what output 
                   * we are cloned to */
                  if ((!e_config->randr_serialized_setup) && 
                      (output->policy == ECORE_X_RANDR_OUTPUT_POLICY_NONE))
                    {
                       /* if we have a previous crtc, check if that is the 
                        * one we are cloned to */
                       if ((pcrtc = eina_list_data_get(eina_list_prev(l))))
                         {
                            /* we have a previous crtc. compare geometry */
                            if ((crtc->geometry.x == pcrtc->geometry.x) && 
                                (crtc->geometry.y == pcrtc->geometry.y) && 
                                (crtc->geometry.w == pcrtc->geometry.w) && 
                                (crtc->geometry.h == pcrtc->geometry.h))
                              {
                                 pmon = 
                                   _e_smart_randr_monitor_find(sd, pcrtc->xid);
                              }
                         }
                    }
                  /* else if we have an existing configuration and this 
                   * output is set to cloned, then see if we can create the 
                   * cloned representation */
                  else if ((e_config->randr_serialized_setup) && 
                           (output->policy == ECORE_X_RANDR_OUTPUT_POLICY_CLONE))
                    {
                       /* if we have a previous crtc, check if that is the 
                        * one we are cloned to */
                       if ((pcrtc = eina_list_data_get(eina_list_prev(l))))
                         {
                            /* we have a previous crtc. compare geometry */
                            if ((crtc->geometry.x == pcrtc->geometry.x) && 
                                (crtc->geometry.y == pcrtc->geometry.y) && 
                                (crtc->geometry.w == pcrtc->geometry.w) && 
                                (crtc->geometry.h == pcrtc->geometry.h))
                              {
                                 pmon = 
                                   _e_smart_randr_monitor_find(sd, pcrtc->xid);
                              }
                         }
                       else
                         {
                            /* we have no previous monitor to clone this to yet
                             * add it to the deferred list and we will check it 
                             * after everything has been setup */
                            deferred = eina_list_append(deferred, output);
                            continue;
                         }
                    }

                  if ((mon = e_smart_monitor_add(evas)))
                    {
                       Evas_Coord cx = 0, cy = 0;
                       Evas_Coord cw = 0, ch = 0;

                       /* add this monitor to the layout */
                       e_smart_randr_monitor_add(obj, mon);

                       /* tell the monitor which layout it references */
                       e_smart_monitor_layout_set(mon, sd->o_layout);

                       /* tell the monitor which output it references */
                       e_smart_monitor_output_set(mon, output);

                       /* tell the monitor which crtc it references */
                       e_smart_monitor_crtc_set(mon, crtc);

                       /* with the layout and output assigned, we can 
                        * tell the monitor to setup
                        * 
                        * NB: This means filling resolutions, getting 
                        * refresh rates, displaying monitor name, etc...
                        * all the graphical stuff */
                       e_smart_monitor_setup(mon);

                       cx = crtc->geometry.x;
                       cy = crtc->geometry.y;
                       cw = crtc->geometry.w;
                       ch = crtc->geometry.h;

                       if ((cw == 0) || (ch == 0))
                         e_smart_monitor_current_geometry_get(mon, NULL, NULL, 
                                                              &cw, &ch);

                       if (pmon)
                         {
                            /* set geometry so that when we "unclone" this 
                             * one, it will unclone to the right */
                            if (pcrtc) cx += pcrtc->geometry.w;
                         }

                       /* resize this monitor to it's current size */
                       e_layout_child_resize(mon, cw, ch);

                       /* move this monitor to it's current location */
                       e_layout_child_move(mon, cx, cy);

                       /* if we are cloned, then tell randr */
                       if (pmon)
                         e_smart_monitor_clone_add(pmon, mon);
                    }
               }
          }
     }

   /* all main monitors should be setup now */

   /* loop any outputs we have deferred */
   EINA_LIST_FOREACH(deferred, l, output)
     {
        Evas_Object *mon = NULL, *pmon = NULL;

        if (!output->crtc) continue;

        /* find a crtc that matches the geometry of this output's crtc */
        EINA_LIST_FOREACH(E_RANDR_12->crtcs, ll, crtc)
          {
             if ((crtc->geometry.x == output->crtc->geometry.x) && 
                 (crtc->geometry.y == output->crtc->geometry.y) && 
                 (crtc->geometry.w == output->crtc->geometry.w) && 
                 (crtc->geometry.h == output->crtc->geometry.h))
               {
                  if ((pmon = _e_smart_randr_monitor_find(sd, crtc->xid)))
                    break;
               }
          }

        if ((mon = e_smart_monitor_add(evas)))
          {
             Evas_Coord cx = 0, cy = 0;
             Evas_Coord cw = 0, ch = 0;

             /* add this monitor to the layout */
             e_smart_randr_monitor_add(obj, mon);

             /* tell the monitor which layout it references */
             e_smart_monitor_layout_set(mon, sd->o_layout);

             /* tell the monitor which output it references */
             e_smart_monitor_output_set(mon, output);

             /* with the layout and output assigned, we can 
              * tell the monitor to setup
              * 
              * NB: This means filling resolutions, getting 
              * refresh rates, displaying monitor name, etc...
              * all the graphical stuff */
             e_smart_monitor_setup(mon);

             cx = output->crtc->geometry.x;
             cy = output->crtc->geometry.y;
             cw = output->crtc->geometry.w;
             ch = output->crtc->geometry.h;

             if ((cw == 0) || (ch == 0))
               {
                  cw = 640;
                  ch = 480;
               }

             /* set geometry so that when we "unclone" this 
              * one, it will unclone to the right */
             if ((pmon) && (crtc)) cx += crtc->geometry.w;

             /* resize this monitor to it's current size */
             e_layout_child_resize(mon, cw, ch);

             /* move this monitor to it's current location */
             e_layout_child_move(mon, cx, cy);

             /* if we are cloned, then tell randr */
             if (pmon)
               e_smart_monitor_clone_add(pmon, mon);
          }
     }
}

void 
e_smart_randr_monitor_add(Evas_Object *obj, Evas_Object *mon)
{
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* add evas callbacks for monitor resize, rotate */
   evas_object_smart_callback_add(mon, "monitor_moving", 
                                  _e_smart_randr_monitor_cb_moving, obj);
   evas_object_smart_callback_add(mon, "monitor_moved", 
                                  _e_smart_randr_monitor_cb_moved, obj);
   evas_object_smart_callback_add(mon, "monitor_resized", 
                                  _e_smart_randr_monitor_cb_resized, obj);
   evas_object_smart_callback_add(mon, "monitor_rotated", 
                                  _e_smart_randr_monitor_cb_rotated, obj);
   evas_object_smart_callback_add(mon, "monitor_changed", 
                                  _e_smart_randr_monitor_cb_changed, obj);

   /* add listener for monitor delete event */
   evas_object_event_callback_add(mon, EVAS_CALLBACK_DEL, 
                                  _e_smart_randr_monitor_cb_deleted, NULL);

   /* add monitor to layout */
   e_layout_pack(sd->o_layout, mon);

   /* add this monitor to our list */
   sd->monitors = eina_list_append(sd->monitors, mon);

   /* show the monitor
    * 
    * NB: Needed. Do Not Remove */
   evas_object_show(mon);
}

void 
e_smart_randr_monitor_del(Evas_Object *obj, Evas_Object *mon)
{
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* delete evas callbacks for monitor resize, rotate */
   evas_object_smart_callback_del(mon, "monitor_moving", 
                                  _e_smart_randr_monitor_cb_moving);
   evas_object_smart_callback_del(mon, "monitor_moved", 
                                  _e_smart_randr_monitor_cb_moved);
   evas_object_smart_callback_del(mon, "monitor_resized", 
                                  _e_smart_randr_monitor_cb_resized);
   evas_object_smart_callback_del(mon, "monitor_rotated", 
                                  _e_smart_randr_monitor_cb_rotated);
   evas_object_smart_callback_del(mon, "monitor_changed", 
                                  _e_smart_randr_monitor_cb_changed);

   /* delete listener for monitor delete event */
   evas_object_event_callback_del(mon, EVAS_CALLBACK_DEL, 
                                  _e_smart_randr_monitor_cb_deleted);

   /* remove monitor from layout */
   e_layout_unpack(mon);

   /* add this monitor to our list */
   sd->monitors = eina_list_remove(sd->monitors, mon);
}

Eina_List *
e_smart_randr_monitors_get(Evas_Object *obj)
{
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return NULL;

   /* return the list of monitors */
   return sd->monitors;
}

Eina_Bool 
e_smart_randr_changed_get(Evas_Object *obj)
{
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return EINA_FALSE;

   return sd->changed;
}

void 
e_smart_randr_changes_apply(Evas_Object *obj)
{
   E_Smart_Data *sd;
   Eina_List *l = NULL;
   Evas_Object *mon = NULL;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* loop the list of monitors */
   EINA_LIST_FOREACH(sd->monitors, l, mon)
     {
        /* tell the monitor to apply these changes */
        e_smart_monitor_changes_apply(mon);

        /* tell monitor to reset changes
         * 
         * NB: This updates the monitor's "original" values with those 
         * that are "current" */
        e_smart_monitor_changes_reset(mon);
     }

   /* FIXME: This should maybe go into a "restore on login" option ?? */

   /* tell randr to save this config */
   e_randr_store_configuration(E_RANDR_CONFIGURATION_STORE_ALL);
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

   /* create the layout */
   sd->o_layout = e_layout_add(evas);

   /* create the scroll */
   sd->o_scroll = e_scrollframe_add(evas);
   e_scrollframe_child_set(sd->o_scroll, sd->o_layout);
   evas_object_smart_member_add(sd->o_scroll, obj);

   /* set the objects smart data */
   evas_object_smart_data_set(obj, sd);
}

static void 
_e_smart_del(Evas_Object *obj)
{
   E_Smart_Data *sd;
   Evas_Object *mon;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* delete the monitors */
   EINA_LIST_FREE(sd->monitors, mon)
     evas_object_del(mon);

   /* delete the layout object */
   if (sd->o_layout) evas_object_del(sd->o_layout);

   /* delete the scrollframe object */
   if (sd->o_scroll) evas_object_del(sd->o_scroll);

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

   /* move the scroll */
   if (sd->o_scroll) evas_object_move(sd->o_scroll, x, y);
}

static void 
_e_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* resize the scroll */
   if (sd->o_scroll) evas_object_resize(sd->o_scroll, w, h);
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
   if (sd->o_scroll) evas_object_show(sd->o_scroll);

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
   if (sd->o_scroll) evas_object_hide(sd->o_scroll);

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
   if (sd->o_scroll) evas_object_clip_set(sd->o_scroll, clip);
}

static void 
_e_smart_clip_unset(Evas_Object *obj)
{
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* unset the clip */
   if (sd->o_scroll) evas_object_clip_unset(sd->o_scroll);
}

static void 
_e_smart_randr_changed_set(Evas_Object *obj)
{
   E_Smart_Data *sd;
   Evas_Object *mon = NULL;
   Eina_List *l = NULL;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* default changed flag */
   sd->changed = EINA_FALSE;

   /* loop list of monitors */
   EINA_LIST_FOREACH(sd->monitors, l, mon)
     {
        E_Smart_Monitor_Changes changes = E_SMART_MONITOR_CHANGED_NONE;

        changes = e_smart_monitor_changes_get(mon);
        if (changes > E_SMART_MONITOR_CHANGED_NONE)
          {
             sd->changed = EINA_TRUE;
             break;
          }
     }

   /* send changed signal to main dialog */
   evas_object_smart_callback_call(obj, "changed", NULL);
}

static int 
_e_smart_randr_modes_sort(const void *data1, const void *data2)
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

static Evas_Object *
_e_smart_randr_monitor_find(E_Smart_Data *sd, Ecore_X_Randr_Crtc xid)
{
   Eina_List *l;
   Evas_Object *mon;

   /* loop the monitor list */
   EINA_LIST_FOREACH(sd->monitors, l, mon)
     {
        E_Randr_Output_Info *output;

        /* try to grab this monitors output */
        if ((output = e_smart_monitor_output_get(mon)))
          {
             /* compare the output's crtc id to the one passed in */
             if ((output->crtc) && (output->crtc->xid == xid))
               return mon;
          }
     }

   return NULL;
}

static void 
_e_smart_randr_monitor_adjacent_move(E_Smart_Data *sd, Evas_Object *obj, Evas_Object *skip)
{
   Eina_List *l = NULL;
   Evas_Object *mon;
   Eina_Rectangle o;

   /* get the current geometry of the monitor we were passed in */
   e_smart_monitor_current_geometry_get(obj, &o.x, &o.y, NULL, NULL);
   e_layout_child_geometry_get(obj, NULL, NULL, &o.w, &o.h);

   /* loop the list of monitors */
   EINA_LIST_FOREACH(sd->monitors, l, mon)
     {
        Eina_Rectangle m;

        /* if this monitor is the one we want to skip, than skip it */
        if (((skip) && (mon == skip)) || (mon == obj))
          continue;

        /* get the current geometry of this monitor */
        e_smart_monitor_current_geometry_get(mon, &m.x, &m.y, NULL, NULL);
        e_layout_child_geometry_get(mon, NULL, NULL, &m.w, &m.h);

        /* check if this monitor is adjacent to the original one, 
         * if it is, then we need to move it */
        if ((m.x == o.x) || (m.y == o.y))
          {
             if ((m.x == o.x))
               {
                  if ((m.y >= o.y))
                    {
                       /* vertical positioning */
                       e_layout_child_move(mon, m.x, (o.y + o.h));
                    }
               }
             else if ((m.y == o.y))
               {
                  if ((m.x >= o.x))
                    {
                       /* horizontal positioning */
                       e_layout_child_move(mon, (o.x + o.w), m.y);
                    }
               }
          }
     }
}

/* local callbacks for monitors */

/* callback received from a monitor object to let us know that it is moving, 
 * and we now have to check for a drop zone */
static void 
_e_smart_randr_monitor_cb_moving(void *data, Evas_Object *obj, void *event EINA_UNUSED)
{
   Evas_Object *o_randr = NULL;
   E_Smart_Data *sd;
   Eina_List *l = NULL;
   Evas_Object *mon;
   Eina_Rectangle o;

   /* data is the randr object */
   if (!(o_randr = data)) return;

   /* try to get the RandR objects smart data */
   if (!(sd = evas_object_smart_data_get(o_randr))) return;

   /* NB FIXME: Hmmmm, this may need to use the geometry of the actual 
    * frame object for comparison */

   /* get the current geometry of the monitor we were passed in */
   e_layout_child_geometry_get(obj, &o.x, &o.y, &o.w, &o.h);

   /* loop the list of monitors */
   EINA_LIST_FOREACH(sd->monitors, l, mon)
     {
        Eina_Rectangle m;

        /* if this monitor is the one we want to skip, than skip it */
        if (mon == obj) continue;

        /* get the current geometry of this monitor */
        e_layout_child_geometry_get(mon, &m.x, &m.y, &m.w, &m.h);

        /* check if the moved monitor is inside an existing one */
        if (E_INSIDE(o.x, o.y, m.x, m.y, m.w, m.h))
          {
             /* turn on the drop zone so tell user they can drop here */
             e_smart_monitor_drop_zone_set(mon, EINA_TRUE);
             break;
          }
        else
          {
             /* moving monitor is outside the drop zone of this monitor. 
              * turn off drop zone hilighting */
             e_smart_monitor_drop_zone_set(mon, EINA_FALSE);
          }
     }
}

/* callback received from a monitor object to let us know that it was moved, 
 * and we now have to adjust the position of any adjacent monitors */
static void 
_e_smart_randr_monitor_cb_moved(void *data, Evas_Object *obj, void *event EINA_UNUSED)
{
   Evas_Object *o_randr = NULL;
   E_Smart_Data *sd;
   Eina_List *l = NULL;
   Evas_Object *mon;
   Eina_Rectangle o;

   /* data is the randr object */
   if (!(o_randr = data)) return;

   /* try to get the RandR objects smart data */
   if (!(sd = evas_object_smart_data_get(o_randr))) return;

   /* get the current geometry of the monitor we were passed in */
   e_layout_child_geometry_get(obj, &o.x, &o.y, &o.w, &o.h);

   /* loop the list of monitors */
   EINA_LIST_FOREACH(sd->monitors, l, mon)
     {
        Eina_Rectangle m;

        /* if this monitor is the one we want to skip, than skip it */
        if (mon == obj) continue;

        /* get the current geometry of this monitor */
        e_layout_child_geometry_get(mon, &m.x, &m.y, &m.w, &m.h);

        /* check if the moved monitor is inside an existing one */
        if (E_INSIDE(o.x, o.y, m.x, m.y, m.w, m.h))
          {
             /* clone this monitor into the obj monitor */
             e_smart_monitor_clone_add(mon, obj);

             /* emit signal to turn off drop zone hilight */
             e_smart_monitor_drop_zone_set(mon, EINA_FALSE);

             break;
          }
     }

   /* tell randr widget about changes */
   _e_smart_randr_changed_set(o_randr);
}

/* callback received from a monitor object to let us know that it was resized, 
 * and we now have to adjust the position of any adjacent monitors */
static void 
_e_smart_randr_monitor_cb_resized(void *data, Evas_Object *obj, void *event EINA_UNUSED)
{
   Evas_Object *o_randr = NULL;
   E_Smart_Data *sd;
   Eina_List *l = NULL;
   Evas_Object *mon;

   /* data is the randr object */
   if (!(o_randr = data)) return;

   /* try to get the RandR objects smart data */
   if (!(sd = evas_object_smart_data_get(o_randr))) return;

   /* freeze the layout widget from redrawing while we shuffle things around */
   e_layout_freeze(sd->o_layout);

   /* move any monitors which are adjacent to this one to their new 
    * positions because of the resize, specifying the resized monitor 
    * as the one to skip */
   _e_smart_randr_monitor_adjacent_move(sd, obj, obj);

   /* move any Other monitors to their new positions */
   EINA_LIST_FOREACH(sd->monitors, l, mon)
     {
        /* skip the current monitor */
        if (mon == obj) continue;

        /* move any monitors which are adjacent to this one to their new 
         * positions because of the resize, specifying the resized monitor 
         * as the one to skip */
        _e_smart_randr_monitor_adjacent_move(sd, mon, obj);
     }

   /* thaw the layout widget, allowing redraws again */
   e_layout_thaw(sd->o_layout);

   /* tell randr widget about changes */
   _e_smart_randr_changed_set(o_randr);
}

/* callback received from a monitor object to let us know that it was rotated, 
 * and we now have to adjust the position of any adjacent monitors */
static void 
_e_smart_randr_monitor_cb_rotated(void *data, Evas_Object *obj, void *event EINA_UNUSED)
{
   Evas_Object *o_randr = NULL;
   E_Smart_Data *sd;
   Eina_List *l = NULL;
   Evas_Object *mon;

   /* data is the randr object */
   if (!(o_randr = data)) return;

   /* try to get the RandR objects smart data */
   if (!(sd = evas_object_smart_data_get(o_randr))) return;

   /* freeze the layout widget from redrawing while we shuffle things around */
   e_layout_freeze(sd->o_layout);

   /* move any monitors which are adjacent to this one to their new 
    * positions because of the resize, specifying the resized monitor 
    * as the one to skip */
   _e_smart_randr_monitor_adjacent_move(sd, obj, obj);

   /* move any Other monitors to their new positions */
   EINA_LIST_FOREACH(sd->monitors, l, mon)
     {
        /* skip the current monitor */
        if (mon == obj) continue;

        /* move any monitors which are adjacent to this one to their new 
         * positions because of the resize, specifying the resized monitor 
         * as the one to skip */
        _e_smart_randr_monitor_adjacent_move(sd, mon, obj);
     }

   /* thaw the layout widget, allowing redraws again */
   e_layout_thaw(sd->o_layout);

   /* tell randr widget about changes */
   _e_smart_randr_changed_set(o_randr);
}

static void 
_e_smart_randr_monitor_cb_deleted(void *data EINA_UNUSED, Evas *evas EINA_UNUSED, Evas_Object *obj, void *event EINA_UNUSED)
{
   /* delete the smart callbacks we were listening on */
   evas_object_smart_callback_del(obj, "monitor_moving", 
                                  _e_smart_randr_monitor_cb_moving);
   evas_object_smart_callback_del(obj, "monitor_moved", 
                                  _e_smart_randr_monitor_cb_moved);
   evas_object_smart_callback_del(obj, "monitor_resized", 
                                  _e_smart_randr_monitor_cb_resized);
   evas_object_smart_callback_del(obj, "monitor_rotated", 
                                  _e_smart_randr_monitor_cb_rotated);
   evas_object_smart_callback_del(obj, "monitor_changed", 
                                  _e_smart_randr_monitor_cb_changed);
}

static void 
_e_smart_randr_monitor_cb_changed(void *data, Evas_Object *obj EINA_UNUSED, void *event EINA_UNUSED)
{
   Evas_Object *o_randr = NULL;

   /* data is the randr object */
   if (!(o_randr = data)) return;

   /* tell randr widget about changes */
   _e_smart_randr_changed_set(o_randr);
}
