#ifndef E_MOD_QUICKPANEL_H
# define E_MOD_QUICKPANEL_H

# define E_ILLUME_QP_TYPE 0xE1b0990

int e_mod_quickpanel_init(void);
int e_mod_quickpanel_shutdown(void);

E_Illume_Quickpanel *e_mod_quickpanel_new(E_Zone *zone);
void e_mod_quickpanel_show(E_Illume_Quickpanel *qp);
void e_mod_quickpanel_hide(E_Illume_Quickpanel *qp);

#endif
