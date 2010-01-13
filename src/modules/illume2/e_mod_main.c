#include "E_Illume.h"
#include "e_mod_main.h"
#include "e_mod_config.h"
#include "e_mod_layout.h"
#include "e_kbd.h"
#include "e_quickpanel.h"

/* local variabels */
static E_Kbd *kbd = NULL;
static Eina_List *quickpanels = NULL;
int _e_illume_log_dom = -1;

EAPI E_Module_Api e_modapi = { E_MODULE_API_VERSION, "Illume2" };

EAPI void *
e_modapi_init(E_Module *m) 
{
   E_Container *con;
   E_Zone *zone;
   Eina_List *l;

   /* setup eina logging */
   if (_e_illume_log_dom < 0) 
     {
        _e_illume_log_dom = 
          eina_log_domain_register("illume2", E_ILLUME_DEFAULT_LOG_COLOR);
        if (_e_illume_log_dom < 0) 
          {
             EINA_LOG_CRIT("Could not register illume2 logging domain");
             return NULL;
          }
     }

   /* set module priority */
   e_module_priority_set(m, 100);

   /* initialize the config subsystem */
   if (!e_mod_config_init(m)) 
     {
        /* cleanup eina logging */
        if (_e_illume_log_dom > 0) 
          {
             eina_log_domain_unregister(_e_illume_log_dom);
             _e_illume_log_dom = -1;
          }
        return NULL;
     }

   con = e_container_current_get(e_manager_current_get());
   EINA_LIST_FOREACH(con->zones, l, zone) 
     {
        E_Illume_Config_Zone *cz;

        cz = e_illume_zone_config_get(zone->id);
        if (cz->mode.dual == 0) 
          ecore_x_e_illume_mode_set(zone->black_win, 
                                    ECORE_X_ILLUME_MODE_SINGLE);
        else 
          {
             if (cz->mode.side == 0)
               ecore_x_e_illume_mode_set(zone->black_win, 
                                         ECORE_X_ILLUME_MODE_DUAL_TOP);
             else
               ecore_x_e_illume_mode_set(zone->black_win, 
                                         ECORE_X_ILLUME_MODE_DUAL_LEFT);
          }
     }

   /* initialize the keyboard subsystem */
   e_kbd_init();

   /* initialize the quickpanel subsystem */
   e_quickpanel_init();

   /* initialize the layout subsystem */
   e_mod_layout_init();

   /* create a new vkbd */
   kbd = e_kbd_new();

   EINA_LIST_FOREACH(con->zones, l, zone) 
     {
        E_Quickpanel *qp;

        /* create a new quickpanel */
        if (!(qp = e_quickpanel_new(zone))) continue;
        quickpanels = eina_list_append(quickpanels, qp);
     }

   return m;
}

EAPI int 
e_modapi_shutdown(E_Module *m) 
{
   E_Quickpanel *qp;

   /* cleanup the quickpanels */
   EINA_LIST_FREE(quickpanels, qp)
     e_object_del(E_OBJECT(qp));

   /* cleanup the keyboard */
   e_object_del(E_OBJECT(kbd));
   kbd = NULL;

   /* shutdown the quickpanel subsystem */
   e_quickpanel_shutdown();

   /* shutdown the keyboard subsystem */
   e_kbd_shutdown();

   /* shutdown the layout subsystem */
   e_mod_layout_shutdown();

   /* shutdown the config subsystem */
   e_mod_config_shutdown();

   /* cleanup eina logging */
   if (_e_illume_log_dom > 0) 
     {
        eina_log_domain_unregister(_e_illume_log_dom);
        _e_illume_log_dom = -1;
     }

   return 1;
}

EAPI int 
e_modapi_save(E_Module *m) 
{
   return e_mod_config_save();
}
