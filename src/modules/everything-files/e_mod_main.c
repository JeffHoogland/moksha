/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */

/* TODO option for maximum items to cache */

#include "Evry.h"
#include "e_mod_main.h"
#include <Efreet_Trash.h>

#define MAX_ITEMS 10
#define MAX_SHOWN 300
#define TERM_ACTION_DIR "%s"

#define CMD_NONE        0
#define CMD_SHOW_ROOT   1
#define CMD_SHOW_HIDDEN 2
#define CMD_SHOW_PARENT 3

#define ACT_TRASH	1
#define ACT_DELETE	2
#define ACT_COPY	3
#define ACT_MOVE	4

#define ONE_DAY  86400.0
#define SIX_DAYS_AGO (ecore_time_get() - 518400.0)
#define TIME_FACTOR(_now) (1.0 - (evry_hist->begin / _now)) / 1000000000000000.0

/* #undef DBG
 * #define DBG(...) ERR(__VA_ARGS__) */

typedef struct _Plugin Plugin;
typedef struct _Data Data;
typedef struct _Module_Config Module_Config;

struct _Plugin
{
  Evry_Plugin base;

  Eina_List *files;
  const char *directory;
  const char *input;
  unsigned int command;
  unsigned int min_query;
  Eina_Bool parent;
  Eina_Bool show_hidden;
  Eina_Bool dirs_only;
  Eina_Bool show_recent;

  Ecore_Thread *thread;
  Ecore_File_Monitor *dir_mon;
  int wait_finish;
};

struct _Data
{
  Plugin *plugin;
  char *directory;
  long id;
  int level;
  int cnt;
  Eina_List *files;
  Eina_List *list;
  DIR *dirp;
  int run_cnt;
};

struct _Module_Config
{
  int version;

  unsigned char show_homedir;
  unsigned char show_recent;
  unsigned char search_recent;
  unsigned char cache_dirs;
  unsigned char search_cache;

  // TODO
  int sort_by;
  Eina_List *search_dirs;

  E_Config_Dialog *cfd;
  E_Module *module;
};

static Module_Config *_conf;
static char _module_icon[] = "system-file-manager";
static Eina_List *_plugins = NULL;
static Eina_List *_actions = NULL;
static const char *_mime_dir;
static const char *_mime_mount;
static Eina_Bool clear_cache = EINA_FALSE;

/***************************************************************************/

static void
_item_fill(Evry_Item_File *file)
{
   if (!file->mime)
     {
	const char *mime = efreet_mime_type_get(file->path);

	if (mime)
	  file->mime = eina_stringshare_ref(mime);
	else
	  file->mime = eina_stringshare_add("unknown");
     }

   if ((file->mime == _mime_dir) ||
       (file->mime == _mime_mount))
     EVRY_ITEM(file)->browseable = EINA_TRUE;

   EVRY_ITEM(file)->context = eina_stringshare_ref(file->mime);

   if (!EVRY_ITEM(file)->detail)
     evry_util_file_detail_set(file);


     evry_util_file_detail_set(file);
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
   GET_FILE(file, it);
   if (file->url) eina_stringshare_del(file->url);
   if (file->path) eina_stringshare_del(file->path);
   if (file->mime) eina_stringshare_del(file->mime);

   E_FREE(file);
}

static void
_scan_func(void *data)
{
   Data *d = data;
   Plugin *p = d->plugin;
   char *filename;
   struct dirent *dp;
   Evry_Item_File *file;
   char buf[4096];
   Eina_Bool is_dir;

   d->dirp = opendir(d->directory);
   if (!d->dirp) return;

   while ((dp = readdir(d->dirp)))
     {
	if ((dp->d_name[0] == '.') &&
	    ((dp->d_name[1] == '\0') ||
	     ((dp->d_name[1] == '.') &&
	      (dp->d_name[2] == '\0'))))
	  continue;

#ifdef _DIRENT_HAVE_D_TYPE
	if (dp->d_type == DT_FIFO ||
	    dp->d_type == DT_BLK  ||
	    dp->d_type == DT_CHR  ||
	    dp->d_type == DT_SOCK)
	  continue;
#endif
	if (!p->show_hidden)
	  {
	     if (dp->d_name[0] == '.')
	       continue;
	  }
	else
	  {
	     if (dp->d_name[0] != '.')
	       continue;
	  }

	is_dir = EINA_FALSE;

#ifdef _DIRENT_HAVE_D_TYPE
	if (dp->d_type & DT_UNKNOWN)
	  {
#endif
	     if (ecore_file_is_dir(file->path))
	       is_dir = EINA_TRUE;

#ifdef _DIRENT_HAVE_D_TYPE
	  }
	else if (dp->d_type & DT_DIR)
	  {
	     is_dir = EINA_TRUE;
	  }
#endif

	if (p->dirs_only && !is_dir)
	  continue;

	file = EVRY_ITEM_NEW(Evry_Item_File, p, NULL, NULL, _item_free);

	filename = strdup(dp->d_name);
	EVRY_ITEM(file)->label = filename;
	EVRY_ITEM(file)->browseable = is_dir;
	d->files = eina_list_append(d->files, file);

	snprintf(buf, sizeof(buf), "%s/%s", d->directory, filename);
	file->path = strdup(buf);
     }
   closedir(d->dirp);
}

static void
_scan_mime_func(void *data)
{
   Data *d = data;
   Plugin *p = d->plugin;
   Evry_Item_File *file;
   Eina_List *l;
   const char *mime;
   int cnt = 0;

   EINA_LIST_FOREACH(d->files, l, file)
     {
	if (!EVRY_ITEM(file)->browseable)
	  {
	     if ((mime = efreet_mime_type_get(file->path)))
	       {
		  file->mime = mime;

		  if (!strncmp(file->mime, "inode/", 6) &&
		      ecore_file_is_dir(file->path))
		    EVRY_ITEM(file)->browseable = EINA_TRUE;
	       }
	     else
	       {
		  file->mime = "unknown";
	       }
	  }
	if (cnt++ > MAX_ITEMS * d->run_cnt) break;
     }
}

static int
_append_files(Plugin *p)
{
   int match;
   int cnt = 0;
   Evry_Item *it;
   Eina_List *l;

   EVRY_PLUGIN_ITEMS_CLEAR(p);

   if (!p->command && p->min_query)
     {
	if (!p->input)
	  return 0;
	if (strlen(p->input) < p->min_query)
	  return 0;
     }

   EINA_LIST_FOREACH(p->files, l, it)
     {
	if (cnt >= MAX_SHOWN) break;

	if (p->dirs_only && !it->browseable)
	  continue;

	if (p->input && (match = evry_fuzzy_match(it->label, p->input)))
	  {
	     it->fuzzy_match = match;
	     if (!it->browseable)
	       it->priority = 1;
	     EVRY_PLUGIN_ITEM_APPEND(p, it);
	     cnt++;
	  }
	else if (!p->input)
	  {
	     if (!it->browseable)
	       it->priority = 1;
	     EVRY_PLUGIN_ITEM_APPEND(p, it);
	     cnt++;
	  }
     }
   return cnt;
}

static void
_scan_cancel_func(void *data)
{
   Data *d = data;
   Plugin *p = d->plugin;
   Evry_Item_File *file;
   Evry_Item *item;

   if (!d->run_cnt)
     {
	EINA_LIST_FREE(d->files, file)
	  {
	     if (file->base.label) free((char *)(file->base.label));
	     if (file->path) free((char *)file->path);
	     free(file);
	  }
     }
   else
     {
	EINA_LIST_FREE(d->files, file)
	  {
	     if (file->path)
	       eina_stringshare_del(file->path);
	     if (EVRY_ITEM(file)->label)
	       eina_stringshare_del(EVRY_ITEM(file)->label);
	     if (EVRY_ITEM(file)->detail)
	       eina_stringshare_del(EVRY_ITEM(file)->detail);
	     free(file);
	  }
     }

   free(d->directory);
   E_FREE(d);

   p->thread = NULL;

   if (p->wait_finish)
     E_FREE(p);
}

static void
_scan_end_func(void *data)
{
   Data *d = data;
   Plugin *p = d->plugin;
   Evry_Item *item;
   Evry_Item_File *f;
   char *filename, *path, *mime;
   Eina_List *l, *ll, *lll;
   History_Item *hi;
   History_Entry *he;
   History_Types *ht = NULL;
   int cnt = 0;

   if (_conf->cache_dirs)
     ht = evry_history_types_get(evry_hist->subjects, EVRY_TYPE_FILE);

   if (!d->run_cnt)
     {
	EINA_LIST_FOREACH_SAFE(d->files, l, ll, item)
	  {
	     GET_FILE(file, item);

	     filename = (char *)item->label;
	     path = (char *) file->path;

	     if (!filename || !path)
	       goto next;

	     file->path = eina_stringshare_add(path);

	     if (item->browseable)
	       file->mime = eina_stringshare_ref(_mime_dir);

	     /* check if we can grab the mimetype from history */
	     if ((_conf->cache_dirs && ht) &&
		 (he = eina_hash_find(ht->types, file->path)))
	       {
		  EINA_LIST_FOREACH(he->items, lll, hi)
		    {
		       if (hi->data)
			 {
			    if (!file->mime)
			      file->mime = eina_stringshare_ref(hi->data);

			    DBG("cached: %s %s", file->mime, file->path);
			    hi->transient = 0;
			    break;
			 }
		    }
	       }

	     if (file->mime)
	       {
		  item->context = eina_stringshare_ref(file->mime);

		  d->files = eina_list_remove_list(d->files, l);
		  p->files = eina_list_append(p->files, file);
	       }

	     item->id = eina_stringshare_ref(file->path);
	     item->label = eina_stringshare_add(filename);
	     evry_util_file_detail_set(file);

	     E_FREE(filename);
	     E_FREE(path);
	     continue;

	  next:
	     E_FREE(filename);
	     E_FREE(path);
	     if (file->path) eina_stringshare_del(file->path);
	     d->files = eina_list_remove(d->files, file);
	     E_FREE(file);
	  }

	if (d->files)
	  {
	     d->run_cnt++;
	     d->files = eina_list_sort(d->files, -1, _cb_sort);
	     p->thread = ecore_thread_run(_scan_mime_func,
					  _scan_end_func,
					  _scan_cancel_func, d);
	     return;
	  }
     }
   else
     {
	EINA_LIST_FREE(d->files, item)
	  {
	     GET_FILE(file, item);

	     mime = (char *) file->mime;

	     if (!mime) break;

	     file->mime = eina_stringshare_ref(mime);
	     item->context = eina_stringshare_ref(mime);

	     p->files = eina_list_append(p->files, file);
	  }

	if (d->files)
	  {
	     d->run_cnt++;
	     p->thread = ecore_thread_run(_scan_mime_func,
					  _scan_end_func,
					  _scan_cancel_func, d);
	  }
     }

   if (!d->files)
     {
	if (!(p->command == CMD_SHOW_HIDDEN) &&  _conf->cache_dirs)
	  {
	     EINA_LIST_FOREACH(p->files, l, item)
	       {
		  GET_FILE(file, item);

		  if (!item->usage &&
		      (hi = evry_history_add(evry_hist->subjects, item, NULL, NULL)))
		    {
		       hi->last_used = SIX_DAYS_AGO - (0.001 * (double) cnt++);
		       hi->usage = TIME_FACTOR(hi->last_used);
		       hi->data = eina_stringshare_ref(file->mime);
		       item->hi = hi;
		    }
		  else if (item->hi && (item->hi->count == 1) &&
			   (item->hi->last_used < SIX_DAYS_AGO))
		    {
		       item->hi->last_used = SIX_DAYS_AGO - (0.001 * (double) cnt++);
		       item->hi->usage = TIME_FACTOR(hi->last_used);
		    }
	       }
	  }

	free(d->directory);
	E_FREE(d);
	p->thread = NULL;
     }

   p->files = eina_list_sort(p->files, -1, _cb_sort);

   _append_files(p);

   evry_plugin_async_update(EVRY_PLUGIN(p), EVRY_ASYNC_UPDATE_ADD);
}

static void
_dir_watcher(void *data, Ecore_File_Monitor *em, Ecore_File_Event event, const char *path)
{
   Plugin *p = data;
   Evry_Item_File *file;
   const char *label;
   Eina_List *ll, *l;

   switch (event)
     {
      case ECORE_FILE_EVENT_DELETED_SELF:
	 EINA_LIST_FREE(p->files, file)
	  evry_item_free(EVRY_ITEM(file));
	break;

      case ECORE_FILE_EVENT_CREATED_DIRECTORY:
      case ECORE_FILE_EVENT_CREATED_FILE:
	 DBG("added %s", path);

	 label = ecore_file_file_get(path);

	 file = EVRY_ITEM_NEW(Evry_Item_File, p, label, NULL, _item_free);
	 file->path = eina_stringshare_add(path);

	 if (event == ECORE_FILE_EVENT_CREATED_DIRECTORY)
	   file->mime = eina_stringshare_ref(_mime_dir);

	 _item_fill(file);

	 p->files = eina_list_append(p->files, file);

	 break;

      case ECORE_FILE_EVENT_DELETED_FILE:
      case ECORE_FILE_EVENT_DELETED_DIRECTORY:
	 label = eina_stringshare_add(path);
	 DBG("delete %s", path);

	 EINA_LIST_FOREACH_SAFE(p->files, l, ll, file)
	   {
	      if (file->path != label) continue;

	      p->files = eina_list_remove_list(p->files, l);

	      EVRY_ITEM_FREE(file);
	      break;
	   }
	 eina_stringshare_del(label);
	 break;

      default:
	 return;
     }

   _append_files(p);

   evry_plugin_async_update(EVRY_PLUGIN(p), EVRY_ASYNC_UPDATE_ADD);
}

static void
_read_directory(Plugin *p)
{
   Data *d = E_NEW(Data, 1);
   d->plugin = p;
   d->directory = strdup(p->directory);

   p->thread = ecore_thread_run(_scan_func, _scan_end_func, _scan_cancel_func, d);

   if (p->dir_mon)
     ecore_file_monitor_del(p->dir_mon);

   p->dir_mon = ecore_file_monitor_add(p->directory, _dir_watcher, p);
}

static Evry_Plugin *
_browse(Evry_Plugin *plugin, const Evry_Item *it)
{
   Plugin *p = NULL;

   if (!it || (it && !CHECK_TYPE(it, EVRY_TYPE_FILE)))
     return NULL;

   GET_PLUGIN(parent, plugin);
   GET_FILE(file, it);

   if (!evry_file_path_get(file) ||
       !ecore_file_is_dir(file->path))
     return NULL;

   p = E_NEW(Plugin, 1);
   p->base = *plugin;
   p->base.items = NULL;

   p->directory = eina_stringshare_add(file->path);
   p->parent = EINA_TRUE;

   _read_directory(p);

   return EVRY_PLUGIN(p);
}

static Evry_Plugin *
_begin(Evry_Plugin *plugin, const Evry_Item *it)
{
   Plugin *p = NULL;

   if (it && !CHECK_TYPE(it, EVRY_TYPE_ACTION))
     return NULL;

   p = E_NEW(Plugin, 1);
   p->base = *plugin;
   p->base.items = NULL;

   if (it)
     {
	/* provide object */

	p->directory = eina_stringshare_add(e_user_homedir_get());
	p->parent = EINA_FALSE;
	_read_directory(p);
     }
   else
     {
	/* provide subject */
	p->min_query = plugin->config->min_query;

	if (_conf->show_homedir)
	  p->directory = eina_stringshare_add(e_user_homedir_get());

	_read_directory(p);

	return EVRY_PLUGIN(p);
     }

   return EVRY_PLUGIN(p);
}

static void
_folder_item_add(Plugin *p, const char *path)
{
   Evry_Item_File *file;

   file = EVRY_ITEM_NEW(Evry_Item_File, p, path, NULL, _item_free);
   file->path = eina_stringshare_add(path);
   file->mime = eina_stringshare_ref(_mime_dir);
   EVRY_ITEM(file)->browseable = EINA_TRUE;
   p->files = eina_list_append(p->files, file);
   EVRY_PLUGIN_ITEM_APPEND(p, file);
}

static void
_free_files(Plugin *p)
{
   Evry_Item_File *file;

   if (p->thread)
     ecore_thread_cancel(p->thread);
   p->thread = NULL;

   EINA_LIST_FREE(p->files, file)
     EVRY_ITEM_FREE(file);

   if (p->dir_mon)
     ecore_file_monitor_del(p->dir_mon);
   p->dir_mon = NULL;
}

static void
_finish(Evry_Plugin *plugin)
{
   GET_PLUGIN(p, plugin);

   IF_RELEASE(p->input);
   IF_RELEASE(p->directory);

   if (p->thread)
     {
	ecore_thread_cancel(p->thread);
	p->wait_finish = 1;
	p->thread = NULL;
     }

   EVRY_PLUGIN_ITEMS_CLEAR(p);

   _free_files(p);

   if (!p->wait_finish)
     E_FREE(p);
}

static int
_fetch(Evry_Plugin *plugin, const char *input)
{
   GET_PLUGIN(p, plugin);
   Evry_Item_File *file;
   int len = (input ? strlen(input) : 0);

   if (!p->command)
     EVRY_PLUGIN_ITEMS_CLEAR(p);

   IF_RELEASE(p->input);

   if (!p->parent && input && !strncmp(input, "/", 1))
     {
	char *path = NULL;

	if ((p->command != CMD_SHOW_ROOT) ||
	    ((ecore_file_is_dir(input) ? (path = strdup(input)) : 0) ||
	     ((path = ecore_file_dir_get(input)) &&
	      (strcmp(p->directory, path)))))
	  {
	     _free_files(p);

	     eina_stringshare_del(p->directory);

	     if (path)
	       {
		  p->directory = eina_stringshare_add(path);
		  free(path);
	       }
	     else
	       {
		  p->directory = eina_stringshare_add("/");
	       }

	     _read_directory(p);

	     p->command = CMD_SHOW_ROOT;

	     return 0;
	  }
	int len = strlen(p->directory);
	len = (len == 1) ? len : len+1;

	p->input = eina_stringshare_add(input + len);
     }
   else if (p->directory && input && !strncmp(input, "..", 2))
     {
	if (p->command != CMD_SHOW_PARENT)
	  {
	     char *dir;
	     char buf[PATH_MAX];

	     if (strncmp(p->directory, "/", 1))
	       return 0;

	     _free_files(p);

	     strncpy(buf, p->directory, PATH_MAX);

	     _folder_item_add(p, p->directory);

	     while (strlen(buf) > 1)
	       {
		  buf[PATH_MAX - 1] = 0;
		  dir = dirname(buf);
		  _folder_item_add(p, dir);
		  strncpy(buf, dir, PATH_MAX);
	       }

	     p->command = CMD_SHOW_PARENT;
	  }
	return 1;
     }
   else if (p->directory && input && !strncmp(input, ".", 1))
     {
	if (p->command != CMD_SHOW_HIDDEN)
	  {
	     _free_files(p);

	     p->show_hidden = EINA_TRUE;
	     _read_directory(p);

	     p->command = CMD_SHOW_HIDDEN;

	     return 0;
	  }
	p->input = eina_stringshare_add(input);
     }
   else if (p->command)
     {
	/* clear command items */
	EINA_LIST_FREE(p->files, file)
	  evry_item_free(EVRY_ITEM(file));

	if (p->command == CMD_SHOW_ROOT)
	  {
	     if (p->directory)
	       eina_stringshare_del(p->directory);

	     p->directory = eina_stringshare_add(e_user_homedir_get());
	  }

	p->show_hidden = EINA_FALSE;
	_read_directory(p);

	p->command = CMD_NONE;
     }

   if (input && !p->command)
     p->input = eina_stringshare_add(input);

   if ((p->command) || (!p->min_query) || (len >= p->min_query))
     _append_files(p);

   return !!(EVRY_PLUGIN(p)->items);
}

/***************************************************************************/
/* recent files */

static int
_recentf_append_files(Plugin *p)
{
   int match;
   int cnt = 0;
   Evry_Item *it;
   Eina_List *l;

   EVRY_PLUGIN_ITEMS_CLEAR(p);

   EINA_LIST_FOREACH(p->files, l, it)
     {
	if (cnt >= MAX_SHOWN) break;

	if (p->dirs_only && !it->browseable)
	  continue;

	if (it->fuzzy_match < 0)
	  {
	     if ((match = evry_fuzzy_match(it->label, p->input)) ||
		 (match = evry_fuzzy_match(EVRY_FILE(it)->path, p->input)))
	       it->fuzzy_match = match;
	     else
	       it->fuzzy_match = 0;
	  }

	if (_conf->show_recent || it->fuzzy_match)
	  {
	     if (!it->browseable)
	       it->priority = 1;
	     EVRY_PLUGIN_ITEM_APPEND(p, it);
	     cnt++;
	  }
     }

   p->files = eina_list_sort(p->files, -1, _cb_sort);

   return cnt;
}

/* use thread only to not block ui for ecore_file_exists ... */
static void
_recentf_func(void *data)
{
   Data *d = data;
   Plugin *p = d->plugin;
   Eina_List *l;
   Evry_Item_File *file;
   History_Item *hi;

   EINA_LIST_FOREACH(d->files, l, file)
     {
	if ((!evry_file_path_get(file)) ||
	    (!ecore_file_exists(file->path)))
	  {
	     EVRY_ITEM(file)->hi->last_used -= ONE_DAY;
	     EVRY_ITEM(file)->hi = NULL;
	  }
     }
}

static void
_recentf_cancel_func(void *data)
{
   Data *d = data;
   Plugin *p = d->plugin;
   Evry_Item_File *file;

   EINA_LIST_FREE(d->files, file)
     {
	EVRY_ITEM_FREE(file);
     }

   E_FREE(d);

   if (p->wait_finish)
     E_FREE(p);
}

static void
_recentf_end_func(void *data)
{
   Data *d = data;
   Plugin *p = d->plugin;
   Eina_List *l, *ll;
   Evry_Item *it;
   const char *label;

   EINA_LIST_FREE(d->files, it)
     {
	GET_FILE(file, it);

	if (!it->hi)
	  {
	     evry_item_free(it);
	     continue;
	  }

	_item_fill(file);

	it->hi->data = eina_stringshare_ref(file->mime);

	p->files = eina_list_append(p->files, it);
     }

   _recentf_append_files(p);

   evry_plugin_async_update(EVRY_PLUGIN(p), EVRY_ASYNC_UPDATE_ADD);

   p->thread = NULL;
   E_FREE(d);
}

static Eina_Bool
_recentf_items_add_cb(const Eina_Hash *hash, const void *key, void *data, void *fdata)
{
   History_Entry *he = data;
   History_Item *hi = NULL, *hi2;
   Eina_List *l, *ll;
   Evry_Item_File *file;
   double last_used = 0.0;
   Data *d = fdata;
   Plugin *p = d->plugin;
   const char *label;
   const char *path;
   int match = 0;

   EINA_LIST_FOREACH(he->items, l, hi2)
     {
	if (hi2->last_used > last_used)
	  hi = hi2;
     }

   if (!hi)
     return EINA_TRUE;

   if (clear_cache)
     {
	DBG("clear %s", (char *)key);

	/* transient marks them for deletion */
	if ((hi->count == 1) && (hi->last_used < SIX_DAYS_AGO))
	  {
	     hi->usage = 0;
	     hi->transient = 1;
	  }

	return EINA_TRUE;
     }

   if (hi->transient)
     return EINA_TRUE;

   if (!_conf->search_cache)
     {
	if ((hi->count == 1) && (hi->last_used < SIX_DAYS_AGO))
	  return EINA_TRUE;
     }

   path = (const char *) key;

   if (!(label = ecore_file_file_get(path)))
     return EINA_TRUE;

   path = eina_stringshare_add(path);

   EINA_LIST_FOREACH(p->files, ll, file)
     {
	if (file->path == path)
	  {
	     DBG("already added %s", path);
	     eina_stringshare_del(path);
	     EVRY_ITEM(file)->fuzzy_match = -1;
	     return EINA_TRUE;
	  }
     }

   /* searching subdirs */
   if (p->directory && strncmp(path, p->directory, strlen(p->directory)))
     {
	DBG("not in dir %s", path);
	eina_stringshare_del(path);
	return EINA_TRUE;
     }

   if (!(match = evry_fuzzy_match(label, p->input)) &&
       !(match = evry_fuzzy_match(path, p->input)))
     {
	DBG("not matched %s", path);
	eina_stringshare_del(path);
	return EINA_TRUE;
     }

   DBG("add %s", path);

   file = EVRY_ITEM_NEW(Evry_Item_File, p, label, NULL, _item_free);

   file->path = path;

   if (hi->data)
     file->mime = eina_stringshare_ref(hi->data);

   EVRY_ITEM(file)->hi = hi;
   EVRY_ITEM(file)->fuzzy_match = match;
   EVRY_ITEM(file)->id = eina_stringshare_ref(file->path);

   d->files = eina_list_append(d->files, file);

   return EINA_TRUE;
}

static Evry_Plugin *
_recentf_browse(Evry_Plugin *plugin, const Evry_Item *it)
{
   Plugin *p = NULL;

   if (!it || (it && !CHECK_TYPE(it, EVRY_TYPE_FILE)))
     return NULL;

   GET_PLUGIN(parent, plugin);
   GET_FILE(file, it);

   if (!evry_file_path_get(file) ||
       !ecore_file_is_dir(file->path))
     return NULL;

   p = E_NEW(Plugin, 1);
   p->base = *plugin;
   p->base.items = NULL;

   p->directory = eina_stringshare_add(file->path);
   p->parent = EINA_TRUE;

   return EVRY_PLUGIN(p);
}

static Evry_Plugin *
_recentf_begin(Evry_Plugin *plugin, const Evry_Item *it)
{
   Plugin *p;

   if (it && !CHECK_TYPE(it, EVRY_TYPE_ACTION))
     return NULL;

   p = E_NEW(Plugin, 1);
   p->base = *plugin;
   p->base.items = NULL;
   p->parent = EINA_FALSE;

   if (it)
     {
	/* provide object */
     }
   else
     {
	/* provide subject */
	p->min_query = plugin->config->min_query;

	if (clear_cache)
	  {
	     History_Types *ht = evry_history_types_get(evry_hist->subjects, EVRY_TYPE_FILE);
	     if (ht)
	       eina_hash_foreach(ht->types, _recentf_items_add_cb, p);

	     clear_cache = EINA_FALSE;
	  }
     }

   return EVRY_PLUGIN(p);
}

static int
_recentf_fetch(Evry_Plugin *plugin, const char *input)
{
   GET_PLUGIN(p, plugin);
   Evry_Item_File *file;
   History_Types *ht;
   int len = (input ? strlen(input) : 0);

   EVRY_PLUGIN_ITEMS_CLEAR(p);

   IF_RELEASE(p->input);

   if (len >= plugin->config->min_query)
     {
	Eina_List *l;
	Evry_Item *it;

	if (input)
	  p->input = eina_stringshare_add(input);

	if ((ht = evry_history_types_get(evry_hist->subjects, EVRY_TYPE_FILE)))
	  {
	     Data *d = E_NEW(Data, 1);
	     d->plugin = p;
	     eina_hash_foreach(ht->types, _recentf_items_add_cb, d);
	     p->thread = ecore_thread_run(_recentf_func, _recentf_end_func,
					  _recentf_cancel_func, d);
	  }

	EINA_LIST_FOREACH(p->files, l, it)
	  it->fuzzy_match = -1;

	return _recentf_append_files(p);
     }

   return 0;
}

/***************************************************************************/
/* actions */

static int
_open_folder_check(Evry_Action *act __UNUSED__, const Evry_Item *it)
{
   return (it->browseable && e_action_find("fileman"));
}

static int
_open_folder_action(Evry_Action *act)
{
   E_Action *action;
   Eina_List *m;
   char *dir;

   if (!(action = e_action_find("fileman")))
     return 0;

   GET_FILE(file, act->it1.item);

   if (!(evry_file_path_get(file)))
     return 0;

   m = e_manager_list();

   if (!IS_BROWSEABLE(file))
     {
	dir = ecore_file_dir_get(file->path);
	if (!dir) return 0;
	action->func.go(E_OBJECT(m->data), dir);
	free(dir);
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
   GET_FILE(file, act->it1.item);
   Evry_Item_App *tmp;
   char cwd[4096];
   char *dir;
   int ret = 0;

   if (!(evry_file_path_get(file)))
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
	tmp->file = evry_conf->cmd_terminal;

	ret = evry_util_exec_app(EVRY_ITEM(tmp), NULL);
	E_FREE(tmp);
	E_FREE(dir);
	if (chdir(cwd))
	  return 0;
     }

   return ret;
}


static int
_file_trash_action(Evry_Action *act)
{
   Efreet_Uri *euri;
   int ok = 0;
   int force = (EVRY_ITEM_DATA_INT_GET(act) == ACT_DELETE);

   GET_FILE(file, act->it1.item);

   if (!(evry_file_uri_get(file)))
     return 0;

   euri = efreet_uri_decode(file->url);

   if (euri)
     {
	ok = efreet_trash_delete_uri(euri, force);
	efreet_uri_free(euri);
     }

   return (ok > 0);
}

static int
_file_copy_action(Evry_Action *act)
{
   GET_FILE(src, act->it1.item);
   GET_FILE(dst, act->it2.item);

   char buf[PATH_MAX];
   char *ddst;

   if (!(evry_file_path_get(src)))
     return 0;

   if (!(evry_file_path_get(dst)))
     return 0;

   if (!ecore_file_is_dir(dst->path))
     ddst = ecore_file_dir_get(dst->path);
   else
     ddst = strdup(dst->path);
   if (!ddst)
     return 0;

   snprintf(buf, sizeof(buf), "%s/%s", ddst, ecore_file_file_get(src->path));
   free(ddst);

   DBG(" %s -> %s\n", src->path, buf);

   if (EVRY_ITEM_DATA_INT_GET(act) == ACT_COPY)
     {
	return ecore_file_cp(src->path, buf);
     }
   else if (EVRY_ITEM_DATA_INT_GET(act) == ACT_MOVE)
     {
	return ecore_file_mv(src->path, buf);
     }

   return 0;
}

static Eina_Bool
_plugins_init(void)
{
   Evry_Action *act;
   Evry_Plugin *p;
   int prio = 0;

   if (!evry_api_version_check(EVRY_API_VERSION))
     return EINA_FALSE;

#define PLUGIN_NEW(_name, _icon, _begin, _finish, _fetch) \
   p = EVRY_PLUGIN_NEW(Evry_Plugin, _name, _icon, EVRY_TYPE_FILE, \
		       _begin, _finish, _fetch, NULL);	\
   p->config_path = "extensions/everything-files";	\
   _plugins = eina_list_append(_plugins, p);		\


   PLUGIN_NEW(N_("Files"), _module_icon,
	      _begin, _finish, _fetch);
   p->browse = &_browse;
   if (evry_plugin_register(p, EVRY_PLUGIN_SUBJECT, 2))
     p->config->min_query = 1;

   PLUGIN_NEW(N_("Files"), _module_icon,
	      _begin, _finish, _fetch);
   p->browse = &_browse;
   evry_plugin_register(p, EVRY_PLUGIN_OBJECT, 2);

   if (_conf->show_recent || _conf->search_recent)
     {
	PLUGIN_NEW(N_("Recent Files"), _module_icon,
		   _recentf_begin, _finish, _recentf_fetch);
	p->browse = &_recentf_browse;
	if (evry_plugin_register(p, EVRY_PLUGIN_SUBJECT, 3))
	  p->config->min_query = 3;

	PLUGIN_NEW(N_("Recent Files"), _module_icon,
		   _recentf_begin, _finish, _recentf_fetch);
	p->browse = &_recentf_browse;
	if (evry_plugin_register(p, EVRY_PLUGIN_OBJECT, 3))
	  p->config->min_query = 3;
     }

#undef PLUGIN_NEW

#define ACTION_NEW(_name, _type2, _icon, _act, _check)	\
   act = EVRY_ACTION_NEW(_name, EVRY_TYPE_FILE, _type2, _icon, _act, _check); \
   evry_action_register(act, prio++);			\
   _actions = eina_list_append(_actions, act);		\

   ACTION_NEW(N_("Copy To ..."), EVRY_TYPE_FILE, "go-next",
	      _file_copy_action, NULL);
   act->it2.subtype = EVRY_TYPE_DIR;
   EVRY_ITEM_DATA_INT_SET(act, ACT_COPY);

   ACTION_NEW(N_("Move To ..."), EVRY_TYPE_FILE, "go-next",
	      _file_copy_action, NULL);
   act->it2.subtype = EVRY_TYPE_DIR;
   EVRY_ITEM_DATA_INT_SET(act, ACT_MOVE);

   ACTION_NEW(N_("Move to Trash"), 0, "edit-delete",
	      _file_trash_action, NULL);
   EVRY_ITEM_DATA_INT_SET(act, ACT_TRASH);

   ACTION_NEW(N_("Open Folder (EFM)"), 0, "folder-open",
	      _open_folder_action, _open_folder_check);
   act->remember_context = EINA_TRUE;

   ACTION_NEW(N_("Open Terminal here"), 0, "system-run",
	      _open_term_action, NULL);
#undef ACTION_NEW

   return EINA_TRUE;
}

static void
_plugins_shutdown(void)
{
   Evry_Action *act;
   Evry_Plugin *p;

   EINA_LIST_FREE(_plugins, p)
     evry_plugin_free(p);

   EINA_LIST_FREE(_actions, act)
     evry_action_free(act);
}


/***************************************************************************/

static E_Config_DD *conf_edd = NULL;

struct _E_Config_Dialog_Data
{
  int show_homedir;
  int show_recent;
  int search_recent;
  int search_cache;
  int cache_dirs;
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

   if (e_config_dialog_find("everything-files", "extensions/everything-files")) return NULL;

   v = E_NEW(E_Config_Dialog_View, 1);
   if (!v) return NULL;

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;
   v->basic.apply_cfdata = _basic_apply;

   cfd = e_config_dialog_new(con, _("Everything Files"), "everything-files",
			     "extensions/everything-files", _module_icon, 0, v, NULL);

   _conf->cfd = cfd;
   return cfd;
}

static void
_clear_cache_cb(void *data, void *data2)
{
   clear_cache = EINA_TRUE;
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o = NULL, *of = NULL, *ow = NULL;

   o = e_widget_list_add(evas, 0, 0);

   of = e_widget_framelist_add(evas, _("General"), 0);
   e_widget_framelist_content_align_set(of, 0.0, 0.0);

   ow = e_widget_check_add(evas, _("Show home directory"),
			   &(cfdata->show_homedir));
   e_widget_framelist_object_append(of, ow);

   ow = e_widget_check_add(evas, _("Show recent files"),
			   &(cfdata->show_recent));
   e_widget_framelist_object_append(of, ow);

   ow = e_widget_check_add(evas, _("Search recent files"),
			   &(cfdata->search_recent));
   e_widget_framelist_object_append(of, ow);

   ow = e_widget_check_add(evas, _("Search cached files"),
			   &(cfdata->search_cache));
   e_widget_framelist_object_append(of, ow);

   ow = e_widget_check_add(evas, _("Cache visited directories"),
			   &(cfdata->cache_dirs));
   e_widget_framelist_object_append(of, ow);

   ow = e_widget_button_add(evas, _("Clear cache"), NULL,
   			   _clear_cache_cb,
   			   NULL, NULL);
   e_widget_framelist_object_append(of, ow);


   e_widget_list_object_append(o, of, 1, 1, 0.5);
   return o;
}

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
#define C(_name) cfdata->_name = _conf->_name;
   C(show_homedir);
   C(show_recent);
   C(search_recent);
   C(search_cache);
   C(cache_dirs);
#undef C
}

static int
_basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
#define C(_name) _conf->_name = cfdata->_name;
   C(show_homedir);
   C(show_recent);
   C(search_recent);
   C(search_cache);
   C(cache_dirs);
#undef C

   e_config_domain_save("module.everything-files", conf_edd, _conf);
   e_config_save_queue();
   return 1;
}

static void
_conf_new(void)
{
   _conf = E_NEW(Module_Config, 1);
   _conf->version = (MOD_CONFIG_FILE_EPOCH << 16);

#define IFMODCFG(v) if ((_conf->version & 0xffff) < v) {
#define IFMODCFGEND }

   /* setup defaults */
   IFMODCFG(0x008d);
   _conf->show_recent = 0;
   _conf->show_homedir = 1;
   _conf->search_recent = 1;
   _conf->cache_dirs = 0;
   _conf->search_cache = 0;
   IFMODCFGEND;

   _conf->version = MOD_CONFIG_FILE_VERSION;

   e_config_domain_save("module.everything-files", conf_edd, _conf);
   e_config_save_queue();
}

static void
_conf_free(void)
{
   E_FREE(_conf);
}

static void
_conf_init(E_Module *m)
{
   e_configure_registry_category_add("extensions", 80, _("Extensions"),
				     NULL, "preferences-extensions");

   e_configure_registry_item_add("extensions/everything-files", 110, _("Everything Files"),
				 NULL, _module_icon, _conf_dialog);

   conf_edd = E_CONFIG_DD_NEW("Module_Config", Module_Config);

#undef T
#undef D
#define T Module_Config
#define D conf_edd
   E_CONFIG_VAL(D, T, version, INT);
   E_CONFIG_VAL(D, T, show_homedir, UCHAR);
   E_CONFIG_VAL(D, T, show_recent, UCHAR);
   E_CONFIG_VAL(D, T, search_recent, UCHAR);
   E_CONFIG_VAL(D, T, search_cache, UCHAR);
   E_CONFIG_VAL(D, T, cache_dirs, UCHAR);
#undef T
#undef D

   _conf = e_config_domain_load("module.everything-files", conf_edd);

   if (_conf && !evry_util_module_config_check(_("Everything Files"), _conf->version,
					       MOD_CONFIG_FILE_EPOCH, MOD_CONFIG_FILE_VERSION))
	  _conf_free();

   if (!_conf) _conf_new();

   _conf->module = m;
}

static void
_conf_shutdown(void)
{
   E_FREE(_conf);

   E_CONFIG_DD_FREE(conf_edd);
}

/***************************************************************************/

static Eina_Bool active = EINA_FALSE;

EAPI E_Module_Api e_modapi =
{
   E_MODULE_API_VERSION,
   "everything-files"
};

EAPI void *
e_modapi_init(E_Module *m)
{
   _conf_init(m);

   if (e_datastore_get("everything_loaded"))
     active = _plugins_init();

   _mime_dir = eina_stringshare_add("inode/directory");
   _mime_mount = eina_stringshare_add("inode/mountpoint");

   e_module_delayed_set(m, 1);

   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   if (active && e_datastore_get("everything_loaded"))
     _plugins_shutdown();

   eina_stringshare_del(_mime_dir);
   eina_stringshare_del(_mime_mount);

   _conf_shutdown();

   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   e_config_domain_save("module.everything-files", conf_edd, _conf);
   return 1;
}
