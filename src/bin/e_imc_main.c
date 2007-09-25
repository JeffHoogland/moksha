/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

static void _e_help(void);

/* externally accessible functions */

int
main(int argc, char **argv)
{
   int i = 0;
   int valid_args = 0;
   int write_ops = 0;
   Eet_File *ef = NULL;
   E_Input_Method_Config *write_imc = NULL;
   E_Input_Method_Config *read_imc = NULL;
   
   char *file = NULL;
   char *set_name = NULL;
   char *set_exe = NULL;
   char *set_setup = NULL;
   char *set_gtk_im_module = NULL;
   char *set_qt_im_module = NULL;
   char *set_xmodifiers = NULL;
   int list = 0;

   /* handle some command-line parameters */
   for (i = 1; i < argc; i++)
     {
	if ((!strcmp(argv[i], "-set-name")) && (i < (argc - 1)))
	  {
	     i++;
	     set_name = argv[i];
             valid_args++;
             write_ops++;
	  }
	else if ((!strcmp(argv[i], "-set-exe")) && (i < (argc - 1)))
	  {
	     i++;
	     set_exe = argv[i];
             valid_args++;
             write_ops++;
	  }
	else if ((!strcmp(argv[i], "-set-setup")) && (i < (argc - 1)))
	  {
	     i++;
	     set_setup = argv[i];
             valid_args++;
             write_ops++;
	  }
	else if ((!strcmp(argv[i], "-set-gtk-im-module")) && (i < (argc - 1)))
	  {
	     i++;
	     set_gtk_im_module = argv[i];
             valid_args++;
             write_ops++;
	  }
	else if ((!strcmp(argv[i], "-set-qt-im-module")) && (i < (argc - 1)))
	  {
	     i++;
	     set_qt_im_module = argv[i];
             valid_args++;
             write_ops++;
	  }
	else if ((!strcmp(argv[i], "-set-xmodifiers")) && (i < (argc - 1)))
	  {
	     i++;
	     set_xmodifiers = argv[i];
             valid_args++;
             write_ops++;
	  }
	else if ((!strcmp(argv[i], "-h")) ||
		 (!strcmp(argv[i], "-help")) ||
		 (!strcmp(argv[i], "--h")) ||
		 (!strcmp(argv[i], "--help")))
	  {
	     _e_help();
	     exit(0);
	  }
	else if ((!strcmp(argv[i], "-list")))
	  {
	     list = 1;
	     valid_args++;
	  }
	else
	  file = argv[i];
     }
   if (!file)
     {
	printf("ERROR: no file specified!\n");
	_e_help();
	exit(0);
     }

    if (valid_args == 0) {
        printf("ERROR: no valid arguments!\n");
        _e_help();
        exit(0);
    }

   eet_init();
   e_intl_data_init();
   
   if (write_ops != 0 && ecore_file_exists(file))
     {
	ef = eet_open(file, EET_FILE_MODE_READ_WRITE);
     }
   else if (write_ops != 0)
     {
	ef = eet_open(file, EET_FILE_MODE_WRITE);
     }
   else
     {
        ef = eet_open(file, EET_FILE_MODE_READ);
     }
   
   if (!ef)
     {
	printf("ERROR: cannot open file %s for READ/WRITE (%d:%s)\n", file, errno, strerror(errno));
	return -1;
     }

   /* If File Exists, Try to read imc */
   read_imc = e_intl_input_method_config_read(ef);
   
   /* else create new imc */
   if (write_ops != 0)
     {
	int write_ok;
	
	write_imc = calloc(sizeof(E_Input_Method_Config), 1);
	write_imc->version = E_INTL_INPUT_METHOD_CONFIG_VERSION;
	if (read_imc != NULL)
	  {
	     write_imc->e_im_name = read_imc->e_im_name;
	     write_imc->gtk_im_module = read_imc->gtk_im_module;
	     write_imc->qt_im_module = read_imc->qt_im_module;
	     write_imc->xmodifiers = read_imc->xmodifiers;
	     write_imc->e_im_exec = read_imc->e_im_exec;
	     write_imc->e_im_setup_exec = read_imc->e_im_setup_exec;
	  }
	     
	if (set_name != NULL)	
	  write_imc->e_im_name = set_name;     
	if (set_gtk_im_module != NULL)     
	  write_imc->gtk_im_module = set_gtk_im_module;     
	if (set_qt_im_module != NULL)     
	  write_imc->qt_im_module = set_qt_im_module;
	if (set_xmodifiers != NULL)     
	  write_imc->xmodifiers = set_xmodifiers;
	if (set_exe != NULL)
	  write_imc->e_im_exec = set_exe;
	if (set_setup != NULL)
	  write_imc->e_im_setup_exec = set_setup;

	
	/* write imc to file */
	write_ok = e_intl_input_method_config_write(ef, write_imc);
     }


   if (list)
     {
	printf("Config File List:\n");
	printf("Config Version:\t%d\n", read_imc->version);
	printf("Config Name:\t%s\n", read_imc->e_im_name);
	printf("Command Line:\t%s\n", read_imc->e_im_exec);
	printf("Setup Line:\t%s\n", read_imc->e_im_setup_exec);
	printf("gtk_im_module:\t%s\n", read_imc->gtk_im_module);
	printf("qt_im_module:\t%s\n", read_imc->qt_im_module);
	printf("xmodifiers:\t%s\n", read_imc->xmodifiers);
     }
   
   e_intl_input_method_config_free(read_imc);
   E_FREE(write_imc); 
   eet_close(ef);
   e_intl_data_shutdown();
   eet_shutdown();
   /* just return 0 to keep the compiler quiet */
   return 0;
}

static void
_e_help(void)
{
   printf("OPTIONS:\n"
	  "  -set-name NAME             Set the application name\n"
	  "  -set-exe EXE               Set the application execute line\n"
	  "  -set-setup EXE             Set the setup application execute line\n"
	  "  -set-gtk-im-module         Set the gtk_im_module env var\n"
	  "  -set-qt-im-module          Set the qt_im_module env var\n"
	  "  -set-xmodifiers            Set the xmodifiers env var\n"
	  "  -list                      List Contents of Input Method Config file\n"
	  );
}
