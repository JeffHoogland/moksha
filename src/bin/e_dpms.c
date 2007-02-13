/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
 
EAPI int
e_dpms_init(void)
{
   int standby=0, suspend=0, off=0;
    
   ecore_x_dpms_enabled_set(e_config->dpms_enable);

   if (e_config->dpms_standby_enable)
     standby = e_config->dpms_standby_timeout;
   
   if (e_config->dpms_suspend_enable)
     suspend = e_config->dpms_suspend_timeout;
   
   if (e_config->dpms_off_enable)
     off = e_config->dpms_off_timeout;

   ecore_x_dpms_timeouts_set(standby, suspend, off);
   
   return 1;
}
