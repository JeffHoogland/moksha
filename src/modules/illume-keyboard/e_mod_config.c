#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"

EAPI Il_Kbd_Config *il_kbd_cfg = NULL;
static E_Config_DD *conf_edd = NULL;

/* public functions */
EAPI int 
il_kbd_config_init(E_Module *m) 
{
   conf_edd = E_CONFIG_DD_NEW("Illume_Kbd_Cfg", Il_Kbd_Config);
   #undef T
   #undef D
   #define T Il_Kbd_Config
   #define D conf_edd
   E_CONFIG_VAL(D, T, version, INT);
   E_CONFIG_VAL(D, T, use_internal, INT);
   E_CONFIG_VAL(D, T, run_keyboard, STR);
   E_CONFIG_VAL(D, T, dict, STR);

   il_kbd_cfg = e_config_domain_load("module.illume-keyboard", conf_edd);
   if ((il_kbd_cfg) && 
       ((il_kbd_cfg->version >> 16) < IL_CONFIG_MAJ)) 
     {
        E_FREE(il_kbd_cfg);
        il_kbd_cfg = NULL;
     }
   if (!il_kbd_cfg) 
     {
        il_kbd_cfg = E_NEW(Il_Kbd_Config, 1);
        il_kbd_cfg->version = 0;
        il_kbd_cfg->use_internal = 1;
        il_kbd_cfg->run_keyboard = NULL;
        il_kbd_cfg->dict = eina_stringshare_add("English_(US).dic");
     }
   if (il_kbd_cfg) 
     {
        /* Add new config variables here */
        /* if ((il_kbd_cfg->version & 0xffff) < 1) */
        il_kbd_cfg->version = (IL_CONFIG_MAJ << 16) | IL_CONFIG_MIN;
     }

   il_kbd_cfg->mod_dir = eina_stringshare_add(m->dir);
   return 1;
}

EAPI int 
il_kbd_config_shutdown(void) 
{
   if (il_kbd_cfg->mod_dir) eina_stringshare_del(il_kbd_cfg->mod_dir);
   if (il_kbd_cfg->run_keyboard) eina_stringshare_del(il_kbd_cfg->run_keyboard);
   if (il_kbd_cfg->dict) eina_stringshare_del(il_kbd_cfg->dict);

   E_FREE(il_kbd_cfg);
   il_kbd_cfg = NULL;

   E_CONFIG_DD_FREE(conf_edd);
   return 1;
}

EAPI int 
il_kbd_config_save(void) 
{
   e_config_domain_save("module.illume-keyboard", conf_edd, il_kbd_cfg);
   return 1;
}
