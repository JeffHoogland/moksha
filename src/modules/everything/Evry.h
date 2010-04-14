#ifndef EVRY_H
#define EVRY_H

#include "e.h"

#define EVRY_API_VERSION 1


#define EVRY_ACTION_OTHER 0
#define EVRY_ACTION_FINISHED 1
#define EVRY_ACTION_CONTINUE 2

#define EVRY_ASYNC_UPDATE_ADD 0
#define EVRY_ASYNC_UPDATE_CLEAR 1
#define EVRY_ASYNC_UPDATE_REFRESH 2

#define VIEW_MODE_LIST   0
#define VIEW_MODE_DETAIL 1
#define VIEW_MODE_THUMB  2

extern int _e_module_evry_log_dom;

#ifndef EINA_LOG_DEFAULT_COLOR
#define EINA_LOG_DEFAULT_COLOR EINA_COLOR_CYAN
#endif

#undef DBG
#undef INF
#undef WRN
#undef ERR

#define DBG(...) EINA_LOG_DOM_DBG(_e_module_evry_log_dom , __VA_ARGS__)
#define INF(...) EINA_LOG_DOM_INFO(_e_module_evry_log_dom , __VA_ARGS__)
#define WRN(...) EINA_LOG_DOM_WARN(_e_module_evry_log_dom , __VA_ARGS__)
#define ERR(...) EINA_LOG_DOM_ERR(_e_module_evry_log_dom , __VA_ARGS__)

typedef struct _Evry_Plugin    Evry_Plugin;
typedef struct _Evry_Item      Evry_Item;
typedef struct _Evry_Item_App  Evry_Item_App;
typedef struct _Evry_Item_File Evry_Item_File;
typedef struct _Evry_Action    Evry_Action;
typedef struct _Evry_State     Evry_State;
typedef struct _Evry_View      Evry_View;
typedef struct _Plugin_Config  Plugin_Config;
typedef struct _History Evry_History;
typedef struct _History_Entry History_Entry;
typedef struct _History_Item   History_Item;
typedef struct _Config Evry_Config;

#define EVRY_ITEM(_item) ((Evry_Item *)_item)
#define EVRY_PLUGIN(_plugin) ((Evry_Plugin *) _plugin)
#define EVRY_VIEW(_view) ((Evry_View *) _view)
#define ITEM_FILE(_file, _item) Evry_Item_File *_file = (Evry_Item_File *) _item
#define ITEM_APP(_app, _item)   Evry_Item_App *_app = (Evry_Item_App *) _item
#define PLUGIN(_p, _plugin) Plugin *_p = (Plugin*) _plugin
#define VIEW(_v, _view) View *_v = (View*) _view

#define EVRY_PLUGIN_ITEMS_CLEAR(_p)       		\
  if (EVRY_PLUGIN(_p)->items)				\
    eina_list_free(EVRY_PLUGIN(_p)->items); 		\
  EVRY_PLUGIN(_p)->items = NULL;

#define EVRY_PLUGIN_ITEMS_FREE(_p)			\
  Evry_Item *evryitem;					\
  EINA_LIST_FREE(EVRY_PLUGIN(_p)->items, evryitem)	\
    evry_item_free(evryitem);

#define EVRY_PLUGIN_ITEMS_SORT(_p, _sortcb)		\
  EVRY_PLUGIN(_p)->items = eina_list_sort		\
    (EVRY_PLUGIN(_p)->items,				\
     eina_list_count(EVRY_PLUGIN(_p)->items), _sortcb);	\

#define EVRY_PLUGIN_ITEM_APPEND(_p, _item)			\
  EVRY_PLUGIN(_p)->items =					\
    eina_list_append(EVRY_PLUGIN(_p)->items, EVRY_ITEM(_item))	\

/* if you extended a plugin struct and you are sure
   not to have any data lying around after cleanup you
   can use this */
#define EVRY_PLUGIN_FREE(_p)			\
  evry_plugin_free(EVRY_PLUGIN(_p), 0);		\
  E_FREE(_p);


struct _Plugin_Config
{
  const char *name;

  int loaded;
  int enabled;

  /* minimum input chars to query this source */
  int min_query;

  int priority;

  const char *trigger;
};


struct _Evry_Item
{
  /* label to show for this item */
  const char *label;

  /* optional: more information to be shown */
  const char *detail;
  
  /* optional: for 'static' fdo icon name, otherwise use _icon_get */
  const char *icon;
  
  /* item can be browsed, e.g. folders */
  Eina_Bool browseable;

  /* optional: for internally use by plugins */
  void *data;

  /* optional: priority hints for sorting */
  int priority;

  /* optional: store value of fuzzy match with input */
  int fuzzy_match;

  /* optional: plugin can set id to identify
   * it in history otherwise label is used */
  const char *id;

  /* optional: context provided by item. e.g. to remember which action
   * was performed on a file with a specific mimetype */
  const char *context;

  /* do not set by plugin! */
  Evry_Item   *next;
  Eina_Bool    selected;
  Evry_Plugin *plugin;
  int ref;
  void (*free) (Evry_Item *item);
  double usage;
};

struct _Evry_Item_App
{
  Evry_Item base;
  const char *file;
  Efreet_Desktop *desktop;
};

struct _Evry_Item_File
{
  Evry_Item base;
  const char *url;
  const char *path;
  const char *mime;
};

struct _Evry_Plugin
{
  const char *name;
  const char *icon;

  enum { type_subject, type_action, type_object } type;

  const char *type_in;
  const char *type_out;

  const char *trigger;

  /* list of items visible for everything */
  Eina_List *items;

  /* run when plugin is activated. when returns positve it is added
     to the list of current plugins and queried for results */
  Evry_Plugin *(*begin) (Evry_Plugin *p, const Evry_Item *item);

  /* get candidates matching string, fill 'items' list */
  int  (*fetch) (Evry_Plugin *p, const char *input);

  /* run when state is removed in which this plugin is active */
  void (*cleanup) (Evry_Plugin *p);

  /* get an icon for an item. will be freed automatically */
  Evas_Object *(*icon_get) (Evry_Plugin *p, const Evry_Item *it, Evas *e);
  
  /* only used when plugin is of type_action */
  int  (*action) (Evry_Plugin *p, const Evry_Item *item);

  /* handle key events: return 1 when key is handled by plugin */
  int  (*cb_key_down)  (Evry_Plugin *p, const Ecore_Event_Key *ev);

  /* optional: use this when you extend the plugin struct */
  void (*free) (Evry_Plugin *p);
  
  /* show in aggregator */
  /* default TRUE */
  Eina_Bool aggregate;
  
  /* whether the plugin uses evry_async_update to add new items. */
  /* default FALSE */
  Eina_Bool async_fetch;

  /* request VIEW_MODE for plugin */
  int view_mode;

  /* request items to be remembered for usage statistic */
  /* default TRUE */
  Eina_Bool history;

  /* if transient, item is removed from history on shutdown */
  /* default FALSE */
  Eina_Bool transient;
  
  /* not to be set by plugin! */
  Plugin_Config *config;
};


struct _Evry_State
{
  char *input;
  /* all available plugins for current state */
  Eina_List   *plugins;

  /* currently active plugins, i.e. those that provide items */
  Eina_List   *cur_plugins;

  /* active plugin */
  Evry_Plugin *plugin;

  /* selected item */
  Evry_Item   *cur_item;

  Eina_List   *sel_items;

  /* this is for the case when the current plugin was not selected
     manually and a higher priority (async) plugin retrieves
     candidates, the higher priority plugin is made current */
  Eina_Bool plugin_auto_selected;
  Eina_Bool item_auto_selected;

  Evry_View *view;
};

struct _Evry_View
{
  Evry_View  *id;
  const char *name;
  const char *trigger;
  int active;
  Evas_Object *o_list;
  Evas_Object *o_bar;

  Evry_View *(*create) (Evry_View *view, const Evry_State *s, const Evas_Object *swallow);
  void (*destroy)      (Evry_View *view);
  int  (*cb_key_down)  (Evry_View *view, const Ecore_Event_Key *ev);
  int  (*update)       (Evry_View *view);
  void (*clear)        (Evry_View *view);

  int priority;
};

struct _Evry_Action
{
  const char *name;

  const char *type_in1;
  const char *type_in2;
  const char *type_out;

  const Evry_Item *item1;
  const Evry_Item *item2;

  int  (*action)     (Evry_Action *act);
  int  (*check_item) (Evry_Action *act, const Evry_Item *it);
  int  (*intercept)  (Evry_Action *act);
  void (*cleanup)    (Evry_Action *act);
  Eina_List *(*actions)    (Evry_Action *act);
  Evas_Object *(*icon_get) (Evry_Action *act, Evas *e);

  /* optional: use this when you keep stuff in 'data' */
  void (*free) (Evry_Action *act);
  
  /* use icon name from theme */
  const char *icon;

  void *data;

  int priority;
};

struct _Config
{
  int version;
  /* position */
  double rel_x, rel_y;
  /* size */
  int width, height;

  Eina_List *modules;
  
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

/* evry.c */
EAPI void evry_item_select(const Evry_State *s, Evry_Item *it);
EAPI void evry_item_mark(const Evry_State *state, Evry_Item *it, Eina_Bool mark);
EAPI void evry_plugin_select(const Evry_State *s, Evry_Plugin *p);
EAPI int  evry_list_win_show(void);
EAPI void evry_list_win_hide(void);
EAPI Evry_Item *evry_item_new(Evry_Item *base, Evry_Plugin *p, const char *label, void (*cb_free) (Evry_Item *item));
EAPI void evry_item_free(Evry_Item *it);
EAPI void evry_item_ref(Evry_Item *it);
EAPI void evry_plugin_async_update(Evry_Plugin *plugin, int state);
EAPI void evry_clear_input(void);

/* evry_util.c */
EAPI Evas_Object *evry_icon_mime_get(const char *mime, Evas *e);
EAPI Evas_Object *evry_icon_theme_get(const char *icon, Evas *e);
EAPI int  evry_fuzzy_match(const char *str, const char *match);
EAPI Eina_List *evry_fuzzy_match_sort(Eina_List *items);
EAPI int evry_util_exec_app(const Evry_Item *it_app, const Evry_Item *it_file);
EAPI char *evry_util_unescape(const char *string, int length);
EAPI void evry_util_file_detail_set(Evry_Item_File *file);

/* e_mod_main.c */
EAPI void evry_plugin_register(Evry_Plugin *p, int priority);
EAPI void evry_plugin_unregister(Evry_Plugin *p);
EAPI void evry_action_register(Evry_Action *act, int priority);
EAPI void evry_action_unregister(Evry_Action *act);
EAPI void evry_view_register(Evry_View *view, int priority);
EAPI void evry_view_unregister(Evry_View *view);

EAPI void evry_history_load(void);
EAPI void evry_history_unload(void);
EAPI void evry_history_add(Eina_Hash *hist, Evry_State *s, const char *ctxt);
EAPI int  evry_history_item_usage_set(Eina_Hash *hist, Evry_Item *it, const char *input, const char *ctxt);

EAPI Evry_Plugin *evry_plugin_new(Evry_Plugin *base, const char *name, int type,
				  const char *type_in, const char *type_out,
				  int async_fetch, const char *icon, const char *trigger,
				  Evry_Plugin *(*begin) (Evry_Plugin *p, const Evry_Item *item),
				  void (*cleanup) (Evry_Plugin *p),
				  int  (*fetch)   (Evry_Plugin *p, const char *input),
				  int  (*action)  (Evry_Plugin *p, const Evry_Item *item),
				  Evas_Object *(*icon_get) (Evry_Plugin *p, const Evry_Item *it, Evas *e),
				  void (*free) (Evry_Plugin *p));

EAPI void evry_plugin_free(Evry_Plugin *p, int free_pointer);


EAPI Evry_Action *evry_action_new(const char *name, const char *type_in1,
				  const char *type_in2, const char *type_out,
				  const char *icon,
				  int  (*action)     (Evry_Action *act),
				  int  (*check_item) (Evry_Action *act, const Evry_Item *it),
				  void (*cleanup)    (Evry_Action *act),
				  int  (*intercept)  (Evry_Action *act),
				  Evas_Object *(*icon_get) (Evry_Action *act, Evas *e),
				  void (*free) (Evry_Action *p));

EAPI void evry_action_free(Evry_Action *act);

EAPI int evry_api_version_check(int version);

typedef struct _Evry_Event_Item_Changed Evry_Event_Item_Changed;

struct _Evry_Event_Item_Changed
{
  Evry_Item *item;
};

extern EAPI int EVRY_EVENT_ITEM_SELECT;
extern EAPI int EVRY_EVENT_ITEM_CHANGED;
extern EAPI int EVRY_EVENT_ITEMS_UPDATE;

EAPI extern Evry_History *evry_hist;
EAPI extern Evry_Config *evry_conf;

#endif
