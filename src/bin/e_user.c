#include "e.h"

/* externally accessible functions */
char *
e_user_homedir_get(void)
{
   char *homedir;
   
   homedir = getenv("HOME");
   if (!homedir) return strdup("/tmp");
   return strdup(homedir);
}
