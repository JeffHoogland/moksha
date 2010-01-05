#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"
#include "e_mod_win.h"

/* local variables */
static Eina_List *iwins = NULL;

EAPI E_Module_Api e_modapi = { E_MODULE_API_VERSION, "Illume-Indicator" };

EAPI void *
e_modapi_init(E_Module *m) 
{
   E_Screen *screen;
   const Eina_List *l;

   if (!il_ind_config_init(m)) return NULL;

   e_mod_ind_win_init();

   EINA_LIST_FOREACH(e_xinerama_screens_get(), l, screen) 
     {
        Il_Ind_Win *iwin = NULL;

        if (!(iwin = e_mod_ind_win_new(screen))) continue;
        iwins = eina_list_append(iwins, iwin);
     }

   return m;
}

EAPI int 
e_modapi_shutdown(E_Module *m) 
{
   Il_Ind_Win *iwin;

   EINA_LIST_FREE(iwins, iwin) 
     {
        e_object_del(E_OBJECT(iwin));
        iwin = NULL;
     }

   e_mod_ind_win_shutdown();
   il_ind_config_shutdown();
   return 1;
}

EAPI int 
e_modapi_save(E_Module *m) 
{
   return il_ind_config_save();
}

