#ifndef E_PLACE_H
#define E_PLACE_H

#include "e.h"
#include "border.h"
#include "desktops.h"

#define E_PLACE_MANUAL  0
#define E_PLACE_SMART   1
#define E_PLACE_MIDDLE  2
#define E_PLACE_CASCADE 3
#define E_PLACE_RANDOM  4

int  e_place_border(E_Border *b, E_Desktop *desk, int *x, int *y, int method);
void e_place_init(void);
    
#endif

