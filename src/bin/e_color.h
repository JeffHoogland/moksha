/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#ifdef E_TYPEDEFS

typedef enum _E_Color_Component E_Color_Component;
typedef struct _E_Color E_Color;

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

#else
#ifndef E_COLOR_HEADER
#define E_COLOR_HEADER

/* used so that a single color struct can be shared by all elements of the color dialog */
struct _E_Color
{
   int r, g, b;
   float h, s, v;
   int a;
};

EAPI void e_color_update_rgb (E_Color *ec);
EAPI void e_color_update_hsv (E_Color *ec);
EAPI void e_color_copy       (const E_Color *from, E_Color *to);
#endif
#endif
