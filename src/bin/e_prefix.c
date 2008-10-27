/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* local subsystem functions */
static int _e_prefix_share_hunt(void);
static int _e_prefix_fallbacks(void);
static int _e_prefix_try_proc(void);
static int _e_prefix_try_argv(char *argv0);

/* local subsystem globals */
static char *_exe_path = NULL;
static char *_prefix_path = NULL;
static char *_prefix_path_locale = NULL;
static char *_prefix_path_bin = NULL;
static char *_prefix_path_data = NULL;
static char *_prefix_path_lib = NULL;

#define PREFIX_CACHE_FILE 1
#define SHARE_D "share/enlightenment"
#define MAGIC_FILE "AUTHORS"
#define MAGIC_DAT SHARE_D"/"MAGIC_FILE
#define LOCALE_D "share/locale"

/* externally accessible functions */
EAPI int
e_prefix_determine(char *argv0)
{
   char *p, buf[4096];

   e_prefix_shutdown();

   /* if user provides E_PREFIX - then use that or also more specific sub
    * dirs for bin, lib, data and locale */
   if (getenv("E_PREFIX"))
     {
	_prefix_path = strdup(getenv("E_PREFIX"));
	if (getenv("E_BIN_DIR"))
	  snprintf(buf, sizeof(buf), "%s/bin", getenv("E_BIN_DIR"));
	else
	  snprintf(buf, sizeof(buf), "%s/bin", _prefix_path);
	_prefix_path_bin = strdup(buf);

	if (getenv("E_LIB_DIR"))
	  snprintf(buf, sizeof(buf), "%s/lib", getenv("E_LIB_DIR"));
	else
	  snprintf(buf, sizeof(buf), "%s/lib", _prefix_path);
	_prefix_path_lib = strdup(buf);
	
	if (getenv("E_DATA_DIR"))
	  snprintf(buf, sizeof(buf), "%s/"SHARE_D, getenv("E_DATA_DIR"));
	else
	  snprintf(buf, sizeof(buf), "%s/"SHARE_D, _prefix_path);
	_prefix_path_data = strdup(buf);
	
	if (getenv("E_LOCALE_DIR"))
	  snprintf(buf, sizeof(buf), "%s/"LOCALE_D, getenv("E_LOCALE_DIR"));
	else
	  snprintf(buf, sizeof(buf), "%s/"LOCALE_D, _prefix_path);
	_prefix_path_locale = strdup(buf);
	return 1;
     }
   /* no env var - examine process and possible argv0 */
   if (!_e_prefix_try_proc())
     {
	if (!_e_prefix_try_argv(argv0))
	  {
	     e_prefix_fallback();
	     return 0;
	  }
     }
   /* _exe_path is now a full absolute path TO this exe - figure out rest */
   /*   if
    * exe        = /blah/whatever/bin/exe
    *   then
    * prefix     = /blah/whatever
    * bin_dir    = /blah/whatever/bin
    * data_dir   = /blah/whatever/share/enlightenment
    * locale_dir = /blah/whatever/share/locale
    * lib_dir    = /blah/whatever/lib
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
		       ecore_strlcpy(_prefix_path, _exe_path, p - _exe_path + 1);

		       /* bin and lib always together */
		       snprintf(buf, sizeof(buf), "%s/bin", _prefix_path);
		       _prefix_path_bin = strdup(buf);
		       snprintf(buf, sizeof(buf), "%s/lib", _prefix_path);
		       _prefix_path_lib = strdup(buf);
		       
		       printf("DYNAMIC DETERMINED PREFIX: %s\n", _prefix_path);

		       /* check if AUTHORS file is there - then our guess is right */
		       snprintf(buf, sizeof(buf), "%s/"MAGIC_DAT, _prefix_path);
		       if (ecore_file_exists(buf))
			 {
			    snprintf(buf, sizeof(buf), "%s/"SHARE_D, _prefix_path);
			    _prefix_path_data = strdup(buf);
			    snprintf(buf, sizeof(buf), "%s/"LOCALE_D, _prefix_path);
			    _prefix_path_locale = strdup(buf);
			 }
		       /* AUTHORS file not there. time to start hunting! */
		       else
			 {
			    if (_e_prefix_share_hunt())
			      {
				 printf("DIFFERENT DYNAMIC DETERMINED DATA DIR: %s\n", _prefix_path_data);
				 printf("DIFFERENT DYNAMIC DETERMINED LOCALE DIR: %s\n", _prefix_path_locale);
			      }
			    else
			      {
				 e_prefix_fallback();
				 return 0;
			      }
			 }
		       return 1;
		    }
		  else
		    {
		       e_prefix_fallback();
		       return 0;
		    }
	       }
	     p--;
	  }
     }
   e_prefix_fallback();
   return 0;
}

EAPI void
e_prefix_shutdown(void)
{
   E_FREE(_exe_path);
   E_FREE(_prefix_path);
   E_FREE(_prefix_path_locale);
   E_FREE(_prefix_path_bin);
   E_FREE(_prefix_path_data);
   E_FREE(_prefix_path_lib);
}
   
EAPI void
e_prefix_fallback(void)
{
   e_prefix_shutdown();
   _e_prefix_fallbacks();
}

EAPI const char *
e_prefix_get(void)
{
   return _prefix_path;
}

EAPI const char *
e_prefix_locale_get(void)
{
   return _prefix_path_locale;
}

EAPI const char *
e_prefix_bin_get(void)
{
   return _prefix_path_bin;
}

EAPI const char *
e_prefix_data_get(void)
{
   return _prefix_path_data;
}

EAPI const char *
e_prefix_lib_get(void)
{
   return _prefix_path_lib;
}

/* local subsystem functions */
static int
_e_prefix_share_hunt(void)
{
   char buf[4096], buf2[4096], *p;
   FILE *f;
#ifdef PREFIX_CACHE_FILE
   const char *home;
#endif   

   /* sometimes this isnt the case - so we need to do a more exhaustive search
    * through more parent and subdirs. hre is an example i have seen:
    * 
    * /blah/whatever/exec/bin/exe
    * ->
    * /blah/whatever/exec/bin
    * /blah/whatever/common/share/enlightenment
    * /blah/whatever/common/share/locale
    * /blah/whatever/exec/lib
    */
   
   /* this is pure black magic to try and find data shares */
#ifdef PREFIX_CACHE_FILE
   /* 1. check cache file - as a first attempt. this will speed up subsequent
    * hunts - if needed */
   home = e_user_homedir_get();
 
   snprintf(buf, sizeof(buf), "%s/.e/e/prefix_share_cache.txt", home);
   f = fopen(buf, "r");
   if (f)
     {
	if (fgets(buf2, sizeof(buf2), f))
	  {
	     int len;
	     
	     len = strlen(buf2);
	     if (len > 1) buf2[len - 1] = 0;
	     snprintf(buf, sizeof(buf), "%s/"MAGIC_FILE, buf2);
	     if (ecore_file_exists(buf))
	       {
		  /* path is ok - magic file found */
		  _prefix_path_data = strdup(buf2);
		  snprintf(buf, sizeof(buf), "%s", buf2);
		  p = strrchr(buf, '/');
		  if (p) *p = 0;
		  snprintf(buf2, sizeof(buf2), "%s/locale", buf);
		  _prefix_path_locale = strdup(buf2);
		  fclose(f);
		  return 1;
	       }
	  }
	fclose(f);
     }
#endif   
   /* 2. cache file doesn't exist or is invalid - we need to search - this is
    * where the real black magic begins */
   
   /* BLACK MAGIC 1:
    * /blah/whatever/dir1/bin
    * /blah/whatever/dir2/share/enlightenment
    */
   if (!_prefix_path_data)
     {
	Ecore_List *files;
	
	snprintf(buf, sizeof(buf), "%s", _prefix_path);
	p = strrchr(buf, '/');
	if (p) *p = 0;
	files = ecore_file_ls(buf);
	if (files)
	  {
	     char *file;
	     
	     ecore_list_first_goto(files);
	     while ((file = ecore_list_current(files)))
	       {
		  snprintf(buf2, sizeof(buf2), "%s/%s/"MAGIC_DAT, buf, file);
		  if (ecore_file_exists(buf2))
		    {
		       snprintf(buf2, sizeof(buf2), "%s/%s/"SHARE_D, buf, file);
		       _prefix_path_data = strdup(buf2);
		       snprintf(buf2, sizeof(buf2), "%s/%s/"LOCALE_D, buf, file);
		       _prefix_path_locale = strdup(buf2);
		       break;
		    }
		  ecore_list_next(files);
	       }
	     ecore_list_destroy(files);
	  }
     }
   
   /* BLACK MAGIC 2:
    * /blah/whatever/dir1/bin
    * /blah/whatever/share/enlightenment
    */
   if (!_prefix_path_data)
     {
	snprintf(buf, sizeof(buf), "%s", _prefix_path);
	p = strrchr(buf, '/');
	if (p) *p = 0;
	snprintf(buf2, sizeof(buf2), "%s/"MAGIC_DAT, buf);
	if (ecore_file_exists(buf2))
	  {
	     snprintf(buf2, sizeof(buf2), "%s/"SHARE_D, buf);
	     _prefix_path_data = strdup(buf2);
	     snprintf(buf2, sizeof(buf2), "%s/"LOCALE_D, buf);
	     _prefix_path_locale = strdup(buf2);
	  }
     }
   
   /* add more black magic as required as we discover weridnesss - remember
    * this is to save users having to set environment variables to tell
    * e where it lives, so e auto-adapts. so these code snippets are just
    * logic to figure that out for the user
    */
   
   /* done. we found it - write cache file */
   if (_prefix_path_data)
     {
#ifdef PREFIX_CACHE_FILE
	snprintf(buf, sizeof(buf), "%s/.e/e", home);
	ecore_file_mkpath(buf);
	snprintf(buf, sizeof(buf), "%s/.e/e/prefix_share_cache.txt", home);
	f = fopen(buf, "w");
	if (f)
	  {
	     fprintf(f, "%s\n", _prefix_path_data);
	     fclose(f);
	  }
#endif	
	return 1;
     }
   /* fail. everything failed */
   return 0;
}

static int
_e_prefix_fallbacks(void)
{
   char *p;

   _prefix_path = strdup(PACKAGE_BIN_DIR);
   p = strrchr(_prefix_path, '/');
   if (p) *p = 0;
   _prefix_path_locale = strdup(LOCALE_DIR);
   _prefix_path_bin    = strdup(PACKAGE_BIN_DIR);
   _prefix_path_data   = strdup(PACKAGE_DATA_DIR);
   _prefix_path_lib    = strdup(PACKAGE_LIB_DIR);
   printf("WARNING: Enlightenment could not determine its installed prefix\n"
	  "         and is falling back on the compiled in default:\n"
	  "           %s\n"
	  "         You might like to try setting the following environment variables:\n"
	  "           E_PREFIX     - points to the base prefix of install\n"
	  "           E_BIN_DIR    - optional in addition to E_PREFIX to provide\n"
	  "                          a more specific binary directory\n"
	  "           E_LIB_DIR    - optional in addition to E_PREFIX to provide\n"
	  "                          a more specific library dir\n"
	  "           E_DATA_DIR   - optional in addition to E_PREFIX to provide\n"
	  "                          a more specific location for shared data\n"
	  "           E_LOCALE_DIR - optional in addition to E_PREFIX to provide\n"
	  "                          a more specific location for locale data\n"
	  ,
	  _prefix_path);
   return 1;
}

static int
_e_prefix_try_proc(void)
{
   FILE *f;
   char buf[4096];
   void *func = NULL;

   func = (void *)_e_prefix_try_proc;
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
_e_prefix_try_argv(char *argv0)
{
   char *path, *p, *cp, *s;
   int len, lenexe;
   char buf[4096], buf2[4096], buf3[4096];
   
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
