#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

typedef struct _Cpu_Status       Cpu_Status;
typedef struct _Config       Config;

#define CPUFREQ_CONFIG_VERSION 1

struct _Cpu_Status
{
   Eina_List     *frequencies;
   Eina_List     *governors;
   int            cur_frequency;
#ifdef __OpenBSD__
   int            cur_percent;
#endif
   int            cur_min_frequency;
   int            cur_max_frequency;
   int            can_set_frequency;
   int            pstate_min;
   int            pstate_max;
   char          *cur_governor;
   const char    *orig_governor;
   unsigned char  active;
   unsigned char  pstate;
   unsigned char  pstate_turbo;
};

struct _Config
{
   /* saved * loaded config values */
   int           config_version;
   int           poll_interval;
   int           restore_governor;
   int           auto_powersave;
   const char   *powersave_governor;
   const char   *governor;
   int           pstate_min;
   int           pstate_max;
   /* just config state */
   E_Module     *module;
   Eina_List    *instances;
   E_Menu       *menu;
   E_Menu       *menu_poll;
   E_Menu       *menu_governor;
   E_Menu       *menu_frequency;
   E_Menu       *menu_powersave;
   E_Menu       *menu_pstate1;
   E_Menu       *menu_pstate2;
   Cpu_Status   *status;
   char         *set_exe_path;
   Ecore_Thread *frequency_check_thread;
   Ecore_Event_Handler *handler;
   E_Config_Dialog *config_dialog;
};

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init     (E_Module *m);
EAPI int   e_modapi_shutdown (E_Module *m);
EAPI int   e_modapi_save     (E_Module *m);

E_Config_Dialog *e_int_config_cpufreq_module(E_Container *parent, const char *params);
void _cpufreq_poll_interval_update(void);
void _cpufreq_set_governor(const char *governor);
void _cpufreq_set_pstate(int min, int max);

extern Config *cpufreq_config;



/**
 * @addtogroup Optional_Gadgets
 * @{
 *
 * @defgroup Module_CPUFreq CPU Frequency Monitor
 *
 * Monitors CPU frequency and may do actions given some thresholds.
 *
 * @see Module_Temperature
 * @see Module_Battery
 * @}
 */
#endif
