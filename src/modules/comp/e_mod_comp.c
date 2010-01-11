#include "e.h"
#include "e_mod_main.h"
#include "e_mod_comp.h"
#include "config.h"

// TODO (no specific order):
//   1. abstract evas object and compwin so we can duplicate the object N times
//      in N canvases - for winlist, everything, pager etc. too
//   2. implement "unmapped composite cache" -> N pixels worth of unmapped
//      windows to be fully composited. only the most active/recent.
//   3. for unmapped windows - when window goes out of unmapped comp cache
//      make a miniature copy (1/4 width+height?) and set property on window
//      with pixmap id
//   4. add shadow to rect non argb windows
//   5. abstract composite canvas to add extras in and "expose" it
//   6. other engine fast-paths (gl specifically)!

typedef struct _Update      Update;
typedef struct _Update_Rect Update_Rect;

struct _Update
{
   int w, h;
   int tw, th;
   int tsw, tsh;
   unsigned char *tiles;
};

struct _Update_Rect
{
   EINA_INLIST;
   int x, y, w, h;
};


typedef struct _Comp     Comp;
typedef struct _Comp_Win Comp_Win;

struct _Comp
{
   Ecore_X_Window  win;
   Ecore_Evas     *ee;
   Ecore_X_Window  ee_win;
   Evas           *evas;
   E_Manager      *man;
   Eina_Inlist    *wins;
   Eina_List      *updates;
   Ecore_Animator *render_animator;
};

struct _Comp_Win
{
   EINA_INLIST;
   
   Comp           *c; // parent compositor
   Ecore_X_Window  win; // raw window - for menus etc.
   E_Border       *bd; // if its a border - later
   E_Popup        *pop; // if its a popup - later
   int             x, y, w, h; // geometry
   int             pw, ph; // pixmap w/h
   int             border; // border width
   Ecore_X_Pixmap  pixmap; // the compositing pixmap
   Ecore_X_Damage  damage; // damage region
   Ecore_X_Visual  vis;
   int             depth;
   Evas_Object    *obj; // shadow object
   Ecore_X_Image  *xim; // x image - software fallback
   Update         *up; // update handler
   Eina_Bool       visible : 1; // is visible
   Eina_Bool       input_only : 1; // is input_only
   Eina_Bool       argb : 1; // is argb
   Eina_Bool       shaped : 1; // is shaped
   Eina_Bool       update : 1; // has updates to fetch
   Eina_Bool       redirected : 1; // has updates to fetch
   Eina_Bool       shape_changed : 1; // shape changed
};

static Eina_List *handlers = NULL;
static Eina_List *compositors = NULL;
static Eina_Hash *windows = NULL;
static Eina_Hash *damages = NULL;

//////////////////////////////////////////////////////////////////////////

#if 0
#define DBG(f, x...) printf(f, ##x)
#else
#define DBG(f, x...)
#endif

static Update *
_e_mod_comp_update_new(void)
{
   Update *up;
   
   up = calloc(1, sizeof(Update));
   up->tsw = 32;
   up->tsh = 32;
   return up;
}

static void
_e_mod_comp_update_free(Update *up)
{
   if (up->tiles) free(up->tiles);
   free(up);
}

static void
_e_mod_comp_update_resize(Update *up, int w, int h)
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

static void
_e_mod_comp_tiles_alloc(Update *up)
{
   if (up->tiles) return;
   up->tiles = calloc(up->tw * up->th, sizeof(unsigned char));
}

static void
_e_mod_comp_update_add(Update *up, int x, int y, int w, int h)
{
   int tx, ty, txx, tyy, xx, yy;
   unsigned char *t, *t2;
   
   if ((w <= 0) || (h <= 0)) return;
   if ((up->tw <= 0) || (up->th <= 0)) return;
   
   _e_mod_comp_tiles_alloc(up);
   
   E_RECTS_CLIP_TO_RECT(x, y, w, h, 0, 0, up->w, up->h);
   if ((w <= 0) || (h <= 0)) return;
   
   // fixme: adjust to allow for optimizations in grabbing of ximages
   if (1)
     {
        if (w > (up->w / 2))
          {
             x = 0;
             w = up->w;
          }
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

static Update_Rect *
_e_mod_comp_update_rects_get(Update *up)
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

static void
_e_mod_comp_update_clear(Update *up)
{
   if (up->tiles)
     {
        free(up->tiles);
        up->tiles = NULL;
     }
}

//////////////////////////////////////////////////////////////////////////


static void _e_mod_comp_render_queue(Comp *c);
static void _e_mod_comp_win_damage(Comp_Win *cw, int x, int y, int w, int h, Eina_Bool dmg);

static int
_e_mod_comp_cb_animator(void *data)
{
   Comp *c = data;
   Comp_Win *cw;
   Eina_List *new_updates = NULL; // for failed pixmap fetches - get them next frame
   
   c->render_animator = NULL;
   EINA_LIST_FREE(c->updates, cw)
     {
        Update_Rect *r;
        int i;
        
        cw->update = 0;
        if (!cw->pixmap)
          {
             cw->pixmap = ecore_x_composite_name_window_pixmap_get(cw->win);
             if (cw->pixmap)
               ecore_x_pixmap_geometry_get(cw->pixmap, NULL, NULL, &(cw->pw), &(cw->ph));
             else
               {
                  cw->pw = 0;
                  cw->ph = 0;
               }
             DBG("REND [0x%x] pixma = [0x%x], %ix%i\n", cw->win, cw->pixmap, cw->pw, cw->ph);
             if ((cw->pw <= 0) || (cw->ph <= 0))
               {
                  ecore_x_pixmap_free(cw->pixmap);
                  cw->pixmap = 0;
                  cw->pw = 0;
                  cw->ph = 0;
               }
          }
        if ((cw->pw > 0) && (cw->ph > 0))
          {
             _e_mod_comp_update_resize(cw->up, cw->pw, cw->ph);
             if (!cw->xim)
               {
                  if (cw->xim = ecore_x_image_new(cw->pw, cw->ph, cw->vis, cw->depth))
                    _e_mod_comp_update_add(cw->up, 0, 0, cw->pw, cw->ph);
               }
             r = _e_mod_comp_update_rects_get(cw->up);
             if (r) 
               {
                  if (cw->xim)
                    {
                       _e_mod_comp_update_clear(cw->up);
                       for (i = 0; r[i].w > 0; i++)
                         {
                            unsigned int *pix;
                            int x, y, w, h;
                            
                            x = r[i].x;
                            y = r[i].y;
                            w = r[i].w;
                            h = r[i].h;
                            ecore_x_image_get(cw->xim, cw->pixmap, x, y, x, y, w, h);
                            DBG("UPDATE [0x%x] %i %i %ix%i\n", cw->win, x, y, w, h);
                            pix = ecore_x_image_data_get(cw->xim, NULL, NULL, NULL);
                            evas_object_image_size_set(cw->obj, cw->pw, cw->ph);
                            evas_object_image_data_set(cw->obj, pix);
                            evas_object_image_data_update_add(cw->obj, x, y, w, h);
                            if (cw->shaped) cw->shape_changed = 1;
                         }
                       if ((cw->shape_changed) && (!cw->argb))
//                       if (0)
                         {
                            Ecore_X_Rectangle *rects;
                            int num, i;
                            
                            DBG("SHAPE [0x%x] change\n", cw->win);
                            rects = ecore_x_window_shape_rectangles_get
                              (cw->win, &num);
                            if ((rects) && (num == 1))
                              {
                                 DBG("SHAPE [0x%x] rect 1\n", cw->win);
                                 if ((rects[0].x == 0) &&
                                     (rects[0].y == 0) &&
                                     (rects[0].width == cw->w) &&
                                     (rects[0].height == cw->h))
                                   {
                                      DBG("SHAPE [0x%x] rect solid\n", cw->win);
                                      free(rects);
                                      rects = NULL;
                                   }
                              }
                            if (rects)
                              {
                                 unsigned int *pix, *p;
                                 unsigned char *spix, *sp;
                                 int w, h, px, py;
                                 
                                 cw->shaped = 1;
                                 evas_object_image_size_get(cw->obj, &w, &h);
                                 if ((w > 0) && (h > 0))
                                   {
                                      pix = evas_object_image_data_get(cw->obj, 1);
                                      spix = calloc(w * h, sizeof(unsigned char));
                                      if (spix)
                                        {
                                           DBG("SHAPE [0x%x] rects %i\n", num);
                                           for (i = 0; i < num; i++)
                                             {
                                                int rx, ry, rw, rh;
                                                
                                                rx = rects[i].x;
                                                ry = rects[i].y;
                                                rw = rects[i].width;
                                                rh = rects[i].height;
                                                E_RECTS_CLIP_TO_RECT
                                                  (rx, ry, rw, rh, 0, 0, w, h);
                                                sp = spix + (w * ry) + rx;
                                                for (py = 0; py < rh; py++)
                                                  {
                                                     for (px = 0; px < rw; px++)
                                                       {
                                                          *sp = 1;
                                                          sp++;
                                                       }
                                                     sp += w - rw;
                                                  }
                                             }
                                           sp = spix;
                                           p = pix;
                                           for (py = 0; py < h; py++)
                                             {
                                                for (px = 0; px < w; px++)
                                                  {
                                                     if (*sp)
                                                       *p |= 0xff000000;
                                                     else
                                                       *p = 0x00000000;
                                                     sp++;
                                                     p++;
                                                  }
                                             }
                                           free(spix);
                                        }
                                      evas_object_image_data_set(cw->obj, pix);
                                      evas_object_image_alpha_set(cw->obj, 1);
                                      evas_object_image_data_update_add(cw->obj, 0, 0, w, h);
                                   }
                                 free(rects);
                              }
                            else
                              {
                                 cw->shaped = 0;
                                 evas_object_image_alpha_set(cw->obj, 0);
                                 // dont need to fix alpha chanel as blending 
                                 // should be totally off here regardless of
                                 // alpha channe; content
                              }
                            cw->shape_changed = 0;
                         }
                    }
                  free(r);
               }
             else
               {
                  cw->update = 1;
                  new_updates = eina_list_append(new_updates, cw);
                  _e_mod_comp_render_queue(c);
               }
          }
     }
   ecore_evas_manual_render(c->ee);
   c->updates = new_updates;
   return 0;
}

static void
_e_mod_comp_render_queue(Comp *c)
{
   if (c->render_animator) return;
   c->render_animator = ecore_animator_add(_e_mod_comp_cb_animator, c);
}

static void
_e_mod_comp_win_render_queue(Comp_Win *cw)
{
   _e_mod_comp_render_queue(cw->c);
}

static Comp *
_e_mod_comp_find(Ecore_X_Window root)
{
   Eina_List *l;
   Comp *c;
   
   // fixme: use hash if compositors list > 4
   EINA_LIST_FOREACH(compositors, l, c)
     {
        if (c->man->root == root) return c;
     }
   return NULL;
}

static Comp_Win *
_e_mod_comp_win_find(Ecore_X_Window win)
{
   Eina_List *l;
   Comp_Win *cw;

   cw = eina_hash_find(windows, e_util_winid_str_get(win));
   return cw;
}

static Comp_Win *
_e_mod_comp_win_damage_find(Ecore_X_Damage damage)
{
   Comp_Win *cw;
   
   cw = eina_hash_find(damages, e_util_winid_str_get(damage));
   return cw;
}

static Comp_Win *
_e_mod_comp_win_add(Comp *c, Ecore_X_Window win)
{
   Ecore_X_Window_Attributes att;
   Comp_Win *cw;
   
   cw = calloc(1, sizeof(Comp_Win));
   if (!cw) return NULL;
   cw->win = win;
   // FIXME: check if bd or pop - track
   c->wins = eina_inlist_append(c->wins, EINA_INLIST_GET(cw));
   cw->c = c;
   memset((&att), 0, sizeof(Ecore_X_Window_Attributes));
   ecore_x_window_attributes_get(cw->win, &att);
   cw->input_only = att.input_only;
   cw->vis = att.visual;
   cw->depth = att.depth;
   cw->argb = ecore_x_window_argb_get(cw->win);
   eina_hash_add(windows, e_util_winid_str_get(cw->win), cw);
   if (!cw->input_only)
     {
        Ecore_X_Rectangle *rects;
        int num;
        
        cw->damage = ecore_x_damage_new(cw->win, ECORE_X_DAMAGE_REPORT_DELTA_RECTANGLES);
        eina_hash_add(damages, e_util_winid_str_get(cw->damage), cw);
        cw->obj = evas_object_image_filled_add(c->evas);
        evas_object_image_colorspace_set(cw->obj, EVAS_COLORSPACE_ARGB8888);
        if (cw->argb)
          evas_object_image_alpha_set(cw->obj, 1);
        else
          evas_object_image_alpha_set(cw->obj, 0);
        ecore_x_window_shape_events_select(cw->win, 1);
        rects = ecore_x_window_shape_rectangles_get(cw->win, &num);
        if (rects)
          {
             if ((rects) && (num == 1))
               {
                  if ((rects[0].x == 0) &&
                      (rects[0].y == 0) &&
                      (rects[0].width == att.w) &&
                      (rects[0].height == att.h))
                    {
                       free(rects);
                       rects = NULL;
                    }
               }
             if (rects)
               {
                  cw->shaped = 1;
                  cw->shape_changed = 1;
                  free(rects);
               }
          }
     }
   else
     {
        cw->obj = evas_object_rectangle_add(c->evas);
        evas_object_color_set(cw->obj, 0, 0, 0, 64);
     }
   cw->up = _e_mod_comp_update_new();
   DBG("  [0x%x] add\n", cw->win);
   return cw;
}

static void
_e_mod_comp_win_del(Comp_Win *cw)
{
   _e_mod_comp_update_free(cw->up);
   DBG("  [0x%x] del\n", cw->win);
   if (cw->pixmap)
     {
        ecore_x_pixmap_free(cw->pixmap);
     }
   if (cw->update)
     {
        cw->update = 0;
        cw->c->updates = eina_list_remove(cw->c->updates, cw);
     }
   if (cw->xim)
     {
        evas_object_image_data_set(cw->obj, NULL);
        ecore_x_image_free(cw->xim);
     }
   if (cw->obj)
     {
        evas_object_del(cw->obj);
     }
   eina_hash_del(windows, e_util_winid_str_get(cw->win), cw);
   if (cw->damage)
     {
        Ecore_X_Region parts;
   
        eina_hash_del(damages, e_util_winid_str_get(cw->damage), cw);
        parts = ecore_x_region_new(NULL, 0);
        ecore_x_damage_subtract(cw->damage, 0, parts);
        ecore_x_region_free(parts);
        ecore_x_damage_free(cw->damage);
        cw->damage = 0;
     }
   cw->c->wins = eina_inlist_remove(cw->c->wins, EINA_INLIST_GET(cw));
   free(cw);
}

static void
_e_mod_comp_win_show(Comp_Win *cw)
{
   if (cw->visible) return;
   cw->visible = 1;
   DBG("  [0x%x] sho ++++++++++\n", cw->win);
   if (cw->input_only) return;
   if (!cw->redirected)
     {
        cw->redirected = 1;
        ecore_x_composite_redirect_window(cw->win, ECORE_X_COMPOSITE_UPDATE_MANUAL);
        _e_mod_comp_win_damage(cw, 0, 0, cw->w, cw->h, 0);
     }
   evas_object_show(cw->obj);
   _e_mod_comp_win_render_queue(cw);
}

static void
_e_mod_comp_win_hide(Comp_Win *cw)
{
   if (!cw->visible) return;
   cw->visible = 0;
   if (cw->input_only) return;
   DBG("  [0x%x] hid --\n", cw->win);
   if (cw->redirected)
     {
        ecore_x_composite_unredirect_window(cw->win, ECORE_X_COMPOSITE_UPDATE_MANUAL);
        cw->redirected = 0;
        cw->pw = 0;
        cw->ph = 0;
     }
   if (cw->pixmap)
     {
        ecore_x_pixmap_free(cw->pixmap);
        cw->pixmap = 0;
     }
   evas_object_hide(cw->obj);
   _e_mod_comp_win_render_queue(cw);
}

static void
_e_mod_comp_win_raise_above(Comp_Win *cw, Comp_Win *cw2)
{
   DBG("  [0x%x] abv [0x%x]\n", cw->win, cw2->win);
   cw->c->wins = eina_inlist_remove(cw->c->wins, EINA_INLIST_GET(cw));
   cw->c->wins = eina_inlist_append_relative(cw->c->wins, 
                                             EINA_INLIST_GET(cw), 
                                             EINA_INLIST_GET(cw2));
   evas_object_stack_above(cw->obj, cw2->obj);
   _e_mod_comp_win_render_queue(cw);
   if (cw->input_only) return;
}

static void
_e_mod_comp_win_raise(Comp_Win *cw)
{
   DBG("  [0x%x] rai\n", cw->win);
   cw->c->wins = eina_inlist_remove(cw->c->wins, EINA_INLIST_GET(cw));
   cw->c->wins = eina_inlist_append(cw->c->wins, EINA_INLIST_GET(cw));
   evas_object_raise(cw->obj);
   _e_mod_comp_win_render_queue(cw);
   if (cw->input_only) return;
}

static void
_e_mod_comp_win_lower(Comp_Win *cw)
{
   DBG("  [0x%x] low\n", cw->win);
   cw->c->wins = eina_inlist_remove(cw->c->wins, EINA_INLIST_GET(cw));
   cw->c->wins = eina_inlist_prepend(cw->c->wins, EINA_INLIST_GET(cw));
   evas_object_lower(cw->obj);
   _e_mod_comp_win_render_queue(cw);
   if (cw->input_only) return;
}

static void
_e_mod_comp_win_configure(Comp_Win *cw, int x, int y, int w, int h, int border)
{
   if (!((x == cw->x) && (y == cw->y)))
     {
        DBG("  [0x%x] mov %4i %4i\n", cw->win, x, y);
        cw->x = x;
        cw->y = y;
        evas_object_move(cw->obj, cw->x, cw->y);
     }
   if (!((w == cw->w) && (h == cw->h)))
     {
        DBG("  [0x%x] rsz %4ix%4i\n", cw->win, w, h);
//        ecore_x_composite_unredirect_window(cw->win, ECORE_X_COMPOSITE_UPDATE_MANUAL);
//        ecore_x_composite_redirect_window(cw->win, ECORE_X_COMPOSITE_UPDATE_MANUAL);
        if (cw->pixmap)
          {
             ecore_x_pixmap_free(cw->pixmap);
             cw->pixmap = 0;
             cw->pw = 0;
             cw->ph = 0;
          }
        cw->w = w;
        cw->h = h;
        evas_object_resize(cw->obj, 
                           cw->w + (cw->border * 2), 
                           cw->h + (cw->border * 2));
        if (cw->xim)
          {
             evas_object_image_data_set(cw->obj, NULL);
             ecore_x_image_free(cw->xim);
             cw->xim = NULL;
          }
     }
   if (cw->border != border)
     {
        cw->border = border;
        evas_object_resize(cw->obj, 
                           cw->w + (cw->border * 2), 
                           cw->h + (cw->border * 2));
     }
   if (cw->input_only) return;
   _e_mod_comp_win_render_queue(cw);
}

static void
_e_mod_comp_win_damage(Comp_Win *cw, int x, int y, int w, int h, Eina_Bool dmg)
{
   if (cw->input_only) return;
   DBG("  [0x%x] dmg %4i %4i %4ix%4i\n", cw->win, x, y, w, h);
   if ((dmg) && (cw->damage))
     {
        Ecore_X_Region parts;
   
        parts = ecore_x_region_new(NULL, 0);
        ecore_x_damage_subtract(cw->damage, 0, parts);
        ecore_x_region_free(parts);
     }
   /*
   if (cw->xim)
     {
        unsigned int *pix;
        
        ecore_x_image_get(cw->xim, cw->pixmap, x, y, x, y, w, h);
        pix = ecore_x_image_data_get(cw->xim, NULL, NULL, NULL);
        evas_object_image_data_set(cw->obj, pix);
        evas_object_image_size_set(cw->obj, cw->w, cw->h);
        evas_object_image_data_update_add(cw->obj, x, y, w, h);
     }
    */
   if (!cw->update)
     {
        cw->update = 1;
        cw->c->updates = eina_list_append(cw->c->updates, cw);
     }
   _e_mod_comp_update_add(cw->up, x, y, w, h);
   _e_mod_comp_win_render_queue(cw);
}

static void
_e_mod_comp_win_reshape(Comp_Win *cw)
{
   if (cw->shape_changed) return;
   cw->shape_changed = 1;
   if (!cw->update)
     {
        cw->update = 1;
        cw->c->updates = eina_list_append(cw->c->updates, cw);
     }
   _e_mod_comp_update_add(cw->up, 0, 0, cw->w, cw->h);
   _e_mod_comp_win_render_queue(cw);
}

//////////////////////////////////////////////////////////////////////////

static int
_e_mod_comp_create(void *data, int type, void *event)
{ // do nothing. only worry about maps
   Ecore_X_Event_Window_Create *ev = event;
   Comp_Win *cw;
   Comp *c = _e_mod_comp_find(ev->parent);
   if (!c) return 1;
   if (_e_mod_comp_win_find(ev->win)) return 1;
   if (c->win == ev->win) return 1;
   if (c->ee_win == ev->win) return 1;
   cw = _e_mod_comp_win_add(c, ev->win);
   _e_mod_comp_win_configure(cw, ev->x, ev->y, ev->w, ev->h, ev->border);
   return 1;
}

static int
_e_mod_comp_destroy(void *data, int type, void *event)
{ // do nothing. only worry about unmaps
   Ecore_X_Event_Window_Destroy *ev = event;
   Comp_Win *cw = _e_mod_comp_win_find(ev->win);
   if (!cw) return 1;
//   if (ev->event_win != cw->c->man->root) return 1;
   _e_mod_comp_win_del(cw);
   return 1;
}

static int
_e_mod_comp_show(void *data, int type, void *event)
{
   Ecore_X_Event_Window_Show *ev = event;
   Comp_Win *cw = _e_mod_comp_win_find(ev->win);
   if (!cw) return 1;
//   if (ev->event_win != cw->c->man->root) return 1;
   if (cw->visible) return 1;
   _e_mod_comp_win_show(cw);
   return 1;
}

static int
_e_mod_comp_hide(void *data, int type, void *event)
{
   Ecore_X_Event_Window_Hide *ev = event;
   Comp_Win *cw = _e_mod_comp_win_find(ev->win);
   if (!cw) return 1;
//   if (ev->event_win != cw->c->man->root) return 1;
   if (!cw->visible) return 1;
   _e_mod_comp_win_hide(cw);
   return 1;
}

static int
_e_mod_comp_reparent(void *data, int type, void *event)
{
   Ecore_X_Event_Window_Reparent *ev = event;
   Comp_Win *cw = _e_mod_comp_win_find(ev->win);
   if (!cw) return 1;
   DBG("== repar [0x%x] to [0x%x]\n", ev->win, ev->parent);
   if (ev->parent != cw->c->man->root)
     _e_mod_comp_win_del(cw);
   return 1;
}

static int
_e_mod_comp_configure(void *data, int type, void *event)
{
   Ecore_X_Event_Window_Configure *ev = event;
   Comp_Win *cw = _e_mod_comp_win_find(ev->win);
   if (!cw) return 1;
//   if (ev->event_win != cw->c->man->root) return 1;

   if (ev->abovewin == 0)
     {
        if (EINA_INLIST_GET(cw)->prev) _e_mod_comp_win_lower(cw);
     }
   else
     {
        Comp_Win *cw2 = _e_mod_comp_win_find(ev->abovewin);

        if (cw2)
          {
             Comp_Win *cw3 = (Comp_Win *)(EINA_INLIST_GET(cw)->prev);

             if (cw3 != cw2)
               _e_mod_comp_win_raise_above(cw, cw2);
          }
     }
   
   if (!((cw->x == ev->x) && (cw->y == ev->y) &&
         (cw->w == ev->h) && (cw->h == ev->h) &&
         (cw->border == ev->border)))
     {
        _e_mod_comp_win_configure(cw, ev->x, ev->y, ev->w, ev->h, ev->border);
     }
   return 1;
}

static int
_e_mod_comp_stack(void *data, int type, void *event)
{
   Ecore_X_Event_Window_Stack *ev = event;
   Comp_Win *cw = _e_mod_comp_win_find(ev->win);
   if (!cw) return 1;
//   if (ev->event_win != cw->c->man->root) return 1;
   if (ev->detail == ECORE_X_WINDOW_STACK_ABOVE) _e_mod_comp_win_raise(cw);
   else _e_mod_comp_win_lower(cw);
   return 1;
}

static int
_e_mod_comp_property(void *data, int type, void *event)
{ // later
   Ecore_X_Event_Window_Property *ev = event;
   return 1;
}

static int
_e_mod_comp_message(void *data, int type, void *event)
{ // later
   Ecore_X_Event_Client_Message *ev = event;
   return 1;
}

static int
_e_mod_comp_shape(void *data, int type, void *event)
{ // later
   Ecore_X_Event_Window_Shape *ev = event;
   Comp_Win *cw = _e_mod_comp_win_find(ev->win);
   if (!cw) return 1;
//   if (ev->event_win != cw->c->man->root) return 1;
   _e_mod_comp_win_reshape(cw);
   return 1;
}

//////////////////////////////////////////////////////////////////////////

static int
_e_mod_comp_damage(void *data, int type, void *event)
{
   Ecore_X_Event_Damage *ev = event;
   Comp_Win *cw = _e_mod_comp_win_damage_find(ev->damage);
   if (!cw) return 1;
   _e_mod_comp_win_damage(cw, 
                          ev->area.x, ev->area.y, 
                          ev->area.width, ev->area.height, 1); 
   return 1;
}

static Comp *
_e_mod_comp_add(E_Manager *man)
{
   Comp *c;
   Ecore_X_Window *wins;
   int i, num;
   
   c = calloc(1, sizeof(Comp));
   if (!c) return NULL;
   c->man = man;
   c->win = ecore_x_composite_render_window_enable(man->root);
   if (c->man->num == 0) e_alert_composite_win = c->win;
   c->ee = ecore_evas_software_x11_new(NULL, c->win, 0, 0, man->w, man->h);
   ecore_evas_manual_render_set(c->ee, 1);
   c->evas = ecore_evas_get(c->ee);
   ecore_evas_show(c->ee);
   
   c->ee_win = ecore_evas_software_x11_window_get(c->ee);
   ecore_x_screen_is_composited_set(c->man->num, c->ee_win);
//   ecore_x_composite_unredirect_subwindows
//     (c->man->root, ECORE_X_COMPOSITE_UPDATE_MANUAL);

   wins = ecore_x_window_children_get(c->man->root, &num);
   if (wins)
     {
        for (i = 0; i < num; i++)
          {
             Comp_Win *cw;
             int x, y, w, h, border;
             
             cw = _e_mod_comp_win_add(c, wins[i]);
             if (!cw) continue;
             ecore_x_window_geometry_get(cw->win, &x, &y, &w, &h);
             border = ecore_x_window_border_width_get(cw->win);
             if (wins[i] == c->win) continue;
             _e_mod_comp_win_configure(cw, x, y, w, h, border);
             if (ecore_x_window_visible_get(wins[i]))
               {
                  _e_mod_comp_win_show(cw);
               }
          }
        free(wins);
     }
   
   return c;
}

//////////////////////////////////////////////////////////////////////////

Eina_Bool
e_mod_comp_init(void)
{
   Eina_List *l;
   E_Manager *man;

   windows = eina_hash_string_superfast_new(NULL);
   damages = eina_hash_string_superfast_new(NULL);
   
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_CREATE,    _e_mod_comp_create,    NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_DESTROY,   _e_mod_comp_destroy,   NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_SHOW,      _e_mod_comp_show,      NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_HIDE,      _e_mod_comp_hide,      NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_REPARENT,  _e_mod_comp_reparent,  NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_CONFIGURE, _e_mod_comp_configure, NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_STACK,     _e_mod_comp_stack,     NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_PROPERTY,  _e_mod_comp_property,  NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE,   _e_mod_comp_message,   NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_WINDOW_SHAPE,     _e_mod_comp_shape,     NULL));
   handlers = eina_list_append(handlers, ecore_event_handler_add(ECORE_X_EVENT_DAMAGE_NOTIFY,    _e_mod_comp_damage,    NULL));

   EINA_LIST_FOREACH(e_manager_list(), l, man)
     {
        compositors = eina_list_append(compositors, _e_mod_comp_add(man));
     }
   
   return 1;
}

void
e_mod_comp_shutdown(void)
{
   Comp *c;
   
   EINA_LIST_FREE(compositors, c)
     {
        Comp_Win *cw;

        ecore_x_screen_is_composited_set(c->man->num, 0);
        while (c->wins)
          {
             cw = (Comp_Win *)(c->wins);
             _e_mod_comp_win_hide(cw);
             _e_mod_comp_win_del(cw);
          }
        ecore_evas_free(c->ee);
//             ecore_x_composite_unredirect_subwindows
//               (c->man->root, ECORE_X_COMPOSITE_UPDATE_MANUAL);
        ecore_x_composite_render_window_disable(c->win);
        if (c->man->num == 0) e_alert_composite_win = 0;
        if (c->render_animator) ecore_animator_del(c->render_animator);
        free(c);
     }
   
   E_FREE_LIST(handlers, ecore_event_handler_del);
   
   eina_hash_free(damages);
   eina_hash_free(windows);
}
