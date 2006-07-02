#include "e.h"
#include "e_mod_main.h"

struct _E_Config_Dialog_Data
{
   int show_alert;   
   double poll_time;   
   int alarm_time;
};

/* Protos */
static void          *_create_data(E_Config_Dialog *cfd);
static void          _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object   *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int           _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object   *_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int           _advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);

void
_config_battery_module(void) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   
   v = E_NEW(E_Config_Dialog_View, 1);
   
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;
   v->advanced.apply_cfdata = _advanced_apply_data;
   v->advanced.create_widgets = _advanced_create_widgets;
   
   cfd = e_config_dialog_new(e_container_current_get(e_manager_current_get()), 
			     _("Battery Monitor Configuration"), 
			     NULL, 0, v, NULL);
   battery_config->config_dialog = cfd;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata) 
{
   if (!battery_config) return;
   cfdata->alarm_time = battery_config->alarm;
   cfdata->poll_time = battery_config->poll_time;
   if (cfdata->alarm_time > 0) 
     cfdata->show_alert = 1;
   else 
     cfdata->show_alert = 0;
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
   if (!battery_config) return;
   battery_config->config_dialog = NULL;
   free(cfdata);
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o, *of, *ob;
      
   o = e_widget_list_add(evas, 0, 0);
   of = e_widget_framelist_add(evas, _("Basic Settings"), 0);
   ob = e_widget_check_add(evas, _("Show alert when battery is low"), &(cfdata->show_alert));
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   return o;
}

static int 
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   if (!battery_config) return 0;
   if (cfdata->show_alert) 
     battery_config->alarm = cfdata->alarm_time;
   else 
     battery_config->alarm = 0;
   _battery_config_updated();
   e_config_save_queue();
   return 1;
}

static Evas_Object *
_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o, *of, *ob;
   
   /* Use Sliders for both cfg options */
   o = e_widget_list_add(evas, 0, 0);
   of = e_widget_frametable_add(evas, _("Advanced Settings"), 1);
   
   ob = e_widget_label_add(evas, _("Check battery every:"));
   e_widget_frametable_object_append(of, ob, 0, 0, 1, 1, 1, 0, 1, 0);
   
   ob = e_widget_slider_add(evas, 1, 0, _("%1.1f seconds"), 0.5, 1000.0, 0.5, 0, &(cfdata->poll_time), NULL, 200);
   e_widget_frametable_object_append(of, ob, 0, 1, 1, 1, 1, 0, 1, 0);
   
   ob = e_widget_check_add(evas, _("Show alert when battery is low"), &(cfdata->show_alert));
   e_widget_frametable_object_append(of, ob, 0, 3, 1, 1, 1, 1, 1, 0);   
   
   ob = e_widget_label_add(evas, _("Alert when battery is down to:"));
   e_widget_frametable_object_append(of, ob, 0, 4, 1, 1, 1, 0, 1, 1);
   
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f minutes"), 1, 60, 1, 0, NULL, &(cfdata->alarm_time), 200);
   e_widget_frametable_object_append(of, ob, 0, 5, 1, 1, 1, 0, 1, 0);

   e_widget_list_object_append(o, of, 1, 1, 0.5);
   return o;
}

static int 
_advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   if (!battery_config) return 0;
   battery_config->poll_time = cfdata->poll_time;
   if (cfdata->show_alert) 
     battery_config->alarm = cfdata->alarm_time;
   else 
     battery_config->alarm = 0;
   _battery_config_updated();
   e_config_save_queue();
   return 1;
}

