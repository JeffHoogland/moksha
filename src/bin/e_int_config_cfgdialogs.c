#include "e.h"

static void        *_create_data(E_Config_Dialog *cfd);
static void        _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int         _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int         _advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

struct _E_Config_Dialog_Data 
{
   int auto_apply;

   /* Advanced */
   int default_mode;
};

EAPI E_Config_Dialog *
e_int_config_cfgdialogs(E_Container *con) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   
   v = E_NEW(E_Config_Dialog_View, 1);
   
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;
   v->advanced.apply_cfdata = NULL;
   v->advanced.create_widgets = NULL;
   v->override_auto_apply = 1;
   
   cfd = e_config_dialog_new(con, _("Config Dialog Settings"), "enlightenment/configuration", 0, v, NULL);
   return cfd;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata) 
{
   cfdata->auto_apply = e_config->cfgdlg_auto_apply;
   cfdata->default_mode = e_config->cfgdlg_default_mode;
}

static void *
_create_data(E_Config_Dialog *cfd) 
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _fill_data(cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   free(cfdata);
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   /* Auto Apply is disabled in E for now */
   /* (e_config->cfgdlg_auto_apply = cfdata->auto_apply; */

   e_config->cfgdlg_default_mode = cfdata->default_mode;
   e_config_save_queue();
   return 1;
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o, *of, *ob;
   E_Radio_Group *rg;
   
   o = e_widget_list_add(evas, 0, 0);
      
//   of = e_widget_framelist_add(evas, _("General Settings"), 0);
//   ob = e_widget_check_add(evas, _("Auto-Apply Configuration Changes"), &(cfdata->auto_apply));
//   e_widget_framelist_object_append(of, ob);
//   e_widget_list_object_append(o, of, 1, 1, 0.5);   
   
   of = e_widget_framelist_add(evas, _("Default Dialog Mode"), 0);
   rg = e_widget_radio_group_new(&(cfdata->default_mode));

   ob = e_widget_radio_add(evas, _("Basic Mode"), E_CONFIG_DIALOG_CFDATA_TYPE_BASIC, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Advanced Mode"), E_CONFIG_DIALOG_CFDATA_TYPE_ADVANCED, rg);
   e_widget_framelist_object_append(of, ob);
   
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   return o;
}

static int
_advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   /* Auto Apply is disabled in E for now */
   /* (e_config->cfgdlg_auto_apply = cfdata->auto_apply; */

   e_config->cfgdlg_default_mode = cfdata->default_mode;
   e_config_save_queue();
   return 1;
}

static Evas_Object *
_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o, *ob, *of;
   E_Radio_Group *rg;
   
   o = e_widget_list_add(evas, 0, 0);

//   of = e_widget_framelist_add(evas, _("General Settings"), 0);
//   ob = e_widget_check_add(evas, _("Auto-Apply Configuration Changes"), &(cfdata->auto_apply));
//   e_widget_framelist_object_append(of, ob);
//   e_widget_list_object_append(o, of, 1, 1, 0.5);   
   
   of = e_widget_framelist_add(evas, _("Default Dialog Mode"), 0);
   rg = e_widget_radio_group_new(&(cfdata->default_mode));

   ob = e_widget_radio_add(evas, _("Basic Mode"), E_CONFIG_DIALOG_CFDATA_TYPE_BASIC, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Advanced Mode"), E_CONFIG_DIALOG_CFDATA_TYPE_ADVANCED, rg);
   e_widget_framelist_object_append(of, ob);
   
   e_widget_list_object_append(o, of, 1, 1, 0.5);   
   return o;
}
