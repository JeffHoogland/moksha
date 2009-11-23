#ifndef E_MOD_CONFIG_H
# define E_MOD_CONFIG_H

# define IL_CONFIG_MIN 0
# define IL_CONFIG_MAJ 0

typedef struct _Il_Config Il_Config;

struct _Il_Config 
{
   int version;

   struct 
     {
        struct 
          {
             int duration;
          } kbd, softkey;
     } sliding;

   // Not User Configurable. Placeholders
   const char *mod_dir;
   E_Config_Dialog *cfd;
};

EAPI int il_config_init(E_Module *m);
EAPI int il_config_shutdown(void);
EAPI int il_config_save(void);

extern EAPI Il_Config *il_cfg;

#endif
