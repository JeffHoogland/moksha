#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"

EAPI Il_Bar_Config *il_bar_cfg = NULL;
static E_Config_DD *conf_edd = NULL;

/* public functions */
EAPI int 
il_bar_config_init(E_Module *m) 
{
   conf_edd = E_CONFIG_DD_NEW("Illume-Bar_Cfg", Il_Bar_Config);
   #undef T
   #undef D
   #define T Il_Bar_Config
   #define D conf_edd
   E_CONFIG_VAL(D, T, version, INT);

   return 1;
}

EAPI int 
il_bar_config_shutdown(void) 
{
   E_FREE(il_bar_cfg);
   il_bar_cfg = NULL;

   E_CONFIG_DD_FREE(conf_edd);
   return 1;
}

EAPI int 
il_bar_config_save(void) 
{
   return 1;
}

EAPI void 
il_bar_config_show(E_Container *con, const char *params) 
{

}
