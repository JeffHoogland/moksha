#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"

EAPI Il_Sk_Config *il_sk_cfg = NULL;
static E_Config_DD *conf_edd = NULL;

/* public functions */
EAPI int 
il_sk_config_init(E_Module *m) 
{
   conf_edd = E_CONFIG_DD_NEW("Illume-Softkey_Cfg", Il_Sk_Config);
   #undef T
   #undef D
   #define T Il_Sk_Config
   #define D conf_edd
   E_CONFIG_VAL(D, T, version, INT);

   il_sk_cfg = e_config_domain_load("module.illume-softkey", conf_edd);
   if ((il_sk_cfg) && 
       ((il_sk_cfg->version >> 16) < IL_CONFIG_MAJ)) 
     {
        E_FREE(il_sk_cfg);
        il_sk_cfg = NULL;
     }
   if (!il_sk_cfg) 
     {
        il_sk_cfg = E_NEW(Il_Sk_Config, 1);
        il_sk_cfg->version = 0;
     }
   if (il_sk_cfg) 
     {
        /* Add new config variables here */
        /* if ((il_sk_cfg->version & 0xffff) < 1) */
        il_sk_cfg->version = (IL_CONFIG_MAJ << 16) | IL_CONFIG_MIN;
     }
   il_sk_cfg->mod_dir = eina_stringshare_add(m->dir);
   return 1;
}

EAPI int 
il_sk_config_shutdown(void) 
{
   if (il_sk_cfg->mod_dir) eina_stringshare_del(il_sk_cfg->mod_dir);
   il_sk_cfg->mod_dir = NULL;

   E_FREE(il_sk_cfg);
   il_sk_cfg = NULL;

   E_CONFIG_DD_FREE(conf_edd);
   return 1;
}

EAPI int 
il_sk_config_save(void) 
{
   e_config_domain_save("module.illume-softkey", conf_edd, il_sk_cfg);
   return 1;
}

EAPI void 
il_sk_config_show(E_Container *con, const char *params) 
{

}
