/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
 
EAPI int
e_mouse_init(void)
{
   unsigned char map[256];
   int n;

   if (!ecore_x_pointer_control_set(e_config->mouse_accel_numerator,
				    e_config->mouse_accel_denominator,
				    e_config->mouse_accel_threshold))
     return 0;

   if (!(n = ecore_x_pointer_mapping_get(map, 256))) return 0;

   if (((e_config->mouse_hand == E_MOUSE_HAND_LEFT) && (map[2] != 1)) ||
       ((e_config->mouse_hand == E_MOUSE_HAND_RIGHT) && (map[0] != 1)))
     {
	const unsigned char tmp = map[0];
	map[0] = map[2]; map[2] = tmp;
	if (ecore_x_pointer_mapping_set(map, n)) return 0;
     }

   return 1;
}
