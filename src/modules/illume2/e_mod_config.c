#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"

/* local variables */
EAPI Il_Config *il_cfg = NULL;
static E_Config_DD *conf_edd = NULL;

/* public functions */
EAPI int 
il_config_init(E_Module *m) 
{
   conf_edd = E_CONFIG_DD_NEW("Illume_Cfg", Il_Config);
   #undef T
   #undef D
   #define T Il_Config
   #define D conf_edd
   E_CONFIG_VAL(D, T, version, INT);
   E_CONFIG_VAL(D, T, sliding.kbd.duration, INT);
   E_CONFIG_VAL(D, T, sliding.softkey.duration, INT);

   il_cfg = e_config_domain_load("module.illume2", conf_edd);
   if ((il_cfg) && 
       ((il_cfg->version >> 16) < IL_CONFIG_MAJ)) 
     {
        E_FREE(il_cfg);
        il_cfg = NULL;
     }
   if (!il_cfg) 
     {
        il_cfg = E_NEW(Il_Config, 1);
        il_cfg->version = 0;
        il_cfg->sliding.kbd.duration = 1000;
        il_cfg->sliding.softkey.duration = 1000;
     }
   if (il_cfg) 
     {
        /* Add new config variables here */
        /* if ((il_cfg->version & 0xffff) < 1) */
        il_cfg->version = (IL_CONFIG_MAJ << 16) | IL_CONFIG_MIN;
     }
   il_cfg->mod_dir = eina_stringshare_add(m->dir);
   return 1;
}

EAPI int 
il_config_shutdown(void) 
{
   if (il_cfg->mod_dir) eina_stringshare_del(il_cfg->mod_dir);
   E_FREE(il_cfg);
   return 1;
}

EAPI int 
il_config_save(void) 
{
   e_config_domain_save("module.illume2", conf_edd, il_cfg);
   return 1;
}
