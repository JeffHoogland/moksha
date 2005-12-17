#include "e.h"
#include "e_mod_main.h"
#include "config.h"

typedef struct _cfdata CFData;
typedef struct _Cfg_File_Data Cfg_File_Data;

struct _cfdata 
{
   int poll_method;
   double poll_time;
   
   int unit_method;
   Unit units;

   int low_method;
   int low_temp;
   int high_method;
   int high_temp;

   int sensor;
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

void
e_int_config_temperature(E_Container *con, Temperature *temp) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View v;
   
   v.create_cfdata = _create_data;
   v.free_cfdata = _free_data;
   v.basic.apply_cfdata = _basic_apply_data;
   v.basic.create_widgets = _basic_create_widgets;
   v.advanced.apply_cfdata = _advanced_apply_data;
   v.advanced.create_widgets = _advanced_create_widgets;
   
   cfd = e_config_dialog_new(con, _("Temperature Module"), NULL, 0, &v, temp);
}

static void
_fill_data(Temperature *t, CFData *cfdata) 
{
   double p;
   
   cfdata->units = t->conf->units;
   if (t->conf->units == CELCIUS) 
     {
	cfdata->unit_method = 0;
     }
   else 
     {
	cfdata->unit_method = 1;
     }
   
   p = t->conf->poll_time;
   cfdata->poll_time = p;
   if ((p >= 0) && (p <= 5)) 
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
	cfdata->poll_method = 60; //Very Slow
     }
   
   p = t->conf->low;
   cfdata->low_temp = p;
   if ((p >= 0) && (p <= 40)) 
     {
	cfdata->low_method = 40;
     }
   else if ((p > 40) && (p <= 80)) 
     {
	cfdata->low_method = 80;
     }
   else if (p > 80) 
     {
	cfdata->low_method = 100;
     }

   p = t->conf->high;
   cfdata->high_temp = p;
   if ((p >= 0) && (p <= 60)) 
     {
	cfdata->high_method = 60;
     }
   else if ((p > 60) && (p <= 140)) 
     {
	cfdata->high_method = 140;
     }
   else if (p > 140) 
     {
	cfdata->high_method = 220;
     }
   
   if (!strcmp(t->conf->sensor_name, "temp1")) 
     {
	cfdata->sensor = 0;
     }
   else if (!strcmp(t->conf->sensor_name, "temp2")) 
     {
	cfdata->sensor = 1;
     }
   else if (!strcmp(t->conf->sensor_name, "temp3")) 
     {
	cfdata->sensor = 2;
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
   Temperature *t;
   
   t = cfd->data;
   _fill_data(t, cfdata);
   
   o = e_widget_list_add(evas, 0, 0);
   of = e_widget_framelist_add(evas, _("Display Units"), 0);
   rg = e_widget_radio_group_new(&(cfdata->unit_method));
   ob = e_widget_radio_add(evas, _("Celcius"), 0, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Fahrenheit"), 1, rg);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   
   of = e_widget_framelist_add(evas, _("Check Interval"), 0);
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

   of = e_widget_framelist_add(evas, _("Low Temperature"), 0);
   rg = e_widget_radio_group_new(&(cfdata->low_method));
   ob = e_widget_radio_add(evas, _("40 F"), 40, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("80 F"), 80, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("100 F"), 100, rg);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   of = e_widget_framelist_add(evas, _("High Temperature"), 0);
   rg = e_widget_radio_group_new(&(cfdata->high_method));
   ob = e_widget_radio_add(evas, _("60 F"), 60, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("140 F"), 140, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("220 F"), 220, rg);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   
   return o;
}

static int
_basic_apply_data(E_Config_Dialog *cfd, CFData *cfdata) 
{
   Temperature *t;
   
   t = cfd->data;
   e_border_button_bindings_ungrab_all();
   if (cfdata->unit_method == 0) 
     {
	t->conf->units = CELCIUS;	
     }
   else 
     {
	t->conf->units = FAHRENHEIT;
     }
   
   t->conf->poll_time = (double)cfdata->poll_method;
   if (cfdata->low_method == 40) 
     {
	t->conf->low = (10 + (30 * t->conf->units));	
     }
   else if (cfdata->low_method == 80) 
     {
	t->conf->low = (30 + (50 * t->conf->units));		
     }
   else if (cfdata->low_method == 100) 
     {
	t->conf->low = (50 + (70 * t->conf->units));		
     }
   
   if (cfdata->high_method == 60) 
     {
	t->conf->high = (20 + (40 * t->conf->units));
     }
    else if (cfdata->high_method == 140) 
     {
	t->conf->high = (60 + (80 * t->conf->units));
     }
    else if (cfdata->high_method == 220) 
     {
	t->conf->high = (100 + (140 * t->conf->units));
     }

   e_border_button_bindings_grab_all();
   e_config_save_queue();
   
   /* Call Config Update */
   _temperature_face_cb_config_updated(t);
   return 1;
}

static Evas_Object
*_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, CFData *cfdata) 
{
   Evas_Object *o, *of, *ob;
   E_Radio_Group *rg;
   Temperature *t;
   
   t = cfd->data;
   _fill_data(t, cfdata);
   
   o = e_widget_list_add(evas, 0, 0);
   of = e_widget_framelist_add(evas, _("Display Units"), 0);
   rg = e_widget_radio_group_new(&(cfdata->unit_method));
   ob = e_widget_radio_add(evas, _("Celcius"), 0, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Fahrenheit"), 1, rg);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

#ifndef __FreeBSD__
   Ecore_List *therms;

   therms = ecore_file_ls("/proc/acpi/thermal_zone");
   if ((!therms) || (ecore_list_is_empty(therms)))
     {
	FILE *f;
	
	if (therms)
	  {
	     ecore_list_destroy(therms);
	     therms = NULL;
	  }

	f = fopen("/sys/devices/temperatures/cpu_temperature", "rb");
	if (f) fclose(f);
	
	if (!f)
	  {
	     therms = ecore_file_ls("/sys/bus/i2c/devices");
	     if (therms)
	       {
		  if (!ecore_list_is_empty(therms))
		    {
		       of = e_widget_framelist_add(evas, _("Sensors"), 0);
		       rg = e_widget_radio_group_new(&(cfdata->sensor));
		       ob = e_widget_radio_add(evas, _("Temp 1"), 0, rg);
		       e_widget_framelist_object_append(of, ob);
		       ob = e_widget_radio_add(evas, _("Temp 2"), 1, rg);
		       e_widget_framelist_object_append(of, ob);
		       ob = e_widget_radio_add(evas, _("Temp 3"), 2, rg);
		       e_widget_framelist_object_append(of, ob);   
		       e_widget_list_object_append(o, of, 1, 1, 0.5);
		    }
		  ecore_list_destroy(therms);
		  therms = NULL;
	       }
	  }
     }
   if (therms) ecore_list_destroy(therms);
#endif

   of = e_widget_framelist_add(evas, _("Check Interval"), 0);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.1f seconds"), 0.5, 1000.0, 0.5, 0, &(cfdata->poll_time), NULL, 200);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   of = e_widget_framelist_add(evas, _("Low Temperature"), 0);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f F"), 0, 100, 5, 0, NULL, &(cfdata->low_temp), 200);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   of = e_widget_framelist_add(evas, _("High Temperature"), 0);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f F"), 0, 220, 5, 0, NULL, &(cfdata->high_temp), 200);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
      
   return o;
}

static int
_advanced_apply_data(E_Config_Dialog *cfd, CFData *cfdata) 
{
   Temperature *t;
   
   t = cfd->data;
   
   e_border_button_bindings_ungrab_all();
   if (cfdata->unit_method == 0) 
     {
	t->conf->units = CELCIUS;	
     }
   else 
     {
	t->conf->units = FAHRENHEIT;
     }
   
   t->conf->poll_time = cfdata->poll_time;

   if (cfdata->low_temp <= 40) 
     {
	t->conf->low = (10 + (30 * t->conf->units));	
     }
   else if (cfdata->low_temp <= 80) 
     {
	t->conf->low = (30 + (50 * t->conf->units));		
     }
   else if (cfdata->low_temp <= 120) 
     {
	t->conf->low = (50 + (70 * t->conf->units));		
     }
   
   if (cfdata->high_temp <= 60) 
     {
	t->conf->high = (20 + (40 * t->conf->units));
     }
    else if (cfdata->high_temp <= 80) 
     {
	t->conf->high = (30 + (50 * t->conf->units));
     }
    else if (cfdata->high_temp <= 100) 
     {
	t->conf->high = (40 + (60 * t->conf->units));
     }
    else if (cfdata->high_temp <= 120) 
     {
	t->conf->high = (50 + (70 * t->conf->units));
     }
    else if (cfdata->high_temp <= 140) 
     {
	t->conf->high = (60 + (80 * t->conf->units));
     }
    else if (cfdata->high_temp <= 160) 
     {
	t->conf->high = (70 + (90 * t->conf->units));
     }
    else if (cfdata->high_temp <= 180) 
     {
	t->conf->high = (80 + (100 * t->conf->units));
     }
    else if (cfdata->high_temp <= 200) 
     {
	t->conf->high = (90 + (120 * t->conf->units));
     }
    else if (cfdata->high_temp <= 220) 
     {
	t->conf->high = (100 + (140 * t->conf->units));
     }

   switch (cfdata->sensor) 
     {
      case 0:
	t->conf->sensor_name = strdup("temp1");
	break;
      case 1:
	t->conf->sensor_name = strdup("temp2");
	break;
      case 2:
	t->conf->sensor_name = strdup("temp3");
	break;
     }
   
   e_border_button_bindings_grab_all();
   e_config_save_queue();
   
   /* Call Config Update */
   _temperature_face_cb_config_updated(t);

   return 1;
}
