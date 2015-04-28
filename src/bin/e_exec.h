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
   int             screen;
   int             desk_x, desk_y;
   int             used;
   int             walking;
   Eina_List      *watchers;
};

typedef enum
{
   E_EXEC_WATCH_STARTED,
   E_EXEC_WATCH_STOPPED,
   E_EXEC_WATCH_TIMEOUT
} E_Exec_Watch_Type;

EINTERN int  e_exec_init(void);
EINTERN int  e_exec_shutdown(void);
EAPI void e_exec_executor_set(E_Exec_Instance *(*func) (void *data, E_Zone *zone, Efreet_Desktop *desktop, const char *exec, Eina_List *files, const char *launch_method), const void *data);
EAPI E_Exec_Instance *e_exec(E_Zone *zone, Efreet_Desktop *desktop, const char *exec, Eina_List *files, const char *launch_method);
EAPI E_Exec_Instance *e_exec_startup_id_pid_instance_find(int id, pid_t pid);
EAPI Efreet_Desktop *e_exec_startup_id_pid_find(int startup_id, pid_t pid);
EAPI E_Exec_Instance *e_exec_startup_desktop_instance_find(Efreet_Desktop *desktop);
EAPI void e_exec_instance_found(E_Exec_Instance *inst);
EAPI void e_exec_instance_watcher_add(E_Exec_Instance *inst, void (*func) (void *data, E_Exec_Instance *inst, E_Exec_Watch_Type type), const void *data);
EAPI void e_exec_instance_watcher_del(E_Exec_Instance *inst, void (*func) (void *data, E_Exec_Instance *inst, E_Exec_Watch_Type type), const void *data);

#endif
#endif
