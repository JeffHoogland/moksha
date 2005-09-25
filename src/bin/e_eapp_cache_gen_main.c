/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

static void _e_gen_cache(char *path, int recurse);
static int _e_cb_signal_exit(void *data, int ev_type, void *ev);
static void _e_help(void);

/* local subsystem globals */

int
main(int argc, char **argv)
{
   int i;
   char *dir = NULL;
   int recurse = 0;
   
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
	else if (!strcmp(argv[i], "-r"))
	  {
	     recurse = 1;
	  }
	else
	  {
	     dir = argv[i];
	  }
     }
   if (!dir)
     {
	printf("ERROR: No directory specified!\n");
	_e_help();
	exit(0);
     }
  
   /* basic ecore init */
   if (!ecore_init())
     {
	printf("ERROR: Enlightenment_eapp_cache_gen cannot Initialize Ecore!\n"
	       "Perhaps you are out of memory?\n");
	exit(-1);
     }
   ecore_app_args_set((int)argc, (const char **)argv);
   /* setup a handler for when e is asked to exit via a system signal */
   if (!ecore_event_handler_add(ECORE_EVENT_SIGNAL_EXIT, _e_cb_signal_exit, NULL))
     {
	printf("ERROR: Enlightenment_eapp_cache_gen cannot set up an exit signal handler.\n"
	       "Perhaps you are out of memory?\n");
	exit(-1);
     }

   e_app_init();
   _e_gen_cache(dir, recurse);
   e_app_shutdown();
   
   ecore_shutdown();
   /* just return 0 to keep the compiler quiet */
   return 0;
}

static void
_e_gen_cache(char *path, int recurse)
{
   E_App_Cache *ac;
   E_App *a;
   char buf[PATH_MAX], *file;
   
   a = e_app_new(path, recurse);
   if (!a) return;
   ac = e_app_cache_generate(a);
   if (!ac) return;
   e_app_cache_save(ac, path);
   e_app_cache_free(ac);
   if (recurse)
     {
	Ecore_List *files;
	
	files = ecore_file_ls(path);
	if (files)
	  {
	     ecore_list_goto_first(files);
	     while ((file = ecore_list_next(files)))
	       {
		  if (file[0] != '.')
		    {
		       snprintf(buf, sizeof(buf), "%s/%s", path, file);
		       if (ecore_file_is_dir(buf)) _e_gen_cache(buf, recurse);
		    }
	       }
	     ecore_list_destroy(files);
	  }
     }
   return;
}

/* local subsystem functions */
static int
_e_cb_signal_exit(void *data, int ev_type, void *ev)
{
   /* called on ctrl-c, kill (pid) (also SIGINT, SIGTERM and SIGQIT) */
   exit(-1);
   return 1;
}

static void
_e_help(void)
{
   printf("enlightenment_eapp_cache_gen [directory] [OPTIONS]\n"
	  "\n"
	  "OPTIONS:\n"
	  "  -h     This help\n"
	  "  -help  This help\n"
	  "  --help This help\n"
	  "  --h    This help\n"
	  "  -r     Recursively generate caches for all subdirectories too\n"
	  );
}


