#include "e.h"
#include "e_mod_main.h"
#include "e_int_config_randr.h"
#include "e_smart_randr.h"
#include "e_smart_monitor.h"

/* local structures */
struct _E_Config_Dialog_Data
{
   Evas_Object *o_randr;

   int restore, primary;
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
   cfdata->primary = e_randr_cfg->primary;

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
   Evas_Coord mw = 0, mh = 0, ch = 0, fh = 0;
   Eina_List *l, *monitors = NULL;

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

   monitors = e_smart_randr_monitors_get(cfdata->o_randr);
   if (eina_list_count(monitors) > 1)
     {
        E_Radio_Group *rg;
        Evas_Object *mon, *of;

        of = e_widget_framelist_add(evas, _("Primary Output"), 0);
        rg = e_widget_radio_group_new(&(cfdata->primary));
        EINA_LIST_FOREACH(monitors, l, mon)
          {
             int output;
             const char *name;

             name = e_smart_monitor_name_get(mon);
             output = (int)e_smart_monitor_output_get(mon);

             ow = e_widget_radio_add(evas, name, output, rg);
             e_widget_framelist_object_append(of, ow);
          }

        e_widget_list_object_append(o, of, 1, 0, 0.5);
        e_widget_size_min_get(of, NULL, &fh);
     }

   ow = e_widget_check_add(evas, _("Restore On Startup"), &(cfdata->restore));
   e_widget_list_object_append(o, ow, 1, 0, 0.5);
   e_widget_size_min_get(ow, NULL, &ch);

   /* set min size of the list widget */
   e_widget_size_min_set(o, mw, mh + fh + ch);

   e_util_win_auto_resize_fill(cfd->dia->win);
   e_win_centered_set(cfd->dia->win, 1);

   return o;
}

static int 
_basic_apply(E_Config_Dialog *cfd EINA_UNUSED, E_Config_Dialog_Data *cfdata)
{
   Eina_Bool change_primary = EINA_FALSE;

   change_primary = (e_randr_cfg->primary != cfdata->primary);

   e_randr_cfg->primary = cfdata->primary;
   e_randr_cfg->restore = cfdata->restore;
   e_randr_config_save();

   if (change_primary)
     ecore_x_randr_primary_output_set(ecore_x_window_root_first_get(), 
                                      (Ecore_X_Randr_Output)cfdata->primary);

   e_smart_randr_changes_apply(cfdata->o_randr);

   return 1;
}

static int 
_basic_check(E_Config_Dialog *cfd EINA_UNUSED, E_Config_Dialog_Data *cfdata)
{
   return ((e_randr_cfg->restore != cfdata->restore) || 
           (e_randr_cfg->primary != cfdata->primary));
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
