/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */

#include "e.h"

#ifndef E_MOD_MAIN_H
#define E_MOD_MAIN_H

#define E_TYPEDEFS 1
typedef struct _Evry_Plugin Evry_Plugin;
typedef struct _Evry_Item   Evry_Item;
typedef struct _Evry_Action Evry_Action;
typedef struct _Evry_App Evry_App;
#undef E_TYPEDEFS

typedef struct _Config Config;
typedef struct _Plugin_Config Plugin_Config;


#define EVRY_ACTION_OTHER 0
#define EVRY_ACTION_FINISHED 1
#define EVRY_ACTION_CONTINUE 2

#define EVRY_ASYNC_UPDATE_ADD 0
#define EVRY_ASYNC_UPDATE_CLEAR 1


struct _Config
{
  /* position */
  double rel_x, rel_y;
  /* size */
  int width, height;

  /* generic plugin config */
  Eina_List *plugins_conf;

  int scroll_animate;
  double scroll_speed;

  int auto_select_first;

  Eina_Hash *key_bindings;
  
  /**/
  Eina_List *plugins;

  Eina_Hash *history;
};

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
  const char *label;

  const char *uri;
  const char *mime;
  
  /* these are only for internally use by plugins */
  /* used e.g. as pointer for item data (Efreet_Desktop) or */
  /* for internal stuff, like priority hints for sorting, etc */
  void *data[4];
  int priority;

  /* not to be set by plugin! */
  Evas_Object *o_icon;
  Evas_Object *o_bg;  
};
  
struct _Evry_Plugin
{
  const char *name;

  const char *type_in;
  const char *type_out;

  const char *trigger;
  
  /* sync/async ?*/
  Eina_Bool async_query;

  /* whether candidates can be shown without input
   * e.g.  borders, app history */
  Eina_Bool need_query; 

  /* run when plugin is activated. */
  int (*begin) (Evry_Plugin *p, Evry_Item *item);

  /* get candidates matching string, fills 'candidates' list */
  int  (*fetch) (Evry_Plugin *p, const char *input);

  /* run before new query and when hiding 'everything' */
  void (*cleanup) (Evry_Plugin *p);

  /* TODO return icon */
  void (*icon_get) (Evry_Plugin *p, Evry_Item *it, Evas *e);  
  /* provide more information for a candidate */
  /* int (*candidate_info) (Evas *evas, Evry_Item *item); */

  /* optional: default action for this plugins items */
  int  (*action) (Evry_Plugin *p, Evry_Item *item, const char *input);

  /* optional: create list of items when shown (e.g. for sorting) */
  void (*realize_items) (Evry_Plugin *p, Evas *e);

  Eina_List *items;
  
  Evas_Object *(*config_page) (void);
  void (*config_apply) (void);

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

  Evry_Item *thing1;
  Evry_Item *thing2;
  
  int  (*action) (Evry_Action *act);

  int (*check_item) (Evry_Action *act, Evry_Item *it);  

  void (*icon_get) (Evry_Action *act, Evry_Item *it, Evas *e);  
  
  void *priv;

  /* not to be set by plugin! */
  Evas_Object *o_icon;
};  

struct _Evry_App
{
  const char *file;
  Efreet_Desktop *desktop;
};

EAPI extern E_Module_Api e_modapi;

EAPI void *e_modapi_init     (E_Module *m);
EAPI int   e_modapi_shutdown (E_Module *m);
EAPI int   e_modapi_save     (E_Module *m);
EAPI E_Config_Dialog *evry_config_dialog(E_Container *con, const char *params);

EAPI int  evry_init(void);
EAPI int  evry_shutdown(void);
EAPI int  evry_show(E_Zone *zone);
EAPI void evry_hide(void);
EAPI void evry_plugin_register(Evry_Plugin *p);
EAPI void evry_plugin_unregister(Evry_Plugin *p);
EAPI void evry_action_register(Evry_Action *act);
EAPI void evry_action_unregister(Evry_Action *act);
EAPI void evry_plugin_async_update(Evry_Plugin *plugin, int state);
EAPI void evry_clear_input(void);

extern Config *evry_conf;

#endif
