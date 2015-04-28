#include "e.h"
#include "e_mod_main.h"

#define FAR_2_CEL(x) ((x - 32) / 9.0) * 5.0
#define CEL_2_FAR(x) (x * 9.0 / 5.0) + 32

struct _E_Config_Dialog_Data 
{
   struct 
     {
        int interval;
     } poll;

   int unit_method;
#ifdef HAVE_EEZE
   int backend;
#endif
   struct 
     {
        int low, high;
     } temp;

   int sensor;
   Eina_List *sensors;

   Evas_Object *o_high, *o_low;

   Config_Face *inst;
};

/* local function prototypes */
static void *_create_data(E_Config_Dialog *cfd);
static void _fill_data_tempget(E_Config_Dialog_Data *cfdata);
static void _fill_sensors(E_Config_Dialog_Data *cfdata, const char *name);
static void _free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static void _cb_display_changed(void *data, Evas_Object *obj __UNUSED__);

void 
config_temperature_module(Config_Face *inst) 
{
   E_Config_Dialog_View *v;
   char buff[PATH_MAX];

   v = E_NEW(E_Config_Dialog_View, 1);
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;
   v->basic.apply_cfdata = _basic_apply;

   snprintf(buff, sizeof(buff), 
            "%s/e-module-temperature.edj", inst->module->dir);
   inst->config_dialog = 
     e_config_dialog_new(e_container_current_get(e_manager_current_get()), 
                         _("Temperature Settings"), "E", 
                         "_e_mod_temperature_config_dialog", buff, 0, v, inst);
}

/* local function prototypes */
static void *
_create_data(E_Config_Dialog *cfd) 
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfdata->inst = cfd->data;
   _fill_data_tempget(cfdata);
   return cfdata;
}

static void 
_fill_data_tempget(E_Config_Dialog_Data *cfdata) 
{
   cfdata->unit_method = cfdata->inst->units;
   cfdata->poll.interval = cfdata->inst->poll_interval;
   cfdata->temp.low = cfdata->inst->low;
   cfdata->temp.high = cfdata->inst->high;
   cfdata->sensor = 0;
#ifdef HAVE_EEZE
   cfdata->backend = cfdata->inst->backend;
   if (cfdata->backend == TEMPGET)
     {
#endif
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
             _fill_sensors(cfdata, "i2c");
             break;
           case SENSOR_TYPE_LINUX_PCI:
             _fill_sensors(cfdata, "pci");
             break;
           case SENSOR_TYPE_LINUX_ACPI: 
               {
                  Eina_List *l;

                  if ((l = ecore_file_ls("/proc/acpi/thermal_zone")))
                    {
                       char *name;
                       int n = 0;

                       EINA_LIST_FREE(l, name) 
                         {
                            cfdata->sensors = 
                              eina_list_append(cfdata->sensors, name);
                            if (!strcmp(cfdata->inst->sensor_name, name))
                              cfdata->sensor = n;
                            n++;
                         }
                    }
                  break;
               }
           case SENSOR_TYPE_LINUX_SYS: 
               {
                  Eina_List *l;
                  
                  if ((l = ecore_file_ls("/sys/class/thermal")))
                    {
                       char *name;
                       int n = 0;
                       
                       EINA_LIST_FREE(l, name) 
                         {
                            if (!strncmp(name, "thermal", 7))
                              {
                                 cfdata->sensors = 
                                    eina_list_append(cfdata->sensors, name);
                                 if (!strcmp(cfdata->inst->sensor_name, name))
                                    cfdata->sensor = n;
                                 n++;
                              }
                         }
                    }
                  break;
               }
           default:
             break;
          }
#ifdef HAVE_EEZE
   }
#endif
}

static void 
_fill_sensors(E_Config_Dialog_Data *cfdata, const char *name) 
{
   Eina_List *therms, *l;
   char *n;

   if (!name) return;
   if ((therms = temperature_get_bus_files(name)))
     {
        char path[PATH_MAX];

        EINA_LIST_FREE(therms, n) 
          {
             if (ecore_file_exists(n)) 
               {
                  int len;

                  sprintf(path, "%s", ecore_file_file_get(n));
                  len = strlen(path);
                  if (len > 6) path[len - 6] = '\0';
                  cfdata->sensors = 
                    eina_list_append(cfdata->sensors, strdup(path));
               }
             free(n);
          }
     }
   EINA_LIST_FOREACH(cfdata->sensors, l, n) 
     {
        if (!strcmp(cfdata->inst->sensor_name, n)) break;
        cfdata->sensor++;
     }
}

static void 
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata) 
{
   char *sensor;

   cfdata->inst->config_dialog = NULL;
   EINA_LIST_FREE(cfdata->sensors, sensor)
     free(sensor);
   E_FREE(cfdata);
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *otb, *ol, *ow;
   E_Radio_Group *rg;

   otb = e_widget_toolbook_add(evas, 24, 24);

   if (cfdata->sensors) 
     {
        Eina_List *l;
        char *name;
        int n = 0;

        ol = e_widget_list_add(evas, 0, 0);
        rg = e_widget_radio_group_new(&(cfdata->sensor));
        EINA_LIST_FOREACH(cfdata->sensors, l, name) 
          {
             ow = e_widget_radio_add(evas, _(name), n, rg);
             e_widget_list_object_append(ol, ow, 1, 0, 0.5);
             n++;
          }
        e_widget_toolbook_page_append(otb, NULL, _("Sensors"), ol, 
                                      1, 0, 1, 0, 0.5, 0.0);
     }

   ol = e_widget_list_add(evas, 0, 0);
   rg = e_widget_radio_group_new(&(cfdata->unit_method));
   ow = e_widget_radio_add(evas, _("Celsius"), CELCIUS, rg);
   e_widget_on_change_hook_set(ow, _cb_display_changed, cfdata);
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);
   ow = e_widget_radio_add(evas, _("Fahrenheit"), FAHRENHEIT, rg);
   e_widget_on_change_hook_set(ow, _cb_display_changed, cfdata);
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Display Units"), ol, 
                                 0, 0, 0, 0, 0.5, 0.0);

   ol = e_widget_list_add(evas, 0, 0);
   ow = e_widget_slider_add(evas, 1, 0, _("%1.0f ticks"), 1, 1024, 4, 0, 
                            NULL, &(cfdata->poll.interval), 150);
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Check Interval"), ol, 
                                 1, 0, 1, 0, 0.5, 0.0);

   ol = e_widget_list_add(evas, 0, 0);
   ow = e_widget_label_add(evas, _("High Temperature"));
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);
   if (cfdata->unit_method == FAHRENHEIT)
     cfdata->o_high = 
     e_widget_slider_add(evas, 1, 0, _("%1.0f F"), 0, 230, 5, 0, 
                         NULL, &(cfdata->temp.high), 150);
   else
     cfdata->o_high = 
     e_widget_slider_add(evas, 1, 0, _("%1.0f C"), 0, 110, 5, 0, 
                         NULL, &(cfdata->temp.high), 150);
   e_widget_list_object_append(ol, cfdata->o_high, 1, 1, 0.5);

   ow = e_widget_label_add(evas, _("Low Temperature"));
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);
   if (cfdata->unit_method == FAHRENHEIT)
     cfdata->o_low = 
     e_widget_slider_add(evas, 1, 0, _("%1.0f F"), 0, 200, 5, 0, 
                         NULL, &(cfdata->temp.low), 150);
   else
     cfdata->o_low = 
     e_widget_slider_add(evas, 1, 0, _("%1.0f C"), 0, 95, 5, 0, 
                         NULL, &(cfdata->temp.low), 150);
   e_widget_list_object_append(ol, cfdata->o_low, 1, 1, 0.5);

   e_widget_toolbook_page_append(otb, NULL, _("Temperatures"), ol, 
                                 1, 0, 1, 0, 0.5, 0.0);
#ifdef HAVE_EEZE
   ol = e_widget_list_add(evas, 0, 0);
   rg = e_widget_radio_group_new(&(cfdata->backend));
   ow = e_widget_radio_add(evas, _("Internal"), TEMPGET, rg);
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);
   ow = e_widget_radio_add(evas, _("udev"), UDEV, rg);
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Hardware"), ol, 
                                 0, 0, 0, 0, 0.5, 0.0);
#endif
   e_widget_toolbook_page_show(otb, 0);
   return otb;
}

static int 
_basic_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata) 
{
   cfdata->inst->poll_interval = cfdata->poll.interval;
   cfdata->inst->units = cfdata->unit_method;
   cfdata->inst->low = cfdata->temp.low;
   cfdata->inst->high = cfdata->temp.high;
#ifdef HAVE_EEZE
   cfdata->inst->backend = cfdata->backend;
#endif

   eina_stringshare_replace(&cfdata->inst->sensor_name, 
                            eina_list_nth(cfdata->sensors, cfdata->sensor));

   e_config_save_queue();
   temperature_face_update_config(cfdata->inst);
   return 1;
}

static void 
_cb_display_changed(void *data, Evas_Object *obj __UNUSED__) 
{
   E_Config_Dialog_Data *cfdata;
   int val;

   if (!(cfdata = data)) return;
   if (cfdata->unit_method == FAHRENHEIT)
     {
        e_widget_slider_value_range_set(cfdata->o_low, 0, 200);
        e_widget_slider_value_range_set(cfdata->o_high, 0, 230);
        e_widget_slider_value_int_get(cfdata->o_low, &val);
        e_widget_slider_value_int_set(cfdata->o_low, CEL_2_FAR(val));
        e_widget_slider_value_int_get(cfdata->o_high, &val);
        e_widget_slider_value_int_set(cfdata->o_high, CEL_2_FAR(val));
        e_widget_slider_value_format_display_set(cfdata->o_low, _("%1.0f F"));
        e_widget_slider_value_format_display_set(cfdata->o_high, _("%1.0f F"));
     }
   else
     {
        e_widget_slider_value_range_set(cfdata->o_low, 0, 95);
        e_widget_slider_value_range_set(cfdata->o_high, 0, 110);
        e_widget_slider_value_int_get(cfdata->o_low, &val);
        e_widget_slider_value_int_set(cfdata->o_low, FAR_2_CEL(val));
        e_widget_slider_value_int_get(cfdata->o_high, &val);
        e_widget_slider_value_int_set(cfdata->o_high, FAR_2_CEL(val));
        e_widget_slider_value_format_display_set(cfdata->o_low, _("%1.0f C"));
        e_widget_slider_value_format_display_set(cfdata->o_high, _("%1.0f C"));
     }
}
