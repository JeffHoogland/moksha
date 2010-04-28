/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */

#include "Evry.h"
#include "e_mod_main.h"
#include <Efreet_Trash.h>

#define MAX_ITEMS 50
#define MAX_SHOWN 300
#define TERM_ACTION_DIR "%s"

#define CMD_NONE        0
#define CMD_SHOW_ROOT   1
#define CMD_SHOW_HIDDEN 2
#define CMD_SHOW_PARENT 3

typedef struct _Plugin Plugin;
typedef struct _Data Data;
typedef struct _Module_Config Module_Config;

struct _Plugin
{
  Evry_Plugin base;

  Eina_List *files;
  const char *directory;
  const char *input;
  Eina_Bool command;
  Eina_Bool parent;
  Eina_List *hist_added;
  Eina_Bool show_hidden;

  Ecore_Thread *thread;
  Ecore_Thread *thread2;
  Ecore_File_Monitor *dir_mon;
  int cleanup;
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
  Eina_Bool second_run;
};

struct _Module_Config
{
  int version;

  unsigned char show_homedir;
  unsigned char show_recent;
  unsigned char search_recent;

  // TODO
  int sort_by;
  Eina_List *search_dirs;

  E_Config_Dialog *cfd;
  E_Module *module;
};

static Module_Config *_conf;

static Evry_Plugin *p1 = NULL;
static Evry_Plugin *p2 = NULL;
static Eina_List *_actions = NULL;

static void _cleanup(Evry_Plugin *plugin);

static const char *_mime_dir;
static const char *_type_file;


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

   file->mime = eina_stringshare_add("unknown");
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
   const char *mime;
   struct dirent *dp;
   Evry_Item_File *file;
   Eina_List *l;
   char buf[4096];
   int cnt = 0;

   if (!d->files)
     {
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

	     file = EVRY_ITEM_NEW(Evry_Item_File, p, NULL, NULL, _item_free);

	     filename = strdup(dp->d_name);
	     EVRY_ITEM(file)->label = filename;
	     d->files = eina_list_append(d->files, file);

#ifdef _DIRENT_HAVE_D_TYPE
	     if (dp->d_type & DT_UNKNOWN)
	       {
#endif
		  if (ecore_file_is_dir(file->path))
		    EVRY_ITEM(file)->browseable = EINA_TRUE;

#ifdef _DIRENT_HAVE_D_TYPE
	       }
	     else if (dp->d_type & DT_DIR)
	       {
		  EVRY_ITEM(file)->browseable = EINA_TRUE;
	       }
#endif
	  }
	closedir(d->dirp);

	d->files = eina_list_sort(d->files, -1, _cb_sort);
     }

   EINA_LIST_FOREACH(d->files, l, file)
     {
	snprintf(buf, sizeof(buf), "%s/%s",
		 d->directory,
		 EVRY_ITEM(file)->label);

	file->path = strdup(buf);

	if (!EVRY_ITEM(file)->browseable)
	  {
	     if (d->second_run)
	       usleep(2500);

	     if ((mime = efreet_mime_type_get(file->path)))
	       {
		  file->mime = mime;

		  if (!strncmp(file->mime, "inode/", 6) &&
		      ecore_file_is_dir(file->path))
		    EVRY_ITEM(file)->browseable = EINA_TRUE;
	       }
	  }
	if (cnt++ > MAX_ITEMS) break;
     }
}

static int
_append_files(Plugin *p)
{
   int match;
   int cnt = 0;
   Evry_Item_File *file;
   Eina_List *l;

   EVRY_PLUGIN_ITEMS_CLEAR(p);

   EINA_LIST_FOREACH(p->files, l, file)
     {
	if (cnt >= MAX_SHOWN) break;

	if (p->input && (match = evry_fuzzy_match(EVRY_ITEM(file)->label, p->input)))
	  {
	     EVRY_ITEM(file)->fuzzy_match = match;
	     EVRY_ITEM(file)->priority = cnt++;
	     EVRY_PLUGIN_ITEM_APPEND(p, file);

	  }
	else if (!p->input)
	  {
	     EVRY_ITEM(file)->priority = cnt++;
	     EVRY_PLUGIN_ITEM_APPEND(p, file);
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

   EINA_LIST_FREE(d->files, file)
     {
	free((char *)EVRY_ITEM(file)->label);
	free((char *)file->path);
	free(file);
     }

   free(d->directory);
   E_FREE(d);

   p->thread = NULL;

   if (p->cleanup > 0)
     {
	p->cleanup--;

	if (!p->cleanup)
	  E_FREE(p);
     }
}

static void
_scan_end_func(void *data)
{
   Data *d = data;
   Plugin *p = d->plugin;
   Evry_Item *item;
   Evry_Item_File *f;
   char *filename, *path, *mime;
   Eina_List *l;

   EINA_LIST_FREE(d->files, item)
     {
	GET_FILE(file, item);

	filename = (char *)item->label;
	path = (char *) file->path;
	mime = (char *) file->mime;
	if (!path) break;

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

	if (item->browseable)
	  file->mime = eina_stringshare_ref(_mime_dir);
	else if (mime)
	  file->mime = eina_stringshare_add(mime);
	else
	  file->mime = eina_stringshare_add("unknown");

	item->context = eina_stringshare_ref(file->mime);
	item->id = eina_stringshare_ref(file->path);
	item->label = eina_stringshare_add(filename);

	free(filename);
	free(path);

	p->files = eina_list_append(p->files, file);

	evry_util_file_detail_set(file);
     }

   if (d->files)
     {
	d->second_run = EINA_TRUE;
	p->thread = ecore_thread_run(_scan_func, _scan_end_func, _scan_cancel_func, d);
     }
   else
     {
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

   EVRY_PLUGIN_ITEMS_CLEAR(p);

   switch (event)
     {
      case ECORE_FILE_EVENT_DELETED_SELF:
	EINA_LIST_FREE(p->files, file)
	  evry_item_free(EVRY_ITEM(file));
	break;

      case ECORE_FILE_EVENT_CREATED_DIRECTORY:
      case ECORE_FILE_EVENT_CREATED_FILE:

	 label = ecore_file_file_get(path);

	 file = EVRY_ITEM_NEW(Evry_Item_File, p, label, NULL, _item_free);
	 file->path = eina_stringshare_add(path);

	 evry_util_file_detail_set(file);

	 if (event == ECORE_FILE_EVENT_CREATED_DIRECTORY)
	   {
	      file->mime = eina_stringshare_ref(_mime_dir);
	      EVRY_ITEM(file)->browseable = EINA_TRUE;
	   }
	 else
	   {
	      _item_fill(file);
	   }
	 p->files = eina_list_append(p->files, file);
	 break;

      case ECORE_FILE_EVENT_DELETED_FILE:
      case ECORE_FILE_EVENT_DELETED_DIRECTORY:
	 label = eina_stringshare_add(path);

	 EINA_LIST_FOREACH_SAFE(p->files, l, ll, file)
	   {
	      if (file->path != label) continue;

	      p->files = eina_list_remove_list(p->files, l);
	      p->hist_added = eina_list_remove(p->hist_added, file);
	      evry_item_free(EVRY_ITEM(file));
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
_begin(Evry_Plugin *plugin, const Evry_Item *it)
{
   Plugin *p = NULL;

   /* is FILE ? */
   if (it && evry_item_type_check(it, "FILE", NULL))
     {
	GET_FILE(file, it);

	if (!file->path || !ecore_file_is_dir(file->path))
	  return NULL;

	p = E_NEW(Plugin, 1);
	p->base = *plugin;
	p->base.items = NULL;

	p->directory = eina_stringshare_add(file->path);
	p->parent = EINA_TRUE;
     }
   else if (!it)
     {
	p = E_NEW(Plugin, 1);
	p->base = *plugin;
	p->base.items = NULL;
	if (_conf->show_homedir)
	  p->directory = eina_stringshare_add(e_user_homedir_get());
	p->parent = EINA_FALSE;
     }
   else
     {
	return NULL;
     }

   if (p->directory)
     _read_directory(p);

   return EVRY_PLUGIN(p);
}

static int
_hist_add(Evry_Plugin *plugin, Evry_Item_File *file)
{
   Eina_List *l;
   History_Item *hi;
   History_Entry *he;

   he = eina_hash_find(evry_hist->subjects, file->path);

   if (!he) return 0;

   EINA_LIST_FOREACH(he->items, l, hi)
     {
	if (hi->type != _type_file)
	  continue;

	if (hi->data)
	  eina_stringshare_del(hi->data);

	hi->data = eina_stringshare_ref(file->mime);
     }

   return 1;
}

static void
_cleanup(Evry_Plugin *plugin)
{
   GET_PLUGIN(p, plugin);

   Evry_Item_File *file;

   if (p->directory)
     eina_stringshare_del(p->directory);

   EINA_LIST_FREE(p->files, file)
     {
	_hist_add(plugin, file);
	evry_item_free(EVRY_ITEM(file));
     }

   EVRY_PLUGIN_ITEMS_CLEAR(p);

   if (p->input)
     eina_stringshare_del(p->input);

   if (p->dir_mon)
     ecore_file_monitor_del(p->dir_mon);

   if (p->thread2)
     {
	ecore_thread_cancel(p->thread2);
	p->cleanup++;
     }
   else
     EINA_LIST_FREE(p->hist_added, file)
       evry_item_free(EVRY_ITEM(file));

   if (p->thread)
     {
	ecore_thread_cancel(p->thread);
	p->cleanup++;
     }

   if (!p->cleanup)
     E_FREE(p);
}

/* use thread only to not block ui for ecore_file_exists ... */
static void
_hist_func(void *data)
{
   Plugin *p = data;
   Eina_List *l, *ll;
   Evry_Item_File *file;

   EINA_LIST_FOREACH_SAFE(p->hist_added, l, ll, file)
     {
	if (!ecore_file_exists(file->path))
	  p->hist_added = eina_list_remove_list(p->hist_added, l);

	if (!file->mime)
	  {
	     file->mime = eina_stringshare_add(efreet_mime_type_get(file->path));

	     if ((!strcmp(file->mime, "inode/directory")) ||
		 (!strcmp(file->mime, "inode/mount-point")))
	       EVRY_ITEM(file)->browseable = EINA_TRUE;
	  }
     }
}

static void
_hist_cancel_func(void *data)
{
   Plugin *p = data;
   Evry_Item_File *file;

   EINA_LIST_FREE(p->hist_added, file)
     {
	p->files = eina_list_remove(p->files, file);
	evry_item_free(EVRY_ITEM(file));
     }

   p->thread2 = NULL;

   if (p->cleanup > 0)
     {
	p->cleanup--;

	if (!p->cleanup)
	  E_FREE(p);
     }
}

static void
_hist_end_func(void *data)
{
   Plugin *p = data;
   Eina_List *l;
   Evry_Item_File *file;

   EINA_LIST_FOREACH(p->hist_added, l, file)
     {
	evry_item_ref(EVRY_ITEM(file));
	p->files = eina_list_append(p->files, file);
     }

   p->thread2 = NULL;

   _append_files(p);

   evry_plugin_async_update(EVRY_PLUGIN(p), EVRY_ASYNC_UPDATE_ADD);
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
	if (hi->type != _type_file)
	  continue;

	/* filter out files that we already have from history */
	EINA_LIST_FOREACH(p->files, ll, file)
	  if (!strcmp(file->path, key))
	    return EINA_TRUE;

	label = ecore_file_file_get(key);
	if (!label)
	  continue;

	file = EVRY_ITEM_NEW(Evry_Item_File, p, label, NULL, _item_free);

	file->path = eina_stringshare_add(key);
	if (hi->data)
	  file->mime = eina_stringshare_add(hi->data);
	if (hi->data)
	  EVRY_ITEM(file)->context = eina_stringshare_ref(file->mime);

	EVRY_ITEM(file)->id = eina_stringshare_ref(file->path);

	evry_util_file_detail_set(file);

	if (file->mime)
	  {
	     if ((!strcmp(file->mime, "inode/directory")) ||
		 (!strcmp(file->mime, "inode/mount-point")))
	       EVRY_ITEM(file)->browseable = EINA_TRUE;
	  }

	p->hist_added = eina_list_append(p->hist_added, file);
	break;
     }
   return EINA_TRUE;
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

   if (p->thread2)
     ecore_thread_cancel(p->thread2);
   p->thread2 = NULL;

   EINA_LIST_FREE(p->files, file)
     evry_item_free(EVRY_ITEM(file));

   EINA_LIST_FREE(p->hist_added, file)
     evry_item_free(EVRY_ITEM(file));

   if (p->dir_mon)
     ecore_file_monitor_del(p->dir_mon);
   p->dir_mon = NULL;
}

static int
_fetch(Evry_Plugin *plugin, const char *input)
{
   GET_PLUGIN(p, plugin);
   Evry_Item_File *file;

   if (!p->command)
     EVRY_PLUGIN_ITEMS_CLEAR(p);

   if (p->input)
     eina_stringshare_del(p->input);
   p->input = NULL;

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

	     printf("scan %s - %s\n", path, p->directory);

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
	p->input = eina_stringshare_add(input + 1);
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
	return 0;
     }

   /* add recent files */
   if ((!p->parent && !p->command && !p->hist_added) &&
       ((_conf->search_recent && input && strlen(input) > 1) ||
	(_conf->show_recent)))
     {
	eina_hash_foreach(evry_hist->subjects, _hist_items_add_cb, p);
	p->thread2 = ecore_thread_run(_hist_func, _hist_end_func, _hist_cancel_func, p);
     }
   else if (p->hist_added && !input && _conf->search_recent)
     {
	EINA_LIST_FREE(p->hist_added, file)
	  {
	     p->files = eina_list_remove(p->files, file);
	     evry_item_free(EVRY_ITEM(file));
	  }
     }

   if (input && !p->command)
     p->input = eina_stringshare_add(input);

   _append_files(p);

   if (!EVRY_PLUGIN(p)->items)
     return 0;

   if (!p->parent && _conf->show_recent)
     EVRY_PLUGIN_ITEMS_SORT(p, _cb_sort);

   return 1;
}

#define ACT_TRASH	1
#define ACT_DELETE	2
#define ACT_COPY	3
#define ACT_MOVE	4

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

   GET_FILE(file, act->item1);

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
   GET_FILE(file, act->item1);
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
   Efreet_Uri *uri;
   int ok = 0;
   char buf[PATH_MAX];
   int force = (EVRY_ITEM_DATA_INT_GET(act) == ACT_DELETE);

   GET_FILE(file, act->item1);

   if (!file->url)
     {
	/* efreet_uri_decode could be a litle less picky imo */
	snprintf(buf, sizeof(buf), "file://%s", file->path);
	uri = efreet_uri_decode(buf);

	printf("delete %s %p\n", buf, uri);
     }
   else
     {
	uri = efreet_uri_decode(file->url);
     }


   if (uri)
     {
	ok = efreet_trash_delete_uri(uri, force);
	efreet_uri_free(uri);

     }

   return ok;
}

static int
_file_copy_action(Evry_Action *act)
{
   GET_FILE(file, act->item1);
   GET_FILE(dst, act->item2);

   char *path;
   char buf[PATH_MAX];
   int ret;

   if (!ecore_file_is_dir(dst->path))
     path = ecore_file_dir_get(dst->path);
   else
     path = strdup(dst->path);

   if (!path) return 0;

   snprintf(buf, sizeof(buf), "%s/%s", path, act->item1->label);
   free(path);

   if (EVRY_ITEM_DATA_INT_GET(act) == ACT_COPY)
     {
	ret = ecore_file_cp(file->path, buf);
     }
   else if (EVRY_ITEM_DATA_INT_GET(act) == ACT_MOVE)
     {
	ret = ecore_file_mv(file->path, buf);
     }

   return ret;
}

/* static int
 * _complete(Evry_Plugin *p, const Evry_Item *item, char **input)
 * {
 *    return 0;
 * } */

static Eina_Bool
_plugins_init(void)
{
   Evry_Action *act;

   if (!evry_api_version_check(EVRY_API_VERSION))
     return EINA_FALSE;

   p1 = EVRY_PLUGIN_NEW(Evry_Plugin, N_("Files"), NULL, "FILE", _begin, _cleanup, _fetch, NULL);
   p1->config_path = "extensions/everything-files";
   evry_plugin_register(p1, EVRY_PLUGIN_SUBJECT, 3);
   /* p1->complete = &_complete; */

   p2 = EVRY_PLUGIN_NEW(Evry_Plugin, N_("Files"), NULL, "FILE", _begin, _cleanup, _fetch, NULL);
   p2->config_path = "extensions/everything-files";
   evry_plugin_register(p2, EVRY_PLUGIN_OBJECT, 1);

   act = EVRY_ACTION_NEW(N_("Open Folder (EFM)"), "FILE", NULL, "folder-open",
			  _open_folder_action, _open_folder_check);
   evry_action_register(act, 0);
   _actions = eina_list_append(_actions, act);

   act = EVRY_ACTION_NEW(N_("Open Terminal here"), "FILE", NULL, "system-run",
			  _open_term_action, NULL);
   evry_action_register(act, 2);
   _actions = eina_list_append(_actions, act);

   act = EVRY_ACTION_NEW(N_("Move to Trash"), "FILE", NULL, "edit-delete",
			  _file_trash_action, NULL);
   EVRY_ITEM_DATA_INT_SET(act, ACT_TRASH);
   evry_action_register(act, 2);
   _actions = eina_list_append(_actions, act);

   act = EVRY_ACTION_NEW(N_("Delete"), "FILE", NULL, "list-remove",
			 _file_trash_action, NULL);
   EVRY_ITEM_DATA_INT_SET(act, ACT_DELETE);
   evry_action_register(act, 2);

   _actions = eina_list_append(_actions, act);

   act = EVRY_ACTION_NEW(N_("Copy To ..."), "FILE", "FILE", "go-next",
			 _file_copy_action, NULL);
   EVRY_ITEM_DATA_INT_SET(act, ACT_COPY);
   evry_action_register(act, 2);
   _actions = eina_list_append(_actions, act);

   act = EVRY_ACTION_NEW(N_("Move To ..."), "FILE", "FILE", "go-next",
			 _file_copy_action, NULL);
   EVRY_ITEM_DATA_INT_SET(act, ACT_MOVE);
   evry_action_register(act, 2);
   _actions = eina_list_append(_actions, act);

   return EINA_TRUE;
}

static void
_plugins_shutdown(void)
{
   Evry_Action *act;

   EVRY_PLUGIN_FREE(p1);
   EVRY_PLUGIN_FREE(p2);

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

   if (e_config_dialog_find("everything-files", "extensions/everything-files")) return NULL;

   v = E_NEW(E_Config_Dialog_View, 1);
   if (!v) return NULL;

   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;
   v->basic.apply_cfdata = _basic_apply;

   snprintf(buf, sizeof(buf), "%s/e-module.edj", _conf->module->dir);

   cfd = e_config_dialog_new(con, _("Everything Files"), "everything-files",
			     "extensions/everything-files", buf, 0, v, NULL);

   _conf->cfd = cfd;
   return cfd;
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o = NULL, *of = NULL, *ow = NULL;

   o = e_widget_list_add(evas, 0, 0);

   of = e_widget_framelist_add(evas, _("General"), 0);
   e_widget_framelist_content_align_set(of, 0.0, 0.0);

   ow = e_widget_check_add(evas, _("Show Home Directory"),
			   &(cfdata->show_homedir));
   e_widget_framelist_object_append(of, ow);

   ow = e_widget_check_add(evas, _("Show Recent Files"),
			   &(cfdata->show_recent));
   e_widget_framelist_object_append(of, ow);

   ow = e_widget_check_add(evas, _("Search Recent Files"),
			   &(cfdata->search_recent));
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
#undef C
}

static int
_basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
#define C(_name) _conf->_name = cfdata->_name;
   C(show_homedir);
   C(show_recent);
   C(search_recent);
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
   char buf[4096];

   snprintf(buf, sizeof(buf), "%s/e-module.edj", m->dir);

   e_configure_registry_category_add("extensions", 80, _("Extensions"),
				     NULL, "preferences-extensions");

   e_configure_registry_item_add("extensions/everything-files", 110, _("Everything Files"),
				 NULL, buf, _conf_dialog);

   conf_edd = E_CONFIG_DD_NEW("Module_Config", Module_Config);

#undef T
#undef D
#define T Module_Config
#define D conf_edd
   E_CONFIG_VAL(D, T, version, INT);
   E_CONFIG_VAL(D, T, show_homedir, UCHAR);
   E_CONFIG_VAL(D, T, show_recent, UCHAR);
   E_CONFIG_VAL(D, T, search_recent, UCHAR);
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
   if (e_datastore_get("everything_loaded"))
     active = _plugins_init();

   _conf_init(m);

   _mime_dir = eina_stringshare_add("inode/directory");
   _type_file = eina_stringshare_add("FILE");

   e_module_delayed_set(m, 1);

   return m;
}

EAPI int
e_modapi_shutdown(E_Module *m)
{
   if (active && e_datastore_get("everything_loaded"))
     _plugins_shutdown();

   eina_stringshare_del(_mime_dir);
   eina_stringshare_del(_type_file);

   _conf_shutdown();

   return 1;
}

EAPI int
e_modapi_save(E_Module *m)
{
   e_config_domain_save("module.everything-files", conf_edd, _conf);
   return 1;
}
