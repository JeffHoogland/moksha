/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* externally accessible functions */
EAPI char *
e_user_homedir_get(void)
{
   char *homedir;
   int len;
   
   homedir = getenv("HOME");
   if (!homedir) return strdup("/tmp");
   len = strlen(homedir);
   while ((len > 1) && (homedir[len - 1] == '/'))
     {
	homedir[len - 1] = 0;
	len--;
     }
   return strdup(homedir);
}
