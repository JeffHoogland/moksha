#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init     (E_Module *m);
EAPI int   e_modapi_shutdown (E_Module *m);
EAPI int   e_modapi_save     (E_Module *m);

typedef struct _Config Config;

struct _Config
{
   struct {
      int           start, len; // 0->6 0 == sun, 6 == sat, number of days
   } weekend;
   struct {
      int           start; // 0->6 0 == sun, 6 == sat
   } week;
   int digital_clock;
   int digital_24h;
   int show_seconds;
};

extern E_Module *clock_module;
extern E_Config_Dialog *clock_config;
extern Config *clock_cfg;

E_Config_Dialog *e_int_config_clock_module(E_Container *con, const char *params);
void e_int_clock_instances_redo(void);

#endif
