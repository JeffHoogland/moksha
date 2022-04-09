#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init     (E_Module *m);
EAPI int   e_modapi_shutdown (E_Module *m);
EAPI int   e_modapi_save     (E_Module *m);

typedef struct _Config Config;
typedef struct _Config_Item Config_Item;

struct _Config
{
  Eina_List *items;

  E_Module *module;
  E_Config_Dialog *config_dialog;
};

struct _Config_Item
{
  const char *id;
  const char *custom_date_const;  
  
  struct {
      int start, len; // 0->6 0 == sun, 6 == sat, number of days
   } weekend;
  struct {
      int start; // 0->6 0 == sun, 6 == sat
   } week;
  struct {
      double hour, minute;
   } timeset;
   int digital_clock;
   int digital_24h;
   int show_seconds;
   int show_date;
   Eina_Bool changed;
};

void e_int_config_clock_module(E_Container *con, Config_Item *ci);
void e_int_clock_instances_redo(Eina_Bool all);

extern Config *clock_config;


/**
 * @addtogroup Optional_Gadgets
 * @{
 *
 * @defgroup Module_Clock Clock
 *
 * Shows current time and date.
 *
 * @}
 */

#endif
