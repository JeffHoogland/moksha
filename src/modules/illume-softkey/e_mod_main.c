#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"
#include "e_mod_win.h"

static Il_Sk_Win *swin = NULL;

EAPI E_Module_Api e_modapi = { E_MODULE_API_VERSION, "Illume-Softkey" };

/* public functions */
EAPI void *
e_modapi_init(E_Module *m) 
{
   if (!il_sk_config_init(m)) return NULL;

   e_mod_win_init();

   swin = e_mod_win_new();

   return m;
}

EAPI int 
e_modapi_shutdown(E_Module *m) 
{
   e_object_del(E_OBJECT(swin));
   swin = NULL;

   e_mod_win_shutdown();
   il_sk_config_shutdown();
   return 1;
}

EAPI int 
e_modapi_save(E_Module *m) 
{
   return il_sk_config_save();
}
