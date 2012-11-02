#include "config.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <sys/ptrace.h>
#include <limits.h>
#include <fcntl.h>
#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#endif
#include <signal.h>

#include <Eina.h>

static Eina_Bool tainted = EINA_FALSE;

static void env_set(const char *var, const char *val);
EAPI int    prefix_determine(char *argv0);

static void
env_set(const char *var, const char *val)
{
   if (val)
     {
#ifdef HAVE_SETENV
        setenv(var, val, 1);
#else
        char *buf;
        size_t size = strlen(var) + 1 + strlen(val) + 1;

        buf = alloca(size);
        snprintf(buf, size, "%s=%s", var, val);
        if (getenv(var)) putenv(buf);
        else putenv(strdup(buf));
#endif
     }
   else
     {
#ifdef HAVE_UNSETENV
        unsetenv(var);
#else
        if (getenv(var)) putenv(var);
#endif
     }
}

/* local subsystem globals */
static Eina_Prefix *pfx = NULL;

/* externally accessible functions */
EAPI int
prefix_determine(char *argv0)
{
   pfx = eina_prefix_new(argv0, prefix_determine,
                         "E", "enlightenment", "AUTHORS",
                         PACKAGE_BIN_DIR, PACKAGE_LIB_DIR,
                         PACKAGE_DATA_DIR, LOCALE_DIR);
   if (!pfx) return 0;
   return 1;
}

static int
find_valgrind(char *path, size_t path_len)
{
   const char *env = getenv("PATH");

   while (env)
     {
        const char *p = strchr(env, ':');
        ssize_t p_len;

        if (p) p_len = p - env;
        else p_len = strlen(env);
        if (p_len <= 0) goto next;
        else if (p_len + sizeof("/valgrind") >= path_len)
          goto next;
        memcpy(path, env, p_len);
        memcpy(path + p_len, "/valgrind", sizeof("/valgrind"));
        if (access(path, X_OK | R_OK) == 0) return 1;
next:
        if (p) env = p + 1;
        else break;
     }
   path[0] = '\0';
   return 0;
}

/* maximum number of arguments added above */
#define VALGRIND_MAX_ARGS 10
/* bitmask with all supported bits set */
#define VALGRIND_MODE_ALL 15

static int
valgrind_append(char **dst, int valgrind_gdbserver, int valgrind_mode, int valgrind_tool, char *valgrind_path, const char *valgrind_log)
{
   int i = 0;

   if (valgrind_tool)
     {
        dst[i++] = valgrind_path;
        switch (valgrind_tool)
          {
           case 1: dst[i++] = "--tool=massif"; break;

           case 2: dst[i++] = "--tool=callgrind"; break;
          }
        return i;
     }
   if (valgrind_gdbserver) dst[i++] = "--db-attach=yes";
   if (!valgrind_mode) return 0;
   dst[i++] = valgrind_path;
   dst[i++] = "--track-origins=yes";
   dst[i++] = "--malloc-fill=13"; /* invalid pointer, make it crash */
   if (valgrind_log)
     {
        static char logparam[PATH_MAX + sizeof("--log-file=")];

        snprintf(logparam, sizeof(logparam), "--log-file=%s", valgrind_log);
        dst[i++] = logparam;
     }
   if (valgrind_mode & 2) dst[i++] = "--trace-children=yes";
   if (valgrind_mode & 4)
     {
        dst[i++] = "--leak-check=full";
        dst[i++] = "--leak-resolution=high";
        dst[i++] = "--track-fds=yes";
     }
   if (valgrind_mode & 8) dst[i++] = "--show-reachable=yes";
   return i;
}

static void
copy_args(char **dst, char **src, size_t count)
{
   for (; count > 0; count--, dst++, src++)
     *dst = *src;
}

static void
_env_path_prepend(const char *env, const char *path)
{
   char *p, *s;
   const char *p2;
   int len = 0, len2 = 0;

   if (!path) return;

   p = getenv(env);
   p2 = path;
   len2 = strlen(p2);
   if (p)
     {
        len = strlen(p);
        // path already there at the start. dont prepend. :)
        if ((!strcmp(p, p2)) ||
            ((len > len2) &&
             (!strncmp(p, p2, len2)) &&
             (p[len2] == ':')))
          return;
     }
   s = malloc(len + 1 + len2 + 1);
   if (s)
     {
        s[0] = 0;
        strcat(s, p2);
        if (p)
          {
             strcat(s, ":");
             strcat(s, p);
          }
        env_set(env, s);
        free(s);
     }
}

static void
_env_path_append(const char *env, const char *path)
{
   char *p, *s;
   const char *p2;
   int len = 0, len2 = 0;

   if (!path) return;

   p = getenv(env);
   p2 = path;
   len2 = strlen(p2);
   if (p)
     {
        len = strlen(p);
        // path already there at the end. dont append. :)
        if ((!strcmp(p, p2)) ||
            ((len > len2) &&
             (!strcmp((p + len - len2), p2)) &&
             (p[len - len2 - 1] == ':')))
          return;
     }
   s = malloc(len + 1 + len2 + 1);
   if (s)
     {
        s[0] = 0;
        if (p)
          {
             strcat(s, p);
             strcat(s, ":");
          }
        strcat(s, p2);
        env_set(env, s);
        free(s);
     }
}

int
main(int argc, char **argv)
{
   int i, valgrind_mode = 0;
   int valgrind_tool = 0;
   int valgrind_gdbserver = 0;
   char buf[16384], **args, *home;
   char valgrind_path[PATH_MAX] = "";
   const char *valgrind_log = NULL;
   Eina_Bool really_know = EINA_FALSE;
   Eina_Bool restart = EINA_TRUE;

   eina_init();

   /* reexcute myself with dbus-launch if dbus-launch is not running yet */
   if ((!getenv("DBUS_SESSION_BUS_ADDRESS")) &&
       (!getenv("DBUS_LAUNCHD_SESSION_BUS_SOCKET"))) 
     {
        char **dbus_argv;

        dbus_argv = alloca((argc + 3) * sizeof (char *));
        dbus_argv[0] = "dbus-launch";
        dbus_argv[1] = "--exit-with-session";
        copy_args(dbus_argv + 2, argv, argc);
        dbus_argv[2 + argc] = NULL;
        execvp("dbus-launch", dbus_argv);
     }

   prefix_determine(argv[0]);

   env_set("E_START", argv[0]);

   for (i = 1; i < argc; i++)
     {
        if (!strcmp(argv[i], "-valgrind-gdb"))
          valgrind_gdbserver = 1;
        else if (!strncmp(argv[i], "-valgrind", sizeof("-valgrind") - 1))
          {
             const char *val = argv[i] + sizeof("-valgrind") - 1;

             if (*val == '\0') valgrind_mode = 1;
             else if (*val == '-')
               {
                  val++;
                  if (!strncmp(val, "log-file=", sizeof("log-file=") - 1))
                    {
                       valgrind_log = val + sizeof("log-file=") - 1;
                       if (*valgrind_log == '\0') valgrind_log = NULL;
                    }
               }
             else if (*val == '=')
               {
                  val++;
                  if (!strcmp(val, "all")) valgrind_mode = VALGRIND_MODE_ALL;
                  else valgrind_mode = atoi(val);
               }
             else
               printf("Unknown valgrind option: %s\n", argv[i]);
          }
        else if (!strcmp(argv[i], "-display"))
          {
             i++;
             env_set("DISPLAY", argv[i]);
          }
        else if (!strcmp(argv[i], "-massif"))
          valgrind_tool = 1;
        else if (!strcmp(argv[i], "-callgrind"))
          valgrind_tool = 2;
        else if ((!strcmp(argv[i], "-h")) ||
                 (!strcmp(argv[i], "-help")) ||
                 (!strcmp(argv[i], "--help")))
          {
             printf
             (
               "Options:\n"
               "\t-valgrind[=MODE]\n"
               "\t\tRun enlightenment from inside valgrind, mode is OR of:\n"
               "\t\t   1 = plain valgrind to catch crashes (default)\n"
               "\t\t   2 = trace children (thumbnailer, efm slaves, ...)\n"
               "\t\t   4 = check leak\n"
               "\t\t   8 = show reachable after processes finish.\n"
               "\t\t all = all of above\n"
               "\t-massif\n"
               "\t\tRun enlightenment from inside massif valgrind tool.\n"
               "\t-callgrind\n"
               "\t\tRun enlightenment from inside callgrind valgrind tool.\n"
               "\t-valgrind-log-file=<FILENAME>\n"
               "\t\tSave valgrind log to file, see valgrind's --log-file for details.\n"
               "\n"
               "Please run:\n"
               "\tenlightenment %s\n"
               "for more options.\n",
               argv[i]);
             exit(0);
          }
        else if (!strcmp(argv[i], "-i-really-know-what-i-am-doing-and-accept-full-responsibility-for-it"))
          really_know = EINA_TRUE;
     }

   if (really_know)
     _env_path_append("PATH", eina_prefix_bin_get(pfx));
   else
     _env_path_prepend("PATH", eina_prefix_bin_get(pfx));

   if (valgrind_mode || valgrind_tool)
     {
        if (!find_valgrind(valgrind_path, sizeof(valgrind_path)))
          {
             printf("E - valgrind required but no binary found! Ignoring request.\n");
             valgrind_mode = 0;
          }
     }

   printf("E - PID=%i, valgrind=%d", getpid(), valgrind_mode);
   if (valgrind_mode)
     {
        printf(" valgrind-command='%s'", valgrind_path);
        if (valgrind_log) printf(" valgrind-log-file='%s'", valgrind_log);
     }
   putchar('\n');

   /* mtrack memory tracker support */
   home = getenv("HOME");
   if (home)
     {
        FILE *f;

        /* if you have ~/.e-mtrack, then the tracker will be enabled
         * using the content of this file as the path to the mtrack.so
         * shared object that is the mtrack preload */
        snprintf(buf, sizeof(buf), "%s/.e-mtrack", home);
        f = fopen(buf, "r");
        if (f)
          {
             if (fgets(buf, sizeof(buf), f))
               {
                  int len = strlen(buf);
                  if ((len > 1) && (buf[len - 1] == '\n'))
                    {
                       buf[len - 1] = 0;
                       len--;
                    }
                  env_set("LD_PRELOAD", buf);
                  env_set("MTRACK", "track");
                  env_set("E_START_MTRACK", "track");
                  snprintf(buf, sizeof(buf), "%s/.e-mtrack.log", home);
                  env_set("MTRACK_TRACE_FILE", buf);
               }
             fclose(f);
          }
     }

   /* run e directly now */
   snprintf(buf, sizeof(buf), "%s/enlightenment", eina_prefix_bin_get(pfx));

   args = alloca((argc + 2 + VALGRIND_MAX_ARGS) * sizeof(char *));
   i = valgrind_append(args, valgrind_gdbserver, valgrind_mode, valgrind_tool, valgrind_path, valgrind_log);
   args[i++] = buf;
   copy_args(args + i, argv + 1, argc - 1);
   args[i + argc - 1] = NULL;
   /* execv(args[0], args); */

   /* not run at the moment !! */


   /* Now looping until */
   while (restart)
     {
        pid_t child;

        tainted = EINA_FALSE;
        child = fork();

        if (child < 0) /* failed attempt */
          return -1;
        else if (child == 0)
          {
             /* in the child */
             ptrace(PTRACE_TRACEME, 0, NULL, NULL);

             execv(args[0], args);
             return 0; /* We failed, 0 mean normal exit from E with no restart or crash so let exit */
          }
        else
          {
             /* in the parent */
             pid_t result;
             int status;
             Eina_Bool done = EINA_FALSE;

             ptrace(PTRACE_ATTACH, child, NULL, NULL);

             result = waitpid(child, &status, 0);

             if (WIFSTOPPED(status))
               ptrace(PTRACE_CONT, child, NULL, NULL);

             while (!done)
               {
                  result = waitpid(child, &status, 0);

                  if (result == child)
                    {
                       if (WIFSTOPPED(status))
                         {
                            char buffer[4096];
                            char *backtrace_str = NULL;
                            siginfo_t sig;
                            int r;
                            int back;

                            r = ptrace(PTRACE_GETSIGINFO, child, NULL, &sig);
                            back = r == 0 &&
                              sig.si_signo != SIGTRAP ? sig.si_signo : 0;

                            if (r != 0 ||
                                (sig.si_signo != SIGSEGV &&
                                 sig.si_signo != SIGILL &&
                                 sig.si_signo != SIGFPE &&
                                 sig.si_signo != SIGBUS &&
                                 sig.si_signo != SIGABRT))
                              {
                                 ptrace(PTRACE_CONT, child, NULL, back);
                                 continue ;
                              }

                            /* E17 should be in pause, we can detach */
                            ptrace(PTRACE_DETACH, child, NULL, back);

                            /* And call gdb if available */
                            if (home)
                              {
                                 /* call e_sys gdb */
                                 snprintf(buffer, 4096,
                                          "%s/enlightenment/utils/enlightenment_sys gdb %i %s/.xsession-errors",
                                          eina_prefix_lib_get(pfx),
                                          child,
                                          home);
                                 r = system(buffer);

                                 fprintf(stderr, "called gdb with '%s' = %i\n",
                                         buffer, WEXITSTATUS(r));

                                 snprintf(buffer, 4096,
                                          "%s/.xsession-errors",
                                          home);

                                 backtrace_str = strdup(buffer);
                              }

                            /* call e_alert */
                            snprintf(buffer, 4096,
                                     backtrace_str ? "%s/enlightenment/utils/enlightenment_alert %i %i %s" : "%s/enlightenment/utils/enlightenment_alert %i %i %s",
                                     eina_prefix_lib_get(pfx),
                                     sig.si_signo,
                                     child,
                                     backtrace_str);
                            r = system(buffer);

                            /* kill e */
                            kill(child, SIGKILL);

                            if (WEXITSTATUS(r) != 1)
                              {
                                 restart = EINA_FALSE;
                              }
                         }
                       else if (!WIFEXITED(status))
                         {
                            done = EINA_TRUE;
                         }
                    }
                  else if (result == - 1)
                    {
                       done = EINA_TRUE;
                       restart = EINA_FALSE;
                    }
               }
          }
          
     }

   return -1;
}

