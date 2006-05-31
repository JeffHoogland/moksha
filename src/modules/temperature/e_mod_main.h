/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

typedef struct _Config           Config;
	
typedef enum _Unit
{
   CELCIUS,
   FAHRENHEIT
} Unit;

struct _Config
{
   /* saved * loaded config values */
   double           poll_time;
   int              low, high;
   const char      *sensor_name;
   Unit             units;
   /* just config state */
   E_Module        *module;
   E_Config_Dialog *config_dialog;
   Evas_List       *instances;
   E_Menu          *menu;
   Ecore_Timer     *temperature_check_timer;
   unsigned char    have_temp;
};

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init     (E_Module *m);
EAPI int   e_modapi_shutdown (E_Module *m);
EAPI int   e_modapi_save     (E_Module *m);
EAPI int   e_modapi_info     (E_Module *m);
EAPI int   e_modapi_about    (E_Module *m);
EAPI int   e_modapi_config   (E_Module *m);

void _config_temperature_module(void);
void _temperature_face_cb_config_updated(void);
extern Config *temperature_config;


#endif
