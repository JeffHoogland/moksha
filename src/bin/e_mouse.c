#include "e.h"

EAPI int
e_mouse_update(void)
{
   unsigned char map[256] = { 0 };
   int n;

   if (!ecore_x_pointer_control_set(e_config->mouse_accel_numerator,
                                    e_config->mouse_accel_denominator,
                                    e_config->mouse_accel_threshold))
     return 0;

   if (!ecore_x_pointer_mapping_get(map, 256)) return 0;

   for (n = 0; n < 256; n++)
     {
        if (!map[n]) break;
     }
   if (n < 3)
     {
        map[0] = 1;
        map[1] = 2;
        map[2] = 3;
        n = 3;
     }
   if (e_config->mouse_hand == E_MOUSE_HAND_RIGHT)
     {
        map[0] = 1;
        map[2] = 3;
     }
   else if (e_config->mouse_hand == E_MOUSE_HAND_LEFT)
     {
        map[0] = 3;
        map[2] = 1;
     }

   if (!ecore_x_pointer_mapping_set(map, n)) return 0;

   return 1;
}
