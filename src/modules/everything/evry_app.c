#include "e.h"
#include "e_mod_main.h"

static Eina_Hash *added = NULL;
static Evry_Plugin plugin;

static int  _evry_app_fetch(char *string);
static int  _evry_app_action(Evry_Item *item);
static void _evry_app_cleanup(void);
static void _evry_app_item_add(Efreet_Desktop *desktop, char *file, int prio);
static int  _evry_app_cb_sort(const void *data1, const void *data2);
static void _evry_app_item_icon_get(Evry_Item *it, Evas *e);


EAPI int
evry_app_init(void)
{
   plugin.name = "Applications";
   plugin.type = "APPLICATION";
   plugin.need_query = 0;
   plugin.fetch  = &_evry_app_fetch;
   plugin.action = &_evry_app_action;
   plugin.cleanup = &_evry_app_cleanup;
   plugin.icon_get = &_evry_app_item_icon_get;
   plugin.candidates = NULL;   

   evry_plugin_add(&plugin);
   
   return 1;
}

EAPI int
evry_app_shutdown(void)
{
   evry_plugin_remove(&plugin);
   
   return 1;
}

static int
_evry_app_action(Evry_Item *item)
{
   E_Zone *zone;
   Evry_App *app;

   app = item->data;
   zone = e_util_zone_current_get(e_manager_current_get());

   if (app->desktop)
     e_exec(zone, app->desktop, NULL, NULL, "everything");
   else
     e_exec(zone, NULL, app->file, NULL, "everything");

   return 1;
}

static void
_evry_app_cleanup(void)
{
   Evry_Item *it;
   Evry_App *app;
   
   EINA_LIST_FREE(plugin.candidates, it)
     {
	if (it->label) eina_stringshare_del(it->label);
	if (it->o_icon) evas_object_del(it->o_icon);
	app = it->data;
	free(app);
	free(it);
     }
}

static int
_evry_app_fetch(char *string)
{
   char buf[4096];
   Eina_List *list;
   Efreet_Desktop *desktop;
   char *file;
   
   _evry_app_cleanup(); 

   if (string)
     {
	snprintf(buf, sizeof(buf), "%s*", string);
	list = efreet_util_desktop_exec_glob_list(buf);
	EINA_LIST_FREE(list, desktop)
	  _evry_app_item_add(desktop, NULL, 1);
   
	snprintf(buf, sizeof(buf), "*%s*", string);
	list = efreet_util_desktop_name_glob_list(buf);
	EINA_LIST_FREE(list, desktop)
	  _evry_app_item_add(desktop, NULL, 2); 

	// TODO make these optional/configurable
	snprintf(buf, sizeof(buf), "*%s*", string);
	list = efreet_util_desktop_generic_name_glob_list(buf);
	EINA_LIST_FREE(list, desktop)
	  _evry_app_item_add(desktop, NULL, 3); 

	snprintf(buf, sizeof(buf), "*%s*", string);
	list = efreet_util_desktop_comment_glob_list(buf);
	EINA_LIST_FREE(list, desktop)
	  _evry_app_item_add(desktop, NULL, 3);
     }
   else
     {
	// TODO option for popular/recent
	list = e_exehist_list_get();
	EINA_LIST_FREE(list, file)
	  _evry_app_item_add(NULL, file, 1);
     }
   
   if (added)
     {
	eina_hash_free(added);
	added = NULL;

	if (string)
	  plugin.candidates =
	    eina_list_sort(plugin.candidates,
			   eina_list_count(plugin.candidates),
			   _evry_app_cb_sort);
	return 1;
     }
    
   return 0;
}

static void
_evry_app_item_add(Efreet_Desktop *desktop, char *file, int prio)
{
   Evry_Item *it;
   Evry_App *app;

   if (desktop)
     file = ecore_file_app_exe_get(desktop->exec);

   if (!file) return;
   
   if (eina_hash_find(added, file))
     {
	if (desktop) free(file);
	return;
     }
   
   if (!added)
     added = eina_hash_string_superfast_new(NULL);

   eina_hash_add(added, file, file);

   if (desktop)
     {
	free(file);
	file = NULL;
     }
   else 
     {
	desktop = efreet_util_desktop_exec_find(file);
	if (desktop) file = NULL;
     }
   
   it = calloc(1, sizeof(Evry_Item));
   app = calloc(1, sizeof(Evry_App));
   app->desktop = desktop;
   app->file = file;
   it->data = app;
   it->priority = prio;
   if (desktop)
     it->label = eina_stringshare_add(desktop->name);
   else
     it->label = eina_stringshare_add(file);
   it->o_icon = NULL;

   plugin.candidates = eina_list_append(plugin.candidates, it);
}

static void
_evry_app_item_icon_get(Evry_Item *it, Evas *e)
{
   Evry_App *app = it->data;
   
   if (app->desktop)
     it->o_icon = e_util_desktop_icon_add(app->desktop, 24, e);
}

static int
_evry_app_cb_sort(const void *data1, const void *data2)
{
   const Evry_Item *it1, *it2;
   Evry_App *app1, *app2;
   const char *e1, *e2;
   double t1, t2;
   
   it1 = data1;
   it2 = data2;
   app1 = it1->data;
   app2 = it2->data;
   e1 = efreet_util_path_to_file_id(app1->desktop->orig_path);
   e2 = efreet_util_path_to_file_id(app2->desktop->orig_path);
   t1 = e_exehist_newest_run_get(e1);
   t2 = e_exehist_newest_run_get(e2);

   if ((int)(t1 - t2))
     return (int)(t1 - t2);
   else if (it1->priority - it2->priority)
     return (it1->priority - it2->priority);
   // TODO compare exe strings?
   else return 0;
   
}
