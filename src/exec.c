#include "debug.h"
#include "exec.h"
#include "desktops.h"
#include "util.h"

static int          e_argc = 0;
static char       **e_argv = NULL;

typedef struct _e_hack_found_cb E_Hack_Found_CB;

struct _e_hack_found_cb
{
   int                 dirty;
   void                (*func) (Window win, void *data);
   void               *func_data;
};

static Evas_List    hack_found_cb = NULL;

void               *
e_exec_broadcast_cb_add(void (*func) (Window win, void *_data), void *data)
{
   E_Hack_Found_CB    *cb;

   cb = NEW(E_Hack_Found_CB, 1);
   ZERO(cb, E_Hack_Found_CB, 1);
   cb->func = func;
   cb->func_data = data;
   hack_found_cb = evas_list_append(hack_found_cb, cb);
   return cb;
}

void
e_exec_broadcast_cb_del(void *cbp)
{
   E_Hack_Found_CB    *cb;

   cb = (E_Hack_Found_CB *) cbp;
   cb->dirty = 1;
}

void
e_exec_broadcast_e_hack_found(Window win)
{
   Evas_List           l;

   for (l = hack_found_cb; l; l = l->next)
     {
	E_Hack_Found_CB    *cb;

	cb = l->data;
	if (!cb->dirty)
	  {
	     if (cb->func)
		cb->func(win, cb->func_data);
	  }
     }
 again:
   for (l = hack_found_cb; l; l = l->next)
     {
	E_Hack_Found_CB    *cb;

	cb = l->data;
	if (cb->dirty)
	  {
	     hack_found_cb = evas_list_remove(hack_found_cb, cb);
	     goto again;
	  }
     }
}

void
e_exec_set_args(int argc, char **argv)
{
   D_ENTER;

   e_argc = argc;
   e_argv = argv;

   D_RETURN;
}

void
e_exec_restart(void)
{
   int                 i, num;
   char                exe[PATH_MAX];

   D_ENTER;

   D("e_exec_restart()\n");
   /* unset events on root */
   ecore_window_set_events(0, XEV_NONE);
   /* reset focus mode to default pointer root */
   ecore_focus_mode_reset();
   /* destroy all desktops */
   num = e_desktops_get_num();
   for (i = 0; i < num; i++)
     {
	E_Desktop          *desk;

	desk = e_desktops_get(0);
	e_desktops_delete(desk);
     }
   /* ensure it's all done */
   ecore_sync();
   /* rerun myself */
   exe[0] = 0;
   for (i = 0; i < e_argc; i++)
     {
	strcat(exe, e_argv[i]);
	strcat(exe, " ");
     }
   execl("/bin/sh", "/bin/sh", "-c", exe, NULL);

   D_RETURN;
}

pid_t
e_exec_run(char *exe)
{
   pid_t               pid;

   D_ENTER;

   pid = fork();
   if (pid)
      D_RETURN_(pid);
   setsid();
   execl("/bin/sh", "/bin/sh", "-c", exe, NULL);
   exit(0);

   D_RETURN_(0);
}

pid_t
e_exec_run_in_dir(char *exe, char *dir)
{
   pid_t               pid;

   D_ENTER;

   pid = fork();
   if (pid)
      D_RETURN_(pid);
   chdir(dir);
   setsid();
   execl("/bin/sh", "/bin/sh", "-c", exe, NULL);
   exit(0);

   D_RETURN_(0);
}

pid_t
e_exec_in_dir_with_env(char *exe, char *dir, int *launch_id_ret, char **env,
		       char *launch_path)
{
   static int          launch_id = 0;
   char                preload_paths[PATH_MAX];
   char                preload[PATH_MAX];
   char               *exe2;
   pid_t               pid;

   D_ENTER;

   launch_id++;
   if (launch_id_ret)
      *launch_id_ret = launch_id;
   pid = fork();
   if (pid)
      D_RETURN_(pid);
   chdir(dir);
   setsid();
   if (env)
     {
	while (*env)
	  {
	     e_util_set_env(env[0], env[1]);
	     env += 2;
	  }
     }
   /* launch Id hack - if it's an X program the windows popped up should */
   /* have this launch Id number set on them - as well as process ID */
   /* machine name, and user name */
   if (launch_path)
      e_util_set_env("E_HACK_LAUNCH_PATH", launch_path);
   snprintf(preload_paths, PATH_MAX, "E_HACK_LAUNCH_ID=%i LD_PRELOAD_PATH='%s'",
	    launch_id, PACKAGE_LIB_DIR);
   snprintf(preload, PATH_MAX, "LD_PRELOAD='libehack.so libX11.so libdl.so'");
   exe2 = malloc(strlen(exe) + 1 +
		 strlen(preload_paths) + 1 + strlen(preload) + 1);
   snprintf(exe2, PATH_MAX, "%s %s %s", preload_paths, preload, exe);

   execl("/bin/sh", "/bin/sh", "-c", exe2, NULL);
   exit(0);

   D_RETURN_(0);
}
