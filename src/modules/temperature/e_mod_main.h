/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

typedef struct _Config Config;
typedef struct _Config_Face Config_Face;

typedef enum _Unit
{
   CELCIUS,
   FAHRENHEIT
} Unit;

typedef enum _Sensor_Type
{
   SENSOR_TYPE_NONE,
   SENSOR_TYPE_FREEBSD,
   SENSOR_TYPE_OMNIBOOK,
   SENSOR_TYPE_LINUX_MACMINI,
   SENSOR_TYPE_LINUX_I2C,
   SENSOR_TYPE_LINUX_ACPI,
   SENSOR_TYPE_LINUX_PCI,
   SENSOR_TYPE_LINUX_PBOOK,
   SENSOR_TYPE_LINUX_INTELCORETEMP
} Sensor_Type;

struct _Config_Face
{
   const char *id;
   /* saved * loaded config values */
   int poll_interval;
   int low, high;
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

//   Ecore_Poller    *temperature_check_poller;
   unsigned char have_temp;
#ifdef __FreeBSD__
   int mib[5];
#endif
};

struct _Config
{
   /* saved * loaded config values */
   Evas_Hash *faces;
   /* config state */
   E_Module *module;
};

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init (E_Module *m);
EAPI int e_modapi_shutdown (E_Module *m);
EAPI int e_modapi_save (E_Module *m);

void config_temperature_module(Config_Face *inst);
void temperature_face_update_config(Config_Face *inst);
Ecore_List *temperature_get_bus_files(const char* bus);

#endif
