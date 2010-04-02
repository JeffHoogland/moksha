#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"

/* local variables */
EAPI Il_Sft_Config *il_sft_cfg = NULL;
static E_Config_DD *conf_edd = NULL;

int 
il_sft_config_init(void) 
{
   conf_edd = E_CONFIG_DD_NEW("Illume-Softkey_Cfg", Il_Sft_Config);
   #undef T
   #undef D
   #define T Il_Sft_Config
   #define D conf_edd
   E_CONFIG_VAL(D, T, version, INT);
   E_CONFIG_VAL(D, T, height, INT);

   il_sft_cfg = e_config_domain_load("module.illume-softkey", conf_edd);
   if ((il_sft_cfg) && 
       ((il_sft_cfg->version >> 16) < IL_CONFIG_MAJ))
     {
        E_FREE(il_sft_cfg);
     }
   if (!il_sft_cfg) 
     {
        il_sft_cfg = E_NEW(Il_Sft_Config, 1);
        il_sft_cfg->version = 0;
        il_sft_cfg->height = 32;
     }
   if (il_sft_cfg) 
     il_sft_cfg->version = (IL_CONFIG_MAJ << 16) | IL_CONFIG_MIN;

   return 1;
}

int 
il_sft_config_shutdown(void) 
{
   E_FREE(il_sft_cfg);
   E_CONFIG_DD_FREE(conf_edd);

   return 1;
}

int 
il_sft_config_save(void) 
{
   return e_config_domain_save("module.illume-softkey", conf_edd, il_sft_cfg);
}
