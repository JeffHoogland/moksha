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
   int	allow_overlap;
};

struct _Config_Face
{
   unsigned char enabled;
};

struct _Temperature
{
   Config           *conf;
   Ecore_Timer      *temperature_check_timer;
   E_Config_Dialog  *config_dialog;
   unsigned char    have_temp;
   E_Gadget	    *gad;
};

struct _Temperature_Face
{
   Temperature *temp;
   Config_Face *conf;

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
