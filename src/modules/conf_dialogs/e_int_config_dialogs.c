#include "e.h"

static void        *_create_data(E_Config_Dialog *cfd);
static void        _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int         _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
#if 0
static int         _advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
#endif

struct _E_Config_Dialog_Data 
{
   int cnfmdlg_disabled;
   int cfgdlg_auto_apply;
   int cfgdlg_default_mode;
   int cfgdlg_normal_wins;
};

EAPI E_Config_Dialog *
e_int_config_dialogs(E_Container *con, const char *params __UNUSED__) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   
   if (e_config_dialog_find("E", "_config_config_dialog_dialog")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);
   
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;
   v->override_auto_apply = 1;
   
   cfd = e_config_dialog_new(con,
			     _("Dialog Settings"),
			     "E", "_config_config_dialog_dialog",
			     "preferences-dialogs", 0, v, NULL);
   return cfd;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata) 
{
   cfdata->cnfmdlg_disabled = e_config->cnfmdlg_disabled;
   cfdata->cfgdlg_auto_apply = e_config->cfgdlg_auto_apply;
   cfdata->cfgdlg_default_mode = e_config->cfgdlg_default_mode;
   cfdata->cfgdlg_normal_wins =  e_config->cfgdlg_normal_wins;
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
   E_FREE(cfdata);
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   e_config->cnfmdlg_disabled = cfdata->cnfmdlg_disabled;
   /* Auto Apply is disabled in E for now */
   /* (e_config->cfgdlg_auto_apply = cfdata->cfgdlg_auto_apply; */
   e_config->cfgdlg_default_mode = cfdata->cfgdlg_default_mode;
   e_config->cfgdlg_normal_wins = cfdata->cfgdlg_normal_wins;
   e_config_save_queue();
   return 1;
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o, *of, *ob;
   E_Radio_Group *rg;
   
   o = e_widget_list_add(evas, 0, 0);

   of = e_widget_framelist_add(evas, _("General Settings"), 0);

   ob = e_widget_check_add(evas, _("Disable Confirmation Dialogs"), &(cfdata->cnfmdlg_disabled));
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_check_add(evas, _("Normal Windows"), &(cfdata->cfgdlg_normal_wins));
   e_widget_framelist_object_append(of, ob);
//   ob = e_widget_check_add(evas, _("Auto-Apply Settings Changes"), &(cfdata->cfgdlg_auto_apply));
//   e_widget_framelist_object_append(of, ob);
   
   e_widget_list_object_append(o, of, 1, 1, 0.5);
 
   of = e_widget_framelist_add(evas, _("Default Settings Dialogs Mode"), 0);
   rg = e_widget_radio_group_new(&(cfdata->cfgdlg_default_mode));

   ob = e_widget_radio_add(evas, _("Basic Mode"), E_CONFIG_DIALOG_CFDATA_TYPE_BASIC, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Advanced Mode"), E_CONFIG_DIALOG_CFDATA_TYPE_ADVANCED, rg);
   e_widget_framelist_object_append(of, ob);
   
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   return o;
}

#if 0
static int
_advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   /* Auto Apply is disabled in E for now */
   /* (e_config->cfgdlg_auto_apply = cfdata->auto_apply; */

   e_config->cfgdlg_default_mode = cfdata->default_mode;
   e_config->cfgdlg_normal_wins = cfdata->cfgdlg_normal_wins;
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
//   ob = e_widget_check_add(evas, _("Auto-Apply Settings Changes"), &(cfdata->auto_apply));
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
#endif

