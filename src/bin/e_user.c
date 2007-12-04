/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* externally accessible functions */
EAPI const char *
e_user_homedir_get(void)
{
   char *homedir;
   int len;

   homedir = getenv("HOME");
   if (!homedir) return "/tmp";
   len = strlen(homedir);
   while ((len > 1) && (homedir[len - 1] == '/'))
     {
	homedir[len - 1] = 0;
	len--;
     }
   return homedir;
}

/**
 * Return the directory where user .desktop files should be stored.
 * If the directory does not exist, it will be created. If it cannot be
 * created, a dialog will be displayed an NULL will be returned
 */
EAPI const char *
e_user_desktop_dir_get(void)
{
   static char dir[PATH_MAX] = "";

   if (!dir[0])
     snprintf(dir, sizeof(dir), "%s/applications", efreet_data_home_get());

   return dir;
}

/**
 * Return the directory where user .icon files should be stored.
 * If the directory does not exist, it will be created. If it cannot be
 * created, a dialog will be displayed an NULL will be returned
 */
EAPI const char *
e_user_icon_dir_get(void)
{
   static char dir[PATH_MAX] = "";

   if (!dir[0])
     snprintf(dir, sizeof(dir), "%s/icons", efreet_data_home_get());

   return dir;
}
