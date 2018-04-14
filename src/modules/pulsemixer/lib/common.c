#include "common.h"

int _log_domain = -1;

Eina_Bool
epulse_common_init(const char *domain)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(domain, EINA_FALSE);
   if (!eina_init())
     {
        fprintf(stderr, "Could not init eina\n");
        return EINA_FALSE;
     }

   _log_domain = eina_log_domain_register(domain, NULL);
   if (_log_domain < 0)
     {
        EINA_LOG_CRIT("Could not create log domain '%s'", domain);
        goto err_log;
     }

   if (!ecore_init())
     {
        CRIT("Could not init ecore");
        goto err_ecore;
     }

   return EINA_TRUE;

 err_ecore:
   eina_log_domain_unregister(_log_domain);
   _log_domain = -1;
 err_log:
   eina_shutdown();
   return EINA_FALSE;
}

void
epulse_common_shutdown(void)
{
   eina_shutdown();

   eina_log_domain_unregister(_log_domain);
   _log_domain = -1;

   ecore_shutdown();
}

Evas_Object *epulse_layout_add(Evas_Object *parent, const char *group,
                               const char *style)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(group, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(style, NULL);

   Evas_Object *layout = elm_layout_add(parent);
   if (!elm_layout_theme_set(layout, "layout", group, style))
     {
        CRIT("No theme for 'elm/layout/%s/%s' at %s", group, style, EPULSE_THEME);
        evas_object_del(layout);
        return NULL;
     }

   return layout;
}
