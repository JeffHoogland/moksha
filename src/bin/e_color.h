#ifndef E_COLOR_HEADER
#define E_COLOR_HEADER
typedef enum _E_Color_Component E_Color_Component;

enum _E_Color_Component
{
   E_COLOR_COMPONENT_R,
   E_COLOR_COMPONENT_G,
   E_COLOR_COMPONENT_B,
   E_COLOR_COMPONENT_H,
   E_COLOR_COMPONENT_S,
   E_COLOR_COMPONENT_V,
   E_COLOR_COMPONENT_MAX
};

// used so that a single color struct can be shared by all elements of the color dialog
typedef struct _E_Color E_Color;
struct _E_Color
{
   int r, g, b;
   float h, s, v;
   int a;
};

void e_color_update_rgb(E_Color *ec);
void e_color_update_hsv(E_Color *ec);
#endif
