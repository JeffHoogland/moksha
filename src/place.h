#ifndef E_PLACE_H
#define E_PLACE_H

#include "e.h"
#include "border.h"
#include "desktops.h"

typedef enum e_placement_mode
{
  E_PLACE_MANUAL,
  E_PLACE_SMART,
  E_PLACE_MIDDLE,
  E_PLACE_CASCADE,
  E_PLACE_RANDOM
}
E_Placement_Mode;

int  e_place_border(E_Border *b, E_Desktop *desk, int *x, int *y, E_Placement_Mode mode);
void e_place_init(void);
    
#endif

