#include "e.h"
#include <Ecore_Getopt.h>
#include <limits.h>
#include <errno.h>

#ifdef USE_IPC
static void
_ipc_socket_help(void)
{
   fputs("The E_IPC_SOCKET environment variable is not set or invalid.\n"
	 "This is exported by Enlightenment to all processes it launches.\n"
	 "This environment variable must be set and must point to "
	 "Enlightenment's IPC socket file (minus '|' and port number).\n",
	 stderr);
}

static int
_ipc_server_add(void *data, int type, void *event)
{
   Ecore_Ipc_Event_Server_Add *e = event;
   const char *directory = data;
   void *d;
   int len;

   d = e_ipc_codec_2str_enc("fileman", directory, &len);
   if (!d)
     {
	fputs("ERROR: could not encode IPC command.\n", stderr);
	return 1;
     }

   ecore_ipc_server_send
     (e->server, E_IPC_DOMAIN_REQUEST, E_IPC_OP_EXEC_ACTION, 0, 0, 0, d, len);
   free(d);

   return 1;
}

static int
_ipc_server_data(void *data, int type, void *event)
{
   Ecore_Ipc_Event_Server_Data *e = event;
   int *exit_code = data;

   switch (e->minor)
     {
      case E_IPC_OP_EXEC_ACTION_REPLY:
	 if (e->data)
	   {
	      if (!e_ipc_codec_int_dec(e->data, e->size, exit_code))
		fputs("ERROR: could not decode integer from reply.\n", stderr);
	   }
	 ecore_main_loop_quit();
	 break;
      default:
	 fprintf(stderr, "ERROR: unexpected reply from server with minor %d, "
		 "expected %d\n", e->minor, E_IPC_OP_EXEC_ACTION);
	 break;
     }
   return 1;
}


static Ecore_Ipc_Server *
_ipc_init(void)
{
   char *spath;

   spath = getenv("E_IPC_SOCKET");
   if (!spath)
     return NULL;

   return ecore_ipc_server_connect(ECORE_IPC_LOCAL_SYSTEM, spath, 0, NULL);
}

static void
_ipc_shutdown(Ecore_Ipc_Server *s)
{
   if (!s)
     return;

   ecore_ipc_server_del(s);
}

static int
_signal_exit(void *data, int ev_type, void *ev)
{
   ecore_main_loop_quit();
   return 1;
}

static const Ecore_Getopt options = {
    "enlightenment_fm_open",
    "%prog <directory>",
    PACKAGE_VERSION,
    "(C) 2008 Enlightenment, see AUTHORS.",
    "BSD with advertisement, see COPYING.",
    "Open directories with Enlightenment File Manager.",
    1,
    {
      ECORE_GETOPT_VERSION('V', "version"),
      ECORE_GETOPT_COPYRIGHT('R', "copyright"),
      ECORE_GETOPT_LICENSE('L', "license"),
      ECORE_GETOPT_HELP('h', "help"),
      ECORE_GETOPT_SENTINEL
    }
};
#endif /* USE_IPC */

int
main(int argc, char **argv)
{
#ifdef USE_IPC
   unsigned char exit_option = 0;
   Ecore_Getopt_Value values[] = {
     ECORE_GETOPT_VALUE_BOOL(exit_option),
     ECORE_GETOPT_VALUE_BOOL(exit_option),
     ECORE_GETOPT_VALUE_BOOL(exit_option),
     ECORE_GETOPT_VALUE_BOOL(exit_option),
     ECORE_GETOPT_VALUE_NONE
   };
   int nonargs, action_ret, ret = -1;
   const char *arg;
   char directory[PATH_MAX];
   Ecore_Ipc_Server *server = NULL;

   if (ecore_init() <= 0)
     {
	fputs("ERROR: could not initialize ecore!\n", stderr);
	return -1;
     }

   if (ecore_ipc_init() <= 0)
     {
	fputs("ERROR: could not initialize ecore_ipc!\n", stderr);
	goto failed_ecore_ipc;
     }

   ecore_app_args_set(argc, (const char **)argv);
   nonargs = ecore_getopt_parse(&options, values, argc, argv);
   if (nonargs < 0)
     {
	ret = nonargs;
	goto failed_args;
     }

   if (exit_option)
     goto found_exit_option;

   if (nonargs == argc)
     {
	fputs("WARNING: missing directory to open, using $HOME\n", stderr);
	arg = getenv("HOME");
	if (!arg)
	  {
	     fputs("ERROR: no directory to open and no $HOME.\n", stderr);
	     goto missing_args;
	  }
     }
   else
     {
	arg = argv[nonargs];

	if (strcmp(arg, "file://") == 0)
	  arg += sizeof("file://") - 1;
     }

   if (realpath(arg, directory) == NULL)
     {
	fprintf(stderr, "ERROR: invalid directory \"%s\": %s\n",
		directory, strerror(errno));
	goto missing_args;
     }

   server = _ipc_init();
   if (!server)
     {
	fputs("ERROR: could not connect to server.\n", stderr);
	goto failed_ipc;
     }

   if (e_ipc_codec_init() <= 0)
     {
	fputs("ERROR: could not initialize e_ipc_codec!\n", stderr);
	goto failed_ipc_codec;
     }

   action_ret = 0;

   ecore_event_handler_add(ECORE_EVENT_SIGNAL_EXIT, _signal_exit, NULL);
   ecore_event_handler_add(ECORE_IPC_EVENT_SERVER_ADD, _ipc_server_add, directory);
   ecore_event_handler_add(ECORE_IPC_EVENT_SERVER_DATA, _ipc_server_data, &action_ret);

   ecore_main_loop_begin();

   ret = action_ret ? 0 : -1;

   e_ipc_codec_shutdown();
 failed_ipc_codec:
   _ipc_shutdown(server);
 failed_ipc:
 missing_args:
 found_exit_option:
 failed_args:
   ecore_ipc_shutdown();
 failed_ecore_ipc:
   ecore_shutdown();
   return ret;
#else /* USE_IPC */
   fputs("ERROR: this application cannot work without ecore IPC.\n", stderr);
   return 0;
#endif
}
