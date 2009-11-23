#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"

EAPI E_Module_Api e_modapi = { E_MODULE_API_VERSION, "Illume-Softkey" };

/* public functions */
EAPI void *
e_modapi_init(E_Module *m) 
{
   if (!il_sk_config_init(m)) return NULL;
   return m;
}

EAPI int 
e_modapi_shutdown(E_Module *m) 
{
   il_sk_config_shutdown();
   return 1;
}

EAPI int 
e_modapi_save(E_Module *m) 
{
   return il_sk_config_save();
}
