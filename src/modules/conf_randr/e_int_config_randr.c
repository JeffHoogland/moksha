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
static void *_create_data(E_Config_Dialog *cfd EINA_UNUSED);
static void _free_data(E_Config_Dialog *cfd EINA_UNUSED, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _basic_apply(E_Config_Dialog *cfd EINA_UNUSED, E_Config_Dialog_Data *cfdata);
static void _randr_cb_changed(void *data, Evas_Object *obj, void *event EINA_UNUSED);

/* public functions */
E_Config_Dialog *
e_int_config_randr(E_Container *con, const char *params EINA_UNUSED)
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
   e_dialog_resizable_set(cfd->dia, 1);

   return cfd;
}

/* local functions */
static void *
_create_data(E_Config_Dialog *cfd EINA_UNUSED)
{
   E_Config_Dialog_Data *cfdata;

   /* try to allocate the config data structure */
   if (!(cfdata = E_NEW(E_Config_Dialog_Data, 1)))
     return NULL;

   return cfdata;
}

static void 
_free_data(E_Config_Dialog *cfd EINA_UNUSED, E_Config_Dialog_Data *cfdata)
{
   /* if we have the randr smart widget, delete it */
   if (cfdata->o_randr)
     {
        /* delete the hook into randr widget changed signal */
        evas_object_smart_callback_del(cfdata->o_randr, "changed", 
                                       _randr_cb_changed);

        /* delete the randr object */
        evas_object_del(cfdata->o_randr);
     }

   /* free the config data structure */
   E_FREE(cfdata);
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o;
   Evas_Coord cw = 0, ch = 0;

   /* create the base list widget */
   o = e_widget_list_add(evas, 0, 0);

   /* try to create randr smart widget */
   if ((cfdata->o_randr = e_smart_randr_add(evas)))
     {
        Evas_Coord lw = 0, lh = 0;

        /* ask randr widget to compute best layout size based on the 
         * size of available crtcs */
        e_smart_randr_layout_size_get(cfdata->o_randr, &lw, &lh);

        /* calculate virtual size
         * 
         * NB: Get which size is larger. This is done so that the 
         * virtual canvas size can be set such that monitors may be 
         * repositioned easily in a horizontal or vertical layout.
         * Without using MAX (and just using current size) than a 
         * horizontal layout cannot be changed into a vertical layout */
        cw = MAX(lw, lh);
        ch = MAX(lw, lh);

        /* set the virtual size for the randr widget */
        e_smart_randr_current_size_set(cfdata->o_randr, cw, ch);

        /* tell randr widget to create monitors */
        e_smart_randr_monitors_create(cfdata->o_randr);

        /* hook into randr widget changed signal */
        evas_object_smart_callback_add(cfdata->o_randr, "changed", 
                                       _randr_cb_changed, cfd);

        /* add randr widget to the base widget */
        e_widget_list_object_append(o, cfdata->o_randr, 1, 1, 0.5);
     }

   /* set a minimum size to 1/10th */
   e_widget_size_min_set(o, (cw / 10), (ch / 10));

   return o;
}

static int 
_basic_apply(E_Config_Dialog *cfd EINA_UNUSED, E_Config_Dialog_Data *cfdata)
{
   /* tell randr widget to apply changes */
   e_smart_randr_changes_apply(cfdata->o_randr);

   /* return success */
   return 1;
}

static void 
_randr_cb_changed(void *data, Evas_Object *obj, void *event EINA_UNUSED)
{
   E_Config_Dialog *cfd;
   Eina_Bool changed = EINA_FALSE;

   if (!(cfd = data)) return;

   /* get randr widget changed state */
   changed = e_smart_randr_changed_get(obj);

   /* update dialog with changed state */
   e_config_dialog_changed_set(cfd, changed);
}
