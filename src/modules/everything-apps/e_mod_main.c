/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */

#include "e.h"
#include "e_mod_main.h"
#include "evry_api.h"
#include "e_mod_main.h"

typedef struct _Plugin Plugin;
typedef struct _Module_Config Module_Config;
typedef struct _E_Exe E_Exe;
typedef struct _E_Exe_List E_Exe_List;

struct _Plugin
{
  Evry_Plugin	 base;
  Eina_List	*apps_mime;
  Eina_List	*apps_all;
  Eina_List	*apps_hist;

  Eina_Hash	*added;

  Evry_Item_App *app_command;
  Evry_Item_App *app_parameter;
};

struct _E_Exe
{
  const char	*path;
};

struct _E_Exe_List
{
  Eina_List	*list;
};

struct _Module_Config
{
  int		   version;
  unsigned char	   list_executables;
  const char      *cmd_terminal;
  const char      *cmd_sudo;

  E_Config_Dialog *cfd;
  E_Module	  *module;
};

static const Evry_API   *evry            = NULL;
static Evry_Module      *evry_module     = NULL;

static Module_Config	*_conf;
static Evry_Plugin	*plug_apps	 = NULL;
static Evry_Plugin	*plug_apps2	 = NULL;
static Evry_Plugin	*plug_action	 = NULL;
static Eina_List	*_actions	 = NULL;
static Evry_Item	*_act_open_with  = NULL;
static Eina_List	*exe_path	 = NULL;
static Ecore_Idler	*exe_scan_idler	 = NULL;
static E_Config_DD	*exelist_exe_edd = NULL;
static E_Config_DD	*exelist_edd	 = NULL;
static DIR		*exe_dir	 = NULL;
static Eina_List	*exe_list	 = NULL;
static Eina_List	*exe_list2	 = NULL;

static void _scan_executables();


static void _hash_free(void *data)
{
   GET_APP(app, data);
   EVRY_ITEM_FREE(app);
}

static Evry_Plugin *
_begin_open_with(Evry_Plugin *plugin, const Evry_Item *item)
{
   Plugin *p = NULL;

   /* GET_PLUGIN(p, plugin); */

   Efreet_Desktop *d, *d2;
   const char *mime;
   const char *path = NULL;
   Eina_List *l;

   if (CHECK_TYPE(item, EVRY_TYPE_ACTION))
     {
	GET_ACTION(act, item);
	GET_FILE(file, act->it1.item);

	if (!evry->file_path_get(file))
	  return NULL;

	path = file->path;
	mime = file->mime;
     }
   else if (CHECK_TYPE(item, EVRY_TYPE_FILE))
     {
	GET_FILE(file, item);

	if (!evry->file_path_get(file))
	  return NULL;

	path = file->path;
	mime = file->mime;
     }
   else
     {
	return NULL;
     }

   if (!path || !mime || !(mime = efreet_mime_type_get(path)))
     return NULL;

   p = E_NEW(Plugin, 1);
   p->base = *plugin;
   p->base.items = NULL;

   p->apps_mime = efreet_util_desktop_mime_list(mime);

   if (strcmp(mime, "text/plain") && (!strncmp(mime, "text/", 5)))
     {
	l = efreet_util_desktop_mime_list("text/plain");

	EINA_LIST_FREE(l, d)
	  {
	     if (!eina_list_data_find_list(p->apps_mime, d))
	       p->apps_mime = eina_list_append(p->apps_mime, d);
	     else
	       efreet_desktop_free(d);
	  }
     }

   if (item->browseable && strcmp(mime, "x-directory/normal"))
     {
	l = efreet_util_desktop_mime_list("x-directory/normal");

	EINA_LIST_FREE(l, d)
	  {
	     if (!eina_list_data_find_list(p->apps_mime, d))
	       p->apps_mime = eina_list_append(p->apps_mime, d);
	     else
	       efreet_desktop_free(d);
	  }
     }

   if ((d = e_exehist_mime_desktop_get(mime)))
     {
	if ((d2 = eina_list_data_find(p->apps_mime, d)))
	  {
	     p->apps_mime = eina_list_remove(p->apps_mime, d2);
	     efreet_desktop_free(d2);
	  }
	p->apps_mime = eina_list_prepend(p->apps_mime, d);
     }


   p->added = eina_hash_string_small_new(_hash_free);

   return EVRY_PLUGIN(p);
}

static Evry_Plugin *
_begin(Evry_Plugin *plugin, const Evry_Item *item)
{
   Plugin *p;

   p = E_NEW(Plugin, 1);
   p->base = *plugin;
   p->base.items = NULL;

   if (item && (item != _act_open_with))
     return NULL;

   p->added = eina_hash_string_small_new(_hash_free);

   if (_conf->list_executables)
     _scan_executables();

   return EVRY_PLUGIN(p);
}


static void
_finish(Evry_Plugin *plugin)
{
   GET_PLUGIN(p, plugin);
   Efreet_Desktop *desktop;
   char *str;

   if (p->added)
     eina_hash_free(p->added);

   EVRY_PLUGIN_ITEMS_CLEAR(p);

   EINA_LIST_FREE(p->apps_all, desktop)
     efreet_desktop_free(desktop);

   EINA_LIST_FREE(p->apps_hist, desktop)
     efreet_desktop_free(desktop);

   EINA_LIST_FREE(p->apps_mime, desktop)
     efreet_desktop_free(desktop);

   if (_conf->list_executables)
     {
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

   E_FREE(p);
}

static void
_finish_mime(Evry_Plugin *plugin)
{
   GET_PLUGIN(p, plugin);
   Efreet_Desktop *desktop;

   if (p->added)
     eina_hash_free(p->added);

   EVRY_PLUGIN_ITEMS_CLEAR(p);

   EINA_LIST_FREE(p->apps_mime, desktop)
     efreet_desktop_free(desktop);
}

static int
_exec_open_file_action(Evry_Action *act)
{
   return evry->util_exec_app(EVRY_ITEM(act), act->it1.item);
}

static Evas_Object *
_icon_get(Evry_Item *it, Evas *e)
{
   GET_APP(app, it);
   Evas_Object *o = NULL;

   if (app->desktop)
     {
	o = evry->icon_theme_get(app->desktop->icon, e);

	if (!o)
	  o = e_util_desktop_icon_add(app->desktop, 128, e);
     }
   
   if (!o)
     o = evry->icon_theme_get("system-run", e);

   return o;
}

static void
_item_free(Evry_Item *item)
{
   GET_APP(app, item);

   
   if (app->desktop)
     efreet_desktop_free(app->desktop);
   if (app->file)
     eina_stringshare_del(app->file);

   E_FREE(app);
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
     {
	file = NULL;
	exe = desktop->exec;
     }
   else
     {
	exe = file;
     }

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
		  EVRY_PLUGIN_ITEM_APPEND(p, app);
	       }
	     return app;
	  }
     }
   if (desktop && !already_refd)
     efreet_desktop_ref(desktop);

   if (desktop)
     {
	app = EVRY_ITEM_NEW(Evry_Item_App, p, desktop->name, _icon_get, _item_free);

	EVRY_ACTN(app)->action = &_exec_open_file_action;
	EVRY_ITEM(app)->id = eina_stringshare_add(desktop->exec);

	if (desktop->comment)
	  EVRY_ITEM(app)->detail = eina_stringshare_add(desktop->comment);
	else if (desktop->generic_name)
	  EVRY_ITEM(app)->detail = eina_stringshare_add(desktop->generic_name);
     }
   else
     {
	app = EVRY_ITEM_NEW(Evry_Item_App, p, file, _icon_get, _item_free);
	EVRY_ACTN(app)->action = &_exec_open_file_action;
	EVRY_ITEM(app)->id = eina_stringshare_add(file);
     }

   app->desktop = desktop;

   eina_hash_add(p->added, exe, app);

   if (desktop)
     {
	const char *tmp = ecore_file_file_get(desktop->exec);

	if (tmp && strcmp(exe, tmp))
	  {
	     EVRY_ITEM_REF(app);
	     eina_hash_add(p->added, tmp, app);
	  }
     }

   if (file)
     {
	app->file = eina_stringshare_add(file);

	if (strcmp(exe, file))
	  {
	     evry->item_ref(EVRY_ITEM(app));
	     eina_hash_add(p->added, file, app);
	  }
     }

   EVRY_ACTN(app)->remember_context = EINA_TRUE;
   EVRY_ITEM(app)->subtype = EVRY_TYPE_ACTION;
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
   const char *exec, *end;
   char buf[PATH_MAX];

   EINA_LIST_FOREACH(apps, l, desktop)
     {
	if (eina_list_count(EVRY_PLUGIN(p)->items) > 199) break;
	if (!desktop->name || !desktop->exec) continue;

	if (input)
	  {
	     m1 = m2 = 0;

	     exec = ecore_file_file_get(desktop->exec);
	     if (exec && (end = strchr(exec, '%')))
	       {
		  strncpy(buf, exec, (end - exec) - 1);
		  buf[(end - exec)-1] = '\0';
		  m1 = evry->fuzzy_match(buf, input);
	       }
	     else
	       {
		  m1 = evry->fuzzy_match(exec, input);
	       }

	     m2 = evry->fuzzy_match(desktop->name, input);

	     if (!m1 || (m2 && m2 < m1))
	       m1 = m2;

	     if (m1) _item_add(p, desktop, NULL, m1);
	  }
	else
	  {
	     _item_add(p, desktop, NULL, 0);
	  }
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
	app = NULL;

	/* ignore executables for parameter */
	if (!strncmp(key, "_", 1))
	  continue;

	if ((d = efreet_util_desktop_exec_find(key)))
	  {
	     app = _item_add(p, d, NULL, 0);
	  }
	else
	  {
	     app = _item_add(p, NULL, (char *) key, 0);
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

/* TODO make this an option */
static void
_add_executables(Plugin *p, const char *input)
{

   Evry_Item_App *app;
   Evry_Item_App *app2;
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

	     evry->item_changed(EVRY_ITEM(app), 0, 0);
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

		  evry->item_changed(EVRY_ITEM(app), 0, 0);
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
_fetch_mime(Evry_Plugin *plugin, const char *input)
{
   GET_PLUGIN(p, plugin);
   Eina_List *l;
   Evry_Item *it;

   EVRY_PLUGIN_ITEMS_CLEAR(p);

   /* add apps for a given mimetype */
   _add_desktop_list(p, p->apps_mime, input);

   EINA_LIST_FOREACH(plugin->items, l, it)
     evry->history_item_usage_set(it, input, NULL);

   if (input)
     EVRY_PLUGIN_ITEMS_SORT(plugin, _cb_sort);

   return 1;
}

static int
_fetch(Evry_Plugin *plugin, const char *input)
{
   GET_PLUGIN(p, plugin);
   Eina_List *l;
   Efreet_Desktop *desktop;
   Evry_Item *it;

   EVRY_PLUGIN_ITEMS_CLEAR(p);

   if (input && !(p->apps_all))
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

   if (input)
     {
	/* .desktop files */
	_add_desktop_list(p, p->apps_all, input);

	/* add executables */
	_add_executables(p, input);
     }
   else
     {
	/* add matching items */
	_add_desktop_list(p, p->apps_mime, input);
     }

   EINA_LIST_FOREACH(plugin->items, l, it)
     evry->history_item_usage_set(it, input, NULL);

   EVRY_PLUGIN_ITEMS_SORT(plugin, _cb_sort);

   if (!input && !plugin->items)
     {

	/* add history items */
	if (!p->apps_hist)
	  {
	     History_Types *ht;
	     ht = evry->history_types_get(EVRY_TYPE_APP);
	     if (ht)
	       eina_hash_foreach(ht->types, _hist_items_add_cb, p);
	  }
	else
	  _add_desktop_list(p, p->apps_hist, NULL);
     }

   return !!(plugin->items);
}

static int
_complete(Evry_Plugin *plugin, const Evry_Item *it, char **input)
{
   GET_APP(app, it);

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

static int
_exec_app_check_item(Evry_Action *act, const Evry_Item *it)
{
   return 1;
}

static int
_exec_app_action(Evry_Action *act)
{
   return evry->util_exec_app(act->it1.item, act->it2.item);
}

static int
_exec_file_action(Evry_Action *act)
{
   return evry->util_exec_app(act->it2.item, act->it1.item);
}

static int
_exec_term_action(Evry_Action *act)
{
   GET_APP(app, act->it1.item);
   Evry_Item_App *tmp;
   char buf[1024];
   int ret;
   char *escaped = ecore_file_escape_name(app->file);

   tmp = E_NEW(Evry_Item_App, 1);
   snprintf(buf, sizeof(buf), "%s -hold -e %s",
	    _conf->cmd_terminal,
	    (escaped ? escaped : app->file));

   tmp->file = buf;
   ret = evry->util_exec_app(EVRY_ITEM(tmp), NULL);

   E_FREE(tmp);
   E_FREE(escaped);

   return ret;
}

static int
_exec_term_check_item(Evry_Action *act __UNUSED__, const Evry_Item *it)
{
   GET_APP(app, it);

   if (app->file)
     return 1;

   return 0;
}

static int
_exec_sudo_action(Evry_Action *act)
{
   GET_APP(app, act->it1.item);
   Evry_Item_App *tmp;
   char buf[1024];
   int ret;

   tmp = E_NEW(Evry_Item_App, 1);
   snprintf(buf, sizeof(buf), "%s %s",
	    _conf->cmd_sudo,
	    (app->desktop ? app->desktop->exec : app->file));

   tmp->file = buf;
   ret = evry->util_exec_app(EVRY_ITEM(tmp), NULL);

   E_FREE(tmp);

   return ret;
}

static int
_edit_app_check_item(Evry_Action *act __UNUSED__, const Evry_Item *it)
{
   GET_APP(app, it);

   if (app->desktop)
     return 1;

   return 0;
}

static int
_edit_app_action(Evry_Action *act)
{
   Efreet_Desktop *desktop;
   GET_APP(app, act->it1.item);

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
   GET_APP(app, it);

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

   GET_APP(app, act->it1.item);

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
	desktop->exec = (char *)eina_stringshare_add(app->file);
     }
   else
     {
	desktop = efreet_desktop_empty_new(buf);
	if (app->desktop->name)
	  desktop->name = strdup(app->desktop->name);
	if (app->desktop->comment)
	  desktop->comment = strdup(app->desktop->comment);
	if (app->desktop->generic_name)
	  desktop->generic_name = strdup(app->desktop->generic_name);
	if (app->desktop->generic_name)
	  desktop->generic_name = strdup(app->desktop->generic_name);
	if (app->desktop->exec)
	  desktop->exec = strdup(app->desktop->exec);
	if (app->desktop->icon)
	  desktop->icon = strdup(app->desktop->icon);
	if (app->desktop->mime_types)
	  desktop->mime_types = eina_list_clone(app->desktop->mime_types);
     }
   if (desktop)
     e_desktop_edit(e_container_current_get(e_manager_current_get()), desktop);

   return 1;
}

static int
_open_term_action(Evry_Action *act)
{
   GET_FILE(file, act->it1.item);
   Evry_Item_App *tmp;
   char cwd[4096];
   char *dir;
   int ret = 0;

   if (!(evry->file_path_get(file)))
     return 0;

   if (IS_BROWSEABLE(file))
     dir = strdup(file->path);
   else
     dir = ecore_file_dir_get(file->path);

   if (dir)
     {
	if (!getcwd(cwd, sizeof(cwd)))
	  return 0;
	if (chdir(dir))
	  return 0;

	tmp = E_NEW(Evry_Item_App, 1);
	tmp->file = _conf->cmd_terminal;

	ret = evry->util_exec_app(EVRY_ITEM(tmp), NULL);
	E_FREE(tmp);
	E_FREE(dir);
	if (chdir(cwd))
	  return 0;
     }

   return ret;
}

static int
_plugins_init(const Evry_API *api)
{
   Evry_Plugin *p;
   int prio = 0;
   Eina_List *l;
   Evry_Action *act;

   if (evry_module->active)
     return EINA_TRUE;

   evry = api;

   if (!evry->api_version_check(EVRY_API_VERSION))
     return EINA_FALSE;

   p = EVRY_PLUGIN_NEW(Plugin, N_("Applications"), NULL, EVRY_TYPE_APP,
		       _begin, _finish, _fetch, NULL);
   p->complete = &_complete;
   p->config_path = "extensions/everything-apps";
   evry->plugin_register(p, EVRY_PLUGIN_SUBJECT, 1);
   plug_apps = p;

   p = EVRY_PLUGIN_NEW(Plugin, N_("Applications"), NULL, EVRY_TYPE_APP,
		       _begin_open_with, _finish, _fetch, NULL);
   p->complete = &_complete;
   p->config_path = "extensions/everything-apps";
   evry->plugin_register(p, EVRY_PLUGIN_OBJECT, 1);
   plug_apps2 = p;

   p = EVRY_PLUGIN_NEW(Plugin, N_("Open With..."), NULL, EVRY_TYPE_APP,
		       _begin_open_with, _finish_mime, _fetch_mime, NULL);
   p->config_path = "extensions/everything-apps";
   evry->plugin_register(p, EVRY_PLUGIN_ACTION, 1);
   plug_action = p;

   act = EVRY_ACTION_NEW(N_("Launch"),
			 EVRY_TYPE_APP, 0,
			 "everything-launch",
			 _exec_app_action,
			 _exec_app_check_item);
   _actions = eina_list_append(_actions, act);

   act = EVRY_ACTION_NEW(N_("Open File..."),
			 EVRY_TYPE_APP, EVRY_TYPE_FILE,
			 "document-open",
			 _exec_app_action,
			 _exec_app_check_item);
   _actions = eina_list_append(_actions, act);

   act = EVRY_ACTION_NEW(N_("Run in Terminal"),
			 EVRY_TYPE_APP, 0,
			 "system-run",
			 _exec_term_action,
			 _exec_term_check_item);
   _actions = eina_list_append(_actions, act);

   act = EVRY_ACTION_NEW(N_("Edit Application Entry"),
			 EVRY_TYPE_APP, 0,
			 "everything-launch",
			 _edit_app_action,
			 _edit_app_check_item);
   _actions = eina_list_append(_actions, act);

   act = EVRY_ACTION_NEW(N_("New Application Entry"),
			 EVRY_TYPE_APP, 0,
			 "everything-launch",
			 _new_app_action,
			 _new_app_check_item);
   _actions = eina_list_append(_actions, act);

   act = EVRY_ACTION_NEW(N_("Run with Sudo"),
			 EVRY_TYPE_APP, 0,
			 "system-run",
			 _exec_sudo_action, NULL);
   _actions = eina_list_append(_actions, act);

   act = EVRY_ACTION_NEW(N_("Open with..."),
			 EVRY_TYPE_FILE, EVRY_TYPE_APP,
			 "everything-launch",
			 _exec_file_action, NULL);
   _act_open_with = EVRY_ITEM(act);
   _actions = eina_list_append(_actions, act);

   act = EVRY_ACTION_NEW(N_("Open Terminal here"),
			 EVRY_TYPE_FILE, 0,
			 "system-run",
			 _open_term_action, NULL);
   _actions = eina_list_append(_actions, act);

   EINA_LIST_FOREACH(_actions, l, act)
     evry->action_register(act,  prio++);

   return EINA_TRUE;
}

static void
_plugins_shutdown(void)
{
   Evry_Action *act;

   if (!evry_module->active)
     return;

   EVRY_PLUGIN_FREE(plug_apps);
   EVRY_PLUGIN_FREE(plug_apps2);
   EVRY_PLUGIN_FREE(plug_action);

   EINA_LIST_FREE(_actions, act)
     evry->action_free(act);

   evry_module->active = EINA_FALSE;
}

/***************************************************************************/

static E_Config_DD *conf_edd = NULL;

struct _E_Config_Dialog_Data
{
  int list_executables;
  char *cmd_terminal;
  char *cmd_sudo;
};

static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static void _fill_data(E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);

static E_Config_Dialog *
_conf_dialog(E_Container *con, const char *params)
{
   E_Config_Dialog *cfd = NULL;
   E_Config_Dialog_View *v = NULL;
   char buf[4096];

   if (e_config_dialog_find("everything-apps", "extensions/everything-apps")) return NULL;

   v = E_NEW(E_Config_Dialog_View, 1);
   if (!v) return NULL;

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;
   v->basic.apply_cfdata = _basic_apply;

   snprintf(buf, sizeof(buf), "%s/e-module.edj", _conf->module->dir);

   cfd = e_config_dialog_new(con, _("Everything Applications"), "everything-apps",
			     "extensions/everything-apps", buf, 0, v, NULL);

   /* e_dialog_resizable_set(cfd->dia, 1); */
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

static Evas_Object *
_basic_create(E_Config_Dialog *cfd, Evas *e, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o = NULL, *of = NULL, *ow = NULL;

   o = e_widget_list_add(e, 0, 0);

   of = e_widget_framelist_add(e, _("General"), 0);
   e_widget_framelist_content_align_set(of, 0.0, 0.0);
   ow = e_widget_check_add(e, _("Show Executables"),
			   &(cfdata->list_executables));
   e_widget_framelist_object_append(of, ow);

   e_widget_list_object_append(o, of, 1, 1, 0.5);

   of = e_widget_framelist_add(e, _("Commands"), 0);
   ow = e_widget_label_add(e, _("Terminal Command"));
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_entry_add(e, &(cfdata->cmd_terminal), NULL, NULL, NULL);
   e_widget_framelist_object_append(of, ow);

   ow = e_widget_label_add(e, _("Sudo GUI"));
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_entry_add(e, &(cfdata->cmd_sudo), NULL, NULL, NULL);
   e_widget_framelist_object_append(of, ow);

   e_widget_list_object_append(o, of, 1, 1, 0.5);

   return o;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{

#define CP(_name) cfdata->_name = strdup(_conf->_name);
#define C(_name) cfdata->_name = _conf->_name;
   C(list_executables);
   CP(cmd_terminal);
   CP(cmd_sudo);
#undef CP
#undef C
}

static int
_basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
#define CP(_name)					\
   if (_conf->_name)					\
     eina_stringshare_del(_conf->_name);		\
   _conf->_name = eina_stringshare_add(cfdata->_name);
#define C(_name) _conf->_name = cfdata->_name;
   C(list_executables);
   CP(cmd_terminal);
   CP(cmd_sudo);
#undef CP
#undef C

   e_config_domain_save("module.everything-apps", conf_edd, _conf);

   /* e_config_save_queue(); */
   return 1;
}

static void
_conf_new(void)
{
   if (!_conf)
     {
	_conf = E_NEW(Module_Config, 1);
	_conf->version = (MOD_CONFIG_FILE_EPOCH << 16);
     }

#define IFMODCFG(v) if ((_conf->version & 0xffff) < v) {
#define IFMODCFGEND }

   /* setup defaults */
   IFMODCFG(0x008d);
   _conf->list_executables = 1;
   IFMODCFGEND;

   IFMODCFG(0x009d);
   _conf->cmd_terminal = eina_stringshare_add("/usr/bin/xterm");
   _conf->cmd_sudo = eina_stringshare_add("/usr/bin/gksudo --preserve-env");
   IFMODCFGEND;

   _conf->version = MOD_CONFIG_FILE_VERSION;

   /* e_config_save_queue(); */
}

static void
_conf_free(void)
{
   if (!_conf) return;

   IF_RELEASE(_conf->cmd_sudo);
   IF_RELEASE(_conf->cmd_terminal);
   E_FREE(_conf);
}

static void
_conf_init(E_Module *m)
{
   char buf[4096];
   snprintf(buf, sizeof(buf), "%s/e-module.edj", m->dir);

   e_configure_registry_category_add
     ("extensions", 80, _("Extensions"), NULL,
      "preferences-extensions");

   e_configure_registry_item_add
     ("extensions/everything-apps", 110,
      _("Everything Applications"),
      NULL, buf, _conf_dialog);

   conf_edd = E_CONFIG_DD_NEW("Module_Config", Module_Config);

#undef T
#undef D
#define T Module_Config
#define D conf_edd
   E_CONFIG_VAL(D, T, version, INT);
   E_CONFIG_VAL(D, T, list_executables, UCHAR);
   E_CONFIG_VAL(D, T, cmd_terminal, STR);
   E_CONFIG_VAL(D, T, cmd_sudo, STR);
#undef T
#undef D

   _conf = e_config_domain_load("module.everything-apps", conf_edd);


   if (_conf && !e_util_module_config_check
       (_("Everything Applications"), _conf->version,
	MOD_CONFIG_FILE_EPOCH, MOD_CONFIG_FILE_VERSION))
     _conf_free();

   _conf_new();
   _conf->module = m;
}

static void
_conf_shutdown(void)
{
   _conf_free();

   E_CONFIG_DD_FREE(conf_edd);
}

/***************************************************************************/

EAPI E_Module_Api e_modapi =
  {
    E_MODULE_API_VERSION,
    "everything-apps"
  };

EAPI void *
e_modapi_init(E_Module *m)
{
   _conf_init(m);

   evry_module = E_NEW(Evry_Module, 1);
   evry_module->init     = &_plugins_init;
   evry_module->shutdown = &_plugins_shutdown;
   EVRY_MODULE_REGISTER(evry_module);

   if ((evry = e_datastore_get("everything_loaded")))
     evry_module->active = _plugins_init(evry);

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

   e_module_delayed_set(m, 1);

   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   _plugins_shutdown();

   EVRY_MODULE_UNREGISTER(evry_module);
   E_FREE(evry_module);

   _conf_shutdown();

   E_CONFIG_DD_FREE(exelist_edd);
   E_CONFIG_DD_FREE(exelist_exe_edd);

   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   e_config_domain_save("module.everything-apps", conf_edd, _conf);
   return 1;
}

/***************************************************************************/


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

static void
_scan_executables()
{
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
}
