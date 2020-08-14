#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#include "e.h"

#ifdef HAVE_EEZE
# include <Eeze.h>
#else
# include <E_Hal.h>
#endif


typedef enum _Sensor_Type
{
   SENSOR_TYPE_NONE,
   SENSOR_TYPE_FREEBSD,
   SENSOR_TYPE_OPENBSD,
   SENSOR_TYPE_OMNIBOOK,
   SENSOR_TYPE_LINUX_MACMINI,
   SENSOR_TYPE_LINUX_I2C,
   SENSOR_TYPE_LINUX_ACPI,
   SENSOR_TYPE_LINUX_PCI,
   SENSOR_TYPE_LINUX_PBOOK,
   SENSOR_TYPE_LINUX_INTELCORETEMP,
   SENSOR_TYPE_LINUX_THINKPAD,
   SENSOR_TYPE_LINUX_SYS
} Sensor_Type;

typedef struct _Config Config;
typedef struct _Config_Face Config_Face;
typedef struct _Tempthread Tempthread;

typedef enum _Unit
{
   CELSIUS,
   FAHRENHEIT
} Unit;

struct _Tempthread
{
   Config_Face *inst;
   int poll_interval;
   Sensor_Type sensor_type;
   const char *sensor_name;
   const char *sensor_path;
   void *extn;
   E_Powersave_Sleeper *sleeper;
#ifdef HAVE_EEZE
   Eina_List *tempdevs;
#endif
   Eina_Bool initted :1;
};

struct _Config_Face
{
   const char *id;
   /* saved * loaded config values */
   int poll_interval;
   int low, high;
   int show_alert;
   int sensor_type;
   int temp;
   const char *sensor_name;
   Unit units;
   /* config state */
   E_Gadcon_Client *gcc;
   Evas_Object *o_temp;
#ifdef HAVE_EEZE
   Ecore_Poller *poller;
   Tempthread *tth;
   int backend;
#endif
   E_Module *module;

   E_Config_Dialog *config_dialog;
   E_Menu *menu;
   Ecore_Thread *th;

   Eina_Bool have_temp :1;
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

int temperature_udev_get(Tempthread *tth);
#endif

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init(E_Module *m);
EAPI int e_modapi_shutdown(E_Module *m);
EAPI int e_modapi_save(E_Module *m);

void config_temperature_module(Config_Face *inst);
void temperature_face_update_config(Config_Face *inst);

int temperature_tempget_get(Tempthread *tth);

/**
 * @addtogroup Optional_Gadgets
 * @{
 *
 * @defgroup Module_Temperature Temperature
 *
 * Monitors computer temperature sensors and may do actions given some
 * thresholds.
 *
 * @see Module_CPUFreq
 * @}
 */
#endif
