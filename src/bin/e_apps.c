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
   const char    *path;
   E_App_Cache   *cache;
   E_App         *app;
   Ecore_List    *files;
   Ecore_Timer   *timer;
   unsigned char  need_rewrite : 1;
};

static void      _e_app_free               (E_App *a);
static E_App     *_e_app_subapp_file_find  (E_App *a, const char *file);
static int        _e_app_new_save          (E_App *a);    
static void      _e_app_change             (E_App *a, E_App_Change ch);
static int       _e_apps_cb_exit           (void *data, int type, void *event);
static void      _e_app_cb_monitor         (void *data, Ecore_File_Monitor *em, Ecore_File_Event event, const char *path);
static void      _e_app_subdir_rescan      (E_App *app);
static int       _e_app_is_eapp            (const char *path);
static int       _e_app_copy               (E_App *dst, E_App *src);
static void      _e_app_save_order         (E_App *app);
static int       _e_app_cb_event_border_add(void *data, int type, void *event);
static int       _e_app_cb_expire_timer    (void *data);
static void      _e_app_cache_copy         (E_App_Cache *ac, E_App *a);
static int       _e_app_cb_scan_cache_timer(void *data);
static E_App    *_e_app_cache_new          (E_App_Cache *ac, const char *path, int scan_subdirs);
static int       _e_app_exe_valid_get      (const char *exe);
static char     *_e_app_localized_val_get (Eet_File *ef, const char *lang, const char *field, int *size);
static void      _e_app_print(const char *path, Ecore_File_Event event);

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
static const char  *_e_apps_path_all = NULL;
static const char  *_e_apps_path_trash = NULL;
static Evas_List   *_e_apps_start_pending = NULL;

#define EAP_MIN_WIDTH 8
#define EAP_MIN_HEIGHT 8

#define EAP_EDC_TMPL \
"images {\n"  \
"   image: \"%s\" COMP;\n" \
"}\n" \
"collections {\n" \
"   group {\n" \
"      name: \"icon\";\n" \
"      max: %d %d;\n" \
"      parts {\n" \
"	 part {\n" \
"	    name: \"image\";\n" \
"	    type: IMAGE;\n" \
"	    mouse_events: 0;\n" \
"	    description {\n" \
"	       state: \"default\" 0.00;\n" \
"	       visible: 1;\n" \
"	       aspect: 1.00 1.00;\n" \
"	       rel1 {\n" \
"		  relative: 0.00 0.00;\n" \
"		  offset: 0 0;\n" \
"	       }\n" \
"	       rel2 {\n" \
"		  relative: 1.00 1.00;\n" \
"		  offset: -1 -1;\n" \
"	       }\n" \
"	       image {\n" \
"		  normal: \"%s\";\n" \
"	       }\n" \
"	    }\n" \
"	 }\n" \
"      }\n" \
"   }\n" \
"}\n"

#define EAP_EDC_TMPL_EMPTY \
"images {\n " \
"}\n" \
"collections {\n" \
"}\n"


/* externally accessible functions */
EAPI int
e_app_init(void)
{
   char *home;
   char buf[PATH_MAX];
   
   e_app_cache_init();
   home = e_user_homedir_get();
   snprintf(buf, sizeof(buf), "%s/.e/e/applications/trash", home);
   _e_apps_path_trash = evas_stringshare_add(buf);
   snprintf(buf, sizeof(buf), "%s/.e/e/applications/all", home);
   _e_apps_path_all = evas_stringshare_add(buf);
   free(home);
   _e_apps_repositories = evas_list_append(_e_apps_repositories, evas_stringshare_add(buf));
   _e_apps_exit_handler = ecore_event_handler_add(ECORE_EXE_EVENT_DEL, _e_apps_cb_exit, NULL);
   _e_apps_border_add_handler = ecore_event_handler_add(E_EVENT_BORDER_ADD, _e_app_cb_event_border_add, NULL);
   _e_apps_all = e_app_new(buf, 1);
   return 1;
}

EAPI int
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
	evas_stringshare_del(_e_apps_repositories->data);
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
   evas_stringshare_del(_e_apps_path_trash);
   evas_stringshare_del(_e_apps_path_all);
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

EAPI void
e_app_unmonitor_all(void)
{
   Evas_List *l;
   
   for (l = _e_apps_list; l; l = l->next)
     {
	E_App *a;

	a = l->data;
	if (a->monitor)
	  {
	     ecore_file_monitor_del(a->monitor);
	     a->monitor = NULL;
	  }
     }
}

EAPI E_App *
e_app_raw_new(void)
{
   E_App *a;
   
   a = E_OBJECT_ALLOC(E_App, E_APP_TYPE, _e_app_free);
   return a;
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

EAPI E_App *
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

   ac = e_app_cache_load(path);
   if (ac)
     {
	a = _e_app_cache_new(ac, path, scan_subdirs);
	if (a)
	  {
	     _e_apps = evas_hash_add(_e_apps, a->path, a);
	     _e_apps_list = evas_list_prepend(_e_apps_list, a);
	     a->scanned = 1;
	  }
//	e_app_cache_free(ac);
     }
   else
     {
	if (ecore_file_exists(path))
	  {
	     a = E_OBJECT_ALLOC(E_App, E_APP_TYPE, _e_app_free);
	     
	     /* no image for now */
	     a->image = NULL;
	     a->width = 0;
	     a->height = 0;
	     /* record the path */
	     a->path = evas_stringshare_add(path);
	     
	     if (ecore_file_is_dir(a->path))
	       {
		  snprintf(buf, sizeof(buf), "%s/.directory.eap", path);
		  if (ecore_file_exists(buf))
		    e_app_fields_fill(a, buf);
		  else
		    a->name = evas_stringshare_add(ecore_file_get_file(a->path));
		  if (scan_subdirs) e_app_subdir_scan(a, scan_subdirs);
		  
		  a->monitor = ecore_file_monitor_add(a->path, _e_app_cb_monitor, a);
	       }
	     else if (_e_app_is_eapp(path))
	       {
		  e_app_fields_fill(a, path);
		  
		  /* no exe field.. not valid. drop it */
		  if (!_e_app_exe_valid_get(a->exe))
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
   if (a->path) evas_stringshare_del(a->path);
   e_app_fields_empty(a);
   free(a);
   return NULL;
}

EAPI E_App *
e_app_empty_new(const char *path)
{
   E_App *a;
   
   a = E_OBJECT_ALLOC(E_App, E_APP_TYPE, _e_app_free);
   a->image = NULL;
   if (path) a->path = evas_stringshare_add(path);   
   else
     {   
	if ((_e_apps_all) && (_e_apps_all->path))
	  {
	     char buf[4096];
	     
	     snprintf(buf, sizeof(buf), "%s/_new_app_%1.1f.eap", 
		      _e_apps_all->path, ecore_time_get());
	     a->parent = _e_apps_all;
	     _e_apps_all->subapps = evas_list_append(_e_apps_all->subapps, a);
	     a->path = evas_stringshare_add(buf);
	     _e_apps = evas_hash_add(_e_apps, a->path, a);
             _e_apps_list = evas_list_prepend(_e_apps_list, a);
	     _e_app_change(a, E_APP_ADD);
	  }
     }
   return a;
}

EAPI void
e_app_image_size_set(E_App *a, int w, int h)
{
   a->width = w;
   a->height = h;
}

EAPI int
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

EAPI int
e_app_equals(E_App *app1, E_App *app2)
{
   if ((!app1) || (!app2)) return 0;
   return ((app1 == app2) || (app1->orig == app2) ||
	   (app1 == app2->orig) || (app1->orig == app2->orig));
}

EAPI void
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
	  }
	ecore_list_destroy(files);
     }

   ac = e_app_cache_generate(a);
   e_app_cache_save(ac, a->path);
   e_app_cache_free(ac);
}

EAPI int
e_app_exec(E_App *a, int launch_id)
{
   Ecore_Exe *exe;
   E_App_Instance *inst;
   Evas_List *l;
   
   E_OBJECT_CHECK_RETURN(a, 0);
   E_OBJECT_TYPE_CHECK_RETURN(a, E_APP_TYPE, 0);
   if (!a->exe) return 0;
   /* FIXME: set up locale, encoding and input method env vars if they are in
    * the eapp file */
   inst = E_NEW(E_App_Instance, 1);
   if (!inst) return 0;
   /* We want the stdout and stderr as lines for the error dialog if it exits abnormally. */
   exe = ecore_exe_pipe_run(a->exe, ECORE_EXE_PIPE_AUTO | ECORE_EXE_PIPE_READ | ECORE_EXE_PIPE_ERROR | ECORE_EXE_PIPE_READ_LINE_BUFFERED | ECORE_EXE_PIPE_ERROR_LINE_BUFFERED, inst);
   if (!exe)
     {
	free(inst);
	e_util_dialog_show(_("Run Error"),
			   _("Enlightenment was unable to fork a child process:<br>"
			     "<br>"
			     "%s<br>"),
			   a->exe);
	return 0;
     }
   /* 20 lines at start and end, 20x100 limit on bytes at each end. */
   ecore_exe_auto_limits_set(exe, 2000, 2000, 20, 20);
   ecore_exe_tag_set(exe, "E/app");
   inst->app = a;
   inst->exe = exe;
   inst->launch_id = launch_id;
   inst->launch_time = ecore_time_get();
   inst->expire_timer = ecore_timer_add(10.0, _e_app_cb_expire_timer, inst);
   a->instances = evas_list_append(a->instances, inst);
//   e_object_ref(E_OBJECT(a));
   _e_apps_start_pending = evas_list_append(_e_apps_start_pending, a);
   if (a->startup_notify) a->starting = 1;
   for (l = a->references; l; l = l->next)
     {
	E_App *a2;
	
	a2 = l->data;
	_e_app_change(a2, E_APP_EXEC);
     }
   _e_app_change(a, E_APP_EXEC);
   return 1;
}

EAPI int
e_app_starting_get(E_App *a)
{
   E_OBJECT_CHECK_RETURN(a, 0);
   E_OBJECT_TYPE_CHECK_RETURN(a, E_APP_TYPE, 0);
   return a->starting;
}

EAPI int
e_app_running_get(E_App *a)
{
   E_OBJECT_CHECK_RETURN(a, 0);
   E_OBJECT_TYPE_CHECK_RETURN(a, E_APP_TYPE, 0);
   if (a->instances) return 1;
   return 0;
}

static void
_e_app_list_prepend_relative(E_App *add, E_App *before, E_App *parent)
{
   FILE *f;
   char buf[PATH_MAX];
   Evas_List *l;
   
   if ((!add) || (!parent)) return;
   snprintf(buf, sizeof(buf), "%s/.order", parent->path);
   f = fopen(buf, "wb");
   if (!f) return;

   for (l = parent->subapps; l; l = l->next)
     {
	E_App *a;

	a = l->data;
	if (a == before) fprintf(f, "%s\n", ecore_file_get_file(add->path));
	fprintf(f, "%s\n", ecore_file_get_file(a->path));
     }
   if (before == NULL) fprintf(f, "%s\n", ecore_file_get_file(add->path));
   fclose(f);
}

static void
_e_app_files_list_prepend_relative(Evas_List *files, E_App *before, E_App *parent)
{
   FILE *f;
   char buf[PATH_MAX];
   Evas_List *l, *l2;
   
   if ((!files) || (!parent)) return;
   snprintf(buf, sizeof(buf), "%s/.order", parent->path);
   f = fopen(buf, "wb");
   if (!f) return;

   for (l = parent->subapps; l; l = l->next)
     {
	E_App *a;

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
   if (before == NULL)
     {
	/* Add the new files */
	for (l2 = files; l2; l2 = l2->next)
	  {
	     char *file;
	     
	     file = l2->data;
	     fprintf(f, "%s\n", ecore_file_get_file(file));
	  }
     }
   fclose(f);
}

static void
_e_app_files_download(Evas_List *files)
{
   Evas_List *l;
   
   for (l = files; l; l = l->next)
     {
	char *file;
	char buf[PATH_MAX];
	
	file = l->data;
	if (!_e_app_is_eapp(file)) continue;
        snprintf(buf, sizeof(buf), "%s/%s", _e_apps_path_all,
		 ecore_file_get_file(file));
	if (!ecore_file_download(file, buf, NULL, NULL, NULL)) continue;
     }
}

EAPI void
e_app_list_prepend_relative(E_App *add, E_App *before)
{
   if ((!add) || (!before)) return;
   if (!before->parent) return;
   _e_app_list_prepend_relative(add, before, before->parent);
}

EAPI void
e_app_list_append(E_App *add, E_App *parent)
{
   if ((!add) || (!parent)) return;
   _e_app_list_prepend_relative(add, NULL, parent);
}

EAPI void
e_app_files_list_prepend_relative(Evas_List *files, E_App *before)
{
   _e_app_files_download(files);
   /* Force rescan of all subdir */
   _e_app_subdir_rescan(_e_apps_all);
   /* Change .order file */
   _e_app_files_list_prepend_relative(files, before, before->parent);
}

EAPI void
e_app_files_list_append(Evas_List *files, E_App *parent)
{
   _e_app_files_download(files);
   /* Force rescan of all subdir */
   _e_app_subdir_rescan(_e_apps_all);
   /* Change .order file */
   _e_app_files_list_prepend_relative(files, NULL, parent);
}

EAPI void
e_app_prepend_relative(E_App *add, E_App *before)
{
   char buf[PATH_MAX];

   if ((!add) || (!before)) return;
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
	evas_stringshare_del(add->path);
	add->path = evas_stringshare_add(buf);
     }

   _e_app_save_order(before->parent);
   _e_app_change(add, E_APP_ADD);
   _e_app_change(before->parent, E_APP_ORDER);
}

EAPI void
e_app_append(E_App *add, E_App *parent)
{
   char buf[PATH_MAX];

   if ((!add) || (!parent)) return;
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
	evas_stringshare_del(add->path);
	add->path = evas_stringshare_add(buf);
     }

   _e_app_save_order(parent);
   _e_app_change(add, E_APP_ADD);
}

EAPI void
e_app_files_prepend_relative(Evas_List *files, E_App *before)
{
   Evas_List *l;

   if (!before) return;
   if (!before->parent) return;

   _e_app_files_download(files);
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

EAPI void
e_app_files_append(Evas_List *files, E_App *parent)
{
   Evas_List *l, *subapps;

   if (!parent) return;
   subapps = parent->subapps;

   _e_app_files_download(files);
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

EAPI void
e_app_remove(E_App *a)
{
   Evas_List *l;
   char buf[PATH_MAX];

   if (!a) return;
   if (!a->parent) return;

   a->parent->subapps = evas_list_remove(a->parent->subapps, a);
   /* Check if this app is in a repository or in the parents dir */
   snprintf(buf, sizeof(buf), "%s/%s", a->parent->path, ecore_file_get_file(a->path));
   if (ecore_file_exists(buf))
     {
	/* Move to trash */
	snprintf(buf, sizeof(buf), "%s/%s", _e_apps_path_trash, ecore_file_get_file(a->path));
	ecore_file_mv(a->path, buf);
	evas_stringshare_del(a->path);
	a->path = evas_stringshare_add(buf);
     }
   _e_app_save_order(a->parent);
   for (l = a->references; l; l = l->next)
     {
	E_App *a2;
	
	a2 = l->data;
	e_app_remove(a2);
     }
   _e_app_change(a, E_APP_DEL);
   a->parent = NULL;
   e_object_unref(E_OBJECT(a));
}

EAPI void
e_app_change_callback_add(void (*func) (void *data, E_App *a, E_App_Change ch), void *data)
{
   E_App_Callback *cb;
   
   cb = E_NEW(E_App_Callback, 1);
   cb->func = func;
   cb->data = data;
   _e_apps_change_callbacks = evas_list_append(_e_apps_change_callbacks, cb);
}

/* 
 * Delete the registered callback which has been registered with the data 
 * given data pointer. This function will return after the first match is
 * made.
 *
 * This will only delete the internal callback function reference. It will 
 * not delete the data. If the data or callback pointers can not be matched 
 * this function does nothing. 
 * 
 * @func pointer to function to be deleted 
 * @data pointer that was initialy registered with the add function
 */
EAPI void
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

EAPI E_App *
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

EAPI E_App *
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

EAPI E_App *
e_app_file_find(const char *file)
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

EAPI E_App *
e_app_name_find(const char *name)
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

EAPI E_App *
e_app_generic_find(const char *generic)
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

EAPI E_App *
e_app_exe_find(const char *exe)
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



EAPI Evas_List *
e_app_name_glob_list(const char *name)
{
   Evas_List *l, *list = NULL;
   
   if (!name) return NULL;

   for (l = _e_apps_list; l; l = l->next)
     {
	E_App *a;
	
	a = l->data;
	if (a->name)
	  {
	     if (e_util_glob_case_match(a->name, name))
	       list = evas_list_append(list, a);
	  }
     }
   return list;
}

EAPI Evas_List *
e_app_generic_glob_list(const char *generic)
{
   Evas_List *l, *list = NULL;
   
   if (!generic) return NULL;

   for (l = _e_apps_list; l; l = l->next)
     {
	E_App *a;
	
	a = l->data;
	if (a->generic)
	  {
	     if (e_util_glob_case_match(a->generic, generic))
	       list = evas_list_append(list, a);
	  }
     }
   return list;
}

EAPI Evas_List *
e_app_exe_glob_list(const char *exe)
{
   Evas_List *l, *list = NULL;
   
   if (!exe) return NULL;

   for (l = _e_apps_list; l; l = l->next)
     {
	E_App *a;
	
	a = l->data;
	if (a->exe)
	  {
	     if (e_util_glob_match(a->exe, exe))
	       list = evas_list_append(list, a);
	  }
     }
   return list;
}

EAPI Evas_List *
e_app_comment_glob_list(const char *comment)
{
   Evas_List *l, *list = NULL;
   
   if (!comment) return NULL;

   for (l = _e_apps_list; l; l = l->next)
     {
	E_App *a;
	
	a = l->data;
	if (a->comment)
	  {
	     if (e_util_glob_case_match(a->comment, comment))
	       list = evas_list_append(list, a);
	  }
     }
   return list;
}





EAPI void
e_app_fields_fill(E_App *a, const char *path)
{
   Eet_File *ef;
   char *str, *v;
   const char *lang;
   int size;
   
   /* get our current language */
   lang = e_intl_language_alias_get();
   
   /* if its "C" its the default - so drop it */
   if (!strcmp(lang, "C"))
     {
	lang = NULL;
     }
   if (!path) path = a->path;
   ef = eet_open(path, EET_FILE_MODE_READ);
   if (!ef) return;

#define STORE(member) \
   if (v) \
     { \
	str = alloca(size + 1); \
	memcpy(str, v, size); \
	str[size] = 0; \
	a->member = evas_stringshare_add(str); \
	free(v); \
     }
   v = _e_app_localized_val_get(ef, lang, "app/info/name", &size);
   STORE(name);
   v = _e_app_localized_val_get(ef, lang, "app/info/generic", &size);
   STORE(generic);
   v = _e_app_localized_val_get(ef, lang, "app/info/comment", &size);
   STORE(comment);

   v = eet_read(ef, "app/info/exe", &size);
   STORE(exe);
   v = eet_read(ef, "app/icon/class", &size);
   STORE(icon_class);
   v = eet_read(ef, "app/window/name", &size);
   STORE(win_name);
   v = eet_read(ef, "app/window/class", &size);
   STORE(win_class);
   v = eet_read(ef, "app/window/title", &size);
   STORE(win_title);
   v = eet_read(ef, "app/window/role", &size);
   STORE(win_role);
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

static char *
_e_app_localized_val_get(Eet_File *ef, const char *lang, const char *field, int *size)
{
   char *s, *v;
   char buf[PATH_MAX];

   if (lang)
     {
	s = e_intl_locale_canonic_get(lang, E_INTL_LOC_LANG | E_INTL_LOC_REGION);
	snprintf(buf, sizeof(buf), "%s[%s]", field, s);
	free(s);
	v = eet_read(ef, buf, size);
	if (v)
	  return v;

	s = e_intl_locale_canonic_get(lang, E_INTL_LOC_LANG);
	snprintf(buf, sizeof(buf), "%s[%s]", field, s);
	free(s);
	v = eet_read(ef, buf, size);
	if (v)
	  return v;
     }
   /* Fall back to default locale */
   return eet_read(ef, field, size);
}

EAPI void
e_app_fields_save(E_App *a)
{
   Eet_File *ef;
   char buf[PATH_MAX];
   const char *lang;
   unsigned char tmp[1];
   int img;

//   if ((!a->path) || (!ecore_file_exists(a->path)))
//     {
	_e_app_new_save(a);
//	img = 0;
//     }
//   else
//     img = 1;

   /* get our current language */
   lang = e_intl_language_alias_get();

   /* if its "C" its the default - so drop it */
   if (!strcmp(lang, "C")) lang = NULL;

   ef = eet_open(a->path, EET_FILE_MODE_READ_WRITE);
   if (!ef) return;

   if (a->name)
     {
	/*if (lang) snprintf(buf, sizeof(buf), "app/info/name[%s]", lang);  
	  else */
	snprintf(buf, sizeof(buf), "app/info/name");
	eet_write(ef, buf, a->name, strlen(a->name), 0);
     }
   
   if (a->generic)
     {
	/*if (lang) snprintf(buf, sizeof(buf), "app/info/generic[%s]", lang);
	  else */
	snprintf(buf, sizeof(buf), "app/info/generic");
	eet_write(ef, buf, a->generic, strlen(a->generic), 0);
     }

   if (a->comment)
     {
	/*if (lang) snprintf(buf, sizeof(buf), "app/info/comment[%s]", lang);
	  else*/
	snprintf(buf, sizeof(buf), "app/info/comment");
	eet_write(ef, buf, a->comment, strlen(a->comment), 0);
     }

   if (a->exe)
     eet_write(ef, "app/info/exe", a->exe, strlen(a->exe), 0);
   if (a->win_name)
     eet_write(ef, "app/window/name", a->win_name, strlen(a->win_name), 0);
   if (a->win_class)
     eet_write(ef, "app/window/class", a->win_class, strlen(a->win_class), 0);
   if (a->win_title)
     eet_write(ef, "app/window/title", a->win_title, strlen(a->win_title), 0);
   if (a->win_role)
     eet_write(ef, "app/window/role", a->win_role, strlen(a->win_role), 0);
   if (a->icon_class)
     eet_write(ef, "app/icon/class", a->icon_class, strlen(a->icon_class), 0);
   
   if (a->startup_notify)
     tmp[0] = 1;
   else
     tmp[0] = 0;
   eet_write(ef, "app/info/startup_notify", tmp, 1, 0);
   
   if (a->wait_exit)
     tmp[0] = 1;
   else
     tmp[0] = 0;   
   eet_write(ef, "app/info/wait_exit", tmp, 1, 0);

   /*
   if ((a->image) && (img))
     {
	int alpha;
	Ecore_Evas *buf;
	Evas *evasbuf;
	Evas_Coord iw, ih;
	Evas_Object *im;
	const int *data;

	buf = ecore_evas_buffer_new(1, 1);
	evasbuf = ecore_evas_get(buf);
	im = evas_object_image_add(evasbuf);
	evas_object_image_file_set(im, a->image, NULL);
	iw = 0; ih = 0;
	evas_object_image_size_get(im, &iw, &ih);
	alpha = evas_object_image_alpha_get(im);
	if (a->width <= EAP_MIN_WIDTH)
	  a->width = EAP_MIN_WIDTH;
	if (a->height <= EAP_MIN_HEIGHT)
	  a->height = EAP_MIN_HEIGHT;	
	if ((iw > 0) && (ih > 0))
	  {
	     ecore_evas_resize(buf, a->width, a->height);
	     evas_object_image_fill_set(im, 0, 0, a->width, a->height);
	     evas_object_resize(im, a->height, a->width);
	     evas_object_move(im, 0, 0);
	     evas_object_show(im);	     
	     data = ecore_evas_buffer_pixels_get(buf);
	     eet_data_image_write(ef, "images/0", (void *)data, a->width, a->height, alpha, 1, 0, 0);
	  }
     }
    */
   eet_close(ef);
   if (a->parent)
     {
	Evas_List *l;
	
	_e_app_change(a->parent, E_APP_CHANGE);
	_e_app_change(a, E_APP_CHANGE);
	for (l = a->references; l; l = l->next)
	  {
	     E_App *a2;
	     
	     a2 = l->data;
	     if (_e_app_copy(a2, a)) _e_app_change(a2, E_APP_CHANGE);
	  }
	_e_app_subdir_rescan(a->parent);
     }
}

EAPI void
e_app_fields_empty(E_App *a)
{
   if (a->name) evas_stringshare_del(a->name);
   if (a->generic) evas_stringshare_del(a->generic);
   if (a->comment) evas_stringshare_del(a->comment);
   if (a->exe) evas_stringshare_del(a->exe);
   if (a->icon_class) evas_stringshare_del(a->icon_class);
   if (a->win_name) evas_stringshare_del(a->win_name);
   if (a->win_class) evas_stringshare_del(a->win_class);
   if (a->win_title) evas_stringshare_del(a->win_title);
   if (a->win_role) evas_stringshare_del(a->win_role);
   a->name = NULL;
   a->generic = NULL;
   a->comment = NULL;
   a->exe = NULL;
   a->icon_class = NULL;
   a->win_name = NULL;
   a->win_class = NULL;
   a->win_title = NULL;
   a->win_role = NULL;
}

EAPI Ecore_List *
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
	ecore_list_set_free_cb(files2, free);
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
	       ecore_list_append(files2, strdup(file));
	  }
	ecore_list_destroy(files);
     }
   files = files2;
   if (files)
     ecore_list_goto_first(files);
   return files;
}

EAPI int
e_app_valid_exe_get(E_App *a)
{
   char *exe;
   int ok = 1;

   if (!a->exe) return 0;
   exe = ecore_file_app_exe_get(a->exe);
   if (!exe) return 0;
   ok = ecore_file_app_installed(exe);
   free(exe);
   return ok;
}

EAPI Evas_Object *
e_app_icon_add(Evas *evas, E_App *a)
{
   Evas_Object *o;
   
   o = edje_object_add(evas);
   if (!e_util_edje_icon_list_set(o, a->icon_class))
     edje_object_file_set(o, a->path, "icon");
   return o;
}


/* local subsystem functions */

/* write out a new eap, code borrowed from Engrave */
static int
_e_app_new_save(E_App *a)
{
   static char tmpn[4096];
   int fd = 0, ret = 0;
   char cmd[2048];  
   char ipart[512];
   FILE *out = NULL;
   char *start, *end, *imgdir = NULL;
   int i;   
      
   if (!a->path) return 0;
   strcpy(tmpn, "/tmp/eapp_edit_cc.edc-tmp-XXXXXX");
   fd = mkstemp(tmpn);
   if (fd < 0)
     {
	fprintf(stderr, "Unable to create tmp file: %s\n", strerror(errno));
	return 0;
     }
   close(fd);
   
   out = fopen(tmpn, "w");
   if (!out)
     {
	printf("can't open %s for writing\n", tmpn);
	return 0;
     }
   
   i = 0;
   if (a->image)
     {
	start = strchr(a->image, '/');
	end = strrchr(a->image, '/');

	if (start == end) imgdir = strdup("/");
	else if ((!start) || (!end)) imgdir = strdup("");
	else
	  {
	     imgdir = malloc((end - start + 1));
	     if (imgdir)
	       {
		  memcpy(imgdir, start, end - start);
		  imgdir[end - start] = 0;
	       }
	  }
     }     

   if (imgdir)
     {
	snprintf(ipart, sizeof(ipart), "-id %s",
		 e_util_filename_escape(imgdir));
	free(imgdir);
     }
   else ipart[0] = 0;
   
   if (a->image)
     {
	if (a->width <= 0) a->width = EAP_MIN_WIDTH;
	if (a->height <= 0) a->height = EAP_MIN_HEIGHT;
	fprintf(out, EAP_EDC_TMPL, 
		e_util_filename_escape(ecore_file_get_file(a->image)), 
		a->width, a->height, 
		e_util_filename_escape(ecore_file_get_file(a->image)));
     }
   else
     fprintf(out, EAP_EDC_TMPL_EMPTY);
   fclose(out);
   
   snprintf(cmd, sizeof(cmd), "edje_cc -v %s %s %s", ipart, tmpn, 
	    e_util_filename_escape(a->path));
   ret = system(cmd);
   
   if (ret < 0)
     {
	fprintf(stderr, "Unable to execute edje_cc on tmp file: %s\n",
		strerror(errno));
	return 0;
     }
   
   ecore_file_unlink(tmpn);
   return 1;   
}

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
	while (a->references)
	  {
	     E_App *a2;
	     
	     a2 = a->references->data;
	     a2->orig = NULL;
	     a->references = evas_list_remove_list(a->references, a->references);
	     printf("BUG: An original eapp shouldn't have any references when freed! %d\n", evas_list_count(a->references));
	  }

	if (a->parent)
	  a->parent->subapps = evas_list_remove(a->parent->subapps, a);
	if (a->monitor)
	  ecore_file_monitor_del(a->monitor);
	_e_apps = evas_hash_del(_e_apps, a->path, a);
	_e_apps_list = evas_list_remove(_e_apps_list, a);
	e_app_fields_empty(a);
	if (a->path) evas_stringshare_del(a->path);
	free(a);
     }
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

   if (ch == E_APP_DEL)
     printf("APP_DEL %s\n", a->path);
   if (ch == E_APP_CHANGE)
     printf("APP_CHANGE %s\n", a->path);
   if (ch == E_APP_ADD)
     printf("APP_ADD %s\n", a->path);
   _e_apps_callbacks_walking = 1;
   for (l = _e_apps_change_callbacks; l; l = l->next)
     {
	E_App_Callback *cb;
	
	cb = l->data;
	if (!cb->delete_me) cb->func(cb->data, a, ch);
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
   Evas_List *l;
   
   app = data;
   if ((!app) || (app->deleted))
     {
	printf("Event on a deleted eapp\n");
	return;
     }

   /* If this dir isn't scanned yet, no need to report changes! */
   if (!app->scanned)
     return;

#if 0
   _e_app_print(path, event);
#endif

   file = (char *)ecore_file_get_file((char *)path);
   if (!strcmp(file, ".order"))
     {
	if ((event == ECORE_FILE_EVENT_CREATED_FILE) ||
	    (event == ECORE_FILE_EVENT_DELETED_FILE) ||
	    (event == ECORE_FILE_EVENT_MODIFIED))
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
	if ((event == ECORE_FILE_EVENT_CREATED_FILE) ||
	    (event == ECORE_FILE_EVENT_MODIFIED))
	  {
	     e_app_fields_empty(app);
	     e_app_fields_fill(app, path);
	     _e_app_change(app, E_APP_CHANGE);
	     for (l = app->references; l; l = l->next)
	       {
		  E_App *a2;
		  
		  a2 = l->data;
		  if (_e_app_copy(a2, app)) _e_app_change(a2, E_APP_CHANGE);
	       }
	  }
	else if (event == ECORE_FILE_EVENT_DELETED_FILE)
	  {
	     e_app_fields_empty(app);
	     app->name = evas_stringshare_add(ecore_file_get_file(app->path));
	  }
	else
	  {
	     printf("BUG: Weird event for .directory.eap: %d\n", event);
	  }
     }
   else if (!strcmp(file, ".eap.cache.cfg"))
     {
	/* ignore this file */
     }
   else
     {
	if (event == ECORE_FILE_EVENT_MODIFIED)
	  {
	     E_App *a;

	     a = _e_app_subapp_file_find(app, file);
	     if (a)
	       {
		  Evas_List *l;

		  e_app_fields_empty(a);
		  e_app_fields_fill(a, path);
		  if (!_e_app_exe_valid_get(a->exe))
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
		  else
		    {
		       _e_app_change(a, E_APP_CHANGE);
		       for (l = a->references; l; l = l->next)
			 {
			    E_App *a2;

			    a2 = l->data;
			    if (_e_app_copy(a2, a))
			      _e_app_change(a2, E_APP_CHANGE);
			 }
		       _e_app_subdir_rescan(app);
		    }
	       }
	  }
	else if ((event == ECORE_FILE_EVENT_CREATED_FILE) ||
		 (event == ECORE_FILE_EVENT_CREATED_DIRECTORY))
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
		       ch = E_NEW(E_App_Change_Info, 1);
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
				      ch = E_NEW(E_App_Change_Info, 1);
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
	     ch = E_NEW(E_App_Change_Info, 1);
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
   ch = E_NEW(E_App_Change_Info, 1);
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
   Ecore_Exe_Event_Del *ev;
   E_App_Instance *ai;
   E_App *a;
   Evas_List *l;

   ev = event;
   if (!ev->exe) return 1;
   if (!(ecore_exe_tag_get(ev->exe) && 
	 (!strcmp(ecore_exe_tag_get(ev->exe), "E/app")))) return 1;
   ai = ecore_exe_data_get(ev->exe);
   if (!ai) return 1;
   a = ai->app;
   if (!a) return 1;

   /* /bin/sh uses this if cmd not found */
   if ((ev->exited) &&
       ((ev->exit_code == 127) || (ev->exit_code == 255)))
     {
	E_Dialog *dia;
	
	dia = e_dialog_new(e_container_current_get(e_manager_current_get()));
	if (dia)
	  {
	     char buf[4096];
	     
	     e_dialog_title_set(dia, _("Application run error"));
	     snprintf(buf, sizeof(buf),
		      _("Enlightenment was unable to run the application:<br>"
			"<br>"
			"%s<br>"
			"<br>"
			"The application failed to start."),
		      a->exe);
	     e_dialog_text_set(dia, buf);
//	     e_dialog_icon_set(dia, "enlightenment/error", 64);
	     e_dialog_button_add(dia, _("OK"), NULL, NULL, NULL);
	     e_dialog_button_focus_num(dia, 1);
	     e_win_centered_set(dia->win, 1);
	     e_dialog_show(dia);
	  }
     }
   /* Let's hope that everyhing returns this properly. */
   else if (!((ev->exited) && (ev->exit_code == EXIT_SUCCESS))) 
     {   /* Show the error dialog with details from the exe. */
	E_App_Autopsy *aut;
	
	aut = E_NEW(E_App_Autopsy, 1);
	aut->app = a;
	aut->del = *ev;
	aut->error = ecore_exe_event_data_get(ai->exe, ECORE_EXE_PIPE_ERROR);
	aut->read = ecore_exe_event_data_get(ai->exe, ECORE_EXE_PIPE_READ);
	e_app_error_dialog(NULL, aut);
     }
   if (ai->expire_timer) ecore_timer_del(ai->expire_timer);
   free(ai);
   a->instances = evas_list_remove(a->instances, ai);
   _e_apps_start_pending = evas_list_remove(_e_apps_start_pending, a);
   for (l = a->references; l; l = l->next)
     {
	E_App *a2;
	
	a2 = l->data;
	_e_app_change(a2, E_APP_EXIT);
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
   if (ev->border->client.netwm.startup_id <= 0) return 1;
   for (l = _e_apps_start_pending; l; l = l->next)
     {
	a = l->data;
	for (ll = a->instances; ll; ll = ll->next)
	  {
	     inst = ll->data;
	     if (inst->launch_id == ev->border->client.netwm.startup_id)
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
	for (l = a->references; l; l = l->next)
	  {
	     E_App *a2;
	     
	     a2 = l->data;
	     _e_app_change(a2, E_APP_READY);
	  }
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
   Evas_List *l;
   
   inst = data;
   a = inst->app;
   _e_apps_start_pending = evas_list_remove(_e_apps_start_pending, a);
   inst->expire_timer = NULL;
   for (l = a->references; l; l = l->next)
     {
	E_App *a2;
	
	a2 = l->data;
	_e_app_change(a2, E_APP_READY_EXPIRE);
     }
   _e_app_change(a, E_APP_READY_EXPIRE);
   return 0;
}

static void
_e_app_cache_copy(E_App_Cache *ac, E_App *a)
{
#define IF_DUP(x) if ((ac->x) && (ac->x[0] != 0)) a->x = evas_stringshare_add(ac->x)
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
	evas_stringshare_del(sc->path);
	free(sc);
	printf("Cache scan finish.\n");
	return 0;
     }
   snprintf(buf, sizeof(buf), "%s/%s", sc->path, s);
   is_dir = ecore_file_is_dir(buf);
   if (_e_app_is_eapp(s) || is_dir)
     {
	ac = evas_hash_find(sc->cache->subapps_hash, s);
	if (ac)
	  {
	     if (is_dir != ac->is_dir)
	       {
		  printf("Cache %s - CHANGED TYPE\n", s);
		  sc->need_rewrite = 1;
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
_e_app_cache_new(E_App_Cache *ac, const char *path, int scan_subdirs)
{
   Evas_List *l;
   E_App *a;
   char buf[PATH_MAX];
   E_App_Scan_Cache *sc;
   
   a = E_OBJECT_ALLOC(E_App, E_APP_TYPE, _e_app_free);
   _e_app_cache_copy(ac, a);
   a->path = evas_stringshare_add(path);
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
	     if (a2) 
	       {
	          a2->parent = a;
		  a->subapps = evas_list_append(a->subapps, a2);
	       }
	  }
	else
	  {
	     if (!ac2->is_link)
	       {
		  a2 = E_OBJECT_ALLOC(E_App, E_APP_TYPE, _e_app_free);
		  _e_app_cache_copy(ac2, a2);
		  if (ac2->is_dir)
		    {
		       if (a2->exe) evas_stringshare_del(a2->exe);
		    }
		  a2->parent = a;
		  a2->path = evas_stringshare_add(buf);
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

   sc = E_NEW(E_App_Scan_Cache, 1);
   if (sc)
     {
	sc->path = evas_stringshare_add(path);
	sc->cache = ac;
	sc->app = a;
	sc->files = e_app_dir_file_list_get(a);
	sc->timer = ecore_timer_add(0.500, _e_app_cb_scan_cache_timer, sc);
	e_object_ref(E_OBJECT(sc->app));
     }
   else
     e_app_cache_free(ac);
   return a;
}

static int
_e_app_exe_valid_get(const char *exe)
{
   if ((!exe) || (!exe[0])) return 0;
   return 1;
}

static void
_e_app_print(const char *path, Ecore_File_Event event)
{
   switch (event)
     {
      case ECORE_FILE_EVENT_NONE:
	 printf("E_App none: %s\n", path);
	 break;
      case ECORE_FILE_EVENT_CREATED_FILE:
	 printf("E_App created file: %s\n", path);
	 break;
      case ECORE_FILE_EVENT_CREATED_DIRECTORY:
	 printf("E_App created directory: %s\n", path);
	 break;
      case ECORE_FILE_EVENT_DELETED_FILE:
	 printf("E_App deleted file: %s\n", path);
	 break;
      case ECORE_FILE_EVENT_DELETED_DIRECTORY:
	 printf("E_App deleted directory: %s\n", path);
	 break;
      case ECORE_FILE_EVENT_DELETED_SELF:
	 printf("E_App deleted self: %s\n", path);
	 break;
      case ECORE_FILE_EVENT_MODIFIED:
	 printf("E_App modified: %s\n", path);
	 break;
     }
}
