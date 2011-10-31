#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#ifdef HAVE_EEZE
# include <Eeze.h>
#else
# include <E_Hal.h>
#endif

#include "e_mod_main_private.h"

typedef struct _Config Config;
typedef struct _Config_Face Config_Face;

typedef enum _Unit
{
   CELCIUS,
   FAHRENHEIT
} Unit;

struct _Config_Face
{
   const char *id;
   /* saved * loaded config values */
   int poll_interval;
   int low, high;
#ifdef HAVE_EEZE
   Eina_List *tempdevs;
   int backend;
   Ecore_Poller *temp_poller;
#endif
   int sensor_type;
   const char *sensor_name;
   Unit units;
   /* config state */
   E_Gadcon_Client *gcc;
   Evas_Object *o_temp;

   E_Module *module;

   E_Config_Dialog *config_dialog;
   E_Menu *menu;
   Ecore_Exe *tempget_exe;
   Ecore_Event_Handler *tempget_data_handler;
   Ecore_Event_Handler *tempget_del_handler;

   Eina_Bool have_temp:1;
#if defined (__FreeBSD__) || defined (__OpenBSD__)
   int mib[5];
#endif
};

struct _Config
{
   /* saved * loaded config values */
   Eina_Hash *faces;
   /* config state */
   E_Module *module;
};

#ifdef HAVE_EEZE
typedef enum _Backend
{
   TEMPGET,
   UDEV
} Backend;

Eina_Bool temperature_udev_update_poll(void *data);
void temperature_udev_update(void *data);
#endif

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init(E_Module *m);
EAPI int e_modapi_shutdown(E_Module *m);
EAPI int e_modapi_save(E_Module *m);

Eina_Bool _temperature_cb_exe_data(void *data, int type, void *event);
Eina_Bool _temperature_cb_exe_del(void *data, int type, void *event);
void _temperature_face_level_set(Config_Face *inst, double level);
void config_temperature_module(Config_Face *inst);
void temperature_face_update_config(Config_Face *inst);
Eina_List *temperature_get_bus_files(const char* bus);

#endif
