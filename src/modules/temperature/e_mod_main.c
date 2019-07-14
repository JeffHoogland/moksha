#include "e.h"
#include "e_mod_main.h"

#if defined(__FreeBSD__) || defined(__DragonFly__)
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#ifdef __OpenBSD__
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/sensors.h>
#endif


/* gadcon requirements */
static E_Gadcon_Client *_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style);
static void _gc_shutdown(E_Gadcon_Client *gcc);
static void _gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient);
static const char *_gc_label(const E_Gadcon_Client_Class *client_class);
static Evas_Object *_gc_icon(const E_Gadcon_Client_Class *client_class, Evas *evas);
static const char *_gc_id_new(const E_Gadcon_Client_Class *client_class);
/* and actually define the gadcon class that this module provides (just 1) */
static const E_Gadcon_Client_Class _gadcon_class =
{
   GADCON_CLIENT_CLASS_VERSION, "temperature",
     {
        _gc_init, _gc_shutdown, _gc_orient, _gc_label, _gc_icon, _gc_id_new, NULL,
        e_gadcon_site_is_not_toolbar
     },
   E_GADCON_CLIENT_STYLE_PLAIN
};

/* actual module specifics */

static void _temperature_face_cb_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info);

static void _temperature_face_cb_menu_configure(void *data, E_Menu *m, E_Menu_Item *mi);

static Eina_Bool _temperature_face_shutdown(const Eina_Hash *hash __UNUSED__, const void *key __UNUSED__, void *hdata, void *fdata __UNUSED__);
static Eina_Bool _temperature_face_id_max(const Eina_Hash *hash __UNUSED__, const void *key, void *hdata __UNUSED__, void *fdata);

static E_Config_DD *conf_edd = NULL;
static E_Config_DD *conf_face_edd = NULL;

static int uuid = 0;

static Config *temperature_config = NULL;

static void
_temperature_thread_free(Tempthread *tth)
{
#if defined(HAVE_EEZE)
   const char *s;
#endif
   eina_stringshare_del(tth->sensor_name);
   eina_stringshare_del(tth->sensor_path);
#if defined(HAVE_EEZE)
   EINA_LIST_FREE(tth->tempdevs, s) eina_stringshare_del(s);
#endif
   e_powersave_sleeper_free(tth->sleeper);
   free(tth->extn);
   free(tth);
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
_temperature_apply(Config_Face *inst, int temp)
{
   char buf[64];

   if (inst->temp == temp) return;
   inst->temp = temp;
   if (temp != -999)
     {
        if (inst->units == FAHRENHEIT) temp = (temp * 9.0 / 5.0) + 32;

        if (!inst->have_temp)
          {
             /* enable therm object */
             edje_object_signal_emit(inst->o_temp, "e,state,known", "");
             inst->have_temp = EINA_TRUE;
          }
        if (inst->units == FAHRENHEIT)
          snprintf(buf, sizeof(buf), "%i°F", temp);
        else
          snprintf(buf, sizeof(buf), "%i°C", temp);

        _temperature_face_level_set(inst,
                                    (double)(temp - inst->low) /
                                    (double)(inst->high - inst->low));
        edje_object_part_text_set(inst->o_temp, "e.text.reading", buf);
     }
   else
     {
        if (inst->have_temp)
          {
             /* disable therm object */
             edje_object_signal_emit(inst->o_temp, "e,state,unknown", "");
             edje_object_part_text_set(inst->o_temp, "e.text.reading", "N/A");
             _temperature_face_level_set(inst, 0.5);
             inst->have_temp = EINA_FALSE;
          }
     }
}

#ifdef HAVE_EEZE
static Eina_Bool
_temperature_udev_poll(void *data)
{
   Tempthread *tth = data;
   int temp = temperature_udev_get(tth);

   _temperature_apply(tth->inst, temp);
   return EINA_TRUE;
}
#endif

static E_Gadcon_Client *
_gc_init(E_Gadcon *gc, const char *name, const char *id, const char *style)
{
   Evas_Object *o;
   E_Gadcon_Client *gcc;
   Config_Face *inst;

   inst = eina_hash_find(temperature_config->faces, id);
   if (!inst)
     {
        inst = E_NEW(Config_Face, 1);
        inst->id = eina_stringshare_add(id);
        inst->poll_interval = 128;
        inst->low = 30;
        inst->high = 80;
        inst->sensor_type = SENSOR_TYPE_NONE;
        inst->sensor_name = NULL;
        inst->units = CELSIUS;
#ifdef HAVE_EEZE
        inst->backend = UDEV;
#endif
        if (!temperature_config->faces)
          temperature_config->faces = eina_hash_string_superfast_new(NULL);
        eina_hash_direct_add(temperature_config->faces, inst->id, inst);
     }
   if (!inst->id) inst->id = eina_stringshare_add(id);
   E_CONFIG_LIMIT(inst->poll_interval, 1, 1024);
   E_CONFIG_LIMIT(inst->low, 0, 100);
   E_CONFIG_LIMIT(inst->high, 0, 220);
   E_CONFIG_LIMIT(inst->units, CELSIUS, FAHRENHEIT);
#ifdef HAVE_EEZE
   E_CONFIG_LIMIT(inst->backend, TEMPGET, UDEV);
#endif

   o = edje_object_add(gc->evas);
   e_theme_edje_object_set(o, "base/theme/modules/temperature",
                           "e/modules/temperature/main");
   gcc = e_gadcon_client_new(gc, name, id, style, o);
   gcc->data = inst;

   inst->gcc = gcc;
   inst->o_temp = o;
   inst->module = temperature_config->module;
   inst->have_temp = EINA_FALSE;
   
#ifdef HAVE_EEZE
   eeze_init();
#endif

   temperature_face_update_config(inst);

   evas_object_event_callback_add(o, EVAS_CALLBACK_MOUSE_DOWN,
				  _temperature_face_cb_mouse_down, inst);
   return gcc;
}

static void
_gc_shutdown(E_Gadcon_Client *gcc)
{
   Config_Face *inst;

   inst = gcc->data;

#ifdef HAVE_EEZE
    eeze_shutdown();
 #endif
   if (inst->o_temp) evas_object_del(inst->o_temp);
   inst->o_temp = NULL;
   if (inst->config_dialog) e_object_del(E_OBJECT(inst->config_dialog));
   inst->config_dialog = NULL;
}

static void
_gc_orient(E_Gadcon_Client *gcc, E_Gadcon_Orient orient __UNUSED__)
{
   e_gadcon_client_aspect_set(gcc, 16, 16);
   e_gadcon_client_min_size_set(gcc, 16, 16);
}

static const char *
_gc_label(const E_Gadcon_Client_Class *client_class __UNUSED__)
{
   return _("Temperature");
}

static Evas_Object *
_gc_icon(const E_Gadcon_Client_Class *client_class __UNUSED__, Evas *evas)
{
   Evas_Object *o;
   char buf[PATH_MAX];

   o = edje_object_add(evas);
   snprintf(buf, sizeof(buf), "%s/e-module-temperature.edj",
            e_module_dir_get(temperature_config->module));
   edje_object_file_set(o, buf, "icon");
   return o;
}

static const char *
_gc_id_new(const E_Gadcon_Client_Class *client_class __UNUSED__)
{
   Config_Face *inst;
   char id[128];

   snprintf(id, sizeof(id), "%s.%d", _gadcon_class.name, ++uuid);

   inst = E_NEW(Config_Face, 1);
   inst->id = eina_stringshare_add(id);
   inst->poll_interval = 128;
   inst->low = 30;
   inst->high = 80;
   inst->sensor_type = SENSOR_TYPE_NONE;
   inst->sensor_name = NULL;
   inst->units = CELSIUS;
#ifdef HAVE_EEZE
   inst->backend = TEMPGET;
#endif
   if (!temperature_config->faces)
     temperature_config->faces = eina_hash_string_superfast_new(NULL);
   eina_hash_direct_add(temperature_config->faces, inst->id, inst);
   return inst->id;
}

static void
_temperature_face_cb_mouse_down(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Config_Face *inst;
   Evas_Event_Mouse_Down *ev;

   inst = data;
   ev = event_info;
   if (ev->button == 3)
     {
        E_Menu *m;
        E_Menu_Item *mi;
        int cx, cy;

        m = e_menu_new();
        mi = e_menu_item_new(m);
        e_menu_item_label_set(mi, _("Settings"));
        e_util_menu_item_theme_icon_set(mi, "configure");
        e_menu_item_callback_set(mi, _temperature_face_cb_menu_configure, inst);

        m = e_gadcon_client_util_menu_items_append(inst->gcc, m, 0);

        e_gadcon_canvas_zone_geometry_get(inst->gcc->gadcon, &cx, &cy, NULL, NULL);
        e_menu_activate_mouse(m, e_util_zone_current_get(e_manager_current_get()),
                              cx + ev->output.x, cy + ev->output.y, 1, 1,
                              E_MENU_POP_DIRECTION_AUTO, ev->timestamp);
        evas_event_feed_mouse_up(inst->gcc->gadcon->evas, ev->button,
                                 EVAS_BUTTON_NONE, ev->timestamp, NULL);
     }
}

static void
_temperature_face_cb_menu_configure(void *data, E_Menu *m __UNUSED__, E_Menu_Item *mi __UNUSED__)
{
   Config_Face *inst;

   inst = data;
   if (inst->config_dialog) return;
   config_temperature_module(inst);
}

static Eina_Bool
_temperature_face_shutdown(const Eina_Hash *hash EINA_UNUSED, const void *key EINA_UNUSED, void *hdata, void *fdata EINA_UNUSED)
{
   Config_Face *inst;

   inst = hdata;
   if (inst->th) ecore_thread_cancel(inst->th);
   if (inst->sensor_name) eina_stringshare_del(inst->sensor_name);
   if (inst->id) eina_stringshare_del(inst->id);
#ifdef HAVE_EEZE
   if (inst->poller)
     {
        ecore_poller_del(inst->poller);
        _temperature_thread_free(inst->tth);
     }
#endif
   E_FREE(inst);
   return EINA_TRUE;
}


static Eina_Bool
_temperature_face_id_max(const Eina_Hash *hash __UNUSED__, const void *key, void *hdata __UNUSED__, void *fdata)
{
   const char *p;
   int *max;
   int num = -1;

   max = fdata;
   p = strrchr(key, '.');
   if (p) num = atoi(p + 1);
   if (num > *max) *max = num;
   return EINA_TRUE;
}

static void
_temperature_check_main(void *data, Ecore_Thread *th)
{
   Tempthread *tth = data;
   int ptemp = -500, temp;

   for (;;)
     {
        if (ecore_thread_check(th)) break;
        temp = temperature_tempget_get(tth);
        if (ptemp != temp) ecore_thread_feedback(th, (void *)((long)temp));
        ptemp = temp;
        e_powersave_sleeper_sleep(tth->sleeper, tth->poll_interval);
        if (ecore_thread_check(th)) break;
     }
}

static void
_temperature_check_notify(void *data, Ecore_Thread *th, void *msg)
{
   Tempthread *tth  = data;
   Config_Face *inst = tth->inst;
   int temp = (int)((long)msg);

   if (th != inst->th) return;
   _temperature_apply(inst, temp);
}

static void
_temperature_check_done(void *data, Ecore_Thread *th EINA_UNUSED)
{
   _temperature_thread_free(data);
}

void
temperature_face_update_config(Config_Face *inst)
{
   Tempthread *tth;

   if (inst->th) ecore_thread_cancel(inst->th);

   tth = calloc(1, sizeof(Tempthread));
   tth->poll_interval = inst->poll_interval;
   tth->sensor_type = inst->sensor_type;
   tth->inst = inst;
   tth->sleeper = e_powersave_sleeper_new();
   if (inst->sensor_name)
     tth->sensor_name = eina_stringshare_add(inst->sensor_name);

#ifdef HAVE_EEZE
   if (inst->backend != TEMPGET)
     {
        inst->poller = ecore_poller_add(ECORE_POLLER_CORE, inst->poll_interval,
                                        _temperature_udev_poll, tth);
        inst->tth = tth;
     }
   else
#endif
     inst->th = ecore_thread_feedback_run(_temperature_check_main,
                                          _temperature_check_notify,
                                          _temperature_check_done,
                                          _temperature_check_done,
                                          tth, EINA_TRUE);
}

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
   E_CONFIG_VAL(D, T, id, STR);
   E_CONFIG_VAL(D, T, poll_interval, INT);
   E_CONFIG_VAL(D, T, low, INT);
   E_CONFIG_VAL(D, T, high, INT);
   E_CONFIG_VAL(D, T, sensor_type, INT);
#ifdef HAVE_EEZE
   E_CONFIG_VAL(D, T, backend, INT);
#endif
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
     temperature_config = E_NEW(Config, 1);
   else
     eina_hash_foreach(temperature_config->faces, _temperature_face_id_max, &uuid);
   temperature_config->module = m;

   e_gadcon_provider_register(&_gadcon_class);
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m __UNUSED__)
{
   e_gadcon_provider_unregister(&_gadcon_class);
   eina_hash_foreach(temperature_config->faces, _temperature_face_shutdown, NULL);
   eina_hash_free(temperature_config->faces);
   free(temperature_config);
   temperature_config = NULL;
   E_CONFIG_DD_FREE(conf_face_edd);
   E_CONFIG_DD_FREE(conf_edd);
   return 1;
}

EAPI int
e_modapi_save(E_Module *m __UNUSED__)
{
   e_config_domain_save("module.temperature", conf_edd, temperature_config);
   return 1;
}
