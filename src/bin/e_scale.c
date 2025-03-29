#include "e.h"

EAPI double e_scale = 1.0;

EINTERN int
e_scale_init(void)
{
   e_scale_update();
   return 1;
}

EINTERN int
e_scale_shutdown(void)
{
   return 1;
}

EAPI double
e_scale_dpi_get(void)
{
   int x_core_dpi = ecore_x_dpi_get();
   return x_core_dpi;
}

EAPI void
e_scale_update(void)
{
   int dpi;
   char buf[128];

   if (e_config->scale.use_dpi)
     {
        dpi = ecore_x_dpi_get();
        e_scale = (double)dpi / (double)e_config->scale.base_dpi;
        if (e_scale > e_config->scale.max) e_scale = e_config->scale.max;
        else if (e_scale < e_config->scale.min) e_scale = e_config->scale.min;
     }
   else if (e_config->scale.use_custom)
     {
        e_scale = e_config->scale.factor;
        if (e_scale > e_config->scale.max) e_scale = e_config->scale.max;
        else if (e_scale < e_config->scale.min) e_scale = e_config->scale.min;
     }

   edje_scale_set(e_scale);
   e_util_env_set("E_SCALE", buf);
   e_hints_scale_update();
   e_xsettings_config_update();
   if (e_config->scale.set_xapp_dpi)
     {
        snprintf(buf, sizeof(buf), "%i",
                 (int)((double)e_config->scale.xapp_base_dpi * e_scale));
        ecore_x_resource_db_string_set("Xft.dpi", buf);
        ecore_x_resource_db_string_set("Xft.hinting", "1");
        ecore_x_resource_db_string_set("Xft.antialias", "1");
        ecore_x_resource_db_string_set("Xft.autohint", "0");
        ecore_x_resource_db_string_set("Xft.hintstyle", "hintfull");
        ecore_x_resource_db_string_set("Xft.rgba", "none");
        ecore_x_resource_db_flush();
     }
}
