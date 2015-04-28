#ifndef E_ILLUME_PRIVATE_H
# define E_ILLUME_PRIVATE_H

# include "e_illume.h"

/* define policy object type */
# define E_ILLUME_POLICY_TYPE 0xE0b200b

/* external variable to store list of quickpanels */
extern Eina_List *_e_illume_qps;

/* external variable to store keyboard */
extern E_Illume_Keyboard *_e_illume_kbd;

/* external variable to store active config */
extern E_Illume_Config *_e_illume_cfg;

/* external variable to store module directory */
extern const char *_e_illume_mod_dir;

/* external event for policy changes */
extern int E_ILLUME_POLICY_EVENT_CHANGE;

#endif
