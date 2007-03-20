/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
 
EAPI int
e_mouse_init(void)
{
   if (!ecore_x_pointer_control_set(e_config->mouse_accel_numerator,
					e_config->mouse_accel_denominator,
					e_config->mouse_accel_threshold))
					return 0;
	   
   return 1;
}
