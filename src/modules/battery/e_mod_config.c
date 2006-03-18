#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"
#include "config.h"

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
_config_battery_module(E_Container *con, Battery *bat) 
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
   
   cfd = e_config_dialog_new(con, _("Battery Configuration"), NULL, 0, v, bat);
   bat->config_dialog = cfd;
}

static void
_fill_data(Battery *b, E_Config_Dialog_Data *cfdata) 
{
   cfdata->alarm_time = b->conf->alarm;
   cfdata->poll_time = b->conf->poll_time;
   if (cfdata->alarm_time > 0) 
     cfdata->show_alert = 1;
   else 
     cfdata->show_alert = 0;
}

static void *
_create_data(E_Config_Dialog *cfd) 
{
   E_Config_Dialog_Data *cfdata;
   Battery *b;
   
   b = cfd->data;
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _fill_data(b, cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   Battery *b;
   
   b = cfd->data;
   b->config_dialog = NULL;
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
   Battery *b;
   
   b = cfd->data;
   e_border_button_bindings_ungrab_all();
   b->conf->poll_time = 10.0;   

   e_border_button_bindings_grab_all();
   e_config_save_queue();
   
   _battery_face_cb_config_updated(b);
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
   Battery *b;
   
   b = cfd->data;
   e_border_button_bindings_ungrab_all();
   
   b->conf->poll_time = cfdata->poll_time;
   if (cfdata->show_alert) 
     b->conf->alarm = cfdata->alarm_time;
   else 
     b->conf->alarm = 0;

   e_border_button_bindings_grab_all();
   e_config_save_queue();
   
   _battery_face_cb_config_updated(b);
   return 1;
}
