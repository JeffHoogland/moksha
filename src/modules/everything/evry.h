/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef struct _Evry_Plugin Evry_Plugin;
/* TODO find a better name - Registry ? */
typedef struct _Evry_Plugin_Class Evry_Plugin_Class;
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

  const char *uri;
  const char *mime;

  /* set by icon_get plugin method */
  Evas_Object *o_icon;

  /* used by 'everything' for display */
  Evas_Object *o_bg;  
  
  /* these are only for internally use by plugins */
  /* used e.g. as pointer for item data (Efreet_Desktop) or */
  /* for internal stuff, like priority hints for sorting, etc */
  void *data[4];
  int priority;
};

struct _Evry_Plugin_Class
{
  const char *name;

  const char *type_in;
  const char *type_out;

  /* TODO option */
  int prio;
  
  /* sync/async ?*/
  unsigned char async_query : 1;

  /* whether candidates can be shown without input: e.g. borders, history */
  /* if 0 fetch MUST provide all candidates when string is NULL */
  unsigned char need_query : 1; 

  Evry_Plugin *(*new) (void);
  void (*free) (Evry_Plugin *p);
  
  Evas_Object *(*config_page) (void);
  void (*config_apply) (void);

  Eina_List *instances;
};

  
struct _Evry_Plugin
{
  Evry_Plugin_Class *class;
  
  /* run when plugin is activated. */
  int (*begin) (Evry_Plugin *p, Evry_Item *item);

  /* get candidates matching string, fills 'candidates' list */
  int  (*fetch) (Evry_Plugin *p, const char *input);

  /* run action with a given candidate - TODO register actions per
     candidate type */
  int  (*action) (Evry_Plugin *p, Evry_Item *item, const char *input);

  /* run before new query and when hiding 'everything' */
  void (*cleanup) (Evry_Plugin *p);

  void (*icon_get) (Evry_Plugin *p, Evry_Item *it, Evas *e);  
  /* provide more information for a candidate */
  /* int (*candidate_info) (Evas *evas, Evry_Item *item); */

  Eina_List *items;

  Evas_Object *tab;
  
  void *priv;
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

EAPI void evry_plugin_register(Evry_Plugin_Class *pc);
EAPI void evry_plugin_unregister(Evry_Plugin_Class *pc);

EAPI void evry_plugin_async_update(Evry_Plugin *plugin, int state);

EAPI Evas* evry_evas_get(void);
#endif
#endif

