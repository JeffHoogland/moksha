#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"
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
   Eina_List *ml;

   /* set module priority so we load before others */
   e_module_priority_set(m, 85);

   /* set module directory variable */
   _sft_mod_dir = eina_stringshare_add(m->dir);

   /* init config subsystem */
   if (!il_sft_config_init()) 
     {
        /* clear module directory variable */
        if (_sft_mod_dir) eina_stringshare_del(_sft_mod_dir);
        _sft_mod_dir = NULL;
        return NULL;
     }

   /* loop through the managers (root windows) */
   EINA_LIST_FOREACH(e_manager_list(), ml, man) 
     {
        E_Container *con;
        Eina_List *cl;

        /* loop through containers */
        EINA_LIST_FOREACH(man->containers, cl, con) 
          {
             E_Zone *zone;
             Eina_List *zl;

             /* for each zone, create a softkey window */
             EINA_LIST_FOREACH(con->zones, zl, zone) 
               {
                  Sft_Win *swin;

                  /* try to create new softkey window */
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
   Sft_Win *swin;

   /* destroy the softkey windows */
   EINA_LIST_FREE(swins, swin)
     e_object_del(E_OBJECT(swin));

   /* reset softkey geometry for conformant apps */
   ecore_x_e_illume_softkey_geometry_set(ecore_x_window_root_first_get(), 
                                         0, 0, 0, 0);

   /* shutdown config */
   il_sft_config_shutdown();

   /* clear module directory variable */
   if (_sft_mod_dir) eina_stringshare_del(_sft_mod_dir);
   _sft_mod_dir = NULL;

   return 1;
}

EAPI int 
e_modapi_save(E_Module *m) 
{
   return il_sft_config_save();
}
