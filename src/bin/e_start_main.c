#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"

static void env_set(const char *var, const char *val);
static int prefix_determine(char *argv0);
static const char *prefix_get(void);

static void
env_set(const char *var, const char *val)
{
   if (val)
     {
#ifdef HAVE_SETENV
	setenv(var, val, 1);
#else
	char buf[8192];
	
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

static const char *
prefix_get(void)
{
   return _prefix_path;
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

int
main(int argc, char **argv)
{
   int i;
   char buf[16384];
   char **args;
   char *p;

   prefix_determine(argv[0]);
   p = getenv("PATH");
   if (p)
     snprintf(buf, sizeof(buf), "%s/bin:%s", _prefix_path, p);
   else
     snprintf(buf, sizeof(buf), "%s/bin", _prefix_path);
   env_set("PATH", buf);

   p = getenv("LD_LIBRARY_PATH");
   if (p)
     snprintf(buf, sizeof(buf), "%s/lib:%s", _prefix_path, p);
   else
     snprintf(buf, sizeof(buf), "%s/lib", _prefix_path);
   env_set("LD_LIBRARY_PATH", buf);

   args = malloc((argc + 1) * sizeof(char *));
   args[0] = "enlightenment";
   for (i = 1; i < argc; i++)
     args[i] = argv[i];
   args[i] = NULL;
   
   snprintf(buf, sizeof(buf), "%s/bin/enlightenment", _prefix_path);
   return execv(buf, args);
}
