/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef enum _E_Sys_Action E_Sys_Action;

enum _E_Sys_Action
{
   E_SYS_NONE,
   E_SYS_EXIT,
   E_SYS_RESTART,
   E_SYS_EXIT_NOW,
   E_SYS_LOGOUT,
   E_SYS_HALT,
   E_SYS_HALT_NOW,
   E_SYS_REBOOT,
   E_SYS_SUSPEND,
   E_SYS_HIBERNATE
};

#else
#ifndef E_SYS_H
#define E_SYS_H

EAPI int e_sys_init(void);
EAPI int e_sys_shutdown(void);
EAPI int e_sys_action_possible_get(E_Sys_Action a);
EAPI int e_sys_action_do(E_Sys_Action a, char *param);
    
#endif
#endif
