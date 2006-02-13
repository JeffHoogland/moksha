/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"
#include "config.h"

/* celsius */
#define TEMP_LOW_LOW	32
#define TEMP_LOW_MID	43
#define TEMP_LOW_HIGH	55
#define TEMP_HIGH_LOW	43
#define TEMP_HIGH_MID	65
#define TEMP_HIGH_HIGH	93

#define FAR_2_CEL(x) (x - 32) / 1.8
#define CEL_2_FAR(x) (x * 1.8) + 32

struct _E_Config_Dialog_Data
{
   int poll_method;
   double poll_time;
   
   int unit_method;
   Unit units;

   int low_method;
   int low_temp;
   int high_method;
   int high_temp;

   int allow_overlap;

   int sensor;
};

/* Protos */
static void          *_create_data(E_Config_Dialog *cfd);
static void          _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object   *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int           _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object   *_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int           _advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);

void
_config_temperature_module(E_Container *con, Temperature *temp) 
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
   
   cfd = e_config_dialog_new(con, _("Temperature Configuration"), NULL, 0, v, temp);
   temp->config_dialog = cfd;
}

static void
_fill_data(Temperature *t, E_Config_Dialog_Data *cfdata) 
{
   double p;
   
   cfdata->units = t->conf->units;
   if (t->conf->units == CELCIUS) 
     cfdata->unit_method = 0;
   else 
     cfdata->unit_method = 1;
   
   p = t->conf->poll_time;
   cfdata->poll_time = p;
   if ((p >= 0) && (p <= 5)) 
     cfdata->poll_method = 1; //Fast
   else if ((p > 5) && (p <= 10)) 
     cfdata->poll_method = 10; //Normal
   else if ((p > 10) && (p <= 30)) 
     cfdata->poll_method = 30; //Slow
   else if (p > 30) 
     cfdata->poll_method = 60; //Very Slow
   
   p = t->conf->low;
   if (cfdata->units == FAHRENHEIT)
     p = FAR_2_CEL(p - 1); // -1 so the conversion doesn't make mid go hi
   cfdata->low_temp = p;
   if ((p >= 0) && (p <= TEMP_LOW_LOW)) 
     cfdata->low_method = TEMP_LOW_LOW;
   else if ((p > TEMP_LOW_LOW) && (p <= TEMP_LOW_MID)) 
     cfdata->low_method = TEMP_LOW_MID;
   else if (p > TEMP_LOW_MID) 
     cfdata->low_method = TEMP_LOW_HIGH;

   p = t->conf->high;
   if (cfdata->units == FAHRENHEIT)
     p = FAR_2_CEL(p - 1);
   cfdata->high_temp = p;
   if ((p >= 0) && (p <= TEMP_HIGH_LOW)) 
     cfdata->high_method = TEMP_HIGH_LOW;
   else if ((p > TEMP_HIGH_LOW) && (p <= TEMP_HIGH_MID)) 
     cfdata->high_method = TEMP_HIGH_MID;
   else if (p > TEMP_HIGH_MID) 
     cfdata->high_method = TEMP_HIGH_HIGH;
   
   if (!strcmp(t->conf->sensor_name, "temp1")) 
     cfdata->sensor = 0;
   else if (!strcmp(t->conf->sensor_name, "temp2")) 
     cfdata->sensor = 1;
   else if (!strcmp(t->conf->sensor_name, "temp3")) 
     cfdata->sensor = 2;

   cfdata->allow_overlap = t->conf->allow_overlap;
}

static void *
_create_data(E_Config_Dialog *cfd) 
{
   E_Config_Dialog_Data *cfdata;
   Temperature *t;
   
   t = cfd->data;
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _fill_data(t, cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   Temperature *t;
   
   t = cfd->data;
   t->config_dialog = NULL;
   free(cfdata);
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o, *of, *ob;
   E_Radio_Group *rg;
   
   o = e_widget_list_add(evas, 0, 0);
   of = e_widget_framelist_add(evas, _("Display Units"), 0);
   rg = e_widget_radio_group_new(&(cfdata->unit_method));
   ob = e_widget_radio_add(evas, _("Celsius"), 0, rg);
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

   
   if (cfdata->units == FAHRENHEIT)
     {
	of = e_widget_framelist_add(evas, _("High Temperature"), 0);
	rg = e_widget_radio_group_new(&(cfdata->high_method));

	ob = e_widget_radio_add(evas, _("200 F"), TEMP_HIGH_HIGH, rg);
	e_widget_framelist_object_append(of, ob);
	ob = e_widget_radio_add(evas, _("150 F"), TEMP_HIGH_MID, rg);
	e_widget_framelist_object_append(of, ob);
	ob = e_widget_radio_add(evas, _("110 F"), TEMP_HIGH_LOW, rg);
	e_widget_framelist_object_append(of, ob);
	e_widget_list_object_append(o, of, 1, 1, 0.5);
	
	of = e_widget_framelist_add(evas, _("Low Temperature"), 0);
	rg = e_widget_radio_group_new(&(cfdata->low_method));

	ob = e_widget_radio_add(evas, _("130 F"), TEMP_LOW_HIGH, rg);
	e_widget_framelist_object_append(of, ob);
	ob = e_widget_radio_add(evas, _("110 F"), TEMP_LOW_MID, rg);
	e_widget_framelist_object_append(of, ob);
	ob = e_widget_radio_add(evas, _("90 F"), TEMP_LOW_LOW, rg);
	e_widget_framelist_object_append(of, ob);
	e_widget_list_object_append(o, of, 1, 1, 0.5);
     }
   else
     {
	of = e_widget_framelist_add(evas, _("High Temperature"), 0);
	rg = e_widget_radio_group_new(&(cfdata->high_method));

	ob = e_widget_radio_add(evas, _("93 C"), TEMP_HIGH_HIGH, rg);
	e_widget_framelist_object_append(of, ob);
	ob = e_widget_radio_add(evas, _("65 C"), TEMP_HIGH_MID, rg);
	e_widget_framelist_object_append(of, ob);
	ob = e_widget_radio_add(evas, _("43 C"), TEMP_HIGH_LOW, rg);
	e_widget_framelist_object_append(of, ob);
	e_widget_list_object_append(o, of, 1, 1, 0.5);
	
	of = e_widget_framelist_add(evas, _("Low Temperature"), 0);
	rg = e_widget_radio_group_new(&(cfdata->low_method));

	ob = e_widget_radio_add(evas, _("55 C"), TEMP_LOW_HIGH, rg);
	e_widget_framelist_object_append(of, ob);
	ob = e_widget_radio_add(evas, _("43 C"), TEMP_LOW_MID, rg);
	e_widget_framelist_object_append(of, ob);
	ob = e_widget_radio_add(evas, _("32 C"), TEMP_LOW_LOW, rg);
	e_widget_framelist_object_append(of, ob);
	e_widget_list_object_append(o, of, 1, 1, 0.5);
     }

   of = e_widget_framelist_add(evas, _("Extras"), 0);
   ob = e_widget_check_add(evas, _("Allow windows to overlap this gadget"), &(cfdata->allow_overlap));
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   return o;
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   Temperature *t;
   
   t = cfd->data;
   e_border_button_bindings_ungrab_all();
   if (cfdata->unit_method == 0) 
     t->conf->units = CELCIUS;	
   else 
     t->conf->units = FAHRENHEIT;
   
   t->conf->poll_time = (double)cfdata->poll_method;

   if (t->conf->units == FAHRENHEIT)
     {
	t->conf->low = CEL_2_FAR(cfdata->low_method);
	t->conf->high = CEL_2_FAR(cfdata->high_method);
     }
   else
     {
	t->conf->low = cfdata->low_method;
	t->conf->high = cfdata->high_method;
     }

   if (cfdata->allow_overlap && !t->conf->allow_overlap)
     t->conf->allow_overlap = 1;
   else if (!cfdata->allow_overlap && t->conf->allow_overlap)
     t->conf->allow_overlap = 0;

   e_border_button_bindings_grab_all();
   e_config_save_queue();
   
   /* Call Config Update */
   _temperature_face_cb_config_updated(t);
   return 1;
}

static Evas_Object *
_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o, *of, *ob;
   E_Radio_Group *rg;
   Temperature *t;
   
   t = cfd->data;
   
   o = e_widget_list_add(evas, 0, 0);
   of = e_widget_framelist_add(evas, _("Display Units"), 0);
   rg = e_widget_radio_group_new(&(cfdata->unit_method));
   ob = e_widget_radio_add(evas, _("Celsius"), 0, rg);
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

   cfdata->low_temp = t->conf->low;
   cfdata->high_temp = t->conf->high;

   if (t->conf->units == FAHRENHEIT)
     {
	/* round-off to closest 5 */
	if (cfdata->high_temp % 5 > 3)
	  cfdata->high_temp += 5 - (cfdata->high_temp % 5);
	else
	  cfdata->high_temp -= (cfdata->high_temp % 5);

	if (cfdata->low_temp % 5 > 3)
	  cfdata->low_temp += 5 - (cfdata->low_temp % 5);
	else
	  cfdata->low_temp -= (cfdata->low_temp % 5);

	of = e_widget_framelist_add(evas, _("High Temperature"), 0);
	ob = e_widget_slider_add(evas, 1, 0, _("%1.0f F"), 0, 230, 5, 0, NULL, &(cfdata->high_temp), 200);
	e_widget_framelist_object_append(of, ob);
	e_widget_list_object_append(o, of, 1, 1, 0.5);
	
	of = e_widget_framelist_add(evas, _("Low Temperature"), 0);
	ob = e_widget_slider_add(evas, 1, 0, _("%1.0f F"), 0, 200, 5, 0, NULL, &(cfdata->low_temp), 200);
	e_widget_framelist_object_append(of, ob);
	e_widget_list_object_append(o, of, 1, 1, 0.5);
     }
   else
     {
	of = e_widget_framelist_add(evas, _("High Temperature"), 0);
	ob = e_widget_slider_add(evas, 1, 0, _("%1.0f C"), 0, 110, 1, 0, NULL, &(cfdata->high_temp), 200);
	e_widget_framelist_object_append(of, ob);
	e_widget_list_object_append(o, of, 1, 1, 0.5);
	
	of = e_widget_framelist_add(evas, _("Low Temperature"), 0);
	ob = e_widget_slider_add(evas, 1, 0, _("%1.0f C"), 0, 95, 1, 0, NULL, &(cfdata->low_temp), 200);
	e_widget_framelist_object_append(of, ob);
	e_widget_list_object_append(o, of, 1, 1, 0.5);
     }  

   of = e_widget_framelist_add(evas, _("Extras"), 0);
   ob = e_widget_check_add(evas, _("Allow windows to overlap this gadget"), &(cfdata->allow_overlap));
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   return o;
}

static int
_advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   Temperature *t;
   
   t = cfd->data;
   
   e_border_button_bindings_ungrab_all();

   /* Check if Display Units has been toggled */
   if (cfdata->unit_method != t->conf->units)
     {
	if (cfdata->unit_method == 0)
	  {
	     cfdata->high_temp = FAR_2_CEL(cfdata->high_temp);
	     cfdata->low_temp = FAR_2_CEL(cfdata->low_temp);
	  }
	else
	  {
	     cfdata->high_temp = CEL_2_FAR(cfdata->high_temp);
	     cfdata->low_temp = CEL_2_FAR(cfdata->low_temp);
	  }
     }

   if (cfdata->unit_method == 0) 
     t->conf->units = CELCIUS;	
   else 
     t->conf->units = FAHRENHEIT;
   
   t->conf->poll_time = cfdata->poll_time;

   t->conf->low = cfdata->low_temp;
   t->conf->high = cfdata->high_temp;

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
   
   if (cfdata->allow_overlap && !t->conf->allow_overlap)
     t->conf->allow_overlap = 1;
   else if (!cfdata->allow_overlap && t->conf->allow_overlap)
     t->conf->allow_overlap = 0;

   e_border_button_bindings_grab_all();
   e_config_save_queue();
   
   /* Call Config Update */
   _temperature_face_cb_config_updated(t);

   return 1;
}
