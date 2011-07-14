#include "e.h"

EINTERN int
e_deskenv_init(void)
{
   char buf[PATH_MAX], buf2[PATH_MAX];
   
   // run xdrb -load .Xdefaults & .Xresources
   // NOTE: one day we should replace this with an e based config + service
   if (e_config->deskenv.load_xrdb)
     {
        e_user_homedir_concat(buf, sizeof(buf), ".Xdefaults");
        if (ecore_file_exists(buf))
          {
             snprintf(buf2, sizeof(buf2), "xrdb -load %s", buf);
             ecore_exe_run(buf2, NULL);
          }
        e_user_homedir_concat(buf, sizeof(buf), ".Xresources");
        if (ecore_file_exists(buf))
          {
             snprintf(buf2, sizeof(buf2), "xrdb -load %s", buf);
             ecore_exe_run(buf2, NULL);
          }
     }

   // load ~/.Xmodmap
   // NOTE: one day we should replace this with an e based config + service
   if (e_config->deskenv.load_xmodmap)
     {
        e_user_homedir_concat(buf, sizeof(buf), ".Xmodmap");
        if (ecore_file_exists(buf))
          {
             snprintf(buf2, sizeof(buf2), "xmodmap %s", buf);
             ecore_exe_run(buf2, NULL);
          }
     }

   // make gnome apps happy
   // NOTE: one day we should replace this with an e based config + service
   if (e_config->deskenv.load_gnome)
     {
        ecore_exe_run("gnome-settings-daemon", NULL);
     }

   // make kde apps happy
   // NOTE: one day we should replace this with an e based config + service ??
   if (e_config->deskenv.load_kde)
     {
        ecore_exe_run("kdeinit", NULL);
     }
   return 1;
}

EINTERN int
e_deskenv_shutdown(void)
{
   return 1;
}
