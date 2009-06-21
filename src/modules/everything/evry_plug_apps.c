#include "e.h"
#include "e_mod_main.h"

typedef struct _Inst Inst;

struct _Inst
{
  Eina_Hash *added;

  Eina_List *apps;
  Evry_Item *candidate;
};

static Evry_Plugin *_plug_new();
static Evry_Plugin *_plug_new2();
static void _plug_free(Evry_Plugin *p);
static int  _begin(Evry_Plugin *p, Evry_Item *item);
static int  _fetch(Evry_Plugin *p, const char *input);
static int  _action(Evry_Plugin *p, Evry_Item *item, const char *input);
static void _cleanup(Evry_Plugin *p);
static void _item_add(Evry_Plugin *p, Efreet_Desktop *desktop, char *file, int prio);
static int  _cb_sort(const void *data1, const void *data2);
static void _item_icon_get(Evry_Plugin *p, Evry_Item *it, Evas *e);

static Evry_Plugin_Class class;
static Evry_Plugin_Class class2;

EAPI int
evry_plug_apps_init(void)
{
   class.name = "Applications";
   class.type_in  = "NONE";
   class.type_out = "APPLICATION";
   class.need_query = 0;
   class.new = &_plug_new;
   class.free = &_plug_free;
   class.prio = 1;
   evry_plugin_register(&class);

   class2.name = "Open With..";
   class2.type_in  = "FILE";
   class2.type_out = "APPLICATION";
   class2.need_query = 0;
   class2.new = &_plug_new2;
   class2.free = &_plug_free;
   class2.prio = 3;
   evry_plugin_register(&class2);

   return 1;
}

EAPI int
evry_plug_apps_shutdown(void)
{
   evry_plugin_unregister(&class);
   evry_plugin_unregister(&class2);
   
   return 1;
}

static Evry_Plugin *
_plug_new()
{
   Evry_Plugin *p = E_NEW(Evry_Plugin, 1);
   p->class = &class;
   p->fetch = &_fetch;
   p->action = &_action;
   p->cleanup = &_cleanup;
   p->icon_get = &_item_icon_get;
   p->items = NULL;   

   Inst *inst = E_NEW(Inst, 1);
   inst->added = NULL;
   p->priv = inst;

   return p;
}

static Evry_Plugin *
_plug_new2()
{
   Evry_Plugin *p = E_NEW(Evry_Plugin, 1);
   p->class = &class2;
   p->begin = &_begin;
   p->fetch = &_fetch;
   p->action = &_action;
   p->cleanup = &_cleanup;
   p->icon_get = &_item_icon_get;
   p->items = NULL;   

   Inst *inst = E_NEW(Inst, 1);
   inst->added = NULL;
   p->priv = inst;

   return p;
}

static void
_plug_free(Evry_Plugin *p)
{
   Inst *inst = p->priv;
   
   _cleanup(p);
   if (inst->apps) eina_list_free(inst->apps);
   
   E_FREE(inst);
   E_FREE(p);
}

static int
_begin(Evry_Plugin *p, Evry_Item *it)
{
   Inst *inst;
   const char *mime;
   
   _cleanup(p);

   if (!it || !it->uri) return 0;
   inst = p->priv;
   inst->candidate = it;
   
   if (!it->mime)
     mime = efreet_mime_type_get(it->uri);
   else
     mime = it->mime;
   
   if (!mime) return 0;

   /* if (!strcmp(mime, "Folder"))
    *   {
    * 	apps = 
    *   }
    * else */

   inst->apps = efreet_util_desktop_mime_list(mime);
   
   return 1;
}


static int
_action(Evry_Plugin *p, Evry_Item *it, const char *input)
{
   E_Zone *zone;
   Evry_App *app;
   Efreet_Desktop *desktop = NULL; 
   Eina_List *files = NULL;
   Inst *inst = p->priv;
   int ret = 0;

   app = it->data[0];
   zone = e_util_zone_current_get(e_manager_current_get());

   if (inst->candidate)
     files = eina_list_append(files, inst->candidate->uri);
   
   if (app->desktop)
     {
	desktop = app->desktop;
     }
   else if (input || app->file)
     {
	if (app->file)
	  input = app->file;
	
	desktop = efreet_desktop_empty_new("");
	if (strchr(input, '%'))
	  {
	     desktop->exec = strdup(input);
	  }
	else
	  {
	     int len = strlen(input) + 4;
	     desktop->exec = malloc(len);
	     if (desktop->exec)
	       snprintf(desktop->exec, len, "%s %%U", input);
	  }
     }

   if (desktop)
     {
	e_exec(zone, desktop, NULL, files, NULL /*"everything"*/);

	if (!it)
	  efreet_desktop_free(desktop);
	
	ret = 1;
     }
   
   eina_list_free(files);

   
   /* if (app->desktop)
    *   e_exec(zone, app->desktop, NULL, NULL, "everything");
    * else
    *   e_exec(zone, NULL, app->file, NULL, "everything"); */

   return ret;
}

static void
_cleanup(Evry_Plugin *p)
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
_fetch(Evry_Plugin *p, const char *input)
{
   Eina_List *l;
   Efreet_Desktop *desktop;
   char *file;
   char match1[4096];
   char match2[4096];
   
   Inst *inst = p->priv;
   
   _cleanup(p); 

   if (inst->apps)
     {
	if (!input)
	  {
	     EINA_LIST_FOREACH(inst->apps, l, desktop)
	       _item_add(p, desktop, NULL, 1);
	  }
	else
	  {
	     snprintf(match1, sizeof(match1), "%s*", input);
	     snprintf(match2, sizeof(match2), "*%s*", input);

	     EINA_LIST_FOREACH(inst->apps, l, desktop)
	       {
		  if (desktop->name)
		    {		  
		       if (e_util_glob_case_match(desktop->name, match1))
			 _item_add(p, desktop, NULL, 1);
		       else if (e_util_glob_case_match(desktop->name, match2))
			 _item_add(p, desktop, NULL, 2);
		       else if (desktop->comment)
			 {
			    if (e_util_glob_case_match(desktop->comment, match1))
			      _item_add(p, desktop, NULL, 3);
			    else if (e_util_glob_case_match(desktop->comment, match2))
			      _item_add(p, desktop, NULL, 4);
			 }
		    }
	       }
	  }
     }
   
   
   if (!p->items && input)
     {
	snprintf(match1, sizeof(match1), "%s*", input);
	l = efreet_util_desktop_exec_glob_list(match1);
	EINA_LIST_FREE(l, desktop)
	  _item_add(p, desktop, NULL, 1);
   
	snprintf(match1, sizeof(match1), "*%s*", input);
	l = efreet_util_desktop_name_glob_list(match1);
	EINA_LIST_FREE(l, desktop)
	  _item_add(p, desktop, NULL, 2); 

	// TODO make these optional/configurable
	l = efreet_util_desktop_generic_name_glob_list(match1);
	EINA_LIST_FREE(l, desktop)
	  _item_add(p, desktop, NULL, 3); 

	l = efreet_util_desktop_comment_glob_list(match1);
	EINA_LIST_FREE(l, desktop)
	  _item_add(p, desktop, NULL, 3);
     }
   else if (!p->items)
     {
	// TODO option for popular/recent
	l = e_exehist_list_get();
	EINA_LIST_FREE(l, file)
	  _item_add(p, NULL, file, 1);
     }
   
   if (inst->added)
     {
	eina_hash_free(inst->added);
	inst->added = NULL;
     }
   
   if (p->items)
     {
	if (input)
	  p->items = eina_list_sort(p->items,
			   eina_list_count(p->items),
			   _cb_sort);
	return 1;
     }
    
   return 0;
}

static void
_item_add(Evry_Plugin *p, Efreet_Desktop *desktop, char *file, int prio)
{
   Evry_Item *it;
   Evry_App *app;
   Inst *inst = p->priv;
   Efreet_Desktop *desktop2;
   
   if (desktop)
     file = ecore_file_app_exe_get(desktop->exec);

   if (!file) return;

   if (desktop)
     {
	if ((desktop2 = eina_hash_find(inst->added, file)))
	  {
	     if (desktop == desktop2)
	       {
		  free(file);
		  return;
	       }
	  }
     }
   
   if (!inst->added)
     inst->added = eina_hash_string_superfast_new(NULL);

   eina_hash_add(inst->added, file, desktop);

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
_item_icon_get(Evry_Plugin *p, Evry_Item *it, Evas *e)
{
   Evry_App *app = it->data[0];
   
   if (app->desktop)
     it->o_icon = e_util_desktop_icon_add(app->desktop, 24, e);

   if (!it->o_icon)
     {
	it->o_icon = edje_object_add(e);
	/* e_util_icon_theme_set(it->o_icon, "system-run") */
	e_theme_edje_object_set(it->o_icon, "base/theme/fileman", "e/icons/system-run");
     }
}

static int
_cb_sort(const void *data1, const void *data2)
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
