#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

typedef struct _Config       Config;
typedef struct _Battery      Battery;
typedef struct _Battery_Face Battery_Face;

#define CHECK_NONE 0
#define CHECK_LINUX_ACPI 1
#define CHECK_LINUX_APM 2

struct _Config
{
   double poll_time;
   int alarm;
};

struct _Battery
{
   E_Menu       *config_menu; 
   E_Menu       *config_menu_alarm;
   E_Menu       *config_menu_poll;
   Battery_Face *face;
   
   E_Config_DD *conf_edd;
   Config      *conf;
   int alarm_triggered;
};

struct _Battery_Face
{
   Battery     *bat;
   E_Container *con;
   Evas        *evas;
   
   Evas_Object *bat_object;
   Evas_Object *event_object;
   
   int                  battery_check_mode;
   Ecore_Timer         *battery_check_timer;
   int                  battery_prev_drain;
   int                  battery_prev_ac;
   int                  battery_prev_battery;
   
   E_Gadman_Client *gmc;
};

EAPI void *init     (E_Module *m);
EAPI int   shutdown (E_Module *m);
EAPI int   save     (E_Module *m);
EAPI int   info     (E_Module *m);
EAPI int   about    (E_Module *m);

#endif
