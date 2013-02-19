#include "e.h"
#include "e_mod_main.h"
#include "e_int_config_randr.h"
#include "e_smart_randr.h"

/* local structures */
struct _E_Config_Dialog_Data
{
   Evas_Object *o_randr;

   int restore;
};

/* local function prototypes */
static void *_create_data(E_Config_Dialog *cfd EINA_UNUSED);
static void _free_data(E_Config_Dialog *cfd EINA_UNUSED, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _basic_apply(E_Config_Dialog *cfd EINA_UNUSED, E_Config_Dialog_Data *cfdata);
static int _basic_check(E_Config_Dialog *cfd EINA_UNUSED, E_Config_Dialog_Data *cfdata);

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

   /* try to allocate dialog view */
   if (!(v = E_NEW(E_Config_Dialog_View, 1)))
     return NULL;

   /* set dialog view functions & properties */
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;
   v->basic.apply_cfdata = _basic_apply;
   v->basic.check_changed = _basic_check;
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
_create_data(E_Config_Dialog *cfd EINA_UNUSED)
{
   E_Config_Dialog_Data *cfdata;

   /* try to allocate the config data structure */
   if (!(cfdata = E_NEW(E_Config_Dialog_Data, 1)))
     return NULL;

   cfdata->restore = e_randr_cfg->restore;

   return cfdata;
}

static void 
_free_data(E_Config_Dialog *cfd EINA_UNUSED, E_Config_Dialog_Data *cfdata)
{
   /* if we have the randr smart widget, delete it */
   if (cfdata->o_randr)
     {
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
   Evas_Object *ow;
   Evas_Coord mw = 0, mh = 0, ch = 0;

   /* create the base list widget */
   o = e_widget_list_add(evas, 0, 0);

   /* try to create randr smart widget */
   if ((cfdata->o_randr = e_smart_randr_add(evas)))
     {
        /* hook into randr widget changed callback */
        evas_object_smart_callback_add(cfdata->o_randr, "randr_changed", 
                                       _randr_cb_changed, cfd);

        /* tell randr widget to calculate virtual size */
        e_smart_randr_virtual_size_calc(cfdata->o_randr);

        /* tell randr widget to create monitors */
        e_smart_randr_monitors_create(cfdata->o_randr);

        /* append randr widget to list object */
        e_widget_list_object_append(o, cfdata->o_randr, 1, 1, 0.5);

        /* ask randr widget to calculate min size */
        e_smart_randr_min_size_get(cfdata->o_randr, &mw, &mh);
     }

   ow = e_widget_check_add(evas, _("Restore On Startup"), &(cfdata->restore));
   e_widget_list_object_append(o, ow, 1, 0, 0.5);
   e_widget_size_min_get(ow, NULL, &ch);

   /* set min size of the list widget */
   e_widget_size_min_set(o, mw, mh + ch);

   e_util_win_auto_resize_fill(cfd->dia->win);
   e_win_centered_set(cfd->dia->win, 1);

   return o;
}

static int 
_basic_apply(E_Config_Dialog *cfd EINA_UNUSED, E_Config_Dialog_Data *cfdata)
{
   e_randr_cfg->restore = cfdata->restore;
   e_randr_config_save();

   e_smart_randr_changes_apply(cfdata->o_randr);

   return 1;
}

static int 
_basic_check(E_Config_Dialog *cfd EINA_UNUSED, E_Config_Dialog_Data *cfdata)
{
   return (e_randr_cfg->restore != cfdata->restore);
}

static void 
_randr_cb_changed(void *data, Evas_Object *obj, void *event EINA_UNUSED)
{
   E_Config_Dialog *cfd;
   Eina_Bool changed = EINA_FALSE;

   if (!(cfd = data)) return;

   changed = e_smart_randr_changed_get(obj);
   e_config_dialog_changed_set(cfd, changed);
}
