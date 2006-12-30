/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

#ifdef __FreeBSD__
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

/***************************************************************************/
/**/
/* gadcon requirements */
static E_Gadcon_Client *_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style);
static void _gc_shutdown(E_Gadcon_Client *gcc);
static void _gc_orient(E_Gadcon_Client *gcc);
static char *_gc_label(void);
static Evas_Object *_gc_icon(Evas *evas);
/* and actually define the gadcon class that this module provides (just 1) */
static const E_Gadcon_Client_Class _gadcon_class =
{
   GADCON_CLIENT_CLASS_VERSION,
     "temperature",
     {
        _gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon
     },
   E_GADCON_CLIENT_STYLE_PLAIN
};
/**/
/***************************************************************************/

/***************************************************************************/
/**/
/* actual module specifics */

static void _temperature_face_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _temperature_face_cb_post_menu(void *data, E_Menu *m);
static void _temperature_sensor_init(Config_Face *inst);
static int  _temperature_cb_check(void *data);
static void _temperature_face_level_set(Config_Face *inst, double level);
static void _temperature_face_cb_menu_configure(void *data, E_Menu *m, E_Menu_Item *mi);

static Evas_Bool _temperature_face_shutdown(Evas_Hash *hash, const char *key, void *hdata, void *fdata);

static E_Config_DD *conf_edd = NULL;
static E_Config_DD *conf_face_edd = NULL;

static Config *temperature_config = NULL;

static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style)
{
   Evas_Object *o;
   E_Gadcon_Client *gcc;
   Config_Face *inst;

   inst = evas_hash_find(temperature_config->faces, id);
   if (!inst)
     {
	inst = E_NEW(Config_Face, 1);
	temperature_config->faces = evas_hash_add(temperature_config->faces, id, inst);
	inst->poll_time = 10.0;
	inst->low = 30;
	inst->high = 80;
	inst->sensor_type = SENSOR_TYPE_NONE;
	inst->sensor_name = NULL;
	inst->sensor_path = NULL;
	inst->units = CELCIUS;
     }
   E_CONFIG_LIMIT(inst->poll_time, 0.5, 1000.0);
   E_CONFIG_LIMIT(inst->low, 0, 100);
   E_CONFIG_LIMIT(inst->high, 0, 220);
   E_CONFIG_LIMIT(inst->units, CELCIUS, FAHRENHEIT);

   o = edje_object_add(gc->evas);
   e_theme_edje_object_set(o, "base/theme/modules/temperature",
			   "e/modules/temperature/main");
   
   gcc = e_gadcon_client_new(gc, style, o);
   gcc->data = inst;
   
   inst->gcc = gcc;
   inst->o_temp = o;
   inst->module = temperature_config->module;
   inst->have_temp = -1;
   inst->temperature_check_timer = 
     ecore_timer_add(inst->poll_time, _temperature_cb_check, inst);
 
   
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN,
				  _temperature_face_cb_mouse_down, inst);
   _temperature_sensor_init(inst);
   _temperature_cb_check(inst);
   return gcc;
}

static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
   Config_Face *inst;
   
   inst = gcc->data;
   if (inst->temperature_check_timer) ecore_timer_del(inst->temperature_check_timer);
   inst->temperature_check_timer = NULL;
   if (inst->o_temp) evas_object_del(inst->o_temp);
   inst->o_temp = NULL;
   if (inst->config_dialog) e_object_del(E_OBJECT(inst->config_dialog));
   inst->config_dialog = NULL;
   if (inst->menu) e_object_del(E_OBJECT(inst->menu));
   inst->menu = NULL;
}

static void
_gc_orient(E_Gadcon_Client *gcc)
{
   Config_Face *inst;
   
   inst = gcc->data;
   e_gadcon_client_aspect_set(gcc, 16, 16);
   e_gadcon_client_min_size_set(gcc, 16, 16);
}
   
static char *
_gc_label(void)
{
   return _("Temperature");
}

static Evas_Object *
_gc_icon(Evas *evas)
{
   Evas_Object *o;
   char buf[4096];
   
   o = edje_object_add(evas);
   snprintf(buf, sizeof(buf), "%s/module.edj",
	    e_module_dir_get(temperature_config->module));
   edje_object_file_set(o, buf, "icon");
   return o;
}
/**/
/***************************************************************************/

/***************************************************************************/
/**/
static void
_temperature_face_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Config_Face *inst;
   Evas_Event_Mouse_Down *ev;
   
   inst = data;
   ev = event_info;
   if ((ev->button == 3) && (!inst->menu))
     {
	E_Menu *mn;
	E_Menu_Item *mi;
	int cx, cy, cw, ch;
	
	mn = e_menu_new();
	e_menu_post_deactivate_callback_set(mn, _temperature_face_cb_post_menu, inst);
	inst->menu = mn;
	
	mi = e_menu_item_new(mn);
	e_menu_item_label_set(mi, _("Configuration"));
	e_util_menu_item_edje_icon_set(mi, "enlightenment/configuration");
	e_menu_item_callback_set(mi, _temperature_face_cb_menu_configure, inst);
	
	e_gadcon_client_util_menu_items_append(inst->gcc, mn, 0);
	
	e_gadcon_canvas_zone_geometry_get(inst->gcc->gadcon,
					  &cx, &cy, &cw, &ch);
	e_menu_activate_mouse(mn,
			      e_util_zone_current_get(e_manager_current_get()),
			      cx + ev->output.x, cy + ev->output.y, 1, 1,
			      E_MENU_POP_DIRECTION_DOWN, ev->timestamp);
	e_util_evas_fake_mouse_up_later(inst->gcc->gadcon->evas,
					ev->button);
     }
}

static void
_temperature_face_cb_post_menu(void *data, E_Menu *m)
{
   Config_Face *inst;

   inst = data;

   if (!inst->menu) return;
   e_object_del(E_OBJECT(inst->menu));
   inst->menu = NULL;
}

static void
_temperature_sensor_init(Config_Face *inst)
{
   Ecore_List *therms;
   char        path[PATH_MAX];
#ifdef __FreeBSD__
   int         len;
#endif

   if ((!inst->sensor_type) || (!inst->sensor_name))
     {
	if (inst->sensor_name) evas_stringshare_del(inst->sensor_name);
	if (inst->sensor_path) evas_stringshare_del(inst->sensor_path);
	inst->sensor_path = NULL;
#ifdef __FreeBSD__
	/* TODO: FreeBSD can also have more temperature sensors! */
	inst->sensor_type = SENSOR_TYPE_FREEBSD;
	inst->sensor_name = evas_stringshare_add("tz0");
#else  
	therms = ecore_file_ls("/proc/acpi/thermal_zone");
	if ((therms) && (!ecore_list_is_empty(therms)))
	  {
	     char *name;

	     name = ecore_list_next(therms);
	     inst->sensor_type = SENSOR_TYPE_LINUX_ACPI;
	     inst->sensor_name = evas_stringshare_add(name);

	     ecore_list_destroy(therms);
	  }
	else
	  {
	     if (therms) ecore_list_destroy(therms);

	     if (ecore_file_exists("/proc/omnibook/temperature"))
	       {
		  inst->sensor_type = SENSOR_TYPE_OMNIBOOK;
		  inst->sensor_name = evas_stringshare_add("dummy");
	       }
	     else if (ecore_file_exists("/sys/devices/temperatures/cpu_temperature"))
	       {
		  inst->sensor_type = SENSOR_TYPE_LINUX_MACMINI;
		  inst->sensor_name = evas_stringshare_add("dummy");
	       }
	     else
	       {
		  /* TODO: Is there I2C devices with more than 3 temperature sensors? */
		  /* TODO: What to do when there is more than one tempX? */
		  therms = ecore_file_ls("/sys/bus/i2c/devices");
		  if (therms)
		    {
		       char *name;

		       while ((name = ecore_list_next(therms)))
			 {
			    char *sensors[] = { "temp1", "temp2", "temp3" };
			    int i;

			    for (i = 0; i < 3; i++)
			      {
				 sprintf(path, "/sys/bus/i2c/devices/%s/%s_input",
				       name, sensors[i]);
				 if (ecore_file_exists(path))
				   {
				      inst->sensor_type = SENSOR_TYPE_LINUX_I2C;
				      inst->sensor_name = evas_stringshare_add(sensors[i]);
				      break;
				   }
			      }
			    if (inst->sensor_type) break;
			 }
		       ecore_list_destroy(therms);
		    }
	       }
	  }
#endif
     }
   if ((inst->sensor_type) &&
       (inst->sensor_name) && 
       (!inst->sensor_path))
     {
	switch (inst->sensor_type)
	  {
	   case SENSOR_TYPE_NONE:
	      break;
	   case SENSOR_TYPE_FREEBSD:
#ifdef __FreeBSD__
	      snprintf(path, sizeof(path), "hw.acpi.thermal.%s.temperature",
		       inst->sensor_name);
	      inst->sensor_path = evas_stringshare_add(path);

	      len = 5;
	      sysctlnametomib(inst->sensor_path, inst->mib, &len);
#endif
	      break;
	   case SENSOR_TYPE_OMNIBOOK:
	      inst->sensor_path = evas_stringshare_add("/proc/omnibook/temperature");
	      break;
	   case SENSOR_TYPE_LINUX_MACMINI:
		  inst->sensor_path = evas_stringshare_add("/sys/devices/temperatures/cpu_temperature");
	      break;
	   case SENSOR_TYPE_LINUX_I2C:
	      therms = ecore_file_ls("/sys/bus/i2c/devices");
	      if (therms)
		{
		   char *name;

		   while ((name = ecore_list_next(therms)))
		     {
			sprintf(path, "/sys/bus/i2c/devices/%s/%s_input",
			      name, inst->sensor_name);
			if (ecore_file_exists(path))
			  {
			     inst->sensor_path = evas_stringshare_add(path);
			     break;
			  }
		     }
		   ecore_list_destroy(therms);
		}
	      break;
	   case SENSOR_TYPE_LINUX_ACPI:
	      snprintf(path, sizeof(path), "/proc/acpi/thermal_zone/%s/temperature",
		       inst->sensor_name);
	      inst->sensor_path = evas_stringshare_add(path);
	      break;
	  }
     }
}

static int
_temperature_cb_check(void *data)
{
   FILE *f;
   int ret = 0;
   Config_Face *inst;
   int temp = 0;
   char buf[4096];
#ifdef __FreeBSD__
   int len;
#endif

   inst = data;

   /* TODO: Make standard parser. Seems to be two types of temperature string:
    * - Somename: <temp> C
    * - <temp>
    */
   switch (inst->sensor_type)
     {
      case SENSOR_TYPE_NONE:
	 /* TODO: Slow down timer? */
	 break;
      case SENSOR_TYPE_FREEBSD:
#ifdef __FreeBSD__
	 len = sizeof(temp);
	 if (sysctl(inst->mib, 5, &temp, &len, NULL, 0) != -1)
	   {
	      temp = (temp - 2732) / 10;
	      ret = 1;
	   }
	 else
	   goto error;
#endif
	 break;
      case SENSOR_TYPE_OMNIBOOK:
	 f = fopen(inst->sensor_path, "r");
	 if (f) 
	   {
	      char dummy[4096];

	      fgets(buf, sizeof(buf), f); buf[sizeof(buf) - 1] = 0;
	      if (sscanf(buf, "%s %s %i", dummy, dummy, &temp) == 3)
		ret = 1;
	      else
		goto error;
	      fclose(f);
	   }
	 else
	   goto error;
	 break;
      case SENSOR_TYPE_LINUX_MACMINI:
	 f = fopen(inst->sensor_path, "rb");
	 if (f)
	   {
	      fgets(buf, sizeof(buf), f); buf[sizeof(buf) - 1] = 0;
	      if (sscanf(buf, "%i", &temp) == 1)
		ret = 1;
	      else
		goto error;
	      fclose(f);
	   }
	 else
	   goto error;
	 break;
      case SENSOR_TYPE_LINUX_I2C:
	 f = fopen(inst->sensor_path, "r");
	 if (f)
	   {
	      fgets(buf, sizeof(buf), f);
	      buf[sizeof(buf) - 1] = 0;

	      /* actuallty read the temp */
	      if (sscanf(buf, "%i", &temp) == 1)
		ret = 1;
	      else
		goto error;
	      /* Hack for temp */
	      temp = temp / 1000;
	      fclose(f);
	   }
	 else
	   goto error;
	 break;
      case SENSOR_TYPE_LINUX_ACPI:
	 f = fopen(inst->sensor_path, "r");
	 if (f)
	   {
	      char *p, *q;
	      fgets(buf, sizeof(buf), f); buf[sizeof(buf) - 1] = 0;
	      fclose(f);
	      p = strchr(buf, ':');
	      if (p)
		{
		   p++;
		   while (*p == ' ') p++;
		   q = strchr(p, ' ');
		   if (q) *q = 0;
		   temp = atoi(p);
		   ret = 1;
		}
	      else
		goto error;
	   }
	 else
	   goto error;
	 break;
     }

   if (inst->units == FAHRENHEIT)
     temp = (temp * 9.0 / 5.0) + 32;

   if (ret)
     {
	char *utf8;

	if (inst->have_temp != 1)
	  {
	     /* enable therm object */
	     edje_object_signal_emit(inst->o_temp, "e,state,known", "");
	     inst->have_temp = 1;
	  }

	if (inst->units == FAHRENHEIT) 
	  snprintf(buf, sizeof(buf), "%i°F", temp);
	else
	  snprintf(buf, sizeof(buf), "%i°C", temp);               
	utf8 = ecore_txt_convert("iso-8859-1", "utf-8", buf);

        _temperature_face_level_set(inst,
				    (double)(temp - inst->low) /
				    (double)(inst->high - inst->low));
	edje_object_part_text_set(inst->o_temp, "e.text.reading", utf8);
	free(utf8);
     }
   else
     {
	if (inst->have_temp != 0)
	  {
	     /* disable therm object */
	     edje_object_signal_emit(inst->o_temp, "e,state,unknown", "");
	     edje_object_part_text_set(inst->o_temp, "e.text.reading", "NO TEMP");
	     _temperature_face_level_set(inst, 0.5);
	     inst->have_temp = 0;
	  }
     }
   return 1;
   
error:
   /* TODO: Error count? Might be a temporary problem */
   /* TODO: Error dialog */
   /* TODO: This should be further up, so that it will affect the gadcon */
   inst->sensor_type = SENSOR_TYPE_NONE;
   if (inst->sensor_name) evas_stringshare_del(inst->sensor_name);
   inst->sensor_name = NULL;
   if (inst->sensor_path) evas_stringshare_del(inst->sensor_path);
   inst->sensor_path = NULL;
   return 1;
}

static void
_temperature_face_level_set(Config_Face *inst, double level)
{
   Edje_Message_Float msg;

   if (level < 0.0) level = 0.0;
   else if (level > 1.0) level = 1.0;
   msg.val = level;
   edje_object_message_send(inst->o_temp, EDJE_MESSAGE_FLOAT, 1, &msg);
}

static void
_temperature_face_cb_menu_configure(void *data, E_Menu *m, E_Menu_Item *mi)
{
   Config_Face *inst;

   inst = data;
   if (inst->config_dialog) return;
   config_temperature_module(inst);
}

static Evas_Bool
_temperature_face_shutdown(Evas_Hash *hash, const char *key, void *hdata, void *fdata)
{
   Config_Face *inst;

   inst = hdata;

   if (inst->sensor_name) evas_stringshare_del(inst->sensor_name);
   if (inst->sensor_path) evas_stringshare_del(inst->sensor_path);
   free(inst);
   return 1;
}

void 
temperature_face_update_config(Config_Face *inst)
{
   if (inst->sensor_path)
     evas_stringshare_del(inst->sensor_path);
   inst->sensor_path = NULL;

   _temperature_sensor_init(inst);

   if (inst->temperature_check_timer) ecore_timer_del(inst->temperature_check_timer);
   inst->temperature_check_timer = 
     ecore_timer_add(inst->poll_time, _temperature_cb_check, inst);
}

/***************************************************************************/
/**/
/* module setup */
EAPI E_Module_Api e_modapi = 
{
   E_MODULE_API_VERSION,
     "Temperature"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   conf_face_edd = E_CONFIG_DD_NEW("Temperature_Config_Face", Config_Face);
#undef T
#undef D
#define T Config_Face
#define D conf_face_edd
   E_CONFIG_VAL(D, T, poll_time, DOUBLE);
   E_CONFIG_VAL(D, T, low, INT);
   E_CONFIG_VAL(D, T, high, INT);
   E_CONFIG_VAL(D, T, sensor_type, INT);
   E_CONFIG_VAL(D, T, sensor_name, STR);
   E_CONFIG_VAL(D, T, units, INT);

   conf_edd = E_CONFIG_DD_NEW("Temperature_Config", Config);
#undef T
#undef D
#define T Config
#define D conf_edd
   E_CONFIG_HASH(D, T, faces, conf_face_edd);

   temperature_config = e_config_domain_load("module.temperature", conf_edd);
   if (!temperature_config)
     {
	temperature_config = E_NEW(Config, 1);
     }
   temperature_config->module = m;
   
   e_gadcon_provider_register(&_gadcon_class);
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   e_gadcon_provider_unregister(&_gadcon_class);
   
   evas_hash_foreach(temperature_config->faces, _temperature_face_shutdown, NULL);
   evas_hash_free(temperature_config->faces);
   free(temperature_config);
   temperature_config = NULL;
   E_CONFIG_DD_FREE(conf_face_edd);
   E_CONFIG_DD_FREE(conf_edd);
   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   e_config_domain_save("module.temperature", conf_edd, temperature_config);
   return 1;
}

EAPI int
e_modapi_about(E_Module *m)
{
   e_module_dialog_show(m, _("Enlightenment Temperature Module"),
			_("A module to measure the <hilight>ACPI Thermal sensor</hilight> on Linux.<br>"
			  "It is especially useful for modern Laptops with high speed<br>"
			  "CPUs that generate a lot of heat."));
   return 1;
}

/**/
/***************************************************************************/
