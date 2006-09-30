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

#define DEBUG 0
#define NO_APP_LIST 1
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

struct _E_App_Hash_Idler
{
   Ecore_Idler *idler;
   Evas *evas;
   int all_done;
   double begin;
};

static Evas_Bool _e_apps_hash_cb_init      (Evas_Hash *hash, const char *key, void *data, void *fdata);
static int       _e_apps_hash_idler_cb     (void *data);
static Evas_Bool _e_apps_hash_idler_cb_init(Evas_Hash *hash, const char *key, void *data, void *fdata);
static void      _e_app_free               (E_App *a);
static E_App     *_e_app_subapp_file_find  (E_App *a, const char *file);
static int        _e_app_new_save          (E_App *a);    
static void      _e_app_change             (E_App *a, E_App_Change ch);
static int       _e_apps_cb_exit           (void *data, int type, void *event);
static void      _e_app_cb_monitor         (void *data, Ecore_File_Monitor *em, Ecore_File_Event event, const char *path);
static void      _e_app_subdir_rescan      (E_App *app);
static int       _e_app_is_eapp            (const char *path);
static int       _e_app_copy               (E_App *dst, E_App *src);
static void      _e_app_fields_save_others(E_App *a);
static void      _e_app_save_order         (E_App *app);
static int       _e_app_cb_event_border_add(void *data, int type, void *event);
static int       _e_app_cb_expire_timer    (void *data);
static int       _e_app_exe_valid_get      (const char *exe);
static char     *_e_app_localized_val_get  (Eet_File *ef, const char *lang, const char *field, int *size);
#if DEBUG
static void      _e_app_print              (const char *path, Ecore_File_Event event);
#endif
static void      _e_app_check_order        (const char *file);
static int       _e_app_order_contains     (E_App *a, const char *file);
static void      _e_app_resolve_file_name  (char *buf, size_t size, const char *path, const char *file);

/* local subsystem globals */
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
static Evas_Hash   *_e_apps_every_app = NULL;
static struct _E_App_Hash_Idler _e_apps_hash_idler;

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
   Ecore_List  *_e_apps_all_filenames = NULL;
   const char *home;
   char buf[PATH_MAX];
   double begin;
   
   home = e_user_homedir_get();
   snprintf(buf, sizeof(buf), "%s/.e/e/applications/trash", home);
   _e_apps_path_trash = evas_stringshare_add(buf);
   snprintf(buf, sizeof(buf), "%s/.e/e/applications/all", home);
   _e_apps_path_all = evas_stringshare_add(buf);
   _e_apps_repositories = evas_list_append(_e_apps_repositories, evas_stringshare_add(buf));
   _e_apps_exit_handler = ecore_event_handler_add(ECORE_EXE_EVENT_DEL, _e_apps_cb_exit, NULL);
   _e_apps_border_add_handler = ecore_event_handler_add(E_EVENT_BORDER_ADD, _e_app_cb_event_border_add, NULL);
   ecore_desktop_instrumentation_reset();
   begin = ecore_time_get();
   _e_apps_all_filenames = ecore_file_ls(_e_apps_path_all);
   if (_e_apps_all_filenames)
     {
        const char *file;
	
        while ((file = ecore_list_next(_e_apps_all_filenames)))
	  {
	     E_App *app;
	     
             snprintf(buf, sizeof(buf), "%s/%s", _e_apps_path_all, file);
             app = e_app_empty_new(buf);
	     if ((app) && (app->path))
	       _e_apps_every_app = evas_hash_direct_add(_e_apps_every_app, app->path, app);
	  }
        ecore_list_destroy(_e_apps_all_filenames);
     }
   _e_apps_all = e_app_new(_e_apps_path_all, 0);
   evas_hash_foreach(_e_apps_every_app, _e_apps_hash_cb_init, NULL);
   printf("INITIAL APP SCAN %3.3f\n", ecore_time_get() - begin);
   ecore_desktop_instrumentation_print();
   _e_apps_hash_idler.all_done = 0;
   /* FIXME: I need a fake evas here so that the icon searching will work. */
   _e_apps_hash_idler.evas = NULL;
   _e_apps_hash_idler.begin = ecore_time_get();
   _e_apps_hash_idler.idler = ecore_idler_add(_e_apps_hash_idler_cb, &_e_apps_hash_idler);
   return 1;
}

static Evas_Bool 
_e_apps_hash_cb_init(Evas_Hash *hash, const char *key, void *data, void *fdata)
{
   E_App *a;

   a = data;
   /* Link it in to all if needed. */
   if ((!a->parent) && (_e_apps_all) && (a != _e_apps_all) /*&& (strncmp(a->path, _e_apps_path_all, strlen(_e_apps_path_all)) == 0)*/)
     {
        a->parent = _e_apps_all;
        _e_apps_all->subapps = evas_list_prepend(_e_apps_all->subapps, a);
        e_object_ref(E_OBJECT(a));
     }
   return 1;
}

static int
_e_apps_hash_idler_cb(void *data)
{
   struct _E_App_Hash_Idler *idler;

   idler = data;
   idler->all_done = 1;
   evas_hash_foreach(_e_apps_every_app, _e_apps_hash_idler_cb_init, idler);
   if (idler->all_done)
     {
        printf("IDLE APP FILLING SCAN %3.3f\n", ecore_time_get() - idler->begin);
	idler->idler = NULL;
        return 0;
     }
   return 1;
}

static Evas_Bool 
_e_apps_hash_idler_cb_init(Evas_Hash *hash, const char *key, void *data, void *fdata)
{
   E_App *a;
   struct _E_App_Hash_Idler *idler;

   a = data;
   idler = fdata;
   E_OBJECT_CHECK(a);
   E_OBJECT_TYPE_CHECK(a, E_APP_TYPE);
   /* Either fill an E_App, or look for an icon of an already filled E_App.
    * Icon searching can take a long time, so don't do both at once. */
   if (!a->filled)
     {
        e_app_fields_fill(a, a->path);
	if (!a->filled) return 1;
	idler->all_done = 0;
	return 0;
     }
   else if ((!a->found_icon) && (!a->no_icon))
     {
        Evas_Object *o = NULL;

printf("IDLY SEARCHING AN FDO ICON FOR %s\n", a->path);
        if (idler->evas)
           o = e_app_icon_add(idler->evas, a);
        if (o)
	   evas_object_del(o);
	else
	   a->found_icon = 1;   /* Seems strange, but this stops it from looping infinitely. */
	idler->all_done = 0;
	return 0;
     }
   return 1;
}


EAPI int
e_app_shutdown(void)
{
   if (_e_apps_hash_idler.idler)
     {
        ecore_idler_del(_e_apps_hash_idler.idler);
	_e_apps_hash_idler.idler = NULL;
     }
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
	for (l = _e_apps_all->subapps; l; l = l->next)
	  {
	     E_App *a;
	     a = l->data;
	     printf("BUG: References %d %s\n", E_OBJECT(a)->references, a->path);
	  }
     }
   evas_hash_free(_e_apps_every_app);
   _e_apps_every_app = NULL;
   return 1;
}


EAPI void
e_app_unmonitor_all(void)
{
   Evas_List *l;
   
   for (l = _e_apps_all->subapps; l; l = l->next)
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

/* FIXME: Not actualy used anywhere, should we nuke it or should we use it everywhere that an E_App is allocated? */
EAPI E_App *
e_app_raw_new(void)
{
   E_App *a;
   
   a = E_OBJECT_ALLOC(E_App, E_APP_TYPE, _e_app_free);
   return a;
}

EAPI E_App *
e_app_new(const char *path, int scan_subdirs)
{
   E_App *a;
   struct stat         st;
   int                 stated = 0;
   int                 new_app = 0;
   char buf[PATH_MAX];

   if (!path)   return NULL;

   a = e_app_path_find(path);
   /* Is it a virtual path from inside a .order file? */
   if ((!a) && (!ecore_file_exists(path)))
     {
	snprintf(buf, sizeof(buf), "%s/%s", _e_apps_path_all, ecore_file_get_file(path));
	if (ecore_file_exists(path))
	  {
	     path = buf;
	     a = e_app_path_find(path);
	  }
     }

   /* Check if the cache is still valid. */
   if (a)
     {
	if (stat(a->path, &st) >= 0)
	  {
	     stated = 1;
	     if (st.st_mtime > a->mtime)
	       e_app_fields_empty(a);
	  }
	e_object_ref(E_OBJECT(a));
     }
   
   if ((!a) && (ecore_file_exists(path)))
     {
        /* Create it. */
        a = e_app_empty_new(path);
	new_app = 1;
     }

   if ((a) && (a->path))
      {
         if (ecore_file_is_dir(a->path))
	    {
	       if (!a->filled)
	         {
	            snprintf(buf, sizeof(buf), "%s/.directory.eap", path);
	            if (ecore_file_exists(buf))
		       e_app_fields_fill(a, buf);
	            else
	              {
		         a->name = evas_stringshare_add(ecore_file_get_file(a->path));
		         a->filled = 1;
		      }
		 }
	       if (!a->filled) goto error;
	       if (scan_subdirs)
	          {
		     if (stated)
		        _e_app_subdir_rescan(a);
		     else
		        e_app_subdir_scan(a, scan_subdirs);
		  }
		  
	       /* Don't monitor the all directory, all changes to that must go through e_app. */
               if ((!stated) && (strcmp(_e_apps_path_all, a->path) != 0))
		  a->monitor = ecore_file_monitor_add(a->path, _e_app_cb_monitor, a);
	    }
	 else if (_e_app_is_eapp(a->path))
	    {
	        if (!a->filled)
		   e_app_fields_fill(a, a->path);
                _e_apps_hash_cb_init(_e_apps_every_app, a->path, a, NULL);
		  
		/* no exe field.. not valid. drop it */
//		  if (!_e_app_exe_valid_get(a->exe))
//		     goto error;
	  }
	else
	  goto error;

	if (new_app)
	  _e_apps_every_app = evas_hash_direct_add(_e_apps_every_app, a->path, a);
	
	/* Timestamp the cache, and no need to stat the file twice if the cache was stale. */
	if ((stated) || (stat(a->path, &st) >= 0))
	  {
	     a->mtime = st.st_mtime;
	     stated = 1;
	  }
	if ((_e_apps_all) && (_e_apps_all->subapps))
	  {
	     _e_apps_all->subapps = evas_list_remove(_e_apps_all->subapps, a);
	     _e_apps_all->subapps = evas_list_prepend(_e_apps_all->subapps, a);
	  }
     }
   else
     goto error;
   
   return a;
   
   error:
   if (a)
     {
	if (a->monitor) ecore_file_monitor_del(a->monitor);
	if (a->path) evas_stringshare_del(a->path);
	e_app_fields_empty(a);
	free(a);
     }
   return NULL;
}

EAPI E_App *
e_app_empty_new(const char *path)
{
   E_App *a;
   
   a = E_OBJECT_ALLOC(E_App, E_APP_TYPE, _e_app_free);
   if (!a) return NULL;
   /* no image for now */
   a->image = NULL;
   a->width = 0;
   a->height = 0;
   /* record the path, or make one up */
   if (path) a->path = evas_stringshare_add(path);   
   else if (_e_apps_path_all)
     {
	char buf[PATH_MAX];
	
	snprintf(buf, sizeof(buf), "%s/_new_app_%1.1f.desktop", _e_apps_path_all, ecore_time_get());
	a->path = evas_stringshare_add(buf);
     }
   printf("NEW APP %p %s\n", a, a->path);
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
   
   E_OBJECT_CHECK(a);
   E_OBJECT_TYPE_CHECK(a, E_APP_TYPE);
   /* FIXME: This is probably the wrong test. */
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

             _e_app_resolve_file_name(buf, sizeof(buf), a->path, s);
	     if (ecore_file_exists(buf))
	       {
		  a2 = e_app_new(buf, scan_subdirs);
		  if (a2)
		    {
		       a2->parent = a;
		       a->subapps = evas_list_append(a->subapps, a2);
                       e_object_ref(E_OBJECT(a2));
		    }
	       }
	     else
	       {
		  E_App *a3;
		  Evas_List *pl;

		  pl = _e_apps_repositories;
		  while ((!a2) && (pl))
		    {
                       _e_app_resolve_file_name(buf, sizeof(buf), (char *)pl->data, s);
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
                                 e_object_ref(E_OBJECT(a3));
				 a2->references = evas_list_append(a2->references, a3);
#if ! NO_APP_LIST
				 _e_apps_all->subapps = evas_list_prepend(_e_apps_all->subapps, a3);
#endif
			      }
			    else
			      e_object_del(E_OBJECT(a3));
			 }
		    }
	       }
	  }
	ecore_list_destroy(files);
     }
}

EAPI int
e_app_exec(E_App *a, int launch_id)
{
   Ecore_Exe *exe;
   E_App *original;
   E_App_Instance *inst;
   Evas_List *l;
   char *command;
   
   E_OBJECT_CHECK_RETURN(a, 0);
   E_OBJECT_TYPE_CHECK_RETURN(a, E_APP_TYPE, 0);
   if (!a->exe) return 0;

  /* no exe field, don't exe it. */
   if (!_e_app_exe_valid_get(a->exe))
     return 0;
   
   /* FIXME: set up locale, encoding and input method env vars if they are in
    * the eapp file */
   inst = E_NEW(E_App_Instance, 1);
   if (!inst) return 0;
   
   if (a->orig)
     original = a->orig;
   else
     original = a;
   
   if (a->desktop)
     command = ecore_desktop_get_command(a->desktop, NULL, 1);
   else
     command = strdup(a->exe);
   if (!command)
     {
	free(inst);
	e_util_dialog_show(_("Run Error"),
			   _("Enlightenment was unable to process a command line:<br>"
			     "<br>"
			     "%s %s<br>"),
			   a->exe, (a->exe_params != NULL) ? a->exe_params : "" );
	return 0;
     }
   /* We want the stdout and stderr as lines for the error dialog if it exits abnormally. */
   exe = ecore_exe_pipe_run(command, ECORE_EXE_PIPE_AUTO | ECORE_EXE_PIPE_READ | ECORE_EXE_PIPE_ERROR | ECORE_EXE_PIPE_READ_LINE_BUFFERED | ECORE_EXE_PIPE_ERROR_LINE_BUFFERED, inst);
   if (!exe)
     {
	free(command);
	free(inst);
	e_util_dialog_show(_("Run Error"),
			   _("Enlightenment was unable to fork a child process:<br>"
			     "<br>"
			     "%s %s<br>"),
			   a->exe, (a->exe_params != NULL) ? a->exe_params : "" );
	return 0;
     }
   /* 20 lines at start and end, 20x100 limit on bytes at each end. */
   ecore_exe_auto_limits_set(exe, 2000, 2000, 20, 20);
   ecore_exe_tag_set(exe, "E/app");
   inst->app = original;
   inst->exe = exe;
   inst->launch_id = launch_id;
   inst->launch_time = ecore_time_get();
   inst->expire_timer = ecore_timer_add(10.0, _e_app_cb_expire_timer, inst);
   
   _e_apps_all->subapps = evas_list_remove(_e_apps_all->subapps, original);
   _e_apps_all->subapps = evas_list_prepend(_e_apps_all->subapps, original);

   original->instances = evas_list_append(original->instances, inst);
   _e_apps_start_pending = evas_list_append(_e_apps_start_pending, original);
   if (original->startup_notify) original->starting = 1;
   for (l = original->references; l; l = l->next)
     {
	E_App *a2;
	
	a2 = l->data;
	_e_app_change(a2, E_APP_EXEC);
     }
   _e_app_change(original, E_APP_EXEC);
   free(command);
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
   if (a->orig)
     a = a->orig;
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
   snprintf(buf, sizeof(buf), "%s/.eap.cache.cfg", parent->path);
   ecore_file_unlink(buf);
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
   snprintf(buf, sizeof(buf), "%s/.eap.cache.cfg", parent->path);
   ecore_file_unlink(buf);
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
	snprintf(buf, sizeof(buf), "%s/.eap.cache.cfg", _e_apps_path_all);
	ecore_file_unlink(buf);
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
   e_object_ref(E_OBJECT(add));
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
   e_object_ref(E_OBJECT(add));
   add->parent = parent;

   /* Check if this app is in the trash */
   if (!strncmp(add->path, _e_apps_path_trash, strlen(_e_apps_path_trash)))
     {
	/* Move to all */
	snprintf(buf, sizeof(buf), "%s/%s", _e_apps_path_all, ecore_file_get_file(add->path));
	if (ecore_file_exists(buf))
	  snprintf(buf, sizeof(buf), "%s/%s", parent->path, ecore_file_get_file(add->path));
	ecore_file_mv(add->path, buf);
        _e_apps_every_app = evas_hash_del(_e_apps_every_app, add->path, add);
	evas_stringshare_del(add->path);
	add->path = evas_stringshare_add(buf);
        _e_apps_every_app = evas_hash_direct_add(_e_apps_every_app, add->path, add);
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
	snprintf(buf, sizeof(buf), "%s/.eap.cache.cfg", before->parent->path);
	ecore_file_unlink(buf);
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
	snprintf(buf, sizeof(buf), "%s/.eap.cache.cfg", parent->path);
	ecore_file_unlink(buf);
     }
}

EAPI void
e_app_remove(E_App *a)
{
   Evas_List *l;
   char buf[PATH_MAX];

   if (!a) return;
   E_OBJECT_CHECK(a);
   E_OBJECT_TYPE_CHECK(a, E_APP_TYPE);
   if (!a->parent) return;

   a->parent->subapps = evas_list_remove(a->parent->subapps, a);
   /* Check if this app is in a repository or in the parents dir */
   snprintf(buf, sizeof(buf), "%s/%s", a->parent->path, ecore_file_get_file(a->path));
   if (ecore_file_exists(buf))
     {
	/* Move to trash */
	snprintf(buf, sizeof(buf), "%s/%s", _e_apps_path_trash, ecore_file_get_file(a->path));
	ecore_file_mv(a->path, buf);
        _e_apps_every_app = evas_hash_del(_e_apps_every_app, a->path, a);
	evas_stringshare_del(a->path);
	a->path = evas_stringshare_add(buf);
        _e_apps_every_app = evas_hash_direct_add(_e_apps_every_app, a->path, a);
     }
   _e_app_save_order(a->parent);
   for (l = a->references; l; l = l->next)
     {
	E_App *a2;
	
	a2 = l->data;
	e_app_remove(a2);
     }
   snprintf(buf, sizeof(buf), "%s/.eap.cache.cfg", a->parent->path);
   ecore_file_unlink(buf);
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

/* Used by e_border and ibar. */
EAPI E_App *
e_app_launch_id_pid_find(int launch_id, pid_t pid)
{
   Evas_List *l, *ll;
   
   for (l = _e_apps_all->subapps; l; l = l->next)
     {
	E_App *a;

	a = l->data;
        E_OBJECT_CHECK_RETURN(a, NULL);
        E_OBJECT_TYPE_CHECK_RETURN(a, E_APP_TYPE, NULL);
	/* No need to fill up unfilled E_Apps during this scan. */
	for (ll = a->instances; ll; ll = ll->next)
	  {
	     E_App_Instance *ai;
	     
	     ai = ll->data;
	     if (((launch_id > 0) && (ai->launch_id > 0) && (ai->launch_id == launch_id)) ||
		 ((pid > 1) && (ai->exe) && (ecore_exe_pid_get(ai->exe) == pid)))
	       {
		  _e_apps_all->subapps = evas_list_remove_list(_e_apps_all->subapps, l);
		  _e_apps_all->subapps = evas_list_prepend(_e_apps_all->subapps, a);
		  return a;
	       }
	  }
     }
   return NULL;
}

/* Used by e_border and ibar. */
EAPI E_App *
e_app_border_find(E_Border *bd)
{
   Evas_List *l, *l_match = NULL;
   int ok, match = 0;
   E_App *a, *a_match = NULL;
   char *title;

   if ((!bd->client.icccm.name) && (!bd->client.icccm.class) &&
       (!bd->client.icccm.title) && (bd->client.netwm.name) &&
       (!bd->client.icccm.window_role) && (!bd->client.icccm.command.argv))
     return NULL;

   title = bd->client.netwm.name;
   if (!title) title = bd->client.icccm.title;
   for (l = _e_apps_all->subapps; l; l = l->next)
     {
	a = l->data;
        E_OBJECT_CHECK_RETURN(a, NULL);
        E_OBJECT_TYPE_CHECK_RETURN(a, E_APP_TYPE, NULL);
        if (!a->filled) e_app_fields_fill(a, a->path);
	if (!a->filled) continue;
	ok = 0;
	if ((a->win_name) || (a->win_class) || (a->win_title) || (a->win_role) || (a->exe))
	  {
	     if ((a->win_name) && (a->win_class) && 
		 (bd->client.icccm.name) && (bd->client.icccm.class))
	       {
		  if ((e_util_glob_match(bd->client.icccm.name, a->win_name)) &&
		      (e_util_glob_match(bd->client.icccm.class, a->win_class)))
		    ok += 2;
	       }
	     else if ((a->win_class) && (bd->client.icccm.class))
	       {
		  if (e_util_glob_match(bd->client.icccm.class, a->win_class))
		    ok += 2;
	       }
	     
	     if (//(!a->win_title) ||
		 ((a->win_title) && (title) && (e_util_glob_match(title, a->win_title))))
	       ok++;
	     if (//(!a->win_role) ||
		 ((a->win_role) && (bd->client.icccm.window_role) && (e_util_glob_match(bd->client.icccm.window_role, a->win_role))))
	       ok++;
	     if (
		 (a->exe) && (bd->client.icccm.command.argc) && (bd->client.icccm.command.argv))
	       {
		  char *ts, *p;
		  
		  ts = alloca(strlen(a->exe) + 1);
		  strcpy(ts, a->exe);
		  p = ts;
		  while (*p)
		    {
		       if (isspace(*p))
			 {
			    *p = 0;
			    break;
			 }
		       p++;
		    }
		  if (!strcmp(ts, bd->client.icccm.command.argv[0]))
		    ok++;
	       }
	  }
	if (ok > match)
	  {
	     match = ok;
	     a_match = a;
	     l_match = l;
	  }
     }
   if ((a_match) && (l_match))
     {
	_e_apps_all->subapps = evas_list_remove_list(_e_apps_all->subapps, l_match);
	_e_apps_all->subapps = evas_list_prepend(_e_apps_all->subapps, a_match);
     }
   return a_match;
}

/* Used by e_actions. */
EAPI E_App *
e_app_file_find(const char *file)
{
   Evas_List *l;
   
   if (!file) return NULL;

   for (l = _e_apps_all->subapps; l; l = l->next)
     {
	E_App *a;
	
	a = l->data;
        E_OBJECT_CHECK_RETURN(a, NULL);
        E_OBJECT_TYPE_CHECK_RETURN(a, E_APP_TYPE, NULL);
	if (a->path)
	  {
	     char *p;
	     
	     p = strchr(a->path, '/');
	     if (p)
	       {
		  p++;
		  if (!strcmp(p, file))
		    {
		       _e_apps_all->subapps = evas_list_remove_list(_e_apps_all->subapps, l);
		       _e_apps_all->subapps = evas_list_prepend(_e_apps_all->subapps, a);
		       return a;
		    }
	       }
	  }
     }
   return NULL;
}

/* Used by e_apps. */
EAPI E_App *
e_app_path_find(const char *path)
{
   E_App *a = NULL;

   if ((path) && (_e_apps_every_app))
     a = evas_hash_find(_e_apps_every_app, path);
   return a;
}

/* Used by e_actions and e_exebuf. */
EAPI E_App *
e_app_name_find(const char *name)
{
   Evas_List *l;
   
   if (!name) return NULL;

   for (l = _e_apps_all->subapps; l; l = l->next)
     {
	E_App *a;
	
	a = l->data;
        E_OBJECT_CHECK_RETURN(a, NULL);
        E_OBJECT_TYPE_CHECK_RETURN(a, E_APP_TYPE, NULL);
        if (!a->filled)
	  e_app_fields_fill(a, a->path);
	if (!a->filled) continue;
	if (a->name)
	  {
	     if (!strcasecmp(a->name, name))
	       {
		  _e_apps_all->subapps = evas_list_remove_list(_e_apps_all->subapps, l);
		  _e_apps_all->subapps = evas_list_prepend(_e_apps_all->subapps, a);
		  return a;
	       }
	  }
     }
   return NULL;
}

/* Used by e_actions and e_exebuf. */
EAPI E_App *
e_app_generic_find(const char *generic)
{
   Evas_List *l;
   
   if (!generic) return NULL;

   for (l = _e_apps_all->subapps; l; l = l->next)
     {
	E_App *a;
	
	a = l->data;
        E_OBJECT_CHECK_RETURN(a, NULL);
        E_OBJECT_TYPE_CHECK_RETURN(a, E_APP_TYPE, NULL);
        if (!a->filled)
	  e_app_fields_fill(a, a->path);
	if (!a->filled) continue;
	if (a->generic)
	  {
	     if (!strcasecmp(a->generic, generic))
	       {
		  _e_apps_all->subapps = evas_list_remove_list(_e_apps_all->subapps, l);
		  _e_apps_all->subapps = evas_list_prepend(_e_apps_all->subapps, a);
		  return a;
	       }
	  }
     }
   return NULL;
}

/* Used by e_actions, e_exebuf, and e_zone. */
EAPI E_App *
e_app_exe_find(const char *exe)
{
   Evas_List *l;
   
   if (!exe) return NULL;

   for (l = _e_apps_all->subapps; l; l = l->next)
     {
	E_App *a;
	
	a = l->data;
        E_OBJECT_CHECK_RETURN(a, NULL);
        E_OBJECT_TYPE_CHECK_RETURN(a, E_APP_TYPE, NULL);
        if (!a->filled)
	  e_app_fields_fill(a, a->path);
	if (!a->filled) continue;
	if (a->exe)
	  {
	     if (!strcmp(a->exe, exe))
	       {
		  _e_apps_all->subapps = evas_list_remove_list(_e_apps_all->subapps, l);
		  _e_apps_all->subapps = evas_list_prepend(_e_apps_all->subapps, a);
		  return a;
	       }
	  }
     }
   return NULL;
}



/* Used e_exebuf. */
EAPI Evas_List *
e_app_name_glob_list(const char *name)
{
   Evas_List *l, *list = NULL;
   
   if (!name) return NULL;

   for (l = _e_apps_all->subapps; l; l = l->next)
     {
	E_App *a;
	
	a = l->data;
        E_OBJECT_CHECK_RETURN(a, NULL);
        E_OBJECT_TYPE_CHECK_RETURN(a, E_APP_TYPE, NULL);
        if (!a->filled)
	  e_app_fields_fill(a, a->path);
	if (!a->filled) continue;
	if (a->name)
	  {
	     if (e_util_glob_case_match(a->name, name))
	       list = evas_list_append(list, a);
	  }
     }
   return list;
}

/* Used e_exebuf. */
EAPI Evas_List *
e_app_generic_glob_list(const char *generic)
{
   Evas_List *l, *list = NULL;
   
   if (!generic) return NULL;

   for (l = _e_apps_all->subapps; l; l = l->next)
     {
	E_App *a;
	
	a = l->data;
        E_OBJECT_CHECK_RETURN(a, NULL);
        E_OBJECT_TYPE_CHECK_RETURN(a, E_APP_TYPE, NULL);
        if (!a->filled)
	  e_app_fields_fill(a, a->path);
	if (!a->filled) continue;
	if (a->generic)
	  {
	     if (e_util_glob_case_match(a->generic, generic))
	       list = evas_list_append(list, a);
	  }
     }
   return list;
}

/* Used e_exebuf. */
EAPI Evas_List *
e_app_exe_glob_list(const char *exe)
{
   Evas_List *l, *list = NULL;
   
   if (!exe) return NULL;

   for (l = _e_apps_all->subapps; l; l = l->next)
     {
	E_App *a;
	
	a = l->data;
        E_OBJECT_CHECK_RETURN(a, NULL);
        E_OBJECT_TYPE_CHECK_RETURN(a, E_APP_TYPE, NULL);
        if (!a->filled)
	  e_app_fields_fill(a, a->path);
	if (!a->filled) continue;
	if (a->exe)
	  {
	     if (e_util_glob_match(a->exe, exe))
	       list = evas_list_append(list, a);
	  }
     }
   return list;
}

/* Used e_exebuf. */
EAPI Evas_List *
e_app_comment_glob_list(const char *comment)
{
   Evas_List *l, *list = NULL;
   
   if (!comment) return NULL;

   for (l = _e_apps_all->subapps; l; l = l->next)
     {
	E_App *a;
	
	a = l->data;
        E_OBJECT_CHECK_RETURN(a, NULL);
        E_OBJECT_TYPE_CHECK_RETURN(a, E_APP_TYPE, NULL);
        if (!a->filled)
	  e_app_fields_fill(a, a->path);
	if (!a->filled)
	  continue;
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
   char *str, *v;
   const char *lang, *ext;
   int size;
   
   /* get our current language */
   lang = e_intl_language_alias_get();
   
   /* if its "C" its the default - so drop it */
   if (!strcmp(lang, "C"))
     {
	lang = NULL;
     }
   if (!path) path = a->path;
   if (!path) return;

   ext = strrchr(path, '.');
   if ((ext) && (strcmp(ext, ".desktop") == 0))
     {   /* It's a .desktop file. */
	Ecore_Desktop *desktop;
	
	desktop = ecore_desktop_get(path, lang);
	if (desktop)
	  {
	     a->desktop = desktop;
	     
	     if (desktop->name)  a->name = evas_stringshare_add(desktop->name);
	     if (desktop->generic)  a->generic = evas_stringshare_add(desktop->generic);
	     if (desktop->comment)  a->comment = evas_stringshare_add(desktop->comment);
	     
	     if (desktop->exec)  a->exe = evas_stringshare_add(desktop->exec);
	     if (desktop->exec_params)  a->exe_params = evas_stringshare_add(desktop->exec_params);
	     if (desktop->icon)  a->icon = evas_stringshare_add(desktop->icon);
	     if (desktop->icon_theme)  a->icon_theme = evas_stringshare_add(desktop->icon_theme);
	     if (desktop->icon_class)  a->icon_class = evas_stringshare_add(desktop->icon_class);
	     if (desktop->icon_path)  a->icon_path = evas_stringshare_add(desktop->icon_path);
	     if (desktop->window_name)  a->win_name = evas_stringshare_add(desktop->window_name);
	     if (desktop->window_class)  a->win_class = evas_stringshare_add(desktop->window_class);
	     if (desktop->window_title)  a->win_title = evas_stringshare_add(desktop->window_title);
	     if (desktop->window_role)  a->win_role = evas_stringshare_add(desktop->window_role);
	     a->icon_time = desktop->icon_time;
	     a->startup_notify = desktop->startup;
	     a->wait_exit = desktop->wait_exit;
	     a->hard_icon = desktop->hard_icon;
	     a->dirty_icon = 0;
	     a->no_icon = 0;
	     a->found_icon = 0;

	     a->filled = 1;

//	   if (desktop->type)  a->type = evas_stringshare_add(desktop->type);
//	   if (desktop->categories)  a->categories = evas_stringshare_add(desktop->categories);
	  }
     }
   else
     {   /* Must be an .eap file. */
	Eet_File *ef;

/* FIXME: This entire process seems inefficient, each of the strings gets duped then freed three times.
 * On the other hand, raster wants .eaps to go away, so no big deal.  B-)
 */

#define STORE_N_FREE(member) \
   if (v) \
      { \
         str = alloca(size + 1); \
	 memcpy(str, (v), size); \
	 str[size] = 0; \
	 a->member = evas_stringshare_add(str); \
	 free(v); \
      }
	
	ef = eet_open(path, EET_FILE_MODE_READ);
	if (!ef) return;
	
	v = _e_app_localized_val_get(ef, lang, "app/info/name", &size);
	STORE_N_FREE(name);
	v = _e_app_localized_val_get(ef, lang, "app/info/generic", &size);
	STORE_N_FREE(generic);
	v = _e_app_localized_val_get(ef, lang, "app/info/comment", &size);
	STORE_N_FREE(comment);
	
	v = eet_read(ef, "app/info/exe", &size);
	STORE_N_FREE(exe);
	v = eet_read(ef, "app/icon/class", &size);
	STORE_N_FREE(icon_class);
	v = eet_read(ef, "app/window/name", &size);
	STORE_N_FREE(win_name);
	v = eet_read(ef, "app/window/class", &size);
	STORE_N_FREE(win_class);
	v = eet_read(ef, "app/window/title", &size);
	STORE_N_FREE(win_title);
	v = eet_read(ef, "app/window/role", &size);
	STORE_N_FREE(win_role);
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
	a->filled = 1;
     }
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
   char buf[PATH_MAX];
   const char *lang, *ext = NULL;
   int new_eap = 0;

   E_OBJECT_CHECK(a);
   E_OBJECT_TYPE_CHECK(a, E_APP_TYPE);
   /* Check if it's a new one that has not been saved yet. */
   if (a->path)
      ext = ecore_file_get_file(a->path);
   if ( (!a->path) || ((strncmp(ext, "_new_app_", 9) == 0) && (!ecore_file_exists(a->path))) )
      {
         snprintf(buf, sizeof(buf), "%s/%s.desktop", _e_apps_path_all, a->name);
	 if (a->path)  evas_stringshare_del(a->path);
	 a->path = evas_stringshare_add(buf);
      }
   if (!a->path)  return;
   /* This still lets old ones that are not in all to be saved, but new ones are forced to be in all. */
   if (!ecore_file_exists(a->path))
      {
         /* Force it to be in all. */
         snprintf(buf, sizeof(buf), "%s/%s", _e_apps_path_all, ecore_file_get_file(a->path));
	 if (a->path)  evas_stringshare_del(a->path);
	 a->path = evas_stringshare_add(buf);
      }
   if (!a->path)  return;
   if (ecore_file_exists(a->path))
      _e_apps_every_app = evas_hash_del(_e_apps_every_app, a->path, a);
   else
      new_eap = 1;

   ext = strrchr(a->path, '.');
   if ((ext) && (strcmp(ext, ".desktop") == 0))
      {   /* It's a .desktop file. */
         Ecore_Desktop *desktop;
	 int created = 0;

         desktop = ecore_desktop_get(a->path, NULL);
	 if (!desktop)
	    {
	       desktop = E_NEW(Ecore_Desktop, 1);
	       desktop->original_path = strdup(a->path);
	       created = 1;
	    }
	 if (desktop)
	    {
	       desktop->name = (char *) a->name;
	       desktop->generic = (char *) a->generic;
	       desktop->comment = (char *) a->comment;

	       desktop->exec = (char *) a->exe;
	       desktop->exec_params = (char *) a->exe_params;
	       desktop->icon = (char *) a->icon;
	       desktop->icon_theme = (char *) a->icon_theme;
	       desktop->icon_class = (char *) a->icon_class;
               desktop->icon_path = (char *) a->icon_path;
	       desktop->window_name = (char *) a->win_name;
	       desktop->window_class = (char *) a->win_class;
	       desktop->window_title = (char *) a->win_title;
	       desktop->window_role = (char *) a->win_role;
	       desktop->icon_time = a->icon_time;
	       desktop->hard_icon = a->hard_icon;
	       desktop->startup = a->startup_notify;
	       desktop->wait_exit = a->wait_exit;

               desktop->type = "Application";
//               desktop.categories = a->categories;

               ecore_desktop_save(desktop);
               a->dirty_icon = 0;
	       if (created)
	          E_FREE(desktop);
	    }
      }
   else
      {   /* Must be an .eap file. */
         Eet_File *ef;
         unsigned char tmp[1];
//         int img;
//         if ((!a->path) || (!ecore_file_exists(a->path)))
//           {
	      _e_app_new_save(a);
//	      img = 0;
//           }
//         else
//           img = 1;

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
      }

   _e_apps_every_app = evas_hash_direct_add(_e_apps_every_app, a->path, a);
   if (new_eap)
      {
         /* Careful, if this is being created from the border icon, this E_App is already part of the border. */
         a->parent = _e_apps_all;
         _e_apps_all->subapps = evas_list_append(_e_apps_all->subapps, a);
         e_object_ref(E_OBJECT(a));
	 /* FIXME: Don't know if we need to copy and reference this since it is in the repository. */
	 _e_app_change(a, E_APP_ADD);
      }
   else
      _e_app_fields_save_others(a);
}

static void
_e_app_fields_save_others(E_App *a)
{
   Evas_List *l;
   char buf[PATH_MAX];

   for (l = a->references; l; l = l->next)
     {
        E_App *a2;
	     
	a2 = l->data;
	if (_e_app_copy(a2, a)) _e_app_change(a2, E_APP_CHANGE);
     }
   if (a->orig)
     {
	E_App *a2;
	     
	a2 = a->orig;
	if (_e_app_copy(a2, a)) _e_app_change(a2, E_APP_CHANGE);
     }
   _e_app_change(a, E_APP_CHANGE);
   if (a->parent)
     {
	snprintf(buf, sizeof(buf), "%s/.eap.cache.cfg", a->parent->path);
	ecore_file_unlink(buf);
	_e_app_change(a->parent, E_APP_CHANGE);
	/* I don't think we need to rescan.
	 * A) we should be changing all the relevant stuff here.
	 * B) monitoring will catch other changes.
	 * C) it's a waste of time.
	_e_app_subdir_rescan(a->parent);
	 */
     }
}


EAPI void
e_app_fields_empty(E_App *a)
{
   if (a->name) evas_stringshare_del(a->name);
   if (a->generic) evas_stringshare_del(a->generic);
   if (a->comment) evas_stringshare_del(a->comment);
   if (a->exe) evas_stringshare_del(a->exe);
   if (a->exe_params) evas_stringshare_del(a->exe_params);
   if (a->icon_theme) evas_stringshare_del(a->icon_theme);
   if (a->icon) evas_stringshare_del(a->icon);
   if (a->icon_class) evas_stringshare_del(a->icon_class);
   if (a->icon_path) evas_stringshare_del(a->icon_path);
   if (a->win_name) evas_stringshare_del(a->win_name);
   if (a->win_class) evas_stringshare_del(a->win_class);
   if (a->win_title) evas_stringshare_del(a->win_title);
   if (a->win_role) evas_stringshare_del(a->win_role);
   a->desktop = NULL;
   a->name = NULL;
   a->generic = NULL;
   a->comment = NULL;
   a->exe = NULL;
   a->exe_params = NULL;
   a->icon_theme = NULL;
   a->icon = NULL;
   a->icon_class = NULL;
   a->icon_path = NULL;
   a->win_name = NULL;
   a->win_class = NULL;
   a->win_title = NULL;
   a->win_role = NULL;
   a->dirty_icon = 0;
   a->hard_icon = 0;
   a->no_icon = 0;
   a->found_icon = 0;
   a->filled = 0;
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
	     if (ecore_file_get_file(file)[0] != '.')
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

static Evas_Object *
_e_app_icon_path_add(Evas *evas, E_App *a)
{
   Evas_Object *o;
   char *ext;

   o = e_icon_add(evas);
   ext = strrchr(a->icon_path, '.');
   if (ext)
      {
         if (strcmp(ext, ".edj") == 0)
            e_icon_file_edje_set(o, a->icon_path, "icon");
         else
            e_icon_file_set(o, a->icon_path);
      }
   else
      e_icon_file_set(o, a->icon_path);
   e_icon_fill_inside_set(o, 1);

   return o;
}


static void
_e_app_icon_path_add_to_menu_item(E_Menu_Item *mi, E_App *a)
{
   char *ext;

   ext = strrchr(a->icon_path, '.');
   if (ext)
      {
          if (strcmp(ext, ".edj") == 0)
             e_menu_item_icon_edje_set(mi, a->icon_path, "icon");
          else
             e_menu_item_icon_file_set(mi, a->icon_path);
      }
   else
      e_menu_item_icon_file_set(mi, a->icon_path);
}

static void
_e_app_fdo_icon_search(E_App *a)
{
   if ((!a->no_icon) && (a->icon_class))
     {
        char *v = NULL;
		  
	/* FIXME: Use a real icon size. */
	v = (char *)ecore_desktop_icon_find(a->icon_class, NULL, e_config->icon_theme);
	if (v)
	  {
             if (a->icon_path) evas_stringshare_del(a->icon_path);
	     a->icon_path = evas_stringshare_add(v);
	     if (e_config->icon_theme)
	       {
                  if (a->icon_theme) evas_stringshare_del(a->icon_theme);
		  a->icon_theme = evas_stringshare_add(e_config->icon_theme);
	       }
	     a->dirty_icon = 1;
	     free(v);
	  }
        else
           a->no_icon = 1;
        /* Copy the new found icon data to the original. */
        _e_app_fields_save_others(a);
      }
}

EAPI Evas_Object *
e_app_icon_add(Evas *evas, E_App *a)
{
   Evas_Object *o = NULL;
   int theme_match = 0;

#if DEBUG
printf("e_app_icon_add(%s)   %s   %s   %s\n", a->path, a->icon_class, e_config->icon_theme, a->icon_path);
#endif

   /* First check to see if the icon theme is different. */
   if ((e_config->icon_theme) && (a->icon_theme))
     {
        if (strcmp(e_config->icon_theme, a->icon_theme) == 0)
	   theme_match = 1;
     }
   else if ((e_config->icon_theme == NULL) && (a->icon_theme == NULL))
        theme_match = 1;

   /* If the icon was hard coded into the .desktop files Icon field, then theming doesn't matter. */
   if (a->hard_icon)
      theme_match = 1;

   /* Then check if we already know the icon path. */
   if ((theme_match) && (a->icon_path) && (a->icon_path[0] != 0))
     o = _e_app_icon_path_add(evas, a);
   else
     {
	o = edje_object_add(evas);
	/* Check the theme for icons. */
	if (!e_util_edje_icon_list_set(o, a->icon_class))
	  {
	     if (edje_object_file_set(o, a->path, "icon"))
	       {
		  a->found_icon = 1;
	       }
	     else /* If that fails, then this might be an FDO icon. */
                _e_app_fdo_icon_search(a);
	     
	     if (a->icon_path)
	       {
		  /* Free the aborted object first. */
		  if (o)   evas_object_del(o);
		  o = _e_app_icon_path_add(evas, a);
	       }
	  }
     }
   if (o)
      a->found_icon = 1;
   return o;
}

/* Search order? -
 *
 * fixed path to icon
 * an .edj icon in ~/.e/e/icons
 * icon_class in theme
 * icon from a->path in theme
 * FDO search for icon_class
 */

EAPI void
e_app_icon_add_to_menu_item(E_Menu_Item *mi, E_App *a)
{
   int theme_match = 0;

   mi->app = a;
   /* e_menu_item_icon_edje_set() just tucks away the params, the actual call to edje_object_file_set() happens later. */
   /* e_menu_item_icon_file_set() just tucks away the params, the actual call to e_icon_add() happens later. */

#if DEBUG
printf("e_app_icon_add_to_menu_item(%s)   %s   %s   %s\n", a->path, a->icon_class, e_config->icon_theme, a->icon_path);
#endif
   /* First check to see if the icon theme is different. */
   if ((e_config->icon_theme) && (a->icon_theme))
     {
        if (strcmp(e_config->icon_theme, a->icon_theme) == 0)
	   theme_match = 1;
     }
   else if ((e_config->icon_theme == NULL) && (a->icon_theme == NULL))
        theme_match = 1;

   /* If the icon was hard coded into the .desktop files Icon field, then theming doesn't matter. */
   if (a->hard_icon)
      theme_match = 1;

   /* Then check if we already know the icon path. */
   if ((theme_match) && (a->icon_path) && (a->icon_path[0] != 0))
     {
         _e_app_icon_path_add_to_menu_item(mi, a);
         a->found_icon = 1;
     }
   else
      {
	/* Check the theme for icons. */
         if (e_util_menu_item_edje_icon_list_set(mi, a->icon_class))
	    {
               a->found_icon = 1;
	    }
	 else
	    {
               if (edje_file_group_exists(a->path, "icon"))
	         {
                    e_menu_item_icon_edje_set(mi, a->path, "icon");
                    a->found_icon = 1;
		 }
               else /* If that fails, then this might be an FDO icon. */
                  _e_app_fdo_icon_search(a);

               if (a->icon_path)
	         {
                    _e_app_icon_path_add_to_menu_item(mi, a);
                    a->found_icon = 1;
		 }
	    }
      }
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
   
   if (a->parent)
     {
	char buf[PATH_MAX];
	
	snprintf(buf, sizeof(buf), "%s/.eap.cache.cfg", a->parent->path);
	ecore_file_unlink(buf);
     }
   ecore_file_unlink(tmpn);
   return 1;   
}

static void
_e_app_free(E_App *a)
{
   if (a->path)
     _e_apps_every_app = evas_hash_del(_e_apps_every_app, a->path, a);

   /* FIXME: natsy - but trying to fix a bug */
   /* somehow apps for "startup" end up in subapps of _e_apps_all and
    * when thet startup app aprent is freed the subapps of the startup are
    * freed but the list entries for _e_apps_all->subapps is still there
    * and so u have a nasty dangling pointer to garbage memory
    */
   if (_e_apps_all)
     _e_apps_all->subapps = evas_list_remove(_e_apps_all->subapps, a);
   /* END FIXME */
   
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
	  {
	     a->parent->subapps = evas_list_remove(a->parent->subapps, a);
	  }
	a->orig->references = evas_list_remove(a->orig->references, a);
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
		  ecore_exe_tag_set(inst->exe, NULL);
//		  ecore_exe_free(inst->exe);
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
	e_app_fields_empty(a);
	if (a->path)
	  evas_stringshare_del(a->path);
	free(a);
     }
}

/* Used by _e_app_cb_monitor and _e_app_subdir_rescan */ 
static E_App *
_e_app_subapp_file_find(E_App *a, const char *file)
{
   Evas_List *l;
   
   for (l = a->subapps; l; l = l->next)
     {
	E_App *a2;
	
	a2 = l->data;
        E_OBJECT_CHECK_RETURN(a2, NULL);
        E_OBJECT_TYPE_CHECK_RETURN(a2, E_APP_TYPE, NULL);
	if ((a2->deleted) || ((a2->orig) && (a2->orig->deleted))) continue;
	if (!strcmp(ecore_file_get_file(a2->path), ecore_file_get_file(file))) return a2;
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
     printf("APP_CHANGE %p %s\n", a, a->path);
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

#if DEBUG
   _e_app_print(path, event);
#endif

   file = (char *)ecore_file_get_file(path);
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
	     if (app->filled)
	       {
		  _e_app_change(app, E_APP_CHANGE);
		  for (l = app->references; l; l = l->next)
		    {
		       E_App *a2;
		       
		       a2 = l->data;
		       if (_e_app_copy(a2, app)) _e_app_change(a2, E_APP_CHANGE);
		    }
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
		  if (a->filled)
		    {
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
	  }
	else if ((event == ECORE_FILE_EVENT_CREATED_FILE) ||
		 (event == ECORE_FILE_EVENT_CREATED_DIRECTORY))
	  {
	     _e_app_subdir_rescan(app);
	     /* If this is the all app, check if someone wants to reference this app */
	     if (app == _e_apps_all)
	       _e_app_check_order(file);

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
		  e_object_ref(E_OBJECT(a2));
	       }
	     else
	       {
		  /* If we still haven't found it, it is new! */
                  _e_app_resolve_file_name(buf, sizeof(buf), app->path, s);
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
		       e_object_ref(E_OBJECT(a2));
		    }
		  else
		    {
		       /* We ask for a reference! */
		       Evas_List *pl;
		       E_App *a3;

		       pl = _e_apps_repositories;
		       while ((!a2) && (pl))
			 {
                            _e_app_resolve_file_name(buf, sizeof(buf), (char *)pl->data, s);
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
		                      e_object_ref(E_OBJECT(a3));
				      a2->references = evas_list_append(a2->references, a3);
#if ! NO_APP_LIST
				      _e_apps_all->subapps = evas_list_prepend(_e_apps_all->subapps, a3);
#endif
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
               e_object_unref(E_OBJECT(a2));
	       a2 = NULL;
	       break;
	    }
	if (a2)
	  {
	     a2->deleted = 1;
	     ch = E_NEW(E_App_Change_Info, 1);
	     ch->app = a2;
	     ch->change = E_APP_DEL;
	     e_object_ref(E_OBJECT(ch->app));
	     changes = evas_list_append(changes, ch);
             e_object_unref(E_OBJECT(a2));
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
   evas_list_free(changes);
}

static int
_e_app_is_eapp(const char *path)
{
   char *p;

   if (!path)
     return 0;
   p = strrchr(path, '.');
   if (!p)
     return 0;

   p++;
   if ((strcasecmp(p, "eap")) && (strcasecmp(p, "desktop")))
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
   dst->desktop = src->desktop;

   dst->name = src->name;
   dst->generic = src->generic;
   dst->comment = src->comment;
   dst->exe = src->exe;
   dst->exe_params = src->exe_params;
   dst->path = src->path;
   dst->win_name = src->win_name;
   dst->win_class = src->win_class;
   dst->win_title = src->win_title;
   dst->win_role = src->win_role;
   dst->icon_theme = src->icon_theme;
   dst->icon = src->icon;
   dst->icon_class = src->icon_class;
   dst->icon_path = src->icon_path;
   dst->startup_notify = src->startup_notify;
   dst->wait_exit = src->wait_exit;
   dst->icon_time = src->icon_time;
   dst->starting = src->starting;
   dst->scanned = src->scanned;
   dst->dirty_icon = src->dirty_icon;
   dst->hard_icon = src->hard_icon;
   dst->no_icon = src->no_icon;
   dst->filled = src->filled;

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
   if (app->parent)
     {
	char buf[PATH_MAX];
	
	snprintf(buf, sizeof(buf), "%s/.eap.cache.cfg", app->parent->path);
	ecore_file_unlink(buf);
     }
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
	
	dia = e_dialog_new(e_container_current_get(e_manager_current_get()), "E", "_app_run_error_dialog");
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
   a->instances = evas_list_remove(a->instances, ai);
   ai->exe = NULL;
   if (ai->expire_timer) ecore_timer_del(ai->expire_timer);
   free(ai);
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


static int
_e_app_exe_valid_get(const char *exe)
{
   if ((!exe) || (!exe[0])) return 0;
   return 1;
}

#if DEBUG
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
#endif

static void
_e_app_check_order(const char *file)
{
   Evas_List *l;

   for (l = _e_apps_all->subapps; l; l = l->next)
     {
	E_App *a;

	a = l->data;
	if (a->monitor)
	  {
	     /* This is a directory */
	     if (_e_app_order_contains(a, file))
	       _e_app_subdir_rescan(a);
	  }
     }
}

static int
_e_app_order_contains(E_App *a, const char *file)
{
   char buf[4096];
   int ret = 0;
   FILE *f;

   snprintf(buf, sizeof(buf), "%s/.order", a->path);
   if (!ecore_file_exists(buf)) return 0;
   f = fopen(buf, "rb");
   if (!f) return 0;

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
	     if (!strcmp(buf, file))
	       {
		  ret = 1;
		  break;
	       }
	  }
     }
   fclose(f);
   return ret;
}


static void
_e_app_resolve_file_name(char *buf, size_t size, const char *path, const char *file)
{
   size_t length;

   length = strlen(path);
   if (file[0] == '/')
      snprintf(buf, size, "%s", file);
   else if ((length > 0) && (path[length - 1] == '/'))
      snprintf(buf, size, "%s%s", path, file);
   else
      snprintf(buf, size, "%s/%s", path, file);
}
