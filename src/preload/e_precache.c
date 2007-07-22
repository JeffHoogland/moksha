#include "config.h"
#include "e_precache.h"


static void *lib_evas = NULL;
static void *lib_ecore_file = NULL;
static void *lib_eet = NULL;

static int *e_precache_end = NULL;

/* internal calls */
static int log_fd = -1;
static int do_log = 0;

static void
log_open(void)
{
   char buf[4096] = "DUMMY", *home;
   
   if (log_fd != -1) return;
   if (!e_precache_end)
     {
#ifdef HAVE_UNSETENV
	unsetenv("LD_PRELOAD");
#else
	if (getenv("LD_PRELOAD")) putenv("LD_PRELOAD");
#endif
	e_precache_end = dlsym(NULL, "e_precache_end");
     }
   if (!e_precache_end) return;
   if (*e_precache_end) return;
   
   home = getenv("HOME");
   if (home)
     snprintf(buf, sizeof(buf), "%s/.e-precache", home);
   else
     snprintf(buf, sizeof(buf), "/tmp/.e-precache");
   log_fd = open(buf, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
   if (log_fd) do_log = 1;
}

static void
log_close(void)
{
   if (log_fd >= 0)
     {
	close(log_fd);
	log_fd = -1;
     }
   do_log = 0;
}

static void
log_write(const char *type, const char *file)
{
   static Evas_Hash *s_hash = NULL;
   static Evas_Hash *o_hash = NULL;
   static Evas_Hash *d_hash = NULL;
   char buf[2];

   if ((e_precache_end) && (*e_precache_end))
     {
	log_close();
	return;
     }
   if (type[0] == 's')
     {
	if (evas_hash_find(s_hash, file)) return;
	s_hash = evas_hash_add(s_hash, file, (void *)1);
     }
   else if (type[0] == 'o')
     {
	if (evas_hash_find(o_hash, file)) return;
	o_hash = evas_hash_add(o_hash, file, (void *)1);
     }
   else if (type[0] == 'd')
     {
	if (evas_hash_find(d_hash, file)) return;
	d_hash = evas_hash_add(d_hash, file, (void *)1);
     } 
   buf[0] = type[0]; buf[1] = ' ';
   write(log_fd, buf, 2);
   write(log_fd, file, strlen(file));
   write(log_fd, "\n", 1);
}

static void *
lib_func(const char *lib1, const char *lib2, const char *fname, const char *libname, void **lib)
{
   void *func;
   
   if (!*lib) *lib = dlopen(lib1, RTLD_GLOBAL | RTLD_LAZY);
   if (!*lib) *lib = dlopen(lib2, RTLD_GLOBAL | RTLD_LAZY);
   func = dlsym(*lib, fname);
   if (!func)
     {
	printf("ABORT: Can't find %s() in %s or %s (%s = %p)\n",
	       fname, lib1, lib2, libname, *lib);
	abort();
     }
   log_open();
   return func;
}

/* intercepts */
void
evas_object_image_file_set(Evas_Object *obj, const char *file, const char *key)
{
   static void (*func) (Evas_Object *obj, const char *file, const char *key) = NULL;
   if (!func)
     func = lib_func("libevas.so", "libevas.so.1", 
		     "evas_object_image_file_set", "lib_evas", &lib_evas);
   if (do_log) log_write("o", file);
   (*func) (obj, file, key);
}

long long
ecore_file_mod_time(const char *file)
{
   static long long (*func) (const char *file) = NULL;
   if (!func)
     func = lib_func("libecore_file.so", "libecore_file.so.1", 
		     "ecore_file_mod_time", "lib_ecore_file", &lib_ecore_file);
   if (do_log) log_write("s", file);
   return (*func) (file);
}

long long
ecore_file_size(const char *file)
{
   static int (*func) (const char *file) = NULL;
   if (!func)
     func = lib_func("libecore_file.so", "libecore_file.so.1", 
		     "ecore_file_size", "lib_ecore_file", &lib_ecore_file);
   if (do_log) log_write("s", file);
   return (*func) (file);
}

int
ecore_file_exists(const char *file)
{
   static int (*func) (const char *file) = NULL;
   if (!func)
     func = lib_func("libecore_file.so", "libecore_file.so.1", 
		     "ecore_file_exists", "lib_ecore_file", &lib_ecore_file);
   if (do_log) log_write("s", file);
   return (*func) (file);
}

int
ecore_file_is_dir(const char *file)
{
   static int (*func) (const char *file) = NULL;
   if (!func)
     func = lib_func("libecore_file.so", "libecore_file.so.1", 
		     "ecore_file_is_dir", "lib_ecore_file", &lib_ecore_file);
   if (do_log) log_write("s", file);
   return (*func) (file);
}

int
ecore_file_can_read(const char *file)
{
   static int (*func) (const char *file) = NULL;
   if (!func)
     func = lib_func("libecore_file.so", "libecore_file.so.1", 
		     "ecore_file_can_read", "lib_ecore_file", &lib_ecore_file);
   if (do_log) log_write("s", file);
   return (*func) (file);
}

int
ecore_file_can_write(const char *file)
{
   static int (*func) (const char *file) = NULL;
   if (!func)
     func = lib_func("libecore_file.so", "libecore_file.so.1", 
		     "ecore_file_can_write", "lib_ecore_file", &lib_ecore_file);
   if (do_log) log_write("s", file);
   return (*func) (file);
}

int
ecore_file_can_exec(const char *file)
{
   static int (*func) (const char *file) = NULL;
   if (!func)
     func = lib_func("libecore_file.so", "libecore_file.so.1", 
		     "ecore_file_can_exec", "lib_ecore_file", &lib_ecore_file);
   if (do_log) log_write("s", file);
   return (*func) (file);
}

Ecore_List *
ecore_file_ls(const char *file)
{
   static Ecore_List * (*func) (const char *file) = NULL;
   if (!func)
     func = lib_func("libecore_file.so", "libecore_file.so.1", 
		     "ecore_file_ls", "lib_ecore_file", &lib_ecore_file);
   if (do_log) log_write("d", file);
   return (*func) (file);
}

Eet_File *
eet_open(const char *file, Eet_File_Mode mode)
{
   static Eet_File * (*func) (const char *file, Eet_File_Mode mode) = NULL;
   if (!func)
     func = lib_func("libeet.so", "libeet.so.0", 
		     "eet_open", "lib_eet", &lib_eet);
   if (do_log) log_write("o", file);
   return (*func) (file, mode);
}
