#include "e.h"

#define EVRY_ACTION_OTHER 0
#define EVRY_ACTION_FINISHED 1
#define EVRY_ACTION_CONTINUE 2

#define EVRY_ASYNC_UPDATE_ADD 0
#define EVRY_ASYNC_UPDATE_CLEAR 1


typedef struct _Evry_Plugin Evry_Plugin;
typedef struct _Evry_Item   Evry_Item;
typedef struct _Evry_Action Evry_Action;
typedef struct _Evry_State Evry_State;
typedef struct _Evry_View   Evry_View;
typedef struct _Evry_App Evry_App;
typedef struct _Plugin_Config Plugin_Config;

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

  const char *uri;
  const char *mime;

  /* item can be browsed, e.g. folders */
  Eina_Bool browseable;

  /* these are only for internally use by plugins */
  /* used e.g. as pointer for item data (Efreet_Desktop) */
  void *data[4];

  /* priority hints for sorting */
  int priority;

  /* store value of fuzzy match with input */
  int fuzzy_match;

  /* do not set by plugin! */
  Evry_Plugin *plugin;
  Evas_Object *o_icon;
  Evas_Object *o_bg;
  int ref;
  void (*cb_free) (Evry_Item *item);
};

struct _Evry_Plugin
{
  const char *name;
  const char *icon;

  enum { type_subject, type_action, type_object } type;

  const char *type_in;
  const char *type_out;

  const char *trigger;

  /* whether the plugin uses evry_async_update to add new items */
  int async_fetch;

  /* run when plugin is activated. when return true plugin is added
     to the list of current plugins and queried for results */
  int (*begin) (Evry_Plugin *p, const Evry_Item *item);

  int (*browse) (Evry_Plugin *p, const Evry_Item *item);

  /* get candidates matching string, fills 'candidates' list */
  int  (*fetch) (Evry_Plugin *p, const char *input);

  /* run when state is removed in which this plugin is active */
  void (*cleanup) (Evry_Plugin *p);

  Evas_Object *(*icon_get) (Evry_Plugin *p, const Evry_Item *it, Evas *e);
  /* provide more information for a candidate */
  /* int (*candidate_info) (Evas *evas, Evry_Item *item); */

  /* optional: default action for this plugins items */
  int  (*action) (Evry_Plugin *p, const Evry_Item *item, const char *input);


  Eina_List *items;

  Evas_Object *(*config_page) (Evry_Plugin *p);
  void (*config_apply) (Evry_Plugin *p);

  /* only for internal use by plugin */
  void *private;

  /* not to be set by plugin! */
  Evas_Object *tab;
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
  Evry_Item   *sel_item;

  /* Eina_List *sel_items; */

  /* this is for the case when the current plugin was not selected
     manually and a higher priority (async) plugin retrieves
     candidates, the higher priority plugin is made current */
  Eina_Bool plugin_auto_selected;
  Eina_Bool item_auto_selected;

  Evry_View *view;
  Evas_Object *o_view;
};

struct _Evry_View
{
  const char *name;

  Evas_Object *(*begin) (Evry_View *v, const Evry_State *s, const Evas_Object *swallow);

  int (*cb_key_down) (Evry_View *v, const Ecore_Event_Key *ev);
  int (*update) (Evry_View *v, const Evry_State *s);
  void (*clear)  (Evry_View *v, const Evry_State *s);
  void (*cleanup) (Evry_View *v);

  const Evry_State *state;

  int priority;
};

struct _Evry_Action
{
  const char *name;

  const char *type_in1;
  const char *type_in2;

  int  (*action) (Evry_Action *act, const Evry_Item *it1, const Evry_Item *it2, const char *input);

  int (*check_item) (Evry_Action *act, const Evry_Item *it);

  Evas_Object *(*icon_get) (Evry_Action *act, Evas *e);

  /* use icon name from theme */
  const char *icon;

  Eina_Bool is_default;

  /* only for internal use by plugin */
  void *private;

  /* not to be set by plugin! */
  Evas_Object *o_icon;
};

struct _Evry_App
{
  const char *file;
  Efreet_Desktop *desktop;
};

void evry_plugin_register(Evry_Plugin *p, int priority);
void evry_plugin_unregister(Evry_Plugin *p);
void evry_action_register(Evry_Action *act);
void evry_action_unregister(Evry_Action *act);
void evry_view_register(Evry_View *view, int priority);
void evry_view_unregister(Evry_View *view);

void evry_item_select(const Evry_State *s, Evry_Item *it);
int  evry_list_win_show(void);
void evry_list_win_hide(void);


Evry_Item *evry_item_new(Evry_Plugin *p, const char *label, void (*cb_free) (Evry_Item *item));
void evry_item_free(Evry_Item *it);
void evry_item_ref(Evry_Item *it);

void evry_plugin_async_update(Evry_Plugin *plugin, int state);
void evry_clear_input(void);

Evas_Object *evry_icon_mime_get(const char *mime, Evas *e);
Evas_Object *evry_icon_theme_get(const char *icon, Evas *e);

int  evry_fuzzy_match(const char *str, const char *match);

Evry_Plugin *evry_plugin_new(const char *name, int type,
			     const char *type_in, const char *type_out,
			     int async_fetch, const char *icon, const char *trigger,
			     int  (*begin)   (Evry_Plugin *p, const Evry_Item *item),
			     void (*cleanup) (Evry_Plugin *p),
			     int  (*fetch)   (Evry_Plugin *p, const char *input),
			     int  (*action)  (Evry_Plugin *p, const Evry_Item *item, const char *input),
			     int  (*browse)  (Evry_Plugin *p, const Evry_Item *item),
			     Evas_Object *(*icon_get) (Evry_Plugin *p, const Evry_Item *it, Evas *e),
			     Evas_Object *(*config_page) (Evry_Plugin *p),
			     void (*config_apply) (Evry_Plugin *p));

void evry_plugin_free(Evry_Plugin *p);


Evry_Action *evry_action_new(const char *name, const char *type_in1, const char *type_in2, const char *icon,
			     int  (*action) (Evry_Action *act, const Evry_Item *it1, const Evry_Item *it2, const char *input),
			     int (*check_item) (Evry_Action *act, const Evry_Item *it),
			     Evas_Object *(*icon_get) (Evry_Action *act, Evas *e));

void evry_action_free(Evry_Action *act);
