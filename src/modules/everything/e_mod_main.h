#include "Evry.h"

#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H


typedef struct _Config Config;
typedef struct _Evry_Selector Evry_Selector;
typedef struct _Tab_View Tab_View;
typedef struct _History History;
typedef struct _History_Entry History_Entry;
typedef struct _History_Item History_Item;

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

  const char *cmd_terminal;
  const char *cmd_sudo;

  int view_mode;
  int view_zoom;

  int history_sort_mode;
  
  /* use up/down keys for prev/next in thumb view */
  int cycle_mode;

  /* not saved data */
  Eina_List *plugins;
  Eina_List *actions;
  Eina_List *views;

  int min_w, min_h;
};



struct _History_Item
{
  const char *plugin;
  const char *context;
  const char *input;
  double last_used;
  double usage;
  int count;
  int transient;
};

struct _History_Entry
{
  Eina_List *items;
};

struct _History
{
  int version;
  Eina_Hash *subjects;
  Eina_Hash *actions;
  double begin;
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

  Evry_Plugin *actions;

  /* */
  Eina_List   *cur_actions;

  /* all plugins that belong to this selector*/
  Eina_List   *plugins;

  Evry_View   *view;

  Evas_Object *o_thumb;
  Eina_Bool    do_thumb;

  Eina_Hash   *history;

  Ecore_Timer *update_timer;
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

EAPI Tab_View *evry_tab_view_new(const Evry_State *s, Evas *e);
EAPI void evry_tab_view_free(Tab_View *v);

int  evry_init(void);
int  evry_shutdown(void);
int  evry_show(E_Zone *zone, const char *params);
void evry_hide(void);

EAPI Evry_Plugin *evry_plug_aggregator_new(Evry_Selector *selector);
EAPI void evry_plug_aggregator_free(Evry_Plugin *plugin);

EAPI Evry_Plugin *evry_plug_actions_new(int type);
EAPI void evry_plug_actions_free(Evry_Plugin *plugin);

void evry_history_init(void);
void evry_history_free(void);
EAPI void evry_history_load(void);
EAPI void evry_history_unload(void);
EAPI void evry_history_add(Eina_Hash *hist, Evry_State *s, const char *ctxt);
int  evry_history_item_usage_set(Eina_Hash *hist, Evry_Item *it, const char *input, const char *ctxt);

EAPI int  evry_browse_item(Evry_Selector *sel);
void evry_browse_back(Evry_Selector *sel);

EAPI extern Config *evry_conf;
EAPI extern History *evry_hist;
extern Evry_Selector **selectors;
extern const char *action_selector;
#endif
