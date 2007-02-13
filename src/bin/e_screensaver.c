/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
 
EAPI int
e_screensaver_init(void)
{
   int timeout=0, interval=0, blanking=0, expose=0;
    
   if (e_config->screensaver_enable)
     timeout = e_config->screensaver_timeout;
   
   interval = e_config->screensaver_interval;
   blanking = e_config->screensaver_blanking;
   expose = e_config->screensaver_expose;
  
   ecore_x_screensaver_set(timeout, interval, blanking, expose);
   return 1;
}
