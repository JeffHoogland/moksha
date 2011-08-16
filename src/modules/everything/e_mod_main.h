#ifndef EVRY_H
#define EVRY_H

#include "e.h"
#include "evry_api.h"

#define MOD_CONFIG_FILE_EPOCH 0x0005
#define MOD_CONFIG_FILE_GENERATION 0x0002
#define MOD_CONFIG_FILE_VERSION					\
  ((MOD_CONFIG_FILE_EPOCH << 16) | MOD_CONFIG_FILE_GENERATION)

#define SLIDE_LEFT   1
#define SLIDE_RIGHT -1

typedef struct _History		Evry_History;
typedef struct _Config		Evry_Config;
typedef struct _Evry_Selector	Evry_Selector;
typedef struct _Tab_View	Tab_View;
typedef struct _Evry_Window	Evry_Window;

struct _Evry_Window
{
  E_Win *ewin;
  Evas *evas;
  E_Zone *zone;
  Eina_Bool shaped;
  Evas_Object *o_main;

  Eina_Bool request_selection;
  Eina_Bool plugin_dedicated;
  Eina_Bool visible;

  Eina_List *handlers;

  Evry_Selector  *selector;
  Evry_Selector **selectors;
  Evry_Selector **sel_list;

  unsigned int level;

  unsigned int mouse_button;
  Eina_Bool mouse_out;
  
  Eina_Bool grab;

  Evry_State *state_clearing;
};

struct _Evry_Selector
{
  Evry_Window *win;

  /* current state */
  Evry_State  *state;

  /* stack of states (for browseable plugins) */
  Eina_List   *states;

  /* provides collection of items from other plugins */
  Evry_Plugin *aggregator;

  /* action selector plugin */
  Evry_Plugin *actions;

  /* all plugins that belong to this selector*/
  Eina_List   *plugins;

  /* list view instance */
  Evry_View   *view;

  Evas_Object *o_icon;
  Evas_Object *o_thumb;
  Eina_Bool    do_thumb;

  Ecore_Timer *update_timer;
  Ecore_Timer *action_timer;

  const char *edje_part;
};

struct _Evry_State
{
  Evry_Selector *selector;

  char *inp; /* alloced input */

  char *input; /* pointer to input + trigger */
  /* all available plugins for current state */
  Eina_List   *plugins;

  /* currently active plugins, i.e. those that provide items */
  Eina_List   *cur_plugins;

  /* active plugin */
  Evry_Plugin *plugin;

  /* aggregator instance */
  Evry_Plugin *aggregator;

  /* selected item */
  Evry_Item   *cur_item;

  /* marked items */
  Eina_List   *sel_items;

  Eina_Bool plugin_auto_selected;
  Eina_Bool item_auto_selected;

  /* current view instance */
  Evry_View *view;

  Eina_Bool changed;
  Eina_Bool trigger_active;

  unsigned int request;

  Ecore_Timer *clear_timer;

  Eina_Bool delete_me;
};

struct _Tab_View
{
  const Evry_State *state;

  Evry_View *view;
  Evas *evas;

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

struct _Config
{
  int version;
  /* position */
  double rel_x, rel_y;
  /* size */
  int width, height;
  int edge_width, edge_height;

  Eina_List *modules;

  /* generic plugin config */
  Eina_List *conf_subjects;
  Eina_List *conf_actions;
  Eina_List *conf_objects;
  Eina_List *conf_views;
  Eina_List *collections;

  int scroll_animate;
  double scroll_speed;

  int hide_input;
  int hide_list;

  /* quick navigation mode */
  int quick_nav;

  /* default view mode */
  int view_mode;
  int view_zoom;

  int history_sort_mode;

  /* use up/down keys for prev/next in thumb view */
  int cycle_mode;

  unsigned char first_run;

  /* not saved data */
  Eina_List *actions;
  Eina_List *views;

  int min_w, min_h;
};

struct _History
{
  int version;
  Eina_Hash *subjects;
  double begin;
};

/*** Evry_Api functions ***/
void  evry_item_select(const Evry_State *s, Evry_Item *it);
void  evry_item_mark(const Evry_State *state, Evry_Item *it, Eina_Bool mark);
void  evry_plugin_select(Evry_Plugin *p);
Evry_Item *evry_item_new(Evry_Item *base, Evry_Plugin *p, const char *label,
			      Evas_Object *(*icon_get) (Evry_Item *it, Evas *e),
			      void (*cb_free) (Evry_Item *item));
void  evry_item_free(Evry_Item *it);
void  evry_item_ref(Evry_Item *it);

void  evry_plugin_update(Evry_Plugin *plugin, int state);
void  evry_clear_input(Evry_Plugin *p);

/* evry_util.c */
/* Evas_Object *evry_icon_mime_get(const char *mime, Evas *e); */
Evas_Object *evry_icon_theme_get(const char *icon, Evas *e);
int   evry_fuzzy_match(const char *str, const char *match);
Eina_List *evry_fuzzy_match_sort(Eina_List *items);
int   evry_util_exec_app(const Evry_Item *it_app, const Evry_Item *it_file);
char *evry_util_url_escape(const char *string, int inlength);
char *evry_util_url_unescape(const char *string, int length);
void  evry_util_file_detail_set(Evry_Item_File *file);
int   evry_util_module_config_check(const char *module_name, int conf, int epoch, int version);
Evas_Object *evry_util_icon_get(Evry_Item *it, Evas *e);
int   evry_util_plugin_items_add(Evry_Plugin *p, Eina_List *items, const char *input, int match_detail, int set_usage);
int   evry_items_sort_func(const void *data1, const void *data2);
void  evry_item_changed(Evry_Item *it, int change_icon, int change_selected);
char *evry_util_md5_sum(const char *str);

const char *evry_file_path_get(Evry_Item_File *file);
const char *evry_file_url_get(Evry_Item_File *file);

int   evry_plugin_register(Evry_Plugin *p, int type, int priority);
void  evry_plugin_unregister(Evry_Plugin *p);
Evry_Plugin *evry_plugin_find(const char *name);
void  evry_action_register(Evry_Action *act, int priority);
void  evry_action_unregister(Evry_Action *act);
void  evry_view_register(Evry_View *view, int priority);
void  evry_view_unregister(Evry_View *view);
Evry_Action *evry_action_find(const char *name);

void  evry_history_load(void);
void  evry_history_unload(void);
History_Item *evry_history_item_add(Evry_Item *it, const char *ctxt, const char *input);
int   evry_history_item_usage_set(Evry_Item *it, const char *input, const char *ctxt);
History_Types *evry_history_types_get(Evry_Type type);

Evry_Plugin *evry_plugin_new(Evry_Plugin *base, const char *name, const char *label, const char *icon,
			     Evry_Type item_type,
			     Evry_Plugin *(*begin) (Evry_Plugin *p, const Evry_Item *item),
			     void (*cleanup) (Evry_Plugin *p),
			     int  (*fetch)   (Evry_Plugin *p, const char *input),
			     void (*free) (Evry_Plugin *p));

void  evry_plugin_free(Evry_Plugin *p);

Evry_Action *evry_action_new(const char *name, const char *label,
			     Evry_Type type1, Evry_Type type2,
			     const char *icon,
			     int  (*action)     (Evry_Action *act),
			     int  (*check_item) (Evry_Action *act, const Evry_Item *it));

void  evry_action_free(Evry_Action *act);

int   evry_api_version_check(int version);

Evry_Type evry_type_register(const char *type);
const char *evry_type_get(Evry_Type type);

/*** internal ***/
Tab_View *evry_tab_view_new(Evry_View *view, const Evry_State *s, Evas *e);
void  evry_tab_view_free(Tab_View *v);

Eina_Bool evry_view_init(void);
void  evry_view_shutdown(void);

Eina_Bool evry_view_help_init(void);
void  evry_view_help_shutdown(void);

Eina_Bool evry_plug_clipboard_init(void);
void  evry_plug_clipboard_shutdown(void);

Eina_Bool evry_plug_text_init(void);
void  evry_plug_text_shutdown(void);

Eina_Bool evry_plug_collection_init(void);
void  evry_plug_collection_shutdown(void);

int   evry_init(void);
int   evry_shutdown(void);
Evry_Window *evry_show(E_Zone *zone, E_Zone_Edge edge, const char *params, Eina_Bool popup);
void  evry_hide(Evry_Window *win, int clear);

int   evry_plug_actions_init();
void  evry_plug_actions_shutdown();

Evry_Plugin *evry_aggregator_new(Evry_Window *win, int type);

void  evry_history_init(void);
void  evry_history_free(void);

int   evry_browse_item(Evry_Item *it);
int   evry_browse_back(Evry_Selector *sel);

void  evry_plugin_action(Evry_Window *win, int finished);

int   evry_state_push(Evry_Selector *sel, Eina_List *plugins);
int   evry_selectors_switch(Evry_Window *win,int dir, int slide);
int   evry_view_toggle(Evry_State *s, const char *trigger);

int evry_gadget_init(void);
void evry_gadget_shutdown(void);

Eina_Bool evry_plug_apps_init(E_Module *m);
void evry_plug_apps_shutdown(void);
void evry_plug_apps_save(void);

Eina_Bool evry_plug_files_init(E_Module *m);
void evry_plug_files_shutdown(void);
void evry_plug_files_save(void);

Eina_Bool evry_plug_windows_init(E_Module *m);
void evry_plug_windows_shutdown(void);
void evry_plug_windows_save(void);

Eina_Bool evry_plug_settings_init(E_Module *m);
void evry_plug_settings_shutdown(void);
void evry_plug_settings_save(void);

Eina_Bool evry_plug_calc_init(E_Module *m);
void evry_plug_calc_shutdown(void);
void evry_plug_calc_save(void);

Ecore_Event_Handler *evry_event_handler_add(int type, Eina_Bool (*func) (void *data, int type, void *event), const void *data);

extern Evry_API *evry;
extern Evry_History *evry_hist;
extern Evry_Config  *evry_conf;
extern int  _evry_events[NUM_EVRY_EVENTS];

/*** E Module ***/
EAPI void *e_modapi_init     (E_Module *m);
EAPI int   e_modapi_shutdown (E_Module *m);
EAPI int   e_modapi_save     (E_Module *m);
EAPI E_Config_Dialog *evry_config_dialog(E_Container *con, const char *params);
EAPI E_Config_Dialog *evry_collection_conf_dialog(E_Container *con, const char *params);
EAPI extern E_Module_Api e_modapi;

//#define CHECK_REFS 1
//#define PRINT_REFS 1
//#define CHECK_TIME 1
//#undef DBG
//#define DBG(...) ERR(__VA_ARGS__)

#ifdef CHECK_REFS
extern Eina_List *_refd;
#endif

#ifdef CHECK_TIME
extern double _evry_time;
#endif

#endif
