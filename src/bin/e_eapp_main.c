/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* FIXME: handle LANG!!!! */

static void _e_help(void);

/* externally accessible functions */
int
main(int argc, char **argv)
{
   int i = 0;
   int valid_args = 0;
   Eet_File *ef = NULL;
   char buf[4096];
   
   char *lang = NULL;
   int del_name = 0;
   int del_generic = 0;
   int del_comment = 0;
   int del_exe = 0;
   int del_win_name = 0;
   int del_win_class = 0;
   int del_win_title = 0;
   int del_startup_notify = 0;
   int del_wait_exit = 0;
   char *file = NULL;
   char *set_name = NULL;
   char *set_generic = NULL;
   char *set_comment = NULL;
   char *set_exe = NULL;
   char *set_win_name = NULL;
   char *set_win_class = NULL;
   char *set_win_title = NULL;
   int   set_startup_notify = -1;
   int   set_wait_exit = -1;
   
   /* handle some command-line parameters */
   for (i = 1; i < argc; i++)
     {
	if ((!strcmp(argv[i], "-lang")) && (i < (argc - 1)))
	  {
	     i++;
	     lang = argv[i];
             valid_args++;
	  }
	else if ((!strcmp(argv[i], "-set-name")) && (i < (argc - 1)))
	  {
	     i++;
	     set_name = argv[i];
             valid_args++;
	  }
	else if ((!strcmp(argv[i], "-set-generic")) && (i < (argc - 1)))
	  {
	     i++;
	     set_generic = argv[i];
             valid_args++;
	  }
	else if ((!strcmp(argv[i], "-set-comment")) && (i < (argc - 1)))
	  {
	     i++;
	     set_comment = argv[i];
             valid_args++;
	  }
	else if ((!strcmp(argv[i], "-set-exe")) && (i < (argc - 1)))
	  {
	     i++;
	     set_exe = argv[i];
             valid_args++;
	  }
	else if ((!strcmp(argv[i], "-set-win-name")) && (i < (argc - 1)))
	  {
	     i++;
	     set_win_name = argv[i];
             valid_args++;
	  }
	else if ((!strcmp(argv[i], "-set-win-class")) && (i < (argc - 1)))
	  {
	     i++;
	     set_win_class = argv[i];
             valid_args++;
	  }
	else if ((!strcmp(argv[i], "-set-win-title")) && (i < (argc - 1)))
	  {
	     i++;
	     set_win_title = argv[i];
             valid_args++;
	  }
	else if ((!strcmp(argv[i], "-set-startup-notify")) && (i < (argc - 1)))
	  {
	     i++;
	     set_startup_notify = atoi(argv[i]);
             valid_args++;
	  }
	else if ((!strcmp(argv[i], "-set-wait-exit")) && (i < (argc - 1)))
	  {
	     i++;
	     set_wait_exit = atoi(argv[i]);
             valid_args++;
	  }
	else if ((!strcmp(argv[i], "-del-all")))
	  {
	     del_name = 1;
	     del_generic = 1;
	     del_comment = 1;
	     del_exe = 1;
	     del_win_name = 1;
	     del_win_class = 1;
	     del_win_title = 1;
	     del_startup_notify = 1;
	     del_wait_exit = 1;
             valid_args++;
	  }
	else if ((!strcmp(argv[i], "-del-name")))
	  {
	     del_name = 1;
             valid_args++;
	  }
	else if ((!strcmp(argv[i], "-del-generic")))
	  {
	     del_generic = 1;
             valid_args++;
	  }
	else if ((!strcmp(argv[i], "-del-comment")))
	  {
	     del_comment = 1;
             valid_args++;
	  }
	else if ((!strcmp(argv[i], "-del-exe")))
	  {
	     del_exe = 1;
             valid_args++;
	  }
	else if ((!strcmp(argv[i], "-del-win-name")))
	  {
	     del_win_name = 1;
             valid_args++;
	  }
	else if ((!strcmp(argv[i], "-del-win-class")))
	  {
	     del_win_class = 1;
             valid_args++;
	  }
	else if ((!strcmp(argv[i], "-del-win-title")))
	  {
	     del_win_title = 1;
             valid_args++;
	  }
	else if ((!strcmp(argv[i], "-del-startup-notify")))
	  {
	     del_startup_notify = 1;
             valid_args++;
	  }
	else if ((!strcmp(argv[i], "-del-wait-exit")))
	  {
	     del_wait_exit = 1;
             valid_args++;
	  }
	else if ((!strcmp(argv[i], "-h")) ||
		 (!strcmp(argv[i], "-help")) ||
		 (!strcmp(argv[i], "--h")) ||
		 (!strcmp(argv[i], "--help")))
	  {
	     _e_help();
	     exit(0);
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
   ef = eet_open(file, EET_FILE_MODE_READ_WRITE);
   if (!ef)
     {
	printf("ERROR: cannot open file %s for READ/WRITE\n", file);
	return -1;
     }
   if (set_name)
     {
	if (lang) snprintf(buf, sizeof(buf), "app/info/name[%s]", lang);
	else snprintf(buf, sizeof(buf), "app/info/name");
	eet_write(ef, buf, set_name, strlen(set_name), 0);
     }
   if (set_generic)
     {
	if (lang) snprintf(buf, sizeof(buf), "app/info/generic[%s]", lang);
	else snprintf(buf, sizeof(buf), "app/info/generic");
	eet_write(ef, buf, set_generic, strlen(set_generic), 0);
     }
   if (set_comment)
     {
	if (lang) snprintf(buf, sizeof(buf), "app/info/comment[%s]", lang);
	else snprintf(buf, sizeof(buf), "app/info/comment");
	eet_write(ef, buf, set_comment, strlen(set_comment), 0);
     }
   if (set_exe)
     eet_write(ef, "app/info/exe", set_exe, strlen(set_exe), 0);
   if (set_win_name)
     eet_write(ef, "app/window/name", set_win_name, strlen(set_win_name), 0);
   if (set_win_class)
     eet_write(ef, "app/window/class", set_win_class, strlen(set_win_class), 0);
   if (set_win_title)
     eet_write(ef, "app/window/title", set_win_title, strlen(set_win_title), 0);
   if (set_startup_notify >= 0)
     {
	unsigned char tmp[1];
	
	tmp[0] = set_startup_notify;
	if (set_startup_notify)
	  eet_write(ef, "app/info/startup_notify", tmp, 1, 0);
	else
	  eet_write(ef, "app/info/startup_notify", tmp, 1, 0);
     }
   if (set_wait_exit >= 0)
     {
	unsigned char tmp[1];
	
	tmp[0] = set_wait_exit;
	if (set_wait_exit)
	  eet_write(ef, "app/info/wait_exit", tmp, 1, 0);
	else
	  eet_write(ef, "app/info/wait_exit", tmp, 1, 0);
     }
   if (del_name)
     {
	char **matches = NULL;
	int match_num = 0;
	
	matches = eet_list(ef, "app/info/name*", &match_num);
	if (matches)
	  {
	     for (i = 0; i < match_num; i++) eet_delete(ef, matches[i]);
	     free(matches);
	  }
     }
   if (del_generic)
     {
	char **matches = NULL;
	int match_num = 0;
	
	matches = eet_list(ef, "app/info/generic*", &match_num);
	if (matches)
	  {
	     for (i = 0; i < match_num; i++) eet_delete(ef, matches[i]);
	     free(matches);
	  }
     }
   if (del_comment)
     {
	char **matches = NULL;
	int match_num = 0;
	
	matches = eet_list(ef, "app/info/comment*", &match_num);
	if (matches)
	  {
	     for (i = 0; i < match_num; i++) eet_delete(ef, matches[i]);
	     free(matches);
	  }
     }
   if (del_exe)
     eet_delete(ef, "app/info/exe");
   if (del_win_name)
     eet_delete(ef, "app/window/name");
   if (del_win_class)
     eet_delete(ef, "app/window/class");
   if (del_win_title)
     eet_delete(ef, "app/window/title");
   if (del_startup_notify)
     eet_delete(ef, "app/info/startup_notify");
   if (del_wait_exit)
     eet_delete(ef, "app/info/wait_exit");
   
   eet_close(ef);
   eet_shutdown();
   /* just return 0 to keep the compiler quiet */
   return 0;
}

static void
_e_help(void)
{
   printf("OPTIONS:\n"
	  "  -lang LANG                 Set the language properties to modify\n"
	  "  -set-name NAME             Set the application name\n"
	  "  -set-generic GENERIC       Set the application generic name\n"
	  "  -set-comment COMMENT       Set the application comment\n"
	  "  -set-exe EXE               Set the application execute line\n"
	  "  -set-win-name WIN_NAME     Set the application window name\n"
	  "  -set-win-class WIN_CLASS   Set the application window class\n"
	  "  -set-win-title WIN_TITLE   Set the application window title glob\n"
	  "  -set-startup-notify [1/0]  Set the application startup notify flag\n"
	  "  -set-wait-exit [1/0]       Set the application wait exit flag\n"
	  "  -del-name                  Delete the application name\n"
	  "  -del-generic               Delete the application generic name\n"
	  "  -del-comment               Delete the application comment\n"
	  "  -del-exe                   Delete the application execute line\n"
	  "  -del-win-name              Delete the application window name\n"
	  "  -del-win-class             Delete the application window class\n"
	  "  -del-win-title             Delete the application window title glob\n"
	  "  -del-startup-notify        Delete the application startup notify flag\n"
	  "  -del-wait-exit             Delete the application wait exit flag\n"
	  );
}
