#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"

/* local variables */
EAPI Il_Ind_Config *il_ind_cfg = NULL;
static E_Config_DD *conf_edd = NULL;

int 
il_ind_config_init(void) 
{
   conf_edd = E_CONFIG_DD_NEW("Illume-Indicator_Cfg", Il_Ind_Config);
   #undef T
   #undef D
   #define T Il_Ind_Config
   #define D conf_edd
   E_CONFIG_VAL(D, T, version, INT);
   E_CONFIG_VAL(D, T, height, INT);

   il_ind_cfg = e_config_domain_load("module.illume-indicator", conf_edd);
   if ((il_ind_cfg) && 
       ((il_ind_cfg->version >> 16) < IL_CONFIG_MAJ))
     {
        E_FREE(il_ind_cfg);
     }
   if (!il_ind_cfg) 
     {
        il_ind_cfg = E_NEW(Il_Ind_Config, 1);
        il_ind_cfg->version = 0;
        il_ind_cfg->height = 32;
     }
   if (il_ind_cfg) 
     il_ind_cfg->version = (IL_CONFIG_MAJ << 16) | IL_CONFIG_MIN;

   return 1;
}

int 
il_ind_config_shutdown(void) 
{
   E_FREE(il_ind_cfg);
   E_CONFIG_DD_FREE(conf_edd);

   return 1;
}

int 
il_ind_config_save(void) 
{
   return e_config_domain_save("module.illume-indicator", conf_edd, il_ind_cfg);
}
