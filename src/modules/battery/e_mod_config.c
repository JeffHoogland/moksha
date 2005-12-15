#include "e.h"
#include "e_mod_main.h"
#include "config.h"

typedef struct _cfdata CFData;
typedef struct _Cfg_File_Data Cfg_File_Data;

struct _cfdata 
{
   double poll_time;
   int alarm_time;
   int poll_method;
   int alarm_method;
};

struct _Cfg_File_Data 
{
   E_Config_Dialog *cfd;
   char *file;
};

/* Protos */
static Evas_Object   *_create_widgets(E_Config_Dialog *cfd, Evas *evas, Config *cfdata);
static void          *_create_data(E_Config_Dialog *cfd);
static void          _free_data(E_Config_Dialog *cfd, CFData *cfdata);
static Evas_Object   *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, CFData *cfdata);
static int           _basic_apply_data(E_Config_Dialog *cfd, CFData *cfdata);
static Evas_Object   *_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, CFData *cfdata);
static int           _advanced_apply_data(E_Config_Dialog *cfd, CFData *cfdata);

Battery *b = NULL;

void
e_int_config_battery(E_Container *con, Battery *bat) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View v;
   
   b = bat;
   
   v.create_cfdata = _create_data;
   v.free_cfdata = _free_data;
   v.basic.apply_cfdata = _basic_apply_data;
   v.basic.create_widgets = _basic_create_widgets;
   v.advanced.apply_cfdata = _advanced_apply_data;
   v.advanced.create_widgets = _advanced_create_widgets;
   
   cfd = e_config_dialog_new(con, _("Battery Module"), NULL, 0, &v, bat);
}

static void
_fill_data(CFData *cfdata) 
{
   double p;
   int a;
   
   /* Fill Data */
   p = b->conf->poll_time;
   cfdata->poll_time = p;
   if ((p >= 1) && (p <= 5)) 
     {
	cfdata->poll_method = 1; //Fast
     }
   else if ((p > 5) && (p <= 10)) 
     {
	cfdata->poll_method = 10; //Normal
     }   
   else if ((p > 10) && (p <= 30)) 
     {
	cfdata->poll_method = 30; //Slow
     }
   else if (p > 30) 
     {
	cfdata->poll_method = 60; // Very Slow
     }
   
   a = b->conf->alarm;
   cfdata->alarm_time = a;
   if (a == 0) 
     {
	cfdata->alarm_method = 0; //Disable
     }
   else if (a <=10) 
     {
	cfdata->alarm_method = 10; // 10 mins
     }
   else if (a <=30) 
     {
	cfdata->alarm_method = 30; // 10 mins
     }
   else if (a <=60) 
     {
	cfdata->alarm_method = 60; // 10 mins
     }
   
}

static void
*_create_data(E_Config_Dialog *cfd) 
{
   CFData *cfdata;
   cfdata = E_NEW(CFData, 1);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, CFData *cfdata) 
{
   free(cfdata);
}

static Evas_Object
*_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, CFData *cfdata) 
{
   Evas_Object *o, *of, *ob;
   E_Radio_Group *rg;
   
   _fill_data(cfdata);
   
   o = e_widget_list_add(evas, 0, 0);
   of = e_widget_framelist_add(evas, _("Poll Time"), 0);
   rg = e_widget_radio_group_new(&(cfdata->poll_method));
   ob = e_widget_radio_add(evas, _("Fast"), 1, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Normal"), 10, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Slow"), 30, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Very Slow"), 60, rg);
   e_widget_framelist_object_append(of, ob);   
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   
   of = e_widget_framelist_add(evas, _("Alarm Time"), 0);
   rg = e_widget_radio_group_new(&(cfdata->alarm_method));
   
   ob = e_widget_radio_add(evas, _("Disable"), 0, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("10 Mins"), 10, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("30 Mins"), 30, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("1 Hour"), 60, rg);
   e_widget_framelist_object_append(of, ob);

   e_widget_list_object_append(o, of, 1, 1, 0.5);
   return o;
}

static int 
_basic_apply_data(E_Config_Dialog *cfd, CFData *cfdata) 
{
   e_border_button_bindings_ungrab_all();
   
   b->conf->poll_time = (double)cfdata->poll_method;
   b->conf->alarm = cfdata->alarm_method;
   
   e_border_button_bindings_grab_all();
   e_config_save_queue();
   
   _battery_face_cb_config_updated(b);
   return 1;
}

static Evas_Object
*_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, CFData *cfdata) 
{
   Evas_Object *o, *of, *ob;
   E_Radio_Group *rg;
   
   /* Use Sliders for both cfg options */
   _fill_data(cfdata);

   o = e_widget_list_add(evas, 0, 0);
   of = e_widget_framelist_add(evas, _("Poll Time"), 0);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.1f seconds"), 0.5, 1000.0, 0.5, 0, &(cfdata->poll_time), NULL, 200);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   of = e_widget_framelist_add(evas, _("Alarm Time"), 0);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f minutes"), 0, 60, 1, 0, NULL, &(cfdata->alarm_time), 200);
   e_widget_framelist_object_append(of, ob);
   
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   return o;
}

static int 
_advanced_apply_data(E_Config_Dialog *cfd, CFData *cfdata) 
{
   e_border_button_bindings_ungrab_all();
   
   b->conf->poll_time = cfdata->poll_time;
   b->conf->alarm = cfdata->alarm_time;
   
   e_border_button_bindings_grab_all();
   e_config_save_queue();
   
   _battery_face_cb_config_updated(b);
   return 1;
}
