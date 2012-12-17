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
e_smart_randr_current_size_set(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* set the virtual size of the layout */
   e_layout_virtual_size_set(sd->o_layout, w, h);

   /* resize the layout widget
    * NB: This is using an arbitrary scale of 1/10th the screen size */
   evas_object_resize(sd->o_layout, (w / 10), (h / 10));
}

void 
e_smart_randr_monitors_create(Evas_Object *obj)
{
   E_Smart_Data *sd;
   Eina_List *l;
   E_Randr_Crtc_Info *crtc;
   Evas *evas;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   evas = evas_object_evas_get(sd->o_layout);

   /* loop the crtcs, checking for valid output */
   EINA_LIST_FOREACH(E_RANDR_12->crtcs, l, crtc)
     {
        Eina_List *ll;
        E_Randr_Output_Info *output;

        printf("Checking Crtc: %d\n", crtc->xid);
        printf("\tGeom: %d %d %d %d\n", crtc->geometry.x, 
               crtc->geometry.y, crtc->geometry.w, crtc->geometry.h);

        /* loop the outputs on this crtc */
        EINA_LIST_FOREACH(crtc->outputs, ll, output)
          {
             printf("\tChecking Output: %d\n", output->xid);

             if (output->wired_clones)
               printf("\tHAS WIRED CLONES !!\n");

             if (output->connection_status == 
                 ECORE_X_RANDR_CONNECTION_STATUS_CONNECTED)
               {
                  Evas_Object *m;

                  printf("\t\tConnected\n");
                  if ((m = e_smart_monitor_add(evas)))
                    {
                       /* tell the monitor what layout it belongs to
                        * NB: This is needed so we can calculate best 
                        * thumbnail size (among other things) */
                       e_smart_monitor_layout_set(m, sd->o_layout);

                       /* tell the monitor which output it references */
                       e_smart_monitor_output_set(m, output);

                       /* add monitor to layout */
                       e_layout_pack(sd->o_layout, m);
                       e_layout_child_move(m, crtc->geometry.x, 
                                           crtc->geometry.y);
                       e_layout_child_resize(m, crtc->geometry.w, 
                                             crtc->geometry.h);

                       sd->monitors = eina_list_append(sd->monitors, m);
                    }
               }

             /* else if (output->connection_status ==  */
             /*          ECORE_X_RANDR_CONNECTION_STATUS_DISCONNECTED) */
             /*   printf("\t\tDisconnected\n"); */
             /* else */
             /*   printf("\t\tUnknown\n"); */
          }

        /* loop possible outputs on this crtc */
        EINA_LIST_FOREACH(crtc->possible_outputs, ll, output)
          {
             printf("\tChecking Possible Output: %d\n", output->xid);
             if (output->connection_status == 
                 ECORE_X_RANDR_CONNECTION_STATUS_CONNECTED)
               printf("\t\tConnected\n");
             else if (output->connection_status == 
                      ECORE_X_RANDR_CONNECTION_STATUS_DISCONNECTED)
               printf("\t\tDisconnected\n");
             else
               printf("\t\tUnknown\n");
          }
     }
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
