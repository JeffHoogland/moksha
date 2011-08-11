#ifndef EVRY_TYPES_H
#define EVRY_TYPES_H

typedef struct _Evry_Plugin		Evry_Plugin;
typedef struct _Plugin_Config		Plugin_Config;
typedef struct _Evry_Item		Evry_Item;
typedef struct _Evry_Item_App		Evry_Item_App;
typedef struct _Evry_Item_File		Evry_Item_File;
typedef struct _Evry_Action		Evry_Action;
typedef struct _History_Item		History_Item;
typedef struct _History_Entry		History_Entry;
typedef struct _History_Types		History_Types;
typedef struct _Evry_State	        Evry_State;
typedef struct _Evry_View	        Evry_View;

typedef unsigned int Evry_Type;

struct _Evry_Item
{
  /* label to show for this item (stringshared) */
  const char *label;

  /* optional: (stringshared) more information to be shown */
  const char *detail;

  /* optional: (stringshared) fdo icon name, otherwise use _icon_get */
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

  /* is set to type of Evry_Plugin by default */
  Evry_Type type;

  /* optional */
  Evry_Type subtype;
  
  Evas_Object *(*icon_get) (Evry_Item *it, Evas *e);
  void (*free) (Evry_Item *it);

  /* do not set! */
  int ref;
  Eina_List *items;
  Eina_Bool selected;
  Eina_Bool marked;
  Evry_Plugin *plugin;
  double usage;
  History_Item *hi;
};

struct _Evry_Action
{
  Evry_Item base;

  /* identifier */
  const char *name;
  
  struct
  {
    /* requested type for action */
    Evry_Type type;
    Evry_Type subtype;
    /* handle multiple items */
    Eina_Bool accept_list;

    /* do not set ! */
    const Evry_Item *item;
    Eina_List *items;
  } it1;

  struct
  {
    Evry_Type type;
    Evry_Type subtype;
    Eina_Bool accept_list;

    /* do not set ! */
    const Evry_Item *item;
    Eina_List *items;
  } it2;


  /* optional: this action is specific for a item 'context'.
     e.g. 'copy' for file mime-type is not, 'image viewer' is.
     default is FALSE */
  Eina_Bool remember_context;

  /* required: do something */
  int  (*action)     (Evry_Action *act);

  /* optional: check whether action fits to chosen item */
  int  (*check_item) (Evry_Action *act, const Evry_Item *it);
  /* optional */
  void (*free)       (Evry_Action *act);
  /* optional: must be defined when  action is browseable, return
     list of Evry_Action items */
  Eina_List *(*fetch) (Evry_Action *act);
};

struct _Evry_Item_App
{
  Evry_Action base;
  const char *file;
  Efreet_Desktop *desktop;
};

struct _Evry_Item_File
{
  Evry_Item base;
  /* path and url must always be checked with
     evry_file_path/uri_get before use !!! */
  const char *url;
  const char *path;
  const char *mime;

  unsigned int modified;
};

struct _Evry_Plugin
{
  Evry_Item base;

  /* identifier */
  const char *name;

  /* list of items visible for everything after fetch */
  Eina_List *items;

  /* required: get candidates matching string, fill 'items' list */
  int  (*fetch) (Evry_Plugin *p, const char *input);

  /* required: run when state is removed in which this plugin is
     active. free 'items' here */
  void (*finish) (Evry_Plugin *p);

  /* plugin is added to the list of current plugins and
     queried for results when not returning NULL. The previous
     selectors item is passed, i.e. a plugin registered as action
     receives the subject, a plugin registered as object receives the
     action item. here you can check wheter the plugin can be queried,
     for given context (provided by item) */
  Evry_Plugin *(*begin) (Evry_Plugin *p, const Evry_Item *item);

  /* optional: provide a list of subitems of 'item'. this function
     must return a new instance which must be freed in 'finish' */
  Evry_Plugin *(*browse) (Evry_Plugin *p, const Evry_Item *item);

  /* optional: try to complete current item:
     return: EVRY_COMPLETE_INPUT when input was changed
     return: EVRY_COMPLETE_BROWSE to browse item */
  int  (*complete) (Evry_Plugin *p, const Evry_Item *item, char **input);

  /* optional: handle key events: return positive when key was
     handled */
  int  (*cb_key_down)  (Evry_Plugin *p, const Ecore_Event_Key *ev);

  /* optional: use this when begin returned a new instance or you
     have extended plugin struct */
  void (*free) (Evry_Plugin *p);

  /* optiona: actions only used with this plugin, dont require */
  Eina_List *actions;
  
  /* optional: set type which the plugin can handle in begin */
  Evry_Type input_type;

  /* optional: whether the plugin uses evry_async_update to add new items */
  /* default FALSE */
  Eina_Bool async_fetch;

  /* optional: request items to be remembered for usage statistic */
  /* default TRUE */
  Eina_Bool history;

  /* optional: if transient, item is removed from history on cleanup */
  /* default FALSE */
  Eina_Bool transient;

  /* optional: config path registered for the module, to show
     'configure' button in everything config */
  const char *config_path;

  /* set theme file to fetch icons from */
  const char *theme_path;

  Evry_View *view;
  
  /* not to be set by plugin! */
  Plugin_Config *config;
  unsigned int request;
  Evry_State *state;
};

struct _Plugin_Config
{
  /* do not set! */
  const char *name;
  int enabled;

  /* request initial sort order of this plugin */
  int priority;

  /* trigger to show plugin exclusively */
  const char *trigger;

  /* only show plugin when triggered */
  int trigger_only;

  /* preffered view mode */
  int view_mode;

  /* minimum input char to start query items,
     this must be handled by plugin */
  int min_query;

  /* show items of plugin in aggregator */
  int aggregate;

  /* if not top-level the plugin is shown in aggregator
     instead of the items  */
  int top_level;

  /* Eina_Hash *settings; */

  /* do not set! */
  Evry_Plugin *plugin;

  Eina_List *plugins;
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

  Evry_State *state;
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
  const char *data;
};

struct _History_Entry
{
  Eina_List *items;
};

struct _History_Types
{
  Eina_Hash *types;
};

#endif
