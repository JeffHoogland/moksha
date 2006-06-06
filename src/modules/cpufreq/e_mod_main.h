#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

typedef struct _Status       Status;
typedef struct _Config       Config;

struct _Status
{
   Evas_List     *frequencies;
   Evas_List     *governors;
   int            cur_frequency;
   int            can_set_frequency;
   char          *cur_governor;
   unsigned char  active;
};

struct _Config
{
   /* saved * loaded config values */
   double       poll_time;
   int          restore_governor;
   const char        *governor;
   /* just config state */
   E_Module    *module;
   Evas_List   *instances;
   E_Menu      *menu;
   E_Menu      *menu_poll;
   E_Menu      *menu_governor;
   E_Menu      *menu_frequency;
   Status      *status;
   char        *set_exe_path;
   Ecore_Timer *frequency_check_timer;
};

EAPI extern E_Module_Api e_modapi;

EAPI int   e_modapi_init     (E_Module *m);
EAPI int   e_modapi_shutdown (E_Module *m);
EAPI int   e_modapi_save     (E_Module *m);
EAPI int   e_modapi_about    (E_Module *m);
EAPI int   e_modapi_config   (E_Module *m);

#endif
