#include "e.h"

static int e_argc = 0;
static char **e_argv = NULL;

void
e_exec_set_args(int argc, char **argv)
{
   e_argc = argc;
   e_argv = argv;
}

void
e_exec_restart(void)
{
   int i, num;
   char exe[4096];

   printf("e_exec_restart()\n");
   /* unset events on root */
   e_window_set_events(0, XEV_NONE);
   /* destroy all desktops */
   num = e_desktops_get_num();
   for (i = 0; i < num; i++)
     {
	E_Desktop *desk;
	
	desk = e_desktops_get(0);
	e_desktops_delete(desk);
     }
   /* ensure it's all done */
   e_sync();
   /* rerun myself */
   exe[0] = 0;
   for (i = 0; i < e_argc; i++)
     {
	strcat(exe, e_argv[i]);
	strcat(exe, " ");
     }
   execl("/bin/sh", "/bin/sh", "-c", exe, NULL);   
}

pid_t
e_exec_run(char *exe)
{
   pid_t               pid;
   
   pid = fork();
   if (pid)
     return pid;
   setsid();
   execl("/bin/sh", "/bin/sh", "-c", exe, NULL);
   exit(0);
   return 0;
}

pid_t
e_exec_run_in_dir(char *exe, char *dir)
{
   pid_t               pid;
   
   pid = fork();
   if (pid)
     return pid;
   chdir(dir);
   setsid();
   execl("/bin/sh", "/bin/sh", "-c", exe, NULL);
   exit(0);
   return 0;
}

pid_t
e_run_in_dir_with_env(char *exe, char *dir, int *launch_id_ret, char **env, char *launch_path)
{
   static int launch_id = 0;
   char preload_paths[4096];
   char preload[4096];
   char *exe2;
   pid_t               pid;
   
   launch_id++;
   if (launch_id_ret) *launch_id_ret = launch_id;
   pid = fork();
   if (pid) return pid;
   chdir(dir);
   setsid();
   if (env)
     {
	while (*env)
	  {
	     e_set_env(env[0], env[1]);
	     env += 2;
	  }
     }
   /* launch Id hack - if it's an X program the windows popped up should */
   /* have this launch Id number set on them - as well as process ID */
   /* machine name, and user name */
   if (launch_path) e_set_env("E_HACK_LAUNCH_PATH", launch_path);
   sprintf(preload_paths, "E_HACK_LAUNCH_ID=%i LD_PRELOAD_PATH='%s'", 
	   launch_id, PACKAGE_LIB_DIR);
   sprintf(preload, "LD_PRELOAD='libehack.so libX11.so libdl.so'");
   exe2 = malloc(strlen(exe) + 1 + 
		 strlen(preload_paths) + 1 + 
		 strlen(preload) + 1);
   sprintf(exe2, "%s %s %s", preload_paths, preload, exe);
   execl("/bin/sh", "/bin/sh", "-c", exe2, NULL);
   exit(0);
   return 0;
}
