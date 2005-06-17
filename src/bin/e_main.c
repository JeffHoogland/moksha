/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* local subsystem functions */
static void _e_main_shutdown_push(int (*func)(void));
static void _e_main_shutdown(int errorcode);

static int  _e_main_x_shutdown(void);
static int  _e_main_dirs_init(void);
static int  _e_main_dirs_shutdown(void);
static int  _e_main_screens_init(void);
static int  _e_main_screens_shutdown(void);
static int  _e_main_path_init(void);
static int  _e_main_path_shutdown(void);
static int  _e_main_ipc_init(void);
static int  _e_main_ipc_shutdown(void);

static void _e_main_cb_x_fatal(void *data);
static int  _e_main_cb_signal_exit(void *data, int ev_type, void *ev);
static int  _e_main_cb_signal_hup(void *data, int ev_type, void *ev);
static int  _e_main_cb_x_flusher(void *data);
static int  _e_main_cb_idler_before(void *data);
static int  _e_main_cb_idler_after(void *data);

static int  _e_main_cb_startup_fake_status(void *data);
static int  _e_main_cb_startup_fake_end(void *data);

static void _e_main_desk_save(void);
static void _e_main_desk_restore(E_Manager *man, E_Container *con);

E_Path *path_data    = NULL;
E_Path *path_images  = NULL;
E_Path *path_fonts   = NULL;
E_Path *path_themes  = NULL;
E_Path *path_init    = NULL;
E_Path *path_icons   = NULL;
E_Path *path_modules = NULL;
E_Path *path_backgrounds = NULL;
int     restart      = 0;
int     good         = 0;
int     evil         = 0;
int     starting     = 1;

/* local subsystem globals */
#define MAX_LEVEL 32
static int (*_e_main_shutdown_func[MAX_LEVEL]) (void);
static int _e_main_level = 0;

static Evas_List *_e_main_idler_before_list = NULL;

static Ecore_Idle_Enterer *_e_main_idle_enterer_before  = NULL;
static Ecore_Idle_Enterer *_e_main_idle_enterer_after   = NULL;
static Ecore_Idle_Enterer *_e_main_idle_enterer_flusher = NULL;

/* externally accessible functions */
int
main(int argc, char **argv)
{
   int ipc_failed = 0;
   int i;
   int nostartup = 0;
   int after_restart = 0; 
   char buf[1024];
   char *s;
   struct sigaction action;

   /* trap deadly bug signals and allow some form of sane recovery */
   /* or ability to gdb attach and debug at this point - better than your */
   /* wm/desktop vanishing and not knowing what happened */
   action.sa_sigaction = e_sigseg_act;
   action.sa_flags = SA_ONSTACK | SA_NODEFER | SA_RESETHAND | SA_SIGINFO;
   sigemptyset(&action.sa_mask);
   sigaction(SIGSEGV, &action, NULL);

   action.sa_sigaction = e_sigill_act;
   action.sa_flags = SA_ONSTACK | SA_NODEFER | SA_RESETHAND | SA_SIGINFO;
   sigemptyset(&action.sa_mask);
   sigaction(SIGILL, &action, NULL);

   action.sa_sigaction = e_sigfpe_act;
   action.sa_flags = SA_ONSTACK | SA_NODEFER | SA_RESETHAND | SA_SIGINFO; 
   sigemptyset(&action.sa_mask);
   sigaction(SIGFPE, &action, NULL);

   action.sa_sigaction = e_sigbus_act;
   action.sa_flags = SA_ONSTACK | SA_NODEFER | SA_RESETHAND | SA_SIGINFO;
   sigemptyset(&action.sa_mask);
   sigaction(SIGBUS, &action, NULL);

   /* FIXME: this is the init code for letting e be relocatable. right now
    * its not used - so i want to see if it can reliably determine its exe
    * prefix
    */
   if (!e_prefix_determine(argv[0]))
     {
	fprintf(stderr,
		"ERROR: Enlightenment cannot determine its installed\n"
		"       prefix from the system or argv[0].\n"
		"       This is because it is not on Linux AND has been\n"
		"       Executed strangely. This is unusal.\n"
		);
	exit(-1);
     }
   
   /* for debugging by redirecting stdout of e to a log file to tail */
   setvbuf(stdout, NULL, _IONBF, 0);
      
   if (getenv("NOSTARTUP")) nostartup = 1;
   if (getenv("RESTART")) after_restart = 1;
   
   e_util_env_set("RESTART", "1");

   e_intl_init();
   
   /* handle some command-line parameters */
   for (i = 1; i < argc; i++)
     {
	if ((!strcmp(argv[i], "-display")) && (i < (argc - 1)))
	  {
	     i++;
	     
	     e_util_env_set("DISPLAY", argv[i]);
	  }
	else if ((!strcmp(argv[i], "-fake-xinerama-screen")) && (i < (argc - 1)))
	  {
	     int x, y, w, h;
	     
	     i++;
	     /* WWxHH+XX+YY */
	     if (sscanf(argv[i], "%ix%i+%i+%i", &w, &h, &x, &y) == 4)
	       e_xinerama_fake_screen_add(x, y, w, h);
	  }
	else if (!strcmp(argv[i], "-good"))
	  {
	     good = 1;
	     evil = 0;
	  }
	else if (!strcmp(argv[i], "-evil"))
	  {
	     good = 0;
	     evil = 1;
	  }
	else if (!strcmp(argv[i], "-psychotic"))
	  {
	     good = 1;
	     evil = 1;
	  }
	else if ((!strcmp(argv[i], "-h")) ||
		 (!strcmp(argv[i], "-help")) ||
		 (!strcmp(argv[i], "--help")))
	  {
	     printf
	       (_(
		  "Options:\n"
		  "\t-display DISPLAY\n"
		  "\t\tConnect to display named DISPLAY.\n"
		  "\t\tEG: -display :1.0\n"
		  "\t-fake-xinerama-screen WxH+X+Y\n"
		  "\t\tAdd a FAKE xinerama screen (instead of the real ones)\n"
		  "\t\tgiven the geometry. Add as many as you like. They all\n"
		  "\t\treplace the real xinerama screens, if any. This can\n"
		  "\t\tbe used to simulate xinerama.\n"
		  "\t\tEG: -fake-xinerama-screen 800x600+0+0 -fake-xinerama-screen 800x600+800+0\n"
		  "\t-good\n"
		  "\t\tBe good.\n"
		  "\t-evil\n"
		  "\t\tBe evil.\n"
		  "\t-psychotic\n"
		  "\t\tBe psychotic.\n"
		  )
		);
	     exit(0);
	  }
     }

   /* fix up DISPLAY to be :N.0 if no .screen is in it */
   s = getenv("DISPLAY");
   if (s)
     {
	char *p;
	
	p = strrchr(s, ':');
	if (!p)
	  {
	     snprintf(buf, sizeof(buf), "%s:0.0", s);
	     e_util_env_set("DISPLAY", buf);
	  }
	else
	  {
	     p = strrchr(p, '.');
	     if (!p)
	       {
		  snprintf(buf, sizeof(buf), "%s.0", s);
		  e_util_env_set("DISPLAY", buf);
	       }
	  }
     }
   
   /* init edje and set it up in frozen mode */
   edje_init();
   edje_freeze();
   
   /* basic ecore init */
   if (!ecore_init())
     {
	e_error_message_show(_("Enlightenment cannot Initialize Ecore!\n"
			       "Perhaps you are out of memory?"));
	exit(-1);
     }
   _e_main_shutdown_push(ecore_shutdown);
   /* setup my args */
   ecore_app_args_set(argc, (const char **)argv);
   /* setup a handler for when e is asked to exit via a system signal */
   if (!ecore_event_handler_add(ECORE_EVENT_SIGNAL_EXIT, _e_main_cb_signal_exit, NULL))
     {
	e_error_message_show(_("Enlightenment cannot set up an exit signal handler.\n"
			       "Perhaps you are out of memory?"));
	_e_main_shutdown(-1);
     }
   if(!ecore_event_handler_add(ECORE_EVENT_SIGNAL_HUP, _e_main_cb_signal_hup, NULL))
     {
	e_error_message_show(_("Enlightenment cannot set up a HUP signal handler.\n"
			       "Perhaps you are out of memory?"));
	_e_main_shutdown(-1);
     }

   /* an idle enterer to be called before all others */
   _e_main_idle_enterer_before = ecore_idle_enterer_add(_e_main_cb_idler_before, NULL);
   
   /* init x */
   if (!ecore_x_init(NULL))
     {
	e_error_message_show(_("Enlightenment cannot initialize its X connection.\n"
			       "Have you set your DISPLAY variable?"));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(_e_main_x_shutdown);
   /* init x */
   if (!e_alert_init(NULL))
     {
	e_error_message_show(_("Enlightenment cannot initialize its emergency alert system.\n"
			       "Have you set your DISPLAY variable?"));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(e_alert_shutdown);
   if (!e_xinerama_init())
     {
	e_error_message_show(_("Enlightenment cannot setup xinerama wrapping.\n"
			       "This should not happen."));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(e_xinerama_shutdown);
   
//   ecore_x_grab();
   
   ecore_x_io_error_handler_set(_e_main_cb_x_fatal, NULL);

   /* Init window manager hints */
   e_hints_init();
   
   /* init generic communications */
   if (!ecore_con_init())
     {
	e_error_message_show(_("Enlightenment cannot initialize the connections system.\n"
			       "Perhaps you are out of memory?"));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(ecore_con_shutdown);
   /* init ipc */
   if (!ecore_ipc_init())
     {
	e_error_message_show(_("Enlightenment cannot initialize the IPC system.\n"
			       "Perhaps you are out of memory?"));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(ecore_ipc_shutdown);
   /* init the evas wrapper */
   if (!ecore_evas_init())
     { 
	e_error_message_show(_("Enlightenment cannot initialize the Evas system.\n"
			       "Perhaps you are out of memory?"));
	_e_main_shutdown(-1);
    }
   if (!ecore_evas_engine_type_supported_get(ECORE_EVAS_ENGINE_SOFTWARE_X11))
     {
	e_error_message_show(_("Enlightenment found ecore_evas doesnt support Software X11\n"
			       "rendering in Evas. Please check your installation of Evas and\n"
			       "Ecore and check they support Software X11 rendering."));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(ecore_evas_shutdown);
    /* init the file system */
   if (!ecore_file_init())
     {
	e_error_message_show(_("Enlightenment cannot initialize the File system.\n"
			       "Perhaps you are out of memory?"));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(ecore_file_shutdown);
   
	 
   /*** Finished loading subsystems, Loading WM Specifics ***/
	 
   /* setup directories we will be using for configurations sotrage etc. */
   if (!_e_main_dirs_init())
     {
	e_error_message_show(_("Enlightenment cannot create directories in your home directory.\n"
			       "Perhaps you have no home directory or the disk is full?"));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(_e_main_dirs_shutdown);
   /* init config system */
   if (!e_config_init())
     {
	e_error_message_show(_("Enlightenment cannot set up its config system."));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(e_config_shutdown);
   /* setup paths for finding things */
   if (!_e_main_path_init())
     {
	e_error_message_show(_("Enlightenment cannot set up paths for finding files.\n"
			       "Perhaps you are out of memory?"));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(_e_main_path_shutdown);
   /* init actions system */
   if (!e_actions_init())
     {
	e_error_message_show(_("Enlightenment cannot set up its actions system."));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(e_actions_shutdown);
   /* init bindings system */
   if (!e_bindings_init())
     {
	e_error_message_show(_("Enlightenment cannot set up its bindings system."));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(e_bindings_shutdown);
   /* init popup system */
   if (!e_popup_init())
     {
	e_error_message_show(_("Enlightenment cannot set up its popup system."));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(e_popup_shutdown);
   
   /* setup edje to animate @ e_config->framerate frames per sec. */
   edje_frametime_set(1.0 / e_config->framerate);

   /* init font system */
   if (!e_font_init())
     {
	e_error_message_show(_("Enlightenment cannot set up its font system."));
        _e_main_shutdown(-1);
     }
   _e_main_shutdown_push(e_font_shutdown);
   e_font_apply();
   e_canvas_recache();

   if (!((!e_config->show_splash) || (after_restart)))
     {
	/* setup init status window/screen */
	if (!e_init_init())
	  {
	     e_error_message_show(_("Enlightenment cannot set up init screen.\n"
				    "Perhaps you are out of memory?"));
	     _e_main_shutdown(-1);
	  }
	_e_main_shutdown_push(e_init_shutdown);
     }
   /* manage the root window */
   if (!_e_main_screens_init())
     {
	e_error_message_show(_("Enlightenment set up window management for all the screens on your system\n"
			       "failed. Perhaps another window manager is running?\n"));
	_e_main_shutdown(-1);
     }
   
   e_container_all_freeze();
   
   _e_main_shutdown_push(_e_main_screens_shutdown);
   /* init app system */
   if (!e_app_init())
     {
	e_error_message_show(_("Enlightenment cannot set up its app system."));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(e_app_shutdown);
   /* init theme system */
   if (!e_theme_init())
     {
	e_error_message_show(_("Enlightenment cannot set up its theme system."));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(e_theme_shutdown);

   
   /* tell the error system that it can use gui dialogs now */
   e_error_gui_set(1);
   
   /* setup e ipc service */
   if (!_e_main_ipc_init())
     {
	e_error_message_show(_("Enlightenment cannot set up the IPC socket.\n"
			       "It likely is already in use by an exisiting copy of Enlightenment.\n"
			       "Double check to see if Enlightenment is not already on this display,\n"
			       "but if that fails try deleting all files in ~/.ecore/enlightenment-*\n"
			       "and try running again."));
	ipc_failed = 1;
     }
   else
     _e_main_shutdown_push(_e_main_ipc_shutdown);

   /* setup generic msg handling etc */
   if (!e_msg_init())
     {
	e_error_message_show(_("Enlightenment cannot set up its msg system."));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(e_msg_shutdown);
   /* setup dnd */
   if (!e_dnd_init())
     {
	e_error_message_show(_("Enlightenment cannot set up its dnd system."));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(e_dnd_shutdown);
   /* setup module loading etc */
   if (!e_module_init())
     {
	e_error_message_show(_("Enlightenment cannot set up its module system."));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(e_module_shutdown);
   /* setup winlist */
   if (!e_winlist_init())
     {
	e_error_message_show(_("Enlightenment cannot set up its window list system."));
	_e_main_shutdown(-1);
     }
   _e_main_shutdown_push(e_winlist_shutdown);

   if (ipc_failed)
     e_error_dialog_show(_("Enlightenment IPC setup error!"),
			 _("Enlightenment cannot set up the IPC socket.\n"
			   "It likely is already in use by an exisiting copy of Enlightenment.\n"
			   "Double check to see if Enlightenment is not already on this display,\n"
			   "but if that fails try deleting all files in ~/.ecore/enlightenment-*\n"
			   "and try running again."));
   
   /* add in a handler that just before we go idle we flush x */
   _e_main_idle_enterer_flusher = ecore_idle_enterer_add(_e_main_cb_x_flusher, NULL);
      
   /* an idle enterer to be called after all others */
   _e_main_idle_enterer_after = ecore_idle_enterer_add(_e_main_cb_idler_after, NULL);

   e_managers_keys_grab();
   
//   ecore_x_ungrab();
   
   e_init_title_set(_("Enlightenment"));
   e_init_version_set(VERSION);
   e_init_status_set(_("Enlightenment Starting. Please wait."));
   
   if (!nostartup)
     {
	if (after_restart) e_startup(E_STARTUP_RESTART);
	else e_startup(E_STARTUP_START);
     }
   
   if (!((!e_config->show_splash) || (after_restart)))
     {
	ecore_timer_add( 3.0, _e_main_cb_startup_fake_status, _("Artificially slowing startup so you can see it all."));
	ecore_timer_add( 7.5, _e_main_cb_startup_fake_status, _("This is development code, so be warned."));
	ecore_timer_add(12.0, _e_main_cb_startup_fake_status, _("Most features do not work yet, and those that do are buggy."));
	ecore_timer_add(16.0, _e_main_cb_startup_fake_end, NULL);
     }
   
   e_container_all_thaw();
   
   /* run any testing code now we are set up */
   e_test();

   /* no longer starting up */
   starting = 0;
   /* start our main loop */
   ecore_main_loop_begin();
   
   /* ask all modules to save their config and then shutdown */
   /* NB: no need to do this as config shutdown will flush any saves */
   /*     and all changed config was already saved before */
   e_config_save_flush();

   /* Store current selected desktops */
   _e_main_desk_save();

   /* unroll our stack of shutdown functions with exit code of 0 */
   _e_main_shutdown(0);
   
   e_intl_shutdown();
   
   /* if we were flagged to restart, then  restart. */
   if (restart)
     {
	ecore_app_restart();
     }
   
   /* just return 0 to keep the compiler quiet */
   return 0;
}

/* FIXME: make safe to delete within a callback */
E_Before_Idler *
e_main_idler_before_add(int (*func) (void *data), void *data, int once)
{
   E_Before_Idler *eb;
   
   eb = calloc(1, sizeof(E_Before_Idler));
   eb->func = func;
   eb->data = data;
   eb->once = once;
   _e_main_idler_before_list = evas_list_append(_e_main_idler_before_list, eb);
   return eb;
}

void
e_main_idler_before_del(E_Before_Idler *eb)
{
   eb->delete_me = 1;
}

/* local subsystem functions */
static void
_e_main_shutdown_push(int (*func) (void))
{
   _e_main_level++;
   if (_e_main_level > MAX_LEVEL)
     {
	_e_main_level--;
	e_error_message_show("WARNING: too many init levels. MAX = %i", MAX_LEVEL);
	return;
     }
   _e_main_shutdown_func[_e_main_level - 1] = func;
}

static void
_e_main_shutdown(int errorcode)
{
   int i;
   
   printf("E17: Begin shutdown procedure!\n");
   if (_e_main_idle_enterer_before)
     {
	ecore_idle_enterer_del(_e_main_idle_enterer_before);
	_e_main_idle_enterer_before = NULL;
     }
   if (_e_main_idle_enterer_after)
     {
	ecore_idle_enterer_del(_e_main_idle_enterer_after);
	_e_main_idle_enterer_after = NULL;
     }
   if (_e_main_idle_enterer_flusher)
     {
	ecore_idle_enterer_del(_e_main_idle_enterer_flusher);
	_e_main_idle_enterer_flusher = NULL;
     }
   for (i = _e_main_level - 1; i >= 0; i--)
     (*_e_main_shutdown_func[i])();
   if (errorcode < 0) exit(errorcode);
}

static int
_e_main_x_shutdown(void)
{
//   ecore_x_ungrab();
   ecore_x_focus_reset();
   ecore_x_events_allow_all();

   ecore_x_shutdown();

   return 1;
}

static int
_e_main_dirs_init(void)
{
   char *homedir;
   char buf[PATH_MAX];
   const char *dirs[] =
     {
	"%s/.e",
	"%s/.e/e",
	"%s/.e/e/images",
	"%s/.e/e/fonts",
	"%s/.e/e/themes",
	"%s/.e/e/init",
	"%s/.e/e/icons",
	"%s/.e/e/backgrounds",
	"%s/.e/e/applications",
	"%s/.e/e/applications/all",
	"%s/.e/e/applications/favorite",
	"%s/.e/e/applications/bar",
	"%s/.e/e/applications/startup",
	"%s/.e/e/applications/restart",
	"%s/.e/e/applications/trash",
	"%s/.e/e/modules",
	"%s/.e/e/config",
	"%s/.e/e/locale"
     };
   int i;
   
   homedir = e_user_homedir_get();
   if (!homedir) return 0;
   for (i = 0; i < (int)(sizeof(dirs) / sizeof(char *)); i++)
     {
	snprintf(buf, sizeof(buf), dirs[i], homedir);
	if (!ecore_file_mkpath(buf))
	  {
	     e_error_message_show("Error creating directory:\n"
				  "%s",
				  buf);
	     free(homedir);
	     return 0;
	  }
     }
   
   /* FIXME: THIS is a hack to get people started!!! */
   /* err dont just disable it - replace it with a proper wizard tool */
   /* outside e's main source to populate these directories from gnome/kde */
   /* app menu data etc. */
   snprintf(buf, sizeof(buf), "%s/.e/e/applications/bar/.order", homedir);
   if (!ecore_file_exists(buf))
     {
	snprintf(buf, sizeof(buf), 
		 "gzip -d -c < %s/data/other/applications.tar.gz | "
		 "(cd %s/.e/e/ ; tar -xf -)", 
		 PACKAGE_DATA_DIR,
		 homedir);
	system(buf);
     }
   free(homedir);

   return 1;
}

static int
_e_main_dirs_shutdown(void)
{
   return 1;
}

static int
_e_main_screens_init(void)
{
   Ecore_X_Window *roots;
   int num, i;

   if (!e_atoms_init()) return 0;
   if (!e_manager_init()) return 0;
   if (!e_container_init()) return 0;
   if (!e_zone_init()) return 0;
   if (!e_desk_init()) return 0;
   if (!e_gadman_init()) return 0;
   if (!e_menu_init()) return 0;
   
   num = 0;
   roots = ecore_x_window_root_list(&num);
   if ((!roots) || (num <= 0))
     {
	e_error_message_show("X reports there are no root windows and %i screens!\n", 
			     num);
	return 0;
     }
   if (!e_focus_init()) return 0;
   if (!e_border_init()) return 0;
   for (i = 0; i < num; i++)
     {
	E_Manager *man;
	E_Container *con;
	
	man = e_manager_new(roots[i]);
	e_init_show();
	if (man)
	  {
	     man->num = i;
	     e_manager_show(man);
	     if (i == 0) ecore_x_window_focus(man->root);
	  }
	else
	  {
	     e_error_message_show("Cannot create manager object for screen %i\n", 
				  i);
	     return 0;
	  }
	con = e_container_new(man);
	if (con)
	  {
	     e_container_show(con);
	     e_hints_manager_init(man);
	     _e_main_desk_restore(man, con);
	     e_manager_manage_windows(man);
	  }
	else
	  {
	     e_error_message_show("Cannot create desktop object for manager on screen %i\n", 
				  i);
	     return 0;
	  }
     }

   free(roots);
   ecore_x_sync();
   return 1;
}

static int
_e_main_screens_shutdown(void)
{
   e_border_shutdown();
   e_focus_shutdown();
   e_menu_shutdown();
   e_gadman_shutdown();
   e_desk_shutdown();
   e_zone_shutdown();
   e_container_shutdown();
   e_manager_shutdown();
   e_atoms_shutdown();
   return 1;
}

static int
_e_main_path_init(void)
{
   /* setup data paths */
   path_data = e_path_new();
   if (!path_data)
     {
	e_error_message_show("Cannot allocate path for path_data\n");
	return 0;
     }
   e_path_default_path_append(path_data, PACKAGE_DATA_DIR"/data");
   e_path_user_path_set(path_data, &(e_config->path_append_data));

   /* setup image paths */
   path_images = e_path_new();
   if (!path_images)
     {
	e_error_message_show("Cannot allocate path for path_images\n");
	return 0;
     }
   e_path_default_path_append(path_images, "~/.e/e/images");
   e_path_default_path_append(path_images, PACKAGE_DATA_DIR"/data/images");
   e_path_user_path_set(path_images, &(e_config->path_append_images));
   
   /* setup font paths */
   path_fonts = e_path_new();
   if (!path_fonts)
     {
	e_error_message_show("Cannot allocate path for path_fonts\n");
	return 0;
     }
   e_path_default_path_append(path_fonts, "~/.e/e/fonts");
   e_path_default_path_append(path_fonts, PACKAGE_DATA_DIR"/data/fonts");
   e_path_user_path_set(path_fonts, &(e_config->path_append_fonts));

   /* setup theme paths */
   path_themes = e_path_new();
   if (!path_themes)
     {
	e_error_message_show("Cannot allocate path for path_themes\n");
	return 0;
     }
   e_path_default_path_append(path_themes, "~/.e/e/themes");
   e_path_default_path_append(path_themes, PACKAGE_DATA_DIR"/data/themes");
   e_path_user_path_set(path_themes, &(e_config->path_append_themes));

   /* setup icon paths */
   path_icons = e_path_new();
   if (!path_icons)
     {
	e_error_message_show("Cannot allocate path for path_icons\n");
	return 0;
     }
   e_path_default_path_append(path_icons, "~/.e/e/icons");
   e_path_default_path_append(path_icons, PACKAGE_DATA_DIR"/data/icons");
   e_path_user_path_set(path_icons, &(e_config->path_append_icons));

   /* setup init paths */
   path_init = e_path_new();
   if (!path_init)
     {
	e_error_message_show("Cannot allocate path for path_init\n");
	return 0;
     }
   e_path_default_path_append(path_init, "~/.e/e/init");
   e_path_default_path_append(path_init, PACKAGE_DATA_DIR"/data/init");
   e_path_user_path_set(path_init, &(e_config->path_append_init));

   /* setup module paths */
   path_modules = e_path_new();
   if (!path_modules) 
     {
	e_error_message_show("Cannot allocate path for path_modules\n");
	return 0;
     }
   e_path_default_path_append(path_modules, "~/.e/e/modules");
   e_path_default_path_append(path_modules, PACKAGE_LIB_DIR"/enlightenment/modules");
   e_path_default_path_append(path_modules, PACKAGE_LIB_DIR"/enlightenment/modules_extra");
   e_path_user_path_set(path_modules, &(e_config->path_append_modules));    

   /* setup background paths */
   path_backgrounds = e_path_new();
   if (!path_backgrounds) 
     {
	e_error_message_show("Cannot allocate path for path_backgrounds\n");
	return 0;
     }
   e_path_default_path_append(path_backgrounds, "~/.e/e/backgrounds");
   e_path_user_path_set(path_backgrounds, &(e_config->path_append_backgrounds));

   return 1;
}

static int
_e_main_path_shutdown(void)
{
   if (path_data)
     {
	e_object_del(E_OBJECT(path_data));
	path_data = NULL;
     }
   if (path_images)
     {
	e_object_del(E_OBJECT(path_images));
	path_images = NULL;
     }
   if (path_fonts)
     {
	e_object_del(E_OBJECT(path_fonts));
	path_fonts = NULL;
     }
   if (path_themes)
     {
	e_object_del(E_OBJECT(path_themes));
	path_themes = NULL;
     }
   if (path_icons)
     {
	e_object_del(E_OBJECT(path_icons));
	path_icons = NULL;
     }
   if (path_init)
     {
	e_object_del(E_OBJECT(path_init));
	path_init = NULL;
     }
   if (path_modules)
     {
        e_object_del(E_OBJECT(path_modules));
        path_modules = NULL;
     }
   if (path_backgrounds)
     {
	e_object_del(E_OBJECT(path_backgrounds));
        path_backgrounds = NULL;
     }
   return 1;
}

static int
_e_main_ipc_init(void)
{
   if (!e_ipc_init())
     {
	e_error_message_show("Cannot init IPC subsystem!\n");
	return 0;
     }
   return 1;
}

static int
_e_main_ipc_shutdown(void)
{
   e_ipc_shutdown();
   return 1;
}

static void
_e_main_cb_x_fatal(void *data __UNUSED__)
{
   e_error_gui_set(0);
   e_error_message_show("Lost X connection.");
   ecore_main_loop_quit();
   _e_main_shutdown(-1);
}

static int
_e_main_cb_signal_exit(void *data __UNUSED__, int ev_type __UNUSED__, void *ev __UNUSED__)
{
   /* called on ctrl-c, kill (pid) (also SIGINT, SIGTERM and SIGQIT) */
   ecore_main_loop_quit();
   return 1;
}

static int
_e_main_cb_signal_hup(void *data __UNUSED__, int ev_type __UNUSED__, void *ev __UNUSED__)
{
   /* called on SIGHUP to restart Enlightenment */
   restart = 1;
   ecore_main_loop_quit();
   return 1;
}

static int
_e_main_cb_x_flusher(void *data __UNUSED__)
{
   ecore_x_flush();
   return 1;
}

static int
_e_main_cb_idler_before(void *data __UNUSED__)
{
   Evas_List *l, *pl;
   
   e_menu_idler_before();
   e_focus_idler_before();
   e_border_idler_before();
   e_popup_idler_before();
   e_drag_idler_before();
   for (l = _e_main_idler_before_list; l; l = l->next)
     {
	E_Before_Idler *eb;
	
	eb = l->data;
	if (!eb->delete_me)
	  {
	     if (!eb->func(eb->data)) eb->delete_me = 1;
	  }
     }
   for (l = _e_main_idler_before_list; l;)
     {
	E_Before_Idler *eb;
	
	eb = l->data;
	pl = l;
	l = l->next;
	if ((eb->once) || (eb->delete_me))
	  {
	     _e_main_idler_before_list =
	       evas_list_remove_list(_e_main_idler_before_list, pl);
	     free(eb);
	  }
     }
   edje_thaw();
//   printf("IN to idle... %3.3f\n", ecore_time_get());
   return 1;
}

static int
_e_main_cb_idler_after(void *data __UNUSED__)
{
//   printf("OUT of idle... %3.3f\n", ecore_time_get());
   edje_freeze();
   return 1;
}

static int
_e_main_cb_startup_fake_status(void *data)
{
   e_init_status_set((const char *)data);
   return 0;
}

static int
_e_main_cb_startup_fake_end(void *data __UNUSED__)
{
   e_init_hide();
   return 0;
}

static void
_e_main_desk_save(void)
{
   Evas_List *ml;
   char env[1024];
   char name[1024];

   for (ml = e_manager_list(); ml; ml = ml->next)
     {
	E_Manager *man;
	Evas_List *cl;

	man = ml->data;

	for (cl = man->containers; cl; cl = cl->next)
	  {
	     E_Container *con;
	     Evas_List *zl;

	     con = cl->data;
	     
	     for (zl = con->zones; zl; zl = zl->next)
	       {
		  E_Zone *zone;

		  zone = zl->data;

		  snprintf(name, sizeof(name), "DESK_%d_%d_%d", man->num, con->num, zone->num);
		  snprintf(env, sizeof(env), "%d,%d", zone->desk_x_current, zone->desk_y_current);
		  e_util_env_set(name, env);
	       }
	  }
     }
}

static void
_e_main_desk_restore(E_Manager *man, E_Container *con)
{
   Evas_List *zl;
   char *env;
   char name[1024];

   for (zl = con->zones; zl; zl = zl->next)
     {
	E_Zone *zone;
	E_Desk *desk;
	int desk_x, desk_y;

	zone = zl->data;

	snprintf(name, sizeof(name), "DESK_%d_%d_%d", man->num, con->num, zone->num);
	env = getenv(name);
	if (!env) continue;
	if (!sscanf(env, "%d,%d", &desk_x, &desk_y)) continue;
	desk = e_desk_at_xy_get(zone, desk_x, desk_y);
	if (!desk) continue;
	e_desk_show(desk);
     }
}
