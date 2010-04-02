#ifndef E_MOD_CONFIG_H
#define E_MOD_CONFIG_H

#define IL_CONFIG_MIN 0
#define IL_CONFIG_MAJ 0

typedef struct _Il_Ind_Config Il_Ind_Config;

struct _Il_Ind_Config 
{
   int version, height;
};

int il_ind_config_init(void);
int il_ind_config_shutdown(void);
int il_ind_config_save(void);

extern EAPI Il_Ind_Config *il_ind_cfg;

#endif
