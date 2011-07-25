#include "e.h"
#ifdef HAVE_ECORE_IMF
# include <Ecore_IMF.h>
#endif

#define MAX_LEVEL 55

#define TS_DO
#ifdef TS_DO
#define TS(x) \
   { \
      t1 = ecore_time_unix_get(); \
      printf("ESTART: %1.5f [%1.5f] - %s\n", t1 - t0, t1 - t2, x); \
      t2 = t1; \
   }
static double t0, t1, t2;
#else
# define TS(x)
#endif

/*
 * i need to make more use of these when i'm baffled as to when something is
 * up. other hooks:
 * 
 *      void *(*__malloc_hook)(size_t size, const void *caller);
 * 
 *      void *(*__realloc_hook)(void *ptr, size_t size, const void *caller);
 *
 *      void *(*__memalign_hook)(size_t alignment, size_t size,
 *                               const void *caller);
 *
 *      void (*__free_hook)(void *ptr, const void *caller);
 *
 *      void (*__malloc_initialize_hook)(void);
 *
 *      void (*__after_morecore_hook)(void);
 *

static void my_init_hook(void);
static void my_free_hook(void *p, const void *caller);

static void (*old_free_hook)(void *ptr, const void *caller) = NULL;
void (*__free_hook)(void *ptr, const void *caller);

void (*__malloc_initialize_hook) (void) = my_init_hook;
static void
my_init_hook(void)
{
   old_free_hook = __free_hook;
   __free_hook = my_free_hook;
}

//void *magicfree = NULL;

static void 
my_free_hook(void *p, const void *caller)
{
   __free_hook = old_free_hook;
//   if ((p) && (p == magicfree))
//     {
//	printf("CAUGHT!!!!! %p ...\n", p);
//	abort();
//     }
   free(p);
   __free_hook = my_free_hook;
}
*/

/* local function prototypes */
static void _e_main_shutdown(int errcode);
static void _e_main_shutdown_push(int (*func)(void));
static void _e_main_parse_arguments(int argc, char **argv);
static void _e_main_cb_x_fatal(void *data __UNUSED__);
static Eina_Bool _e_main_cb_signal_exit(void *data __UNUSED__, int ev_type __UNUSED__, void *ev __UNUSED__);
static Eina_Bool _e_main_cb_signal_hup(void *data __UNUSED__, int ev_type __UNUSED__, void *ev __UNUSED__);
static Eina_Bool _e_main_cb_signal_user(void *data __UNUSED__, int ev_type __UNUSED__, void *ev);
static int _e_main_x_shutdown(void);
static int _e_main_dirs_init(void);
static int _e_main_dirs_shutdown(void);
static int _e_main_path_init(void);
static int _e_main_path_shutdown(void);
static void _e_main_test_formats(void);
static int _e_main_screens_init(void);
static int _e_main_screens_shutdown(void);
static void _e_main_desk_save(void);
static void _e_main_desk_restore(E_Manager *man, E_Container *con);
static void _e_main_efreet_paths_init(void);
static void _e_main_modules_load(Eina_Bool safe_mode);
static void _e_main_manage_all(void);
static Eina_Bool _e_main_cb_x_flusher(void *data __UNUSED__);
static Eina_Bool _e_main_cb_idle_before(void *data __UNUSED__);
static Eina_Bool _e_main_cb_idle_after(void *data __UNUSED__);
static Eina_Bool _e_main_cb_startup_fake_end(void *data __UNUSED__);

/* local variables */
static Eina_Bool really_know = EINA_FALSE;
static Eina_Bool locked = EINA_FALSE;
static Eina_Bool inloop = EINA_FALSE;
static jmp_buf x_fatal_buff;

static int _e_main_lvl = 0;
static int (*_e_main_shutdown_func[MAX_LEVEL])(void);

static Eina_List *_idle_before_list = NULL;
static Ecore_Idle_Enterer *_idle_before = NULL;
static Ecore_Idle_Enterer *_idle_after = NULL;
static Ecore_Idle_Enterer *_idle_flush = NULL;

/* external variables */
EAPI Eina_Bool e_precache_end = EINA_FALSE;
EAPI Eina_Bool x_fatal = EINA_FALSE;
EAPI Eina_Bool good = EINA_FALSE;
EAPI Eina_Bool evil = EINA_FALSE;
EAPI Eina_Bool starting = EINA_TRUE;
EAPI Eina_Bool stopping = EINA_FALSE;
EAPI Eina_Bool restart = EINA_FALSE;

/* externally accessible functions */
int 
main(int argc, char **argv) 
{
   Eina_Bool nostartup = EINA_FALSE;
   Eina_Bool safe_mode = EINA_FALSE;
   Eina_Bool after_restart = EINA_FALSE;
   double t = 0.0, tstart = 0.0;
   char *s = NULL, buff[PATH_MAX];
   struct sigaction action;
#ifdef TS_DO
   t0 = t1 = t2 = ecore_time_unix_get();
#endif

   TS("Begin Startup");

   /* trap deadly bug signals and allow some form of sane recovery */
   /* or ability to gdb attach and debug at this point - better than your */
   /* wm/desktop vanishing and not knowing what happened */
   TS("Signal Trap");
   action.sa_sigaction = e_sigseg_act;
   action.sa_flags = SA_NODEFER | SA_RESETHAND | SA_SIGINFO;
   sigemptyset(&action.sa_mask);
   sigaction(SIGSEGV, &action, NULL);

   action.sa_sigaction = e_sigill_act;
   action.sa_flags = SA_NODEFER | SA_RESETHAND | SA_SIGINFO;
   sigemptyset(&action.sa_mask);
   sigaction(SIGILL, &action, NULL);

   action.sa_sigaction = e_sigfpe_act;
   action.sa_flags = SA_NODEFER | SA_RESETHAND | SA_SIGINFO; 
   sigemptyset(&action.sa_mask);
   sigaction(SIGFPE, &action, NULL);

   action.sa_sigaction = e_sigbus_act;
   action.sa_flags = SA_NODEFER | SA_RESETHAND | SA_SIGINFO;
   sigemptyset(&action.sa_mask);
   sigaction(SIGBUS, &action, NULL);

   action.sa_sigaction = e_sigabrt_act;
   action.sa_flags = SA_NODEFER | SA_RESETHAND | SA_SIGINFO;
   sigemptyset(&action.sa_mask);
   sigaction(SIGABRT, &action, NULL);
   TS("Signal Trap Done");

   t = ecore_time_unix_get();
   s = getenv("E_START_TIME");
   if ((s) && (!getenv("E_RESTART_OK"))) 
     {
        tstart = atof(s);
        if ((t - tstart) < 5.0) safe_mode = EINA_TRUE;
     }
   tstart = t;
   snprintf(buff, sizeof(buff), "%1.1f", tstart);
   e_util_env_set("E_START_TIME", buff);

   TS("Eina Init");
   if (!eina_init()) 
     {
        e_error_message_show(_("Enlightenment cannot initialize Eina!\n"));
        _e_main_shutdown(-1);
     }
   TS("Eina Init Done");
   _e_main_shutdown_push(eina_shutdown);

   TS("Determine Prefix");
   if (!e_prefix_determine(argv[0])) 
     {
        fprintf(stderr, 
                "ERROR: Enlightenment cannot determine it's installed\n"
                "       prefix from the system or argv[0].\n"
                "       This is because it is not on Linux AND has been\n"
                "       executed strangely. This is unusual.\n");
     }
   TS("Determine Prefix Done");

   /* for debugging by redirecting stdout of e to a log file to tail */
   setvbuf(stdout, NULL, _IONBF, 0);

   TS("Environment Variables");
   if (getenv("E_RESTART")) after_restart = EINA_TRUE;
   if (getenv("DESKTOP_STARTUP_ID"))
     e_util_env_set("DESKTOP_STARTUP_ID", NULL);
   e_util_env_set("E_RESTART_OK", NULL);
   e_util_env_set("E_RESTART", "1");
   e_util_env_set("PANTS", "ON");
   e_util_env_set("DESKTOP", "Enlightenment-0.17.0");
   TS("Environment Variables Done");

   TS("Parse Arguments");
   _e_main_parse_arguments(argc, argv);
   TS("Parse Arguments Done");

   /*** Initialize Core EFL Libraries We Need ***/

   TS("Eet Init");
   if (!eet_init()) 
     {
        e_error_message_show(_("Enlightenment cannot initialize Eet!\n"));
        _e_main_shutdown(-1);
     }
   TS("Eet Init Done");
   _e_main_shutdown_push(eet_shutdown);

   TS("Ecore Init");
   if (!ecore_init()) 
     {
        e_error_message_show(_("Enlightenment cannot initialize Ecore!\n"));
        _e_main_shutdown(-1);
     }
   TS("Ecore Init Done");
   _e_main_shutdown_push(ecore_shutdown);

   ecore_app_args_set(argc, (const char **)argv);

   TS("Ecore Event Handlers");
   if (!ecore_event_handler_add(ECORE_EVENT_SIGNAL_EXIT, 
                                _e_main_cb_signal_exit, NULL))
     {
   	e_error_message_show(_("Enlightenment cannot set up an exit signal handler.\n"
   			       "Perhaps you are out of memory?"));
   	_e_main_shutdown(-1);
     }
   if (!ecore_event_handler_add(ECORE_EVENT_SIGNAL_HUP, 
                                _e_main_cb_signal_hup, NULL))
     {
   	e_error_message_show(_("Enlightenment cannot set up a HUP signal handler.\n"
   			       "Perhaps you are out of memory?"));
   	_e_main_shutdown(-1);
     }
   if (!ecore_event_handler_add(ECORE_EVENT_SIGNAL_USER, 
                                _e_main_cb_signal_user, NULL))
     {
   	e_error_message_show(_("Enlightenment cannot set up a USER signal handler.\n"
   			       "Perhaps you are out of memory?"));
   	_e_main_shutdown(-1);
     }
   TS("Ecore Event Handlers Done");

#ifdef HAVE_ECORE_IMF
   TS("Ecore_IMF Init");
   if (!ecore_imf_init()) 
     {
        e_error_message_show(_("Enlightenment cannot initialize Ecore_IMF!\n"));
        _e_main_shutdown(-1);
     }
   TS("Ecore_IMF Init Done");
   _e_main_shutdown_push(ecore_imf_shutdown);
#endif

   TS("Ecore_File Init");
   if (!ecore_file_init()) 
     {
        e_error_message_show(_("Enlightenment cannot initialize Ecore_File!\n"));
        _e_main_shutdown(-1);
     }
   TS("Ecore_File Init Done");
   _e_main_shutdown_push(ecore_file_shutdown);

   TS("Ecore_Con Init");
   if (!ecore_con_init()) 
     {
        e_error_message_show(_("Enlightenment cannot initialize Ecore_Con!\n"));
        _e_main_shutdown(-1);
     }
   TS("Ecore_Con Init Done");
   _e_main_shutdown_push(ecore_con_shutdown);

   TS("Ecore_Ipc Init");
   if (!ecore_ipc_init()) 
     {
        e_error_message_show(_("Enlightenment cannot initialize Ecore_Ipc!\n"));
        _e_main_shutdown(-1);
     }
   TS("Ecore_Ipc Init Done");
   _e_main_shutdown_push(ecore_ipc_shutdown);

   _idle_before = ecore_idle_enterer_before_add(_e_main_cb_idle_before, NULL);

   TS("Ecore_X Init");
   if (!ecore_x_init(NULL)) 
     {
        e_error_message_show(_("Enlightenment cannot initialize Ecore_X!\n"));
        _e_main_shutdown(-1);
     }
   TS("Ecore_X Init Done");
   _e_main_shutdown_push(_e_main_x_shutdown);

   ecore_x_io_error_handler_set(_e_main_cb_x_fatal, NULL);

   TS("Ecore_Evas Init");
   if (!ecore_evas_init()) 
     {
        e_error_message_show(_("Enlightenment cannot initialize Ecore_Evas!\n"));
        _e_main_shutdown(-1);
     }
   TS("Ecore_Evas Init Done");
//   _e_main_shutdown_push(ecore_evas_shutdown);

   /* e doesn't sync to compositor - it should be one */
   ecore_evas_app_comp_sync_set(0);

   TS("Ecore_Evas Engine Check");
   if (!ecore_evas_engine_type_supported_get(ECORE_EVAS_ENGINE_SOFTWARE_XCB)) 
     {
        if (!ecore_evas_engine_type_supported_get(ECORE_EVAS_ENGINE_SOFTWARE_XLIB))
          {
             e_error_message_show(_("Enlightenment found ecore_evas doesn't support the Software X11\n"
                                    "rendering in Evas. Please check your installation of Evas and\n"
                                    "Ecore and check they support the Software X11 rendering engine."));
             _e_main_shutdown(-1);
          }
     }
   if (!ecore_evas_engine_type_supported_get(ECORE_EVAS_ENGINE_SOFTWARE_BUFFER))
     {
	e_error_message_show(_("Enlightenment found ecore_evas doesn't support the Software Buffer\n"
			       "rendering in Evas. Please check your installation of Evas and\n"
			       "Ecore and check they support the Software Buffer rendering engine."));
	_e_main_shutdown(-1);
     }
   TS("Ecore_Evas Engine Check Done");

   TS("Efreet Init");
   if (!efreet_init()) 
     {
        e_error_message_show(_("Enlightenment cannot initialize the FDO desktop system.\n"
                               "Perhaps you lack permissions on ~/.cache/efreet or are\n"
                               "out of memory or disk space?"));
        _e_main_shutdown(-1);
     }
   TS("Efreet Init Done");
   _e_main_shutdown_push(efreet_shutdown);

   TS("Edje Init");
   if (!edje_init()) 
     {
        e_error_message_show(_("Enlightenment cannot initialize Edje!\n"));
        _e_main_shutdown(-1);
     }
   TS("Edje Init Done");
   _e_main_shutdown_push(edje_shutdown);
   edje_freeze();

   /*** Initialize E Subsystems We Need ***/

   TS("E Intl Init");
   if (!e_intl_init()) 
     {
        e_error_message_show(_("Enlightenment cannot initialize E_Intl!\n"));
        _e_main_shutdown(-1);
     }
   TS("E Intl Init Done");
   _e_main_shutdown_push(e_intl_shutdown);

   /* init white box of death alert */
   TS("E_Alert Init");
   if (!e_alert_init())
     {
	e_error_message_show(_("Enlightenment cannot initialize its emergency alert system.\n"
			       "Have you set your DISPLAY variable?"));
	_e_main_shutdown(-1);
     }
   TS("E_Alert Init Done");
   _e_main_shutdown_push(e_alert_shutdown);

   TS("E_Xinerama Init");
   if (!e_xinerama_init()) 
     {
        e_error_message_show(_("Enlightenment cannot initialize E_Xinerama!\n"));
        _e_main_shutdown(-1);
     }
   TS("E_Xinerama Init Done");
   _e_main_shutdown_push(e_xinerama_shutdown);

   TS("E_Randr Init");
   if (!e_randr_init()) 
     {
        e_error_message_show(_("Enlightenment cannot initialize E_Randr!\n"));
        _e_main_shutdown(-1);
     }
   TS("E_Randr Init Done");
   _e_main_shutdown_push(e_randr_shutdown);

   TS("E_Hints Init");
   e_hints_init();
   TS("E_Hints Init Done");

   TS("E_Configure Init");
   e_configure_init(); 
   TS("E_Configure Init Done");

   TS("E Directories Init");
   /* setup directories we will be using for configurations storage etc. */
   if (!_e_main_dirs_init())
     {
	e_error_message_show(_("Enlightenment cannot create directories in your home directory.\n"
			       "Perhaps you have no home directory or the disk is full?"));
	_e_main_shutdown(-1);
     }
   TS("E Directories Init Done");
   _e_main_shutdown_push(_e_main_dirs_shutdown);

   TS("E_Filereg Init");
   if (!e_filereg_init())
     {
	e_error_message_show(_("Enlightenment cannot set up its file registry system."));
	_e_main_shutdown(-1);
     }
   TS("E_Filereg Init Done");
   _e_main_shutdown_push(e_filereg_shutdown);

   TS("E_Config Init");
   if (!e_config_init())
     {
	e_error_message_show(_("Enlightenment cannot set up its config system."));
	_e_main_shutdown(-1);
     }
   TS("E_Config Init Done");
   _e_main_shutdown_push(e_config_shutdown);

   /* fix for FOOLS that keep cp'ing default.edj into ~/.e/e/themes */
   e_user_dir_concat_static(buff, "themes/default.edj");
   if (ecore_file_exists(buff)) ecore_file_unlink(buff);

   TS("E_Env Init");
   if (!e_env_init())
     {
	e_error_message_show(_("Enlightenment cannot set up its environment."));
	_e_main_shutdown(-1);
     }
   TS("E_Env Init Done");
   _e_main_shutdown_push(e_env_shutdown);

   e_util_env_set("E_ICON_THEME", e_config->icon_theme);
   ecore_exe_run_priority_set(e_config->priority);
   locked |= e_config->desklock_start_locked;

   TS("E_Scale Init");
   if (!e_scale_init())
     {
	e_error_message_show(_("Enlightenment cannot set up its scale system."));
	_e_main_shutdown(-1);
     }
   TS("E_Scale Init Done");
   _e_main_shutdown_push(e_scale_shutdown);

   TS("E_Pointer Init");
   if (!e_pointer_init()) 
     {
	e_error_message_show(_("Enlightenment cannot set up its pointer system."));
	_e_main_shutdown(-1);
     }
   TS("E_Pointer Init Done");
   _e_main_shutdown_push(e_pointer_shutdown);

   TS("E Paths Init");
   if (!_e_main_path_init())
     {
	e_error_message_show(_("Enlightenment cannot set up paths for finding files.\n"
			       "Perhaps you are out of memory?"));
	_e_main_shutdown(-1);
     }
   TS("E Paths Init Done");
   _e_main_shutdown_push(_e_main_path_shutdown);

   TS("E_Ipc Init");
   if (!e_ipc_init()) _e_main_shutdown(-1);
   TS("E_Ipc Init Done");
   _e_main_shutdown_push(e_ipc_shutdown);

   edje_frametime_set(1.0 / e_config->framerate);

   TS("E_Font Init");
   if (!e_font_init()) 
     {
	e_error_message_show(_("Enlightenment cannot set up its font system."));
	_e_main_shutdown(-1);
     }
   TS("E_Font Init Done");
   _e_main_shutdown_push(e_font_shutdown);

   TS("E_Font Apply");
   e_font_apply();
   TS("E_Font Apply Done");

   TS("E_Canvas Recache");
   e_canvas_recache();
   TS("E_Canvas Recache Done");

   TS("E_Theme Init");
   if (!e_theme_init()) 
     {
	e_error_message_show(_("Enlightenment cannot set up its theme system."));
	_e_main_shutdown(-1);
     }
   TS("E_Theme Init Done");
   _e_main_shutdown_push(e_theme_shutdown);

   TS("E_Moveresize Init");
   e_moveresize_init();
   TS("E_Moveresize Init Done");
   _e_main_shutdown_push(e_moveresize_shutdown);

   if (e_config->show_splash) 
     {
        TS("E_Splash Init");
        if (!e_init_init()) 
          {
             e_error_message_show(_("Enlightenment cannot set up its init screen."));
             _e_main_shutdown(-1);
          }
        TS("E_Splash Init Done");
        _e_main_shutdown_push(e_init_shutdown);
     }
   if (!((!e_config->show_splash) || (after_restart))) 
     {
        e_init_title_set(_("Enlightenment"));
        e_init_version_set(VERSION);
        e_init_show();
        pause();
     }

   if (e_config->show_splash)
     e_init_status_set(_("Starting International Support"));
   TS("E_Intl Post Init");
   if (!e_intl_post_init()) 
     {
	e_error_message_show(_("Enlightenment cannot set up its intl system."));
	_e_main_shutdown(-1);
     }
   TS("E_Intl Post Init Done");
   _e_main_shutdown_push(e_intl_post_shutdown);

   if (!really_know) 
     {
        TS("Test File Format Support");
        _e_main_test_formats();
        TS("Test File Format Support Done");
     }
   else 
     {
        efreet_icon_extension_add(".svg");
        efreet_icon_extension_add(".jpg");
        efreet_icon_extension_add(".png");
        efreet_icon_extension_add(".edj");
     }

   if (e_config->show_splash)
     e_init_status_set(_("Setup Screens"));
   TS("Screens Init");
   if (!_e_main_screens_init()) 
     {
	e_error_message_show(_("Enlightenment set up window management for all the screens on your system\n"
			       "failed. Perhaps another window manager is running?\n"));
	_e_main_shutdown(-1);
     }
   TS("Screens Init Done");
   _e_main_shutdown_push(_e_main_screens_shutdown);

   if (e_config->show_splash)
     e_init_status_set(_("Setup ACPI"));
   TS("E_Acpi Init");
   e_acpi_init();
   TS("E_Acpi Init Done");
   _e_main_shutdown_push(e_acpi_shutdown);

   if (e_config->show_splash)
     e_init_status_set(_("Setup Backlight"));
   TS("E_Backlight Init");
   if (!e_backlight_init()) 
     {
	e_error_message_show(_("Enlightenment cannot configure the backlight."));
	_e_main_shutdown(-1);
     }
   TS("E_Backlight Init Done");
   _e_main_shutdown_push(e_backlight_shutdown);

   if (e_config->show_splash)
     e_init_status_set(_("Setup DPMS"));
   TS("E_Dpms Init");
   if (!e_dpms_init()) 
     {
	e_error_message_show(_("Enlightenment cannot configure the DPMS settings."));
	_e_main_shutdown(-1);
     }
   TS("E_Dpms Init Done");
   _e_main_shutdown_push(e_dpms_shutdown);

   if (e_config->show_splash)
     e_init_status_set(_("Setup Screensaver"));
   TS("E_Screensaver Init");
   if (!e_screensaver_init()) 
     {
	e_error_message_show(_("Enlightenment cannot configure the X screensaver."));
	_e_main_shutdown(-1);
     }
   TS("E_Screensaver Init Done");
   _e_main_shutdown_push(e_screensaver_shutdown);

   if (e_config->show_splash)
     e_init_status_set(_("Setup Powersave Modes"));
   TS("E_Powersave Init");
   if (!e_powersave_init()) 
     {
	e_error_message_show(_("Enlightenment cannot set up its powersave modes."));
	_e_main_shutdown(-1);
     }
   TS("E_Powersave Init Done");
   _e_main_shutdown_push(e_powersave_shutdown);

   if (e_config->show_splash)
     e_init_status_set(_("Setup Desklock"));
   TS("E_Desklock Init");
   if (!e_desklock_init()) 
     {
	e_error_message_show(_("Enlightenment cannot set up its desk locking system."));
	_e_main_shutdown(-1);
     }
   TS("E_Desklock Init Done");
   _e_main_shutdown_push(e_desklock_shutdown);

   if (e_config->show_splash)
     e_init_status_set(_("Setup Popups"));
   TS("E_Popups Init");
   if (!e_popup_init()) 
     {
	e_error_message_show(_("Enlightenment cannot set up its popup system."));
	_e_main_shutdown(-1);
     }
   TS("E_Popups Init Done");
   _e_main_shutdown_push(e_popup_shutdown);

   if ((locked) && ((!e_config->show_splash) && (!after_restart)))
     e_desklock_show();

   if (e_config->show_splash)
     e_init_status_set(_("Setup Message Bus"));
   TS("E_Msgbus Init");
   if (e_msgbus_init()) 
     _e_main_shutdown_push(e_msgbus_shutdown);
   TS("E_Msgbus Init Done");

   if (e_config->show_splash)
     e_init_status_set(_("Setup Paths"));
   TS("Efreet Paths");
   _e_main_efreet_paths_init();
   TS("Efreet Paths Done");

   if (e_config->show_splash)
     e_init_status_set(_("Setup System Controls"));
   TS("E_Sys Init");
   if (!e_sys_init()) 
     {
       e_error_message_show(_("Enlightenment cannot initialize the System Command system.\n"));
       _e_main_shutdown(-1);
     }
   TS("E_Sys Init Done");
   _e_main_shutdown_push(e_sys_shutdown);

   if (e_config->show_splash)
     e_init_status_set(_("Setup Actions"));
   TS("E_Actions Init");
   if (!e_actions_init()) 
     {
	e_error_message_show(_("Enlightenment cannot set up its actions system."));
	_e_main_shutdown(-1);
     }
   TS("E_Actions Init Done");
   _e_main_shutdown_push(e_actions_shutdown);

   if (e_config->show_splash)
     e_init_status_set(_("Setup Execution System"));
   TS("E_Exec Init");
   if (!e_exec_init()) 
     {
	e_error_message_show(_("Enlightenment cannot set up its exec system."));
	_e_main_shutdown(-1);
     }
   TS("E_Exec Init Done");
   _e_main_shutdown_push(e_exec_shutdown);

   TS("E_Container Freeze");
   e_container_all_freeze();
   TS("E_Container Freeze Done");

   if (e_config->show_splash)
     e_init_status_set(_("Setup Filemanager"));
   TS("E_Fm2 Init");
   if (!e_fm2_init()) 
     {
       e_error_message_show(_("Enlightenment cannot initialize the File manager.\n"));
       _e_main_shutdown(-1);
     }
   TS("E_Fm2 Init Done");
   _e_main_shutdown_push(e_fm2_shutdown);

   if (e_config->show_splash)
     e_init_status_set(_("Setup Message System"));
   TS("E_Msg Init");
   if (!e_msg_init()) 
     {
	e_error_message_show(_("Enlightenment cannot set up its msg system."));
	_e_main_shutdown(-1);
     }
   TS("E_Msg Init Done");
   _e_main_shutdown_push(e_msg_shutdown);

   if (e_config->show_splash)
     e_init_status_set(_("Setup DND"));
   TS("E_Dnd Init");
   if (!e_dnd_init()) 
     {
	e_error_message_show(_("Enlightenment cannot set up its dnd system."));
	_e_main_shutdown(-1);
     }
   TS("E_Dnd Init Done");
   _e_main_shutdown_push(e_dnd_shutdown);

   if (e_config->show_splash)
     e_init_status_set(_("Setup Grab Input Handling"));
   TS("E_Grabinput Init");
   if (!e_grabinput_init()) 
     {
	e_error_message_show(_("Enlightenment cannot set up its grab input handling system."));
	_e_main_shutdown(-1);
     }
   TS("E_Grabinput Init Done");
   _e_main_shutdown_push(e_grabinput_shutdown);

   if (e_config->show_splash)
     e_init_status_set(_("Setup Modules"));
   TS("E_Module Init");
   if (!e_module_init()) 
     {
	e_error_message_show(_("Enlightenment cannot set up its module system."));
	_e_main_shutdown(-1);
     }
   TS("E_Module Init Done");
   _e_main_shutdown_push(e_module_shutdown);

   if (e_config->show_splash)
     e_init_status_set(_("Setup Remembers"));
   TS("E_Remember Init");
   if (!e_remember_init(after_restart ? E_STARTUP_RESTART : E_STARTUP_START)) 
     {
       e_error_message_show(_("Enlightenment cannot setup remember settings."));
       _e_main_shutdown(-1);
     }
   TS("E_Remember Init Done");
   _e_main_shutdown_push(e_remember_shutdown);

   if (e_config->show_splash)
     e_init_status_set(_("Setup Color Classes"));
   TS("E_Color_Class Init");
   if (!e_color_class_init()) 
     {
	e_error_message_show(_("Enlightenment cannot set up its color class system."));
	_e_main_shutdown(-1);
     }
   TS("E_Color_Class Init Done");
   _e_main_shutdown_push(e_color_class_shutdown);

   if (e_config->show_splash)
     e_init_status_set(_("Setup Gadcon"));
   TS("E_Gadcon Init");
   if (!e_gadcon_init()) 
     {
	e_error_message_show(_("Enlightenment cannot set up its gadget control system."));
	_e_main_shutdown(-1);
     }
   TS("E_Gadcon Init Done");
   _e_main_shutdown_push(e_gadcon_shutdown);

   if (e_config->show_splash)
     e_init_status_set(_("Setup Wallpaper"));
   TS("E_Bg Init");
   if (!e_bg_init()) 
     {
	e_error_message_show(_("Enlightenment cannot set up its desktop background system."));
	_e_main_shutdown(-1);
     }
   TS("E_Bg Init Done");
   _e_main_shutdown_push(e_bg_shutdown);

   if (e_config->show_splash)
     e_init_status_set(_("Setup Mouse"));
   TS("E_Mouse Init");
   if (!e_mouse_update()) 
     {
	e_error_message_show(_("Enlightenment cannot configure the mouse settings."));
	_e_main_shutdown(-1);
     }
   TS("E_Mouse Init Done");

   if (e_config->show_splash) 
     e_init_status_set(_("Setup Bindings"));
   TS("E_Bindings Init");
   if (!e_bindings_init()) 
     {
	e_error_message_show(_("Enlightenment cannot set up its bindings system."));
	_e_main_shutdown(-1);
     }
   TS("E_Bindings Init Done");
   _e_main_shutdown_push(e_bindings_shutdown);

   if (e_config->show_splash)
     e_init_status_set(_("Setup Shelves"));
   TS("E_Shelf Init");
   if (!e_shelf_init()) 
     {
	e_error_message_show(_("Enlightenment cannot set up its module system."));
	_e_main_shutdown(-1);
     }
   TS("E_Shelf Init Done");
   _e_main_shutdown_push(e_shelf_shutdown);

   if (e_config->show_splash)
     e_init_status_set(_("Setup Thumbnailer"));
   TS("E_Thumb Init");
   if (!e_thumb_init()) 
     {
        e_error_message_show(_("Enlightenment cannot initialize the Thumbnailing system.\n"));
        _e_main_shutdown(-1);
     }
   TS("E_Thumb Init Done");
   _e_main_shutdown_push(e_thumb_shutdown);

   if (!after_restart) 
     {
        if (e_config->show_splash)
          e_init_status_set(_("Setup Desktop Environment"));
        TS("E_Deskenv Init");
        if (!e_deskenv_init()) 
          {
             e_error_message_show(_("Enlightenment cannot initialize its desktop environment.\n"));
             _e_main_shutdown(-1);
          }
        TS("E_Deskenv Init Done");
        _e_main_shutdown_push(e_deskenv_shutdown);
     }

   if (e_config->show_splash)
     e_init_status_set(_("Setup File Ordering"));
   TS("E_Order Init");
   if (!e_order_init()) 
     {
	e_error_message_show(_("Enlightenment cannot set up its order file system."));
	_e_main_shutdown(-1);
     }
   TS("E_Order Init Done");
   _e_main_shutdown_push(e_order_shutdown);

   TS("Add Idler For X Flush");
   _idle_flush = ecore_idle_enterer_add(_e_main_cb_x_flusher, NULL);
   TS("Add Idler For X Flush Done");

   TS("E_Manager Keys Grab");
   e_managers_keys_grab();
   TS("E_Manager Keys Grab Done");

   if (e_config->show_splash)
     e_init_status_set(_("Load Modules"));
   TS("Load Modules");
   _e_main_modules_load(safe_mode);
   TS("Load Modules Done");

   TS("Run Startup Apps");
   if (!nostartup) 
     {
        if (after_restart) 
          e_startup(E_STARTUP_RESTART);
        else
          e_startup(E_STARTUP_START);
     }
   TS("Run Startup Apps Done");

   if (!((!e_config->show_splash) || (after_restart))) 
     {
        ecore_timer_add(2.0, _e_main_cb_startup_fake_end, NULL);
        if (locked) e_desklock_show();
     }

   TS("E_Container Thaw");
   e_container_all_thaw();
   TS("E_Container Thaw Done");

   TS("E_Test Init");
   e_test();
   TS("E_Test Done");

   if (e_config->show_splash)
     e_init_status_set(_("Configure Shelves"));
   TS("E_Shelf Config Update");
   e_shelf_config_update();
   TS("E_Shelf Config Update Done");

   TS("Manage all windows");
   _e_main_manage_all();

   _idle_after = ecore_idle_enterer_add(_e_main_cb_idle_after, NULL);

   if (e_config->show_splash)
     e_init_status_set(_("Almost Done"));

   starting = EINA_FALSE;
   inloop = EINA_TRUE;

   TS("MAIN LOOP AT LAST");
   if (!setjmp(x_fatal_buff))
     ecore_main_loop_begin();
   else
     printf("FATAL: X Died. Connection gone. Abbreviated Shutdown\n");

   inloop = EINA_FALSE;
   stopping = EINA_TRUE;

   if (!x_fatal) e_canvas_idle_flush();

   e_config_save_flush();
   _e_main_desk_save();
   e_remember_internal_save();

   _e_main_shutdown(0);

   if (restart) 
     {
        e_util_env_set("E_RESTART_OK", "1");
        ecore_app_restart();
     }

   e_prefix_shutdown();

   return 0;
}

/* FIXME: make safe to delete within a callback */
EAPI E_Before_Idler *
e_main_idler_before_add(int (*func) (void *data), void *data, int once)
{
   E_Before_Idler *eb;

   eb = calloc(1, sizeof(E_Before_Idler));
   eb->func = func;
   eb->data = data;
   eb->once = once;
   _idle_before_list = eina_list_append(_idle_before_list, eb);
   return eb;
}

EAPI void
e_main_idler_before_del(E_Before_Idler *eb)
{
   eb->delete_me = 1;
}

/* local functions */
static void 
_e_main_shutdown(int errcode) 
{
   int i = 0;

   printf("E17: Begin Shutdown Procedure!\n");

   if (_idle_before) ecore_idle_enterer_del(_idle_before);
   _idle_before = NULL;
   if (_idle_after) ecore_idle_enterer_del(_idle_after);
   _idle_after = NULL;
   if (_idle_flush) ecore_idle_enterer_del(_idle_flush);
   _idle_flush = NULL;

   for (i = (_e_main_lvl - 1); i >= 0; i--)
     (*_e_main_shutdown_func[i])();
   if (errcode < 0) exit(errcode);
}

static void 
_e_main_shutdown_push(int (*func)(void)) 
{
   _e_main_lvl++;
   if (_e_main_lvl > MAX_LEVEL) 
     {
        _e_main_lvl--;
	e_error_message_show("WARNING: too many init levels. MAX = %i", 
                             MAX_LEVEL);
        return;
     }
   _e_main_shutdown_func[_e_main_lvl - 1] = func;
}

static void 
_e_main_parse_arguments(int argc, char **argv) 
{
   char *s = NULL;
   int i = 0;

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
	     if (sscanf(argv[i], "%ix%i+%i+%i", &w, &h, &x, &y) == 4)
	       e_xinerama_fake_screen_add(x, y, w, h);
	  }
	else if (!strcmp(argv[i], "-good"))
	  {
	     good = EINA_TRUE;
	     evil = EINA_FALSE;
	     printf("LA LA LA\n");
	  }
	else if (!strcmp(argv[i], "-evil"))
	  {
	     good = EINA_FALSE;
	     evil = EINA_TRUE;
	     printf("MUHAHAHAHHAHAHAHAHA\n");
	  }
	else if (!strcmp(argv[i], "-psychotic"))
	  {
	     good = EINA_TRUE;
	     evil = EINA_TRUE;
	     printf("MUHAHALALALALALALALA\n");
	  }
	else if ((!strcmp(argv[i], "-profile")) && (i < (argc - 1)))
	  {
	     i++;
	     if (!getenv("E_CONF_PROFILE"))
	       e_util_env_set("E_CONF_PROFILE", argv[i]);
	  }
	else if (!strcmp(argv[i], "-i-really-know-what-i-am-doing-and-accept-full-responsibility-for-it"))
          really_know = EINA_TRUE;
	else if (!strcmp(argv[i], "-locked"))
          locked = EINA_TRUE;
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
		  "\t-profile CONF_PROFILE\n"
		  "\t\tUse the configuration profile CONF_PROFILE instead of the user selected default or just \"default\".\n"
		  "\t-good\n"
		  "\t\tBe good.\n"
		  "\t-evil\n"
		  "\t\tBe evil.\n"
		  "\t-psychotic\n"
		  "\t\tBe psychotic.\n"
		  "\t-locked\n"
		  "\t\tStart with desklock on, so password will be asked.\n"
		  "\t-i-really-know-what-i-am-doing-and-accept-full-responsibility-for-it\n"
		  "\t\tIf you need this help, you don't need this option.\n"
		  )
		);
	     _e_main_shutdown(-1);
	  }
     }

   /* fix up DISPLAY to be :N.0 if no .screen is in it */
   s = getenv("DISPLAY");
   if (s)
     {
	char *p, buff[PATH_MAX];

	if (!(p = strrchr(s, ':')))
	  {
             snprintf(buff, sizeof(buff), "%s:0.0", s);
             e_util_env_set("DISPLAY", buff);
	  }
	else
	  {
	     if (!(p = strrchr(p, '.')))
	       {
		  snprintf(buff, sizeof(buff), "%s.0", s);
		  e_util_env_set("DISPLAY", buff);
	       }
	  }
     }

   /* we want to have been launched by enlightenment_start. there is a very */
   /* good reason we want to have been launched this way, thus check */
   if (!getenv("E_START"))
     {
	e_error_message_show(_("You are executing enlightenment directly. This is\n"
                               "bad. Please do not execute the \"enlightenment\"\n"
                               "binary. Use the \"enlightenment_start\" launcher. It\n"
                               "will handle setting up environment variables, paths,\n"
                               "and launching any other required services etc.\n"
                               "before enlightenment itself begins running.\n"));
	_e_main_shutdown(-1);
     }
}

static void 
_e_main_cb_x_fatal(void *data __UNUSED__) 
{
   e_error_message_show("Lost X Connection.");
   ecore_main_loop_quit();
   if (!x_fatal) 
     {
        x_fatal = EINA_TRUE;
        if (inloop) longjmp(x_fatal_buff, -99);
     }
}

static Eina_Bool
_e_main_cb_signal_exit(void *data __UNUSED__, int ev_type __UNUSED__, void *ev __UNUSED__)
{
   /* called on ctrl-c, kill (pid) (also SIGINT, SIGTERM and SIGQIT) */
   e_sys_action_do(E_SYS_EXIT, NULL);
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_e_main_cb_signal_hup(void *data __UNUSED__, int ev_type __UNUSED__, void *ev __UNUSED__)
{
   e_sys_action_do(E_SYS_RESTART, NULL);
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_e_main_cb_signal_user(void *data __UNUSED__, int ev_type __UNUSED__, void *ev)
{
   Ecore_Event_Signal_User *e = ev;
   
   if (e->number == 1)
     {
        E_Action *a = e_action_find("configuration");
        if ((a) && (a->func.go)) a->func.go(NULL, NULL);
     }
   else if (e->number == 2)
     {
        // comp module has its own handler for this for enabling/disabling fps debug
     }
   return ECORE_CALLBACK_RENEW;
}

static int 
_e_main_x_shutdown(void)
{
   if (x_fatal) return 1;
   /* ecore_x_ungrab(); */
   ecore_x_focus_reset();
   ecore_x_events_allow_all();
   ecore_x_shutdown();
   return 1;
}

static int 
_e_main_dirs_init(void)
{
   const char *base;
   const char *dirs[] = 
     {
        "images",
        "fonts",
        "themes",
        "icons",
        "backgrounds",
        "applications",
        "applications/menu",
        "applications/menu/favorite",
        "applications/menu/all",
        "applications/bar",
        "applications/bar/default",
        "applications/startup",
        "applications/restart",
        "applications/trash",
        "modules",
        "config",
        "locale",
        "input_methods",
        NULL
     };

   base = e_user_dir_get();
   if (ecore_file_mksubdirs(base, dirs) != sizeof(dirs)/sizeof(dirs[0]) - 1)
     {
	e_error_message_show("Could not create one of the required "
                             "subdirectories of '%s'", base);
	return 0;
     }

   return 1;
}

static int 
_e_main_dirs_shutdown(void)
{
   return 1;
}

static int
_e_main_path_init(void)
{
   char buf[PATH_MAX];

   /* setup data paths */
   path_data = e_path_new();
   if (!path_data)
     {
	e_error_message_show("Cannot allocate path for path_data\n");
	return 0;
     }
   e_prefix_data_concat_static(buf, "data");
   e_path_default_path_append(path_data, buf);
   e_path_user_path_set(path_data, &(e_config->path_append_data));

   /* setup image paths */
   path_images = e_path_new();
   if (!path_images)
     {
	e_error_message_show("Cannot allocate path for path_images\n");
	return 0;
     }
   e_user_dir_concat_static(buf, "/images");
   e_path_default_path_append(path_images, buf);
   e_prefix_data_concat_static(buf, "data/images");
   e_path_default_path_append(path_images, buf);
   e_path_user_path_set(path_images, &(e_config->path_append_images));

   /* setup font paths */
   path_fonts = e_path_new();
   if (!path_fonts)
     {
	e_error_message_show("Cannot allocate path for path_fonts\n");
	return 0;
     }
   e_user_dir_concat_static(buf, "/fonts");
   e_path_default_path_append(path_fonts, buf);
   e_prefix_data_concat_static(buf, "data/fonts");
   e_path_default_path_append(path_fonts, buf);
   e_path_user_path_set(path_fonts, &(e_config->path_append_fonts));

   /* setup theme paths */
   path_themes = e_path_new();
   if (!path_themes)
     {
	e_error_message_show("Cannot allocate path for path_themes\n");
	return 0;
     }
   e_user_dir_concat_static(buf, "/themes");
   e_path_default_path_append(path_themes, buf);
   e_prefix_data_concat_static(buf, "data/themes");
   e_path_default_path_append(path_themes, buf);
   e_path_user_path_set(path_themes, &(e_config->path_append_themes));

   /* setup icon paths */
   path_icons = e_path_new();
   if (!path_icons)
     {
	e_error_message_show("Cannot allocate path for path_icons\n");
	return 0;
     }
   e_user_dir_concat_static(buf, "/icons");
   e_path_default_path_append(path_icons, buf);
   e_prefix_data_concat_static(buf, "data/icons");
   e_path_default_path_append(path_icons, buf);
   e_path_user_path_set(path_icons, &(e_config->path_append_icons));

   /* setup module paths */
   path_modules = e_path_new();
   if (!path_modules) 
     {
	e_error_message_show("Cannot allocate path for path_modules\n");
	return 0;
     }
   e_user_dir_concat_static(buf, "/modules");
   e_path_default_path_append(path_modules, buf);
   snprintf(buf, sizeof(buf), "%s/enlightenment/modules", e_prefix_lib_get());
   e_path_default_path_append(path_modules, buf);
   /* FIXME: eventually this has to go - moduels should have installers that
    * add appropriate install paths (if not installed to user homedir) to
    * e's module search dirs
    */
   snprintf(buf, sizeof(buf), "%s/enlightenment/modules_extra", e_prefix_lib_get());
   e_path_default_path_append(path_modules, buf);
   e_path_user_path_set(path_modules, &(e_config->path_append_modules));    

   /* setup background paths */
   path_backgrounds = e_path_new();
   if (!path_backgrounds) 
     {
	e_error_message_show("Cannot allocate path for path_backgrounds\n");
	return 0;
     }
   e_user_dir_concat_static(buf, "/backgrounds");
   e_path_default_path_append(path_backgrounds, buf);
   e_prefix_data_concat_static(buf, "data/backgrounds");
   e_path_default_path_append(path_backgrounds, buf);
   e_path_user_path_set(path_backgrounds, &(e_config->path_append_backgrounds));

   path_messages = e_path_new();
   if (!path_messages) 
     {
	e_error_message_show("Cannot allocate path for path_messages\n");
	return 0;
     }
   e_user_dir_concat_static(buf, "/locale");
   e_path_default_path_append(path_messages, buf);   
   e_path_default_path_append(path_messages, e_prefix_locale_get());
   e_path_user_path_set(path_messages, &(e_config->path_append_messages));

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
   if (path_messages)
     {
	e_object_del(E_OBJECT(path_messages));
	path_messages = NULL;			          
     }
   return 1;
}

static void 
_e_main_test_formats(void) 
{
   Evas *evas;
   Ecore_Evas *ee;
   Evas_Object *im, *txt;
   Evas_Coord tw, th;
   char buff[PATH_MAX];

   if (e_config->show_splash)
     e_init_status_set(_("Testing Format Support"));

   if (!(ee = ecore_evas_buffer_new(1, 1))) 
     {
        e_error_message_show(_("Enlightenment found Evas can't create a buffer canvas. Please check\n"
                               "Evas has Software Buffer engine support.\n"));
        _e_main_shutdown(-1);
     }
   e_canvas_add(ee);
   evas = ecore_evas_get(ee);
   im = evas_object_image_add(evas);

   e_prefix_data_concat_static(buff, "data/images/test.svg");
   evas_object_image_file_set(im, buff, NULL);
   if (evas_object_image_load_error_get(im) != EVAS_LOAD_ERROR_NONE) 
     {
        e_error_message_show(_("Enlightenment found Evas can't load SVG files. "
                               "Check Evas has SVG loader support.\n"));
     }
   else 
     efreet_icon_extension_add(".svg");

   e_prefix_data_concat_static(buff, "data/images/test.jpg");
   evas_object_image_file_set(im, buff, NULL);
   if (evas_object_image_load_error_get(im) != EVAS_LOAD_ERROR_NONE)
     {
        e_error_message_show(_("Enlightenment found Evas can't load JPEG files. "
                               "Check Evas has JPEG loader support.\n"));
        _e_main_shutdown(-1);
     }
   efreet_icon_extension_add(".jpg");

   e_prefix_data_concat_static(buff, "data/images/test.png");
   evas_object_image_file_set(im, buff, NULL);
   if (evas_object_image_load_error_get(im) != EVAS_LOAD_ERROR_NONE) 
     {
        e_error_message_show(_("Enlightenment found Evas can't load PNG files. "
                               "Check Evas has PNG loader support.\n"));
        _e_main_shutdown(-1);
     }
   efreet_icon_extension_add(".png");

   e_prefix_data_concat_static(buff, "data/images/test.edj");
   evas_object_image_file_set(im, buff, "images/0");
   if (evas_object_image_load_error_get(im) != EVAS_LOAD_ERROR_NONE)
     {
        e_error_message_show(_("Enlightenment found Evas can't load EET files. "
                               "Check Evas has EET loader support.\n"));
        _e_main_shutdown(-1);
     }
   efreet_icon_extension_add(".edj");

   evas_object_del(im);

   txt = evas_object_text_add(evas);
   evas_object_text_font_set(txt, "Sans", 10);
   evas_object_text_text_set(txt, "Hello");
   evas_object_geometry_get(txt, NULL, NULL, &tw, &th);
   if ((tw <= 0) && (th <= 0))
     {
        e_error_message_show(_("Enlightenment found Evas can't load the 'Sans' font. Check Evas has fontconfig\n"
                               "support and system fontconfig defines a 'Sans' font.\n"));
        _e_main_shutdown(-1);
     }
   evas_object_del(txt);
   e_canvas_del(ee);
   ecore_evas_free(ee);
}

static int
_e_main_screens_init(void)
{
   Ecore_X_Window *roots;
   int num, i;

   TS("\tscreens: atoms");
   if (!e_atoms_init()) return 0;
   TS("\tscreens: manager");
   if (!e_manager_init()) return 0;
   TS("\tscreens: container");
   if (!e_container_init()) return 0;
   TS("\tscreens: zone");
   if (!e_zone_init()) return 0;
   TS("\tscreens: desk");
   if (!e_desk_init()) return 0;
   TS("\tscreens: menu");
   if (!e_menu_init()) return 0;
   TS("\tscreens: exehist");
   if (!e_exehist_init()) return 0;

   TS("\tscreens: get roots");
   num = 0;
   roots = ecore_x_window_root_list(&num);
   if ((!roots) || (num <= 0))
     {
	e_error_message_show("X reports there are no root windows and %i screens!\n", 
			     num);
	return 0;
     }
   TS("\tscreens: focus");
   if (!e_focus_init()) return 0;
   TS("\tscreens: border");
   if (!e_border_init()) return 0;
   TS("\tscreens: win");
   if (!e_win_init()) return 0;
   TS("\tscreens: manage roots");
   for (i = 0; i < num; i++)
     {
	E_Manager *man;
	E_Container *con;

	man = e_manager_new(roots[i], i);
	if (man)
	  e_manager_show(man);
	else
	  {
	     e_error_message_show("Cannot create manager object for screen %i\n", 
				  i);
	     free(roots);
	     return 0;
	  }
	con = e_container_new(man);
	if (con)
	  {
	     e_container_show(con);
	     e_grabinput_focus(con->bg_win, E_FOCUS_METHOD_PASSIVE);
	     e_hints_manager_init(man);
	     _e_main_desk_restore(man, con);
//	     e_manager_manage_windows(man);
	  }
	else
	  {
	     e_error_message_show("Cannot create desktop object for manager on screen %i\n", 
				  i);
	     free(roots);
	     return 0;
	  }
     }
   free(roots);

   TS("\tscreens: sync");
   ecore_x_sync();

   return 1;
}

static int
_e_main_screens_shutdown(void)
{
   e_win_shutdown();
   e_border_shutdown();
   e_focus_shutdown();
   e_exehist_shutdown();
   e_menu_shutdown();
// ecore_evas closes evas - deletes objs - deletes fm widgets which tries to
// ipc to slave to stop monitoring - but ipc has been shut down. dont shut
// down.   
//   e_desk_shutdown();
//   e_zone_shutdown();
//   e_container_shutdown();
//   e_manager_shutdown();
   e_atoms_shutdown();
   return 1;
}

static void
_e_main_desk_save(void)
{
   Eina_List *ml;
   E_Manager *man;
   char env[1024], name[1024];

   EINA_LIST_FOREACH(e_manager_list(), ml, man)
     {
	Eina_List *cl;
	E_Container *con;

	EINA_LIST_FOREACH(man->containers, cl, con)
	  {
	     Eina_List *zl;
	     E_Zone *zone;

	     EINA_LIST_FOREACH(con->zones, zl, zone)
	       {
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
   Eina_List *zl;
   E_Zone *zone;
   char *env;
   char name[1024];

   EINA_LIST_FOREACH(con->zones, zl, zone)
     {
	E_Desk *desk;
	int desk_x, desk_y;

	snprintf(name, sizeof(name), "DESK_%d_%d_%d", man->num, con->num, zone->num);
	env = getenv(name);
	if (!env) continue;
	if (!sscanf(env, "%d,%d", &desk_x, &desk_y)) continue;
	desk = e_desk_at_xy_get(zone, desk_x, desk_y);
	if (!desk) continue;
	e_desk_show(desk);
     }
}

static void 
_e_main_efreet_paths_init(void) 
{
   Eina_List **list;

   if ((list = efreet_icon_extra_list_get()))
     {
        char buff[PATH_MAX];

        e_user_dir_concat_static(buff, "icons");
        *list = eina_list_prepend(*list, (void *)eina_stringshare_add(buff));
        e_prefix_data_concat_static(buff, "data/icons");
        *list = eina_list_prepend(*list, (void *)eina_stringshare_add(buff));
     }
}

static void 
_e_main_modules_load(Eina_Bool safe_mode) 
{
   if (!safe_mode)
     e_module_all_load();
   else 
     {
	E_Module *m; 
	char *crashmodule;

	crashmodule = getenv("E_MODULE_LOAD");
	if (crashmodule) m = e_module_new(crashmodule);

	if ((crashmodule) && (m))
	  {
	     e_module_disable(m);
	     e_object_del(E_OBJECT(m));

	     e_int_config_modules(e_container_current_get(e_manager_current_get()), NULL);
	     e_error_message_show
	       (_("Enlightenment crashed early on start and has<br>"
		  "been restarted. There was an error loading<br>"
		  "module named: %s. This module has been disabled<br>"
		  "and will not be loaded."), crashmodule);
	     e_util_dialog_show
	       (_("Enlightenment crashed early on start and has been restarted"),
		   _("Enlightenment crashed early on start and has been restarted.<br>"
		     "There was an error loading module named: %s<br><br>"
		     "This module has been disabled and will not be loaded."), crashmodule);
	     e_module_all_load();
	  }
	else
	  {	
	     e_int_config_modules(e_container_current_get(e_manager_current_get()), NULL);
	     e_error_message_show
	       (_("Enlightenment crashed early on start and has<br>"
		  "been restarted. All modules have been disabled<br>"
		  "and will not be loaded to help remove any problem<br>"
		  "modules from your configuration. The module<br>"
		  "configuration dialog should let you select your<br>"
		  "modules again."));
	     e_util_dialog_show
	       (_("Enlightenment crashed early on start and has been restarted"),
		   _("Enlightenment crashed early on start and has been restarted.<br>"
		     "All modules have been disabled and will not be loaded to help<br>"
		     "remove any problem modules from your configuration.<br><br>"
		     "The module configuration dialog should let you select your<br>"
		     "modules again."));
	  }
     }
}

static void
_e_main_manage_all(void)
{
   Eina_List *l;
   E_Manager *man;

   EINA_LIST_FOREACH(e_manager_list(), l, man)
     e_manager_manage_windows(man);
}

static Eina_Bool
_e_main_cb_idle_before(void *data __UNUSED__)
{
   Eina_List *l, *pl;
   E_Before_Idler *eb;

   e_menu_idler_before();
   e_focus_idler_before();
   e_border_idler_before();
   e_popup_idler_before();
   e_drag_idler_before();
   e_pointer_idler_before();
   EINA_LIST_FOREACH(_idle_before_list, l, eb)
     {
	if (!eb->delete_me)
	  {
	     if (!eb->func(eb->data)) eb->delete_me = 1;
	  }
     }
   EINA_LIST_FOREACH_SAFE(_idle_before_list, pl, l, eb)
     {
	if ((eb->once) || (eb->delete_me))
	  {
	     _idle_before_list =
	       eina_list_remove_list(_idle_before_list, pl);
	     free(eb);
	  }
     }
   edje_thaw();
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_e_main_cb_idle_after(void *data __UNUSED__)
{
   static int first_idle = 1;

   edje_freeze();

   if (first_idle)
     {
        TS("SLEEP");
        first_idle = 0;
        e_precache_end = EINA_TRUE;
     }

   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_e_main_cb_x_flusher(void *data __UNUSED__)
{
   eet_clearcache();
   ecore_x_flush();
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_e_main_cb_startup_fake_end(void *data __UNUSED__)
{
   e_init_hide();
   return ECORE_CALLBACK_CANCEL;
}
