#include "debug.h"
#include "util.h"

void
e_util_set_env(char *variable, char *content)
{
   char env[PATH_MAX];
   
   D_ENTER;
   
   sprintf(env, "%s=%s", variable, content);
   putenv(env);

   D_RETURN;
}

char *
e_util_get_user_home(void)
{
   static char *home = NULL;
   
   D_ENTER;
   
   if (home) D_RETURN_(home);
   home = getenv("HOME");
   if (!home) home = getenv("TMPDIR");
   if (!home) home = "/tmp";

   D_RETURN_(home);
}

void *
e_util_memdup(void *data, int size)
{
   void *data_dup;
   
   D_ENTER;
   
   data_dup = malloc(size);
   if (!data_dup) D_RETURN_(NULL);
   memcpy(data_dup, data, size);

   D_RETURN_(data_dup);
}

int
e_util_glob_matches(char *str, char *glob)
{
   D_ENTER;
   
   if (!fnmatch(glob, str, 0)) D_RETURN_(1);

   D_RETURN_(0);
}

