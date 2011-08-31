#include "e.h"
#include "evry_api.h"

#define MOD_CONFIG_FILE_EPOCH 0x0001
#define MOD_CONFIG_FILE_GENERATION 0x009d
#define MOD_CONFIG_FILE_VERSION                                 \
     ((MOD_CONFIG_FILE_EPOCH << 16) | MOD_CONFIG_FILE_GENERATION)

// FIXME clear cache on .desktop chage event

/* #undef DBG
 * #define DBG(...) ERR(__VA_ARGS__) */

#define MAX_ITEMS 200

typedef struct _Plugin Plugin;
typedef struct _Module_Config Module_Config;
typedef struct _E_Exe E_Exe;
typedef struct _E_Exe_List E_Exe_List;
typedef struct _Item_Menu Item_Menu;

struct _Plugin
{
  Evry_Plugin	 base;
  Eina_Bool      browse;
  const char    *input;
  Eina_List	*apps_mime;
  Eina_List	*apps_all;
  Eina_List	*apps_hist;
  Eina_List	*menu_items;

  Eina_Hash	*added;
  Efreet_Menu   *menu;;
  
  Evry_Item_App *command;
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
  const char      *cmd_terminal;
  const char      *cmd_sudo;

  E_Config_Dialog *cfd;
  E_Module	  *module;
};

struct _Item_Menu
{
  Evry_Item base;

  Efreet_Menu *menu;;
};

static const Evry_API   *evry            = NULL;
static Evry_Module      *evry_module     = NULL;
static Eina_List        *handlers        = NULL;

static Module_Config	*_conf;
static char _module_icon[] = "system-run";
static Eina_List 	*_plugins	 = NULL;
static Eina_List	*_actions	 = NULL;
static Evry_Item	*_act_open_with  = NULL;
static Eina_List	*exe_path	 = NULL;
static Ecore_Idler	*exe_scan_idler	 = NULL;
static E_Config_DD	*exelist_exe_edd = NULL;
static E_Config_DD	*exelist_edd	 = NULL;
static DIR		*exe_dir	 = NULL;
static Eina_List	*exe_list	 = NULL;
static Eina_List	*exe_list2	 = NULL;
static Eina_List        *apps_cache      = NULL;
static void _scan_executables();


#define GET_MENU(_m, _it) Item_Menu *_m = (Item_Menu *)_it
		  
/***************************************************************************/

static void _hash_free(void *data)
{
   GET_APP(app, data);
   EVRY_ITEM_FREE(app);
}

static int
_exec_open_file_action(Evry_Action *act)
{
   return evry->util_exec_app(EVRY_ITEM(act), act->it1.item);
}

static void
_cb_item_free(Evry_Item *item)
{
   GET_APP(app, item);

   if (app->desktop)
     efreet_desktop_free(app->desktop);
   if (app->file)
     eina_stringshare_del(app->file);

   E_FREE(app);
}

static Evry_Item_App *
_item_new(Plugin *p, const char *label, const char *id)
{
   Evry_Item_App *app;

   app = EVRY_ITEM_NEW(Evry_Item_App, p, label, NULL, _cb_item_free);
   EVRY_ACTN(app)->action = &_exec_open_file_action;
   EVRY_ACTN(app)->it1.type = EVRY_TYPE_FILE;
   EVRY_ITEM(app)->id = eina_stringshare_add(id);

   eina_hash_add(p->added, id, app);

   EVRY_ACTN(app)->remember_context = EINA_TRUE;
   EVRY_ITEM(app)->subtype = EVRY_TYPE_ACTION;

   return app;
}

static Item_Menu *
_item_menu_add(Plugin *p, Efreet_Menu *menu)
{
   Item_Menu *m;

   m = EVRY_ITEM_NEW(Item_Menu, p, NULL, NULL, NULL);
   EVRY_ITEM(m)->type = EVRY_TYPE_NONE;
   EVRY_ITEM(m)->browseable = EINA_TRUE;
   EVRY_ITEM(m)->label = eina_stringshare_add(menu->name);
   EVRY_ITEM(m)->icon = eina_stringshare_add(menu->icon);
   EVRY_ITEM(m)->usage = -1;

   m->menu = menu;
   p->menu_items = eina_list_append(p->menu_items, m); 

   return m;
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

   return strcasecmp(it1->label, it2->label);
}

/***************************************************************************/

static Evry_Item_App *
_item_exe_add(Plugin *p, const char *exe, int match)
{
   Evry_Item_App *app = NULL;

   if ((app = eina_hash_find(p->added, exe)))
     {
	if (eina_list_data_find_list(p->base.items, app))
	  return app;
     }

   if (!app)
     {
	app = _item_new(p, ecore_file_file_get(exe), exe);
	app->file = eina_stringshare_ref(EVRY_ITEM(app)->id);
     }

   EVRY_ITEM(app)->fuzzy_match = match;
   EVRY_PLUGIN_ITEM_APPEND(p, app);

   return app;
}

static Eina_Bool
_hist_exe_get_cb(const Eina_Hash *hash __UNUSED__, const void *key, void *data, void *fdata)
{
   History_Entry *he = data;
   History_Item *hi;
   Plugin *p = fdata;
   Eina_List *l;
   Evry_Item_App *app;
   int match;
   const char *exe = key;

   EINA_LIST_FOREACH(he->items, l, hi)
     {
	app = NULL;

	if (strcmp(hi->plugin, EVRY_PLUGIN(p)->name))
	  continue;

	if (!p->input)
	  {
	     app = _item_exe_add(p, exe, 0);
	  }
	else if ((match = evry->fuzzy_match(exe, p->input)))
	  {
	     app = _item_exe_add(p, exe, match);
	  }

	if (app)
	  {
	     EVRY_ITEM(app)->hi = hi;
	     evry->history_item_usage_set(EVRY_ITEM(app), p->input, NULL);
	  }

	break;
     }

   return EINA_TRUE;
}

static int
_fetch_exe(Evry_Plugin *plugin, const char *input)
{
   GET_PLUGIN(p, plugin);
   Eina_List *l;
   Evry_Item *it;
   History_Types *ht;
   unsigned int len = (input ? strlen(input) : 0);
   double max = 0.0;
   const char *tmp, *file = NULL;
   unsigned int min = 0, cnt = 0, end = len, tmp_len;
   unsigned int query = (len >= (unsigned int)plugin->config->min_query);
   EVRY_PLUGIN_ITEMS_CLEAR(p);

   p->input = input;

   ht = evry->history_types_get(EVRY_TYPE_APP);
   if (ht) eina_hash_foreach(ht->types, _hist_exe_get_cb, p);

   if (input)
     {
	if ((tmp = strchr(input, ' ')))
	  end = tmp - input;

	if ((!exe_list) && (!exe_scan_idler))
	  _scan_executables();

	EINA_LIST_FOREACH(exe_list, l, tmp)
	  {
	     tmp_len = strlen(tmp);

	     if ((end < len) &&  (tmp_len > end))
	       continue;

	     if (!strncmp(input, tmp, end))
	       {
		  if (query && (cnt++ < 50) && (len != tmp_len))
		    _item_exe_add(p, tmp, 15);

		  if ((!min) || (tmp_len < min))
		    {
		       min = tmp_len;
		       file = tmp;
		    }
		  if ((!query) && (tmp_len == len))
		    break;
	       }
	  }

	if (file)
	  {
	     GET_ITEM(it, p->command);

	     if (strlen(file) < len)
	       file = input;

	     EVRY_ITEM_LABEL_SET(it, file);
	     IF_RELEASE(p->command->file);
	     p->command->file = eina_stringshare_ref(it->label);
	     it->fuzzy_match = 11; // prefix match
	     EVRY_PLUGIN_ITEM_APPEND(p, it);
	     evry->item_changed(it, 0, 0);
	  }
     }

   EINA_LIST_FOREACH(plugin->items, l, it)
     {
	evry->history_item_usage_set(it, input, NULL);
	if (input && (it->usage > max) && !strncmp(input, it->label, len))
	  max = it->usage;
     }
   EVRY_ITEM(p->command)->usage = (max * 2.0);

   EVRY_PLUGIN_ITEMS_SORT(p, _cb_sort);

   return EVRY_PLUGIN_HAS_ITEMS(p);
}

static Evry_Plugin *
_begin_exe(Evry_Plugin *plugin, const Evry_Item *item)
{
   Plugin *p;
   Evry_Item_App *app;

   if (item && (item != _act_open_with))
     return NULL;

   EVRY_PLUGIN_INSTANCE(p, plugin)

   p->added = eina_hash_string_small_new(_hash_free);

   app = EVRY_ITEM_NEW(Evry_Item_App, p, NULL, NULL, _cb_item_free);
   EVRY_ACTN(app)->action = &_exec_open_file_action;
   EVRY_ACTN(app)->remember_context = EINA_TRUE;
   EVRY_ITEM(app)->subtype = EVRY_TYPE_ACTION;
   p->command = app;

   return EVRY_PLUGIN(p);
}

static void
_finish_exe(Evry_Plugin *plugin)
{
   GET_PLUGIN(p, plugin);
   char *str;

   EVRY_PLUGIN_ITEMS_CLEAR(p);
   EVRY_ITEM_FREE(p->command);

   if (p->added)
     eina_hash_free(p->added);

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

   E_FREE(p);
}

/***************************************************************************/

static void
_item_desktop_add(Plugin *p, Efreet_Desktop *desktop, int match)
{
   Evry_Item_App *app = NULL;

   if ((app = eina_hash_find(p->added, desktop->exec)))
     {
	if (eina_list_data_find_list(p->base.items, app))
	  return;
     }

   if (!app)
     {
	app = _item_new(p, desktop->name, desktop->exec);
	efreet_desktop_ref(desktop);
	app->desktop = desktop;

	if (desktop->comment)
	  {
	     EVRY_ITEM_DETAIL_SET(app, desktop->comment);
	  }
	else if (desktop->generic_name)
	  {
	     EVRY_ITEM_DETAIL_SET(app, desktop->generic_name);
	  }
     }

   EVRY_ITEM(app)->fuzzy_match = match;
   EVRY_PLUGIN_ITEM_APPEND(p, app);
}

static void
_desktop_list_add(Plugin *p, Eina_List *apps, const char *input)
{
   Efreet_Desktop *desktop;
   Eina_List *l;
   int m1, m2;
   const char *exec, *end;
   char buf[PATH_MAX];

   EINA_LIST_FOREACH(apps, l, desktop)
     {
	if (eina_list_count(p->base.items) >= MAX_ITEMS) break;

	m1 = m2 = 0;

	if (input)
	  {
	     /* strip path and parameter */
	     exec = ecore_file_file_get(desktop->exec);
	     if ((exec) && (end = strchr(exec, '%')) &&
		 ((end - exec) - 1 > 0))
	       {
		  strncpy(buf, exec, (end - exec) - 1);
		  buf[(end - exec) - 1] = '\0';
		  m1 = evry->fuzzy_match(buf, input);
	       }
	     else
	       {
		  m1 = evry->fuzzy_match(exec, input);
	       }

	     m2 = evry->fuzzy_match(desktop->name, input);

	     if (!m1 || (m2 && m2 < m1)) m1 = m2;
	  }

	if (!input || m1) _item_desktop_add(p, desktop, m1);
     }
}

static Eina_List *
_desktop_list_get(void)
{
   Eina_List *apps = NULL;
   Eina_List *cat_ss;
   Eina_List *l, *ll;
   Efreet_Desktop *d;

   apps = efreet_util_desktop_name_glob_list("*");

   /* remove screensaver */
   cat_ss = efreet_util_desktop_category_list("Screensaver");
   EINA_LIST_FOREACH(cat_ss, l, d)
     {
	if ((ll = eina_list_data_find_list(apps, d)))
	  {
	     efreet_desktop_free(d);
	     apps = eina_list_remove_list(apps, ll);
	  }
	printf("%d %s\n", d->ref, d->name);

	efreet_desktop_free(d);
     }

   return apps;
}

static Eina_Bool
_hist_items_get_cb(const Eina_Hash *hash __UNUSED__, const void *key, void *data, void *fdata)
{
   History_Entry *he = data;
   History_Item *hi;
   Plugin *p = fdata;
   Efreet_Desktop *d;
   Eina_List *l, *ll;
   const char *exec = key;

   EINA_LIST_FOREACH(he->items, l, hi)
     {
	d = NULL;

	if (hi->transient)
	  continue;

	if (strcmp(hi->plugin, EVRY_PLUGIN(p)->name))
	  continue;

	EINA_LIST_FOREACH(apps_cache, ll, d)
	  if (d->exec && !strcmp(d->exec, exec)) break;

	if (!d)
	  {
	     if (!p->apps_all)
	       p->apps_all = _desktop_list_get();

	     EINA_LIST_FOREACH(p->apps_all, ll, d)
	       if (d->exec && !strcmp(d->exec, exec)) break;

	     if (d)
	       {
		  efreet_desktop_ref(d);
		  apps_cache = eina_list_append(apps_cache, d);
	       }
	     else
	       {
		  /* remove from history */
		  /* DBG("no desktop: %s", exec);
		   * hi->transient = 1; */
	       }
	  }

	if (!d)
	  {
	     DBG("app not found %s", exec);
	     break;
	  }

	p->apps_hist = eina_list_append(p->apps_hist, d);
	break;
     }

   return EINA_TRUE;
}

static Evry_Plugin *
_begin(Evry_Plugin *plugin, const Evry_Item *item)
{
   Plugin *p;
   
   if (item && (item != _act_open_with))
     return NULL;

   EVRY_PLUGIN_INSTANCE(p, plugin);
   p->added = eina_hash_string_small_new(_hash_free);
   p->menu = efreet_menu_get();
   
   return EVRY_PLUGIN(p);
}

static Evry_Plugin *
_browse(Evry_Plugin *plugin, const Evry_Item *item)
{
   Plugin *p;
   
   if (!item)
     return NULL;

   if (!CHECK_TYPE(item, EVRY_TYPE_NONE))
     return NULL;
   
   EVRY_PLUGIN_INSTANCE(p, plugin);
   GET_MENU(m, item);
   
   p->added = eina_hash_string_small_new(_hash_free);
   p->menu = m->menu;
   p->browse = EINA_TRUE;
   
   return EVRY_PLUGIN(p);
}

static void
_finish(Evry_Plugin *plugin)
{
   Efreet_Desktop *desktop;
   Evry_Item *it;
   
   GET_PLUGIN(p, plugin);
   
   EVRY_PLUGIN_ITEMS_CLEAR(p);

   /* TODO share with browse instances */
   if (p->added)
     eina_hash_free(p->added);

   if ((!p->browse) && (p->menu))
     efreet_menu_free(p->menu);
   
   EINA_LIST_FREE(p->apps_all, desktop)
     efreet_desktop_free(desktop);

   EINA_LIST_FREE(p->apps_hist, desktop);

   EINA_LIST_FREE(p->apps_mime, desktop)
     efreet_desktop_free(desktop);

   EINA_LIST_FREE(p->menu_items, it)
     EVRY_ITEM_FREE(it);

   E_FREE(p);
}

static int
_fetch(Evry_Plugin *plugin, const char *input)
{
   GET_PLUGIN(p, plugin);
   Eina_List *l;
   Evry_Item *it;
   History_Types *ht;

   Efreet_Menu *entry;
   int i = 1;

   EVRY_PLUGIN_ITEMS_CLEAR(p);

   if (!p->browse)
     {
	if (input)
	  {
	     if (!p->apps_all)
	       p->apps_all = _desktop_list_get();

	     _desktop_list_add(p, p->apps_all, input);
	  }
	else
	  {
	     _desktop_list_add(p, p->apps_mime, input);
	  }

	if ((!input) && (!plugin->items))
	  {
	     if (!p->apps_hist)
	       {
		  ht = evry->history_types_get(EVRY_TYPE_APP);
		  if (ht) eina_hash_foreach(ht->types, _hist_items_get_cb, p);
	       }

	     _desktop_list_add(p, p->apps_hist, NULL);
	  }

	EINA_LIST_FOREACH(plugin->items, l, it)
	  evry->history_item_usage_set(it, input, NULL);

	EVRY_PLUGIN_ITEMS_SORT(p, _cb_sort);
     }
   
   if ((p->menu) && (!p->menu_items))
     {
	EINA_LIST_FOREACH(p->menu->entries, l, entry)
	  {
	     if (entry->type == EFREET_MENU_ENTRY_DESKTOP)
	       {
	     	  _item_desktop_add(p, entry->desktop, i++); 	     
	       }
	     else if (entry->type == EFREET_MENU_ENTRY_MENU)
	       {
		  _item_menu_add(p, entry);	     
	       }
	     /* else if (entry->type == EFREET_MENU_ENTRY_SEPARATOR)
	      *   continue;
	      * else if (entry->type == EFREET_MENU_ENTRY_HEADER)
	      *   continue; */
	  }
     }


   EINA_LIST_FOREACH(p->menu_items, l, it)
     {
	if (!input)
	  {
	     EVRY_PLUGIN_ITEM_APPEND(p, it);
	     continue;
	  }

	if ((it->fuzzy_match = evry->fuzzy_match(it->label, input)))
	  EVRY_PLUGIN_ITEM_APPEND(p, it);
     }
   
   return EVRY_PLUGIN_HAS_ITEMS(p);
}

/***************************************************************************/

static Evry_Plugin *
_begin_mime(Evry_Plugin *plugin, const Evry_Item *item)
{
   Plugin *p = NULL;
   Efreet_Desktop *d;
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

   EVRY_PLUGIN_INSTANCE(p, plugin);

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
	if ((l = eina_list_data_find_list(p->apps_mime, d)))
	  {
	     p->apps_mime = eina_list_promote_list(p->apps_mime, l);
	     efreet_desktop_free(d);
	  }
	else
	  {
	     p->apps_mime = eina_list_prepend(p->apps_mime, d);
	  }
     }

   p->added = eina_hash_string_small_new(_hash_free);

   return EVRY_PLUGIN(p);
}

static void
_finish_mime(Evry_Plugin *plugin)
{
   GET_PLUGIN(p, plugin);
   Efreet_Desktop *desktop;

   EVRY_PLUGIN_ITEMS_CLEAR(p);

   if (p->added)
     eina_hash_free(p->added);

   EINA_LIST_FREE(p->apps_mime, desktop)
     efreet_desktop_free(desktop);

   E_FREE(p);
}

static int
_fetch_mime(Evry_Plugin *plugin, const char *input)
{
   GET_PLUGIN(p, plugin);
   Eina_List *l;
   Evry_Item *it;

   EVRY_PLUGIN_ITEMS_CLEAR(p);

   /* add apps for a given mimetype */
   _desktop_list_add(p, p->apps_mime, input);

   EINA_LIST_FOREACH(plugin->items, l, it)
     evry->history_item_usage_set(it, input, NULL);

   if (input)
     EVRY_PLUGIN_ITEMS_SORT(p, _cb_sort);

   return 1;
}


static int
_complete(Evry_Plugin *plugin __UNUSED__, const Evry_Item *it, char **input)
{
   GET_APP(app, it);

   char buf[128];

   if (app->desktop)
     {
	char *space = strchr(app->desktop->exec, ' ');

	snprintf(buf, sizeof(buf), "%s", app->desktop->exec);
	if (space)
	  buf[1 + space - app->desktop->exec] = '\0';
     }
   else
     snprintf(buf, sizeof(buf), "%s", app->file);

   *input = strdup(buf);

   return EVRY_COMPLETE_INPUT;
}

/***************************************************************************/

static int
_exec_app_check_item(Evry_Action *act __UNUSED__, const Evry_Item *it __UNUSED__)
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
_check_executable(Evry_Action *act __UNUSED__, const Evry_Item *it)
{
   GET_FILE(file, it);

   return ecore_file_can_exec(evry->file_path_get(file));
}

static int
_run_executable(Evry_Action *act)
{
   GET_FILE(file, act->it1.item);

   e_exec(e_util_zone_current_get(e_manager_current_get()), NULL, file->path, NULL, NULL);

   return 1;
}

/***************************************************************************/
static Eina_Bool
_desktop_cache_update(void *data __UNUSED__, int type __UNUSED__, void *event __UNUSED__)
{
   Efreet_Desktop *d;

   EINA_LIST_FREE(apps_cache, d)
     efreet_desktop_unref(d);

   return EINA_TRUE;
}

static int
_plugins_init(const Evry_API *api)
{
   Evry_Plugin *p;
   int prio = 0;
   Eina_List *l;
   Evry_Action *act;

   evry = api;

   if (!evry->api_version_check(EVRY_API_VERSION))
     return EINA_FALSE;

   p = EVRY_PLUGIN_BASE("Applications", _module_icon, EVRY_TYPE_APP,
			_begin, _finish, _fetch);
   p->complete = &_complete;
   p->browse = &_browse;
   p->config_path = "extensions/everything-apps";
   evry->plugin_register(p, EVRY_PLUGIN_SUBJECT, 1);
   _plugins = eina_list_append(_plugins, p);


   p = EVRY_PLUGIN_BASE("Exebuf", _module_icon, EVRY_TYPE_APP,
			_begin_exe, _finish_exe, _fetch_exe);
   p->complete = &_complete;
   p->config_path = "extensions/everything-apps";
   _plugins = eina_list_append(_plugins, p);
   if (evry->plugin_register(p, EVRY_PLUGIN_SUBJECT, 3))
     p->config->min_query = 5;

   p = EVRY_PLUGIN_BASE("Applications", _module_icon, EVRY_TYPE_APP,
			_begin_mime, _finish, _fetch);
   p->complete = &_complete;
   p->config_path = "extensions/everything-apps";
   evry->plugin_register(p, EVRY_PLUGIN_OBJECT, 1);
   _plugins = eina_list_append(_plugins, p);

   p = EVRY_PLUGIN_BASE("Open With...", _module_icon, EVRY_TYPE_APP,
			_begin_mime, _finish_mime, _fetch_mime);
   p->config_path = "extensions/everything-apps";
   evry->plugin_register(p, EVRY_PLUGIN_ACTION, 1);
   _plugins = eina_list_append(_plugins, p);

   act = EVRY_ACTION_NEW("Launch",
			 EVRY_TYPE_APP, 0,
			 "system-run",
			 _exec_app_action,
			 _exec_app_check_item);
   _actions = eina_list_append(_actions, act);

   act = EVRY_ACTION_NEW("Open File...",
			 EVRY_TYPE_APP, EVRY_TYPE_FILE,
			 "document-open",
			 _exec_app_action,
			 _exec_app_check_item);
   _actions = eina_list_append(_actions, act);

   act = EVRY_ACTION_NEW("Run in Terminal",
			 EVRY_TYPE_APP, 0,
			 "system-run",
			 _exec_term_action,
			 _exec_term_check_item);
   _actions = eina_list_append(_actions, act);

   act = EVRY_ACTION_NEW("Edit Application Entry",
			 EVRY_TYPE_APP, 0,
			 "everything-launch",
			 _edit_app_action,
			 _edit_app_check_item);
   _actions = eina_list_append(_actions, act);

   act = EVRY_ACTION_NEW("New Application Entry",
			 EVRY_TYPE_APP, 0,
			 "everything-launch",
			 _new_app_action,
			 _new_app_check_item);
   _actions = eina_list_append(_actions, act);

   act = EVRY_ACTION_NEW("Run with Sudo",
			 EVRY_TYPE_APP, 0,
			 "system-run",
			 _exec_sudo_action, NULL);
   _actions = eina_list_append(_actions, act);

   act = EVRY_ACTION_NEW("Open with...",
			 EVRY_TYPE_FILE, EVRY_TYPE_APP,
			 "everything-launch",
			 _exec_file_action, NULL);
   _act_open_with = EVRY_ITEM(act);
   _actions = eina_list_append(_actions, act);

   act = EVRY_ACTION_NEW("Open Terminal here",
			 EVRY_TYPE_FILE, 0,
			 "system-run",
			 _open_term_action, NULL);
   _actions = eina_list_append(_actions, act);

   act = EVRY_ACTION_NEW("Run Executable",
			 EVRY_TYPE_FILE, 0,
			 "system-run",
			 _run_executable,
			 _check_executable);
   _actions = eina_list_append(_actions, act);

   EINA_LIST_FOREACH(_actions, l, act)
     evry->action_register(act,  prio++);

   handlers = eina_list_append
     (handlers, ecore_event_handler_add
      (EFREET_EVENT_DESKTOP_CACHE_UPDATE, _desktop_cache_update, NULL));

   return EINA_TRUE;
}

static void
_plugins_shutdown(void)
{
   Evry_Action *act;
   Evry_Plugin *p;
   Efreet_Desktop *d;
   Ecore_Event_Handler *h;

   EINA_LIST_FREE(apps_cache, d)
     efreet_desktop_unref(d);

   EINA_LIST_FREE(_plugins, p)
     EVRY_PLUGIN_FREE(p);

   EINA_LIST_FREE(_actions, act)
     EVRY_ACTION_FREE(act);

   EINA_LIST_FREE(handlers, h)
     ecore_event_handler_del(h);
}

/***************************************************************************/

static E_Config_DD *conf_edd = NULL;

struct _E_Config_Dialog_Data
{
  char *cmd_terminal;
  char *cmd_sudo;
};

static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static void _fill_data(E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);

static E_Config_Dialog *
_conf_dialog(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd = NULL;
   E_Config_Dialog_View *v = NULL;

   if (e_config_dialog_find("everything-apps", "launcher/everything-apps")) return NULL;

   v = E_NEW(E_Config_Dialog_View, 1);
   if (!v) return NULL;

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;
   v->basic.apply_cfdata = _basic_apply;

   cfd = e_config_dialog_new(con, _("Everything Applications"), "everything-apps",
			     "launcher/everything-apps", _module_icon, 0, v, NULL);

   /* e_dialog_resizable_set(cfd->dia, 1); */
   _conf->cfd = cfd;
   return cfd;
}

static void *
_create_data(E_Config_Dialog *cfd __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = NULL;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _fill_data(cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   _conf->cfd = NULL;
   E_FREE(cfdata);
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd __UNUSED__, Evas *e, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o = NULL, *of = NULL, *ow = NULL;

   o = e_widget_list_add(e, 0, 0);

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
   CP(cmd_terminal);
   CP(cmd_sudo);
#undef CP
#undef C
}

static int
_basic_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
#define CP(_name)					\
   if (_conf->_name)					\
     eina_stringshare_del(_conf->_name);		\
   _conf->_name = eina_stringshare_add(cfdata->_name);
#define C(_name) _conf->_name = cfdata->_name;
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
   char title[4096];

   snprintf(title, sizeof(title), "%s: %s", _("Everything Plugin"), _("Applications"));
   
   e_configure_registry_item_add("launcher/everything-apps", 110, title,
				 NULL, _module_icon, _conf_dialog);

   conf_edd = E_CONFIG_DD_NEW("Module_Config", Module_Config);

#undef T
#undef D
#define T Module_Config
#define D conf_edd
   E_CONFIG_VAL(D, T, version, INT);
   E_CONFIG_VAL(D, T, cmd_terminal, STR);
   E_CONFIG_VAL(D, T, cmd_sudo, STR);
#undef T
#undef D

   _conf = e_config_domain_load("module.everything-apps", conf_edd);


   if (_conf && !e_util_module_config_check(_("Everything Applications"),
					    _conf->version,
					    MOD_CONFIG_FILE_VERSION))
     _conf_free();

   _conf_new();
   _conf->module = m;
}

static void
_conf_shutdown(void)
{
   e_configure_registry_item_del("launcher/everything-apps");
   
   _conf_free();

   E_CONFIG_DD_FREE(conf_edd);
}

/***************************************************************************/

Eina_Bool
evry_plug_apps_init(E_Module *m)
{
   _conf_init(m);

   EVRY_MODULE_NEW(evry_module, evry, _plugins_init, _plugins_shutdown);

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

void
evry_plug_apps_shutdown(void)
{
   EVRY_MODULE_FREE(evry_module);

   _conf_shutdown();

   E_CONFIG_DD_FREE(exelist_edd);
   E_CONFIG_DD_FREE(exelist_exe_edd);
}

void
evry_plug_apps_save(void)
{
   e_config_domain_save("module.everything-apps", conf_edd, _conf);
}

/***************************************************************************/


/* taken from e_exebuf.c */
static Eina_Bool
_scan_idler(void *data __UNUSED__)
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
	return ECORE_CALLBACK_CANCEL;
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
   /* we have more scannign to do */
   return ECORE_CALLBACK_RENEW;
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
