#ifndef E_GUIDES_H
#define E_GUIDES_H

typedef enum e_guides_mode
{
   E_GUIDES_OPAQUE,		/* configure window border & client */
   E_GUIDES_BORDER,		/* unmap client and configure window border */
   E_GUIDES_BOX,		/* box outline */
   E_GUIDES_TECHNICAL		/* lots of lines & info */
}
E_Guides_Mode;

typedef enum e_guides_location
{
   E_GUIDES_DISPLAY_LOCATION_WINDOW_MIDDLE,
   E_GUIDES_DISPLAY_LOCATION_SCREEN_MIDDLE
}
E_Guides_Location;

/**
 * e_guides_init - Guides initialization.
 *
 * This function initializes guides handling. Guides are
 * little help windows that pop up when you move or resize
 * a window.
 */
void                e_guides_init(void);

void                e_guides_show(void);
void                e_guides_hide(void);
void                e_guides_move(int x, int y);
void                e_guides_resize(int w, int h);
void                e_guides_display_text(char *text);
void                e_guides_display_icon(char *icon);
void                e_guides_set_display_location(E_Guides_Location loc);
void                e_guides_set_display_alignment(double x, double y);
void                e_guides_set_mode(E_Guides_Mode mode);

#endif
