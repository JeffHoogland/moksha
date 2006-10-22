#include "config.h"
#include "e_precache.h"


static void *lib_evas = NULL;
static void *lib_ecore_file = NULL;
static void *lib_eet = NULL;

static int *e_precache_end = 0;

/* internal calls */
static int log_fd = -1;
static int do_log = 0;

static void
log_open(void)
{
   char buf[4096], *home;
   
   if (log_fd != -1) return;
#ifdef HAVE_UNSETENV
   unsetenv("LD_PRELOAD");
#else
   if (getenv("LD_PRELOAD")) putenv("LD_PRELOAD");
#endif
   home = getenv("HOME");
   if (home)
     snprintf(buf, sizeof(buf), "%s/.e-precache", home);
   else
     snprintf(buf, sizeof(buf), "/tmp/.e-precache");
   log_fd = open(buf, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR|S_IWUSR);
   do_log = 1;
   e_precache_end = dlsym(NULL, "e_precache_end");
}

static void
log_close(void)
{
   close(log_fd);
   do_log = 0;
}

static void
log_write(const char *type, const char *file)
{
   static Evas_Hash *s_hash = NULL;
   static Evas_Hash *o_hash = NULL;
   static Evas_Hash *d_hash = NULL;
   char buf[2];

   if ((*e_precache_end) && (*e_precache_end))
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


/* intercepts */
void
evas_object_image_file_set(Evas_Object *obj, const char *file, const char *key)
{
   static void (*func) (Evas_Object *obj, const char *file, const char *key) = NULL;
   if (!func)
     {
	if (!lib_evas)
	  lib_evas = dlopen("libevas.so", RTLD_GLOBAL | RTLD_LAZY);
	func = dlsym(lib_evas, "evas_object_image_file_set");
	log_open();
     }
   if (do_log) log_write("o", file);
   (*func) (obj, file, key);
}

time_t
ecore_file_mod_time(const char *file)
{
   static time_t (*func) (const char *file) = NULL;
   if (!func)
     {
	if (!lib_ecore_file)
	  lib_ecore_file = dlopen("libecore_file.so", RTLD_GLOBAL | RTLD_LAZY);
	func = dlsym(lib_ecore_file, "ecore_file_mod_time");
	log_open();
     }
   if (do_log) log_write("s", file);
   return (*func) (file);
}

int
ecore_file_size(const char *file)
{
   static int (*func) (const char *file) = NULL;
   if (!func)
     {
	if (!lib_ecore_file)
	  lib_ecore_file = dlopen("libecore_file.so", RTLD_GLOBAL | RTLD_LAZY);
	func = dlsym(lib_ecore_file, "ecore_file_size");
	log_open();
     }
   if (do_log) log_write("s", file);
   return (*func) (file);
}

int
ecore_file_exists(const char *file)
{
   static int (*func) (const char *file) = NULL;
   if (!func)
     {
	if (!lib_ecore_file)
	  lib_ecore_file = dlopen("libecore_file.so", RTLD_GLOBAL | RTLD_LAZY);
	func = dlsym(lib_ecore_file, "ecore_file_exists");
	log_open();
     }
   if (do_log) log_write("s", file);
   return (*func) (file);
}

int
ecore_file_is_dir(const char *file)
{
   static int (*func) (const char *file) = NULL;
   if (!func)
     {
	if (!lib_ecore_file)
	  lib_ecore_file = dlopen("libecore_file.so", RTLD_GLOBAL | RTLD_LAZY);
	func = dlsym(lib_ecore_file, "ecore_file_is_dir");
	log_open();
     }
   if (do_log) log_write("s", file);
   return (*func) (file);
}

int
ecore_file_can_read(const char *file)
{
   static int (*func) (const char *file) = NULL;
   if (!func)
     {
	if (!lib_ecore_file)
	  lib_ecore_file = dlopen("libecore_file.so", RTLD_GLOBAL | RTLD_LAZY);
	func = dlsym(lib_ecore_file, "ecore_file_can_read");
	log_open();
     }
   if (do_log) log_write("s", file);
   return (*func) (file);
}

int
ecore_file_can_write(const char *file)
{
   static int (*func) (const char *file) = NULL;
   if (!func)
     {
	if (!lib_ecore_file)
	  lib_ecore_file = dlopen("libecore_file.so", RTLD_GLOBAL | RTLD_LAZY);
	func = dlsym(lib_ecore_file, "ecore_file_can_write");
	log_open();
     }
   if (do_log) log_write("s", file);
   return (*func) (file);
}

int
ecore_file_can_exec(const char *file)
{
   static int (*func) (const char *file) = NULL;
   if (!func)
     {
	if (!lib_ecore_file)
	  lib_ecore_file = dlopen("libecore_file.so", RTLD_GLOBAL | RTLD_LAZY);
	func = dlsym(lib_ecore_file, "ecore_file_can_exec");
	log_open();
     }
   if (do_log) log_write("s", file);
   return (*func) (file);
}

Ecore_List *
ecore_file_ls(const char *file)
{
   static Ecore_List * (*func) (const char *file) = NULL;
   if (!func)
     {
	if (!lib_ecore_file)
	  lib_ecore_file = dlopen("libecore_file.so", RTLD_GLOBAL | RTLD_LAZY);
	func = dlsym(lib_ecore_file, "ecore_file_ls");
	log_open();
     }
   if (do_log) log_write("d", file);
   return (*func) (file);
}

Eet_File *
eet_open(const char *file, Eet_File_Mode mode)
{
   static Eet_File * (*func) (const char *file, Eet_File_Mode mode) = NULL;
   if (!func)
     {
	if (!lib_eet)
	  lib_eet = dlopen("libeet.so", RTLD_GLOBAL | RTLD_LAZY);
	func = dlsym(lib_eet, "eet_open");
	log_open();
     }
   if (do_log) log_write("o", file);
   return (*func) (file, mode);
}
