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

/**
 * e_place_init - window placement initialization
 *
 * This function registers the event handlers necessary
 * to be able to manage window placement strategies.
 */
void                e_place_init(void);

/**
 * e_place_border - calculates window coordinates with given strategy
 * @b: The border if the window for which to calculate coordinates
 * @desk: The desktop on which to perform the calculation
 * @x: Result pointer to an integer which gets the resulting x coordinate
 * @y: Result pointer to an integer which gets the resulting y coordinate
 * @mode: The placement mode. Any of the values in the E_Placement_Mode enum.
 *
 * This function calculates coordinates for a given window border
 * and returns them in the @x and @y pointers. It does not actually
 * place the window.
 */
int                 e_place_border(E_Border * b, E_Desktop * desk, int *x,
				   int *y, E_Placement_Mode mode);

#endif
