/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* TODO List:
 * 
 * - We assume only .eapp files in 'all', no subdirs
 * - If a .order file references a non-existing file, and the file
 *   is added in 'all', it doesn't show!
 * - track app execution state, visibility state etc. and call callbacks
 * - calls to execute an app or query its runing/starting state etc.
 */

/* local subsystem functions */
typedef struct _E_App_Change_Info E_App_Change_Info;
typedef struct _E_App_Callback    E_App_Callback;

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

static void      _e_app_free               (E_App *a);
static void      _e_app_fields_fill        (E_App *a, const char *path);
static void      _e_app_fields_empty       (E_App *a);
static Evas_List *_e_app_dir_file_list_get (E_App *a);
static E_App     *_e_app_subapp_path_find  (E_App *a, const char *subpath);
static void      _e_app_change             (E_App *a, E_App_Change ch);
static int       _e_apps_cb_exit           (void *data, int type, void *event);
static void      _e_app_cb_monitor         (void *data, Ecore_File_Monitor *em, Ecore_File_Type type, Ecore_File_Event event, const char *path);
static void      _e_app_subdir_rescan      (E_App *app);
static int       _e_app_is_eapp            (const char *path);

/* local subsystem globals */
static Evas_Hash   *_e_apps = NULL;
static Evas_List   *_e_apps_list = NULL;
static int          _e_apps_callbacks_walking = 0;
static int          _e_apps_callbacks_delete_me = 0;
static Evas_List   *_e_apps_change_callbacks = NULL;
static Ecore_Event_Handler *_e_apps_exit_handler = NULL;
static Evas_List   *_e_apps_repositories = NULL;
static E_App       *_e_apps_all = NULL;

/* externally accessible functions */
int
e_app_init(void)
{
   char *home;
   char buf[PATH_MAX];
   
   home = e_user_homedir_get();
   snprintf(buf, sizeof(buf), "%s/.e/e/applications/all", home);
   free(home);
   _e_apps_repositories = evas_list_append(_e_apps_repositories, strdup(buf));
   _e_apps_exit_handler = ecore_event_handler_add(ECORE_EVENT_EXE_EXIT, _e_apps_cb_exit, NULL);
   _e_apps_all = e_app_new(buf, 1);
   return 1;
}

int
e_app_shutdown(void)
{
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
   return 1;
}

E_App *
e_app_new(const char *path, int scan_subdirs)
{
   E_App *a;
   char buf[PATH_MAX];

   a = evas_hash_find(_e_apps, path);
   if (a)
     {
	if (a->deleted)
	  return NULL;
	e_object_ref(E_OBJECT(a));
	return a;
     }

   if (ecore_file_exists(path))
     {
	a = E_OBJECT_ALLOC(E_App, E_APP_TYPE, _e_app_free);
	/* record the path */
	a->path = strdup(path);

	if (ecore_file_is_dir(a->path))
	  {
	     snprintf(buf, sizeof(buf), "%s/.directory.eapp", path);
	     if (ecore_file_exists(buf))
	       _e_app_fields_fill(a, buf);
	     else
	       a->name = strdup(ecore_file_get_file(a->path));
	     if (scan_subdirs) e_app_subdir_scan(a, scan_subdirs);

	     a->monitor = ecore_file_monitor_add(a->path, _e_app_cb_monitor, a);
	  }
	else if (_e_app_is_eapp(path))
	  {
	     _e_app_fields_fill(a, path);

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
   return a;

error:
   if (a->monitor) ecore_file_monitor_del(a->monitor);
   _e_app_fields_empty(a);
   free(a);
   return NULL;
}

int
e_app_is_parent(E_App *parent, E_App *app)
{
   Evas_List *l;

   if (app->parent == parent)
     return 1;

   for (l = app->references; l; l = l->next)
     {
	E_App *a2;

	a2 = l->data;
	if (a2 == parent)
	  return 1;
     }
   return 0;
}

void
e_app_subdir_scan(E_App *a, int scan_subdirs)
{
   Evas_List *files, *l;
   char buf[PATH_MAX];

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
   files = _e_app_dir_file_list_get(a);
   for (l = files; l; l = l->next)
     {
	E_App *a2;
	char *s;

	s = l->data;
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
		  a2->references = evas_list_append(a2->references, a);
		  a->subapps = evas_list_append(a->subapps, a2);
	       }
	  }
	free(s);
     }
   files = evas_list_free(files);
}

int
e_app_exec(E_App *a)
{
   Ecore_Exe *exe;
   
   E_OBJECT_CHECK_RETURN(a, 0);
   E_OBJECT_TYPE_CHECK_RETURN(a, E_APP_TYPE, 0);
   if (!a->exe) return 0;
   exe = ecore_exe_run(a->exe, a);
   if (!exe) return 0;
   a->instances = evas_list_append(a->instances, exe);
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
e_app_window_name_class_find(char *name, char *class)
{
   Evas_List *l;
   
   if (!name && !class)
     return NULL;

   for (l = _e_apps_list; l; l = l->next)
     {
	E_App *a;
	
	a = l->data;
	if ((a->win_name) || (a->win_class))
	  {
	     int ok = 0;
	     
//	     printf("%s.%s == %s.%s\n", name, class, a->win_name, a->win_class);
	     if ((!a->win_name) ||
		 ((a->win_name) && name && (!strcmp(a->win_name, name))))
	       ok++;
	     if ((!a->win_class) ||
		 ((a->win_class) && class && (!strcmp(a->win_class, class))))
	       ok++;
	     if (ok >= 2)
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
   Evas_List *l;

   while (a->instances)
     {
	Ecore_Exe *exe;
	
	exe = a->instances->data;
	ecore_exe_free(exe);
	a->instances = evas_list_remove_list(a->instances, a->instances);
     }
   while (a->subapps)
     {
	E_App *a2;
	
	a2 = a->subapps->data;
	a->subapps = evas_list_remove_list(a->subapps, a->subapps);
	/* Only remove the parent if it is us. */
	if (a2->parent == a)
	  a2->parent = NULL;
	e_object_unref(E_OBJECT(a2));
     }
   for (l = a->references; l; l = l->next)
     {
	E_App *a2;

	a2 = l->data;
	a2->subapps = evas_list_remove(a2->subapps, a);
     }
   evas_list_free(a->references);

   if (a->parent)
     a->parent->subapps = evas_list_remove(a->parent->subapps, a);
   if (a->monitor)
     ecore_file_monitor_del(a->monitor);
   _e_apps = evas_hash_del(_e_apps, a->path, a);
   _e_apps_list = evas_list_remove(_e_apps_list, a);
   _e_app_fields_empty(a);
   free(a);
}

static void
_e_app_fields_fill(E_App *a, const char *path)
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

static void
_e_app_fields_empty(E_App *a)
{
   if (a->name)
     {
	free(a->name);
	a->name = NULL;
     }
   if (a->generic)
     {
	free(a->generic);
	a->generic = NULL;
     }
   if (a->comment)
     {
	free(a->comment);
	a->comment = NULL;
     }
   if (a->exe)
     {
	free(a->exe);
	a->exe = NULL;
     }
   if (a->win_name)
     {
	free(a->win_name);
	a->win_name = NULL;
     }
   if (a->win_class)
     {
	free(a->win_class);
	a->win_class = NULL;
     }
}

static Evas_List *
_e_app_dir_file_list_get(E_App *a)
{
   Evas_List *files, *files2 = NULL, *l;
   FILE *f;
   char buf[PATH_MAX];
   
   snprintf(buf, sizeof(buf), "%s/.order", a->path);
   files = ecore_file_ls(a->path);
   f = fopen(buf, "rb");
   if (f)
     {
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
		       for (l = files; l; l = l->next)
			 {
			    if (!strcmp(buf, l->data))
			      {
				 free(l->data);
				 files = evas_list_remove_list(files, l);
				 break;
			      }
			 }
		       files2 = evas_list_append(files2, strdup(buf));
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
   while (files)
     {
	char *s;
	
	s = files->data;
	if (s[0] != '.')
	  files2 = evas_list_append(files2, s);
	else
	  free(s);
	files = evas_list_remove_list(files, files);
     }
   files = files2;
   return files;
}

static E_App *
_e_app_subapp_path_find(E_App *a, const char *subpath)
{
   Evas_List *l;
   
   for (l = a->subapps; l; l = l->next)
     {
	E_App *a2;
	
	a2 = l->data;
	if ((!strcmp(a2->path, subpath)) && (!a2->deleted)) return a2;
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

static int
_e_apps_cb_exit(void *data, int type, void *event)
{
   Ecore_Event_Exe_Exit *ev;
   E_App *a;
   
   ev = event;
   if (ev->exe)
     {
	a = ecore_exe_data_get(ev->exe);
	if (a)
	  {
	     a->instances = evas_list_remove(a->instances, ev->exe);
	     _e_app_change(a, E_APP_EXIT);
	  }
     }
   return 1;
}

static void
_e_app_cb_monitor(void *data, Ecore_File_Monitor *em,
		  Ecore_File_Type type, Ecore_File_Event event, const char *path)
{
   E_App     *app;
   Evas_List *l;
   char      *file;
   
   app = data;

   if (ecore_file_monitor_type_get(em) != ECORE_FILE_TYPE_DIRECTORY)
     {
	printf("BUG: E_App should only monitor directories!\n");
	return;
     }

   /* If this dir isn't scanned yet, no need to report changes! */
   if (!app->scanned)
     return;

   file = ecore_file_get_file((char *)path);
   if (!strcmp(file, ".order"))
     {
	switch (event)
	  {
	   case ECORE_FILE_EVENT_NONE:
	   case ECORE_FILE_EVENT_EXISTS:
	      break;
	   case ECORE_FILE_EVENT_CREATED:
	   case ECORE_FILE_EVENT_DELETED:
	   case ECORE_FILE_EVENT_CHANGED:
	      _e_app_subdir_rescan(app);
	      break;
	  }
     }
   else if (!strcmp(file, ".directory.eapp"))
     {
	switch (event)
	  {
	   case ECORE_FILE_EVENT_NONE:
	   case ECORE_FILE_EVENT_EXISTS:
	      break;
	   case ECORE_FILE_EVENT_CREATED:
	   case ECORE_FILE_EVENT_CHANGED:
	      _e_app_fields_empty(app);
	      _e_app_fields_fill(app, path);
	      _e_app_change(app, E_APP_CHANGE);
	      break;
	   case ECORE_FILE_EVENT_DELETED:
	      _e_app_fields_empty(app);
	      app->name = strdup(ecore_file_get_file(app->path));
	      break;
	  }
     }
   else
     {
	E_App *a2;

	switch (event)
	  {
	   case ECORE_FILE_EVENT_NONE:
	   case ECORE_FILE_EVENT_EXISTS:
	      break;
	   case ECORE_FILE_EVENT_CREATED:
	      /* If a file is created, wait for the directory change to update
	       * the eapp. */
	      break;
	   case ECORE_FILE_EVENT_DELETED:
	      /* If something is deleted, mark it as deleted
	       * and wait for the directory change to update
	       * the client. */
	      a2 = _e_app_subapp_path_find(app, path);
	      if (a2)
		{
		   a2->deleted = 1;
		   /* If this app is in a main repository, tell all referencing
		    * apps to rescan. */
		   for (l = a2->references; l; l = l->next)
		     {
			E_App *app;

			app = l->data;
			_e_app_subdir_rescan(app);
		     }
		}
	      break;
	   case ECORE_FILE_EVENT_CHANGED:
	      if (type == ECORE_FILE_TYPE_FILE)
		{
		   a2 = _e_app_subapp_path_find(app, path);
		   if (a2)
		     {
			_e_app_fields_empty(a2);
			_e_app_fields_fill(a2, path);
			_e_app_change(a2, E_APP_CHANGE);
		     }
		}
	      /* If the current directory has changed, we must rescan
	       * to check which files are here. */
	      if ((type == ECORE_FILE_TYPE_DIRECTORY) && !strcmp(path, app->path))
		{
		   /* We don't know why it's changed, better rescan... */
		   _e_app_subdir_rescan(app);
		}
	      break;
	  }
     }
}

static void
_e_app_subdir_rescan(E_App *app)
{
   Evas_List         *files, *l, *l2;
   Evas_List         *subapps = NULL, *changes = NULL;
   E_App_Change_Info *ch;
   char               buf[PATH_MAX];

   files = _e_app_dir_file_list_get(app);
   for (l = files; l; l = l->next)
     {
	E_App *a2;
	char *s;

	s = l->data;
	snprintf(buf, sizeof(buf), "%s/%s", app->path, s);
	a2 = _e_app_subapp_path_find(app, buf);
	if (!a2)
	  {
	     /* Do we have a reference? */
	     Evas_List *pl;

	     pl = _e_apps_repositories;
	     while ((!a2) && (pl))
	       {
		  snprintf(buf, sizeof(buf), "%s/%s", (char *)pl->data, s);
		  a2 = _e_app_subapp_path_find(app, buf);
		  pl = pl->next;
	       }
	  }
	if (!a2)
	  {
	     /* If we still haven't found it, it is new! */
	     a2 = e_app_new(buf, 1);
	     if (a2)
	       {
		  a2->parent = app;
		  ch = calloc(1, sizeof(E_App_Change_Info));
		  ch->app = a2;
		  ch->change = E_APP_ADD;
		  e_object_ref(E_OBJECT(ch->app));
		  changes = evas_list_append(changes, ch);
	       }
	     else
	       {
		  /* We ask for a reference! */
		  Evas_List *pl;

		  pl = _e_apps_repositories;
		  while ((!a2) && (pl))
		    {
		       snprintf(buf, sizeof(buf), "%s/%s", (char *)pl->data, s);
		       a2 = e_app_new(buf, 1);
		       pl = pl->next;
		    }
		  if (a2)
		    {
		       a2->references = evas_list_append(a2->references, app);
		       ch = calloc(1, sizeof(E_App_Change_Info));
		       ch->app = a2;
		       ch->change = E_APP_ADD;
		       e_object_ref(E_OBJECT(ch->app));
		       changes = evas_list_append(changes, ch);
		    }
	       }
	  }
	if (a2)
	  subapps = evas_list_append(subapps, a2);
	free(s);
     }
   evas_list_free(files);
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
   if ((strcasecmp(p, "eapp")))
     return 0;

   return 1;
}

