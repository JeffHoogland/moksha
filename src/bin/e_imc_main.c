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
   char *set_ecore_imf_module = NULL;
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
	else if ((!strcmp(argv[i], "-set-ecore-imf-module")) && (i < (argc - 1)))
	  {
	     i++;
	     set_ecore_imf_module = argv[i];
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
	printf("ERROR: cannot open file %s for READ/WRITE (%i:%s)\n", 
               file, errno, strerror(errno));
	return -1;
     }

   /* If File Exists, Try to read imc */
   read_imc = e_intl_input_method_config_read(ef);
   
   /* else create new imc */
   if (write_ops != 0)
     {
	write_imc = calloc(sizeof(E_Input_Method_Config), 1);
	write_imc->version = E_INTL_INPUT_METHOD_CONFIG_VERSION;
	if (read_imc)
	  {
	     write_imc->e_im_name = read_imc->e_im_name;
	     write_imc->gtk_im_module = read_imc->gtk_im_module;
	     write_imc->qt_im_module = read_imc->qt_im_module;
	     write_imc->xmodifiers = read_imc->xmodifiers;
	     write_imc->ecore_imf_module = read_imc->ecore_imf_module;
	     write_imc->e_im_exec = read_imc->e_im_exec;
	     write_imc->e_im_setup_exec = read_imc->e_im_setup_exec;
	  }
	     
	if (set_name)	
	  write_imc->e_im_name = set_name;     
	if (set_gtk_im_module)     
	  write_imc->gtk_im_module = set_gtk_im_module;     
	if (set_qt_im_module)     
	  write_imc->qt_im_module = set_qt_im_module;
	if (set_xmodifiers)     
	  write_imc->xmodifiers = set_xmodifiers;
	if (set_ecore_imf_module)     
	  write_imc->ecore_imf_module = set_ecore_imf_module;
	if (set_exe)
	  write_imc->e_im_exec = set_exe;
	if (set_setup)
	  write_imc->e_im_setup_exec = set_setup;

	
	/* write imc to file */
	e_intl_input_method_config_write(ef, write_imc);
     }


   if (list)
     {
	printf("Config File List:\n");
	printf("Config Version:    %i\n", read_imc->version);
	printf("Config Name:       %s\n", read_imc->e_im_name);
	printf("Command Line:      %s\n", read_imc->e_im_exec);
	printf("Setup Line:        %s\n", read_imc->e_im_setup_exec);
	printf("gtk_im_module:     %s\n", read_imc->gtk_im_module);
	printf("qt_im_module:      %s\n", read_imc->qt_im_module);
	printf("xmodifiers:        %s\n", read_imc->xmodifiers);
	printf("ecore_imf_module:  %s\n", read_imc->ecore_imf_module);
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
	  "  -set-ecore-imf-module      Set the ecore_imf_module env var\n"
	  "  -set-xmodifiers            Set the xmodifiers env var\n"
	  "  -list                      List Contents of Input Method Config file\n"
	  );
}
