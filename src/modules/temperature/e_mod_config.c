/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

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
   int poll_interval;
   
   int unit_method;
   Unit units;

   int low_method;
   int low_temp;
   int high_method;
   int high_temp;

   int sensor;
   Ecore_List *sensors;

   Config_Face *inst;
};

/* Protos */
static void          *_create_data(E_Config_Dialog *cfd);
static void          _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object   *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int           _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object   *_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int           _advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);

void
config_temperature_module(Config_Face *inst) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   char buf[4096];
   
   v = E_NEW(E_Config_Dialog_View, 1);
   
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;
   v->advanced.apply_cfdata = _advanced_apply_data;
   v->advanced.create_widgets = _advanced_create_widgets;

   snprintf(buf, sizeof(buf), "%s/e-module-temperature.edj", e_module_dir_get(inst->module));
   cfd = e_config_dialog_new(e_container_current_get(e_manager_current_get()),
			     _("Temperature Settings"),
			     "E", "_e_mod_temperature_config_dialog",
			     buf, 0, v, inst);
   inst->config_dialog = cfd;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata) 
{
   double      p;
   int         pi;
   Ecore_List *therms;
   char       *name;
   char        path[PATH_MAX];
   
   cfdata->units = cfdata->inst->units;
   if (cfdata->inst->units == CELCIUS) 
     cfdata->unit_method = 0;
   else 
     cfdata->unit_method = 1;
   
   pi = cfdata->inst->poll_interval;
   cfdata->poll_interval = pi;
   if ((pi >= 1) && (pi <= 64)) 
     cfdata->poll_method = 4; /* Fast */
   else if ((pi > 64) && (pi <= 128)) 
     cfdata->poll_method = 128; /* Normal */
   else if ((pi > 128) && (pi <= 256)) 
     cfdata->poll_method = 256; /* Slow */
   else if (pi > 256) 
     cfdata->poll_method = 512; /* Very Slow */
   
   p = cfdata->inst->low;
   if (cfdata->units == FAHRENHEIT)
     p = FAR_2_CEL(p - 1); /* -1 so the conversion doesn't make mid go hi */
   cfdata->low_temp = p;
   if ((p >= 0) && (p <= TEMP_LOW_LOW)) 
     cfdata->low_method = TEMP_LOW_LOW;
   else if ((p > TEMP_LOW_LOW) && (p <= TEMP_LOW_MID)) 
     cfdata->low_method = TEMP_LOW_MID;
   else if (p > TEMP_LOW_MID) 
     cfdata->low_method = TEMP_LOW_HIGH;

   p = cfdata->inst->high;
   if (cfdata->units == FAHRENHEIT)
     p = FAR_2_CEL(p - 1);
   cfdata->high_temp = p;
   if ((p >= 0) && (p <= TEMP_HIGH_LOW)) 
     cfdata->high_method = TEMP_HIGH_LOW;
   else if ((p > TEMP_HIGH_LOW) && (p <= TEMP_HIGH_MID)) 
     cfdata->high_method = TEMP_HIGH_MID;
   else if (p > TEMP_HIGH_MID) 
     cfdata->high_method = TEMP_HIGH_HIGH;
   
   cfdata->sensor = 0;
   switch (cfdata->inst->sensor_type)
     {
      case SENSOR_TYPE_NONE:
      case SENSOR_TYPE_FREEBSD:
      case SENSOR_TYPE_OMNIBOOK:
      case SENSOR_TYPE_LINUX_MACMINI:
      case SENSOR_TYPE_LINUX_PBOOK:
      case SENSOR_TYPE_LINUX_INTELCORETEMP:
	 break;
      case SENSOR_TYPE_LINUX_I2C:
	 therms = temperature_get_bus_files("i2c");
	 if (therms)
	   {
              char *name;

	      while ((name = ecore_list_next(therms)))
	        {
		   if (ecore_file_exists(name))
		     {
		        int len;

			sprintf(path, "%s", ecore_file_file_get(name));
			len = strlen(path);
			if (len > 6)
			   path[len - 6] = '\0';
	                ecore_list_append(cfdata->sensors, strdup(path));
			/* TODO: Track down the user friendly names and display them instead.
			 * User friendly names are not available on the system, lm-sensors 
			 * contains a database in /etc/sensors.conf, but the format may change,
			 * so the best way to use that database is thru libsensors, but we 
			 * don't want to add any more library dependencies. 
			 */
		     }
		}
	      ecore_list_destroy(therms);
	   }

	 ecore_list_first_goto(cfdata->sensors);
	 while ((name = ecore_list_next(cfdata->sensors)))
	   {
	      if (!strcmp(cfdata->inst->sensor_name, name)) 
		break;
	      cfdata->sensor++;
	   }
	 break;
      case SENSOR_TYPE_LINUX_PCI:
	 therms = temperature_get_bus_files("pci");
	 if (therms)
	   {
              char *name;

	      while ((name = ecore_list_next(therms)))
	        {
		   if (ecore_file_exists(name))
		     {
		        int len;

			sprintf(path, "%s", ecore_file_file_get(name));
			len = strlen(path);
			if (len > 6)
			   path[len - 6] = '\0';
	                ecore_list_append(cfdata->sensors, strdup(path));
			/* TODO: Track down the user friendly names and display them instead.
			 * User friendly names are not available on the system, lm-sensors 
			 * contains a database in /etc/sensors.conf, but the format may change,
			 * so the best way to use that database is thru libsensors, but we 
			 * don't want to add any more library dependencies. 
			 */
		     }
		}
	      ecore_list_destroy(therms);
	   }

	 ecore_list_first_goto(cfdata->sensors);
	 while ((name = ecore_list_next(cfdata->sensors)))
	   {
	      if (!strcmp(cfdata->inst->sensor_name, name)) 
		break;
	      cfdata->sensor++;
	   }
	 break;
      case SENSOR_TYPE_LINUX_ACPI:
	 therms = ecore_file_ls("/proc/acpi/thermal_zone");
	 if (therms)
	   {
	      int n = 0;

	      while ((name = ecore_list_next(therms)))
		{
		   ecore_list_append(cfdata->sensors, strdup(name));
		   if (!strcmp(cfdata->inst->sensor_name, name))
		     {
			cfdata->sensor = n;
		     }
		   n++;
		}
	      ecore_list_destroy(therms);
	   }
	 break;
     }
}

static void *
_create_data(E_Config_Dialog *cfd) 
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfdata->inst = cfd->data;
   cfdata->sensors = ecore_list_new();
   ecore_list_free_cb_set(cfdata->sensors, free);
   _fill_data(cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   cfdata->inst->config_dialog = NULL;
   if (cfdata->sensors) ecore_list_destroy(cfdata->sensors);
   cfdata->sensors = NULL;
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
   ob = e_widget_radio_add(evas, _("Fast"), 4, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Normal"), 128, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Slow"), 256, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Very Slow"), 512, rg);
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

   return o;
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   if (cfdata->unit_method == 0) 
     cfdata->inst->units = CELCIUS;	
   else 
     cfdata->inst->units = FAHRENHEIT;
   cfdata->inst->poll_interval = cfdata->poll_method;
   if (cfdata->inst->units == FAHRENHEIT)
     {
	cfdata->inst->low = CEL_2_FAR(cfdata->low_method);
	cfdata->inst->high = CEL_2_FAR(cfdata->high_method);
     }
   else
     {
	cfdata->inst->low = cfdata->low_method;
	cfdata->inst->high = cfdata->high_method;
     }
   temperature_face_update_config(cfdata->inst);
   e_config_save_queue();
   return 1;
}

static Evas_Object *
_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
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

   if (!ecore_list_empty_is(cfdata->sensors))
     {
	/* TODO: Notify user which thermal system is in use */
	/* TODO: Let the user choose the wanted thermal system */
	char *name;
	int   n = 0;

	of = e_widget_framelist_add(evas, _("Sensors"), 0);
	rg = e_widget_radio_group_new(&(cfdata->sensor));
	ecore_list_first_goto(cfdata->sensors);
	while ((name = ecore_list_next(cfdata->sensors)))
	  {
	     ob = e_widget_radio_add(evas, _(name), n, rg);
	     e_widget_framelist_object_append(of, ob);
	     n++;
	  }
	e_widget_list_object_append(o, of, 1, 1, 0.5);
     }

   of = e_widget_framelist_add(evas, _("Check Interval"), 0);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f ticks"), 1, 1024, 4, 0, NULL, &(cfdata->poll_interval), 200);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   if (cfdata->units == FAHRENHEIT)
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

   return o;
}

static int
_advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   if (cfdata->unit_method != cfdata->inst->units)
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
     cfdata->inst->units = CELCIUS;	
   else 
     cfdata->inst->units = FAHRENHEIT;
   cfdata->inst->poll_interval = cfdata->poll_interval;
   cfdata->inst->low = cfdata->low_temp;
   cfdata->inst->high = cfdata->high_temp;
   if (cfdata->inst->sensor_name)
     eina_stringshare_del(cfdata->inst->sensor_name);
   cfdata->inst->sensor_name =
     eina_stringshare_add(ecore_list_index_goto(cfdata->sensors, cfdata->sensor));

   temperature_face_update_config(cfdata->inst);
   e_config_save_queue();
   return 1;
}
