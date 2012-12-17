#include "e.h"
#include "e_mod_main.h"
#include "e_int_config_randr.h"
#include "e_smart_randr.h"

/* local structures */
struct _E_Config_Dialog_Data
{
   Evas_Object *o_randr;
};

/* local function prototypes */
static void *_create_data(E_Config_Dialog *cfd __UNUSED__);
static void _free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _basic_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata);

/* public functions */
E_Config_Dialog *
e_int_config_randr(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   /* check for existing dialog */
   if (e_config_dialog_find("E", "screen/screen_setup"))
     return NULL;

   /* check for valid XRandR protocol version */
   if (e_randr_screen_info.randr_version < ECORE_X_RANDR_1_2)
     return NULL;

   /* try to allocate dialog view */
   if (!(v = E_NEW(E_Config_Dialog_View, 1)))
     return NULL;

   /* set dialog view functions & properties */
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;
   v->basic.apply_cfdata = _basic_apply;
   v->override_auto_apply = EINA_TRUE;

   /* create new dialog */
   cfd = e_config_dialog_new(con, _("Screen Setup"), 
                             "E", "screen/screen_setup",
                             "preferences-system-screen-resolution", 
                             0, v, NULL);

   return cfd;
}

/* local functions */
static void *
_create_data(E_Config_Dialog *cfd __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;

   /* try to allocate the config data structure */
   if (!(cfdata = E_NEW(E_Config_Dialog_Data, 1)))
     return NULL;

   return cfdata;
}

static void 
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   /* if we have the randr smart widget, delete it */
   if (cfdata->o_randr)
     evas_object_del(cfdata->o_randr);

   /* free the config data structure */
   E_FREE(cfdata);
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o;

   /* create the base list widget */
   o = e_widget_list_add(evas, 0, 0);

   /* try to create randr smart widget */
   if ((cfdata->o_randr = e_smart_randr_add(evas)))
     {
        /* set the virtual size for the randr widget */
        e_smart_randr_current_size_set(cfdata->o_randr, 
                                       E_RANDR_12->current_size.width, 
                                       E_RANDR_12->current_size.height);

        /* tell randr widget to create monitors */
        e_smart_randr_monitors_create(cfdata->o_randr);

        /* add randr widget to the base widget */
        e_widget_list_object_append(o, cfdata->o_randr, 1, 1, 0.5);
     }

   /* set a minimum size */
   e_widget_size_min_set(o, (E_RANDR_12->current_size.width / 10), 
                         (E_RANDR_12->current_size.height / 10));

   /* set this dialog to be resizable */
   e_dialog_resizable_set(cfd->dia, EINA_TRUE);

   /* return the base widget */
   return o;
}

static int 
_basic_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   return 1;
}
