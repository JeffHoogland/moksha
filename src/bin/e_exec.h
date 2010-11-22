#ifdef E_TYPEDEFS

typedef struct _E_Exec_Instance E_Exec_Instance;

#else
#ifndef E_EXEC_H
#define E_EXEC_H

struct _E_Exec_Instance
{
   Efreet_Desktop *desktop;
   const char     *key;
   Ecore_Exe      *exe;
   int             startup_id;
   double          launch_time;
   Ecore_Timer    *expire_timer;
};

EINTERN int  e_exec_init(void);
EINTERN int  e_exec_shutdown(void);
EAPI E_Exec_Instance *e_exec(E_Zone *zone, Efreet_Desktop *desktop, const char *exec, Eina_List *files, const char *launch_method);

EAPI Efreet_Desktop *e_exec_startup_id_pid_find(int startup_id, pid_t pid);

#endif
#endif
