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
     }
};
/**/
/***************************************************************************/

/***************************************************************************/
/**/
/* actual module specifics */

typedef struct _Instance Instance;

struct _Instance
{
   E_Gadcon_Client *gcc;
   Evas_Object     *o_temp;
};

static void _button_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _menu_cb_post(void *data, E_Menu *m);
static int _temperature_cb_check(void *data);
static void _temperature_face_level_set(Instance *inst, double level);
static void _temperature_face_cb_menu_configure(void *data, E_Menu *m, E_Menu_Item *mi);

static E_Config_DD *conf_edd = NULL;

Config *temperature_config = NULL;

static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style)
{
   Evas_Object *o;
   E_Gadcon_Client *gcc;
   Instance *inst;
   
   inst = E_NEW(Instance, 1);
   
   o = edje_object_add(gc->evas);
   e_theme_edje_object_set(o, "base/theme/modules/temperature",
			   "modules/temperature/main");
   
   gcc = e_gadcon_client_new(gc, name, id, style, o);
   gcc->data = inst;
   
   inst->gcc = gcc;
   inst->o_temp = o;
   
   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN,
				  _button_cb_mouse_down, inst);
   temperature_config->instances = evas_list_append(temperature_config->instances, inst);
   temperature_config->have_temp = -1;
   _temperature_cb_check(NULL);
   return gcc;
}

static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
   Instance *inst;
   
   inst = gcc->data;
   temperature_config->instances = evas_list_remove(temperature_config->instances, inst);
   evas_object_del(inst->o_temp);
   free(inst);
}

static void
_gc_orient(E_Gadcon_Client *gcc)
{
   Instance *inst;
   
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
   snprintf(buf, sizeof(buf), "%s/module.eap",
	    e_module_dir_get(temperature_config->module));
   edje_object_file_set(o, buf, "icon");
   return o;
}
/**/
/***************************************************************************/

/***************************************************************************/
/**/
static void
_button_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Instance *inst;
   Evas_Event_Mouse_Down *ev;
   
   inst = data;
   ev = event_info;
   if ((ev->button == 3) && (!temperature_config->menu))
     {
	E_Menu *mn;
	E_Menu_Item *mi;
	int cx, cy, cw, ch;
	
	mn = e_menu_new();
	e_menu_post_deactivate_callback_set(mn, _menu_cb_post, inst);
	temperature_config->menu = mn;
	
	mi = e_menu_item_new(mn);
	e_menu_item_label_set(mi, _("Configuration"));
	e_util_menu_item_edje_icon_set(mi, "enlightenment/configuration");
	e_menu_item_callback_set(mi, _temperature_face_cb_menu_configure, NULL);
	
	e_gadcon_client_util_menu_items_append(inst->gcc, mn, 0);
	
	e_gadcon_canvas_zone_geometry_get(inst->gcc->gadcon,
					  &cx, &cy, &cw, &ch);
	e_menu_activate_mouse(mn,
			      e_util_zone_current_get(e_manager_current_get()),
			      cx + ev->output.x, cy + ev->output.y, 1, 1,
			      E_MENU_POP_DIRECTION_DOWN, ev->timestamp);
	evas_event_feed_mouse_up(inst->gcc->gadcon->evas, ev->button,
				 EVAS_BUTTON_NONE, ev->timestamp, NULL);
     }
}

static void
_menu_cb_post(void *data, E_Menu *m)
{
   if (!temperature_config->menu) return;
   e_object_del(E_OBJECT(temperature_config->menu));
   temperature_config->menu = NULL;
}

static int
_temperature_cb_check(void *data)
{
   int ret = 0;
   Instance *inst;
   Ecore_List *therms;
   Evas_List *l;
   int temp = 0;
   char buf[4096];
#ifdef __FreeBSD__
   static int mib[5] = {-1};
   int len;
#endif

#ifdef __FreeBSD__
   if (mib[0] == -1)
     {
	len = 5;
	sysctlnametomib("hw.acpi.thermal.tz0.temperature", mib, &len);
     }
   
   if (mib[0] != -1)
     {
	len = sizeof(temp);
        if (sysctl(mib, 5, &temp, &len, NULL, 0) != -1)
	  {
	     temp = (temp - 2732) / 10;
	     ret = 1;
	  }
     }
#else  
   therms = ecore_file_ls("/proc/acpi/thermal_zone");
   if ((!therms) || ecore_list_is_empty(therms))
     {
	FILE *f;

	if (therms) ecore_list_destroy(therms);

	f = fopen("/sys/devices/temperatures/cpu_temperature", "rb");
	if (f)
	  {
	     fgets(buf, sizeof(buf), f); buf[sizeof(buf) - 1] = 0;
	     if (sscanf(buf, "%i", &temp) == 1)
	       ret = 1;
	     fclose(f);
	  }
	else
	  {
	     therms = ecore_file_ls("/sys/bus/i2c/devices");
	     if (therms)
	       {
		  const char *name, *sensor;

		  sensor = temperature_config->sensor_name;
		  if (!sensor) sensor = "temp1";

		  while ((name = ecore_list_next(therms)))
		    {
		       char fname[1024];

		       sprintf(fname, "/sys/bus/i2c/devices/%s/%s_input",
			       name, sensor);
		       if (ecore_file_exists(fname))
			 {
			    FILE *f;

			    f = fopen(fname,"r");
			    if (f) 
			      {
				 fgets(buf, sizeof(buf), f);
				 buf[sizeof(buf) - 1] = 0;

				 /* actuallty read the temp */
				 if (sscanf(buf, "%i", &temp) == 1)
				   ret = 1;
				 /* Hack for temp */
				 temp = temp / 1000;
				 fclose(f);
			      }
			 }
		    }
		  ecore_list_destroy(therms);
	       }
	  }
     }
   else
     {
	char *name;

	ret = 1;
	while ((name = ecore_list_next(therms)))
	  {
	     char *p, *q;
	     FILE *f;

	     snprintf(buf, sizeof(buf), "/proc/acpi/thermal_zone/%s/temperature", name);
	     f = fopen(buf, "rb");
	     if (f)
	       {
		  fgets(buf, sizeof(buf), f); buf[sizeof(buf) - 1] = 0;
		  fclose(f);
		  p = strchr(buf, ':');
		  if (!p)
		    {
		       ret = 0;
		       continue;
		    }
		  p++;
		  while (*p == ' ') p++;
		  q = strchr(p, ' ');
		  if (q) *q = 0;
		  temp = atoi(p);
	       }
	  }
	ecore_list_destroy(therms);
     }
#endif   
   
   if (temperature_config->units == FAHRENHEIT)
     temp = (temp * 9.0 / 5.0) + 32;

   if (ret)
     {
	char *utf8;

	if (temperature_config->have_temp != 1)
	  {
	     /* enable therm object */
	     for (l = temperature_config->instances; l; l = l->next)
	       {
		  inst = l->data;
		  edje_object_signal_emit(inst->o_temp, "known", "");
	       }
	     temperature_config->have_temp = 1;
	  }

	if (temperature_config->units == FAHRENHEIT) 
	  snprintf(buf, sizeof(buf), "%i°F", temp);
	else
	  snprintf(buf, sizeof(buf), "%i°C", temp);               
	utf8 = ecore_txt_convert("iso-8859-1", "utf-8", buf);

	for (l = temperature_config->instances; l; l = l->next)
	  {
	     inst = l->data;
	     _temperature_face_level_set(inst,
					 (double)(temp - temperature_config->low) /
					 (double)(temperature_config->high - temperature_config->low));
	     edje_object_part_text_set(inst->o_temp, "reading", utf8);
	  }
	free(utf8);
     }
   else
     {
	if (temperature_config->have_temp != 0)
	  {
	     /* disable therm object */
	     for (l = temperature_config->instances; l; l = l->next)
	       {
		  inst = l->data;
		  edje_object_signal_emit(inst->o_temp, "unknown", "");
		  edje_object_part_text_set(inst->o_temp, "reading", "NO TEMP");
		  _temperature_face_level_set(inst, 0.5);
	       }
	     temperature_config->have_temp = 0;
	  }
     }
   return 1;
}

static void
_temperature_face_level_set(Instance *inst, double level)
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
   if (!temperature_config) return;
   if (temperature_config->config_dialog) return;
   _config_temperature_module();
}

void 
_temperature_face_cb_config_updated(void)
{
   ecore_timer_del(temperature_config->temperature_check_timer);
   temperature_config->temperature_check_timer = 
     ecore_timer_add(temperature_config->poll_time, _temperature_cb_check, 
		     NULL);
}

/***************************************************************************/
/**/
/* module setup */
EAPI E_Module_Api e_modapi = 
{
   E_MODULE_API_VERSION,
     "Temperature"
};

EAPI int
e_modapi_init(E_Module *m)
{
   conf_edd = E_CONFIG_DD_NEW("Temperature_Config", Config);
#undef T
#undef D
#define T Config
#define D conf_edd
   E_CONFIG_VAL(D, T, poll_time, DOUBLE);
   E_CONFIG_VAL(D, T, low, INT);
   E_CONFIG_VAL(D, T, high, INT);
   E_CONFIG_VAL(D, T, sensor_name, STR);
   E_CONFIG_VAL(D, T, units, INT);

   temperature_config = e_config_domain_load("module.temperature", conf_edd);
   if (!temperature_config)
     {
	temperature_config = E_NEW(Config, 1);
	temperature_config->poll_time = 10.0;
	temperature_config->low = 30;
	temperature_config->high = 80;
	temperature_config->sensor_name = evas_stringshare_add("temp1");
	temperature_config->units = CELCIUS;
     }
   E_CONFIG_LIMIT(temperature_config->poll_time, 0.5, 1000.0);
   E_CONFIG_LIMIT(temperature_config->low, 0, 100);
   E_CONFIG_LIMIT(temperature_config->high, 0, 220);
   E_CONFIG_LIMIT(temperature_config->units, CELCIUS, FAHRENHEIT);

   temperature_config->have_temp = -1;
   temperature_config->temperature_check_timer = 
     ecore_timer_add(temperature_config->poll_time, _temperature_cb_check, 
		     NULL);
   
   temperature_config->module = m;
   
   e_gadcon_provider_register(&_gadcon_class);
   return 1;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   e_gadcon_provider_unregister(&_gadcon_class);
   
   if (temperature_config->config_dialog) 
     e_object_del(E_OBJECT(temperature_config->config_dialog));
   if (temperature_config->menu)
     {
	e_menu_post_deactivate_callback_set(temperature_config->menu , NULL, NULL);
	e_object_del(E_OBJECT(temperature_config->menu));
	temperature_config->menu = NULL;
     }
   if (temperature_config->temperature_check_timer)
     ecore_timer_del(temperature_config->temperature_check_timer);
   if (temperature_config->sensor_name)
     evas_stringshare_del(temperature_config->sensor_name);
   free(temperature_config);
   temperature_config = NULL;
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
   e_module_dialog_show(_("Enlightenment Temperature Module"),
			_("A module to measure the <hilight>ACPI Thermal sensor</hilight> on Linux.<br>"
			  "It is especially useful for modern Laptops with high speed<br>"
			  "CPUs that generate a lot of heat."));
   return 1;
}

EAPI int
e_modapi_config(E_Module *m)
{
   if (!temperature_config) return 0;
   if (temperature_config->config_dialog) return 0;
   _config_temperature_module();
   return 1;
}
/**/
/***************************************************************************/
