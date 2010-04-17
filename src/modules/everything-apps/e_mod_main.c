/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */

#include "Evry.h"
#include "e_mod_main.h"

typedef struct _Plugin Plugin;

struct _Plugin
{
  Evry_Plugin base;
  Eina_List *apps_mime;
  Eina_List *apps_all;
  Eina_List *apps_hist;
  const Evry_Item *candidate;
  Eina_Hash *added;

  Evry_Item_App *app_command;
  Evry_Item_App *app_parameter;
};

/* taken from exebuf module */
typedef struct _E_Exe E_Exe;
typedef struct _E_Exe_List E_Exe_List;
typedef struct _Module_Config Module_Config;

struct _E_Exe
{
  const char *path;
};

struct _E_Exe_List
{
  Eina_List *list;
};


struct _Module_Config 
{  
  int version;

  unsigned char list_executables;

  E_Config_Dialog *cfd;
  E_Module *module;
};

static Plugin *p1 = NULL;
static Plugin *p2 = NULL;
static Evry_Action *act = NULL;
static Evry_Action *act1 = NULL;
static Evry_Action *act2 = NULL;
static Evry_Action *act3 = NULL;
static Evry_Action *act4 = NULL;
static Evry_Action *act5 = NULL;

static Module_Config    *_conf;

static Eina_List *exe_path = NULL;
static Ecore_Idler *exe_scan_idler = NULL;
static E_Config_DD *exelist_exe_edd = NULL;
static E_Config_DD *exelist_edd = NULL;
static DIR       *exe_dir = NULL;
static Eina_List *exe_list = NULL;
static Eina_List *exe_list2 = NULL;

static int _scan_idler(void *data);

static void _hash_free(void *data)
{
   ITEM_APP(app, data);
   evry_item_free(EVRY_ITEM(app));
}


static Evry_Plugin *
_begin_open_with(Evry_Plugin *plugin, const Evry_Item *item)
{
   PLUGIN(p, plugin);

   const char *mime;

   if (!item) return 0;

   ITEM_FILE(file, item);
   Efreet_Desktop *d, *d2;

   if (!file->path) return NULL;

   if (!file->mime)
     mime = efreet_mime_type_get(file->path);
   else
     mime = file->mime;

   if (!mime) return NULL;

   p->candidate = item;
   
   p->apps_mime = efreet_util_desktop_mime_list(mime);

   if (strcmp(mime, "text/plain") &&
       !strncmp(mime, "text/", 5))
     {
	Eina_List *tmp;
	tmp = efreet_util_desktop_mime_list("text/plain");

	EINA_LIST_FREE(tmp, d)
	  {
	     if (!eina_list_data_find_list(p->apps_mime, d))
	       p->apps_mime = eina_list_append(p->apps_mime, d);
	     else
	       efreet_desktop_free(d); 
	  }
     }

   d = e_exehist_mime_desktop_get(mime);
   if (d)
     {
	if ((d2 = eina_list_data_find(p->apps_mime, d)))
	  {
	     p->apps_mime = eina_list_remove(p->apps_mime, d2);
	     efreet_desktop_free(d2);
	  }
	
	p->apps_mime = eina_list_prepend(p->apps_mime, d);
     }
   
   p->added = eina_hash_string_small_new(_hash_free);

   return plugin;
}

static Evry_Plugin *
_begin(Evry_Plugin *plugin, const Evry_Item *item)
{
   PLUGIN(p, plugin);

   p->added = eina_hash_string_small_new(_hash_free);

   if (!_conf->list_executables)
     return plugin;
   
   /* taken from exebuf module */
   char *path, *pp, *last;
   E_Exe_List *el;

   el = e_config_domain_load("exebuf_exelist_cache", exelist_edd);
   if (el)
     {
	E_Exe *ee;

	EINA_LIST_FREE(el->list, ee)
	  {
	     exe_list = eina_list_append(exe_list, strdup(ee->path));
	     eina_stringshare_del(ee->path);
	     free(ee);
	  }
	free(el);
     }
   path = getenv("PATH");
   if (path)
     {
	path = strdup(path);
	last = path;
	for (pp = path; pp[0]; pp++)
	  {
	     if (pp[0] == ':') pp[0] = '\0';
	     if (pp[0] == 0)
	       {
		  exe_path = eina_list_append(exe_path, strdup(last));
		  last = pp + 1;
	       }
	  }
	if (pp > last)
	  exe_path = eina_list_append(exe_path, strdup(last));
	free(path);
     }

   exe_scan_idler = ecore_idler_add(_scan_idler, NULL);

   return plugin;
}

static void
_item_free(Evry_Item *item)
{
   ITEM_APP(app, item);

   if (app->desktop)
     efreet_desktop_free(app->desktop);
   if (app->file)
     eina_stringshare_del(app->file);

   E_FREE(app);
}

static void
_cleanup(Evry_Plugin *plugin)
{
   PLUGIN(p, plugin);
   Efreet_Desktop *desktop;

   if (p->added)
     eina_hash_free(p->added);

   EVRY_PLUGIN_ITEMS_CLEAR(p);

   EINA_LIST_FREE(p->apps_all, desktop)
     efreet_desktop_free(desktop);

   EINA_LIST_FREE(p->apps_hist, desktop)
     efreet_desktop_free(desktop);
   
   if (plugin->type == type_action)
     {
	EINA_LIST_FREE(p->apps_mime, desktop)
	  efreet_desktop_free(desktop);
     }
   else
     {
	char *str;
	
	if (exe_dir)
	  {
	     closedir(exe_dir);
	     exe_dir = NULL;
	  }
	EINA_LIST_FREE(exe_path, str)
	  free(str);

	if (exe_scan_idler)
	  {
	     ecore_idler_del(exe_scan_idler);
	     exe_scan_idler = NULL;
	  }
	EINA_LIST_FREE(exe_list, str)
	  free(str);
	EINA_LIST_FREE(exe_list2, str)
	  free(str);
     }
   
   p->app_command = NULL;
   p->app_parameter = NULL;

}

static Evry_Item_App *
_item_add(Plugin *p, Efreet_Desktop *desktop, const char *file, int match)
{
   Evry_Item_App *app;
   Efreet_Desktop *d2;
   int already_refd = 0;
   const char *exe;
   
   if (file)
     {
	Eina_List *l;
	int len;
	char buf[1024];
	char *tmp;

	if ((app = eina_hash_find(p->added, file)))
	  {
	     if (!eina_list_data_find_list(EVRY_PLUGIN(p)->items, app))
	       {
		  EVRY_ITEM(app)->fuzzy_match = match;
		  EVRY_ITEM(app)->plugin = EVRY_PLUGIN(p);
		  EVRY_PLUGIN_ITEM_APPEND(p, app);
	       }
	     return app;
	  }

	len = strlen(file);
	tmp = ecore_file_app_exe_get(file);
	snprintf(buf, sizeof(buf), "%s*", tmp);
	l = efreet_util_desktop_exec_glob_list(buf);

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
     }

   if (desktop)
     exe = desktop->exec;
   else
     exe = file;

   if (!exe) return NULL;

   if ((app = eina_hash_find(p->added, exe)))
     {
	if (!desktop || (!app->desktop) ||
	    (desktop == app->desktop) ||
	    (!strcmp(desktop->exec, app->desktop->exec)))
	  {
	     if (!eina_list_data_find_list(EVRY_PLUGIN(p)->items, app))
	       {
		  EVRY_ITEM(app)->fuzzy_match = match;
		  EVRY_ITEM(app)->plugin = EVRY_PLUGIN(p);
		  EVRY_PLUGIN_ITEM_APPEND(p, app);
	       }
	     return app;
	  }
     }
   if (desktop && !already_refd)
     efreet_desktop_ref(desktop);

   app = E_NEW(Evry_Item_App, 1);
   
   if (desktop)
     {
	evry_item_new(EVRY_ITEM(app), EVRY_PLUGIN(p), desktop->name, _item_free);
	EVRY_ITEM(app)->id = eina_stringshare_add(desktop->exec);
	if (desktop->comment)
	  EVRY_ITEM(app)->detail = eina_stringshare_add(desktop->comment);
	else if (desktop->generic_name)
	  EVRY_ITEM(app)->detail = eina_stringshare_add(desktop->generic_name);
     }
   else
     {
	evry_item_new(EVRY_ITEM(app), EVRY_PLUGIN(p), file, _item_free);
	EVRY_ITEM(app)->id = eina_stringshare_add(file);
	/* EVRY_ITEM(app)->detail = eina_stringshare_add(file);	 */
     }
   
   app->desktop = desktop;

   eina_hash_add(p->added, exe, app);

   if (desktop)
     {
	const char *tmp = ecore_file_file_get(desktop->exec);

	if (tmp && strcmp(exe, tmp))
	  {
	     evry_item_ref(EVRY_ITEM(app));
	     eina_hash_add(p->added, tmp, app);
	  }
     }

   if (file)
     {
	app->file = eina_stringshare_add(file);

	if (strcmp(exe, file))
	  {
	     evry_item_ref(EVRY_ITEM(app));
	     eina_hash_add(p->added, file, app);
	  }
     }
   
   EVRY_ITEM(app)->fuzzy_match = match;
   EVRY_PLUGIN_ITEM_APPEND(p, app);

   return app;
}

static void
_add_desktop_list(Plugin *p, Eina_List *apps, const char *input)
{
   Efreet_Desktop *desktop;
   Eina_List *l;
   int m1, m2;

   EINA_LIST_FOREACH(apps, l, desktop)
     {
	if (eina_list_count(EVRY_PLUGIN(p)->items) > 199) break;
	if (!desktop->name || !desktop->exec) continue;

	if (input)
	  {
	     char *exec = strrchr(desktop->exec, '/');
	     if (!exec++ || !exec) exec = desktop->exec;

	     m1 = evry_fuzzy_match(exec, input);
	     m2 = evry_fuzzy_match(desktop->name, input);

	     if (!m1 || (m2 && m2 < m1))
	       m1 = m2;
	  }
	
	if (!input || m1) _item_add(p, desktop, NULL, m1);
     }
}

static int
_cb_sort(const void *data1, const void *data2)
{
   const Evry_Item *it1 = data1;
   const Evry_Item *it2 = data2;

   if (it1->usage && it2->usage)
     return (it1->usage > it2->usage ? -1 : 1);
   if (it1->usage && !it2->usage)
     return -1;
   if (it2->usage && !it1->usage)
     return 1;

   if (it1->fuzzy_match || it2->fuzzy_match)
     {
	if (it1->fuzzy_match && !it2->fuzzy_match)
	  return -1;

	if (!it1->fuzzy_match && it2->fuzzy_match)
	  return 1;

	if (it1->fuzzy_match - it2->fuzzy_match)
	  return (it1->fuzzy_match - it2->fuzzy_match);
     }
   
   return 0;
}

static Eina_Bool
_hist_items_add_cb(const Eina_Hash *hash, const void *key, void *data, void *fdata)
{
   History_Entry *he = data;
   History_Item *hi;
   Plugin *p = fdata;
   Efreet_Desktop *d;
   Eina_List *l;
   Evry_Item_App *app;

   EINA_LIST_FOREACH(he->items, l, hi)
     {
	if (hi->plugin != p->base.name)
	  continue;
	app = NULL;

	/* ignore executables for parameter */
	if (!strncmp(key, "_", 1))
	  continue;
	
	if ((d = efreet_util_desktop_exec_find(key)))
	  {
	     app = _item_add(p, d, NULL, 1);
	  }
	else
	  {	     
	     app = _item_add(p, NULL, (char *) key, 1);
	     if (app && app->desktop)
	       efreet_desktop_ref(app->desktop);
	  }
      
	if (app && app->desktop)
	  {
	     p->apps_hist = eina_list_append(p->apps_hist, app->desktop);
	  }

	if (app) break;
     }
   return EINA_TRUE;
}

static void
_cb_free_item_changed(void *data, void *event)
{
   Evry_Event_Item_Changed *ev = event;

   evry_item_free(ev->item);
   E_FREE(ev);
}

/* TODO make this an option */
static void
_add_executables(Plugin *p, const char *input)
{

   Evry_Item_App *app;
   Evry_Item_App *app2;
   Evry_Event_Item_Changed *ev;
   Eina_List *l;
   char buf[256];
   char *space, *file;   
   int found_app = 0;
   int found_cmd = 0;
   int len;

   if (!input) goto end;
   
   if ((space = strchr(input, ' ')))
     len = (space - input);
   else
     len = strlen(input);
     
   if (len < 2) goto end;
   
   EINA_LIST_FOREACH(exe_list, l, file)
     {
	if (strncmp(file, input, len)) continue;
	
	if (!(app = _item_add(p, NULL, file, 100)))
	  continue;

	if ((space) && (app->desktop))
	  {
		       
	     /* restore old desktop entry */
	     if (p->app_parameter)
	       {
		  app2 = p->app_parameter;
		  eina_stringshare_del(app2->file);
		  app2->file = NULL;
		  eina_stringshare_del(EVRY_ITEM(app2)->label);

		  if (p->app_parameter != app)
		    EVRY_ITEM(app2)->label = eina_stringshare_add(app2->desktop->name);
	       }

	     if (p->app_parameter != app)
	       eina_stringshare_del(EVRY_ITEM(app)->label);

	     p->app_parameter = app;

	     snprintf(buf, sizeof(buf), "%s %s",  app->desktop->name, space);
	     EVRY_ITEM(app)->label = eina_stringshare_add(buf);

	     snprintf(buf, sizeof(buf), "%s %s",  file, space);
	     app->file = eina_stringshare_add(buf);

	     if (!eina_list_data_find(p->base.items, app))
	       EVRY_PLUGIN_ITEM_APPEND(p, app);

	     ev = E_NEW(Evry_Event_Item_Changed, 1);
	     evry_item_ref(EVRY_ITEM(app));
	     ev->item = EVRY_ITEM(app);
	     ecore_event_add(EVRY_EVENT_ITEM_CHANGED, ev, _cb_free_item_changed, NULL);
	     found_app = 1;
	  }

	if (app->desktop)
	  {
	     snprintf(buf, sizeof(buf), "_%s_", file);
	     app = _item_add(p, NULL, buf, 100);
	     if (!app) continue;
		  
	     eina_stringshare_del(EVRY_ITEM(app)->label);
	     eina_stringshare_del(app->file);

	     EVRY_ITEM(app)->label = eina_stringshare_add(file);
	     app->file = eina_stringshare_add(file);

	     if (space && !found_cmd)
	       {
		  /* restore old desktop entry */
		  if (p->app_command)
		    {
		       app2 = p->app_command;
		       eina_stringshare_del(app2->file);
		       app2->file = NULL;
		       eina_stringshare_del(EVRY_ITEM(app2)->label);

		       if (p->app_command != app)
			 {
			    EVRY_ITEM(app2)->label = eina_stringshare_add(file);
			 }
		    }

		  if (p->app_command != app)
		    eina_stringshare_del(EVRY_ITEM(app)->label);

		  p->app_command = app;

		  snprintf(buf, sizeof(buf), "%s %s", file, space);
		  EVRY_ITEM(app)->label = eina_stringshare_add(buf);
		       
		  app->file = eina_stringshare_add(buf);

		  if (!eina_list_data_find(p->base.items, app))
		    EVRY_PLUGIN_ITEM_APPEND(p, app);

		  ev = E_NEW(Evry_Event_Item_Changed, 1);
		  evry_item_ref(EVRY_ITEM(app));
		  ev->item = EVRY_ITEM(app);
		  ecore_event_add(EVRY_EVENT_ITEM_CHANGED, ev, _cb_free_item_changed, NULL);
		  found_cmd = 1;
	       }
	  }
     }

 end:
   
   if (!found_app && p->app_parameter)
     {
	/* restore old desktop entry */
	app2 = p->app_parameter;
	eina_stringshare_del(app2->file);
	app2->file = NULL;
	eina_stringshare_del(EVRY_ITEM(app2)->label);
	EVRY_ITEM(app2)->label = eina_stringshare_add(app2->desktop->name);
	p->app_parameter = NULL;
     }
   
   if (!found_cmd && p->app_command)
     {
	eina_hash_del_by_data(p->added, p->app_command); 
	p->app_command = NULL;
     }
}


static int
_fetch(Evry_Plugin *plugin, const char *input)
{
   PLUGIN(p, plugin);
   Eina_List *l, *ll, *previous;
   Efreet_Desktop *desktop;
   Evry_Item *it;

   plugin->changed = 0;
   
   previous = plugin->items;
   plugin->items = NULL;
   
   /* add apps for a given mimetype */
   if (plugin->type == type_action)
     _add_desktop_list(p, p->apps_mime, input);

   /* add apps matching input */
   if (input)
     {
	if (!p->apps_all)
	  {
	     Eina_List *apps = NULL;
	     Eina_List *cat_ss;
	     Eina_List *l, *ll;

	     apps = efreet_util_desktop_name_glob_list("*");

	     /* remove screensaver */
	     cat_ss = efreet_util_desktop_category_list("Screensaver");
	     EINA_LIST_FOREACH(cat_ss, l, desktop)
	       {
		  if ((ll = eina_list_data_find_list(apps, desktop)))
		    {
		       efreet_desktop_free(desktop);
		       apps = eina_list_remove_list(apps, ll);
		    }
		  efreet_desktop_free(desktop);
	       }

	     p->apps_all = apps;
	  }

	_add_desktop_list(p, p->apps_all, input);
     }
   /* add exe history items */
   else if (!plugin->items)
     {
	if (!p->apps_hist)
	  eina_hash_foreach(evry_hist->subjects, _hist_items_add_cb, p);
	else
	  _add_desktop_list(p, p->apps_hist, NULL);
     }
   
   /* add executables */
   _add_executables(p, input);

   if (!plugin->items)
     {
	plugin->items = previous;
	EVRY_PLUGIN_ITEMS_CLEAR(p);
	return 0;
     }
   
   if (plugin->type == type_action)
     {
	EINA_LIST_FOREACH(plugin->items, l, it)
	  evry_history_item_usage_set(evry_hist->actions, it, input, NULL); 
     }
   else
     {
	EINA_LIST_FOREACH(plugin->items, l, it)
	  evry_history_item_usage_set(evry_hist->subjects, it, input, NULL);
     }
   
   if (plugin->type != type_action || input)
     EVRY_PLUGIN_ITEMS_SORT(plugin, _cb_sort);

   for (l = previous, ll = plugin->items; l && ll; l = l->next, ll = ll->next)
     {
	if (l->data != ll->data)
	  {
	     plugin->changed = 1;
	     break;
	  }
     }
   if (l || ll) plugin->changed = 1;

   if (previous) eina_list_free(previous);
   
   return 1;
}


static int
_complete(Evry_Plugin *plugin, const Evry_Item *it, char **input)
{
   ITEM_APP(app, it);
   
   char buf[128];

   if (app->desktop)
     {
	char *space = strchr(app->desktop->exec, ' ');
	
	snprintf(buf, sizeof(buf), "%s ", app->desktop->exec);
	if (space)
	  buf[1 + space - app->desktop->exec] = '\0';
     }
   else
     snprintf(buf, sizeof(buf), "%s ", app->file);

   *input = strdup(buf);
     
   return EVRY_COMPLETE_INPUT;
}

static Evas_Object *
_icon_get(Evry_Plugin *p __UNUSED__, const Evry_Item *it, Evas *e)
{
   ITEM_APP(app, it);
   Evas_Object *o = NULL;

   if (app->desktop)
     o = e_util_desktop_icon_add(app->desktop, 64, e);

   if (!o)
     o = evry_icon_theme_get("system-run", e);

   return o;
}

static int
_exec_app_check_item(Evry_Action *act __UNUSED__, const Evry_Item *it)
{
   /* ITEM_APP(app, it); */

   /* if (app->desktop)
    *   return 1; */

   /* run in terminal or do a .desktop entry! it's easy now */
   /* if (app->file && strlen(app->file) > 0)
    *   return 1; */

   return 1;
}

static int
_exec_app_action(Evry_Action *act)
{
   return evry_util_exec_app(act->item1, act->item2);
}

static int
_exec_term_action(Evry_Action *act)
{
   ITEM_APP(app, act->item1);
   Evry_Item_App *tmp;
   char buf[1024];
   int ret;
   char *escaped = ecore_file_escape_name(app->file);
   
   tmp = E_NEW(Evry_Item_App, 1);
   snprintf(buf, sizeof(buf), "%s -hold -e %s",
	    evry_conf->cmd_terminal,
	    (escaped ? escaped : app->file));

   tmp->file = buf;
   ret = evry_util_exec_app(EVRY_ITEM(tmp), NULL);

   E_FREE(tmp);
   E_FREE(escaped);
   
   return ret;
}

static int
_exec_term_check_item(Evry_Action *act __UNUSED__, const Evry_Item *it)
{
   ITEM_APP(app, it);

   if (app->file)
     return 1;

   return 0;
}

static int
_exec_sudo_action(Evry_Action *act)
{
   ITEM_APP(app, act->item1);
   Evry_Item_App *tmp;
   char buf[1024];
   int ret;

   tmp = E_NEW(Evry_Item_App, 1);
   snprintf(buf, sizeof(buf), "%s %s",
	    evry_conf->cmd_sudo,
	    (app->desktop ? app->desktop->exec : app->file));

   tmp->file = buf;
   ret = evry_util_exec_app(EVRY_ITEM(tmp), NULL);

   E_FREE(tmp);

   return ret;
}

static int
_open_with_action(Evry_Plugin *plugin, const Evry_Item *it)
{
   PLUGIN(p, plugin);

   if (p->candidate)
     return evry_util_exec_app(it, p->candidate);

   return 0;
}

static int
_edit_app_check_item(Evry_Action *act __UNUSED__, const Evry_Item *it)
{
   ITEM_APP(app, it);

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
	/* XXX check if this is freed by efreet*/
	desktop->exec = strdup(app->file);
     }

   e_desktop_edit(e_container_current_get(e_manager_current_get()), desktop);

   return 1;
}

static int
_new_app_check_item(Evry_Action *act __UNUSED__, const Evry_Item *it)
{
   ITEM_APP(app, it);

   if (app->desktop)
     return 1;

   if (app->file && strlen(app->file) > 0)
     return 1;

   return 0;
}

static int
_new_app_action(Evry_Action *act)
{
   char *name;
   char buf[4096];
   char *end;
   Efreet_Desktop *desktop;
   int i;

   ITEM_APP(app, act->item1);

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
	desktop = efreet_desktop_new(buf);
     }

   e_desktop_edit(e_container_current_get(e_manager_current_get()), desktop);

   return 1;
}

//#define TIME_FACTOR(_now) (1.0 - (evry_hist->begin / _now)) / 1000000000000000.0

static void
_free_plugin(Evry_Plugin *plugin)
{
   PLUGIN(p, plugin);

   E_FREE(p);
}


static Eina_Bool
module_init(void)
{
   if (!evry_api_version_check(EVRY_API_VERSION))
     return EINA_FALSE;

   p1 = E_NEW(Plugin, 1);
   EVRY_PLUGIN_NEW(EVRY_PLUGIN(p1), "Applications", type_subject, "", "APPLICATION",
	      _begin, _cleanup, _fetch, _icon_get, _free_plugin);
   EVRY_PLUGIN(p1)->complete = &_complete;
   
   p2 = E_NEW(Plugin, 1);
   EVRY_PLUGIN_NEW(EVRY_PLUGIN(p2), "Open With...", type_action, "FILE", "",
	      _begin_open_with, _cleanup, _fetch,
	      _icon_get, _free_plugin);

   EVRY_PLUGIN(p2)->action = &_open_with_action;

   evry_plugin_register(EVRY_PLUGIN(p1), 1);
   evry_plugin_register(EVRY_PLUGIN(p2), 1);

   act = EVRY_ACTION_NEW("Launch", "APPLICATION", NULL,
		    "everything-launch", _exec_app_action, _exec_app_check_item);

   act1 = EVRY_ACTION_NEW("Open File...", "APPLICATION", "FILE", "document-open",
		     _exec_app_action, _exec_app_check_item);

   act2 = EVRY_ACTION_NEW("Run in Terminal", "APPLICATION", NULL, "system-run",
		     _exec_term_action, _exec_term_check_item);

   act3 = EVRY_ACTION_NEW("Edit Application Entry", "APPLICATION", NULL, "everything-launch",
		     _edit_app_action, _edit_app_check_item);

   act4 = EVRY_ACTION_NEW("New Application Entry", "APPLICATION", NULL, "everything-launch",
		     _new_app_action, _new_app_check_item);

   act5 = EVRY_ACTION_NEW("Run with Sudo", "APPLICATION", NULL, "system-run",
		     _exec_sudo_action, NULL);

   evry_action_register(act,  0);
   evry_action_register(act1, 1);
   evry_action_register(act2, 2);
   evry_action_register(act3, 3);
   evry_action_register(act4, 4);
   evry_action_register(act5, 5);

   /* taken from e_exebuf.c */
   exelist_exe_edd = E_CONFIG_DD_NEW("E_Exe", E_Exe);
#undef T
#undef D
#define T E_Exe
#define D exelist_exe_edd
   E_CONFIG_VAL(D, T, path, STR);

   exelist_edd = E_CONFIG_DD_NEW("E_Exe_List", E_Exe_List);
#undef T
#undef D
#define T E_Exe_List
#define D exelist_edd
   E_CONFIG_LIST(D, T, list, exelist_exe_edd);

   return EINA_TRUE;
}

static void
module_shutdown(void)
{
   EVRY_PLUGIN_FREE(p1);
   EVRY_PLUGIN_FREE(p2);

   evry_action_free(act);
   evry_action_free(act1);
   evry_action_free(act2);
   evry_action_free(act3);
   evry_action_free(act4);
   evry_action_free(act5);
}

/* taken from e_exebuf.c */
static int
_scan_idler(void *data)
{
   struct stat st;
   struct dirent *dp;
   char *dir;
   char buf[4096];

   /* no more path items left - stop scanning */
   if (!exe_path)
     {
	Eina_List *l, *l2;
	E_Exe_List *el;
	E_Exe *ee;
	int different = 0;

	/* FIXME: check wheter they match or not */
	for (l = exe_list, l2 = exe_list2; l && l2; l = l->next, l2 = l2->next)
	  {
	     if (strcmp(l->data, l2->data))
	       {
		  different = 1;
		  break;
	       }
	  }
	if ((l) || (l2)) different = 1;
	if (exe_list2)
	  {
	     while (exe_list)
	       {
		  free(eina_list_data_get(exe_list));
		  exe_list = eina_list_remove_list(exe_list, exe_list);
	       }
	     exe_list = exe_list2;
	     exe_list2 = NULL;
	  }
	if (different)
	  {
	     el = calloc(1, sizeof(E_Exe_List));
	     if (el)
	       {
		  el->list = NULL;
		  for (l = exe_list; l; l = l->next)
		    {
		       ee = malloc(sizeof(E_Exe));
		       if (ee)
			 {
			    ee->path = eina_stringshare_add(l->data);
			    el->list = eina_list_append(el->list, ee);
			 }
		    }
		  e_config_domain_save("exebuf_exelist_cache", exelist_edd, el);
		  while (el->list)
		    {
		       ee = eina_list_data_get(el->list);
		       eina_stringshare_del(ee->path);
		       free(ee);
		       el->list = eina_list_remove_list(el->list, el->list);
		    }
		  free(el);
	       }
	  }
	exe_scan_idler = NULL;
	return 0;
     }
   /* no dir is open - open the first path item */
   if (!exe_dir)
     {
	dir = exe_path->data;
	exe_dir = opendir(dir);
     }
   /* if we have an opened dir - scan the next item */
   if (exe_dir)
     {
	dir = exe_path->data;

	dp = readdir(exe_dir);
	if (dp)
	  {
	     if ((strcmp(dp->d_name, ".")) && (strcmp(dp->d_name, "..")))
	       {
		  snprintf(buf, sizeof(buf), "%s/%s", dir, dp->d_name);
		  if ((stat(buf, &st) == 0) &&
		      ((!S_ISDIR(st.st_mode)) &&
		       (!access(buf, X_OK))))
		    {
		       if (!exe_list)
			 exe_list = eina_list_append(exe_list, strdup(dp->d_name));
		       else
			 exe_list2 = eina_list_append(exe_list2, strdup(dp->d_name));
		    }
	       }
	  }
	else
	  {
	     /* we reached the end of a dir - remove the dir at the head
	      * of the path list so we advance and next loop we will pick up
	      * the next item, or if null- abort
	      */
	     closedir(exe_dir);
	     exe_dir = NULL;
	     free(eina_list_data_get(exe_path));
	     exe_path = eina_list_remove_list(exe_path, exe_path);
	  }
     }
   /* obviously the dir open failed - so remove the first path item */
   else
     {
	free(eina_list_data_get(exe_path));
	exe_path = eina_list_remove_list(exe_path, exe_path);
     }
   /* we have mroe scannign to do */
   return 1;
}


/***************************************************************************/

static E_Config_DD *conf_edd = NULL;

static Eina_Bool active = EINA_FALSE;

EAPI E_Module_Api e_modapi = 
{
   E_MODULE_API_VERSION,
   "everything-apps"
};

struct _E_Config_Dialog_Data 
{
  int list_executables;
};

static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static void _fill_data(E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);

E_Config_Dialog *
_conf_dialog(E_Container *con, const char *params) 
{
   E_Config_Dialog *cfd = NULL;
   E_Config_Dialog_View *v = NULL;
   char buf[4096];

   if (e_config_dialog_find("everything-apps", "advanced/everything-apps")) return NULL;

   v = E_NEW(E_Config_Dialog_View, 1);
   if (!v) return NULL;

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;
   v->basic.apply_cfdata = _basic_apply;

   snprintf(buf, sizeof(buf), "%s/e-module.edj", _conf->module->dir);

   cfd = e_config_dialog_new(con, _("Everything Applications"), "everything-apps", 
                             "advanced/everything-apps", buf, 0, v, NULL);

   e_dialog_resizable_set(cfd->dia, 1);
   _conf->cfd = cfd;
   return cfd;
}

/* Local Functions */
static void *
_create_data(E_Config_Dialog *cfd) 
{
   E_Config_Dialog_Data *cfdata = NULL;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _fill_data(cfdata);
   return cfdata;
}

static void 
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   _conf->cfd = NULL;
   E_FREE(cfdata);
}

static void 
_fill_data(E_Config_Dialog_Data *cfdata) 
{
   cfdata->list_executables = _conf->list_executables;
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o = NULL, *of = NULL, *ow = NULL;

   o = e_widget_list_add(evas, 0, 0);

   of = e_widget_framelist_add(evas, _("General"), 0);
   e_widget_framelist_content_align_set(of, 0.0, 0.0);
   ow = e_widget_check_add(evas, _("Show Executables"), 
                           &(cfdata->list_executables));
   e_widget_framelist_object_append(of, ow);
   e_widget_list_object_append(o, of, 1, 1, 0.5);

   return o;
}

static int 
_basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   _conf->list_executables = cfdata->list_executables;
   e_config_domain_save("module.everything-apps", conf_edd, _conf);

   e_config_save_queue();
   return 1;
}

/***************************************************************************/

static void 
_conf_new(void)
{
   _conf = E_NEW(Module_Config, 1);
   _conf->version = (MOD_CONFIG_FILE_EPOCH << 16);

#define IFMODCFG(v) if ((_conf->version & 0xffff) < v) {
#define IFMODCFGEND }

   /* setup defaults */
   IFMODCFG(0x008d);
   _conf->list_executables = 1;
   IFMODCFGEND;

   _conf->version = MOD_CONFIG_FILE_VERSION;

   e_config_save_queue();
}

EAPI void *
e_modapi_init(E_Module *m)
{
   char buf[4096];
   
   if (e_datastore_get("everything_loaded"))
     active = module_init();

   snprintf(buf, sizeof(buf), "%s/e-module.edj", m->dir);

   e_configure_registry_category_add("extensions", 80, _("Extensions"),
				     NULL, "preferences-extensions");

   e_configure_registry_item_add("extensions/everything-apps", 110, _("Everything Applications"), 
                                 NULL, buf, _conf_dialog);

   conf_edd = E_CONFIG_DD_NEW("Module_Config", Module_Config);

#undef T
#undef D
#define T Module_Config
#define D conf_edd
   E_CONFIG_VAL(D, T, version, INT);
   E_CONFIG_VAL(D, T, list_executables, UCHAR);
#undef T
#undef D

   _conf = e_config_domain_load("module.everything-apps", conf_edd);

   if (_conf) 
     {
	if (!evry_util_module_config_check
	    (_("Everything Applications"), _conf->version,
	     MOD_CONFIG_FILE_EPOCH, MOD_CONFIG_FILE_VERSION))
	  {
	     E_FREE(_conf);
	  }
     }

   if (!_conf) _conf_new();

   _conf->module = m;
   
   e_module_delayed_set(m, 1); 

   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   if (active && e_datastore_get("everything_loaded"))
     module_shutdown();

   E_CONFIG_DD_FREE(exelist_edd);
   E_CONFIG_DD_FREE(exelist_exe_edd);


   E_FREE(_conf);

   E_CONFIG_DD_FREE(conf_edd);

   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   return 1;
}

/**/
/***************************************************************************/

