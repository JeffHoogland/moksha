#ifndef E_MOD_CONFIG_H
#define E_MOD_CONFIG_H

#define IL_CONFIG_MIN 0
#define IL_CONFIG_MAJ 1

typedef struct _Il_Kbd_Config Il_Kbd_Config;

struct _Il_Kbd_Config 
{
   int version;

   int use_internal;
   const char *dict, *run_keyboard;

   // Not User Configurable. Placeholders
   const char *mod_dir;
   int zoom_level;
   int slide_dim;
   double hold_timer;
   E_Config_Dialog *cfd;
};

EAPI int il_kbd_config_init(E_Module *m);
EAPI int il_kbd_config_shutdown(void);
EAPI int il_kbd_config_save(void);

EAPI void il_kbd_config_show(E_Container *con, const char *params);

extern EAPI Il_Kbd_Config *il_kbd_cfg;

#endif
