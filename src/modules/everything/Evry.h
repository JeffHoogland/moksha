#include "e.h"

#define EVRY_ACTION_OTHER 0
#define EVRY_ACTION_FINISHED 1
#define EVRY_ACTION_CONTINUE 2

#define EVRY_ASYNC_UPDATE_ADD 0
#define EVRY_ASYNC_UPDATE_CLEAR 1


typedef struct _Evry_Plugin Evry_Plugin;
typedef struct _Evry_Item   Evry_Item;
typedef struct _Evry_Action Evry_Action;
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
  Evry_Plugin *plugin;

  const char *label;

  const char *uri;
  const char *mime;

  Eina_Bool browseable;

  /* these are only for internally use by plugins */
  /* used e.g. as pointer for item data (Efreet_Desktop) or */
  /* for internal stuff, like priority hints for sorting, etc */
  void *data[4];
  int priority;

  /* not to be set by plugin! */
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

  /* sync/async ?*/
  Eina_Bool async_query;

  /* whether candidates can be shown without input
   * e.g.  borders, app history */
  Eina_Bool need_query;

  Eina_Bool browseable;

  /* run when plugin is activated. */
  int (*begin) (Evry_Plugin *p, const Evry_Item *item);


  int (*browse) (Evry_Plugin *p, const Evry_Item *item);

  /* get candidates matching string, fills 'candidates' list */
  int  (*fetch) (Evry_Plugin *p, const char *input);

  /* run before new query and when hiding 'everything' */
  void (*cleanup) (Evry_Plugin *p);

  Evas_Object *(*icon_get) (Evry_Plugin *p, const Evry_Item *it, Evas *e);
  /* provide more information for a candidate */
  /* int (*candidate_info) (Evas *evas, Evry_Item *item); */

  /* optional: default action for this plugins items */
  int  (*action) (Evry_Plugin *p, const Evry_Item *item, const char *input);
  Evry_Action *act;

  /* optional: create list of items when shown (e.g. for sorting) */
  void (*realize_items) (Evry_Plugin *p, Evas *e);

  Eina_List *items;

  Evas_Object *(*config_page) (void);
  void (*config_apply) (void);

  /* only for internal use by plugin */
  void *private;

  /* not to be set by plugin! */
  Evas_Object *tab;
  Plugin_Config *config;
};

struct _Evry_Action
{
  const char *name;

  const char *type_in1;
  const char *type_in2;
  const char *type_out;

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

void evry_plugin_register(Evry_Plugin *p);
void evry_plugin_unregister(Evry_Plugin *p);
void evry_action_register(Evry_Action *act);
void evry_action_unregister(Evry_Action *act);

Evry_Item *evry_item_new(Evry_Plugin *p, const char *label, void (*cb_free) (Evry_Item *item));
void evry_item_free(Evry_Item *it);
void evry_plugin_async_update(Evry_Plugin *plugin, int state);
void evry_clear_input(void);
int  evry_icon_theme_set(Evas_Object *obj, const char *icon);
