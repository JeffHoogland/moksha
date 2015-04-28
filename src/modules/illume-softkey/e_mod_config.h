#ifndef E_MOD_CONFIG_H
#define E_MOD_CONFIG_H

#define IL_CONFIG_MIN 0
#define IL_CONFIG_MAJ 0

typedef struct _Il_Sft_Config Il_Sft_Config;

struct _Il_Sft_Config 
{
   int version, height;
};

int il_sft_config_init(void);
int il_sft_config_shutdown(void);
int il_sft_config_save(void);

extern EAPI Il_Sft_Config *il_sft_cfg;

#endif
