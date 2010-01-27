#include "e.h"
#include "e_mod_main.h"
#include "e_mod_win.h"

/* local variables */
static Eina_List *swins = NULL;
const char *mod_dir = NULL;

EAPI E_Module_Api e_modapi = { E_MODULE_API_VERSION, "Illume-Softkey" };

/* public functions */
EAPI void *
e_modapi_init(E_Module *m) 
{
   E_Manager *man;
   E_Container *con;
   E_Zone *zone;
   Eina_List *l, *ll, *lll;

   mod_dir = eina_stringshare_add(m->dir);
   EINA_LIST_FOREACH(e_manager_list(), l, man) 
     {
        EINA_LIST_FOREACH(man->containers, ll, con) 
          {
             EINA_LIST_FOREACH(con->zones, lll, zone) 
               {
                  Il_Sk_Win *swin = NULL;

                  if (!(swin = e_mod_softkey_win_new(zone))) continue;
                  swins = eina_list_append(swins, swin);
               }
          }
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

   if (mod_dir) eina_stringshare_del(mod_dir);
   mod_dir = NULL;

   ecore_x_e_illume_bottom_panel_geometry_set(ecore_x_window_root_first_get(), 
                                              0, 0, 0, 0);

   return 1;
}

EAPI int 
e_modapi_save(E_Module *m) 
{
   return 1;
}
