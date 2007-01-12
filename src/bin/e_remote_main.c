/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

#ifdef USE_IPC

typedef struct _Opt Opt;

struct _Opt
{
   char     *opt;
   int       num_param;
   char     *desc;
   int       num_reply;
   E_Ipc_Op  opcode;
};

Opt opts[] = {
#define TYPE  E_REMOTE_OPTIONS
#include      "e_ipc_handlers.h"
#undef TYPE
};

static int _e_cb_signal_exit(void *data, int ev_type, void *ev);
static int _e_ipc_init(void);
static void _e_ipc_shutdown(void);
static int _e_ipc_cb_server_add(void *data, int type, void *event);
static int _e_ipc_cb_server_del(void *data, int type, void *event);
static int _e_ipc_cb_server_data(void *data, int type, void *event);
static void _e_help(void);

/* local subsystem globals */
static Ecore_Ipc_Server *_e_ipc_server  = NULL;
static const char *display_name = NULL;
static int reply_count = 0;
static int reply_expect = 0;

#endif

int
main(int argc, char **argv)
{
#ifdef USE_IPC
   int i;
   
   /* handle some command-line parameters */
   display_name = getenv("DISPLAY");
   for (i = 1; i < argc; i++)
     {
	if ((!strcmp(argv[i], "-h")) ||
	    (!strcmp(argv[i], "-help")) ||
	    (!strcmp(argv[i], "--h")) ||
	    (!strcmp(argv[i], "--help")))
	  {
	     _e_help();
	     exit(0);
	  }
     }
  
   /* basic ecore init */
   if (!ecore_init())
     {
	printf("ERROR: Enlightenment_remote cannot Initialize Ecore!\n"
	       "Perhaps you are out of memory?\n");
	exit(-1);
     }
   ecore_app_args_set(argc, (const char **)argv);
   /* setup a handler for when e is asked to exit via a system signal */
   if (!ecore_event_handler_add(ECORE_EVENT_SIGNAL_EXIT, _e_cb_signal_exit, NULL))
     {
	printf("ERROR: Enlightenment_remote cannot set up an exit signal handler.\n"
	       "Perhaps you are out of memory?\n");
	exit(-1);
     }
   /* init ipc */
   if (!ecore_ipc_init())
     {
	printf("ERROR: Enlightenment_remote cannot initialize the ipc system.\n"
	       "Perhaps you are out of memory?\n");
	exit(-1);
     }
   
   /* setup e ipc service */
   if (!_e_ipc_init())
     {
	printf("ERROR: Enlightenment_remote cannot set up the IPC socket.\n"
	       "Maybe try the '-display :0.0' option?\n");
	exit(-1);
     }
   e_ipc_codec_init();

   /* start our main loop */
   ecore_main_loop_begin();

   e_ipc_codec_shutdown();
   _e_ipc_shutdown();
   ecore_ipc_shutdown();
   ecore_shutdown();
#endif
   
   /* just return 0 to keep the compiler quiet */
   return 0;
}

#ifdef USE_IPC
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
   char *sdir;
   
   sdir = getenv("E_IPC_SOCKET");
   if (!sdir)
     {
	printf("The E_IPC_SOCKET environment variable is not set. This is\n"
	       "exported by Enlightenment to all processes it launches.\n"
	       "This environment variable must be set and must point to\n"
	       "Enlightenment's IPC socket file (minus port number).\n");
	return 0;
     }
   _e_ipc_server = ecore_ipc_server_connect(ECORE_IPC_LOCAL_SYSTEM, sdir, 0, NULL);
   if (!_e_ipc_server) return 0;
   
   ecore_event_handler_add(ECORE_IPC_EVENT_SERVER_ADD, _e_ipc_cb_server_add, NULL);
   ecore_event_handler_add(ECORE_IPC_EVENT_SERVER_DEL, _e_ipc_cb_server_del, NULL);
   ecore_event_handler_add(ECORE_IPC_EVENT_SERVER_DATA, _e_ipc_cb_server_data, NULL);
   
   return 1;
}

static void
_e_ipc_shutdown(void)
{
   if (_e_ipc_server)
     {
	ecore_ipc_server_del(_e_ipc_server);
	_e_ipc_server = NULL;
     }
}

static int
_e_ipc_cb_server_add(void *data, int type, void *event)
{
   Ecore_Ipc_Event_Server_Add *e;
   char **argv, **params;
   int i, j, argc, process_count = 0;
   
   e = event;
   ecore_app_args_get(&argc, &argv);
   for (i = 1; i < argc; i++)
     {
	for (j = 0; j < (int)(sizeof(opts) / sizeof(Opt)); j++)
	  {
	     Opt *opt;
	     
	     opt = &(opts[j]);
	     if (!strcmp(opt->opt, argv[i]))
	       {
		  if (i >= (argc - opt->num_param))
		    {
		       printf("ERROR: option %s expects %i parameters\n",
			      opt->opt, opt->num_param);
		       exit(-1);
		    }
		  else
		    {
		       params = &(argv[i + 1]);
		       switch (opt->opcode)
			 {
#define TYPE  E_REMOTE_OUT
#include      "e_ipc_handlers.h"
#undef TYPE
			  default:
			    break;
			 }
		       process_count++;
		       reply_expect += opt->num_reply;
		       i += opt->num_param;
		    }
	       }
	  }
     }
   if (process_count <= 0) _e_help();
   if (reply_count >= reply_expect) ecore_main_loop_quit();
   return 1;
}

static int
_e_ipc_cb_server_del(void *data, int type, void *event)
{
   Ecore_Ipc_Event_Server_Del *e;
   
   e = event;
   return 1; 
}

static int
_e_ipc_cb_server_data(void *data, int type, void *event)
{
   Ecore_Ipc_Event_Server_Data *e;
   
   e = event;
   printf("REPLY <- BEGIN\n");
   switch (e->minor)
     {
#define TYPE  E_REMOTE_IN
#include      "e_ipc_handlers.h"
#undef TYPE
      default:
	 break;
     }
   printf("REPLY <- END\n");
   reply_count++;
   if (reply_count >= reply_expect) ecore_main_loop_quit();
   return 1;
}

static void
_e_help(void)
{
   int i, j;
   
   printf("OPTIONS:\n");
   printf("  -h This help\n");
   printf("  -help This help\n");
   printf("  --help This help\n");
   printf("  --h This help\n");
   for (j = 0; j < (int)(sizeof(opts) / sizeof(Opt)); j++)
     {
	Opt *opt;
	
	opt = &(opts[j]);
	printf("  %s ", opt->opt);
	for (i = 0; i < opt->num_param; i++)
	  printf("OPT%i ", i + 1);
	printf("%s\n", opt->desc);
     }
}
#endif
