/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */

#include "Evry.h"
#include "e_mod_main.h"

#define MAX_ITEMS 50
#define TERM_ACTION_DIR "%s"

typedef struct _Plugin Plugin;
typedef struct _Data Data;

struct _Plugin
{
  Evry_Plugin base;

  Eina_List *files;
  const char *directory;
  const char *input;
  Eina_Bool command;
  Eina_Bool parent;
  Eina_Bool hist_added;
  
  Ecore_Thread *thread;
  Eina_Bool cleanup;
};

struct _Data
{
  Plugin *plugin;
  long id;
  int level;
  int cnt;
  Eina_List *files;
  Eina_List *list;
};

static Evry_Plugin *p1 = NULL;
static Evry_Plugin *p2 = NULL;
static Evry_Action *act1 = NULL;
static Evry_Action *act2 = NULL;

static E_Module *module = NULL;
static Eina_Bool active = EINA_FALSE;

static void _cleanup(Evry_Plugin *plugin);

  
static void
_item_fill(Evry_Item_File *file)
{
   const char *mime;
   
   if (file->mime) return;

   if ((mime = efreet_mime_type_get(file->path)))
     {
	file->mime = eina_stringshare_add(mime);
	EVRY_ITEM(file)->context = eina_stringshare_ref(file->mime);
	
	if ((!strcmp(mime, "inode/directory")) ||
	    (!strcmp(mime, "inode/mount-point")))
	  EVRY_ITEM(file)->browseable = EINA_TRUE;

	return;
     }

   file->mime = eina_stringshare_add("None");
}

static int
_cb_sort(const void *data1, const void *data2)
{
   const Evry_Item *it1 = data1;
   const Evry_Item *it2 = data2;

   if (it1->browseable && !it2->browseable)
     return -1;

   if (!it1->browseable && it2->browseable)
     return 1;

   if (it1->fuzzy_match && it2->fuzzy_match)
     if (it1->fuzzy_match - it2->fuzzy_match)
       return (it1->fuzzy_match - it2->fuzzy_match);

   return strcasecmp(it1->label, it2->label);
}

static void
_item_free(Evry_Item *it)
{
   ITEM_FILE(file, it);
   if (file->path) eina_stringshare_del(file->path);
   if (file->mime) eina_stringshare_del(file->mime);

   E_FREE(file);
}

static void
_scan_func(void *data)
{
   Data *d = data;
   Plugin *p = d->plugin;
   Eina_List *files;
   char *filename;
   const char *mime;
   
   Evry_Item_File *file;
   char buf[4096];
   int cnt = 0;

   if (!d->list)
     d->list = ecore_file_ls(p->directory);

   EINA_LIST_FREE(d->list, filename)
     {
	if (filename[0] == '.')
	  {
	     free(filename);
	     continue;
	  }

	file = E_NEW(Evry_Item_File, 1);
	if (!file)
	  {
	     free(filename);
	     continue;
	  }

	evry_item_new(EVRY_ITEM(file), EVRY_PLUGIN(p), NULL, _item_free);

	EVRY_ITEM(file)->label = filename;

	snprintf(buf, sizeof(buf), "%s/%s", p->directory, filename);
	file->path = strdup(buf);

	if (ecore_file_is_dir(file->path))
	  {
	     file->mime = "inode/directory";
	     EVRY_ITEM(file)->browseable = EINA_TRUE;
	  }
	else if ((mime = efreet_mime_type_get(file->path)))
	  {
	     file->mime = mime;
	  }
	
	d->files = eina_list_append(d->files, file);

	if (cnt > MAX_ITEMS)
	  break;
     }
}

static int
_append_file(Plugin *p, Evry_Item_File *file)
{
   int match;

   if (p->input && (match = evry_fuzzy_match(EVRY_ITEM(file)->label, p->input)))
     {
	EVRY_ITEM(file)->fuzzy_match = match;
	EVRY_PLUGIN_ITEM_APPEND(p, file);
	return 1;
     }
   else if (!p->input)
     {
	EVRY_PLUGIN_ITEM_APPEND(p, file);
	return 1;
     }

   return 0;
}

static void
_scan_cancel_func(void *data)
{
   Data *d = data;
   Plugin *p = d->plugin;
   Evry_Item_File *file;
   char *filename;
   
   EINA_LIST_FREE(d->files, file)
     {
	free((char *)EVRY_ITEM(file)->label);
	free((char *)file->path);	
	free(file);
     }

   EINA_LIST_FREE(d->list, filename)
     free(filename);     
   
   E_FREE(d);

   if (p->directory)
     eina_stringshare_del(p->directory);

   p->thread = NULL;
   
   if (p->cleanup)
     _cleanup(EVRY_PLUGIN(p));
}

static void
_scan_end_func(void *data)
{
   Data *d = data;
   Plugin *p = d->plugin;
   Evry_Item *item;
   Evry_Item_File *f;
   char *filename, *path, *mime;
   int cnt = eina_list_count(p->base.items);
   Eina_List *l;
   
   p->thread = NULL;

   
   EINA_LIST_FREE(d->files, item)
     {
	ITEM_FILE(file, item);

	filename = (char *)item->label;
	path = (char *) file->path;
	mime = (char *) file->mime;
	
	file->path = eina_stringshare_add(path);

	/* filter out files that we already have from history */
	EINA_LIST_FOREACH(p->files, l, f)
	  {
	     if (f->path == file->path)
	       {
		  free(filename);
		  free(path);
		  E_FREE(file);
		  file = NULL;
		  break;
	       }
	  }
	if (!file) continue;
	
	if (mime)
	  file->mime = eina_stringshare_add(mime);
	else
	  file->mime = eina_stringshare_add("None");	  
	item->context = eina_stringshare_ref(file->mime);
	item->id = eina_stringshare_ref(file->path);
	item->label = eina_stringshare_add(filename);

	free(filename);
	free(path);

	p->files = eina_list_append(p->files, file);

	evry_util_file_detail_set(file);
	
	if (cnt >= MAX_ITEMS) continue;

	cnt += _append_file(p, file);
     }
   if (d->files)
     p->thread = ecore_thread_run(_scan_func, _scan_end_func, _scan_cancel_func, d);
   else
     E_FREE(d);
   
   EVRY_PLUGIN_ITEMS_SORT(p, _cb_sort);
   evry_plugin_async_update(EVRY_PLUGIN(p), EVRY_ASYNC_UPDATE_ADD);
}

static void
_read_directory(Plugin *p)
{
   Data *d = E_NEW(Data, 1);
   d->plugin = p;

   p->thread = ecore_thread_run(_scan_func, _scan_end_func, _scan_cancel_func, d);
}

static Evry_Plugin *
_begin(Evry_Plugin *plugin, const Evry_Item *it)
{
   Plugin *p = NULL;

   /* is FILE ? */
   if (it && (it->plugin->type_out == plugin->type_in))
     {
	ITEM_FILE(file, it);

	if (!file->path || !ecore_file_is_dir(file->path))
	  return NULL;

	p = E_NEW(Plugin, 1);
	p->base = *plugin;
	p->base.items = NULL;

	p->directory = eina_stringshare_add(file->path);
	p->parent = EINA_TRUE;
     }
   else
     {
	p = E_NEW(Plugin, 1);
	p->base = *plugin;
	p->base.items = NULL;
	p->directory = eina_stringshare_add(e_user_homedir_get());
	p->parent = EINA_FALSE;
     }

   _read_directory(p);

   return EVRY_PLUGIN(p);
}

static void
_cleanup(Evry_Plugin *plugin)
{
   PLUGIN(p, plugin);

   Evry_Item_File *file;
   
   if (p->thread)
     {	
	ecore_thread_cancel(p->thread);
	p->cleanup = EINA_TRUE;
     }
   else
     {
	if (p->directory)
	  eina_stringshare_del(p->directory);

	EINA_LIST_FREE(p->files, file)
	  evry_item_free(EVRY_ITEM(file));

	EVRY_PLUGIN_ITEMS_CLEAR(p);

	if (p->input)
	  eina_stringshare_del(p->input); 

	E_FREE(p);
     }
}

static Eina_Bool
_hist_items_add_cb(const Eina_Hash *hash, const void *key, void *data, void *fdata)
{
   History_Entry *he = data;
   History_Item *hi;
   Plugin *p = fdata;
   Eina_List *l, *ll;
   Evry_Item_File *file;
   const char *label;
   
   EINA_LIST_FOREACH(he->items, l, hi)
     {
	if (hi->type != p->base.type_out)
	  continue;

	/* filter out files that we already have from history */
	EINA_LIST_FOREACH(p->files, ll, file)
	  if (!strcmp(file->path, key))
	    return EINA_TRUE;

	/* XXX this blocks ui when drive is sleeping */
	/* if (!ecore_file_exists(key))
	 *   continue; */

	label = ecore_file_file_get(key);
	if (!label)
	  continue;
	
	file = E_NEW(Evry_Item_File, 1);
	if (!file)
	  continue;

	evry_item_new(EVRY_ITEM(file), EVRY_PLUGIN(p), label, _item_free);

	file->path = eina_stringshare_add(key); 
	if (hi->context)
	  file->mime = eina_stringshare_add(hi->context);
	if (hi->context)
	  EVRY_ITEM(file)->context = eina_stringshare_ref(file->mime);

	EVRY_ITEM(file)->id = eina_stringshare_ref(file->path);

	evry_util_file_detail_set(file); 

	if (ecore_file_is_dir(file->path))
	EVRY_ITEM(file)->browseable = EINA_TRUE;

	p->files = eina_list_append(p->files, file); 
	break;
     }
   return EINA_TRUE;
}

static int
_fetch(Evry_Plugin *plugin, const char *input)
{
   PLUGIN(p, plugin);
   Evry_Item_File *file;
   Eina_List *l;
   int cnt = 0;

   if (!p->command)
     EVRY_PLUGIN_ITEMS_CLEAR(p);

   if (p->input)
     eina_stringshare_del(p->input); 
   p->input = NULL;

   if (!p->parent && input)
     {
	/* input is command ? */
   	if (!strcmp(input, "/"))
   	  {
	     /* browse root */
	     EINA_LIST_FREE(p->files, file)
	       evry_item_free(EVRY_ITEM(file));

	     eina_stringshare_del(p->directory); 
	     p->directory = eina_stringshare_add("/");
	     _read_directory(p);

	     p->command = EINA_TRUE;
	     return 0;
   	  }

	/* add recent files */
	if (!p->hist_added)
	  eina_hash_foreach(evry_hist->subjects, _hist_items_add_cb, p);
	p->hist_added = EINA_TRUE;
     }

   /* clear command items */
   if (!p->parent && !input && p->command)
     {
   	p->command = EINA_FALSE;
	
	EINA_LIST_FREE(p->files, file)
	  evry_item_free(EVRY_ITEM(file));

	eina_stringshare_del(p->directory); 
	p->directory = eina_stringshare_add(e_user_homedir_get());
	_read_directory(p);
	return 0;
     }

   if (input)
     {
	/* skip command prefix */
	if (p->command)
	  p->input = eina_stringshare_add(input + 1);
	else
	  p->input = eina_stringshare_add(input);
     }
   
   EINA_LIST_FOREACH(p->files, l, file)
     {
	if (cnt >= MAX_ITEMS);
	cnt += _append_file(p, file);
     }

   if (!EVRY_PLUGIN(p)->items)
     return 0;

   EVRY_PLUGIN_ITEMS_SORT(p, _cb_sort);
   
   return 1;
}

static Evas_Object *
_icon_get(Evry_Plugin *p __UNUSED__, const Evry_Item *it, Evas *e)
{
   Evas_Object *o = NULL;
   ITEM_FILE(file, it);

   if (!file->mime)
     _item_fill(file);

   if (!file->mime) return NULL;

   if (it->browseable)
     o = evry_icon_theme_get("folder", e);
   else
     o = evry_icon_mime_get(file->mime, e);

   return o;
}


static int
_open_folder_check(Evry_Action *act __UNUSED__, const Evry_Item *it)
{
   return (it->browseable && e_action_find("fileman"));
}

static int
_open_folder_action(Evry_Action *act)
{
   E_Action *action = e_action_find("fileman");
   char *path;
   Eina_List *m;

   if (!action) return 0;

   m = e_manager_list();

   ITEM_FILE(file, act->item1);

   if (!act->item1->browseable)
     {
	path = ecore_file_dir_get(file->path);
	if (!path) return 0;
	action->func.go(E_OBJECT(m->data), path);
	free(path);
     }
   else
     {
	action->func.go(E_OBJECT(m->data), file->path);
     }

   return 1;
}

static int
_open_term_action(Evry_Action *act)
{
   ITEM_FILE(file, act->item1);
   Evry_Item_App *tmp;
   char cwd[4096];
   char *dir;
   int ret = 0;

   if (act->item1->browseable)
     dir = strdup(file->path);
   else
     dir = ecore_file_dir_get(file->path);

   if (dir)
     {
	getcwd(cwd, sizeof(cwd));
	chdir(dir);
	
	tmp = E_NEW(Evry_Item_App, 1);
	tmp->file = evry_conf->cmd_terminal;

	ret = evry_util_exec_app(EVRY_ITEM(tmp), NULL);
	E_FREE(tmp);
	E_FREE(dir);
	chdir(cwd);
     }
   
   return ret;
}

static Eina_Bool
module_init(void)
{
   if (!evry_api_version_check(EVRY_API_VERSION))
     return EINA_FALSE;

   p1 = EVRY_PLUGIN_NEW(NULL, "Files", type_subject, "FILE", "FILE",
			_begin, _cleanup, _fetch, _icon_get, NULL);

   p2 = EVRY_PLUGIN_NEW(NULL, "Files", type_object, "FILE", "FILE",
			_begin, _cleanup, _fetch, _icon_get, NULL);
   
   evry_plugin_register(p1, 3);
   evry_plugin_register(p2, 1);

   act1 = EVRY_ACTION_NEW("Open Folder (EFM)", "FILE", NULL, "folder-open",
			  _open_folder_action, _open_folder_check);
   evry_action_register(act1, 0);

   act2 = EVRY_ACTION_NEW("Open Terminal here", "FILE", NULL, "system-run",
			  _open_term_action, NULL);
   evry_action_register(act2, 2);

   return EINA_TRUE;
}

static void
module_shutdown(void)
{
   EVRY_PLUGIN_FREE(p1);
   EVRY_PLUGIN_FREE(p2);

   evry_action_free(act1);
   evry_action_free(act2);
}


/***************************************************************************/

EAPI E_Module_Api e_modapi = 
{
   E_MODULE_API_VERSION,
   "everything-files"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   module = m;

   if (e_datastore_get("everything_loaded"))
     active = module_init();

   e_module_delayed_set(m, 1); 
   
   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   if (active && e_datastore_get("everything_loaded"))
     module_shutdown();

   module = NULL;
   
   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   return 1;
}
