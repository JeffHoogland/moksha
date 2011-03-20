#include "e.h"
#include "e_mod_main.h"
#include "e_mod_comp_update.h"

//////////////////////////////////////////////////////////////////////////

static void
_e_mod_comp_tiles_alloc(E_Update *up)
{
   if (up->tiles) return;
   up->tiles = calloc(up->tw * up->th, sizeof(unsigned char));
}

//////////////////////////////////////////////////////////////////////////

E_Update *
e_mod_comp_update_new(void)
{
   E_Update *up;

   up = calloc(1, sizeof(E_Update));
   up->tsw = 32;
   up->tsh = 32;
   up->pol = E_UPDATE_POLICY_RAW;
   return up;
}

void
e_mod_comp_update_free(E_Update *up)
{
   if (up->tiles) free(up->tiles);
   free(up);
}

void
e_mod_comp_update_policy_set(E_Update       *up,
                             E_Update_Policy pol)
{
   up->pol = pol;
}

void
e_mod_comp_update_tile_size_set(E_Update *up,
                                int       tsw,
                                int       tsh)
{
   if ((up->tsw == tsw) && (up->tsh == tsh)) return;
   up->tsw = tsw;
   up->tsh = tsh;
   e_mod_comp_update_clear(up);
}

void
e_mod_comp_update_resize(E_Update *up,
                         int       w,
                         int       h)
{
   unsigned char *ptiles, *p, *pp;
   int ptw, pth, x, y;

   if ((!up) || ((up->w == w) && (up->h == h))) return;

   ptw = up->tw;
   pth = up->th;
   ptiles = up->tiles;

   up->w = w;
   up->h = h;
   up->tw = (up->w + up->tsw - 1) / up->tsw;
   up->th = (up->h + up->tsh - 1) / up->tsh;
   up->tiles = NULL;
   _e_mod_comp_tiles_alloc(up);
   if ((ptiles) && (up->tiles))
     {
        if (pth <= up->th)
          {
             for (y = 0; y < pth; y++)
               {
                  p = up->tiles + (y * up->tw);
                  pp = ptiles + (y * ptw);
                  if (ptw <= up->tw) for (x = 0; x < ptw; x++) *p++ = *pp++;
                  else for (x = 0; x < up->tw; x++) *p++ = *pp++;
               }
          }
        else
          {
             for (y = 0; y < up->th; y++)
               {
                  p = up->tiles + (y * up->tw);
                  pp = ptiles + (y * ptw);
                  if (ptw <= up->tw) for (x = 0; x < ptw; x++) *p++ = *pp++;
                  else for (x = 0; x < up->tw; x++) *p++ = *pp++;
               }
          }
     }
   free(ptiles);
}

void
e_mod_comp_update_add(E_Update *up,
                      int       x,
                      int       y,
                      int       w,
                      int       h)
{
   int tx, ty, txx, tyy, xx, yy;
   unsigned char *t, *t2;

   if ((w <= 0) || (h <= 0)) return;
   if ((up->tw <= 0) || (up->th <= 0)) return;

   _e_mod_comp_tiles_alloc(up);

   E_RECTS_CLIP_TO_RECT(x, y, w, h, 0, 0, up->w, up->h);
   if ((w <= 0) || (h <= 0)) return;

   switch (up->pol)
     {
      case E_UPDATE_POLICY_RAW:
        break;

      case E_UPDATE_POLICY_HALF_WIDTH_OR_MORE_ROUND_UP_TO_FULL_WIDTH:
        if (w > (up->w / 2))
          {
             x = 0;
             w = up->w;
          }
        break;

      default:
        break;
     }

   tx = x / up->tsw;
   ty = y / up->tsh;
   txx = (x + w - 1) / up->tsw;
   tyy = (y + h - 1) / up->tsh;
   t = up->tiles + (ty * up->tw) + tx;
   for (yy = ty; yy <= tyy; yy++)
     {
        t2 = t;
        for (xx = tx; xx <= txx; xx++)
          {
             *t2 = 1;
             t2++;
          }
        t += up->tw;
     }
}

E_Update_Rect *
e_mod_comp_update_rects_get(E_Update *up)
{
   E_Update_Rect *r;
   int ri = 0;
   int x, y;
   unsigned char *t, *t2, *t3;

   if (!up->tiles) return NULL;
   r = calloc((up->tw * up->th) + 1, sizeof(E_Update_Rect));
   if (!r) return NULL;
   t = up->tiles;
   for (y = 0; y < up->th; y++)
     {
        for (x = 0; x < up->tw; x++)
          {
             if (*t)
               {
                  int can_expand_x = 1, can_expand_y = 1;
                  int xx = 0, yy = 0;

                  t2 = t + 1;
                  while (can_expand_x)
                    {
                       xx++;
                       if ((x + xx) >= up->tw) can_expand_x = 0;
                       else if (!*t2)
                         can_expand_x = 0;
                       if (can_expand_x) *t2 = 0;
                       t2++;
                    }
                  t3 = t;
                  while (can_expand_y)
                    {
                       int i;

                       yy++;
                       t3 += up->tw;
                       if ((y + yy) >= up->th) can_expand_y = 0;
                       if (can_expand_y)
                         {
                            t2 = t3;
                            for (i = 0; i < xx; i++)
                              {
                                 if (!*t2)
                                   {
                                      can_expand_y = 0;
                                      break;
                                   }
                                 t2++;
                              }
                         }
                       if (can_expand_y)
                         {
                            t2 = t3;
                            for (i = 0; i < xx; i++)
                              {
                                 *t2 = 0;
                                 t2++;
                              }
                         }
                    }
                  *t = 0;
                  r[ri].x = x * up->tsw;
                  r[ri].y = y * up->tsh;
                  r[ri].w = xx * up->tsw;
                  r[ri].h = yy * up->tsh;
                  if ((r[ri].x + r[ri].w) > up->w) r[ri].w = up->w - r[ri].x;
                  if ((r[ri].y + r[ri].h) > up->h) r[ri].h = up->h - r[ri].y;
                  if ((r[ri].w <= 0) || (r[ri].h <= 0)) r[ri].w = 0;
                  else ri++;
                  x += xx - 1;
                  t += xx - 1;
               }
             t++;
          }
     }
   return r;
}

void
e_mod_comp_update_clear(E_Update *up)
{
   if (up->tiles)
     {
        free(up->tiles);
        up->tiles = NULL;
     }
}

