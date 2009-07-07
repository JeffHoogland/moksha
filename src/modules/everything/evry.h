/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */

#include "e_mod_main.h"

#ifdef E_TYPEDEFS



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


#define EVRY_ACTION_OTHER 0
#define EVRY_ACTION_FINISHED 1
#define EVRY_ACTION_CONTINUE 2

#define EVRY_ASYNC_UPDATE_ADD 0
#define EVRY_ASYNC_UPDATE_CLEAR 1

#else
#ifndef EVRY_H
#define EVRY_H

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

  /* TODO option */
  int prio;
  
  /* sync/async ?*/
  Eina_Bool async_query;

  /* whether candidates can be shown without input
   * e.g.  borders, app history */
  Eina_Bool need_query; 

  /* run when plugin is activated. */
  int (*begin) (Evry_Item *item);

  /* get candidates matching string, fills 'candidates' list */
  int  (*fetch) (const char *input);

  /* default action for this plugins items */
  int  (*action) (Evry_Item *item, const char *input);

  /* run before new query and when hiding 'everything' */
  void (*cleanup) (void);

  /* TODO return icon */
  void (*icon_get) (Evry_Item *it, Evas *e);  
  /* provide more information for a candidate */
  /* int (*candidate_info) (Evas *evas, Evry_Item *item); */

  Eina_List *items;
  
  Evas_Object *(*config_page) (void);
  void (*config_apply) (void);

  /* for internal use by plugin */
  void *priv;

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
  
  int  (*action) (void);

  void (*icon_get) (Evry_Plugin *p, Evry_Item *it, Evas *e);  
  
  void *priv;

  /* not to be set by plugin! */
  Evas_Object *o_icon;
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

EAPI void evry_plugin_register(Evry_Plugin *p);
EAPI void evry_plugin_unregister(Evry_Plugin *p);
EAPI void evry_action_register(Evry_Action *act);
EAPI void evry_action_unregister(Evry_Action *act);

EAPI void evry_plugin_async_update(Evry_Plugin *plugin, int state);

EAPI Evas* evry_evas_get(void);
#endif
#endif

