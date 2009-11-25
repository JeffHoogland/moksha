#ifndef E_MOD_CONFIG_H
#define E_MOD_CONFIG_H

#define IL_CONFIG_MIN 0
#define IL_CONFIG_MAJ 0

typedef struct _Il_Sk_Config Il_Sk_Config;
struct _Il_Sk_Config 
{
   int version;

   const char *mod_dir;
};

int il_sk_config_init(E_Module *m);
int il_sk_config_shutdown(void);
int il_sk_config_save(void);
void il_sk_config_show(E_Container *con, const char *params);

extern EAPI Il_Sk_Config *il_sk_cfg;

#endif
