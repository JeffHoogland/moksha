#include "e.h"

time_t
e_file_modified_time(char *file)
{
   struct stat         st;
   
   if (stat(file, &st) < 0) return 0;
   return st.st_mtime;
}

void
e_set_env(char *variable, char *content)
{
   char env[4096];
   
   sprintf(env, "%s=%s", variable, content);
   putenv(env);
}

int
e_file_exists(char *file)
{
   struct stat         st;
   
   if (stat(file, &st) < 0) return 0;
   return 1;
}

int
e_file_is_dir(char *file)
{
   struct stat         st;
   
   if (stat(file, &st) < 0) return 0;
   if (S_ISDIR(st.st_mode)) return 1;
   return 0;
}

char *
e_file_home(void)
{
   static char *home = NULL;
   
   if (home) return home;
   home = getenv("HOME");
   if (!home) home = getenv("TMPDIR");
   if (!home) home = "/tmp";
   return home;
}

static mode_t       default_mode = S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;

int
e_file_mkdir(char *dir)
{
   if (mkdir(dir, default_mode) < 0) return 0;
   return 1;
}

int
e_file_cp(char *src, char *dst)
{
   FILE *f1, *f2;
   char buf[4096];
   size_t num;
   
   f1 = fopen(src, "rb");
   if (!f1) return 0;
   f2 = fopen(dst, "wb");
   if (!f2)
     {
	fclose(f1);
	return 0;
     }
   while ((num = fread(buf, 1, 4096, f1)) > 0) fwrite(buf, 1, num, f2);
   fclose(f1);
   fclose(f2);
   return 1;
}

char *
e_file_real(char *file)
{
   char buf[4096];
   
   if (!realpath(file, buf)) return strdup("");
   return strdup(buf);
}

char *
e_file_get_file(char *file)
{
   char *p;
   char buf[4096];
   
   p = strchr(file, '/');
   if (!p) return strdup(file);
   return strdup(&(p[1]));
}

char *
e_file_get_dir(char *file)
{
   char *p;
   char buf[4096];
   
   strcpy(buf, file);
   p = strchr(buf, '/');
   if (!p) return strdup(file);
   *p = 0;
   return strdup(buf);
}
