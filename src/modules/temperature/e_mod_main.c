/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"

#ifdef __FreeBSD__
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

/* TODO List:
 *
 * which options should be in main menu, and which in face menu?
 */

/* module private routines */
static Temperature *_temperature_new();
static void      _temperature_free(Temperature *e);
static int       _temperature_cb_check(void *data);

static void      _temperature_face_init(void *data, E_Gadget_Face *face);
static void      _temperature_face_free(void *data, E_Gadget_Face *face);

static void      _temperature_face_level_set(E_Gadget_Face *face, double level);

static E_Config_DD *conf_edd;
static E_Config_DD *conf_face_edd;

/* public module routines. all modules must have these */
EAPI E_Module_Api e_modapi = 
{
   E_MODULE_API_VERSION,
   "Temperature"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   E_Gadget *gad = NULL;
   E_Gadget_Api *api;
   Temperature *e;

   e = _temperature_new();

   /* set up our actual gadget */
   api = E_NEW(E_Gadget_Api, 1);
   api->module = m;
   api->name = "temperature";
   api->func_face_init = _temperature_face_init;
   api->func_face_free = _temperature_face_free;
   api->data = e;

   gad = e_gadget_new(api);
   E_FREE(api);

   /* start the timer */
   e->gad = gad;
   e->temperature_check_timer = ecore_timer_add(e->conf->poll_time, _temperature_cb_check, gad);
   return gad;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   E_Gadget *gad;
   Temperature *e;

   E_CONFIG_DD_FREE(conf_edd);
   E_CONFIG_DD_FREE(conf_face_edd);

   gad = m->data;
   if (!gad) return 0;
   e = gad->data;
   if (e)
     {
	if (e->config_dialog) 
	  {
	     e_object_del(E_OBJECT(e->config_dialog));
	     e->config_dialog = NULL;
	  }
	_temperature_free(e);
     }
   e_object_del(E_OBJECT(gad));
   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   E_Gadget *gad;
   Temperature *e;

   gad = m->data;
   if (!gad) return 0;
   e = gad->data;
   if (!e) return 0;
   e_config_domain_save("module.temperature", conf_edd, e->conf);
   return 1;
}

EAPI int
e_modapi_info(E_Module *m)
{
   char buf[4096];

   snprintf(buf, sizeof(buf), "%s/module_icon.png", e_module_dir_get(m));
   m->icon_file = strdup(buf);
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
   E_Gadget *gad;
   Temperature *e;
   
   gad = m->data;
   if (!gad) return 0;
   e = gad->data; 
   _config_temperature_module(e_container_current_get(e_manager_current_get()), e);
     
   return 1;
}



/* module private routines */
static Temperature *
_temperature_new()
{
   Temperature *e;

   e = E_NEW(Temperature, 1);
   if (!e) return NULL;

   /* create the config edd */
   conf_face_edd = E_CONFIG_DD_NEW("Temperature_Config_Face", Config_Face);
#undef T
#undef D
#define T Config_Face
#define D conf_face_edd
   E_CONFIG_VAL(D, T, enabled, UCHAR);

   conf_edd = E_CONFIG_DD_NEW("Temperature_Config", Config);
#undef T
#undef D
#define T Config
#define D conf_edd
   E_CONFIG_VAL(D, T, poll_time, DOUBLE);
   E_CONFIG_VAL(D, T, low, INT);
   E_CONFIG_VAL(D, T, high, INT);
   E_CONFIG_LIST(D, T, faces, conf_face_edd);
   E_CONFIG_VAL(D, T, sensor_name, STR);
   E_CONFIG_VAL(D, T, units, INT);

   e->conf = e_config_domain_load("module.temperature", conf_edd);
   if (!e->conf)
     {
	e->conf = E_NEW(Config, 1);
	e->conf->poll_time = 10.0;
	e->conf->low = 30;
	e->conf->high = 80;
	e->conf->sensor_name = "temp1";
	e->conf->units = CELCIUS;
     }
   E_CONFIG_LIMIT(e->conf->poll_time, 0.5, 1000.0);
   E_CONFIG_LIMIT(e->conf->low, 0, 100);
   E_CONFIG_LIMIT(e->conf->high, 0, 220);
   E_CONFIG_LIMIT(e->conf->units, CELCIUS, FAHRENHEIT);

   e->have_temp = -1;

   return e;
}

void _temperature_face_init(void *data, E_Gadget_Face *face)
{
   Temperature *e;
   Temperature_Face *ef;
   E_Gadman_Policy  policy;

   e = data;

   ef = E_NEW(Temperature_Face, 1);
   if (!ef) return;

   e_gadget_face_theme_set(face, "base/theme/modules/temperature",
			   "modules/temperature/main");

   policy = E_GADMAN_POLICY_ANYWHERE |
	    E_GADMAN_POLICY_HMOVE |
	    E_GADMAN_POLICY_VMOVE |
	    E_GADMAN_POLICY_HSIZE |
	    E_GADMAN_POLICY_VSIZE;

   e_gadman_client_policy_set(face->gmc, policy);

   ef->conf = evas_list_nth(e->conf->faces, face->face_num);
   if (!ef->conf)
     {
	ef->conf = E_NEW(Config_Face, 1);
	ef->conf->enabled = 1;
	e->conf->faces = evas_list_append(e->conf->faces, ef->conf);
     }

   face->data = ef;

   _temperature_cb_check(face->gad);

   return;
}

static void
_temperature_free(Temperature *e)
{
   Evas_List *l;

   ecore_timer_del(e->temperature_check_timer);

   for (l = e->conf->faces; l; l = l->next)
     free(l->data);
   evas_list_free(e->conf->faces);
   free(e->conf);
   free(e);
}

static void
_temperature_face_free(void *data, E_Gadget_Face *face)
{
   Temperature_Face *ef;
   ef = face->data;

   free(ef);
}

static int
_temperature_cb_check(void *data)
{
   E_Gadget *gad;
   E_Gadget_Face *face;
   Temperature *t;
   int ret = 0;
   Ecore_List *therms;
   Evas_List *l;
   int temp = 0;
   char buf[4096];
#ifdef __FreeBSD__
   static int mib[5] = {-1};
   int len;
#endif

   gad = data;
   if (!gad) return 0;
   t = gad->data;
   if (!t) return 0;

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
		  char *name, *sensor;

		  sensor = t->conf->sensor_name;
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
   
   if (t->conf->units == FAHRENHEIT)
	temp = (temp * 9.0 / 5.0) + 32;

   if (ret)
     {
	char *utf8;

	if (t->have_temp != 1)
	  {
	     /* enable therm object */
	     for (l = gad->faces; l; l = l->next)
	       {
		  face = l->data;
		  edje_object_signal_emit(face->main_obj, "known", "");
	       }
	     t->have_temp = 1;
	  }

	if (t->conf->units == FAHRENHEIT) 
	  snprintf(buf, sizeof(buf), "%i°F", temp);
	else
	  snprintf(buf, sizeof(buf), "%i°C", temp);               
	utf8 = ecore_txt_convert("iso-8859-1", "utf-8", buf);

	for (l = gad->faces; l; l = l->next)
	  {
	     face = l->data;
	     _temperature_face_level_set(face,
				    (double)(temp - t->conf->low) /
				    (double)(t->conf->high - t->conf->low));
		  
	     edje_object_part_text_set(face->main_obj, "reading", utf8);
	  }
	free(utf8);
     }
   else
     {
	if (t->have_temp != 0)
	  {
	     /* disable therm object */
	     for (l = gad->faces; l; l = l->next)

	       {
		  face = l->data;
		  edje_object_signal_emit(face->main_obj, "unknown", "");
		  edje_object_part_text_set(face->main_obj, "reading", "NO TEMP");
		  _temperature_face_level_set(face, 0.5);
	       }
	     t->have_temp = 0;
	  }
     }
   return 1;
}

static void
_temperature_face_level_set(E_Gadget_Face *face, double level)
{
   Edje_Message_Float msg;

   if (level < 0.0) level = 0.0;
   else if (level > 1.0) level = 1.0;
   msg.val = level;
   edje_object_message_send(face->main_obj, EDJE_MESSAGE_FLOAT, 1, &msg);
}

void 
_temperature_face_cb_config_updated(Temperature *temp) 
{
   /* Call all funcs needed to handle update */
   ecore_timer_del(temp->temperature_check_timer);
   temp->temperature_check_timer = ecore_timer_add(temp->conf->poll_time, _temperature_cb_check, temp->gad);
   
}

