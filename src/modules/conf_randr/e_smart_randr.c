#include "e.h"
#include "e_mod_main.h"
#include "e_smart_randr.h"
#include "e_smart_monitor.h"

/* local structures */
typedef struct _E_Smart_Data E_Smart_Data;
struct _E_Smart_Data
{
   /* layout object */
   Evas_Object *o_layout;

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
   Eina_List *l = NULL;
   E_Randr_Crtc_Config *crtc;
   Evas_Coord vw = 0, vh = 0;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* grab the root window */
   root = ecore_x_window_root_first_get();

   /* loop the list of crtcs in our config */
   EINA_LIST_FOREACH(e_randr_cfg->crtcs, l, crtc)
     {
        Eina_List *o = NULL;
        E_Randr_Output_Config *output;

        /* loop the list of outputs in this crtc */
        EINA_LIST_FOREACH(crtc->outputs, o, output)
          {
             Ecore_X_Randr_Mode *modes;
             Evas_Coord mw = 0, mh = 0;
             int num = 0;

             /* get the modes for this output
              * 
              * NB: Xrandr returns these modes in order of largest first */
             modes = ecore_x_randr_output_modes_get(root, output->xid, &num, NULL);
             if (!modes) continue;

             /* get the size of the largest mode */
             ecore_x_randr_mode_size_get(root, modes[0], &mw, &mh);

             vw += MAX(mw, mh);
             vh += MAX(mw, mh);

             /* free any allocated memory from ecore_x_randr */
             free(modes);
          }
     }

   /* set the layout virtual size */
   e_layout_virtual_size_set(sd->o_layout, vw, vh);
}

void 
e_smart_randr_monitors_create(Evas_Object *obj)
{
   E_Smart_Data *sd;
   Evas *evas;
   Eina_List *l = NULL;
   E_Randr_Crtc_Config *crtc;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* grab the canvas of the layout widget */
   evas = evas_object_evas_get(sd->o_layout);

   /* loop the list of crtcs in our config */
   EINA_LIST_FOREACH(e_randr_cfg->crtcs, l, crtc)
     {
        Eina_List *o = NULL;
        E_Randr_Output_Config *output;

        /* loop the list of outputs in this crtc */
        EINA_LIST_FOREACH(crtc->outputs, o, output)
          {
             Evas_Object *mon;

             /* for each output, try to create a monitor */
             if (!(mon = e_smart_monitor_add(evas)))
               continue;

             /* add this monitor to our list */
             sd->monitors = eina_list_append(sd->monitors, mon);

             /* tell monitor what crtc it uses */
             e_smart_monitor_crtc_set(mon, crtc);

             /* tell monitor what output it uses */
             e_smart_monitor_output_set(mon, output);

             /* pack this monitor into the layout */
             e_layout_pack(sd->o_layout, mon);

             /* move this monitor to proper position */
             e_layout_child_move(mon, crtc->x, crtc->y);

             /* resize this monitor to proper size */
             e_layout_child_resize(mon, crtc->width, crtc->height);
          }
     }
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

   /* create the layout object */
   sd->o_layout = e_layout_add(evas);

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

   /* delete the layout object */
   if (sd->o_layout) evas_object_del(sd->o_layout);

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

   /* move the layout object */
   if (sd->o_layout) evas_object_move(sd->o_layout, x, y);
}

static void 
_e_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Smart_Data *sd;

   LOGFN(__FILE__, __LINE__, __FUNCTION__);

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* resize the layout object */
   if (sd->o_layout) evas_object_resize(sd->o_layout, w, h);
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

   /* show the layout object */
   if (sd->o_layout) evas_object_show(sd->o_layout);

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

   /* hide the layout object */
   if (sd->o_layout) evas_object_hide(sd->o_layout);

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
   if (sd->o_layout) evas_object_clip_set(sd->o_layout, clip);
}

static void 
_e_smart_clip_unset(Evas_Object *obj)
{
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* unset the clip */
   if (sd->o_layout) evas_object_clip_unset(sd->o_layout);
}
