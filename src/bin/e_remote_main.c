#include "e.h"

/* local subsystem functions */
static int _e_cb_signal_exit(void *data, int ev_type, void *ev);
static int _e_ipc_init(void);

/* local subsystem globals */

/* externally accessible functions */
int
main(int argc, char **argv)
{
   int i;
   
   /* handle some command-line parameters */
   for (i = 1; i < argc; i++)
     {
	if ((!strcmp(argv[i], "-instance")) && (i < (argc - 1)))
	  {
	     i++;
	  }
     }
  
   /* basic ecore init */
   if (!ecore_init())
     {
	printf("Enlightenment cannot Initialize Ecore!\n"
	       "Perhaps you are out of memory?\n");
	exit(-1);
     }
   ecore_app_args_set((int)argc, (const char **)argv);
   /* setup a handler for when e is asked to exit via a system signal */
   if (!ecore_event_handler_add(ECORE_EVENT_SIGNAL_EXIT, _e_cb_signal_exit, NULL))
     {
	printf("Enlightenment cannot set up an exit signal handler.\n"
	       "Perhaps you are out of memory?\n");
	exit(-1);
     }
   /* init ipc */
   if (!ecore_ipc_init())
     {
	printf("Enlightenment cannot initialize the ipc system.\n"
	       "Perhaps you are out of memory?\n");
	exit(-1);
     }
   
   /* setup e ipc service */
   if (!_e_ipc_init())
     {
	printf("Enlightenment cannot set up the IPC socket.\n"
	       "It likely is already in use by an exisiting copy of Enlightenment.\n"
	       "Double check to see if Enlightenment is not already on this display,\n"
	       "but if that fails try deleting all files in ~/.ecore/enlightenment-*\n"
	       "and try running again.\n");
	exit(-1);
     }

   /* start our main loop */
   ecore_main_loop_begin();
   
   /* just return 0 to keep the compiler quiet */
   return 0;
}

/* local subsystem functions */
static int
_e_cb_signal_exit(void *data, int ev_type, void *ev)
{
   /* called on ctrl-c, kill (pid) (also SIGINT, SIGTERM and SIGQIT) */
   ecore_main_loop_quit();
   return 1;
}

static int
_e_ipc_init(void)
{
   return 0;
}
