#include "e.h"

/* TODO List:
 * 
 * * if a application .eapp file is added in a different location in a monitored app tree but has the same filename as an existing one somewhere else, the existing one gets a changed callback, not an dded callback for the new one
 * * track app execution state, visibility state etc. and call callbacks
 * * calls to execute an app or query its runing/starting state etc.
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
static void      _e_app_fields_fill        (E_App *a, char *path);
static void      _e_app_fields_empty       (E_App *a);
static Evas_List *_e_app_dir_file_list_get (E_App *a, char *path);
static E_App     *_e_app_subapp_path_find  (E_App *a, char *subpath);
static void      _e_app_monitor            (void);
static void      _e_app_change             (E_App *a, E_App_Change ch);
static int       _e_app_check              (void *data);
static Evas_Bool _e_app_check_each         (Evas_Hash *hash, const char *key, void *data, void *fdata);
static int       _e_apps_cb_exit           (void *data, int type, void *event);

/* local subsystem globals */
static Evas_Hash   *_e_apps = NULL;
static Evas_List   *_e_apps_list = NULL;
static Ecore_Timer *_e_apps_checker = NULL;
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
   char buf[4096];
   
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
e_app_new(char *path, int scan_subdirs)
{
   E_App *a;

   a = evas_hash_find(_e_apps, path);
   if (a)
     {
	e_object_ref(E_OBJECT(a));
	return a;
     }
   a = E_OBJECT_ALLOC(E_App, _e_app_free);
   a->mod_time = e_file_mod_time(path);
   if (e_file_is_dir(path))
     {
	char buf[4096];
	
	a->path = strdup(path);
	snprintf(buf, sizeof(buf), "%s/.directory.eapp", path);
	a->directory_mod_time = e_file_mod_time(buf);
	if (e_file_exists(buf))
	  _e_app_fields_fill(a, buf);
	else
	  a->name = strdup(e_file_get_file(path));
	if (scan_subdirs) e_app_subdir_scan(a, scan_subdirs);
     }
   else if (e_file_exists(path))
     {
	char *p;
	
	/* check if file ends in .eapp */
	p = strrchr(path, '.');
	if (!p)
	  {
	     free(a);
	     return NULL;
	  }
	p++;
	if ((strcasecmp(p, "eapp")))
	  {
	     free(a);
	     return NULL;
	  }
	/* record the path */
	a->path = strdup(path);
	
	/* get the field data */
	_e_app_fields_fill(a, path);
	
	/* no exe field.. not valid. drop it */
	if (!a->exe)
	  {
	     if (a->name) free(a->name);
	     if (a->generic) free(a->generic);
	     if (a->comment) free(a->comment);
	     if (a->exe) free(a->exe);
	     if (a->path) free(a->path);
	     if (a->win_name) free(a->win_name);
	     if (a->win_class) free(a->win_class);
	     free(a);
	     return NULL;
	  }
     }
   else
     {
	free(a);
	return NULL;
     }
   _e_apps = evas_hash_add(_e_apps, a->path, a);
   _e_apps_list = evas_list_prepend(_e_apps_list, a);
   _e_app_monitor();
   return a;
}

void
e_app_subdir_scan(E_App *a, int scan_subdirs)
{
   Evas_List *files, *files2 = NULL;
   FILE *f;
   char buf[4096];

   E_OBJECT_CHECK(a);
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
   files = _e_app_dir_file_list_get(a, a->path);
   while (files)
     {
	E_App *a2;
	char *s;
	
	s = files->data;
	a2 = NULL;
	if (s[0] != '.')
	  {
	     Evas_List *pl;
			
	     snprintf(buf, sizeof(buf), "%s/%s", a->path, s);
	     if (e_file_exists(buf)) a2 = e_app_new(buf, scan_subdirs);
	     pl = _e_apps_repositories;
	     while ((!a2) && (pl))
	       {
		  snprintf(buf, sizeof(buf), "%s/%s", (char *)pl->data, s);
		  a2 = e_app_new(buf, scan_subdirs);
		  pl = pl->next;
	       }
	     if (a2)
	       {
		  a->subapps = evas_list_append(a->subapps, a2);
		  a2->parent = a;
	       }
	     free(s);
	  }
	files = evas_list_remove_list(files, files);
     }
}

int
e_app_exec(E_App *a)
{
   Ecore_Exe *exe;
   
   E_OBJECT_CHECK_RETURN(a, 0);
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
   return a->starting;
}

int
e_app_running_get(E_App *a)
{
   E_OBJECT_CHECK_RETURN(a, 0);
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
   _e_app_monitor();
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
		  _e_app_monitor();
	       }
	     return;
	  }
     }
}

E_App *
e_app_window_name_class_find(char *name, char *class)
{
   Evas_List *l;
   
   for (l = _e_apps_list; l; l = l->next)
     {
	E_App *a;
	
	a = l->data;
	if ((a->win_name) || (a->win_class))
	  {
	     int ok = 0;
	     
//	     printf("%s.%s == %s.%s\n", name, class, a->win_name, a->win_class);
	     if ((!a->win_name) ||
		 ((a->win_name) && (!strcmp(a->win_name, name))))
	       ok++;
	     if ((!a->win_class) ||
		 ((a->win_class) && (!strcmp(a->win_class, class))))
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
	a->subapps = evas_list_remove(a->subapps, a2);
	a2->parent = NULL;
	e_object_unref(E_OBJECT(a2));
     }
   if (a->parent)
     a->parent->subapps = evas_list_remove(a->parent->subapps, a);
   _e_apps = evas_hash_del(_e_apps, a->path, a);
   _e_apps_list = evas_list_remove(_e_apps_list, a);
   if (a->name) free(a->name);
   if (a->generic) free(a->generic);
   if (a->comment) free(a->comment);
   if (a->exe) free(a->exe);
   if (a->path) free(a->path);
   if (a->win_name) free(a->win_name);
   if (a->win_class) free(a->win_class);
   free(a);
   _e_app_monitor();
}

static void
_e_app_fields_fill(E_App *a, char *path)
{
   Eet_File *ef;
   char buf[4096];
   char *str, *v;
   char *lang;
   int size;
   
   /* get our current language */
   lang = getenv("LANG");
   /* if its "C" its the default - so drop it */
   if ((lang) && (!strcmp(lang, "C")))
     lang = NULL;
   ef = eet_open(a->path, EET_FILE_MODE_READ);
   if (!ef) return;
   if (lang) snprintf(buf, sizeof(buf), "app/info/name[%s]", lang);
   v = eet_read(ef, buf, &size);
   if (v)
     {
	str = malloc(size + 1);
	memcpy(str, v, size);
	str[size] = 0;
	a->name = str;
	free(v);
     }
   else
     {
	v = eet_read(ef, "app/info/name", &size);
	if (v)
	  {
	     str = malloc(size + 1);
	     memcpy(str, v, size);
	     str[size] = 0;
	     a->name = str;
	     free(v);
	  }
     }
   if (lang) snprintf(buf, sizeof(buf), "app/info/generic[%s]", lang);
   v = eet_read(ef, buf, &size);
   if (v)
     {
	str = malloc(size + 1);
	memcpy(str, v, size);
	str[size] = 0;
	a->generic = str;
	free(v);
     }
   else
     {
	v = eet_read(ef, "app/info/generic", &size);
	if (v)
	  {
	     str = malloc(size + 1);
	     memcpy(str, v, size);
	     str[size] = 0;
	     a->generic = str;
	     free(v);
	  }
     }
   if (lang) snprintf(buf, sizeof(buf), "app/info/comment[%s]", lang);
   v = eet_read(ef, buf, &size);
   if (v)
     {
	str = malloc(size + 1);
	memcpy(str, v, size);
	str[size] = 0;
	a->comment = str;
	free(v);
     }
   else
     {
	v = eet_read(ef, "app/info/comment", &size);
	if (v)
	  {
	     str = malloc(size + 1);
	     memcpy(str, v, size);
	     str[size] = 0;
	     a->comment = str;
	     free(v);
	  }
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
_e_app_dir_file_list_get(E_App *a, char *path)
{
   Evas_List *files, *files2 = NULL, *l;
   FILE *f;
   char buf[4096];
   
   snprintf(buf, sizeof(buf), "%s/.order", path);
   a->order_mod_time = e_file_mod_time(buf);
   files = e_file_ls(path);
   f = fopen(buf, "rb");
   if (f)
     {
	while (fgets(buf, sizeof(buf), f))
	  {
	     int len;
	     
	     len = strlen(buf);
	     if (len > 0)
	       {
		  int ok = 0;
		  
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
_e_app_subapp_path_find(E_App *a, char *subpath)
{
   Evas_List *l;
   
   for (l = a->subapps; l; l = l->next)
     {
	E_App *a2;
	
	a2 = l->data;
	if (!strcmp(a2->path, subpath)) return a2;
     }
   return NULL;
}

static void
_e_app_monitor(void)
{
   if ((_e_apps) && (_e_apps_change_callbacks))
     {
	if (!_e_apps_checker)
	  _e_apps_checker = ecore_timer_add(1.0, _e_app_check, NULL);
     }
   else
     {
	if (_e_apps_checker)
	  {
	     ecore_timer_del(_e_apps_checker);
	     _e_apps_checker = NULL;
	  }
     }
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
	_e_app_monitor();
     }
}

static int
_e_app_check(void *data)
{
   Evas_List *changes = NULL;
   
   evas_hash_foreach(_e_apps, _e_app_check_each, &changes);

   while (changes)
     {
	E_App_Change_Info *ch;
	Evas_List *l;
	
	ch = changes->data;
	changes = evas_list_remove_list(changes, changes);
	_e_app_change(ch->app, ch->change);
	e_object_unref(E_OBJECT(ch->app));
	free(ch);
     }
   if (_e_apps_checker) return 1;
   return 0;
}

static Evas_Bool
_e_app_check_each(Evas_Hash *hash, const char *key, void *data, void *fdata)
{
   Evas_List **changes;
   E_App *a;
   E_App_Change_Info *ch;
   
   changes = fdata;
   a = data;
   if (a->exe)
     {
	time_t mod_time;
	
	mod_time = e_file_mod_time(a->path);
	if (mod_time != a->mod_time)
	  {
	     a->mod_time = mod_time;
	     if (e_file_exists(a->path))
	       {
		  _e_app_fields_empty(a);
		  _e_app_fields_fill(a, a->path);
		  if (!a->exe)
		    {
		       ch = calloc(1, sizeof(E_App_Change_Info));
		       ch->app = a;
		       ch->change = E_APP_DEL;
		       e_object_ref(E_OBJECT(ch->app));
		       *changes = evas_list_append(*changes, ch);
		    }
		  else
		    {
		       ch = calloc(1, sizeof(E_App_Change_Info));
		       ch->app = a;
		       ch->change = E_APP_CHANGE;
		       e_object_ref(E_OBJECT(ch->app));
		       *changes = evas_list_append(*changes, ch);
		    }
	       }
	     else
	       {
		  ch = calloc(1, sizeof(E_App_Change_Info));
		  ch->app = a;
		  ch->change = E_APP_DEL;
		  e_object_ref(E_OBJECT(ch->app));
		  *changes = evas_list_append(*changes, ch);
	       }
	  }
     }
   else
     {
	time_t mod_time, order_mod_time, directory_mod_time;
	char buf[4096];
	
	mod_time = e_file_mod_time(a->path);
	snprintf(buf, sizeof(buf), "%s/.order", a->path);
	order_mod_time = e_file_mod_time(buf);
	snprintf(buf, sizeof(buf), "%s/.directory.eapp", a->path);
	directory_mod_time = e_file_mod_time(buf);
	if ((mod_time != a->mod_time) ||
	    (order_mod_time != a->order_mod_time) ||
	    (directory_mod_time != a->directory_mod_time))
	  {
	     a->mod_time = mod_time;
	     if (!e_file_is_dir(a->path))
	       {
		  ch = calloc(1, sizeof(E_App_Change_Info));
		  ch->app = a;
		  ch->change = E_APP_DEL;
		  e_object_ref(E_OBJECT(ch->app));
		  *changes = evas_list_append(*changes, ch);
	       }
	     else
	       {
		  if (order_mod_time != a->order_mod_time)
		    {
		       ch = calloc(1, sizeof(E_App_Change_Info));
		       ch->app = a;
		       ch->change = E_APP_ORDER;
		       e_object_ref(E_OBJECT(ch->app));
		       *changes = evas_list_append(*changes, ch);
		    }
		  if (directory_mod_time != a->directory_mod_time)
		    {
		       snprintf(buf, sizeof(buf), "%s/.directory.eapp", a->path);
		       _e_app_fields_empty(a);
		       _e_app_fields_fill(a, buf);
		       ch = calloc(1, sizeof(E_App_Change_Info));
		       ch->app = a;
		       ch->change = E_APP_CHANGE;
		       e_object_ref(E_OBJECT(ch->app));
		       *changes = evas_list_append(*changes, ch);
		    }
		  a->order_mod_time = order_mod_time;
		  a->directory_mod_time = directory_mod_time;
		  if (a->scanned)
		    {
		       Evas_List *l, *files;
		       
		       files = _e_app_dir_file_list_get(a, a->path);
		       for (l = files; l; l = l->next)
			 {
			    E_App *a2;
			    char *s;
			    
			    s = l->data;
			    snprintf(buf, sizeof(buf), "%s/%s", a->path, s);
			    if (!_e_app_subapp_path_find(a, buf))
			      {
				 a2 = e_app_new(buf, 0);
				 if (a2)
				   {
				      a2->parent = a;
				      a->subapps = evas_list_append(a->subapps, a2);
				      ch = calloc(1, sizeof(E_App_Change_Info));
				      ch->app = a2;
				      ch->change = E_APP_ADD;
				      e_object_ref(E_OBJECT(ch->app));
				      *changes = evas_list_append(*changes, ch);
				   }
			      }
			 }
		       for (l = files; l; l = l->next)
			 {
			    E_App *a2;
			    char *s;
			    
			    s = l->data;
			    snprintf(buf, sizeof(buf), "%s/%s", a->path, s);
			    a2 = _e_app_subapp_path_find(a, buf);
			    if (a2)
			      {
				 a->subapps = evas_list_remove(a->subapps, a2);
				 a->subapps = evas_list_append(a->subapps, a2);
			      }
			 }
		       while (files)
			 {
			    free(files->data);
			    files = evas_list_remove_list(files, files);
			 }
		    }
	       }
	  }
     }
   return 1;
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
