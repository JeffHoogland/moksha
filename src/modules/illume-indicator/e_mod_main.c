#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"
#include "e_mod_win.h"

/* local variables */
static Il_Ind_Win *iwin = NULL;

EAPI E_Module_Api e_modapi = { E_MODULE_API_VERSION, "Illume-Indicator" };

EAPI void *
e_modapi_init(E_Module *m) 
{
   if (!il_ind_config_init(m)) return NULL;

   e_mod_win_init();

   iwin = e_mod_win_new();

   return m;
}

EAPI int 
e_modapi_shutdown(E_Module *m) 
{
   e_object_del(E_OBJECT(iwin));
   iwin = NULL;

   e_mod_win_shutdown();
   il_ind_config_shutdown();
   return 1;
}

EAPI int 
e_modapi_save(E_Module *m) 
{
   return il_ind_config_save();
}

