/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

typedef struct _Config       Config;
typedef struct _Config_Face  Config_Face;
typedef struct _Battery      Battery;
typedef struct _Battery_Face Battery_Face;

typedef struct _Status       Status;

#define CHECK_NONE 0
#define CHECK_ACPI 1
#define CHECK_APM  2
#define CHECK_PMU  3

struct _Config
{
   double     poll_time;
   int        alarm;
   Evas_List *faces;
};

struct _Config_Face
{
   unsigned char enabled;
};

struct _Battery
{
   E_Menu       *config_menu;
   Evas_List    *faces;

   Config       *conf;
   int           alarm_triggered;

   int           battery_check_mode;
   Ecore_Timer  *battery_check_timer;
   int           battery_prev_drain;
   int           battery_prev_ac;
   int           battery_prev_battery;
   E_Config_Dialog *config_dialog;
};

struct _Battery_Face
{
   Battery *battery;
   E_Container *con;

   E_Menu      *menu;
   Config_Face *conf;

   Evas_Object *bat_object;
   Evas_Object *event_object;

   E_Gadman_Client *gmc;
};

#ifdef __FreeBSD__
#define BATTERY_STATE_NONE 0
#define BATTERY_STATE_DISCHARGING 1
#define BATTERY_STATE_CHARGING 2
#define BATTERY_STATE_REMOVED 7
#else
#define BATTERY_STATE_NONE 0
#define BATTERY_STATE_CHARGING 1
#define BATTERY_STATE_DISCHARGING 2
#endif

struct _Status
{
   /* Low battery */
   unsigned char alarm;
   /* Is there a battery? */
   unsigned char has_battery;
   /* charging, discharging, none */
   unsigned char state;
   /* Battery level */
   double level;
   /* Text */
   /* reading == % left */
   char *reading;
   /* time == time left to empty / full */
   char *time;
};

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init     (E_Module *m);
EAPI int   e_modapi_shutdown (E_Module *m);
EAPI int   e_modapi_save     (E_Module *m);
EAPI int   e_modapi_info     (E_Module *m);
EAPI int   e_modapi_about    (E_Module *m);
EAPI int   e_modapi_config   (E_Module *m);

void	_battery_face_cb_config_updated(Battery *bat);

#endif
