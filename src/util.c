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
