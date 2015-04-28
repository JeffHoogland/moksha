#ifndef E_MOD_CONFIG_H
#define E_MOD_CONFIG_H

#define IL_CONFIG_MIN 0
#define IL_CONFIG_MAJ 0

typedef struct _Il_Home_Config Il_Home_Config;

struct _Il_Home_Config 
{
   int version;
   int mode, icon_size;
   int single_click, single_click_delay;

   // Not User Configurable. Placeholders
   const char *mod_dir;
   E_Config_Dialog *cfd;
};

int il_home_config_init(E_Module *m);
int il_home_config_shutdown(void);
int il_home_config_save(void);
void il_home_config_show(E_Container *con, const char *params);

extern EAPI Il_Home_Config *il_home_cfg;

#endif
