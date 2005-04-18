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
};

struct _Config
{
   double poll_time;
   Evas_List *faces;
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
   
   char *set_exe_path;
   
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

EAPI void *init     (E_Module *module);
EAPI int   shutdown (E_Module *module);
EAPI int   save     (E_Module *module);
EAPI int   info     (E_Module *module);
EAPI int   about    (E_Module *module);

#endif
