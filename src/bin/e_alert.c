#include "e.h"
#include <sys/wait.h>

/* public variables */
EAPI unsigned long e_alert_composite_win = 0;

EINTERN int
e_alert_init(void)
{
   return 1;
}

EINTERN int
e_alert_shutdown(void)
{
   return 1;
}

EAPI void 
e_alert_show(int sig) 
{
   char *args[4];
   pid_t pid;

#define E_ALERT_EXE "/enlightenment/utils/enlightenment_alert"

   args[0] = alloca(strlen(e_prefix_lib_get()) + strlen(E_ALERT_EXE) + 1);
   strcpy(args[0], e_prefix_lib_get());
   strcat(args[0], E_ALERT_EXE);

   args[1] = alloca(10);
   snprintf(args[1], 10, "%d", sig);

   args[2] = alloca(21);
   snprintf(args[2], 21, "%lu", (long unsigned int)getpid());

   args[3] = alloca(21);
   snprintf(args[3], 21, "%lu", e_alert_composite_win);

   pid = fork();
   if (pid < -1)
     goto restart_e;

   if (pid == 0)
     {
        /* The child process */
        execvp(args[0], args);
     }
   else
     {
        /* The parent process */
        pid_t ret;
        int status = 0;

        do
          {
             ret = waitpid(pid, &status, 0);
             if (errno == ECHILD)
               break ;
          }
        while (ret != pid);

        if (status == 0)
          goto restart_e;

        if (!WIFEXITED(status))
          goto restart_e;

        if (WEXITSTATUS(status) == 1)
          goto restart_e;

        exit(-11);
     }

 restart_e:
   ecore_app_restart();
}

