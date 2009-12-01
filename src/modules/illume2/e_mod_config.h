#ifndef E_MOD_CONFIG_H
# define E_MOD_CONFIG_H

# define IL_CONFIG_MIN 2
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

   struct 
     {
        const char *name;
        struct 
          {
             const char *class;
             const char *name;
             const char *title;
             int win_type;
             struct 
               {
                  int class, name, title, win_type;
               } match;
          } vkbd, softkey, home, indicator;
        struct 
          {
             int dual, side;
          } mode;
     } policy;

   // Not User Configurable. Placeholders
   const char *mod_dir;
   E_Config_Dialog *cfd;
};

EAPI int il_config_init(E_Module *m);
EAPI int il_config_shutdown(void);
EAPI int il_config_save(void);

extern EAPI Il_Config *il_cfg;

#endif
