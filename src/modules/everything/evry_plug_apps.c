#include "e.h"
#include "e_mod_main.h"

typedef struct _Inst Inst;

struct _Inst
{
  Eina_Hash *added;
  Eina_List *apps;
  Evry_Item *candidate;
};

static Evry_Plugin *p1;
static Evry_Plugin *p2;
static Evry_Action *act;
static Evry_Action *act1;
static Evry_Action *act2;
static Evry_Action *act3;
static Eina_List *exe_path = NULL;


static int
_begin(Evry_Plugin *p, Evry_Item *it)
{
   const char *mime;
   Inst *inst = NULL;
   
   if (it)
     {
	if (!it->uri) return 0;

	if (!it->mime)
	  mime = efreet_mime_type_get(it->uri);
	else
	  mime = it->mime;

	if (!mime) return 0;

	inst = E_NEW(Inst, 1);
	inst->candidate = it;

	if (strcmp(mime, "Folder"))
	  inst->apps = efreet_util_desktop_mime_list(mime);
	if (!inst->apps)
	  {
	     Efreet_Desktop *desktop;
	     desktop = e_exehist_mime_desktop_get(mime);
	     if (desktop)
	       inst->apps = eina_list_append(inst->apps, desktop);
	  }
     }

   p->private = inst;
   
   return 1;
}

static void
_list_free(Evry_Plugin *p)
{
   Evry_Item *it;
   Evry_App *app;

   EINA_LIST_FREE(p->items, it)
     {
	app = it->data[0];
	if (app->file) eina_stringshare_del(app->file);
	E_FREE(app);
	evry_item_free(it);
     }
}

static void
_cleanup(Evry_Plugin *p)
{
   Inst *inst = p->private;
   
   _list_free(p);

   if (inst)
     {
	if (inst->apps) eina_list_free(inst->apps);
	E_FREE(inst);
     }

   p->private = NULL;
}

static void
_item_add(Evry_Plugin *p, Efreet_Desktop *desktop, char *file, int prio)
{
   Evry_Item *it;
   Evry_App *app;
   Efreet_Desktop *desktop2;
   Inst *inst = p->private;
   
   if (desktop)
     file = desktop->exec;

   if (!file) return;
   
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
	  if (desktop->exec && !strncmp(file, desktop->exec, len))
	    {
	       found = 1;
	       break;
	    }
      
	eina_list_free(l);
	free(tmp);

	if (!found)
	  eina_hash_add(inst->added, file, file);
     }

   if (desktop)
     {
	if ((desktop2 = eina_hash_find(inst->added, file)) &&
	    desktop == desktop2)
	  return;

	eina_hash_add(inst->added, file, desktop);
	file = NULL;
     }

   if (desktop)
     it = evry_item_new(p, desktop->name);
   else
     it = evry_item_new(p, file);
   
   app = E_NEW(Evry_App, 1);
   app->desktop = desktop;
   app->file = file;
   it->data[0] = app;
   it->priority = prio;

   p->items = eina_list_append(p->items, it);
}

static void
_add_desktop_list(Evry_Plugin *p, Eina_List *apps, char *m1, char *m2)
{
   Efreet_Desktop *desktop;
   Eina_List *l;
   
   EINA_LIST_FOREACH(apps, l, desktop)
     {
	if (eina_list_count(p->items) > 99) continue;
	if (!desktop || !desktop->name || !desktop->exec) continue;
		  
	if (e_util_glob_case_match(desktop->exec, m1))
	  _item_add(p, desktop, NULL, 1);
	else if (e_util_glob_case_match(desktop->exec, m2))
	  _item_add(p, desktop, NULL, 2);
	else if (e_util_glob_case_match(desktop->name, m1))
	  _item_add(p, desktop, NULL, 1);
	else if (e_util_glob_case_match(desktop->name, m2))
	  _item_add(p, desktop, NULL, 2);
	/* else if (desktop->comment)
	 *   {
	 *      if (e_util_glob_case_match(desktop->comment, m1))
	 *        _item_add(p, desktop, NULL, 3);
	 *      else if (e_util_glob_case_match(desktop->comment, m2))
	 *        _item_add(p, desktop, NULL, 4);
	 *   } */
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

   if (app1->desktop)
     e1 = app1->desktop->exec;
   else
     e1 = app1->file;

   if (app2->desktop)
     e2 = app2->desktop->exec;
   else
     e2 = app2->file;

   t1 = e_exehist_newest_run_get(e1);
   t2 = e_exehist_newest_run_get(e2);

   if ((int)(t2 - t1))
     return (int)(t2 - t1);
   else if (it1->priority - it2->priority)
     return (it1->priority - it2->priority);

   // TODO compare exe strings?
   else return 0;
}

static int
_fetch(Evry_Plugin *p, const char *input)
{
   Eina_List *l;
   Efreet_Desktop *desktop;
   char *file;
   char match1[4096];
   char match2[4096];
   Evry_Item *it;
   Evry_App *app;
   Inst *inst = p->private;
   
   if (!inst)
     {
	inst = E_NEW(Inst, 1);
	p->private = inst;
     }
   inst->added = eina_hash_string_superfast_new(NULL);
   
   _list_free(p);

   if (input)
     {
	snprintf(match1, sizeof(match1), "%s*", input);
	snprintf(match2, sizeof(match2), "*%s*", input);
     }

   /* add apps for a given mimetype */
   if (inst && inst->apps)
     {
	if (!input)
	  {
	     EINA_LIST_FOREACH(inst->apps, l, desktop)
	       _item_add(p, desktop, NULL, 1);
	  }
	else
	  {
	     _add_desktop_list(p, inst->apps, match1, match2);
	     inst->apps = NULL;
	  }
     }

   /* add apps matching input */
   if (!p->items && input)
     {
	Eina_List *cats = efreet_util_desktop_categories_list();
	Eina_List *apps;
	const char *cat;
	
	EINA_LIST_FREE(cats, cat)
	  {
	     if (eina_list_count(p->items) > 99) break;	     
	     if (!strcmp(cat, "Screensaver")) continue;

	     apps = efreet_util_desktop_category_list(cat);
	     _add_desktop_list(p, apps, match1, match2);
	  }
     }

   /* add exe history items */
   else if (!p->items)
     {
   	l = e_exehist_list_get();
   	EINA_LIST_FREE(l, file)
   	  _item_add(p, NULL, file, 1);
     }

   /* show 'Run Command' item */
   if (input || p == p2)
     {
	int found = 0;
	char *path;
	char *end;

	if (input)
	  {
	     snprintf(match2, 4096, "%s", input);
	
	     if (end = strchr(input, ' '))
	       {
		  int len = (end - input) - 1;
		  if (len >= 0)
		    snprintf(match2, len, "%s", input);
	       }
	
	     EINA_LIST_FOREACH(exe_path, l, path)
	       {
		  snprintf(match1, 4096, "%s/%s", path, match2);
		  if (ecore_file_exists(match1))
		    {
		       found = 1;
		       break;
		    } 
	       }
	  }
	
	if (found || p == p2)
	  {
	     it = evry_item_new(p, _("Run Command"));
	     app = E_NEW(Evry_App, 1);
	     if (input)
	       app->file = eina_stringshare_add(input);
	     else
	       app->file = eina_stringshare_add("");
	     it->data[0] = app;
	     it->priority = 99;
	     p->items = eina_list_append(p->items, it);

	     snprintf(match1, 4096, "xterm -hold -e %s", input);
	     it = evry_item_new(p, _("Run in Terminal"));
	     app = E_NEW(Evry_App, 1);
	     if (input)
	       app->file = eina_stringshare_add(input);
	     else
	       app->file = eina_stringshare_add("");
	     it->data[0] = app;
	     it->priority = 100;
	     p->items = eina_list_append(p->items, it);
	  }
     }

   if (inst->added)
     {
	eina_hash_free(inst->added);
	inst->added = NULL;
     }

   if (p->items)
     {
	p->items = eina_list_sort(p->items, eina_list_count(p->items), _cb_sort);
	return 1;
     }

   return 0;
}

static Evas_Object *
_item_icon_get(Evry_Plugin *p __UNUSED__, Evry_Item *it, Evas *e)
{
   Evas_Object *o = NULL;
   Evry_App *app = it->data[0];

   if (app->desktop)
     o = e_util_desktop_icon_add(app->desktop, 64, e);

   if (!o)
     {
	o = e_icon_add(e);
	evry_icon_theme_set(o, "system-run");
     }

   return o;
}

static int
_exec_app_check_item(Evry_Action *act __UNUSED__, Evry_Item *it)
{
   Evry_App *app = it->data[0];
   if (app->desktop)
     return 1;

   if (app->file && strlen(app->file) > 0)
     return 1;
     
   return 0;
}

static int
_app_action(Evry_Item *it_app, Evry_Item *it_file)
{
   E_Zone *zone;
   Evry_App *app = NULL;
   Efreet_Desktop *desktop = NULL;
   Eina_List *files = NULL;
   char *exe = NULL;
   
   if (!it_app) return 0;
   
   app = it_app->data[0];
   if (!app) return 0;
   
   zone = e_util_zone_current_get(e_manager_current_get());
   
   if (app->desktop)
     {
	if (it_file && it_file->uri)
	  files = eina_list_append(files, it_file->uri);

	e_exec(zone, app->desktop, NULL, files, "everything");
	     
	if (it_file && it_file->mime)
	  e_exehist_mime_desktop_add(it_file->mime, app->desktop);

	if (files) eina_list_free(files);
     }
   else if (app->file)
     {
	if (it_file && it_file->uri)
	  {
	     int len;

	     len = strlen(app->file) + strlen(it_file->uri) + 2;
	     exe = malloc(len);
	     snprintf(exe, len, "%s %s", app->file, it_file->uri);
	  }
	else exe = (char *) app->file;
	  
	e_exec(zone, NULL, exe, NULL, NULL);

	if (it_file && it_file->uri)
	  free(exe);
     }

   return 1;
}

static int
_exec_app_action(Evry_Action *act, Evry_Item *it1, Evry_Item *it2, const char *input)
{
   return _app_action(it1, it2);
}

static int
_open_with_action(Evry_Plugin *p, Evry_Item *it, const char *input __UNUSED__)
{
   Inst *inst = p->private;
   if (inst->candidate)
     return _app_action(it, inst->candidate); 
}

static int
_edit_app_check_item(Evry_Action *act __UNUSED__, Evry_Item *it)
{
   Evry_App *app = it->data[0];
   if (app->desktop)
     return 1;

   return 0;
}


static int
_edit_app_action(Evry_Action *act, Evry_Item *it1, Evry_Item *it2, const char *input)
{
   Evry_App *app;
   Efreet_Desktop *desktop;

   if (!it1) return 0;

   app  = it1->data[0];
   if (app->desktop)
     desktop = app->desktop;
   else
     {
	char buf[128];
	snprintf(buf, 128, "%s/.local/share/applications/%s.desktop",
		 e_user_homedir_get(), app->file);
	desktop = efreet_desktop_empty_new(eina_stringshare_add(buf));
	/* XXX check if this gets freed by efreet*/
	desktop->exec = strdup(app->file); 
     }

   e_desktop_edit(e_container_current_get(e_manager_current_get()), desktop);
	
   return 1;
}


static int
_new_app_check_item(Evry_Action *act __UNUSED__, Evry_Item *it)
{
   Evry_App *app = it->data[0];
   if (app->desktop)
     return 1;

   if (app->file && strlen(app->file) > 0)
     return 1;
     
   return 0;
}


static int
_new_app_action(Evry_Action *act, Evry_Item *it1, Evry_Item *it2, const char *input)
{
   Evry_App *app;
   char *name;
   char buf[4096];
   char *end;
   Efreet_Desktop *desktop;
   int i;

   if (!it1) return 0;

   app = it1->data[0];
	
   if (app->desktop)
     name = strdup(app->desktop->name);
   else
     /* TODO replace '/' and remove other special characters */
     name = strdup(app->file);

   if ((end = strchr(name, ' ')))
     name[end - name] = '\0';
	
   for (i = 0; i < 10; i++)
     {
	snprintf(buf, 4096, "%s/.local/share/applications/%s-%d.desktop",
		 e_user_homedir_get(), name, i);
	if (ecore_file_exists(buf))
	  {
	     buf[0] = '\0';
	     continue;
	  }
	else break;
     }

   free(name);
	   
   if (strlen(buf) == 0)
     return 0;
	
   if (!app->desktop)
     {
	desktop = efreet_desktop_empty_new(buf);
	desktop->exec = strdup(app->file);
     }
   else 
     {
	efreet_desktop_save_as(app->desktop, buf);
	/*XXX hackish - desktop is removed on save_as..*/
	efreet_desktop_new(app->desktop->orig_path);
	     
	desktop = efreet_desktop_new(buf);
     }
	
   e_desktop_edit(e_container_current_get(e_manager_current_get()), desktop);

   return 1;
}

static Eina_Bool
_init(void)
{
   char *path, *p, *last;
   
   p1 = E_NEW(Evry_Plugin, 1);
   p1->name = "Applications";
   p1->type = type_subject;
   p1->type_in  = "NONE";
   p1->type_out = "APPLICATION";
   p1->need_query = 0;
   p1->begin = &_begin;
   p1->fetch = &_fetch;
   p1->cleanup = &_cleanup;
   p1->icon_get = &_item_icon_get;
   evry_plugin_register(p1);

   p2 = E_NEW(Evry_Plugin, 1);
   p2->name = "Open With...";
   p2->type = type_action;
   p2->type_in  = "FILE";
   p2->type_out = "NONE";
   p2->need_query = 0;
   p2->begin = &_begin;
   p2->fetch = &_fetch;
   p2->action = &_open_with_action;
   p2->cleanup = &_cleanup;
   p2->icon_get = &_item_icon_get;
   evry_plugin_register(p2);

   act = E_NEW(Evry_Action, 1);
   act->name = "Launch";
   act->is_default = EINA_TRUE;
   act->type_in1 = "APPLICATION";
   act->action = &_exec_app_action;
   act->check_item = &_exec_app_check_item;
   act->icon = "everything-launch";
   evry_action_register(act);
     
   act1 = E_NEW(Evry_Action, 1);
   act1->name = "Open File...";
   act1->type_in1 = "APPLICATION";
   act1->type_in2 = "FILE";
   act1->action = &_exec_app_action;
   act1->check_item = &_exec_app_check_item;
   act1->icon = "everything-launch";
   evry_action_register(act1);

   act2 = E_NEW(Evry_Action, 1);
   act2->name = "Edit Application Entry";
   act2->type_in1 = "APPLICATION";
   act2->action = &_edit_app_action;
   act2->check_item = &_edit_app_check_item;
   act2->icon = "everything-launch";
   evry_action_register(act2);

   act3 = E_NEW(Evry_Action, 1);
   act3->name = "New Application Entry";
   act3->type_in1 = "APPLICATION";
   act3->action = &_new_app_action;
   act3->check_item = &_new_app_check_item;
   act3->icon = "everything-launch";
   evry_action_register(act3);

   /* taken from e_exebuf.c */
   path = getenv("PATH");
   if (path)
     {
	path = strdup(path);
	last = path;
	for (p = path; p[0]; p++)
	  {
	     if (p[0] == ':') p[0] = '\0';
	     if (p[0] == 0)
	       {
		  exe_path = eina_list_append(exe_path, strdup(last));
		  last = p + 1;
	       }
	  }
	if (p > last)
	  exe_path = eina_list_append(exe_path, strdup(last));
	free(path);
     }
   
   return EINA_TRUE;
}

static void
_shutdown(void)
{
   char *str;
   
   evry_plugin_unregister(p1);
   evry_plugin_unregister(p2);
   evry_action_unregister(act);
   evry_action_unregister(act1);
   evry_action_unregister(act2);
   evry_action_unregister(act3);
   E_FREE(p1);
   E_FREE(p2);
   E_FREE(act);
   E_FREE(act1);
   E_FREE(act2);
   E_FREE(act3);

   EINA_LIST_FREE(exe_path, str)
     free(str);
}

EINA_MODULE_INIT(_init);
EINA_MODULE_SHUTDOWN(_shutdown);
