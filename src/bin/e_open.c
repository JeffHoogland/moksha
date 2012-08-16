# ifdef HAVE_CONFIG_H
#  include "config.h"
# endif

#include <Ecore_Getopt.h>
#include <Efreet.h>
#include <Efreet_Mime.h>
#include <unistd.h>
#include <ctype.h>

static const char *
xdg_defaults_get(const char *path, const char *mime)
{
   Efreet_Ini *ini;
   const char *str;

   if (access(path, R_OK) != 0)
     return NULL;

   ini = efreet_ini_new(path);
   if (!ini)
     return NULL;

   efreet_ini_section_set(ini, "Default Applications");
   str = eina_stringshare_add(efreet_ini_string_get(ini, mime));
   efreet_ini_free(ini);

   return str;
}

static Efreet_Desktop *
xdg_desktop_from_string_list(const char *strlist)
{
   Efreet_Desktop *desktop = NULL;
   char **array = eina_str_split(strlist, ";", 0);
   unsigned int i;

   if (!array)
     return NULL;

   for (i = 0; array[i] != NULL; i++)
     {
        const char *name = array[i];
        if (name[0] == '/')
          desktop = efreet_desktop_new(name);
        else
          desktop = efreet_util_desktop_file_id_find(name);

        if (desktop)
          {
             if (desktop->exec) break;
             else
               {
                  efreet_desktop_free(desktop);
                  desktop = NULL;
               }
          }
     }

   free(array[0]);
   free(array);

   return desktop;
}

static Efreet_Desktop *
desktop_first_free_others(Eina_List *lst)
{
   Efreet_Desktop *desktop, *d;
   if (!lst)
     return NULL;

   desktop = lst->data;
   efreet_desktop_ref(desktop);

   EINA_LIST_FREE(lst, d)
     efreet_desktop_free(d);

   return desktop;
}

static Efreet_Desktop *
handler_find(const char *mime)
{
   Efreet_Desktop *desktop = NULL;
   char path[PATH_MAX];
   const char *name;

   snprintf(path, sizeof(path), "%s/applications/defaults.list",
            efreet_data_home_get());
   name = xdg_defaults_get(path, mime);
   if (!name)
     {
        const Eina_List *n, *dirs = efreet_data_dirs_get();
        const char *d;
        EINA_LIST_FOREACH(dirs, n, d)
          {
             snprintf(path, sizeof(path), "%s/applications/defaults.list", d);
             name = xdg_defaults_get(path, mime);
             if (name)
               break;
          }
     }

   if (name)
     {
        desktop = xdg_desktop_from_string_list(name);
        eina_stringshare_del(name);
     }

   if (!desktop)
     desktop = desktop_first_free_others(efreet_util_desktop_mime_list(mime));

   return desktop;
}

static void *
get_command(void *data, Efreet_Desktop *desktop __UNUSED__, char *command, int remaining __UNUSED__)
{
   Eina_List **p_cmd = data;
   *p_cmd = eina_list_append(*p_cmd, command);
   return NULL;
}

static char **
mime_open(const char *mime, const char * const *argv, int argc)
{
   Efreet_Desktop *desktop = handler_find(mime);
   Eina_List *files = NULL;
   Eina_List *cmds = NULL;
   char **ret;

   if (!desktop)
     return NULL;

   for (; argc > 0; argc--, argv++)
     files = eina_list_append(files, *argv);

   efreet_desktop_command_get(desktop, files, get_command, &cmds);

   if (!cmds) ret = NULL;
   else
     {
        char *c;

        ret = calloc(eina_list_count(cmds) + 1, sizeof(char *));
        if (ret)
          {
             unsigned int i = 0;
             EINA_LIST_FREE(cmds, c)
               {
                  ret[i] = c; /* was strdup by efreet_desktop_command_get() */
                  i++;
               }
             ret[i] = NULL;
          }
        else
          {
             EINA_LIST_FREE(cmds, c)
               free(c);
          }
     }

   eina_list_free(files);

   return ret;
}

static void
append_single_quote_escaped(Eina_Strbuf *b, const char *str)
{
   const char *itr = str;
   for (; *itr != '\0'; itr++)
     {
        if (*itr != '\'')
          eina_strbuf_append_char(b, *itr);
        else
          eina_strbuf_append(b, "\\'");
     }
}

static char **
single_command_open(const char *command, const char * const *argv, int argc)
{
   char **ret = calloc(2, sizeof(char *));
   Eina_Strbuf *b;
   int i;

   if (!ret)
     return NULL;

   b = eina_strbuf_new();
   if (!b)
     {
        free(ret);
        return NULL;
     }
   eina_strbuf_append(b, command);

   for (i = 0; i < argc; i++)
     {
        Eina_Bool has_space = EINA_FALSE;
        int s_idx_sq = -1, s_idx_dq = -1;
        int l_idx_sq = -1, l_idx_dq = -1;
        int idx;
        const char *itr;

        for (idx = 0, itr = argv[i]; *itr != '\0'; itr++, idx++)
          {
             if ((!has_space) && (isspace(*itr)))
               has_space = EINA_TRUE;

             if (*itr == '\'')
               {
                  l_idx_sq = idx;
                  if (s_idx_sq < 0)
                    s_idx_sq = idx;
               }

             if (*itr == '\'')
               {
                  l_idx_dq = idx;
                  if (s_idx_dq < 0)
                    s_idx_dq = idx;
               }
          }
        idx--;

        eina_strbuf_append_char(b, ' ');
        if ((!has_space) ||
            ((s_idx_sq == 0) && (l_idx_sq == idx)) ||
            ((s_idx_dq == 0) && (l_idx_dq == idx)))
          eina_strbuf_append(b, argv[i]);
        else
          {
             char c;
             if ((s_idx_sq >= 0) && (s_idx_dq < 0))
               c = '"';
             else if ((s_idx_sq < 0) && (s_idx_dq >= 0))
               c = '\'';
             else
               c = 0;

             if (c)
               {
                  eina_strbuf_append_char(b, c);
                  eina_strbuf_append(b, argv[i]);
                  eina_strbuf_append_char(b, c);
               }
             else
               {
                  eina_strbuf_append_char(b, '\'');
                  append_single_quote_escaped(b, argv[i]);
                  eina_strbuf_append_char(b, '\'');
               }
          }
     }

   ret[0] = eina_strbuf_string_steal(b);
   eina_strbuf_free(b);

   return ret;
}

static Efreet_Desktop *
_terminal_get(const char *defaults_list)
{
   Efreet_Desktop *tdesktop = NULL;
   Efreet_Ini *ini;
   const char *s;
   
   ini = efreet_ini_new(defaults_list);
   if ((ini) && (ini->data) &&
       (efreet_ini_section_set(ini, "Default Applications")) &&
       (ini->section))
     {
        s = efreet_ini_string_get(ini, "x-scheme-handler/terminal");
        if (s) tdesktop = efreet_util_desktop_file_id_find(s);
     }
   if (ini) efreet_ini_free(ini);
   return tdesktop;
}

static char **
terminal_open(void)
{
   const char *terms[] =
     {
        "terminology.desktop",
        "xterm.desktop",
        "rxvt.desktop",
        "gnome-terimnal.desktop",
        "konsole.desktop",
        NULL
     };
   const char *s;
   char buf[PATH_MAX], **ret;
   Efreet_Desktop *tdesktop = NULL, *td;
   Eina_List *l;
   int i;
   
   s = efreet_data_home_get();
   if (s)
     {
        snprintf(buf, sizeof(buf), "%s/applications/defaults.list", s);
        tdesktop = _terminal_get(buf);
     }
   if (tdesktop) goto have_desktop;
   EINA_LIST_FOREACH(efreet_data_dirs_get(), l, s)
     {
        snprintf(buf, sizeof(buf), "%s/applications/defaults.list", s);
        tdesktop = _terminal_get(buf);
        if (tdesktop) goto have_desktop;
     }
   
   for (i = 0; terms[i]; i++)
     {
        tdesktop = efreet_util_desktop_file_id_find(terms[i]);
        if (tdesktop) goto have_desktop;
     }
   if (!tdesktop)
     {
        l = efreet_util_desktop_category_list("TerminalEmulator");
        if (l)
          {
             // just take first one since above list doesn't work.
             tdesktop = l->data;
             EINA_LIST_FREE(l, td)
               {
                  // free/unref the desktosp we are not going to use
                  if (td != tdesktop) efreet_desktop_free(td);
               }
          }
     }
   if (!tdesktop) return NULL;
have_desktop:
   if (!tdesktop->exec)
     {
        efreet_desktop_free(tdesktop);
        return NULL;
     }
   ret = malloc(sizeof(char *) * 2);
   if (!ret) return NULL;
   ret[0] = strdup(tdesktop->exec);
   ret[1] = NULL;
   if (!ret[0])
     {
        free(ret);
        efreet_desktop_free(tdesktop);
        return NULL;
     }
   efreet_desktop_free(tdesktop);
   return ret;
}

static char **
browser_open(const char * const *argv, int argc)
{
   const char *env = getenv("BROWSER");
   if (env) return single_command_open(env, argv, argc);
   return mime_open("x-scheme-handler/http", argv, argc);
}

static char **
local_open(const char *path)
{
   const char *mime = efreet_mime_type_get(path);
   if (mime)
     {
        char **ret = mime_open(mime, &path, 1);
        if (ret)
          return ret;
        return single_command_open("enlightenment_filemanager", &path, 1);
     }

   fprintf(stderr, "ERROR: Could not get mime type for: %s\n", path);
   return NULL;
}

static char **
protocol_open(const char *str)
{
   Efreet_Uri *uri = efreet_uri_decode(str);
   char **ret = NULL;

   if (!uri)
     {
        fprintf(stderr, "ERROR: Could not decode uri: %s\n", str);
        return NULL;
     }

   if (!uri->protocol)
     fprintf(stderr, "ERROR: Could not get protocol from uri: %s\n", str);
   else if (strcmp(uri->protocol, "file") == 0)
     ret = local_open(uri->path);
   else
     {
        char mime[256];
        snprintf(mime, sizeof(mime), "x-scheme-handler/%s", uri->protocol);
        ret = mime_open(mime, &str, 1);
     }
   efreet_uri_free(uri);
   return ret;
}

static const struct type_mime {
   const char *type;
   const char *mime;
} type_mimes[] = {
  /* {"browser", "x-scheme-handler/http"}, */
  {"mail", "x-scheme-handler/mailto"},
  /*  {"terminal", NULL}, */
  {"filemanager", "x-scheme-handler/file"},
  {"image", "image/jpeg"},
  {"video", "video/x-mpeg"},
  {"music", "audio/mp3"},
  {NULL, NULL}
};

static const char *type_choices[] = {
  "browser",
  "mail",
  "terminal",
  "filemanager",
  "image",
  "video",
  "music",
  NULL
};

static const Ecore_Getopt options = {
   "enlightenment_open",
   "%prog [options] <file-or-folder-or-url>",
   PACKAGE_VERSION,
   "(C) 2012 Gustavo Sverzut Barbieri and others",
   "BSD 2-Clause",
   "Opens the file using Enlightenment standards.",
   EINA_FALSE,
   {
      ECORE_GETOPT_CHOICE('t', "type", "Choose program type to launch.",
                          type_choices),
      ECORE_GETOPT_VERSION('V', "version"),
      ECORE_GETOPT_COPYRIGHT('C', "copyright"),
      ECORE_GETOPT_LICENSE('L', "license"),
      ECORE_GETOPT_HELP('h', "help"),
      ECORE_GETOPT_SENTINEL
   }
};

EAPI int
main(int argc, char *argv[])
{
   Eina_Bool quit_option = EINA_FALSE;
   char *type = NULL;
   Ecore_Getopt_Value values[] = {
     ECORE_GETOPT_VALUE_STR(type),
     ECORE_GETOPT_VALUE_BOOL(quit_option),
     ECORE_GETOPT_VALUE_BOOL(quit_option),
     ECORE_GETOPT_VALUE_BOOL(quit_option),
     ECORE_GETOPT_VALUE_BOOL(quit_option),
     ECORE_GETOPT_VALUE_NONE
   };
   int args;
   char **cmds;

   args = ecore_getopt_parse(&options, values, argc, argv);
   if (args < 0)
     {
        fputs("ERROR: Could not parse command line options.\n", stderr);
        return EXIT_FAILURE;
     }
   if (quit_option) return EXIT_SUCCESS;

   if ((type == NULL) && (args == argc))
     {
        fputs("ERROR: Missing file, directory or URL or --type.\n", stderr);
        return EXIT_FAILURE;
     }

   efreet_init();
   efreet_mime_init();

   if (type)
     {
        if (strcmp(type, "terminal") == 0)
          cmds = terminal_open();
        else if (strcmp(type, "browser") == 0)
          cmds = browser_open((const char * const *)argv + args, argc - args);
        else
          {
             const struct type_mime *itr;

             for (itr = type_mimes; itr->type != NULL; itr++)
               {
                  if (strcmp(type, itr->type) == 0)
                    {
                       cmds = mime_open(itr->mime,
                                        (const char * const *)argv + args,
                                        argc - args);
                       break;
                    }
               }
             if (!itr->type)
               {
                  fprintf(stderr, "ERROR: type not supported %s\n", type);
                  cmds = NULL;
               }
          }
     }
   else if (strstr(argv[args], "://"))
     cmds = protocol_open(argv[args]);
   else
     cmds = local_open(argv[args]);

   efreet_mime_shutdown();
   efreet_shutdown();


   /* No EFL, plain boring sequential system() calls */
   if (!cmds)
     return EXIT_FAILURE;
   else
     {
        char **itr;
        int ret = EXIT_SUCCESS;

        for (itr = cmds; *itr != NULL; itr++)
          {
             /* Question: should we execute them in parallel? */
             int r = system(*itr);
             if (r < 0)
               fprintf(stderr, "ERROR: %s executing %s\n", strerror(errno),
                       *itr);
             free(*itr);
             if (r > 0) /* Question: should we stop the loop on first faiure? */
               ret = r;
          }
        free(cmds);

        return ret;
     }
}
