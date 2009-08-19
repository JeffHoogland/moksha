#include "Evry.h"

#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H


typedef struct _Config Config;

struct _Config
{
  int version;
  /* position */
  double rel_x, rel_y;
  /* size */
  int width, height;

  /* generic plugin config */
  Eina_List *conf_subjects;
  Eina_List *conf_actions;
  Eina_List *conf_objects;

  int scroll_animate;
  double scroll_speed;

  int hide_input;
  int hide_list;

  int quick_nav;

  Eina_Hash *key_bindings;

  /**/
  Eina_List *plugins;
  Eina_List *actions;
  Eina_List *views;

  Eina_Hash *history;
};

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init     (E_Module *m);
EAPI int   e_modapi_shutdown (E_Module *m);
EAPI int   e_modapi_save     (E_Module *m);
EAPI E_Config_Dialog *evry_config_dialog(E_Container *con, const char *params);

int  evry_init(void);
int  evry_shutdown(void);
int  evry_show(E_Zone *zone, const char *params);
void evry_hide(void);

extern Config *evry_conf;

#endif
