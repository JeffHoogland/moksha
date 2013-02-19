#include "e.h"
#include "e_mod_main.h"
#include "e_smart_monitor.h"

/* local structure */
typedef struct _E_Smart_Data E_Smart_Data;
struct _E_Smart_Data
{
   /* canvas variable */
   Evas *evas;

   /* crtc config */
   E_Randr_Crtc_Config *crtc;

   /* output config */
   E_Randr_Output_Config *output;

   /* visibility flag */
   Eina_Bool visible : 1;
}

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
e_smart_monitor_crtc_set(Evas_Object *obj, E_Randr_Crtc_Config *crtc)
{
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* set the crtc config */
   sd->crtc = crtc;
}

void 
e_smart_monitor_output_set(Evas_Object *obj, E_Randr_Output_Config *output)
{
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* set the output config */
   sd->output = output;
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

   /* set the objects smart data */
   evas_object_smart_data_set(obj, sd);
}

static void 
_e_smart_del(Evas_Object *obj)
{
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

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
}

static void 
_e_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;
}

static void 
_e_smart_show(Evas_Object *obj)
{
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* if we are already visible, then nothing to do */
   if (sd->visible) return;

   /* set visibility flag */
   sd->visible = EINA_TRUE;
}

static void 
_e_smart_hide(Evas_Object *obj)
{
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;

   /* if we are already hidden, then nothing to do */
   if (!sd->visible) return;

   /* set visibility flag */
   sd->visible = EINA_FALSE;
}

static void 
_e_smart_clip_set(Evas_Object *obj, Evas_Object *clip)
{
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;
}

static void 
_e_smart_clip_unset(Evas_Object *obj)
{
   E_Smart_Data *sd;

   /* try to get the objects smart data */
   if (!(sd = evas_object_smart_data_get(obj))) return;
}
