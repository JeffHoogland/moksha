#include "Evry.h"

#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H


typedef struct _Config Config;
typedef struct _Evry_Selector Evry_Selector;
typedef struct _Tab_View Tab_View;

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
  Eina_List *conf_views;

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

struct _Evry_Selector
{
  Evas_Object *o_main;
  Evas_Object *o_icon;

  /* current state */
  Evry_State  *state;

  /* stack of states (for browseable plugins) */
  Eina_List   *states;

  /* provides collection of items from other plugins */
  Evry_Plugin *aggregator;

  /* */
  Eina_List   *actions;

  /* all plugins that belong to this selector*/
  Eina_List   *plugins;

  Evry_View   *view;

  Evas_Object *o_thumb;
  Eina_Bool    do_thumb;
};

struct _Tab_View
{
  Evas *evas;
  const Evry_State *state;

  Evas_Object *o_tabs;
  Eina_List *tabs;

  void (*update) (Tab_View *tv);
  void (*clear) (Tab_View *tv);
  int (*key_down) (Tab_View *tv, const Ecore_Event_Key *ev);
  
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

Evry_Plugin *evry_plug_aggregator_new(Evry_Selector *selector);
void evry_plug_aggregator_free(Evry_Plugin *plugin);

Evry_Plugin *evry_plug_actions_new(void);
void evry_plug_actions_free(Evry_Plugin *plugin);

EAPI Tab_View *evry_tab_view_new(const Evry_State *s, Evas *e);
EAPI void evry_tab_view_free(Tab_View *v);

extern Config *evry_conf;
extern Evry_Selector **selectors;
extern Evry_Plugin *action_selector;
#endif
