#include "e.h"

void
e_color_update_rgb(E_Color *ec)
{
  if (!ec) return;
  evas_color_rgb_to_hsv(ec->r, ec->g, ec->b, &(ec->h), &(ec->s), &(ec->v));
}

void
e_color_update_hsv(E_Color *ec)
{

  if (!ec) return;
  if (ec->v == 0)
    ec->r = ec->g = ec->b = 0;
  else
    evas_color_hsv_to_rgb(ec->h, ec->s, ec->v, &(ec->r), &(ec->g), &(ec->b));
}
