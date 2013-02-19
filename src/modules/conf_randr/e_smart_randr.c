#include "e.h"
#include "e_mod_main.h"
#include "e_smart_randr.h"
#include "e_smart_monitor.h"

/* local structures */
typedef struct _E_Smart_Data E_Smart_Data;
struct _E_Smart_Data
{
   Evas_Object *o_base;
   Evas_Object *o_grid;

   /* layout object */
   /* Evas_Object *o_layout; */

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
   Ecore_X_Randr_Crtc *crtcs;
   Evas_Coord vw = 0, vh = 0;
   int ncrtcs = 0;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* NB: The old code here used to get the modes from the e_randr_cfg.
    * I changed it to get directly from Xrandr because of attempts to 
    * run this in Xephyr. Getting the information from e_randr_cfg was not 
    * practical in those cases */

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* grab the root window */
   root = ecore_x_window_root_first_get();

   /* try to get the list of crtcs */
   if ((crtcs = ecore_x_randr_crtcs_get(root, &ncrtcs)))
     {
        int i = 0;
        Ecore_X_Randr_Output *outputs;

        /* loop the list of crtcs and try to get the outputs on each */
        for (i = 0; i < ncrtcs; i++)
          {
             int noutput = 0, j = 0;

             if (!(outputs = 
                   ecore_x_randr_crtc_outputs_get(root, crtcs[i], &noutput)))
               continue;

             /* loop the outputs and get the largest mode */
             for (j = 0; j < noutput; j++)
               {
                  Ecore_X_Randr_Mode *modes;
                  Evas_Coord mw = 0, mh = 0;
                  int nmode = 0;

                  /* try to get the list of modes for this output */
                  modes = 
                    ecore_x_randr_output_modes_get(root, outputs[j], 
                                                   &nmode, NULL);
                  if (!modes) continue;

                  /* get the size of the largest mode */
                  ecore_x_randr_mode_size_get(root, modes[0], &mw, &mh);

                  vw += MAX(mw, mh);
                  vh += MAX(mw, mh);

                  /* free any allocated memory from ecore_x_randr */
                  free(modes);
               }

             /* free any allocated memory from ecore_x_randr */
             free(outputs);
          }

        /* free any allocated memory from ecore_x_randr */
        free(crtcs);
     }

   if ((vw == 0) || (vh == 0))
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
   Ecore_X_Randr_Crtc *crtcs;
   int ncrtcs = 0;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* NB: The old code here used to get the outputs from the e_randr_cfg.
    * I changed it to get directly from Xrandr because of attempts to 
    * run this in Xephyr. Getting the information from e_randr_cfg was not 
    * practical in those cases */

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* grab the canvas of the grid object */
   evas = evas_object_evas_get(sd->o_grid);

   /* grab the root window */
   root = ecore_x_window_root_first_get();

   /* try to get the list of crtcs */
   if ((crtcs = ecore_x_randr_crtcs_get(root, &ncrtcs)))
     {
        int i = 0;
        Ecore_X_Randr_Output *outputs;

        /* loop the list of crtcs and try to get the outputs on each */
        for (i = 0; i < ncrtcs; i++)
          {
             Evas_Coord cx = 0, cy = 0, cw = 0, ch = 0;
             int noutput = 0, j = 0;

             if (!(outputs = 
                   ecore_x_randr_crtc_outputs_get(root, crtcs[i], &noutput)))
               continue;

             /* get the geometry for this crtc */
             ecore_x_randr_crtc_geometry_get(root, crtcs[i], 
                                             &cx, &cy, &cw, &ch);

             /* loop the outputs and create monitors for each */
             for (j = 0; j < noutput; j++)
               {
                  Evas_Object *mon;

                  /* for each output, try to create a monitor */
                  if (!(mon = e_smart_monitor_add(evas)))
                    continue;

                  /* add this monitor to our list */
                  sd->monitors = eina_list_append(sd->monitors, mon);

                  /* pack this monitor into the grid */
                  evas_object_grid_pack(sd->o_grid, mon, cx, cy, cw, ch);

                  /* tell monitor what the virtual grid is */
                  e_smart_monitor_grid_set(mon, sd->o_grid);

                  /* tell monitor what crtc it uses and it's current position */
                  e_smart_monitor_crtc_set(mon, crtcs[i], cx, cy, cw, ch);

                  /* tell monitor what output it uses */
                  e_smart_monitor_output_set(mon, outputs[j]);

                  /* tell monitor to set the background preview */
                  e_smart_monitor_background_set(mon, cx, cy);
               }

             /* free any allocated memory from ecore_x_randr */
             free(outputs);
          }

        /* free any allocated memory from ecore_x_randr */
        free(crtcs);
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

   sd->o_grid = evas_object_grid_add(evas);
   edje_object_part_swallow(sd->o_base, "e.swallow.content", sd->o_grid);

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
     evas_object_del(mon);

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
