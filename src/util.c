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
   
   p = strrchr(file, '/');
   if (!p) return strdup(file);
   return strdup(&(p[1]));
}

char *
e_file_get_dir(char *file)
{
   char *p;
   char buf[4096];
   
   strcpy(buf, file);
   p = strrchr(buf, '/');
   if (!p) return strdup(file);
   *p = 0;
   return strdup(buf);
}

void *
e_memdup(void *data, int size)
{
   void *data_dup;
   
   data_dup = malloc(size);
   if (!data_dup) return NULL;
   memcpy(data_dup, data, size);
   return data_dup;
}

int
e_glob_matches(char *str, char *glob)
{
   if (!fnmatch(glob, str, 0)) return 1;
   return 0;
}

int
e_file_can_exec(struct stat *st)
{
   static uid_t uid = -1;
   static gid_t gid = -1;
   int ok;
   
   if (!st) return 0;
   ok = 0;
   if (uid < 0) uid = getuid();
   if (gid < 0) gid = getgid();
   if (st->st_uid == uid)
     {
	if (st->st_mode & S_IXUSR) ok = 1;
     }
   else if (st->st_gid == gid)
     {
	if (st->st_mode & S_IXGRP) ok = 1;
     }
   else
     {
	if (st->st_mode & S_IXOTH) ok = 1;
     }
   return ok;
}

char *
e_file_link(char *link)
{
   char buf[4096];
   int count;
   
   if ((count = readlink(link, buf, sizeof(buf))) < 0) return NULL;
   buf[count] = 0;
   return strdup(buf);
}
