#include "e.h"
#include "e_mod_main.h"

typedef struct _Inst Inst;

struct _Inst
{
  Eina_Hash *added;
};

static Evry_Plugin *_plug_apps_new();
static void _plug_apps_free(Evry_Plugin *p);
static int  _plug_apps_fetch(Evry_Plugin *p, char *string);
static int  _plug_apps_action(Evry_Plugin *p, Evry_Item *item);
static void _plug_apps_cleanup(Evry_Plugin *p);
static void _plug_apps_item_add(Evry_Plugin *p, Efreet_Desktop *desktop, char *file, int prio);
static int  _plug_apps_cb_sort(const void *data1, const void *data2);
static void _plug_apps_item_icon_get(Evry_Plugin *p, Evry_Item *it, Evas *e);

static Evry_Plugin_Class class;

EAPI int
evry_plug_apps_init(void)
{
   class.name = "Applications";
   class.type_in  = "NONE";
   class.type_out = "APPLICATION";
   class.need_query = 0;
   class.new = &_plug_apps_new;
   class.free = &_plug_apps_free;
   evry_plugin_register(&class);
   
   return 1;
}

EAPI int
evry_plug_apps_shutdown(void)
{
   evry_plugin_unregister(&class);
   
   return 1;
}

static Evry_Plugin *
_plug_apps_new()
{
   Evry_Plugin *p = E_NEW(Evry_Plugin, 1);
   p->class = &class;
   p->fetch = &_plug_apps_fetch;
   p->action = &_plug_apps_action;
   p->cleanup = &_plug_apps_cleanup;
   p->icon_get = &_plug_apps_item_icon_get;
   p->items = NULL;   

   Inst *inst = E_NEW(Inst, 1);
   inst->added = NULL;
   p->priv = inst;

   return p;
}

static void
_plug_apps_free(Evry_Plugin *p)
{
   Inst *inst = p->priv;
   
   _plug_apps_cleanup(p);

   E_FREE(inst);
   E_FREE(p);
}

static int
_plug_apps_action(Evry_Plugin *p, Evry_Item *item)
{
   E_Zone *zone;
   Evry_App *app;

   app = item->data[0];
   zone = e_util_zone_current_get(e_manager_current_get());

   if (app->desktop)
     e_exec(zone, app->desktop, NULL, NULL, "everything");
   else
     e_exec(zone, NULL, app->file, NULL, "everything");

   return 1;
}

static void
_plug_apps_cleanup(Evry_Plugin *p)
{
   Evry_Item *it;
   Evry_App *app;
   
   EINA_LIST_FREE(p->items, it)
     {
	if (it->label) eina_stringshare_del(it->label);
	if (it->o_icon) evas_object_del(it->o_icon);
	app = it->data[0];
	free(app);
	free(it);
     }
}

static int
_plug_apps_fetch(Evry_Plugin *p, char *string)
{
   char buf[4096];
   Eina_List *list;
   Efreet_Desktop *desktop;
   char *file;
   Inst *inst = p->priv;
   
   _plug_apps_cleanup(p); 

   if (string)
     {
	snprintf(buf, sizeof(buf), "%s*", string);
	list = efreet_util_desktop_exec_glob_list(buf);
	EINA_LIST_FREE(list, desktop)
	  _plug_apps_item_add(p, desktop, NULL, 1);
   
	snprintf(buf, sizeof(buf), "*%s*", string);
	list = efreet_util_desktop_name_glob_list(buf);
	EINA_LIST_FREE(list, desktop)
	  _plug_apps_item_add(p, desktop, NULL, 2); 

	// TODO make these optional/configurable
	snprintf(buf, sizeof(buf), "*%s*", string);
	list = efreet_util_desktop_generic_name_glob_list(buf);
	EINA_LIST_FREE(list, desktop)
	  _plug_apps_item_add(p, desktop, NULL, 3); 

	snprintf(buf, sizeof(buf), "*%s*", string);
	list = efreet_util_desktop_comment_glob_list(buf);
	EINA_LIST_FREE(list, desktop)
	  _plug_apps_item_add(p, desktop, NULL, 3);
     }
   else
     {
	// TODO option for popular/recent
	list = e_exehist_list_get();
	EINA_LIST_FREE(list, file)
	  _plug_apps_item_add(p, NULL, file, 1);
     }
   
   if (inst->added)
     {
	eina_hash_free(inst->added);
	inst->added = NULL;

	if (string)
	  p->items = eina_list_sort(p->items,
			   eina_list_count(p->items),
			   _plug_apps_cb_sort);
	return 1;
     }
    
   return 0;
}

static void
_plug_apps_item_add(Evry_Plugin *p, Efreet_Desktop *desktop, char *file, int prio)
{
   Evry_Item *it;
   Evry_App *app;
   Inst *inst = p->priv;
   
   if (desktop)
     file = ecore_file_app_exe_get(desktop->exec);

   if (!file) return;
   
   if (eina_hash_find(inst->added, file))
     {
	if (desktop) free(file);
	return;
     }
   
   if (!inst->added)
     inst->added = eina_hash_string_superfast_new(NULL);

   eina_hash_add(inst->added, file, file);

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
   it->data[0] = app;
   it->priority = prio;
   if (desktop)
     it->label = eina_stringshare_add(desktop->name);
   else
     it->label = eina_stringshare_add(file);
   it->o_icon = NULL;

   p->items = eina_list_append(p->items, it);
}

static void
_plug_apps_item_icon_get(Evry_Plugin *p, Evry_Item *it, Evas *e)
{
   Evry_App *app = it->data[0];
   
   if (app->desktop)
     it->o_icon = e_util_desktop_icon_add(app->desktop, 24, e);
}

static int
_plug_apps_cb_sort(const void *data1, const void *data2)
{
   const Evry_Item *it1, *it2;
   Evry_App *app1, *app2;
   const char *e1, *e2;
   double t1, t2;
   
   it1 = data1;
   it2 = data2;
   app1 = it1->data[0];
   app2 = it2->data[0];
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
