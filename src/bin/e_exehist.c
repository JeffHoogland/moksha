/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* currently default bind is alt+` buf alt+space has been suggested */

/* local subsystem functions */
typedef struct _E_Exehist E_Exehist;
typedef struct _E_Exehist_Item E_Exehist_Item;

struct _E_Exehist
{
   Evas_List *history;
};

struct _E_Exehist_Item
{
   const char   *exe;
   const char   *launch_method;
   double        exetime;
};

static void _e_exehist_unload_queue(void);
static void _e_exehist_load(void);
static void _e_exehist_clear(void);
static void _e_exehist_unload(void);
static void _e_exehist_limit(void);
static int _e_exehist_cb_unload(void *data);

/* local subsystem globals */
static E_Config_DD *_e_exehist_config_edd = NULL;
static E_Config_DD *_e_exehist_config_item_edd = NULL;
static E_Exehist *_e_exehist = NULL;
static Ecore_Timer *_e_exehist_unload_timer = NULL;

/* externally accessible functions */
EAPI int
e_exehist_init(void)
{
   _e_exehist_config_item_edd = E_CONFIG_DD_NEW("E_Exehist_Item", E_Exehist_Item);
#undef T
#undef D
#define T E_Exehist_Item
#define D _e_exehist_config_item_edd
   E_CONFIG_VAL(D, T, exe, STR);
   E_CONFIG_VAL(D, T, launch_method, STR);
   E_CONFIG_VAL(D, T, exetime, DOUBLE);
   
   _e_exehist_config_edd = E_CONFIG_DD_NEW("E_Exehist", E_Exehist);
#undef T
#undef D
#define T E_Exehist
#define D _e_exehist_config_edd
   E_CONFIG_LIST(D, T, history, _e_exehist_config_item_edd);
   return 1;
}

EAPI int
e_exehist_shutdown(void)
{
   _e_exehist_unload();
   E_CONFIG_DD_FREE(_e_exehist_config_item_edd);
   E_CONFIG_DD_FREE(_e_exehist_config_edd);
   return 1;
}

EAPI void
e_exehist_add(const char *launch_method, const char *exe)
{
   E_Exehist_Item *ei;

   _e_exehist_load();
   if (!_e_exehist) return;
   ei = E_NEW(E_Exehist_Item, 1);
   if (!ei) return;
   ei->launch_method = evas_stringshare_add(launch_method);
   ei->exe = evas_stringshare_add(exe);
   ei->exetime = ecore_time_get();
   _e_exehist->history = evas_list_append(_e_exehist->history, ei);
   _e_exehist_limit();
   e_config_domain_save("exehist", _e_exehist_config_edd, _e_exehist);
   _e_exehist_unload_queue();
}

EAPI void
e_exehist_clear(void)
{
   _e_exehist_load();
   if (!_e_exehist) return;
   _e_exehist_clear();
   e_config_domain_save("exehist", _e_exehist_config_edd, _e_exehist);
   _e_exehist_unload_queue();
}

EAPI int
e_exehist_popularity_get(const char *exe)
{
   Evas_List *l;
   int count = 0;
   
   _e_exehist_load();
   if (!_e_exehist) return 0;
   for (l = _e_exehist->history; l; l = l->next)
     {
	E_Exehist_Item *ei;
	
	ei = l->data;
	if ((ei->exe) && (!strcmp(exe, ei->exe))) count++;
     }
   return count;
}

EAPI double
e_exehist_newest_run_get(const char *exe)
{
   Evas_List *l;
   
   _e_exehist_load();
   if (!_e_exehist) return 0.0;
   for (l = evas_list_last(_e_exehist->history); l; l = l->prev)
     {
	E_Exehist_Item *ei;
	
	ei = l->data;
	if ((ei->exe) && (!strcmp(exe, ei->exe))) return ei->exetime;
     }
   return 0.0;
}

/* local subsystem functions */
static void
_e_exehist_unload_queue(void)
{
   if (_e_exehist_unload_timer) ecore_timer_del(_e_exehist_unload_timer);
   _e_exehist_unload_timer = ecore_timer_add(2.0, _e_exehist_cb_unload, NULL);
}

static void
_e_exehist_load(void)
{
   if (!_e_exehist)
     _e_exehist = e_config_domain_load("exehist", _e_exehist_config_edd);
   if (!_e_exehist)
     _e_exehist = E_NEW(E_Exehist, 1);
}

static void
_e_exehist_clear(void)
{
   if (_e_exehist)
     {
	while (_e_exehist->history)
	  {
	     E_Exehist_Item *ei;
	     
	     ei = _e_exehist->history->data;
	     if (ei->exe) evas_stringshare_del(ei->exe);
	     if (ei->launch_method) evas_stringshare_del(ei->launch_method);
	     free(ei);
	     _e_exehist->history = evas_list_remove_list(_e_exehist->history, _e_exehist->history);
	  }
     }
}

static void
_e_exehist_unload(void)
{
   _e_exehist_clear();
   E_FREE(_e_exehist);
}

static void
_e_exehist_limit(void)
{
   /* go from first item in hist on and either delete all items before a
    * specific timestamp, or if the list count > limit then delete items
    */
}

static int
_e_exehist_cb_unload(void *data)
{
   _e_exehist_unload();
   _e_exehist_unload_timer = NULL;
   return 0;
}
