/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

EAPI double e_scale = 1.0;

EAPI int
e_scale_init(void)
{
   e_scale_update();
   return 1;
}

EAPI int
e_scale_shutdown(void)
{
   return 1;
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
   snprintf(buf, sizeof(buf), "%1.3f", e_scale);
   e_util_env_set("E_SCALE", buf);
   e_hints_scale_update();
}
