#include "E_Illume.h"
#include "e_mod_main.h"
#include "e_kbd.h"
#include "e_quickpanel.h"
#include "e_mod_layout.h"
#include "e_mod_config.h"

/* local variables */
static E_Kbd *kbd = NULL;
static E_Quickpanel *qp = NULL;
int _e_illume_log_dom = -1;

EAPI E_Module_Api e_modapi = { E_MODULE_API_VERSION, "Illume2" };

EAPI void *
e_modapi_init(E_Module *m) 
{
   /* set eina logging for error reporting/debugging */
   if (_e_illume_log_dom < 0) 
     {
        _e_illume_log_dom = 
          eina_log_domain_register("illume2", E_ILLUME_DEFAULT_LOG_COLOR);
        if (_e_illume_log_dom < 0) 
          {
             EINA_LOG_CRIT("Could not register log domain for illume2");
             return NULL;
          }
     }

   /* set module priority very high so it loads before other illume modules */
   e_module_priority_set(m, 100);

   /* init the config subsystem */
   if (!e_mod_config_init(m)) return NULL;

   /* init the keyboard subsystem */
   e_kbd_init();

   /* init the quickpanel subsystem */
   e_quickpanel_init();

   /* init the layout subsystem */
   if (!e_mod_layout_init()) 
     {
        /* shutdown the quickpanel subsystem */
        e_quickpanel_shutdown();

        /* shutdown kbd subsystem */
        e_kbd_shutdown();

        /* shutdown config subsystem */
        e_mod_config_shutdown();

        /* cleanup eina log domain */
        if (_e_illume_log_dom >= 0) 
          {
             eina_log_domain_unregister(_e_illume_log_dom);
             _e_illume_log_dom = -1;
          }

        /* report error to user */
        e_error_message_show("There was an error loading Policy\n");
        return NULL;
     }

   /* create a new keyboard */
   kbd = e_kbd_new();

   /* create a new quickpanel */
   qp = e_quickpanel_new(e_util_zone_current_get(e_manager_current_get()));

   return m;
}

EAPI int 
e_modapi_shutdown(E_Module *m) 
{
   /* cleanup the quickpanel */
   e_object_del(E_OBJECT(qp));
   qp = NULL;

   /* cleanup the keyboard */
   e_object_del(E_OBJECT(kbd));
   kbd = NULL;

   /* shutdown the layout subsystem */
   e_mod_layout_shutdown();

   /* shutdown the quickpanel subsystem */
   e_quickpanel_shutdown();

   /* shutdown the keyboard subsystem */
   e_kbd_shutdown();

   /* shutdown the config subsystem */
   e_mod_config_shutdown();

   /* cleanup eina log domain */
   if (_e_illume_log_dom >= 0) 
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
