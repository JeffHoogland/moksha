#include "debug.h"
#include "actions.h"
#include "guides.h"
#include "cursors.h"
#include "border.h"
#include "config.h"
#include "desktops.h"
#include "exec.h"
#include "fs.h"
#include "entry.h"
#include "keys.h"
#include "ipc.h"
#include "menu.h"
#include "view.h"
#include "place.h"
#include "iconbar.h"
#include "util.h"
#include "e_view_machine.h"

#ifdef USE_FERITE
# include "e_ferite.h"
#endif

#include <time.h>
#include <X11/Xproto.h>

#ifdef E_PROF
Evas_List __e_profiles = NULL;
#endif

static void cb_exit(void);
static void wm_running_error(Display * d, XErrorEvent * ev);
static void setup(void);

static void cb_exit(void)
{
   D_ENTER;

   e_fs_cleanup();
   E_PROF_DUMP;
   
   D_RETURN;
}


static void 
wm_running_error(Display * d, XErrorEvent * ev)
{
   D_ENTER;

   if ((ev->request_code == X_ChangeWindowAttributes) && 
       (ev->error_code == BadAccess))
     {
	fprintf(stderr, "A window manager is already running.\n");
	fprintf(stderr, "Exiting Enlightenment. Error.\n");
	exit(-2);
     }   

   D_RETURN;
   UN(d);
}

static void
setup(void)
{
   D_ENTER;
      
   ecore_grab();
   ecore_sync();

   /* Start to manage all those windows that we're interested in ... */
   e_border_adopt_children(0);

   ecore_ungrab();

   D_RETURN;
}

int
main(int argc, char **argv)
{
   char *display = NULL;
   int   i;

   srand(time(NULL));
   atexit(cb_exit);
   e_exec_set_args(argc, argv);
   
   e_config_init();

   /* Check command line options here: */
   for (i = 1; i < argc; i++)
     {
	if ((   (!strcmp("-d",        argv[i])) 
	     || (!strcmp("-disp",     argv[i]))
	     || (!strcmp("-display",  argv[i]))
	     || (!strcmp("--display", argv[i]))) 
	    && (argc - i > 1))
	  {
	     display = argv[++i];
	  }
	else if (   (!strcmp("-h",     argv[i]))
		 || (!strcmp("-?",     argv[i]))
		 || (!strcmp("-help",  argv[i]))
		 || (!strcmp("--help", argv[i])))
	  {
	     printf("enlightenment options:                           \n"
		    "\t[-d | -disp | -display --display] display_name \n"
		    "\t[-v | -version | --version]                    \n");
	     exit(0);
	  }
	else if (   (!strcmp("-v",        argv[i]))
		 || (!strcmp("-version",  argv[i]))
		 || (!strcmp("--version", argv[i])))
	  {
	     printf("Enlightenment Version: %s\n", ENLIGHTENMENT_VERSION);
	     exit(0);
	  }
     }
   
   if (!ecore_display_init(display))
     {
	fprintf(stderr, "Enlightenment Error: cannot connect to display!\n");
	fprintf(stderr, "Exiting Enlightenment. Error.\n");
	exit(-1);
     }

   /* Initialize signal handlers, clear ecore event handlers
    * and hook in ecore's X event handler. */
   ecore_event_signal_init();
   ecore_event_filter_init();
   ecore_event_x_init();
   
   /* become a wm */
   ecore_grab();
   ecore_sync();
   ecore_set_error_handler(wm_running_error);
   ecore_window_set_events(0, XEV_CHILD_REDIRECT | XEV_PROPERTY | XEV_COLORMAP);
   ecore_sync();
   ecore_reset_error_handler();
   ecore_ungrab();

   /* Initialization for the various modules: */

   e_fs_init();
   e_desktops_init();
   e_border_init();
   e_action_init();
   e_menu_init();
   e_view_machine_init();
   e_entry_init();
   e_keys_init();
   e_guides_init();
   e_place_init();
   e_cursors_init();
   e_iconbar_init();
   
#ifdef USE_FERITE
   e_ferite_init();
#endif

   e_desktops_init_file_display(e_desktops_get(0));   

   setup();
   
   ecore_event_loop();

#ifdef USE_FERITE
   e_ferite_deinit();
#endif

   return 0;
}
