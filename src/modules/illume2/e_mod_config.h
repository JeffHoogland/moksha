#ifndef E_MOD_CONFIG_H
# define E_MOD_CONFIG_H

# define IL_CONFIG_MIN 1
# define IL_CONFIG_MAJ 0

int e_mod_config_init(E_Module *m);
int e_mod_config_shutdown(void);
int e_mod_config_save(void);

#endif
