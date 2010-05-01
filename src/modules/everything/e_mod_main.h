#include "Evry.h"

#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H


typedef struct _Evry_Selector Evry_Selector;
typedef struct _Tab_View Tab_View;


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
  /* Eina_List   *cur_actions; */

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

  double align;
  double align_to;
  Ecore_Animator *animator;
  Ecore_Timer *timer;
};



EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init     (E_Module *m);
EAPI int   e_modapi_shutdown (E_Module *m);
EAPI int   e_modapi_save     (E_Module *m);
EAPI E_Config_Dialog *evry_config_dialog(E_Container *con, const char *params);

Tab_View *evry_tab_view_new(const Evry_State *s, Evas *e);
void evry_tab_view_free(Tab_View *v);

Eina_Bool view_thumb_init(void);
void view_thumb_shutdown(void);

Eina_Bool view_help_init(void);
void view_help_shutdown(void);

Eina_Bool view_preview_init(void);
void view_preview_shutdown(void);

Eina_Bool evry_plug_clipboard_init(void);
void evry_plug_clipboard_shutdown(void);

Eina_Bool evry_plug_text_init(void);
void evry_plug_text_shutdown(void);

int  evry_init(void);
int  evry_shutdown(void);
int  evry_show(E_Zone *zone, const char *params);
void evry_hide(int clear);

Evry_Plugin *evry_plug_aggregator_new(Evry_Selector *selector, int type);

int evry_plug_actions_init();
void evry_plug_actions_shutdown();
Evry_Plugin *evry_plug_actions_new(Evry_Selector *selector, int type);

void evry_history_init(void);
void evry_history_free(void);

EAPI int evry_browse_item(Evry_Selector *sel);
EAPI int evry_browse_back(Evry_Selector *sel);

extern Evry_Selector **selectors;

#endif
