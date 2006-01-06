/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

typedef struct _Config           Config;
typedef struct _Config_Face      Config_Face;
typedef struct _Temperature      Temperature;
typedef struct _Temperature_Face Temperature_Face;
	
typedef enum _Unit
{
   CELCIUS,
   FAHRENHEIT
} Unit;

struct _Config
{
   double poll_time;
   int low, high;
   Evas_List *faces;
   char *sensor_name;
   Unit units;
};

struct _Config_Face
{
   unsigned char enabled;
};

struct _Temperature
{
   E_Menu           *config_menu;
   E_Menu           *config_menu_low;
   E_Menu           *config_menu_high;
   E_Menu           *config_menu_poll;
   E_Menu           *config_menu_sensor;
   E_Menu           *config_menu_unit;
   Evas_List        *faces;

   Config           *conf;
   Ecore_Timer      *temperature_check_timer;
   E_Config_Dialog  *config_dialog;
   unsigned char    have_temp;
};

struct _Temperature_Face
{
   Temperature *temp;
   E_Container *con;
   E_Menu      *menu;
   Config_Face *conf;

   Evas_Object *temp_object;
   Evas_Object *event_object;

   E_Gadman_Client *gmc;
};

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init     (E_Module *m);
EAPI int   e_modapi_shutdown (E_Module *m);
EAPI int   e_modapi_save     (E_Module *m);
EAPI int   e_modapi_info     (E_Module *m);
EAPI int   e_modapi_about    (E_Module *m);
EAPI int   e_modapi_config   (E_Module *m);

void	_temperature_face_cb_config_updated(Temperature *temp);

#endif
