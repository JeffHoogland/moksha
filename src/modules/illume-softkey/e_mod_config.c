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

   return 1;
}

EAPI int 
il_sk_config_shutdown(void) 
{
   E_FREE(il_sk_cfg);
   il_sk_cfg = NULL;

   E_CONFIG_DD_FREE(conf_edd);
   return 1;
}

EAPI int 
il_sk_config_save(void) 
{
   return 1;
}

EAPI void 
il_sk_config_show(E_Container *con, const char *params) 
{

}
