/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

#else
#ifndef E_EXEC_H
#define E_EXEC_H

EAPI int  e_exec_init(void);
EAPI int  e_exec_shutdown(void);
EAPI int  e_exec(E_Zone *zone, Efreet_Desktop *desktop, const char *exec, Ecore_List *files, const char *launch_method);

EAPI Efreet_Desktop *e_exec_startup_id_pid_find(int startup_id, pid_t pid);

#endif
#endif
