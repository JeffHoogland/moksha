/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include <dirent.h>

#if 0
/* externally accessible functions */
time_t
e_file_mod_time(char *file)
{
   struct stat st;

   if (stat(file, &st) < 0) return 0;
   return st.st_mtime;
}

int
e_file_exists(char *file)
{
   struct stat st;

   if (stat(file, &st) < 0) return 0;
   return 1;
}

int
e_file_is_dir(char *file)
{
   struct stat st;

   if (stat(file, &st) < 0) return 0;
   if (S_ISDIR(st.st_mode)) return 1;
   return 0;
}

static mode_t default_mode = S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;

int
e_file_mkdir(char *dir)
{
   if (mkdir(dir, default_mode) < 0) return 0;
   return 1;
}

int
e_file_mkpath(char *path)
{
   char ss[PATH_MAX];
   int  i, ii;

   ss[0] = 0;
   i = 0;
   ii = 0;
   while (path[i])
     {
	if (ii == sizeof(ss) - 1) return 0;
	ss[ii++] = path[i];
	ss[ii] = 0;
	if (path[i] == '/')
	  {
	     if (!e_file_is_dir(ss)) e_file_mkdir(ss);
	     else if (!e_file_is_dir(ss)) return 0;
	  }
	i++;
     }
   if (!e_file_is_dir(ss)) e_file_mkdir(ss);
   else if (!e_file_is_dir(ss)) return 0;
   return 1;
}

int
e_file_cp(char *src, char *dst)
{
   FILE               *f1, *f2;
   char                buf[16384];
   size_t              num;

   f1 = fopen(src, "rb");
   if (!f1) return 0;
   f2 = fopen(dst, "wb");
   if (!f2)
     {
	fclose(f1);
	return 0;
     }
   while ((num = fread(buf, 1, 16384, f1)) > 0) fwrite(buf, 1, num, f2);
   fclose(f1);
   fclose(f2);
   return 1;
}

char               *
e_file_realpath(char *file)
{
   char  buf[PATH_MAX];
   struct stat st;

   if (!realpath(file, buf) || stat(buf, &st)) return strdup("");
   return strdup(buf);
}

char               *
e_file_get_file(char *path)
{
   char *result = NULL;

   if (!path) return NULL;
   if ((result = strrchr(path, '/'))) result++;
   else result = path;
   return result;
}

char               *
e_file_get_dir(char *file)
{
   char               *p;
   char                buf[PATH_MAX];

   strncpy(buf, file, PATH_MAX);
   p = strrchr(buf, '/');
   if (!p)
     {
	return strdup(file);
     }
   *p = 0;
   return strdup(buf);
}

int
e_file_can_exec(struct stat *st)
{
   static int          have_uid = 0;
   static uid_t        uid = -1;
   static gid_t        gid = -1;
   int                 ok;

   if (!st) return 0;
   ok = 0;
   if (!have_uid) uid = getuid();
   if (!have_uid) gid = getgid();
   have_uid = 1;
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
   return(ok);
}

char               *
e_file_readlink(char *link)
{
   char                buf[PATH_MAX];
   int                 count;

   if ((count = readlink(link, buf, sizeof(buf))) < 0) return NULL;
   buf[count] = 0;
   return strdup(buf);
}

Evas_List *
e_file_ls(char *dir)
{
   DIR                *dirp;
   struct dirent      *dp;
   Evas_List          *list;

   dirp = opendir(dir);
   if (!dirp) return NULL;
   list = NULL;
   while ((dp = readdir(dirp)))
     {
	if ((strcmp(dp->d_name, ".")) && (strcmp(dp->d_name, "..")))
	  {
	     Evas_List *l;
	     char      *f;

	     /* insertion sort */
	     for (l = list; l; l = l->next)
	       {
		  if (strcmp(l->data, dp->d_name) > 0)
		    {
		       f = strdup(dp->d_name);
		       list = evas_list_prepend_relative(list, f, l->data);
		       break;
		    }
	       }
	     /* nowhwre to go? just append it */
	     f = strdup(dp->d_name);
	     if (!l)
		list = evas_list_append(list, f);
	  }
     }
   closedir(dirp);

   return list;
}
#endif
