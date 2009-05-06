/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef struct _Evry_Plugin Evry_Plugin;
typedef struct _Evry_Item   Evry_Item;
typedef struct _Evry_Action Evry_Action;
typedef struct _Evry_Config Evry_Config;
typedef struct _Evry_App Evry_App;

/* typedef enum _Evry_Plugin_Type
 *   {
 *     EVRY_SYNC,
 *     EVRY_ASYNC,
 *   } Evry_Plugin_Type; */

/* typedef enum _Evry_Plugin_State
 * {
 *   EVRY_STATE_NO_CANDIDATES,
 *   EVRY_STATE_PENDING,
 *   EVRY_STATE_FINISHED,
 * } Evry_Plugin_State; */

#else
#ifndef EVRY_H
#define EVRY_H

struct _Evry_Item
{
  const char *label;
  Evas_Object *o_icon;
  unsigned int type;  /* TODO */

  /* used by 'everything' for display */
  Evas_Object *o_bg;  
  
  /* these are only for internally use by plugins */
  /* used e.g. as pointer for item data (Efreet_Desktop) or */
  /* for internal stuff, like priority hints for sorting, etc */
  void *data;
  int priority;
};

struct _Evry_Plugin
{
  const char *name;
  const char *type;
  Evas_Object *o_icon;
  
  /* sync/async ?*/
  unsigned int async_query;

  /* whether candidates can be shown without input: e.g. borders, history */
  /* if 0 fetch MUST provide all candidates when string is NULL */
  // TODO better use 'need_query_length' ?
  unsigned int need_query; 

  /* run when 'everything' is shown */
  void (*begin) (void);

  /* get candidates matching string, fills 'candidates' list */
  int  (*fetch)  (char *string);

  /* run action with a given candidate - TODO register actions per
     candidate type */
  int  (*action) (Evry_Item *item);

  /* run before new query and when hiding 'everything' */
  void (*cleanup) (void);

  void (*icon_get) (Evry_Item *it, Evas *e);  
  /* provide more information for a candidate */
  /* int (*candidate_info) (Evas *evas, Evry_Item *item); */

  Evas_Object *(*config_page) (void);
  void (*config_apply) (void);
  
  /* Evry_Plugin_State state; */
  Eina_List *candidates;
};

struct _Evry_Action
{
  const char *name;
  const char *type;
  int  (*func) (Evry_Item *item);
};
  

struct _Evry_Config
{
  Eina_List *plugin_order;
  
};


struct _Evry_App
{
  const char *file;
  Efreet_Desktop *desktop;
};


EAPI int evry_init(void);
EAPI int evry_shutdown(void);

EAPI int  evry_show(E_Zone *zone);
EAPI void evry_hide(void);

EAPI void evry_plugin_add(Evry_Plugin *plugin);
EAPI void evry_plugin_remove(Evry_Plugin *plugin);
EAPI void evry_action_add(Evry_Action *action);
EAPI void evry_action_remove(Evry_Action *action);

/* EAPI void evry_plugin_async_update(Evry_Plugin *plugin, int state); */

EAPI Evas* evry_evas_get(void);
#endif
#endif

