#ifdef E_TYPEDEFS

typedef struct _E_Sys_Con_Action E_Sys_Con_Action;
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

struct _E_Sys_Con_Action
{
   const char *label;
   const char *icon_group;
   const char *button_name;
   void (*func) (void *data);
   const void *data;
   Eina_Bool disabled : 1;
};

#else
#ifndef E_SYS_H
#define E_SYS_H

EAPI extern int E_EVENT_SYS_SUSPEND;
EAPI extern int E_EVENT_SYS_HIBERNATE;
EAPI extern int E_EVENT_SYS_RESUME;

EINTERN int e_sys_init(void);
EINTERN int e_sys_shutdown(void);
EAPI int e_sys_action_possible_get(E_Sys_Action a);
EAPI int e_sys_action_do(E_Sys_Action a, char *param);
EAPI int e_sys_action_raw_do(E_Sys_Action a, char *param);

EAPI E_Sys_Con_Action *e_sys_con_extra_action_register(const char *label,
                                                       const char *icon_group,
                                                       const char *button_name,
                                                       void (*func) (void *data),
                                                       const void *data);
EAPI void e_sys_con_extra_action_unregister(E_Sys_Con_Action *sca);
EAPI const Eina_List *e_sys_con_extra_action_list_get(void);
EAPI void e_sys_handlers_set(void (*suspend_func) (void),
                             void (*hibernate_func) (void),
                             void (*reboot_func) (void),
                             void (*shutdown_func) (void),
                             void (*logout_func) (void),
                             void (*resume_func) (void));

#endif
#endif
