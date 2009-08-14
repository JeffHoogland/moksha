#include "Evry.h"

typedef struct _Inst Inst;

struct _Inst
{
  Eina_Hash *added;
  Eina_List *apps_mime;
  Eina_List *apps_all;
  const Evry_Item *candidate;
};

static Evry_Plugin *p1;
static Evry_Plugin *p2;
static Evry_Action *act;
static Evry_Action *act1;
static Evry_Action *act2;
static Evry_Action *act3;
static Evry_Action *act4;
static Eina_List *exe_path = NULL;


static int
_begin(Evry_Plugin *p, const Evry_Item *it)
{
   const char *mime;
   Inst *inst = NULL;

   if (it)
     {
	Eina_List *l;
	Efreet_Desktop *desktop;

	if (!it->uri) return 0;

	if (!it->mime)
	  mime = efreet_mime_type_get(it->uri);
	else
	  mime = it->mime;

	/* TODO  show plugin for items without mimetype ? */
	if (!mime) return 0;

	inst = E_NEW(Inst, 1);
	inst->candidate = it;
	inst->apps_mime = efreet_util_desktop_mime_list(mime);
	desktop = e_exehist_mime_desktop_get(mime);
	if (desktop)
	  {
	     efreet_desktop_ref(desktop);
	     inst->apps_mime = eina_list_prepend(inst->apps_mime, desktop);
	  }
     }
   else
     {
	inst = E_NEW(Inst, 1);
     }

   p->private = inst;

   return 1;
}

static void
_list_free(Evry_Plugin *p)
{
   Evry_Item *it;

   EINA_LIST_FREE(p->items, it)
     evry_item_free(it);
}

static void
_item_free(Evry_Item *it)
{
   Evry_App *app;

   app = it->data[0];
   if (app->file) eina_stringshare_del(app->file);
   if (app->desktop) efreet_desktop_free(app->desktop);
   E_FREE(app);
}

static void
_cleanup(Evry_Plugin *p)
{
   Inst *inst = p->private;

   _list_free(p);

   if (inst)
     {
	Efreet_Desktop *desktop;
	EINA_LIST_FREE(inst->apps_mime, desktop)
	  efreet_desktop_free(desktop);
	EINA_LIST_FREE(inst->apps_all, desktop)
	  efreet_desktop_free(desktop);

	E_FREE(inst);
     }

   p->private = NULL;
}

static int
_item_add(Evry_Plugin *p, Efreet_Desktop *desktop, char *file, int match)
{
   Evry_Item *it;
   Evry_App *app;
   Efreet_Desktop *d2;
   Inst *inst = p->private;
   int already_refd = 0;

   if (desktop)
     file = desktop->exec;

   if (!file) return 0;

   if (!desktop)
     {
	char match[4096];
	Eina_List *l;
	int len;
	char *tmp;

	if (eina_hash_find(inst->added, file))
	  return 0;

	len = strlen(file);
	tmp = ecore_file_app_exe_get(file);
	snprintf(match, sizeof(match), "%s*", tmp);
	l = efreet_util_desktop_exec_glob_list(match);

	EINA_LIST_FREE(l, d2)
	  {
	     if (!desktop && d2->exec && !strncmp(file, d2->exec, len))
	       {
		  desktop = d2;
		  already_refd = 1;
		  efreet_desktop_ref(desktop);
	       }
	     efreet_desktop_free(d2);
	  }

	free(tmp);

	if (!desktop)
	  eina_hash_add(inst->added, file, file);
     }

   if (desktop)
     {
	if ((d2 = eina_hash_find(inst->added, file)) &&
	    ((desktop == d2) ||
	     (!strcmp(desktop->exec, d2->exec))))
	  return 0;

	if (!already_refd)
	  efreet_desktop_ref(desktop);
	eina_hash_add(inst->added, file, desktop);
	file = NULL;
     }

   if (desktop)
     it = evry_item_new(p, desktop->name, &_item_free);
   else
     it = evry_item_new(p, file, &_item_free);

   app = E_NEW(Evry_App, 1);
   app->desktop = desktop;
   app->file = file;
   it->data[0] = app;
   it->fuzzy_match = match;

   p->items = eina_list_append(p->items, it);

   return 1;
}

static void
_add_desktop_list(Evry_Plugin *p, Eina_List *apps, const char *input)
{
   Efreet_Desktop *desktop;
   Eina_List *l;
   int m1, m2, min;

   EINA_LIST_FOREACH(apps, l, desktop)
     {
	if (eina_list_count(p->items) > 199) continue;
	if (!desktop->name || !desktop->exec) continue;

	m1 = evry_fuzzy_match(desktop->exec, input);
	m2 = evry_fuzzy_match(desktop->name, input);

	if (!m1 || (m2 && m2 < m1))
	  m1 = m2;

	if (m1) _item_add(p, desktop, NULL, m1);
     }
}

static int
_cb_sort(const void *data1, const void *data2)
{
   const Evry_Item *it1 = data1;
   const Evry_Item *it2 = data2;
   Evry_App *app1, *app2;
   const char *e1, *e2;
   double t1, t2;
   
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

   if (it1->fuzzy_match && !it2->fuzzy_match)
     return -1;

   if (!it1->fuzzy_match && it2->fuzzy_match)
     return 1;

   if (it1->fuzzy_match - it2->fuzzy_match)
     return (it1->fuzzy_match - it2->fuzzy_match);

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

   inst->added = eina_hash_string_small_new(NULL);

   _list_free(p);

   /* add apps for a given mimetype */
   if (p->type == type_action)
     {
	if (input)
	  {
	     _add_desktop_list(p, inst->apps_mime, input);
	  }
	else
	  {
	     EINA_LIST_FOREACH(inst->apps_mime, l, desktop)
	       _item_add(p, desktop, NULL, 1);
	  }
     }

   /* add apps matching input */
   if (input)
     {
	if (!inst->apps_all)
	  {
	     Eina_List *apps = NULL;
	     Eina_List *stuff;
	     Eina_List *l, *ll;

	     apps = efreet_util_desktop_name_glob_list("*");
	     stuff = efreet_util_desktop_category_list("Screensaver");

	     EINA_LIST_FOREACH(stuff, l, desktop)
	       {
		  ll = eina_list_data_find_list(apps, desktop);
		  if (ll)
		    {
		       efreet_desktop_free(desktop);
		       apps = eina_list_remove_list(apps, ll);
		    }
		  /* efreet_desktop_free(desktop); */
	       }

	     inst->apps_all = apps;
	  }

	_add_desktop_list(p, inst->apps_all, input);
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
	     it = evry_item_new(p, _("Run Command"), &_item_free);
	     app = E_NEW(Evry_App, 1);
	     if (input)
	       app->file = eina_stringshare_add(input);
	     else
	       app->file = eina_stringshare_add("");
	     it->data[0] = app;
	     it->priority = 99;
	     p->items = eina_list_append(p->items, it);

	     snprintf(match1, 4096, "xterm -hold -e %s", input);
	     it = evry_item_new(p, _("Run in Terminal"), &_item_free);
	     app = E_NEW(Evry_App, 1);
	     if (input)
	       app->file = eina_stringshare_add(match1);
	     else
	       app->file = eina_stringshare_add("");
	     it->data[0] = app;
	     it->priority = 100;
	     p->items = eina_list_append(p->items, it);
	  }
     }

   eina_hash_free(inst->added);

   if (p->items)
     {
	int prio;
	p->items = eina_list_sort(p->items, eina_list_count(p->items), _cb_sort);
	EINA_LIST_FOREACH(p->items, l, it)
	  it->priority = prio++;

	return 1;
     }

   return 0;
}

static Evas_Object *
_item_icon_get(Evry_Plugin *p __UNUSED__, const Evry_Item *it, Evas *e)
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
_exec_app_check_item(Evry_Action *act __UNUSED__, const Evry_Item *it)
{
   Evry_App *app = it->data[0];
   if (app->desktop)
     return 1;

   if (app->file && strlen(app->file) > 0)
     return 1;
   
   return 0;
}

static int
_exec_border_check_item(Evry_Action *act __UNUSED__, const Evry_Item *it)
{
   E_Border *bd = it->data[0];
   E_OBJECT_CHECK_RETURN(bd, 0);
   E_OBJECT_TYPE_CHECK_RETURN(bd, E_BORDER_TYPE, 0);

   if ((bd->desktop && bd->desktop->exec) &&
       ((strstr(bd->desktop->exec, "%u")) ||
	(strstr(bd->desktop->exec, "%U")) ||
	(strstr(bd->desktop->exec, "%f")) ||
	(strstr(bd->desktop->exec, "%F"))))
     return 1;

   return 0;
}

static int
_app_action(const Evry_Item *it_app, const Evry_Item *it_file)
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
_exec_app_action(Evry_Action *act, const Evry_Item *it1, const Evry_Item *it2, const char *input)
{
   return _app_action(it1, it2);
}

static int
_exec_border_action(Evry_Action *act, const Evry_Item *it1, const Evry_Item *it2, const char *input)
{
   Evry_Item *it = E_NEW(Evry_Item, 1);
   Evry_App *app = E_NEW(Evry_App, 1);
   E_Border *bd = it1->data[0];

   app->desktop = bd->desktop;
   it->data[0] = app;
   
   return _app_action(it, it2);
}

static int
_open_with_action(Evry_Plugin *p, const Evry_Item *it, const char *input __UNUSED__)
{
   Inst *inst = p->private;
   if (inst->candidate)
     return _app_action(it, inst->candidate);
}

static int
_edit_app_check_item(Evry_Action *act __UNUSED__, const Evry_Item *it)
{
   Evry_App *app = it->data[0];
   if (app->desktop)
     return 1;

   return 0;
}


static int
_edit_app_action(Evry_Action *act, const Evry_Item *it1, const Evry_Item *it2, const char *input)
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
_new_app_check_item(Evry_Action *act __UNUSED__, const Evry_Item *it)
{
   Evry_App *app = it->data[0];
   if (app->desktop)
     return 1;

   if (app->file && strlen(app->file) > 0)
     return 1;

   return 0;
}


static int
_new_app_action(Evry_Action *act, const Evry_Item *it1, const Evry_Item *it2, const char *input)
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

   act4 = E_NEW(Evry_Action, 1);
   act4->name = "Open File...";
   act4->type_in1 = "BORDER";
   act4->type_in2 = "FILE";
   act4->action = &_exec_border_action;
   act4->check_item = &_exec_border_check_item;
   act4->icon = "everything-launch";
   evry_action_register(act4);

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
   E_FREE(act4);

   EINA_LIST_FREE(exe_path, str)
     free(str);
}

EINA_MODULE_INIT(_init);
EINA_MODULE_SHUTDOWN(_shutdown);
