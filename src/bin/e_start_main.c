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
#include <limits.h>
#include <fcntl.h>
#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#endif

static void env_set(const char *var, const char *val);
static int prefix_determine(char *argv0);

static void
env_set(const char *var, const char *val)
{
   if (val)
     {
#ifdef HAVE_SETENV
	setenv(var, val, 1);
#else
	char *buf;

	buf = alloca(strlen(var) + 1 + strlen(val) + 1);
	snprintf(buf, sizeof(buf), "%s=%s", var, val);
	if (getenv(var))
	  putenv(buf);
	else
	  putenv(strdup(buf));
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

/* local subsystem functions */
static int _prefix_fallbacks(void);
static int _prefix_try_proc(void);
static int _prefix_try_argv(char *argv0);

/* local subsystem globals */
static char *_exe_path = NULL;
static char *_prefix_path = NULL;

/* externally accessible functions */
static int
prefix_determine(char *argv0)
{
   char *p;

   if (!_prefix_try_proc())
     {
	if (!_prefix_try_argv(argv0))
	  {
	     _prefix_fallbacks();
	     return 0;
	  }
     }
   /* _exe_path is now a full absolute path TO this exe - figure out rest */
   /*   if
    * exe        = /blah/whatever/bin/exe
    *   then
    * prefix     = /blah/whatever
    */
   p = strrchr(_exe_path, '/');
   if (p)
     {
	p--;
	while (p >= _exe_path)
	  {
	     if (*p == '/')
	       {
		  _prefix_path = malloc(p - _exe_path + 1);
		  if (_prefix_path)
		    {
		       strncpy(_prefix_path, _exe_path, p - _exe_path);
		       _prefix_path[p - _exe_path] = 0;
		       return 1;
		    }
		  else
		    {
		       free(_exe_path);
		       _exe_path = NULL;
		       _prefix_fallbacks();
		       return 0;
		    }
	       }
	     p--;
	  }
     }
   free(_exe_path);
   _exe_path = NULL;
   _prefix_fallbacks();
   return 0;
}

static int
_prefix_fallbacks(void)
{
   char *p;

   _prefix_path = strdup(PACKAGE_BIN_DIR);
   p = strrchr(_prefix_path, '/');
   if (p) *p = 0;
   printf("WARNING: Enlightenment could not determine its installed prefix\n"
	  "         and is falling back on the compiled in default:\n"
	  "         %s\n", _prefix_path);
   return 1;
}

static int
_prefix_try_proc(void)
{
   FILE *f;
   char buf[4096];
   void *func = NULL;

   func = (void *)_prefix_try_proc;
   f = fopen("/proc/self/maps", "r");
   if (!f) return 0;
   while (fgets(buf, sizeof(buf), f))
     {
	int len;
	char *p, mode[5] = "";
	unsigned long ptr1 = 0, ptr2 = 0;

	len = strlen(buf);
	if (buf[len - 1] == '\n')
	  {
	     buf[len - 1] = 0;
	     len--;
	  }
	if (sscanf(buf, "%lx-%lx %4s", &ptr1, &ptr2, mode) == 3)
	  {
	     if (!strcmp(mode, "r-xp"))
	       {
		  if (((void *)ptr1 <= func) && (func < (void *)ptr2))
		    {
		       p = strchr(buf, '/');
		       if (p)
			 {
			    if (len > 10)
			      {
				 if (!strcmp(buf + len - 10, " (deleted)"))
				   buf[len - 10] = 0;
			      }
			    _exe_path = strdup(p);
			    fclose(f);
			    return 1;
			 }
		       else
			 break;
		    }
	       }
	  }
     }
   fclose(f);
   return 0;
}

static int
_prefix_try_argv(char *argv0)
{
   char *path, *p, *cp, *s;
   int len, lenexe;
#ifdef PATH_MAX
   char buf[PATH_MAX], buf2[PATH_MAX], buf3[PATH_MAX];
#else
   char buf[4096], buf2[4096], buf3[4096];
#endif	

   /* 1. is argv0 abs path? */
   if (argv0[0] == '/')
     {
	_exe_path = strdup(argv0);
	if (access(_exe_path, X_OK) == 0) return 1;
	free(_exe_path);
	_exe_path = NULL;
	return 0;
     }
   /* 2. relative path */
   if (strchr(argv0, '/'))
     {
	if (getcwd(buf3, sizeof(buf3)))
	  {
	     snprintf(buf2, sizeof(buf2), "%s/%s", buf3, argv0);
	     if (realpath(buf2, buf))
	       {
		  _exe_path = strdup(buf);
		  if (access(_exe_path, X_OK) == 0) return 1;
		  free(_exe_path);
		  _exe_path = NULL;
	       }
	  }
     }
   /* 3. argv0 no path - look in PATH */
   path = getenv("PATH");
   if (!path) return 0;
   p = path;
   cp = p;
   lenexe = strlen(argv0);
   while ((p = strchr(cp, ':')))
     {
	len = p - cp;
	s = malloc(len + 1 + lenexe + 1);
	if (s)
	  {
	     strncpy(s, cp, len);
	     s[len] = '/';
	     strcpy(s + len + 1, argv0);
	     if (realpath(s, buf))
	       {
		  if (access(buf, X_OK) == 0)
		    {
		       _exe_path = strdup(buf);
		       free(s);
		       return 1;
		    }
	       }
	     free(s);
	  }
        cp = p + 1;
     }
   len = strlen(cp);
   s = malloc(len + 1 + lenexe + 1);
   if (s)
     {
	strncpy(s, cp, len);
	s[len] = '/';
	strcpy(s + len + 1, argv0);
	if (realpath(s, buf))
	  {
	     if (access(buf, X_OK) == 0)
	       {
		  _exe_path = strdup(buf);
		  free(s);
		  return 1;
	       }
	  }
	free(s);
     }
   /* 4. big problems. arg[0] != executable - weird execution */
   return 0;
}

static void
precache(void)
{
   FILE *f;
   char *home;
   char buf[4096], tbuf[256 * 1024];
   struct stat st;
   int l, fd, children = 0, cret;

   home = getenv("HOME");
   if (home) snprintf(buf, sizeof(buf), "%s/.e-precache", home);
   else snprintf(buf, sizeof(buf), "/tmp/.e-precache");
   f = fopen(buf, "r");
   if (!f) return;
   unlink(buf);
   if (fork()) return;
//   while (fgets(buf, sizeof(buf), f));
//   rewind(f);
   while (fgets(buf, sizeof(buf), f))
     {
	l = strlen(buf);
	if (l > 0) buf[l - 1] = 0;
	if (!fork())
	  {
	     if (buf[0] == 's')
               stat(buf + 2, &st);
	     else if (buf[0] == 'o')
	       {
		  fd = open(buf + 2, O_RDONLY);
		  if (fd >= 0)
		    {
		       while (read(fd, tbuf, 256 * 1024) > 0);
		       close(fd);
		    }
	       }
	     else if (buf[0] == 'd')
	       {
		  fd = open(buf + 2, O_RDONLY);
		  if (fd >= 0)
		    {
		       while (read(fd, tbuf, 256 * 1024) > 0);
		       close(fd);
		    }
	       }
	     exit(0);
	  }
	children++;
	if (children > 400)
	  {
	     wait(&cret);
	     children--;
	  }
     }
   fclose(f);
   while (children > 0)
     {
	wait(&cret);
	children--;
     }
   exit(0);
}

static int
find_valgrind(char *path, size_t path_len)
{
   const char *env = getenv("PATH");

   while (env)
     {
	const char *p = strchr(env, ':');
	ssize_t p_len;

	if (p)
	  p_len = p - env;
	else
	  p_len = strlen(env);

	if (p_len <= 0)
	  goto next;
	else if (p_len + sizeof("/valgrind") >= path_len)
	  goto next;

	memcpy(path, env, p_len);
	memcpy(path + p_len, "/valgrind", sizeof("/valgrind"));
	if (access(path, X_OK | R_OK) == 0)
	  return 1;

     next:
	if (p)
	  env = p + 1;
	else
	  break;
     }
   path[0] = '\0';
   return 0;
}


/* maximum number of arguments added above */
#define VALGRIND_MAX_ARGS 9
/* bitmask with all supported bits set */
#define VALGRIND_MODE_ALL 15

static int
valgrind_append(char **dst, int valgrind_mode, int valgrind_tool, char *valgrind_path, const char *valgrind_log)
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

   if (!valgrind_mode)
     return 0;
   dst[i++] = valgrind_path;
   dst[i++] = "--track-origins=yes";
   dst[i++] = "--malloc-fill=13"; /* invalid pointer, make it crash */

   if (valgrind_log)
     {
	static char logparam[PATH_MAX + sizeof("--log-file=")];

	snprintf(logparam, sizeof(logparam), "--log-file=%s", valgrind_log);
	dst[i++] = logparam;
     }

   if (valgrind_mode & 2)
     dst[i++] = "--trace-children=yes";

   if (valgrind_mode & 4)
     {
	dst[i++] = "--leak-check=full";
	dst[i++] = "--leak-resolution=high";
	dst[i++] = "--track-fds=yes";
     }

   if (valgrind_mode & 8)
     dst[i++] = "--show-reachable=yes";

   return i;
}

static void
copy_args(char **dst, char **src, size_t count)
{
   for (; count > 0; count--, dst++, src++)
     *dst = *src;
}

int
main(int argc, char **argv)
{
   int i, do_precache = 0, valgrind_mode = 0;
   int valgrind_tool = 0;
   char buf[16384], **args, *p;
   char valgrind_path[PATH_MAX] = "";
   const char *valgrind_log = NULL;

   prefix_determine(argv[0]);

   env_set("E_START", argv[0]);

   p = getenv("PATH");
   if (p) snprintf(buf, sizeof(buf), "%s/bin:%s", _prefix_path, p);
   else snprintf(buf, sizeof(buf), "%s/bin", _prefix_path);
   env_set("PATH", buf);

   p = getenv("LD_LIBRARY_PATH");
   if (p) snprintf(buf, sizeof(buf), "%s/lib:%s", _prefix_path, p);
   else snprintf(buf, sizeof(buf), "%s/lib", _prefix_path);
   env_set("LD_LIBRARY_PATH", buf);

   for (i = 1; i < argc; i++)
     {
	if (!strcmp(argv[i], "-no-precache")) do_precache = 0;
	else if (!strncmp(argv[i], "-valgrind", sizeof("-valgrind") - 1))
	  {
	     const char *val = argv[i] + sizeof("-valgrind") - 1;

	     if (*val == '\0')
	       valgrind_mode = 1;
	     else if (*val == '-')
	       {
		  val++;
		  if (!strncmp(val, "log-file=", sizeof("log-file=") - 1))
		    {
		       valgrind_log = val + sizeof("log-file=") - 1;
		       if (*valgrind_log == '\0')
			 valgrind_log = NULL;
		    }
	       }
	     else if (*val == '=')
	       {
		  val++;
		  if (!strcmp(val, "all"))
		    valgrind_mode = VALGRIND_MODE_ALL;
		  else
		    valgrind_mode = atoi(val);
	       }
	     else
	       printf("Unknown valgrind option: %s\n", argv[i]);
	  }
	else if (!strcmp(argv[i], "-massif"))
	  {
	     valgrind_tool = 1;
	  }
	else if (!strcmp(argv[i], "-callgrind"))
	  {
	     valgrind_tool = 2;
	  }
        else if ((!strcmp(argv[i], "-h")) ||
		 (!strcmp(argv[i], "-help")) ||
		 (!strcmp(argv[i], "--help")))
	  {
	     printf
	       ("Options:\n"
		"\t-no-precache\n"
		"\t\tDisable pre-caching of files\n"
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
     }

   if (valgrind_mode || valgrind_tool)
     {
	if (!find_valgrind(valgrind_path, sizeof(valgrind_path)))
	  {
	     printf("E - valgrind required but no binary found! Ignoring request.\n");
	     valgrind_mode = 0;
	  }
     }

   printf("E - PID=%i, do_precache=%i, valgrind=%d", getpid(), do_precache, valgrind_mode);
   if (valgrind_mode)
     {
	printf(" valgrind-command='%s'", valgrind_path);
	if (valgrind_log)
	  printf(" valgrind-log-file='%s'", valgrind_log);
     }
   putchar('\n');

   if (do_precache)
     {
	void *lib, *func;

	/* sanity checks - if precache might fail - check here first */
	lib = dlopen("libeina.so", RTLD_GLOBAL | RTLD_LAZY);
	if (!lib) dlopen("libeina.so.1", RTLD_GLOBAL | RTLD_LAZY);
	if (!lib) goto done;
	func = dlsym(lib, "eina_init");
	if (!func) goto done;

	lib = dlopen("libecore.so", RTLD_GLOBAL | RTLD_LAZY);
	if (!lib) dlopen("libecore.so.1", RTLD_GLOBAL | RTLD_LAZY);
	if (!lib) goto done;
	func = dlsym(lib, "ecore_init");
	if (!func) goto done;

	lib = dlopen("libecore_file.so", RTLD_GLOBAL | RTLD_LAZY);
	if (!lib) dlopen("libecore_file.so.1", RTLD_GLOBAL | RTLD_LAZY);
	if (!lib) goto done;
	func = dlsym(lib, "ecore_file_init");
	if (!func) goto done;

	lib = dlopen("libecore_x.so", RTLD_GLOBAL | RTLD_LAZY);
	if (!lib) dlopen("libecore_x.so.1", RTLD_GLOBAL | RTLD_LAZY);
	if (!lib) goto done;
	func = dlsym(lib, "ecore_x_init");
	if (!func) goto done;

	lib = dlopen("libevas.so", RTLD_GLOBAL | RTLD_LAZY);
	if (!lib) dlopen("libevas.so.1", RTLD_GLOBAL | RTLD_LAZY);
	if (!lib) goto done;
	func = dlsym(lib, "evas_init");
	if (!func) goto done;

	lib = dlopen("libedje.so", RTLD_GLOBAL | RTLD_LAZY);
	if (!lib) dlopen("libedje.so.1", RTLD_GLOBAL | RTLD_LAZY);
	if (!lib) goto done;
	func = dlsym(lib, "edje_init");
	if (!func) goto done;

	lib = dlopen("libeet.so", RTLD_GLOBAL | RTLD_LAZY);
	if (!lib) dlopen("libeet.so.0", RTLD_GLOBAL | RTLD_LAZY);
	if (!lib) goto done;
	func = dlsym(lib, "eet_init");
	if (!func) goto done;

	/* precache SHOULD work */
	snprintf(buf, sizeof(buf), "%s/lib/enlightenment/preload/e_precache.so", _prefix_path);
	env_set("LD_PRELOAD", buf);
	printf("E - PRECACHE GOING NOW...\n");
	fflush(stdout);
	precache();
     }
   done:

   /* try dbus-launch */
   snprintf(buf, sizeof(buf), "%s/bin/enlightenment", _prefix_path);

   args = alloca((argc + 2 + VALGRIND_MAX_ARGS) * sizeof(char *));
   if ((!getenv("DBUS_SESSION_BUS_ADDRESS")) &&
       (!getenv("DBUS_LAUNCHD_SESSION_BUS_SOCKET")))
     {
	args[0] = "dbus-launch";
	args[1] = "--exit-with-session";

	i = 2 + valgrind_append(args + 2, valgrind_mode, valgrind_tool, valgrind_path, valgrind_log);
	args[i++] = buf;
	copy_args(args + i, argv + 1, argc - 1);
	args[i + argc - 1] = NULL;
	execvp("dbus-launch", args);
     }

   /* dbus-launch failed - run e direct */
   i = valgrind_append(args, valgrind_mode, valgrind_tool, valgrind_path, valgrind_log);
   args[i++] = buf;
   copy_args(args + i, argv + 1, argc - 1);
   args[i + argc - 1] = NULL;
   execv(args[0], args);

   printf("FAILED TO RUN:\n");
   printf("  %s\n", buf);
   perror("execv");
   return -1;
}
