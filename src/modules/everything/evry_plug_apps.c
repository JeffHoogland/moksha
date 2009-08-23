#include "Evry.h"

typedef struct _Plugin Plugin;

struct _Plugin
{
  Evry_Plugin base;
  Eina_Hash *added;
  Eina_List *apps_mime;
  Eina_List *apps_all;
  const Evry_Item *candidate;
};

static Plugin *p1;
static Plugin *p2;
static Evry_Action *act;
static Evry_Action *act1;
static Evry_Action *act2;
static Evry_Action *act3;
static Evry_Action *act4;
static Eina_List *exe_path = NULL;


static Evry_Plugin *
_begin(Evry_Plugin *plugin, const Evry_Item *item)
{
   /* Plugin *p = (Plugin*) plugin; */
   PLUGIN(p, plugin);

   const char *mime;

   if (item)
     {
	ITEM_FILE(file, item);
	Efreet_Desktop *desktop;

	if (!file->uri) return NULL;

	if (!file->mime)
	  mime = efreet_mime_type_get(file->uri);
	else
	  mime = file->mime;

	if (!mime) return NULL;

	p->candidate = EVRY_ITEM(file);
	p->apps_mime = efreet_util_desktop_mime_list(mime);
	desktop = e_exehist_mime_desktop_get(mime);
	if (desktop)
	  {
	     efreet_desktop_ref(desktop);
	     p->apps_mime = eina_list_prepend(p->apps_mime, desktop);
	  }
     }

   return EVRY_PLUGIN(p);
}

static void
_list_free(Evry_Plugin *plugin)
{

}

static void
_item_free(Evry_Item *item)
{
   ITEM_APP(app, item);

   if (app->file) eina_stringshare_del(app->file);
   if (app->desktop) efreet_desktop_free(app->desktop);

   E_FREE(app);
}

static void
_cleanup(Evry_Plugin *plugin)
{
   PLUGIN(p, plugin);
   Efreet_Desktop *desktop;

   EVRY_PLUGIN_ITEMS_FREE(p);

   EINA_LIST_FREE(p->apps_mime, desktop)
     efreet_desktop_free(desktop);

   EINA_LIST_FREE(p->apps_all, desktop)
     efreet_desktop_free(desktop);
}

static int
_item_add(Plugin *p, Efreet_Desktop *desktop, char *file, int match)
{
   Evry_Item_App *app;
   Efreet_Desktop *d2;
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

	if (eina_hash_find(p->added, file))
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
	  eina_hash_add(p->added, file, file);
     }

   if (desktop)
     {
	if ((d2 = eina_hash_find(p->added, file)) &&
	    ((desktop == d2) ||
	     (!strcmp(desktop->exec, d2->exec))))
	  return 0;

	if (!already_refd)
	  efreet_desktop_ref(desktop);
	eina_hash_add(p->added, file, desktop);
	file = NULL;
     }

   app = E_NEW(Evry_Item_App, 1);

   if (desktop)
     evry_item_new(EVRY_ITEM(app), EVRY_PLUGIN(p), desktop->name, _item_free);
   else
     evry_item_new(EVRY_ITEM(app), EVRY_PLUGIN(p), file, _item_free);

   app->desktop = desktop;
   app->file = file;
   EVRY_ITEM(app)->fuzzy_match = match;

   EVRY_PLUGIN_ITEM_APPEND(p, app);

   return 1;
}

static void
_add_desktop_list(Plugin *p, Eina_List *apps, const char *input)
{
   Efreet_Desktop *desktop;
   Eina_List *l;
   int m1, m2;

   EINA_LIST_FOREACH(apps, l, desktop)
     {
	if (eina_list_count(EVRY_PLUGIN(p)->items) > 199) continue;
	if (!desktop->name || !desktop->exec) continue;

	char *exec = strrchr(desktop->exec, '/');
	if (!exec++ || !exec) exec = desktop->exec;

	m1 = evry_fuzzy_match(exec, input);
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
   const char *e1, *e2;
   double t1, t2;

   ITEM_APP(app1, it1);
   ITEM_APP(app2, it2);

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

   if (it1->fuzzy_match && !it2->fuzzy_match)
     return -1;

   if (!it1->fuzzy_match && it2->fuzzy_match)
     return 1;

   t1 = t1 / (double)it1->fuzzy_match;
   t2 = t2 / (double)it2->fuzzy_match;

   if ((int)(t2 - t1))
     return (int)(t2 - t1);

   if (it1->fuzzy_match - it2->fuzzy_match)
     return (it1->fuzzy_match - it2->fuzzy_match);

   else return 0;
}

static int
_fetch(Evry_Plugin *plugin, const char *input)
{
   PLUGIN(p, plugin);
   Eina_List *l;
   Efreet_Desktop *desktop;
   Evry_Item *it;
   char *file;
   int prio = 0;

   p->added = eina_hash_string_small_new(NULL);

   EVRY_PLUGIN_ITEMS_FREE(p);

   /* add apps for a given mimetype */
   if (plugin->type == type_action)
     {
	if (input)
	  _add_desktop_list(p, p->apps_mime, input);
	else
	  {
	     EINA_LIST_FOREACH(p->apps_mime, l, desktop)
	       _item_add(p, desktop, NULL, 1);
	  }
     }

   /* add apps matching input */
   if (input)
     {
	if (!p->apps_all)
	  {
	     Eina_List *apps = NULL;
	     Eina_List *cats;
	     Eina_List *l, *ll;

	     apps = efreet_util_desktop_name_glob_list("*");
	     cats = efreet_util_desktop_category_list("Screensaver");

	     EINA_LIST_FOREACH(cats, l, desktop)
	       {
		  ll = eina_list_data_find_list(apps, desktop);
		  if (ll)
		    {
		       efreet_desktop_free(desktop);
		       apps = eina_list_remove_list(apps, ll);
		    }
	       }

	     p->apps_all = apps;
	  }

	_add_desktop_list(p, p->apps_all, input);
     }
   /* add exe history items */
   else if (!plugin->items)
     {
   	l = e_exehist_list_get();
   	EINA_LIST_FREE(l, file)
   	  _item_add(p, NULL, file, 1);
     }

   /* show 'Run Command' item */
   if (input)
     {
	int found = 0;
	char *end;
	char *dir;
	char cmd[1024];
	char path[1024];

	snprintf(cmd, sizeof(cmd), "%s", input);

	if ((end = strchr(input, ' ')))
	  {
	     int len = (end - input) + 1;
	     if (len >= 0)
	       snprintf(cmd, len, "%s", input);
	  }

	EINA_LIST_FOREACH(exe_path, l, dir)
	  {
	     snprintf(path, sizeof(path), "%s/%s", dir, cmd);
	     if (ecore_file_exists(path))
	       {
		  found = 1;
		  break;
	       }
	  }

	if (found)
	  {
	     Evry_Item_App *app;
	     app = E_NEW(Evry_Item_App, 1);
	     evry_item_new(EVRY_ITEM(app), plugin, _("Run Command"), _item_free);
	     app->file = eina_stringshare_add(input);
	     EVRY_ITEM(app)->priority = 9999;
	     EVRY_PLUGIN_ITEM_APPEND(p, app);

	     snprintf(cmd, sizeof(cmd), "xterm -hold -e %s", input);
	     app = E_NEW(Evry_Item_App, 1);
	     evry_item_new(EVRY_ITEM(app), plugin, _("Run in Terminal"), _item_free);
	     app->file = eina_stringshare_add(cmd);
	     EVRY_ITEM(app)->priority = 1000;
	     EVRY_PLUGIN_ITEM_APPEND(p, app);
	  }
     }

   eina_hash_free(p->added);

   if (!plugin->items) return 0;

   EVRY_PLUGIN_ITEMS_SORT(plugin, _cb_sort);

   EINA_LIST_FOREACH(plugin->items, l, it)
     it->priority = prio++;

   return 1;
}

static Evas_Object *
_icon_get(Evry_Plugin *p __UNUSED__, const Evry_Item *it, Evas *e)
{
   ITEM_APP(app, it);
   Evas_Object *o = NULL;

   /* Evry_App *app = it->data[0]; */

   if (app->desktop)
     o = e_util_desktop_icon_add(app->desktop, 64, e);

   if (!o)
     o = evry_icon_theme_get("system-run", e);

   return o;
}

static int
_exec_app_check_item(Evry_Action *act __UNUSED__, const Evry_Item *it)
{
   ITEM_APP(app, it);

   if (app->desktop)
     return 1;

   if (app->file && strlen(app->file) > 0)
     return 1;

   return 0;
}

static int
_app_action(const Evry_Item *it1, const Evry_Item *it2)
{
   E_Zone *zone;
   /* Evry_App *app = NULL; */
   Eina_List *files = NULL;
   char *exe = NULL;

   if (!it1) return 0;
   /* app = it_app->data[0]; */
   ITEM_APP(app, it1);

   if (!app) return 0;

   zone = e_util_zone_current_get(e_manager_current_get());

   if (app->desktop)
     {
	if (it2)
	  {
	     ITEM_FILE(file, it2);

	     Eina_List *l;
	     char *mime;
	     char *path = NULL;
	     int open_folder = 0;

	     if (!EVRY_ITEM(file)->browseable)
	       {
		  EINA_LIST_FOREACH(app->desktop->mime_types, l, mime)
		    {
		       if (!strcmp(mime, "x-directory/normal"))
			 {
			    open_folder = 1;
			    break;
			 }
		    }
	       }

	     if (open_folder)
	       {
		  path = ecore_file_dir_get(file->uri);
		  files = eina_list_append(files, path);
	       }
	     else
	       {
		  files = eina_list_append(files, file->uri);
	       }

	     e_exec(zone, app->desktop, NULL, files, NULL);

	     if (file && file->mime && !open_folder)
	       e_exehist_mime_desktop_add(file->mime, app->desktop);

	     if (files)
	       eina_list_free(files);

	     if (open_folder && path)
	       free(path);
	  }
	else
	  e_exec(zone, app->desktop, NULL, NULL, "everything");
     }
   else if (app->file)
     {
	if (it2)
	  {
	     ITEM_FILE(file, it2);
	     int len;

	     len = strlen(app->file) + strlen(file->uri) + 2;
	     exe = malloc(len);
	     snprintf(exe, len, "%s %s", app->file, file->uri);
	     e_exec(zone, NULL, exe, NULL, NULL);
	     free(exe);
	  }
	else
	  {
	     exe = (char *) app->file;
	     e_exec(zone, NULL, exe, NULL, NULL);
	  }
     }

   return 1;
}

static int
_exec_app_action(Evry_Action *act)
{
   return _app_action(act->item1, act->item2);
}

static int
_open_with_action(Evry_Plugin *plugin, const Evry_Item *it)
{
   PLUGIN(p, plugin);
   /* Plugin *p = (Plugin*) plugin; */

   if (p->candidate)
     return _app_action(it, p->candidate);

   return 0;
}

static int
_edit_app_check_item(Evry_Action *act __UNUSED__, const Evry_Item *it)
{
   ITEM_APP(app, it);
   /* Evry_App *app = it->data[0]; */
   if (app->desktop)
     return 1;

   return 0;
}

static int
_edit_app_action(Evry_Action *act)
{
   Efreet_Desktop *desktop;
   ITEM_APP(app, act->item1);

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
   ITEM_APP(app, it);
   /* Evry_App *app = it->data[0]; */
   if (app->desktop)
     return 1;

   if (app->file && strlen(app->file) > 0)
     return 1;

   return 0;
}

static int
_new_app_action(Evry_Action *act)
{
   /* Evry_App *app; */
   char *name;
   char buf[4096];
   char *end;
   Efreet_Desktop *desktop;
   int i;

   ITEM_APP(app, act->item1);
   /* app = act->item1->data[0]; */

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
	/* efreet_desktop_new(app->desktop->orig_path); */

	desktop = efreet_desktop_new(buf);
     }

   e_desktop_edit(e_container_current_get(e_manager_current_get()), desktop);

   return 1;
}

static int
_exec_border_check_item(Evry_Action *act __UNUSED__, const Evry_Item *it)
{
   E_Border *bd = it->data;
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
_exec_border_action(Evry_Action *act)
{
   return _app_action(act->item1, act->item2);
}

static int
_exec_border_intercept(Evry_Action *act)
{
   /* Evry_Item *it = E_NEW(Evry_Item, 1); */
   Evry_Item_App *app = E_NEW(Evry_Item_App, 1);
   E_Border *bd = act->item1->data;

   app->desktop = bd->desktop;
   /* it->data[0] = app; */

   /* act->item1 = it; */
   act->item1 = EVRY_ITEM(app);

   return 1;
}


static void
_exec_border_cleanup(Evry_Action *act)
{
   /* Evry_Item *it = (Evry_Item*) act->item1;
    * Evry_App *app = it->data[0]; */
   ITEM_APP(app, act->item1);

   E_FREE(app);
   /* E_FREE(it); */
}


static Eina_Bool
_init(void)
{
   char *path, *p, *last;

   p1 = E_NEW(Plugin, 1);
   evry_plugin_new(EVRY_PLUGIN(p1), "Applications", type_subject, "", "APPLICATION", 0, NULL, NULL,
		   _begin, _cleanup, _fetch, NULL, _icon_get, NULL, NULL);

   p2 = E_NEW(Plugin, 1);
   evry_plugin_new(EVRY_PLUGIN(p2), "Open With...", type_action, "FILE", "", 0, NULL, NULL,
		   _begin, _cleanup, _fetch, _open_with_action,
		   _icon_get, NULL, NULL);

   evry_plugin_register(EVRY_PLUGIN(p1), 1);
   evry_plugin_register(EVRY_PLUGIN(p2), 3);

   act = evry_action_new("Launch", "APPLICATION", NULL, NULL,
			 "everything-launch",
			 _exec_app_action, _exec_app_check_item,
			 NULL, NULL,NULL);

   act1 = evry_action_new("Open File...", "APPLICATION", "FILE", "APPLICATION",
			  "document-open",
			  _exec_app_action, _exec_app_check_item,
			  NULL, NULL, NULL);

   act2 = evry_action_new("Edit Application Entry", "APPLICATION", NULL, NULL,
			  "everything-launch",
			  _edit_app_action, _edit_app_check_item,
			  NULL, NULL, NULL);

   act3 = evry_action_new("New Application Entry", "APPLICATION", NULL, NULL,
			  "everything-launch",
			  _new_app_action, _new_app_check_item,
			  NULL, NULL, NULL);

   act4 = evry_action_new("Open File...", "BORDER", "FILE", "APPLICATION",
			  "everything-launch",
			  _exec_border_action, _exec_border_check_item,
			  _exec_border_cleanup, _exec_border_intercept, NULL);

   evry_action_register(act);
   evry_action_register(act1);
   evry_action_register(act2);
   evry_action_register(act3);
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

   EVRY_PLUGIN_FREE(p1);
   EVRY_PLUGIN_FREE(p2);

   evry_action_free(act);
   evry_action_free(act1);
   evry_action_free(act2);
   evry_action_free(act3);
   evry_action_free(act4);

   EINA_LIST_FREE(exe_path, str)
     free(str);
}

EINA_MODULE_INIT(_init);
EINA_MODULE_SHUTDOWN(_shutdown);
