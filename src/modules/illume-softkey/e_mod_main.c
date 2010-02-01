#include "e.h"
#include "e_mod_main.h"
#include "e_mod_sft_win.h"

/* local variables */
static Eina_List *swins = NULL;

/* external variables */
const char *_sft_mod_dir = NULL;

EAPI E_Module_Api e_modapi = { E_MODULE_API_VERSION, "Illume-Softkey" };

EAPI void *
e_modapi_init(E_Module *m) 
{
   E_Manager *man;
   Eina_List *l;

   e_module_priority_set(m, 85);

   /* setup variable to hold module directory */
   _sft_mod_dir = eina_stringshare_add(m->dir);

   /* loop through the managers (root windows) */
   EINA_LIST_FOREACH(e_manager_list(), l, man) 
     {
        E_Container *con;
        Eina_List *cl;

        /* loop through the containers */
        EINA_LIST_FOREACH(man->containers, cl, con) 
          {
             E_Zone *zone;
             Eina_List *zl;

             /* for each zone in this container, create a new softkey window */
             EINA_LIST_FOREACH(con->zones, zl, zone) 
               {
                  Il_Sft_Win *swin;

                  if (!(swin = e_mod_sft_win_new(zone))) continue;
                  swins = eina_list_append(swins, swin);
               }
          }
     }

   return m;
}

EAPI int 
e_modapi_shutdown(E_Module *m) 
{
   Il_Sft_Win *swin;

   /* destroy the softkey windows */
   EINA_LIST_FREE(swins, swin) 
     {
        e_object_del(E_OBJECT(swin));
        swin = NULL;
     }

   /* clear the module directory variable */
   if (_sft_mod_dir) eina_stringshare_del(_sft_mod_dir);
   _sft_mod_dir = NULL;

   /* reset bottom panel geometry to zero for conformant apps */
   ecore_x_e_illume_bottom_panel_geometry_set(ecore_x_window_root_first_get(), 
                                              0, 0, 0, 0);
   return 1;
}

EAPI int 
e_modapi_save(E_Module *m) 
{
   return 1;
}
