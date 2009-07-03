#include "e.h"
#include "e_mod_main.h"

typedef struct _Inst Inst;

struct _Inst
{
  Eina_Hash *added;
  Eina_List *apps;
  Evry_Item *candidate;
};

static int  _begin(Evry_Item *item);
static int  _fetch(const char *input);
static int  _action(Evry_Item *item, const char *input);
static void _cleanup(void);
static void _item_add(Efreet_Desktop *desktop, char *file, int prio);
static int  _cb_sort(const void *data1, const void *data2);
static void _item_icon_get(Evry_Item *it, Evas *e);
static int _exec_action(void);

static Evry_Plugin *p;
static Evry_Action *act;
static Inst *inst;



EAPI int
evry_plug_apps_init(void)
{
   p = E_NEW(Evry_Plugin, 1);
   p->type_in  = "NONE|FILE";
   p->type_out = "NONE";
   p->need_query = 0;
   p->begin = &_begin;
   p->fetch = &_fetch;
   p->action = &_action;
   p->cleanup = &_cleanup;
   p->icon_get = &_item_icon_get;
   evry_plugin_register(p);

   act = E_NEW(Evry_Action, 1);
   act->name = "Open File...";
   act->type_in1 = "APPLICATION";
   act->type_in2 = "FILE";
   act->type_out = "NONE";
   act->action =_exec_action;
   evry_action_register(act);

   inst = NULL;
   
   return 1;
}

EAPI int
evry_plug_apps_shutdown(void)
{
   evry_plugin_unregister(p);
   evry_action_unregister(act);

   return 1;
}

static int
_begin(Evry_Item *it)
{
   const char *mime;
   
   if (inst) return 0;

   if (it)
     {
	p->name = "Open With...";
	p->type_out = "NONE";
	p->prio = 3;

	if (!it->uri) return 0;
   
	if (!it->mime)
	  mime = efreet_mime_type_get(it->uri);
	else
	  mime = it->mime;
   
	if (!mime) return 0;
	
	inst = E_NEW(Inst, 1);
	inst->candidate = it;
	inst->apps = efreet_util_desktop_mime_list(mime);
     }
   else
     {
	p->name = "Applications";
	p->type_out = "APPLICATION";
	p->prio = 1;
	
	inst = E_NEW(Inst, 1);
     }
   
   return 1;
}


static int
_action(Evry_Item *it, const char *input)
{
   E_Zone *zone;
   Evry_App *app = NULL;
   Efreet_Desktop *desktop = NULL; 
   Eina_List *files = NULL;

   if (it) app = it->data[0];
      
   if (app && app->desktop)
     {
	desktop = app->desktop;
     }
   else
     {
	if (app && app->file)
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
	if (inst->candidate)
	  files = eina_list_append(files, inst->candidate->uri);

	zone = e_util_zone_current_get(e_manager_current_get());
	e_exec(zone, desktop, NULL, files, NULL /*"everything"*/);

	if (!it)
	  efreet_desktop_free(desktop);

	eina_list_free(files);

	return EVRY_ACTION_FINISHED;
     }
   
   return EVRY_ACTION_CONTINUE;
}


static void
_list_free(void)
{
   Evry_Item *it;
   Evry_App *app;
   
   EINA_LIST_FREE(p->items, it)
     {
	if (it->label) eina_stringshare_del(it->label);
	app = it->data[0];
	E_FREE(app);
	E_FREE(it);
     }
}

static void
_cleanup(void)
{
   _list_free();

   if (inst)
     {
	eina_list_free(inst->apps);
	E_FREE(inst);
     }
   
   inst = NULL;
}


static int
_fetch(const char *input)
{
   Eina_List *l;
   Efreet_Desktop *desktop;
   char *file;
   char match1[4096];
   char match2[4096];
   
   _list_free(); 

   if (inst->apps)
     {
	if (!input)
	  {
	     EINA_LIST_FOREACH(inst->apps, l, desktop)
	       _item_add(desktop, NULL, 1);
	  }
	else
	  {
	     snprintf(match1, sizeof(match1), "%s*", input);
	     snprintf(match2, sizeof(match2), "*%s*", input);

	     EINA_LIST_FOREACH(inst->apps, l, desktop)
	       {
		  if (e_util_glob_case_match(desktop->exec, match1))
		    _item_add(desktop, NULL, 1);
		  else if (e_util_glob_case_match(desktop->exec, match2))
		    _item_add(desktop, NULL, 2);
		  else if (e_util_glob_case_match(desktop->name, match1))
		    _item_add(desktop, NULL, 1);
		  else if (e_util_glob_case_match(desktop->name, match2))
		    _item_add(desktop, NULL, 2);
		  else if (desktop->comment)
		    {
		       if (e_util_glob_case_match(desktop->comment, match1))
			 _item_add(desktop, NULL, 3);
		       else if (e_util_glob_case_match(desktop->comment, match2))
			 _item_add(desktop, NULL, 4);
		    }
	       }
	  }
     }
   
   
   if (!p->items && input)
     {
	snprintf(match1, sizeof(match1), "%s*", input);
	l = efreet_util_desktop_exec_glob_list(match1);
	EINA_LIST_FREE(l, desktop)
	  _item_add(desktop, NULL, 1);
   
	snprintf(match1, sizeof(match1), "*%s*", input);
	l = efreet_util_desktop_name_glob_list(match1);
	EINA_LIST_FREE(l, desktop)
	  _item_add(desktop, NULL, 2); 

	// TODO make these optional/configurable
	l = efreet_util_desktop_generic_name_glob_list(match1);
	EINA_LIST_FREE(l, desktop)
	  _item_add(desktop, NULL, 3); 

	l = efreet_util_desktop_comment_glob_list(match1);
	EINA_LIST_FREE(l, desktop)
	  _item_add(desktop, NULL, 3);
     }
   else if (!p->items)
     {
	// TODO option for popular/recent
	l = e_exehist_list_get();
	EINA_LIST_FREE(l, file)
	  _item_add(NULL, file, 1);
     }
   
   if (inst->added)
     {
	eina_hash_free(inst->added);
	inst->added = NULL;
     }
   
   if (p->items)
     {
	if (input)
	  p->items = eina_list_sort(p->items, eina_list_count(p->items), _cb_sort);
	return 1;
     }
    
   return 0;
}

static void
_item_add(Efreet_Desktop *desktop, char *file, int prio)
{
   Evry_Item *it;
   Evry_App *app;
   Efreet_Desktop *desktop2;

   if (desktop)
     {
	Eina_List *l;
	char *cat;

	/* ignore screensaver.. */
	EINA_LIST_FOREACH(desktop->categories, l, cat)
	  {
	     if (cat && !strcmp(cat, "Screensaver"))
	       return;
	  }
	
	file = desktop->exec;
     }
   
   if (!file) return;

   if (!inst->added)
     inst->added = eina_hash_string_superfast_new(NULL);


   if (!desktop)
   {
      char match[4096];
      Eina_List *l;
      int len;
      char *tmp;
      int found = 0;
	
      if (eina_hash_find(inst->added, file))
	return;

      len = strlen(file);
      tmp = ecore_file_app_exe_get(file);
      snprintf(match, sizeof(match), "%s*", tmp);	
      l = efreet_util_desktop_exec_glob_list(match);

      EINA_LIST_FREE(l, desktop)
	{
	   if (desktop->exec && !strncmp(file, desktop->exec, len))
	     {
		found = 1;
		break;
	     }
	}

      eina_list_free(l);
      free(tmp);

      if (!found)
	eina_hash_add(inst->added, file, file);
   }

   if (desktop)
     {
	if ((desktop2 = eina_hash_find(inst->added, file)))
	  if (desktop == desktop2)
	    return;

	eina_hash_add(inst->added, file, desktop);
	file = NULL;
     }
   
   it = E_NEW(Evry_Item, 1);
   app = E_NEW(Evry_App, 1);
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
_item_icon_get(Evry_Item *it, Evas *e)
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

static int
_exec_action(void)
{
   if (act->thing1 && act->thing2)
     {
	inst = E_NEW(Inst, 1);
	inst->candidate = act->thing2;

	_action(act->thing1, NULL);
	
	E_FREE(inst);
	inst = NULL;
	
	return 1;
     }
   
   return 0;
}
