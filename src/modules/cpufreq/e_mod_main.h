#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#include <e.h>
#include <Evas.h>

typedef struct _Status       Status;
typedef struct _Config       Config;
typedef struct _Config_Face  Config_Face;
typedef struct _Cpufreq      Cpufreq;
typedef struct _Cpufreq_Face Cpufreq_Face;

struct _Status
{
   Evas_List   *frequencies;
   Evas_List   *governors;
   int          cur_frequency;
   int          can_set_frequency;
   char        *cur_governor;
   unsigned char active;
};

struct _Config
{
   double poll_time;
   Evas_List *faces;

   int   restore_governor;
   char *governor;
};

struct _Config_Face
{
   unsigned char enabled;
};

struct _Cpufreq
{
   E_Menu    *config_menu;
   E_Menu    *config_menu_poll;
   E_Menu    *menu_governor;
   E_Menu    *menu_frequency;
   Evas_List *faces;

   Config    *conf;
   Status    *status;
   
   char      *set_exe_path;
   
   Ecore_Timer *frequency_check_timer;   
};

struct _Cpufreq_Face
{
   E_Container *con;
   
   E_Menu      *menu;
   Config_Face *conf;
   Cpufreq     *owner;
   
   Evas_Object *freq_object;
   Evas_Object *event_object;
   
   E_Gadman_Client *gmc;
};

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init     (E_Module *module);
EAPI int   e_modapi_shutdown (E_Module *module);
EAPI int   e_modapi_save     (E_Module *module);
EAPI int   e_modapi_info     (E_Module *module);
EAPI int   e_modapi_about    (E_Module *module);
EAPI int   e_modapi_config   (E_Module *module);

#endif
