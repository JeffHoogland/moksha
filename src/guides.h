#ifndef E_GUIDES_H
#define E_GUIDES_H

#include "e.h"

#define E_GUIDES_OPAQUE    0 /* configure window border & client */
#define E_GUIDES_BORDER    1 /* unmap client and configure window border */
#define E_GUIDES_BOX       2 /* box outline */
#define E_GUIDES_TECHNICAL 3 /* lots of lines & info */

#define E_GUIDES_DISPLAY_LOCATION_WINDOW_MIDDLE 0
#define E_GUIDES_DISPLAY_LOCATION_SCREEN_MIDDLE 1

void e_guides_show(void);
void e_guides_hide(void);
void e_guides_move(int x, int y);
void e_guides_resize(int w, int h);
void e_guides_display_text(char *text);
void e_guides_display_icon(char *icon);
void e_guides_set_display_location(int loc);
void e_guides_set_display_alignment(double x, double y);
void e_guides_set_mode(int mode);
void e_guides_init(void);

#endif
