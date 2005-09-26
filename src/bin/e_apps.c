/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* TODO List:
 * 
 * - We assume only .eap files in 'all', no subdirs
 * - If a .order file references a non-existing file, and the file
 *   is added in 'all', it doesn't show!
 * - track app execution state, visibility state etc. and call callbacks
 * - calls to execute an app or query its runing/starting state etc.
 * - clean up the add app functions. To much similar code.
 */

/* local subsystem functions */
typedef struct _E_App_Change_Info E_App_Change_Info;
typedef struct _E_App_Callback    E_App_Callback;
typedef struct _E_App_Scan_Cache  E_App_Scan_Cache;

struct _E_App_Change_Info
{
   E_App        *app;
   E_App_Change  change;
};

struct _E_App_Callback
{
   void (*func) (void *data, E_App *a, E_App_Change ch);
   void  *data;
   unsigned char delete_me : 1;
};

struct _E_App_Scan_Cache
{
   char          *path;
   E_App_Cache   *cache;
   E_App         *app;
   Ecore_List    *files;
   Ecore_Timer   *timer;
   unsigned char  need_rewrite : 1;
};

static void      _e_app_free               (E_App *a);
static E_App     *_e_app_subapp_file_find  (E_App *a, const char *file);
static void      _e_app_change             (E_App *a, E_App_Change ch);
static int       _e_apps_cb_exit           (void *data, int type, void *event);
static void      _e_app_cb_monitor         (void *data, Ecore_File_Monitor *em, Ecore_File_Event event, const char *path);
static void      _e_app_subdir_rescan      (E_App *app);
static int       _e_app_is_eapp            (const char *path);
static int       _e_app_copy               (E_App *dst, E_App *src);
static void      _e_app_save_order         (E_App *app);
static int       _e_app_cb_event_border_add(void *data, int type, void *event);
static int       _e_app_cb_expire_timer    (void *data);
static E_App    *_e_app_ecore_exe_find     (Ecore_Exe *exe);

/* local subsystem globals */
static Evas_Hash   *_e_apps = NULL;
static Evas_List   *_e_apps_list = NULL;
static int          _e_apps_callbacks_walking = 0;
static int          _e_apps_callbacks_delete_me = 0;
static Evas_List   *_e_apps_change_callbacks = NULL;
static Ecore_Event_Handler *_e_apps_exit_handler = NULL;
static Ecore_Event_Handler *_e_apps_border_add_handler = NULL;
static Evas_List   *_e_apps_repositories = NULL;
static E_App       *_e_apps_all = NULL;
static char        *_e_apps_path_all = NULL;
static char        *_e_apps_path_trash = NULL;
static Evas_List   *_e_apps_start_pending = NULL;

/* externally accessible functions */
int
e_app_init(void)
{
   char *home;
   char buf[PATH_MAX];
   
   e_app_cache_init();
   home = e_user_homedir_get();
   snprintf(buf, sizeof(buf), "%s/.e/e/applications/trash", home);
   _e_apps_path_trash = strdup(buf);
   snprintf(buf, sizeof(buf), "%s/.e/e/applications/all", home);
   _e_apps_path_all = strdup(buf);
   free(home);
   _e_apps_repositories = evas_list_append(_e_apps_repositories, strdup(buf));
   _e_apps_exit_handler = ecore_event_handler_add(ECORE_EVENT_EXE_EXIT, _e_apps_cb_exit, NULL);
   _e_apps_border_add_handler = ecore_event_handler_add(E_EVENT_BORDER_ADD, _e_app_cb_event_border_add, NULL);
   _e_apps_all = e_app_new(buf, 1);
   return 1;
}

int
e_app_shutdown(void)
{
   _e_apps_start_pending = evas_list_free(_e_apps_start_pending);
   if (_e_apps_all)
     {
	e_object_unref(E_OBJECT(_e_apps_all));
	_e_apps_all = NULL;
     }
   while (_e_apps_repositories)
     {
	free(_e_apps_repositories->data);
	_e_apps_repositories = evas_list_remove_list(_e_apps_repositories, _e_apps_repositories);
     }
   if (_e_apps_exit_handler)
     {
	ecore_event_handler_del(_e_apps_exit_handler);
	_e_apps_exit_handler = NULL;
     }
   if (_e_apps_border_add_handler)
     {
	ecore_event_handler_del(_e_apps_border_add_handler);
	_e_apps_border_add_handler = NULL;
     }
   free(_e_apps_path_trash);
   free(_e_apps_path_all);
     {
	Evas_List *l;
	for (l = _e_apps_list; l; l = l->next)
	  {
	     E_App *a;
	     a = l->data;
	     printf("BUG: References %d %s\n", E_OBJECT(a)->references, a->path);
	  }
     }
   e_app_cache_shutdown();
   return 1;
}

E_App *
e_app_raw_new(void)
{
   E_App *a;
   
   a = E_OBJECT_ALLOC(E_App, E_APP_TYPE, _e_app_free);
   return a;
}

static void
_e_app_cache_copy(E_App_Cache *ac, E_App *a)
{
#define IF_DUP(x) if ((ac->x) && (strlen(ac->x) > 0)) a->x = strdup(ac->x)
   IF_DUP(name);
   IF_DUP(generic);
   IF_DUP(comment);
   IF_DUP(exe);
   IF_DUP(win_name);
   IF_DUP(win_class);
   IF_DUP(win_title);
   IF_DUP(win_role);
   IF_DUP(icon_class);
   a->startup_notify = ac->startup_notify;
   a->wait_exit = ac->wait_exit;
}

Evas_Bool 
_e_app_cb_scan_hash_foreach(Evas_Hash *hash, const char *key, void *data, void *fdata)
{
   E_App_Scan_Cache *sc;
   E_App_Cache *ac;
   char *s;
   
   sc = fdata;
   s = (char *)key;
   ac = data;
   /* file "s" has been deleted */
   printf("Cache %s - DELETED\n", s);
   sc->need_rewrite = 1;
   return 1;
}

static int
_e_app_cb_scan_cache_timer(void *data)
{
   E_App_Scan_Cache *sc;
   char *s;
   char buf[4096];
   E_App_Cache *ac;
   int is_dir = 0;

   sc = data;
   s = ecore_list_next(sc->files);
   if (!s)
     {
	evas_hash_foreach(sc->cache->subapps_hash, _e_app_cb_scan_hash_foreach, sc);
	if (sc->need_rewrite)
	  _e_app_subdir_rescan(sc->app);
	sc->app->monitor = ecore_file_monitor_add(sc->app->path, _e_app_cb_monitor, sc->app);
	e_object_unref(E_OBJECT(sc->app));
	ecore_list_destroy(sc->files);
	e_app_cache_free(sc->cache);
	ecore_timer_del(sc->timer);
	free(sc->path);
	free(sc);
	printf("Cache scan finish.\n");
	return 0;
     }
   snprintf(buf, sizeof(buf), "%s/%s", sc->path, s);
   is_dir = ecore_file_is_dir(buf);
   if (e_util_glob_match(s, "*.eap") || is_dir)
     {
	ac = evas_hash_find(sc->cache->subapps_hash, s);
	if (ac)
	  {
	     if (is_dir != ac->is_dir)
	       {
		  printf("Cache %s - CHANGED TYPE\n", s);
		  sc->need_rewrite =1 ;
	       }
	     else if (!is_dir)
	       {
		  unsigned long long mtime;
		  
		  mtime = ecore_file_mod_time(buf);
		  if (mtime != ac->file_mod_time)
		    {
		       /* file "s" has changed */
		       printf("Cache %s - MODIFIED\n", s);
		       sc->need_rewrite = 1;
		    }
	       }
	     sc->cache->subapps_hash = evas_hash_del(sc->cache->subapps_hash, s, ac);
	  }
	else
	  {
	     /* file "s" has been added */
	     printf("Cache %s - MODIFIED\n", s);
	     sc->need_rewrite = 1;
	  }
     }
   return 1;
}

static E_App *
_e_app_cache_new(E_App_Cache *ac, char *path, int scan_subdirs)
{
   Evas_List *l;
   E_App *a;
   char buf[PATH_MAX];
   E_App_Scan_Cache *sc;
   
   a = E_OBJECT_ALLOC(E_App, E_APP_TYPE, _e_app_free);
   _e_app_cache_copy(ac, a);
   a->path = strdup(path);
   a->scanned = 1;
   for (l = ac->subapps; l; l = l->next)
     {
	E_App_Cache *ac2;
	E_App *a2;
	
	ac2 = l->data;
	snprintf(buf, sizeof(buf), "%s/%s", path, ac2->file);
	if ((ac2->is_dir) && (scan_subdirs))
	  {
	     a2 = e_app_new(buf, scan_subdirs);
	     a2->parent = a;
	     a->subapps = evas_list_append(a->subapps, a2);
	  }
	else
	  {
	     if (!ac2->is_link)
	       {
		  a2 = E_OBJECT_ALLOC(E_App, E_APP_TYPE, _e_app_free);
		  _e_app_cache_copy(ac2, a2);
		  if (ac2->is_dir)
		    {
		       E_FREE(a2->exe);
		    }
		  a2->parent = a;
		  a2->path = strdup(buf);
		  a->subapps = evas_list_append(a->subapps, a2);
		  _e_apps = evas_hash_add(_e_apps, a2->path, a2);
		  _e_apps_list = evas_list_prepend(_e_apps_list, a2);
	       }
	     else
	       {
		  E_App *a3;
		  Evas_List *pl;

		  pl = _e_apps_repositories;
		  a2 = NULL;
		  while ((!a2) && (pl))
		    {
		       snprintf(buf, sizeof(buf), "%s/%s", (char *)pl->data, ac2->file);
		       a2 = e_app_new(buf, scan_subdirs);
		       pl = pl->next;
		    }
		  if (a2)
		    {
		       a3 = E_OBJECT_ALLOC(E_App, E_APP_TYPE, _e_app_free);
		       if (a3)
			 {
			    if (_e_app_copy(a3, a2))
			      {
				 a3->parent = a;
				 a->subapps = evas_list_append(a->subapps, a3);
				 a2->references = evas_list_append(a2->references, a3);
				 _e_apps_list = evas_list_prepend(_e_apps_list, a3);
			      }
			    else
			      e_object_del(E_OBJECT(a3));
			 }
		    }
	       }
	  }
     }

   sc = calloc(1, sizeof(E_App_Scan_Cache));
   if (sc)
     {
	sc->path = strdup(path);
	sc->cache = ac;
	sc->app = a;
	sc->files = e_app_dir_file_list_get(a);
	sc->timer = ecore_timer_add(0.001, _e_app_cb_scan_cache_timer, sc);
	e_object_ref(E_OBJECT(sc->app));
     }
   else
     e_app_cache_free(ac);
   return a;
}

E_App *
e_app_new(const char *path, int scan_subdirs)
{
   E_App *a;
   char buf[PATH_MAX];
   E_App_Cache *ac;
   
   a = evas_hash_find(_e_apps, path);
   if (a)
     {
	if (a->deleted)
	  return NULL;
	e_object_ref(E_OBJECT(a));
	return a;
     }

   ac = e_app_cache_load((char *)path);
   if (ac)
     {
	a = _e_app_cache_new(ac, (char *)path, scan_subdirs);
	if (a)
	  {
	     _e_apps = evas_hash_add(_e_apps, a->path, a);
	     _e_apps_list = evas_list_prepend(_e_apps_list, a);
	  }
//	e_app_cache_free(ac);
     }
   else
     {
	if (ecore_file_exists(path))
	  {
	     a = E_OBJECT_ALLOC(E_App, E_APP_TYPE, _e_app_free);
	     /* record the path */
	     a->path = strdup(path);
	     
	     if (ecore_file_is_dir(a->path))
	       {
		  snprintf(buf, sizeof(buf), "%s/.directory.eap", path);
		  if (ecore_file_exists(buf))
		    e_app_fields_fill(a, buf);
		  else
		    a->name = strdup(ecore_file_get_file(a->path));
		  if (scan_subdirs) e_app_subdir_scan(a, scan_subdirs);
		  
		  a->monitor = ecore_file_monitor_add(a->path, _e_app_cb_monitor, a);
	       }
	     else if (_e_app_is_eapp(path))
	       {
		  e_app_fields_fill(a, path);
		  
		  /* no exe field.. not valid. drop it */
		  if (!a->exe)
		    goto error;
	       }
	     else
	       goto error;
	  }
	else
	  return NULL;
	_e_apps = evas_hash_add(_e_apps, a->path, a);
	_e_apps_list = evas_list_prepend(_e_apps_list, a);
	
	ac = e_app_cache_generate(a);
	e_app_cache_save(ac, a->path);
	e_app_cache_free(ac);
     }
   return a;

error:
   if (a->monitor) ecore_file_monitor_del(a->monitor);
   if (a->path) free(a->path);
   e_app_fields_empty(a);
   free(a);
   return NULL;
}

int
e_app_is_parent(E_App *parent, E_App *app)
{
   E_App *current;

   current = app->parent;

   while (current)
     {
	if (current == parent)
	  return 1;
	current = current->parent;
     }

   return 0;
}

int
e_app_equals(E_App *app1, E_App *app2)
{
   return ((app1 == app2) || (app1->orig == app2) ||
	   (app1 == app2->orig) || (app1->orig == app2->orig));
}

void
e_app_subdir_scan(E_App *a, int scan_subdirs)
{
   Ecore_List *files;
   char *s;
   char buf[PATH_MAX];
   E_App_Cache *ac;
   
   E_OBJECT_CHECK(a);
   E_OBJECT_TYPE_CHECK(a, E_APP_TYPE);
   if (a->exe) return;
   if (a->scanned)
     {
	Evas_List *l;
	
	if (!scan_subdirs) return;
	for (l = a->subapps; l; l = l->next)
	  e_app_subdir_scan(l->data, scan_subdirs);
	return;
     }
   a->scanned = 1;
   files = e_app_dir_file_list_get(a);
   if (files)
     {
	while ((s = ecore_list_next(files)))
	  {
	     E_App *a2;

	     a2 = NULL;

	     snprintf(buf, sizeof(buf), "%s/%s", a->path, s);
	     if (ecore_file_exists(buf))
	       {
		  a2 = e_app_new(buf, scan_subdirs);
		  if (a2)
		    {
		       a2->parent = a;
		       a->subapps = evas_list_append(a->subapps, a2);
		    }
	       }
	     else
	       {
		  E_App *a3;
		  Evas_List *pl;

		  pl = _e_apps_repositories;
		  while ((!a2) && (pl))
		    {
		       snprintf(buf, sizeof(buf), "%s/%s", (char *)pl->data, s);
		       a2 = e_app_new(buf, scan_subdirs);
		       pl = pl->next;
		    }
		  if (a2)
		    {
		       a3 = E_OBJECT_ALLOC(E_App, E_APP_TYPE, _e_app_free);
		       if (a3)
			 {
			    if (_e_app_copy(a3, a2))
			      {
				 a3->parent = a;
				 a->subapps = evas_list_append(a->subapps, a3);
				 a2->references = evas_list_append(a2->references, a3);
				 _e_apps_list = evas_list_prepend(_e_apps_list, a3);
			      }
			    else
			      e_object_del(E_OBJECT(a3));
			 }
		    }
	       }
	     free(s);
	  }
	ecore_list_destroy(files);
     }

   ac = e_app_cache_generate(a);
   e_app_cache_save(ac, a->path);
   e_app_cache_free(ac);
}

int
e_app_exec(E_App *a, int launch_id)
{
   Ecore_Exe *exe;
   E_App_Instance *inst;
   
   E_OBJECT_CHECK_RETURN(a, 0);
   E_OBJECT_TYPE_CHECK_RETURN(a, E_APP_TYPE, 0);
   if (!a->exe) return 0;
   /* FIXME: set up locale, encoding and input method env vars if they are in
    * the eapp file */
   inst = E_NEW(E_App_Instance, 1);
   if (!inst) return 0;
   exe = ecore_exe_run(a->exe, inst);
   if (!exe)
     {
	free(inst);
	e_error_dialog_show(_("Run Error"),
			    _("Enlightenment was unable fork a child process:\n"
			      "\n"
			      "%s\n"
			      "\n"),
			    a->exe);
	return 0;
     }
   inst->app = a;
   inst->exe = exe;
   inst->launch_id = launch_id;
   inst->launch_time = ecore_time_get();
   inst->expire_timer = ecore_timer_add(10.0, _e_app_cb_expire_timer, inst);
   a->instances = evas_list_append(a->instances, inst);
//   e_object_ref(E_OBJECT(a));
   _e_apps_start_pending = evas_list_append(_e_apps_start_pending, a);
   if (a->startup_notify) a->starting = 1;
   _e_app_change(a, E_APP_EXEC);
   return 1;
}

int
e_app_starting_get(E_App *a)
{
   E_OBJECT_CHECK_RETURN(a, 0);
   E_OBJECT_TYPE_CHECK_RETURN(a, E_APP_TYPE, 0);
   return a->starting;
}

int
e_app_running_get(E_App *a)
{
   E_OBJECT_CHECK_RETURN(a, 0);
   E_OBJECT_TYPE_CHECK_RETURN(a, E_APP_TYPE, 0);
   if (a->instances) return 1;
   return 0;
}

void
e_app_prepend_relative(E_App *add, E_App *before)
{
   char buf[PATH_MAX];

   if (!before->parent) return;

   before->parent->subapps = evas_list_prepend_relative(before->parent->subapps,
							add, before);
   add->parent = before->parent;

   /* Check if this app is in the trash */
   if (!strncmp(add->path, _e_apps_path_trash, strlen(_e_apps_path_trash)))
     {
	/* Move to all */
	snprintf(buf, sizeof(buf), "%s/%s", _e_apps_path_all, ecore_file_get_file(add->path));
	if (ecore_file_exists(buf))
	  snprintf(buf, sizeof(buf), "%s/%s", before->parent->path, ecore_file_get_file(add->path));
	ecore_file_mv(add->path, buf);
	free(add->path);
	add->path = strdup(buf);
     }

   _e_app_save_order(before->parent);
   _e_app_change(add, E_APP_ADD);
   _e_app_change(before->parent, E_APP_ORDER);
}

void
e_app_append(E_App *add, E_App *parent)
{
   char buf[PATH_MAX];

   parent->subapps = evas_list_append(parent->subapps, add);
   add->parent = parent;

   /* Check if this app is in the trash */
   if (!strncmp(add->path, _e_apps_path_trash, strlen(_e_apps_path_trash)))
     {
	/* Move to all */
	snprintf(buf, sizeof(buf), "%s/%s", _e_apps_path_all, ecore_file_get_file(add->path));
	if (ecore_file_exists(buf))
	  snprintf(buf, sizeof(buf), "%s/%s", parent->path, ecore_file_get_file(add->path));
	ecore_file_mv(add->path, buf);
	free(add->path);
	add->path = strdup(buf);
     }

   _e_app_save_order(parent);
   _e_app_change(add, E_APP_ADD);
}

void
e_app_files_prepend_relative(Evas_List *files, E_App *before)
{
   Evas_List *l;

   if (!before->parent) return;

   for (l = files; l; l = l->next)
     {
	char *file;
	char buf[PATH_MAX];

	file = l->data;
	if (!_e_app_is_eapp(file)) continue;
	/* FIXME: If we are downloading something from net, we must
	 * attach a callback and wait for the download to finish */
	snprintf(buf, sizeof(buf), "%s/%s", _e_apps_path_all,
				   ecore_file_get_file(file));
	if (!ecore_file_download(file, buf, NULL, NULL)) continue;
     }
   /* Force rescan of all subdir */
   _e_app_subdir_rescan(_e_apps_all);
   /* Change .order file */
   if (before->parent != _e_apps_all)
     {
	FILE *f;
	char buf[PATH_MAX];

	snprintf(buf, sizeof(buf), "%s/.order", before->parent->path);
	f = fopen(buf, "wb");
	if (!f) return;

	for (l = before->parent->subapps; l; l = l->next)
	  {
	     E_App *a;
	     Evas_List *l2;

	     a = l->data;
	     if (a == before)
	       {
		  /* Add the new files */
		  for (l2 = files; l2; l2 = l2->next)
		    {
		       char *file;

		       file = l2->data;
		       fprintf(f, "%s\n", ecore_file_get_file(file));
		    }
	       }
	     fprintf(f, "%s\n", ecore_file_get_file(a->path));
	  }
	fclose(f);
     }
}

void
e_app_files_append(Evas_List *files, E_App *parent)
{
   Evas_List *l, *subapps;

   subapps = parent->subapps;

   for (l = files; l; l = l->next)
     {
	char *file;
	char buf[PATH_MAX];

	file = l->data;
	if (!_e_app_is_eapp(file)) continue;
	/* FIXME: If we are downloading something from net, we must
	 * attach a callback and wait for the download to finish */
	snprintf(buf, sizeof(buf), "%s/%s", _e_apps_path_all,
				   ecore_file_get_file(file));
	if (!ecore_file_download(file, buf, NULL, NULL)) continue;
     }
   /* Force rescan of all subdir */
   _e_app_subdir_rescan(_e_apps_all);
   /* Change .order file */
   if (parent != _e_apps_all)
     {
	FILE *f;
	char buf[PATH_MAX];

	snprintf(buf, sizeof(buf), "%s/.order", parent->path);
	f = fopen(buf, "wb");
	if (!f) return;

	for (l = parent->subapps; l; l = l->next)
	  {
	     E_App *a;

	     a = l->data;
	     fprintf(f, "%s\n", ecore_file_get_file(a->path));
	  }
	/* Add the new files */
	for (l = files; l; l = l->next)
	  {
	     char *file;

	     file = l->data;
	     fprintf(f, "%s\n", ecore_file_get_file(file));
	  }
	fclose(f);
     }
}

void
e_app_remove(E_App *remove)
{
   char buf[PATH_MAX];

   if (!remove->parent) return;

   remove->parent->subapps = evas_list_remove(remove->parent->subapps, remove);

   /* Check if this app is in a repository or in the parents dir */
   snprintf(buf, sizeof(buf), "%s/%s", remove->parent->path, ecore_file_get_file(remove->path));
   if (ecore_file_exists(buf))
     {
	/* Move to trash */
	snprintf(buf, sizeof(buf), "%s/%s", _e_apps_path_trash, ecore_file_get_file(remove->path));
	ecore_file_mv(remove->path, buf);
	free(remove->path);
	remove->path = strdup(buf);
     }
   _e_app_save_order(remove->parent);
   _e_app_change(remove, E_APP_DEL);
   remove->parent = NULL;
}

void
e_app_change_callback_add(void (*func) (void *data, E_App *a, E_App_Change ch), void *data)
{
   E_App_Callback *cb;
   
   cb = calloc(1, sizeof(E_App_Callback));
   cb->func = func;
   cb->data = data;
   _e_apps_change_callbacks = evas_list_append(_e_apps_change_callbacks, cb);
}

void
e_app_change_callback_del(void (*func) (void *data, E_App *a, E_App_Change ch), void *data)
{
   Evas_List *l;
   
   for (l = _e_apps_change_callbacks; l; l = l->next)
     {
	E_App_Callback *cb;
	
	cb = l->data;
	if ((cb->func == func) && (cb->data == data))
	  {
	     if (_e_apps_callbacks_walking)
	       {
		  cb->delete_me = 1;
		  _e_apps_callbacks_delete_me = 1;
	       }
	     else
	       {
		  _e_apps_change_callbacks = evas_list_remove_list(_e_apps_change_callbacks, l);
		  free(cb);
	       }
	     return;
	  }
     }
}

E_App *
e_app_launch_id_pid_find(int launch_id, pid_t pid)
{
   Evas_List *l, *ll;
   
   for (l = _e_apps_list; l; l = l->next)
     {
	E_App *a;
	
	a = l->data;
	for (ll = a->instances; ll; ll = ll->next)
	  {
	     E_App_Instance *ai;
	     
	     ai = ll->data;
	     if (((launch_id > 0) && (ai->launch_id > 0) && (ai->launch_id == launch_id)) ||
		 ((pid > 1) && (ai->exe) && (ecore_exe_pid_get(ai->exe) == pid)))
	       {
		  _e_apps_list = evas_list_remove_list(_e_apps_list, l);
		  _e_apps_list = evas_list_prepend(_e_apps_list, a);
		  return a;
	       }
	  }
     }
   return NULL;
}

E_App *
e_app_window_name_class_title_role_find(const char *name, const char *class,
					const char *title, const char *role)
{
   Evas_List *l;
   
   if ((!name) && (!class) && (!title) && (!role))
     return NULL;

   for (l = _e_apps_list; l; l = l->next)
     {
	E_App *a;
	int ok;
	
	a = l->data;
	ok = 0;
	if ((a->win_name) || (a->win_class) || (a->win_title) || (a->win_role))
	  {
	     if ((!a->win_name) ||
		 ((a->win_name) && (name) && (e_util_glob_match(name, a->win_name))))
	       ok++;
	     if ((!a->win_class) ||
		 ((a->win_class) && (class) && (e_util_glob_match(class, a->win_class))))
	       ok++;
	     if ((!a->win_title) ||
		 ((a->win_title) && (title) && (e_util_glob_match(title, a->win_title))))
	       ok++;
	     if ((!a->win_role) ||
		 ((a->win_role) && (role) && (e_util_glob_match(role, a->win_role))))
	       ok++;
	  }
	if (ok >= 4)
	  {
	     _e_apps_list = evas_list_remove_list(_e_apps_list, l);
	     _e_apps_list = evas_list_prepend(_e_apps_list, a);
	     return a;
	  }
     }
   return NULL;
}

E_App *
e_app_file_find(char *file)
{
   Evas_List *l;
   
   if (!file) return NULL;

   for (l = _e_apps_list; l; l = l->next)
     {
	E_App *a;
	
	a = l->data;
	if (a->path)
	  {
	     char *p;
	     
	     p = strchr(a->path, '/');
	     if (p)
	       {
		  p++;
		  if (!strcmp(p, file))
		    {
		       _e_apps_list = evas_list_remove_list(_e_apps_list, l);
		       _e_apps_list = evas_list_prepend(_e_apps_list, a);
		       return a;
		    }
	       }
	  }
     }
   return NULL;
}

E_App *
e_app_name_find(char *name)
{
   Evas_List *l;
   
   if (!name) return NULL;

   for (l = _e_apps_list; l; l = l->next)
     {
	E_App *a;
	
	a = l->data;
	if (a->name)
	  {
	     if (!strcasecmp(a->name, name))
	       {
		  _e_apps_list = evas_list_remove_list(_e_apps_list, l);
		  _e_apps_list = evas_list_prepend(_e_apps_list, a);
		  return a;
	       }
	  }
     }
   return NULL;
}

E_App *
e_app_generic_find(char *generic)
{
   Evas_List *l;
   
   if (!generic) return NULL;

   for (l = _e_apps_list; l; l = l->next)
     {
	E_App *a;
	
	a = l->data;
	if (a->generic)
	  {
	     if (!strcasecmp(a->generic, generic))
	       {
		  _e_apps_list = evas_list_remove_list(_e_apps_list, l);
		  _e_apps_list = evas_list_prepend(_e_apps_list, a);
		  return a;
	       }
	  }
     }
   return NULL;
}

E_App *
e_app_exe_find(char *exe)
{
   Evas_List *l;
   
   if (!exe) return NULL;

   for (l = _e_apps_list; l; l = l->next)
     {
	E_App *a;
	
	a = l->data;
	if (a->exe)
	  {
	     if (!strcmp(a->exe, exe))
	       {
		  _e_apps_list = evas_list_remove_list(_e_apps_list, l);
		  _e_apps_list = evas_list_prepend(_e_apps_list, a);
		  return a;
	       }
	  }
     }
   return NULL;
}


/* local subsystem functions */
static void
_e_app_free(E_App *a)
{
   while (evas_list_find(_e_apps_start_pending, a))
     _e_apps_start_pending = evas_list_remove(_e_apps_start_pending, a);
   if (a->orig)
     {
	while (a->instances)
	  {
	     E_App_Instance *inst;

	     inst = a->instances->data;
	     inst->app = a->orig;
	     a->orig->instances = evas_list_append(a->orig->instances, inst);
	     a->instances = evas_list_remove_list(a->instances, a->instances);
	     _e_apps_start_pending = evas_list_append(_e_apps_start_pending, a->orig);
	  }
	/* If this is a copy, it shouldn't have any references! */
	if (a->references)
	  printf("BUG: A eapp copy shouldn't have any references!\n");
	if (a->parent)
	  a->parent->subapps = evas_list_remove(a->parent->subapps, a);
	a->orig->references = evas_list_remove(a->orig->references, a);
	_e_apps_list = evas_list_remove(_e_apps_list, a);
	e_object_unref(E_OBJECT(a->orig));
	free(a);
     }
   else
     {
	while (a->instances)
	  {
	     E_App_Instance *inst;

	     inst = a->instances->data;
	     if (inst->expire_timer)
	       {
		  ecore_timer_del(inst->expire_timer);
		  inst->expire_timer = NULL;
	       }
	     if (inst->exe)
	       {
		  ecore_exe_free(inst->exe);
		  inst->exe = NULL;
	       }
	     free(inst);
	     a->instances = evas_list_remove_list(a->instances, a->instances);
	  }
	while (a->subapps)
	  {
	     E_App *a2;

	     a2 = a->subapps->data;
	     a->subapps = evas_list_remove_list(a->subapps, a->subapps);
	     /* remove us as the parent */
	     a2->parent = NULL;
	     /* unref the child so it will be deleted too */
	     e_object_unref(E_OBJECT(a2));
	  }
	/* If this is an original, it wont be deleted until all references
	 * are gone */
	if (a->references)
	  {
	     printf("BUG: An original eapp shouldn't have any references when freed! %d\n", evas_list_count(a->references));
	  }

	if (a->parent)
	  a->parent->subapps = evas_list_remove(a->parent->subapps, a);
	if (a->monitor)
	  ecore_file_monitor_del(a->monitor);
	_e_apps = evas_hash_del(_e_apps, a->path, a);
	_e_apps_list = evas_list_remove(_e_apps_list, a);
	e_app_fields_empty(a);
	E_FREE(a->path);
	free(a);
     }
}

void
e_app_fields_fill(E_App *a, const char *path)
{
   Eet_File *ef;
   char buf[PATH_MAX];
   char *str, *v;
   char *lang;
   int size;
   
   /* get our current language */
   lang = getenv("LANG");
   /* if its "C" its the default - so drop it */
   if ((lang) && (!strcmp(lang, "C")))
     lang = NULL;
   if (!path) path = a->path;
   ef = eet_open(path, EET_FILE_MODE_READ);
   if (!ef) return;

   if (lang)
     {
	snprintf(buf, sizeof(buf), "app/info/name[%s]", lang);
	v = eet_read(ef, buf, &size);
	if (!v) v = eet_read(ef, "app/info/name", &size);
     }
   else
     v = eet_read(ef, "app/info/name", &size);
   if (v)
     {
	str = malloc(size + 1);
	memcpy(str, v, size);
	str[size] = 0;
	a->name = str;
	free(v);
     }

   if (lang)
     {
	snprintf(buf, sizeof(buf), "app/info/generic[%s]", lang);
	v = eet_read(ef, buf, &size);
	if (!v) v = eet_read(ef, "app/info/generic", &size);
     }
   else
     v = eet_read(ef, "app/info/generic", &size);

   if (v)
     {
	str = malloc(size + 1);
	memcpy(str, v, size);
	str[size] = 0;
	a->generic = str;
	free(v);
     }

   if (lang)
     {
	snprintf(buf, sizeof(buf), "app/info/comment[%s]", lang);
	v = eet_read(ef, buf, &size);
	if (!v) v = eet_read(ef, "app/info/comment", &size);
     }
   else
     v = eet_read(ef, "app/info/comment", &size);

   if (v)
     {
	str = malloc(size + 1);
	memcpy(str, v, size);
	str[size] = 0;
	a->comment = str;
	free(v);
     }

   v = eet_read(ef, "app/info/exe", &size);
   if (v)
     {
	str = malloc(size + 1);
	memcpy(str, v, size);
	str[size] = 0;
	a->exe = str;
	free(v);
     }
   v = eet_read(ef, "app/icon/class", &size);
   if (v)
     {
	str = malloc(size + 1);
	memcpy(str, v, size);
	str[size] = 0;
	a->icon_class = str;
	free(v);
     }
   v = eet_read(ef, "app/window/name", &size);
   if (v)
     {
	str = malloc(size + 1);
	memcpy(str, v, size);
	str[size] = 0;
	a->win_name = str;
	free(v);
     }
   v = eet_read(ef, "app/window/class", &size);
   if (v)
     {
	str = malloc(size + 1);
	memcpy(str, v, size);
	str[size] = 0;
	a->win_class = str;
	free(v);
     }
   v = eet_read(ef, "app/window/title", &size);
   if (v)
     {
	str = malloc(size + 1);
	memcpy(str, v, size);
	str[size] = 0;
	a->win_title = str;
	free(v);
     }
   v = eet_read(ef, "app/window/role", &size);
   if (v)
     {
	str = malloc(size + 1);
	memcpy(str, v, size);
	str[size] = 0;
	a->win_role = str;
	free(v);
     }
   v = eet_read(ef, "app/info/startup_notify", &size);
   if (v)
     {
	a->startup_notify = *v;
	free(v);
     }
   v = eet_read(ef, "app/info/wait_exit", &size);
   if (v)
     {
	a->wait_exit = *v;
	free(v);
     }
   eet_close(ef);
}

void
e_app_fields_empty(E_App *a)
{
   E_FREE(a->name);
   E_FREE(a->generic);
   E_FREE(a->comment);
   E_FREE(a->exe);
   E_FREE(a->icon_class);
   E_FREE(a->win_name);
   E_FREE(a->win_class);
   E_FREE(a->win_title);
   E_FREE(a->win_role);
}

Ecore_List *
e_app_dir_file_list_get(E_App *a)
{
   Ecore_List *files, *files2;
   char *file;
   FILE *f;
   char buf[PATH_MAX];
   
   snprintf(buf, sizeof(buf), "%s/.order", a->path);
   files = ecore_file_ls(a->path);
   f = fopen(buf, "rb");
   if (f)
     {
	files2 = ecore_list_new();
	while (fgets(buf, sizeof(buf), f))
	  {
	     int len;

	     len = strlen(buf);
	     if (len > 0)
	       {
		  if (buf[len - 1] == '\n')
		    {
		       buf[len - 1] = 0;
		       len--;
		    }
		  if (len > 0)
		    {
		       if (files)
			 {
			    ecore_list_goto_first(files);
			    while ((file = ecore_list_current(files)))
			      {
				 if (!strcmp(buf, file))
				   {
				      ecore_list_remove(files);
				      free(file);
				      break;
				   }
				 ecore_list_next(files);
			      }
			 }
		       ecore_list_append(files2, strdup(buf));
		    }
	       }
	  }
	fclose(f);
     }
   else
     {
	files2 = files;
	files = NULL;
     }
   if (files)
     {
	ecore_list_goto_first(files);
	while ((file = ecore_list_next(files)))
	  {
	     if (file[0] != '.')
	       ecore_list_append(files2, file);
	     else
	       free(file);
	  }
	ecore_list_destroy(files);
     }
   files = files2;
   if (files)
     ecore_list_goto_first(files);
   return files;
}

static E_App *
_e_app_subapp_file_find(E_App *a, const char *file)
{
   Evas_List *l;
   
   for (l = a->subapps; l; l = l->next)
     {
	E_App *a2;
	
	a2 = l->data;
	if ((a2->deleted) || ((a2->orig) && (a2->orig->deleted))) continue;
	if (!strcmp(ecore_file_get_file(a2->path), file)) return a2;
     }
   return NULL;
}

static void
_e_app_change(E_App *a, E_App_Change ch)
{
   Evas_List *l;

   _e_apps_callbacks_walking = 1;
   for (l = _e_apps_change_callbacks; l; l = l->next)
     {
	E_App_Callback *cb;
	
	cb = l->data;
	if (!cb->delete_me)
	  {
	     cb->func(cb->data, a, ch);
	  }
     }
   _e_apps_callbacks_walking = 0;
   if (_e_apps_callbacks_delete_me)
     {
	for (l = _e_apps_change_callbacks; l;)
	  {
	     E_App_Callback *cb;
	     Evas_List *pl;
	     
	     cb = l->data;
	     pl = l;
	     l = l->next;
	     if (cb->delete_me)
	       {
		  _e_apps_change_callbacks = evas_list_remove_list(_e_apps_change_callbacks, pl);
		  free(cb);
	       }
	  }
	_e_apps_callbacks_delete_me = 0;
     }
}

static void
_e_app_cb_monitor(void *data, Ecore_File_Monitor *em,
		  Ecore_File_Event event, const char *path)
{
   E_App     *app;
   char      *file;
   
   app = data;
   if ((!app) || (app->deleted))
     {
	printf("Event on a deleted eapp\n");
	return;
     }

   /* If this dir isn't scanned yet, no need to report changes! */
   if (!app->scanned)
     return;

   file = ecore_file_get_file((char *)path);
   if (!strcmp(file, ".order"))
     {
	if ((event == ECORE_FILE_EVENT_CREATED_FILE)
	    || (event == ECORE_FILE_EVENT_DELETED_FILE)
	    || (event == ECORE_FILE_EVENT_MODIFIED))
	  {
	      _e_app_subdir_rescan(app);
	  }
	else
	  {
	     printf("BUG: Weird event for .order: %d\n", event);
	  }
     }
   else if (!strcmp(file, ".directory.eap"))
     {
	if ((event == ECORE_FILE_EVENT_CREATED_FILE)
	    || (event == ECORE_FILE_EVENT_MODIFIED))
	  {
	     e_app_fields_empty(app);
	     e_app_fields_fill(app, path);
	     _e_app_change(app, E_APP_CHANGE);
	  }
	else if (event == ECORE_FILE_EVENT_DELETED_FILE)
	  {
	     e_app_fields_empty(app);
	     app->name = strdup(ecore_file_get_file(app->path));
	  }
	else
	  {
	     printf("BUG: Weird event for .directory.eap: %d\n", event);
	  }
     }
   else
     {
	if (event == ECORE_FILE_EVENT_MODIFIED)
	  {
	     E_App *a2;

	     a2 = _e_app_subapp_file_find(app, file);
	     if (a2)
	       {
		  Evas_List *l;

		  e_app_fields_empty(a2);
		  e_app_fields_fill(a2, path);
		  _e_app_change(a2, E_APP_CHANGE);

		  for (l = a2->references; l; l = l->next)
		    {
		       E_App *a3;

		       a3 = l->data;
		       if (_e_app_copy(a3, a2))
			 _e_app_change(a3, E_APP_CHANGE);
		    }
	       }
	  }
	else if ((event == ECORE_FILE_EVENT_CREATED_FILE)
		 || (event == ECORE_FILE_EVENT_CREATED_DIRECTORY))
	  {
	     /* FIXME: Check if someone wants a reference to this
	      * app */
	     _e_app_subdir_rescan(app);
	  }
	else if (event == ECORE_FILE_EVENT_DELETED_FILE)
	  {
	     E_App *a;
	     Evas_List *l;

	     a = _e_app_subapp_file_find(app, file);
	     if (a)
	       {
		  a->deleted = 1;
		  for (l = a->references; l;)
		    {
		       E_App *a2;

		       a2 = l->data;
		       l = l->next;
		       if (a2->parent)
			 _e_app_subdir_rescan(a2->parent);
		    }
		  _e_app_subdir_rescan(app);
	       }
	  }
	else if (event == ECORE_FILE_EVENT_DELETED_SELF)
	  {
	     Evas_List *l;

	     app->deleted = 1;
	     for (l = app->references; l;)
	       {
		  E_App *a2;

		  a2 = l->data;
		  l = l->next;
		  if (a2->parent)
		    _e_app_subdir_rescan(a2->parent);
	       }
	     if (app->parent)
	       _e_app_subdir_rescan(app->parent);
	     else
	       {
		  /* A main e_app has been deleted!
		   * We don't unref this, the code which holds this
		   * eapp must do it. */
		  _e_app_change(app, E_APP_DEL);
	       }
	  }
     }
}

static void
_e_app_subdir_rescan(E_App *app)
{
   Ecore_List        *files;
   Evas_List         *subapps = NULL, *changes = NULL, *l, *l2;
   E_App_Change_Info *ch;
   char               buf[PATH_MAX];
   char              *s;

   files = e_app_dir_file_list_get(app);
   if (files)
     {
	while ((s = ecore_list_next(files)))
	  {
	     E_App *a2;

	     a2 = _e_app_subapp_file_find(app, s);
	     if (a2)
	       {
		  subapps = evas_list_append(subapps, a2);
	       }
	     else
	       {
		  /* If we still haven't found it, it is new! */
		  snprintf(buf, sizeof(buf), "%s/%s", app->path, s);
		  a2 = e_app_new(buf, 1);
		  if (a2)
		    {
		       a2->parent = app;
		       ch = calloc(1, sizeof(E_App_Change_Info));
		       ch->app = a2;
		       ch->change = E_APP_ADD;
		       e_object_ref(E_OBJECT(ch->app));
		       changes = evas_list_append(changes, ch);

		       subapps = evas_list_append(subapps, a2);
		    }
		  else
		    {
		       /* We ask for a reference! */
		       Evas_List *pl;
		       E_App *a3;

		       pl = _e_apps_repositories;
		       while ((!a2) && (pl))
			 {
			    snprintf(buf, sizeof(buf), "%s/%s", (char *)pl->data, s);
			    a2 = e_app_new(buf, 1);
			    pl = pl->next;
			 }
		       if (a2)
			 {
			    a3 = E_OBJECT_ALLOC(E_App, E_APP_TYPE, _e_app_free);
			    if (a3)
			      {
				 if (_e_app_copy(a3, a2))
				   {
				      a3->parent = app;
				      ch = calloc(1, sizeof(E_App_Change_Info));
				      ch->app = a3;
				      ch->change = E_APP_ADD;
				      e_object_ref(E_OBJECT(ch->app));
				      changes = evas_list_append(changes, ch);

				      subapps = evas_list_append(subapps, a3);
				      a2->references = evas_list_append(a2->references, a3);
				      _e_apps_list = evas_list_prepend(_e_apps_list, a3);
				   }
				 else
				   e_object_del(E_OBJECT(a3));
			      }
			 }
		    }
	       }
	     free(s);
	  }
	ecore_list_destroy(files);
     }
   for (l = app->subapps; l; l = l->next)
     {
	E_App *a2;

	a2 = l->data;
	for (l2 = subapps; l2; l2 = l2->next)
	  if (l->data == l2->data)
	    {
	       /* We still have this app */
	       a2 = NULL;
	       break;
	    }
	if (a2)
	  {
	     a2->deleted = 1;
	     ch = calloc(1, sizeof(E_App_Change_Info));
	     ch->app = a2;
	     ch->change = E_APP_DEL;
	     /* We don't need to ref this,
	      * it has an extra ref
	      e_object_ref(E_OBJECT(ch->app));
	      */
	     changes = evas_list_append(changes, ch);
	  }
     }
   /* FIXME: We only need to tell about order changes if there are! */
   evas_list_free(app->subapps);
   app->subapps = subapps;
   ch = calloc(1, sizeof(E_App_Change_Info));
   ch->app = app;
   ch->change = E_APP_ORDER;
   e_object_ref(E_OBJECT(ch->app));
   changes = evas_list_append(changes, ch);

   for (l = changes; l; l = l->next)
     {
	ch = l->data;
	_e_app_change(ch->app, ch->change);
	e_object_unref(E_OBJECT(ch->app));
	free(ch);
     }
   if (changes)
     {
	E_App_Cache *ac;
	
	ac = e_app_cache_generate(app);
	e_app_cache_save(ac, app->path);
	e_app_cache_free(ac);
     }
   evas_list_free(changes);
}

static int
_e_app_is_eapp(const char *path)
{
   char *p;

   p = strrchr(path, '.');
   if (!p)
     return 0;

   p++;
   if ((strcasecmp(p, "eap")))
     return 0;

   return 1;
}

static int
_e_app_copy(E_App *dst, E_App *src)
{
   if (src->deleted)
     {
	printf("BUG: This app is deleted, can't make a copy: %s\n", src->path);
	return 0;
     }
   if (!_e_app_is_eapp(src->path))
     {
	printf("BUG: The app isn't an eapp: %s\n", src->path);
	return 0;
     }

   dst->orig = src;

   dst->name = src->name;
   dst->generic = src->generic;
   dst->comment = src->comment;
   dst->exe = src->exe;
   dst->path = src->path;
   dst->win_name = src->win_name;
   dst->win_class = src->win_class;
   dst->win_title = src->win_title;
   dst->win_role = src->win_role;
   dst->icon_class = src->icon_class;
   dst->startup_notify = src->startup_notify;
   dst->wait_exit = src->wait_exit;
   dst->starting = src->starting;
   dst->scanned = src->scanned;

   return 1;
}

static void
_e_app_save_order(E_App *app)
{
   FILE *f;
   char buf[PATH_MAX];
   Evas_List *l;
   
   if (!app) return;

   snprintf(buf, sizeof(buf), "%s/.order", app->path);
   f = fopen(buf, "wb");
   if (!f) return;

   for (l = app->subapps; l; l = l->next)
     {
	E_App *a;

	a = l->data;
	fprintf(f, "%s\n", ecore_file_get_file(a->path));
     }
   fclose(f);
}

static int
_e_apps_cb_exit(void *data, int type, void *event)
{
   Ecore_Event_Exe_Exit *ev;
   Evas_List *l;
   E_App *a;

   ev = event;
   if (!ev->exe) return 1;
   a = _e_app_ecore_exe_find(ev->exe);
   if (!a) return 1;

   if (ev->exit_code == 127) /* /bin/sh uses this if cmd not found */
     e_error_dialog_show(_("Run Error"),
			 _("Enlightenment was unable run the program:\n"
			   "\n"
			   "%s\n"
			   "\n"
			   "The command was not found\n"),
			 a->exe);
   for (l = a->instances; l; l = l->next)
     {
	E_App_Instance *inst;

	inst = l->data;
	if (ev->exe == inst->exe)
	  {
	     if (inst->expire_timer)
	       {
		  ecore_timer_del(inst->expire_timer);
		  inst->expire_timer = NULL;
	       }
	     inst->exe = NULL;
	     a->instances = evas_list_remove_list(a->instances, l);
	     free(inst);
	     _e_apps_start_pending = evas_list_remove(_e_apps_start_pending, a);
	     break;
	  }
     }
   _e_app_change(a, E_APP_EXIT);
   return 1;
}

static int
_e_app_cb_event_border_add(void *data, int type, void *event)
{
   E_Event_Border_Add *ev;
   Evas_List *l, *ll, *removes = NULL;
   E_App *a;
   E_App_Instance *inst;
   
   ev = event;
   if (ev->border->client.netwm.e_start_launch_id <= 0) return 1;
   for (l = _e_apps_start_pending; l; l = l->next)
     {
	a = l->data;
	for (ll = a->instances; ll; ll = ll->next)
	  {
	     inst = ll->data;
	     if (inst->launch_id == ev->border->client.netwm.e_start_launch_id)
	       {
		  if (inst->expire_timer)
		    {
		       ecore_timer_del(inst->expire_timer);
		       inst->expire_timer = NULL;
		    }
		  removes = evas_list_append(removes, a);
		  e_object_ref(E_OBJECT(a));
		  break;
	       }
	  }
     }
   while (removes)
     {
	a = removes->data;
        _e_app_change(a, E_APP_READY);
	_e_apps_start_pending = evas_list_remove(_e_apps_start_pending, a);
        e_object_unref(E_OBJECT(a));
	removes = evas_list_remove_list(removes, removes);
     }
   return 1;
}

static int
_e_app_cb_expire_timer(void *data)
{
   E_App_Instance *inst;
   E_App *a;
   
   inst = data;
   a = inst->app;
   _e_apps_start_pending = evas_list_remove(_e_apps_start_pending, a);
   inst->expire_timer = NULL;
   _e_app_change(a, E_APP_READY_EXPIRE);
   return 0;
}

static E_App *
_e_app_ecore_exe_find(Ecore_Exe *exe)
{
   Evas_List *l, *l2;

   for (l = _e_apps_start_pending; l; l = l->next)
     {
	E_App *a;

	a = l->data;
	for (l2 = a->instances; l2; l2 = l2->next)
	  {
	     E_App_Instance *inst;

	     inst = l2->data;
	     if (inst->exe == exe)
	       return a;
	  }
     }
   return NULL;
}
