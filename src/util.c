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


/*
 * Function to take a URL of the form
 *  file://hostname/dir1/dir2/file
 *
 * Test that 'file://' exists.
 * Test that hostname matches passed value
 * Return a pointer to /dir1/...
 *
 * todo: 
 * - dir or filename which matches hostname will
 *   fool the hostname match
 * - file://dir1/dir2/file is also legal but rejected by
 *   this presently
 */
char *
e_util_de_url_and_verify( const char *fi, const char *hostn )
{
  char *wk;

  D_ENTER;
   
  wk = strstr( fi, "file://" );

  /* Valid URL contains "file://" */
  if( !wk )
    D_RETURN_ (NULL);

  /* Need some form of hostname to continue */
  if( !hostn )
    D_RETURN_ (NULL);

  /* Do we contain hostname? */
  wk = strstr( fi, hostn );

  /* Hostname mismatch, reject file */
  if( !wk )
    D_RETURN_ (NULL);

  /* Local file name starts after "hostname" */
  wk = strchr( wk, '/' );

  if( !wk )
    D_RETURN_ (NULL);

  D_RETURN( wk );
}
