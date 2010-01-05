#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"
#include "e_mod_win.h"

/* local variables */
static Eina_List *swins = NULL;

EAPI E_Module_Api e_modapi = { E_MODULE_API_VERSION, "Illume-Softkey" };

/* public functions */
EAPI void *
e_modapi_init(E_Module *m) 
{
   E_Screen *screen;
   const Eina_List *l;

   if (!il_sk_config_init(m)) return NULL;

   e_mod_sk_win_init();

   EINA_LIST_FOREACH(e_xinerama_screens_get(), l, screen) 
     {
        Il_Sk_Win *swin = NULL;

        if (!(swin = e_mod_sk_win_new(screen))) continue;
        swins = eina_list_append(swins, swin);
     }

   return m;
}

EAPI int 
e_modapi_shutdown(E_Module *m) 
{
   Il_Sk_Win *swin;

   EINA_LIST_FREE(swins, swin) 
     {
        e_object_del(E_OBJECT(swin));
        swin = NULL;
     }

   e_mod_sk_win_shutdown();

   il_sk_config_shutdown();

   return 1;
}

EAPI int 
e_modapi_save(E_Module *m) 
{
   return il_sk_config_save();
}
