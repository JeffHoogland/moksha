/***************************************************
  TODO option for maximum items to cache
  TODO keep common list for recent file instances
FIXME
*/

#include "e.h"
#include "evry_api.h"
#include <Efreet_Trash.h>

#define MOD_CONFIG_FILE_EPOCH 0x0001
#define MOD_CONFIG_FILE_GENERATION 0x008d
#define MOD_CONFIG_FILE_VERSION                                 \
     ((MOD_CONFIG_FILE_EPOCH << 16) | MOD_CONFIG_FILE_GENERATION)

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
#define ACT_SORT_DATE	5
#define ACT_SORT_NAME	6

#define ONE_DAY  86400.0
#define SIX_DAYS_AGO (ecore_time_unix_get() - ONE_DAY * 6)
#define MIN_USAGE 0.0

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
  Eina_Bool sort_by_date;

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

static const Evry_API *evry = NULL;
static Evry_Module *evry_module = NULL;

static Module_Config *_conf;
static char _module_icon[] = "system-file-manager";
static Eina_List *_plugins = NULL;
static Eina_List *_actions = NULL;
static const char *_mime_dir;
static const char *_mime_mount;
static const char *_mime_unknown;
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
     evry->util_file_detail_set(file);


     evry->util_file_detail_set(file);
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

static int
_cb_sort_date(const void *data1, const void *data2)
{
   const Evry_Item_File *it1 = data1;
   const Evry_Item_File *it2 = data2;

   return it2->modified - it1->modified;
}

static void
_item_free(Evry_Item *it)
{
   GET_FILE(file, it);

   IF_RELEASE(file->url);
   IF_RELEASE(file->path);
   IF_RELEASE(file->mime);

   E_FREE(file);
}

static void
_scan_func(void *data, Ecore_Thread *thread)
{
   Data *d = data;
   Eina_Iterator *ls;
   Eina_File_Direct_Info *info;
   Evry_Item_File *file;

   if (!(ls = eina_file_stat_ls(d->directory)))
     return;

   EINA_ITERATOR_FOREACH(ls, info)
     {
	if ((d->plugin->show_hidden) != (*(info->path + info->name_start) == '.'))
	  continue;

	file = EVRY_ITEM_NEW(Evry_Item_File, d->plugin, NULL, NULL, _item_free);
	file->path = strdup(info->path);
	EVRY_ITEM(file)->label = strdup(info->path + info->name_start);
	EVRY_ITEM(file)->browseable = (info->type == EINA_FILE_DIR);

	d->files = eina_list_append(d->files, file);
	/* TODO dont append files in thread, run mime scan
	 * simultaneously. ecore_thread_feedback(thread, file); */

	if (ecore_thread_check(thread))
	  break;
     }

   eina_iterator_free(ls);
}

static void
_scan_mime_func(void *data, Ecore_Thread *thread)
{
   Data *d = data;
   Evry_Item_File *file;
   Eina_List *l;
   int cnt = 0;

   EINA_LIST_FOREACH(d->files, l, file)
     {
	if (ecore_thread_check(thread))
	  break;

	if ((file->mime = efreet_mime_type_get(file->path)))
	  {
	     if (!strncmp(file->mime, "inode/", 6) &&
		 ecore_file_is_dir(file->path))
	       EVRY_ITEM(file)->browseable = EINA_TRUE;
	  }
	else
	  file->mime = _mime_unknown;

	if (cnt++ > MAX_ITEMS * d->run_cnt) break;
     }
}

static int
_files_filter(Plugin *p)
{
   int match;
   int cnt = 0;
   Evry_Item *it;
   Eina_List *l;
   unsigned int len = p->input ? strlen(p->input) : 0;

   EVRY_PLUGIN_ITEMS_CLEAR(p);

   if (!p->command && p->min_query)
     {
	if (!p->input)
	  return 0;
	if (len < p->min_query)
	  return 0;
     }

   EINA_LIST_FOREACH(p->files, l, it)
     {
	if (cnt >= MAX_SHOWN) break;

	if (p->dirs_only && !it->browseable)
	  continue;

	if (len && (match = evry->fuzzy_match(it->label, p->input)))
	  {
	     it->fuzzy_match = match;
	     if (!it->browseable)
	       it->priority = 1;
	     EVRY_PLUGIN_ITEM_APPEND(p, it);
	     cnt++;
	  }
	else if (len == 0)
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
_scan_cancel_func(void *data, Ecore_Thread *thread __UNUSED__)
{
   Data *d = data;
   Plugin *p = d->plugin;
   Evry_Item_File *file;

   EINA_LIST_FREE(p->files, file)
     EVRY_ITEM_FREE(file);

   EINA_LIST_FREE(d->files, file)
     {
	if (file->base.label) free((char *)(file->base.label));
	if (file->path) free((char *)file->path);
	E_FREE(file);
     }

   p->thread = NULL;

   if (p->wait_finish)
     E_FREE(p);

   free(d->directory);
   E_FREE(d);
}

static void
_cache_mime_get(History_Types *ht, Evry_Item_File *file)
{
   History_Entry *he;
   History_Item *hi;
   Eina_List *l;

   if ((he = eina_hash_find(ht->types, file->path)))
     {
	EINA_LIST_FOREACH(he->items, l, hi)
	  {
	     if (!hi->data)
	       continue;

	     if (!file->mime)
	       file->mime = hi->data;

	     hi->transient = 0;
	     EVRY_ITEM(file)->hi = hi;
	     break;
	  }
     }
}

static void
_cache_dir_add(Eina_List *files)
{
   Eina_List *l;
   Evry_Item *item;
   History_Item *hi;
   int cnt = 0;

   EINA_LIST_REVERSE_FOREACH(files, l, item)
     {
	GET_FILE(file, item);

	if (!(item->hi) &&
	    (hi = evry->history_item_add(item, NULL, NULL)))
	  {
	     hi->last_used = SIX_DAYS_AGO;
	     hi->usage = MIN_USAGE * (double) cnt++;
	     hi->data = eina_stringshare_ref(file->mime);
	     item->hi = hi;
	  }
	else if (item->hi && (item->hi->count == 1) &&
		 (item->hi->last_used < SIX_DAYS_AGO))
	  {
	     item->hi->last_used = SIX_DAYS_AGO;
	     item->hi->usage = MIN_USAGE * (double) cnt++;
	  }
     }
}

static void
_file_add(Plugin *p, Evry_Item *item)
{
   GET_FILE(file, item);

   char *filename = (char *)item->label;
   char *path = (char *) file->path;

   file->path = eina_stringshare_add(path);
   file->mime = eina_stringshare_ref(file->mime);

   item->label = eina_stringshare_add(filename);
   item->id = eina_stringshare_ref(file->path);
   item->context = eina_stringshare_ref(file->mime);

   evry->util_file_detail_set(file);

   p->files = eina_list_append(p->files, file);

   E_FREE(filename);
   E_FREE(path);
}

static void
_scan_end_func(void *data, Ecore_Thread *thread __UNUSED__)
{
   Data *d = data;
   Plugin *p = d->plugin;
   Evry_Item *item;
   Evry_Item_File *file;
   Eina_List *l, *ll;
   History_Types *ht = NULL;

   if (_conf->cache_dirs)
     ht = evry->history_types_get(EVRY_TYPE_FILE);

   if (!d->run_cnt) /* _scan_func finished */
     {
	EINA_LIST_FOREACH_SAFE(d->files, l, ll, item)
	  {
	     GET_FILE(file, item);

	     if (item->browseable)
	       file->mime = _mime_dir;
	     else if (ht)
	       _cache_mime_get(ht, file);

	     if (file->mime)
	       {
		  d->files = eina_list_remove_list(d->files, l);
		  _file_add(p, item);
	       }
	  }

	/* sort files by name for mimetypes scan */
	if (d->files)
	  d->files = eina_list_sort(d->files, -1, _cb_sort);
     }
   else	/* _scan_mime_func finished */
     {
	EINA_LIST_FREE(d->files, file)
	  {
	     if (!file->mime) break;
	     _file_add(p, (Evry_Item*) file);
	  }
     }

   if (d->files) /* scan mimetypes */
     {
	d->run_cnt++;
	p->thread = ecore_thread_run(_scan_mime_func,
				     _scan_end_func,
				     _scan_cancel_func, d);

	/* wait for first mime scan to finish */
	if (d->run_cnt == 1) return;
     }
   else /* finished all file/mime scan */
     {
	free(d->directory);
	E_FREE(d);
	p->thread = NULL;

	if (_conf->cache_dirs && !(p->command == CMD_SHOW_HIDDEN))
	  _cache_dir_add(p->files);
     }

   p->files = eina_list_sort(p->files, -1, _cb_sort);

   _files_filter(p);

   EVRY_PLUGIN_UPDATE(p, EVRY_UPDATE_ADD);
}

static void
_dir_watcher(void *data, Ecore_File_Monitor *em __UNUSED__, Ecore_File_Event event, const char *path)
{
   Plugin *p = data;
   Evry_Item_File *file;
   const char *label;
   Eina_List *ll, *l;

   switch (event)
     {
      case ECORE_FILE_EVENT_DELETED_SELF:
	 EINA_LIST_FREE(p->files, file)
	  evry->item_free(EVRY_ITEM(file));
	break;

      case ECORE_FILE_EVENT_CREATED_DIRECTORY:
      case ECORE_FILE_EVENT_CREATED_FILE:
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

   _files_filter(p);

   EVRY_PLUGIN_UPDATE(p, EVRY_UPDATE_ADD);
}

static void
_read_directory(Plugin *p)
{
   Data *d = E_NEW(Data, 1);
   d->plugin = p;
   d->directory = strdup(p->directory);
   d->run_cnt = 0;

   p->thread = ecore_thread_run(_scan_func, _scan_end_func, _scan_cancel_func, d);

   if (p->dir_mon)
     ecore_file_monitor_del(p->dir_mon);

   p->dir_mon = ecore_file_monitor_add(p->directory, _dir_watcher, p);
}

static Evry_Plugin *
_browse(Evry_Plugin *plugin, const Evry_Item *it)
{
   Plugin *p = NULL;

   if (!it || !(CHECK_TYPE(it, EVRY_TYPE_FILE)))
     return NULL;

   GET_FILE(file, it);

   if (!evry->file_path_get(file) ||
       !ecore_file_is_dir(file->path))
     return NULL;

   EVRY_PLUGIN_INSTANCE(p, plugin);

   p->directory = eina_stringshare_add(file->path);
   p->parent = EINA_TRUE;

   _read_directory(p);

   return EVRY_PLUGIN(p);
}

static Evry_Plugin *
_begin(Evry_Plugin *plugin, const Evry_Item *it)
{
   Plugin *p = NULL;

   if (it)
     {
	const char *dir = NULL;

	if ((CHECK_TYPE(it, EVRY_TYPE_FILE)) ||
	    (CHECK_SUBTYPE(it, EVRY_TYPE_FILE)))
	  {
	     /* browse */
	     GET_FILE(file, it);
	     if (!evry->file_path_get(file))
	       return NULL;

	     if (!ecore_file_is_dir(file->path))
	       {
		  char *tmp = ecore_file_dir_get(file->path);
		  dir = eina_stringshare_add(tmp);
		  E_FREE(tmp);
	       }
	     else
	       {
		  dir = eina_stringshare_add(file->path);
	       }
	  }
	else
	  {
	     /* provide object */
	     if (!CHECK_TYPE(it, EVRY_TYPE_ACTION))
	       return NULL;
	  }

	if (!dir)
	  dir = eina_stringshare_add(e_user_homedir_get());

	EVRY_PLUGIN_INSTANCE(p, plugin);
	p->directory = dir;
	p->parent = EINA_FALSE;
	p->min_query = 0;
	_read_directory(p);

	return EVRY_PLUGIN(p);
     }
   else
     {
	/* provide subject */
	EVRY_PLUGIN_INSTANCE(p, plugin);
	p->parent = EINA_FALSE;
	p->directory = eina_stringshare_add(e_user_homedir_get());
	p->min_query = plugin->config->min_query;
	_read_directory(p);

	return EVRY_PLUGIN(p);
     }

   return NULL;
}

static void
_folder_item_add(Plugin *p, const char *path, int prio)
{
   Evry_Item_File *file;

   file = EVRY_ITEM_NEW(Evry_Item_File, p, path, NULL, _item_free);
   file->path = eina_stringshare_add(path);
   file->mime = eina_stringshare_ref(_mime_dir);
   EVRY_ITEM(file)->browseable = EINA_TRUE;
   EVRY_ITEM(file)->priority = prio;
   EVRY_ITEM(file)->usage = -1;
   p->files = eina_list_append(p->files, file);
   EVRY_PLUGIN_ITEM_APPEND(p, file);
}

static void
_free_files(Plugin *p)
{
   Evry_Item_File *file;

   EVRY_PLUGIN_ITEMS_CLEAR(p);

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

   _free_files(p);

   if (!p->wait_finish)
     E_FREE(p);
}

static int
_fetch(Evry_Plugin *plugin, const char *input)
{
   GET_PLUGIN(p, plugin);
   unsigned int len = (input ? strlen(input) : 0);

   if (!p->command)
     EVRY_PLUGIN_ITEMS_CLEAR(p);

   IF_RELEASE(p->input);

   if (!p->parent && input && !strncmp(input, "/", 1))
     {
	char *path = NULL;

	if (p->command != CMD_SHOW_ROOT)
	  {
	     _free_files(p);

	     IF_RELEASE(p->directory);

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
	     int prio = 0;

	     if (strncmp(p->directory, "/", 1))
	       return 0;

	     _free_files(p);

	     strncpy(buf, p->directory, PATH_MAX);

	     _folder_item_add(p, p->directory, prio++);

	     while (strlen(buf) > 1)
	       {
		  buf[PATH_MAX - 1] = 0;
		  dir = dirname(buf);
		  _folder_item_add(p, dir, prio++);
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
	_free_files(p);

	if (p->command == CMD_SHOW_ROOT)
	  {
	     IF_RELEASE(p->directory);
	     p->directory = eina_stringshare_add(e_user_homedir_get());
	  }

	p->command = CMD_NONE;
	p->show_hidden = EINA_FALSE;

	_read_directory(p);
     }

   if (input && !p->command)
     p->input = eina_stringshare_add(input);

   if ((p->command) || (!p->min_query) || (len >= p->min_query))
     _files_filter(p);

   return !!(EVRY_PLUGIN(p)->items);
}

/***************************************************************************/
/* recent files */

static int
_cb_sort_recent(const void *data1, const void *data2)
{
   const Evry_Item *it1 = data1;
   const Evry_Item *it2 = data2;

   if (it1->browseable && !it2->browseable)
     return -1;

   if (!it1->browseable && it2->browseable)
     return 1;

   if (it1->hi && it2->hi)
     return (it1->hi->last_used > it2->hi->last_used ? -1 : 1);

   if (it1->fuzzy_match && it2->fuzzy_match)
     if (it1->fuzzy_match - it2->fuzzy_match)
       return (it1->fuzzy_match - it2->fuzzy_match);

   return strcasecmp(it1->label, it2->label);
}

static int
_recentf_files_filter(Plugin *p)
{
   int match;
   int cnt = 0;
   Evry_Item *it;
   Eina_List *l, *new = NULL;

   EVRY_PLUGIN_ITEMS_CLEAR(p);

   EINA_LIST_FOREACH(p->files, l, it)
     {
	if (p->dirs_only && !it->browseable)
	  continue;

	if (it->fuzzy_match <= 0)
	  {
	     if ((match = evry->fuzzy_match(it->label, p->input)) ||
		 (match = evry->fuzzy_match(EVRY_FILE(it)->path, p->input)))
	       it->fuzzy_match = match;
	     else
	       it->fuzzy_match = 0;

	     DBG("check match %d %s", it->fuzzy_match, it->label);
	  }

	if (_conf->show_recent || it->fuzzy_match)
	  {
	     if (!it->browseable)
	       it->priority = 1;
	     new = eina_list_append(new, it);
	  }
     }

   new = eina_list_sort(new, -1, _cb_sort_recent);

   EINA_LIST_FREE(new, it)
     {
	if (cnt++ < MAX_SHOWN)
	  EVRY_PLUGIN_ITEM_APPEND(p, it);
     }

   return cnt;
}


#if 0
/* use thread only to not block ui for ecore_file_exists ... */

static void
_recentf_func(void *data)
{
   Data *d = data;
   Eina_List *l;
   Evry_Item_File *file;

   EINA_LIST_FOREACH(d->files, l, file)
     {
	if ((!evry->file_path_get(file)) ||
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
   Evry_Item *it;

   EINA_LIST_FREE(d->files, it)
     {
	GET_FILE(file, it);

	if (!it->hi)
	  {
	     evry->item_free(it);
	     continue;
	  }

	_item_fill(file);

	if (!it->hi->data)
	  it->hi->data = eina_stringshare_ref(file->mime);

	p->files = eina_list_append(p->files, it);
     }

   _recentf_files_filter(p);

   EVRY_PLUGIN_UPDATE(p, EVRY_UPDATE_ADD);

   p->thread = NULL;
   E_FREE(d);
}
#endif

static Eina_Bool
_recentf_items_add_cb(const Eina_Hash *hash __UNUSED__, const void *key, void *data, void *fdata)
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
     if (hi2->last_used > last_used)
       {
	  last_used = hi2->last_used;
	  hi = hi2;
       }

   if (!hi)
     return EINA_TRUE;

   if (clear_cache)
     {
	DBG("clear %s", (char *)key);

	/* transient marks them for deletion */
	if (hi->count == 1)
	  {
	     hi->usage = 0;
	     hi->count = 0;
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
	     eina_stringshare_del(path);
	     EVRY_ITEM(file)->fuzzy_match = -1;
	     return EINA_TRUE;
	  }
     }

   /* searching subdirs */
   if (p->directory)
     {
	/* dont show recent files from same dir */
	int len = strlen(p->directory);
	char *end = strrchr(path, '/');
	if (strncmp(path, p->directory, len) ||
	    (end - path) <= len)
	  {
	     /* DBG("not in dir %s", path); */
	     eina_stringshare_del(path);
	     return EINA_TRUE;
	  }
     }

   if (!(match = evry->fuzzy_match(label, p->input)) &&
       !(match = evry->fuzzy_match(path, p->input)))
     {
	eina_stringshare_del(path);
	return EINA_TRUE;
     }

   file = EVRY_ITEM_NEW(Evry_Item_File, p, label, NULL, _item_free);
   file->path = path;

   if (hi->data)
     file->mime = eina_stringshare_add(hi->data);

   EVRY_ITEM(file)->hi = hi;
   EVRY_ITEM(file)->fuzzy_match = match;
   EVRY_ITEM(file)->id = eina_stringshare_ref(file->path);

   _item_fill(file);

   if (!hi->data)
     hi->data = eina_stringshare_ref(file->mime);

   d->files = eina_list_append(d->files, file);

   if (eina_list_count(d->files) > 100)
     return EINA_FALSE;

   return EINA_TRUE;
}

static Evry_Plugin *
_recentf_browse(Evry_Plugin *plugin, const Evry_Item *it)
{
   Plugin *p = NULL;

   if (!it || !CHECK_TYPE(it, EVRY_TYPE_FILE))
     return NULL;

   GET_FILE(file, it);

   if (!evry->file_path_get(file) ||
       !ecore_file_is_dir(file->path))
     return NULL;

   EVRY_PLUGIN_INSTANCE(p, plugin);
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

   EVRY_PLUGIN_INSTANCE(p, plugin);
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
	     History_Types *ht = evry->history_types_get(EVRY_TYPE_FILE);
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

   IF_RELEASE(p->input);

   /* if (p->thread)
    *   ecore_thread_cancel(p->thread);
    * p->thread = NULL; */

   if (input && isspace(input[len - 1]))
     return !!(plugin->items);

   if (len >= plugin->config->min_query)
     {
	if (input)
	  p->input = eina_stringshare_add(input);

	if ((ht = evry->history_types_get(EVRY_TYPE_FILE)))
	  {
	     Data *d = E_NEW(Data, 1);
	     d->plugin = p;
	     eina_hash_foreach(ht->types, _recentf_items_add_cb, d);
	     EINA_LIST_FREE(d->files, file)
	       p->files = eina_list_append(p->files, file);
	     E_FREE(d);

	     _recentf_files_filter(p);

	     /* _recentf_end_func(d);
	      * p->thread = NULL; */
	     /* p->thread = ecore_thread_run(_recentf_func, _recentf_end_func,
	      * 				  _recentf_cancel_func, d); */
	  }
	return !!(plugin->items);
     }

   EVRY_PLUGIN_ITEMS_CLEAR(p);

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

   if (!(evry->file_path_get(file)))
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
_file_trash_action(Evry_Action *act)
{
   Efreet_Uri *euri;
   int ok = 0;
   int force = (EVRY_ITEM_DATA_INT_GET(act) == ACT_DELETE);

   GET_FILE(file, act->it1.item);

   if (!(evry->file_url_get(file)))
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

   if (!(evry->file_path_get(src)))
     return 0;

   if (!(evry->file_path_get(dst)))
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

static void
_sort_by_date(Plugin *p)
{
   Eina_List *l;
   Evry_Item_File *file;
   struct stat s;

   EINA_LIST_FOREACH(p->files, l, file)
     {
	if (file->modified)
	  continue;

	if (lstat(file->path, &s) == 0)
	  file->modified = s.st_mtime;

	EVRY_ITEM(file)->usage = -1;
     }

   p->files = eina_list_sort(p->files, -1, _cb_sort_date);
   _files_filter(p);

   EVRY_PLUGIN_UPDATE(p, EVRY_UPDATE_ADD);

}
static void
_sort_by_name(Plugin *p)
{
   Eina_List *l;
   Evry_Item *it;

   EINA_LIST_FOREACH(p->files, l, it)
     it->usage = 0;

   p->files = eina_list_sort(p->files, -1, _cb_sort);
   _files_filter(p);

   EVRY_PLUGIN_UPDATE(p, EVRY_UPDATE_ADD);

}

static int
_file_sort_action(Evry_Action *act)
{
   GET_PLUGIN(p, act->it1.item);
   if (!p) return 0;

   if (EVRY_ITEM_DATA_INT_GET(act) == ACT_SORT_DATE)
     {
	_sort_by_date(p);
     }
   else
     {
	_sort_by_name(p);
     }

   return 0;
}

static int
_cb_key_down(Evry_Plugin *plugin, const Ecore_Event_Key *ev)
{
   GET_PLUGIN(p, plugin);

   if (!strcmp(ev->key, "F1"))
     {
	_sort_by_name(p);
	return 1;
     }
   if (!strcmp(ev->key, "F2"))
     {
	_sort_by_date(p);
	return 1;
     }

   return 0;
}


static int
_plugins_init(const Evry_API *api)
{
   Evry_Action *act, *act_sort_date, *act_sort_name;
   Evry_Plugin *p;
   int prio = 0;

   evry = api;

   if (!evry->api_version_check(EVRY_API_VERSION))
     return EINA_FALSE;

   _mime_dir     = eina_stringshare_add("inode/directory");
   _mime_mount   = eina_stringshare_add("inode/mountpoint");
   _mime_unknown = eina_stringshare_add("unknown");

#define ACTION_NEW(_name, _type2, _icon, _act, _check, _register)	\
   act = EVRY_ACTION_NEW(_name, EVRY_TYPE_FILE, _type2, _icon, _act, _check); \
   if (_register) evry->action_register(act, prio++);			\
   _actions = eina_list_append(_actions, act);				\

   ACTION_NEW("Copy To ...", EVRY_TYPE_FILE, "go-next",
	      _file_copy_action, NULL, 1);
   act->it2.subtype = EVRY_TYPE_DIR;
   EVRY_ITEM_DATA_INT_SET(act, ACT_COPY);

   ACTION_NEW("Move To ...", EVRY_TYPE_FILE, "go-next",
	      _file_copy_action, NULL, 1);
   act->it2.subtype = EVRY_TYPE_DIR;
   EVRY_ITEM_DATA_INT_SET(act, ACT_MOVE);

   ACTION_NEW("Move to Trash", 0, "user-trash",
	      _file_trash_action, NULL, 1);
   EVRY_ITEM_DATA_INT_SET(act, ACT_TRASH);

   ACTION_NEW("Open Folder (EFM)", 0, "folder-open",
	      _open_folder_action, _open_folder_check, 1);
   act->remember_context = EINA_TRUE;

   ACTION_NEW("Sort by Date", 0, "go-up",
	      _file_sort_action, NULL, 0);
   EVRY_ITEM_DATA_INT_SET(act, ACT_SORT_DATE);
   act_sort_date = act;

   ACTION_NEW("Sort by Name", 0, "go-up",
	      _file_sort_action, NULL, 0);
   EVRY_ITEM_DATA_INT_SET(act, ACT_SORT_NAME);
   act_sort_name = act;

#undef ACTION_NEW


   p = EVRY_PLUGIN_BASE("Files", _module_icon, EVRY_TYPE_FILE,
			_begin, _finish, _fetch);
   p->input_type = EVRY_TYPE_FILE;
   p->cb_key_down = &_cb_key_down;
   p->browse = &_browse;
   p->config_path = "extensions/everything-files";
   p->actions = eina_list_append(p->actions, act_sort_date);
   p->actions = eina_list_append(p->actions, act_sort_name);
   _plugins = eina_list_append(_plugins, p);

   if (evry->plugin_register(p, EVRY_PLUGIN_SUBJECT, 2))
     p->config->min_query = 1;


   p = EVRY_PLUGIN_BASE("Files", _module_icon, EVRY_TYPE_FILE,
			_begin, _finish, _fetch);
   p->cb_key_down = &_cb_key_down;
   p->browse = &_browse;
   p->config_path = "extensions/everything-files";
   p->actions = eina_list_append(p->actions, act_sort_date);
   p->actions = eina_list_append(p->actions, act_sort_name);
   _plugins = eina_list_append(_plugins, p);
   evry->plugin_register(p, EVRY_PLUGIN_OBJECT, 2);

   if (!_conf->show_recent && !_conf->search_recent)
     return EINA_TRUE;

   p = EVRY_PLUGIN_BASE("Recent Files", _module_icon, EVRY_TYPE_FILE,
			_recentf_begin, _finish, _recentf_fetch);
   p->browse = &_recentf_browse;
   p->config_path = "extensions/everything-files";

   if (evry->plugin_register(p, EVRY_PLUGIN_SUBJECT, 3))
     {
	p->config->top_level = EINA_FALSE;
	p->config->min_query = 3;
     }

   p = EVRY_PLUGIN_BASE("Recent Files", _module_icon, EVRY_TYPE_FILE,
			_recentf_begin, _finish, _recentf_fetch);
   p->browse = &_recentf_browse;
   p->config_path = "extensions/everything-files";

   if (evry->plugin_register(p, EVRY_PLUGIN_OBJECT, 3))
     {
	p->config->top_level = EINA_FALSE;
	p->config->min_query = 3;
     }

   return EINA_TRUE;
}

static void
_plugins_shutdown(void)
{
   Evry_Action *act;
   Evry_Plugin *p;

   eina_stringshare_del(_mime_dir);
   eina_stringshare_del(_mime_mount);
   eina_stringshare_del(_mime_unknown);

   EINA_LIST_FREE(_plugins, p)
     {
	if (p->actions)
	  eina_list_free(p->actions);
	EVRY_PLUGIN_FREE(p);
     }

   EINA_LIST_FREE(_actions, act)
     evry->action_free(act);
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
_conf_dialog(E_Container *con, const char *params __UNUSED__)
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
_clear_cache_cb(void *data __UNUSED__, void *data2 __UNUSED__)
{
   clear_cache = EINA_TRUE;
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o = NULL, *of = NULL, *ow = NULL;

   o = e_widget_list_add(evas, 0, 0);

   of = e_widget_framelist_add(evas, _("General"), 0);
   e_widget_framelist_content_align_set(of, 0.0, 0.0);

   /* ow = e_widget_check_add(evas, _("Show home directory"),
    * 			   &(cfdata->show_homedir));
    * e_widget_framelist_object_append(of, ow); */

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
_basic_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
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
}

static void
_conf_free(void)
{
   E_FREE(_conf);
}

static void
_conf_init(E_Module *m)
{
   char title[4096];

   snprintf(title, sizeof(title), "%s: %s", _("Everything Plugin"), _("Files"));

   e_configure_registry_item_add("launcher/everything-files", 110, title,
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

   if (_conf && !e_util_module_config_check
       (_("Everything Files"), _conf->version,
	MOD_CONFIG_FILE_EPOCH, MOD_CONFIG_FILE_VERSION))
	  _conf_free();

   if (!_conf) _conf_new();

   _conf->module = m;
}

static void
_conf_shutdown(void)
{
   e_configure_registry_item_del("launcher/everything-files");

   E_FREE(_conf);
   E_CONFIG_DD_FREE(conf_edd);
}

/***************************************************************************/

Eina_Bool
evry_plug_files_init(E_Module *m)
{
   _conf_init(m);

   EVRY_MODULE_NEW(evry_module, evry, _plugins_init, _plugins_shutdown);

   return EINA_TRUE;
}

void
evry_plug_files_shutdown(void)
{
   EVRY_MODULE_FREE(evry_module);

   _conf_shutdown();
}

void
evry_plug_files_save(void)
{
   e_config_domain_save("module.everything-files", conf_edd, _conf);
}
