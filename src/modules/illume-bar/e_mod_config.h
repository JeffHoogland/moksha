#ifndef E_MOD_CONFIG_H
#define E_MOD_CONFIG_H

#define IL_CONFIG_MIN 0
#define IL_CONFIG_MAJ 0

typedef struct _Il_Bar_Config Il_Bar_Config;
struct _Il_Bar_Config 
{
   int version;
};

EAPI int il_bar_config_init(E_Module *m);
EAPI int il_bar_config_shutdown(void);
EAPI int il_bar_config_save(void);
EAPI void il_bar_config_show(E_Container *con, const char *params);

extern EAPI Il_Bar_Config *il_bar_cfg;

#endif
