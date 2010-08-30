#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"
#include "e_mod_ind_win.h"

#ifdef HAVE_ENOTIFY
# include "e_mod_notify.h"
#endif

/* local variables */
static Eina_List *iwins = NULL;

/* external variables */
const char *_ind_mod_dir = NULL;

EAPI E_Module_Api e_modapi = { E_MODULE_API_VERSION, "Illume-Indicator" };

EAPI void *
e_modapi_init(E_Module *m) 
{
   E_Manager *man;
   Eina_List *ml;

   /* set module priority so we load before others */
   e_module_priority_set(m, 90);

   /* set module directory variable */
   _ind_mod_dir = eina_stringshare_add(m->dir);

   /* init config subsystem */
   if (!il_ind_config_init()) 
     {
        /* clear module directory variable */
        if (_ind_mod_dir) eina_stringshare_del(_ind_mod_dir);
        _ind_mod_dir = NULL;
        return NULL;
     }

#ifdef HAVE_ENOTIFY
   if (!(e_mod_notify_init())) 
     {
        /* shutdown config */
        il_ind_config_shutdown();

        /* clear module directory variable */
        if (_ind_mod_dir) eina_stringshare_del(_ind_mod_dir);
        _ind_mod_dir = NULL;
        return NULL;
     }
#endif

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

             /* TODO: Make this configurable so illume2 can be run
              * on just one zone/screen/etc */

             /* for each zone, create an indicator window */
             EINA_LIST_FOREACH(con->zones, zl, zone) 
               {
                  Ind_Win *iwin;

                  /* try to create new indicator window */
                  if (!(iwin = e_mod_ind_win_new(zone))) continue;
                  iwins = eina_list_append(iwins, iwin);
               }
          }
     }

   return m;
}

EAPI int 
e_modapi_shutdown(E_Module *m __UNUSED__) 
{
   Ind_Win *iwin;

   /* destroy the indicator windows */
   EINA_LIST_FREE(iwins, iwin)
     e_object_del(E_OBJECT(iwin));

   /* reset indicator geometry for conformant apps */
   ecore_x_e_illume_indicator_geometry_set(ecore_x_window_root_first_get(), 
                                           0, 0, 0, 0);

#ifdef HAVE_ENOTIFY
   e_mod_notify_shutdown();
#endif

   /* shutdown config */
   il_ind_config_shutdown();

   /* clear module directory variable */
   if (_ind_mod_dir) eina_stringshare_del(_ind_mod_dir);
   _ind_mod_dir = NULL;

   return 1;
}

EAPI int 
e_modapi_save(E_Module *m __UNUSED__) 
{
   return il_ind_config_save();
}
