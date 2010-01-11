#include "e.h"
#include "e_mod_main.h"
#include "e_mod_comp_update.h"
#include "config.h"

//////////////////////////////////////////////////////////////////////////

struct _Update
{
   int w, h;
   int tw, th;
   int tsw, tsh;
   unsigned char *tiles;
   Update_Policy pol;
};

static void
_e_mod_comp_tiles_alloc(Update *up)
{
   if (up->tiles) return;
   up->tiles = calloc(up->tw * up->th, sizeof(unsigned char));
}

//////////////////////////////////////////////////////////////////////////

Update *
e_mod_comp_update_new(void)
{
   Update *up;
   
   up = calloc(1, sizeof(Update));
   up->tsw = 32;
   up->tsh = 32;
   up->pol = UPDATE_POLICY_RAW;
   return up;
}

void
e_mod_comp_update_free(Update *up)
{
   if (up->tiles) free(up->tiles);
   free(up);
}

void
e_mod_comp_update_policy_set(Update *up, Update_Policy pol)
{
   up->pol = pol;
}

void
e_mod_comp_update_tile_size_set(Update *up, int tsw, int tsh)
{
   if ((up->tsw == tsw) && (up->tsh == tsh)) return;
   up->tsw = tsw;
   up->tsh = tsh;
   e_mod_comp_update_clear(up);
}

void
e_mod_comp_update_resize(Update *up, int w, int h)
{
   if ((up->w == w) && (up->h == h)) return;
   up->w = w;
   up->h = h;
   up->tw = (up->w + up->tsw - 1) / up->tsw;
   up->th = (up->h + up->tsh - 1) / up->tsh;
   if (up->tiles)
     {
        free(up->tiles);
        up->tiles = NULL;
     }
}

void
e_mod_comp_update_add(Update *up, int x, int y, int w, int h)
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
     case UPDATE_POLICY_RAW:
        break;
     case UPDATE_POLICY_HALF_WIDTH_OR_MORE_ROUND_UP_TO_FULL_WIDTH:
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

Update_Rect *
e_mod_comp_update_rects_get(Update *up)
{
   Update_Rect *r;
   int ri = 0;
   int x, y;
   unsigned char *t, *t2, *t3;
   
   if (!up->tiles) return NULL;
   r = calloc((up->tw * up->th) + 1, sizeof(Update_Rect));
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
                       else if (!*t2) can_expand_x = 0;
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
e_mod_comp_update_clear(Update *up)
{
   if (up->tiles)
     {
        free(up->tiles);
        up->tiles = NULL;
     }
}

