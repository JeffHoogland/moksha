#include "e.h"
#include "e_mod_main.h"
#include "e_mod_comp.h"
#include "e_mod_comp_update.h"
#ifdef HAVE_WAYLAND_CLIENTS
#include "e_mod_comp_wl.h"
#endif

#define OVER_FLOW 1

//////////////////////////////////////////////////////////////////////////
//
// TODO (no specific order):
//   1. abstract evas object and compwin so we can duplicate the object N times
//      in N canvases - for winlist, everything, pager etc. too
//   2. implement "unmapped composite cache" -> N pixels worth of unmapped
//      windows to be fully composited. only the most active/recent.
//   3. for unmapped windows - when window goes out of unmapped comp cache
//      make a miniature copy (1/4 width+height?) and set property on window
//      with pixmap id
//   8. obey transparent property
//   9. shortcut lots of stuff to draw inside the compositor - shelf,
//      wallpaper, efm - hell even menus and anything else in e (this is what
//      e18 was mostly about)
//  10. fullscreen windows need to be able to bypass compositing *seems buggy*
//
//////////////////////////////////////////////////////////////////////////

struct _E_Comp
{
   Ecore_X_Window  win;
   Ecore_Evas     *ee;
   Evas           *evas;
   Evas_Object    *layout;
   Eina_List      *zones;
   E_Manager      *man;
   Eina_Inlist    *wins;
   Eina_List      *wins_list;
   Eina_List      *updates;
   Ecore_Animator *render_animator;
   Ecore_Job      *update_job;
   Ecore_Timer    *new_up_timer;
   Evas_Object    *fps_bg;
   Evas_Object    *fps_fg;
   Ecore_Job      *screen_job;
   Ecore_Timer    *nocomp_delay_timer;
   Ecore_Timer    *nocomp_override_timer;
   Ecore_X_Window  ee_win;
   int             animating;
   int             render_overflow;
   double          frametimes[122];
   int             frameskip;

   int             nocomp_override;
   
   E_Manager_Comp  comp;
   Ecore_X_Window  cm_selection;

   Eina_Bool       gl : 1;
   Eina_Bool       grabbed : 1;
   Eina_Bool       nocomp : 1;
   Eina_Bool       nocomp_want : 1;
   Eina_Bool       wins_invalid : 1;
   Eina_Bool       saver : 1;
};

struct _E_Comp_Zone
{
   E_Zone         *zone; // never deref - just use for handle cmp's
   Evas_Object    *base;
   Evas_Object    *over;
   int             container_num;
   int             zone_num;
   int             x, y, w, h;
   double          bl;
   Eina_Bool       bloff;
};

struct _E_Comp_Win
{
   EINA_INLIST;

   E_Comp              *c;  // parent compositor
   Ecore_X_Window       win;  // raw window - for menus etc.
   E_Border            *bd;  // if its a border - later
   E_Popup             *pop;  // if its a popup - later
   E_Menu              *menu;  // if it is a menu - later
   int                  x, y, w, h;  // geometry
   struct
   {
      int x, y, w, h; // hidden geometry (used when its unmapped and re-instated on map)
   } hidden;
   int                  pw, ph;  // pixmap w/h
   int                  border;  // border width
   Ecore_X_Pixmap       pixmap;  // the compositing pixmap
   Ecore_X_Damage       damage;  // damage region
   Ecore_X_Visual       vis;  // window visual
   Ecore_X_Colormap     cmap; // colormap of window
   int                  depth;  // window depth
   Evas_Object         *obj;  // composite object
   Evas_Object         *shobj;  // shadow object
   Eina_List           *obj_mirror;  // extra mirror objects
   Ecore_X_Image       *xim;  // x image - software fallback
   E_Update            *up;  // update handler
   E_Object_Delfn      *dfn;  // delete function handle for objects being tracked
   Ecore_X_Sync_Counter counter;  // sync counter for syncronised drawing
   Ecore_Timer         *update_timeout;  // max time between damage and "done" event
   Ecore_Timer         *ready_timeout;  // max time on show (new window draw) to wait for window contents to be ready if sync protocol not handled. this is fallback.
   int                  dmg_updates;  // num of damage event updates since a redirect
   Ecore_X_Rectangle   *rects;  // shape rects... if shaped :(
   int                  rects_num;  // num rects above

   Ecore_X_Pixmap       cache_pixmap;  // the cached pixmap (1/nth the dimensions)
   int                  cache_w, cache_h;  // cached pixmap size
   int                  update_count;  // how many updates have happened to this win
   double               last_visible_time;  // last time window was visible
   double               last_draw_time;  // last time window was damaged

   int                  pending_count;  // pending event count

   unsigned int         opacity;  // opacity set with _NET_WM_WINDOW_OPACITY

   char                *title, *name, *clas, *role;  // fetched for override-redirect windowa
   Ecore_X_Window_Type  primary_type;  // fetched for override-redirect windowa

   unsigned char        misses; // number of sync misses

   Eina_Bool            delete_pending : 1;  // delete pendig
   Eina_Bool            hidden_override : 1;  // hidden override
   Eina_Bool            animating : 1;  // it's busy animating - defer hides/dels
   Eina_Bool            force : 1;  // force del/hide even if animating
   Eina_Bool            defer_hide : 1;  // flag to get hide to work on deferred hide
   Eina_Bool            delete_me : 1;  // delete me!
   Eina_Bool            visible : 1;  // is visible
   Eina_Bool            input_only : 1;  // is input_only

   Eina_Bool            override : 1;  // is override-redirect
   Eina_Bool            argb : 1;  // is argb
   Eina_Bool            shaped : 1;  // is shaped
   Eina_Bool            update : 1;  // has updates to fetch
   Eina_Bool            redirected : 1;  // has updates to fetch
   Eina_Bool            shape_changed : 1;  // shape changed
   Eina_Bool            native : 1;  // native
   Eina_Bool            drawme : 1;  // drawme flag fo syncing rendering

   Eina_Bool            invalid : 1;  // invalid depth used - just use as marker
   Eina_Bool            nocomp : 1;  // nocomp applied
   Eina_Bool            nocomp_need_update : 1;  // nocomp in effect, but this window updated while in nocomp mode
   Eina_Bool            needpix : 1;  // need new pixmap
   Eina_Bool            needxim : 1;  // need new xim
   Eina_Bool            real_hid : 1;  // last hide was a real window unmap
   Eina_Bool            inhash : 1;  // is in the windows hash
   Eina_Bool            show_ready : 1;  // is this window ready for its first show
   
   Eina_Bool            show_anim : 1; // ran show animation
};

static Eina_List *handlers = NULL;
static Eina_List *compositors = NULL;
static Eina_Hash *windows = NULL;
static Eina_Hash *borders = NULL;
static Eina_Hash *damages = NULL;

//////////////////////////////////////////////////////////////////////////
#undef DBG
#if 0
#define DBG(f, x ...) printf(f, ##x)
#else
#define DBG(f, x ...)
#endif

static void _e_mod_comp_win_ready_timeout_setup(E_Comp_Win *cw);
static void _e_mod_comp_render_queue(E_Comp *c);
static void _e_mod_comp_win_damage(E_Comp_Win *cw,
                                   int x,
                                   int y,
                                   int w,
                                   int h,
                                   Eina_Bool dmg);
static void _e_mod_comp_win_render_queue(E_Comp_Win *cw);
static void _e_mod_comp_win_release(E_Comp_Win *cw);
static void _e_mod_comp_win_adopt(E_Comp_Win *cw);
static void _e_mod_comp_win_del(E_Comp_Win *cw);
static void _e_mod_comp_win_show(E_Comp_Win *cw);
static void _e_mod_comp_win_real_hide(E_Comp_Win *cw);
static void _e_mod_comp_win_hide(E_Comp_Win *cw);
static void _e_mod_comp_win_configure(E_Comp_Win *cw,
                                      int x,
                                      int y,
                                      int w,
                                      int h,
                                      int border);

static void
_e_mod_comp_child_show(E_Comp_Win *cw)
{
   evas_object_show(cw->shobj);
   if (cw->bd)
     {
        Eina_List *l;
        E_Border *tmp;

        EINA_LIST_FOREACH(cw->bd->client.e.state.video_child, l, tmp)
          {
             E_Comp_Win *tcw;

             tcw = eina_hash_find(borders, e_util_winid_str_get(tmp->client.win));
             if (!tcw) continue;

             evas_object_show(tcw->shobj);
          }
     }
}

static void
_e_mod_comp_child_hide(E_Comp_Win *cw)
{
   evas_object_hide(cw->shobj);
   if (cw->bd)
     {
        Eina_List *l;
        E_Border *tmp;

        EINA_LIST_FOREACH(cw->bd->client.e.state.video_child, l, tmp)
          {
             E_Comp_Win *tcw;

             tcw = eina_hash_find(borders, e_util_winid_str_get(tmp->client.win));
             if (!tcw) continue;

             evas_object_hide(tcw->shobj);
          }
     }
}

static void
_e_mod_comp_cb_pending_after(void *data             __UNUSED__,
                             E_Manager *man         __UNUSED__,
                             E_Manager_Comp_Source *src)
{
   E_Comp_Win *cw = (E_Comp_Win *)src;
   cw->pending_count--;
   if (!cw->delete_pending) return;
   if (cw->pending_count == 0)
     {
        free(cw);
     }
}

static E_Comp_Win *
_e_mod_comp_fullscreen_check(E_Comp *c)
{
   E_Comp_Win *cw;

   if (!c->wins) return NULL;
   EINA_INLIST_REVERSE_FOREACH(c->wins, cw)
     {
        if ((!cw->visible) || (cw->input_only) || (cw->invalid))
          continue;
        if ((cw->x == 0) && (cw->y == 0) &&
            ((cw->x + cw->w) >= c->man->w) &&
            ((cw->y + cw->h) >= c->man->h) &&
            (!cw->argb) && (!cw->shaped)
            )
          {
             return cw;
          }
        return NULL;
     }
   return NULL;
}

static inline Eina_Bool
_e_mod_comp_shaped_check(int w,
                         int h,
                         const Ecore_X_Rectangle *rects,
                         int num)
{
   if ((!rects) || (num < 1)) return EINA_FALSE;
   if (num > 1) return EINA_TRUE;
   if ((rects[0].x == 0) && (rects[0].y == 0) &&
       ((int)rects[0].width == w) && ((int)rects[0].height == h))
     return EINA_FALSE;
   return EINA_TRUE;
}

static inline Eina_Bool
_e_mod_comp_win_shaped_check(const E_Comp_Win *cw,
                             const Ecore_X_Rectangle *rects,
                             int num)
{
   return _e_mod_comp_shaped_check(cw->w, cw->h, rects, num);
}

static void
_e_mod_comp_win_shape_rectangles_apply(E_Comp_Win *cw,
                                       const Ecore_X_Rectangle *rects,
                                       int num)
{
   Eina_List *l;
   Evas_Object *o;
   int i;

   DBG("SHAPE [0x%x] change, rects=%p (%d)\n", cw->win, rects, num);
   if (!_e_mod_comp_win_shaped_check(cw, rects, num))
     {
        rects = NULL;
     }
   if (rects)
     {
        unsigned int *pix, *p;
        unsigned char *spix, *sp;
        int w, h, px, py;

        evas_object_image_size_get(cw->obj, &w, &h);
        if ((w > 0) && (h > 0))
          {
             if (cw->native)
               {
                  fprintf(stderr, "BUGGER: shape with native surface? cw=%p\n", cw);
                  return;
               }

             evas_object_image_native_surface_set(cw->obj, NULL);
             evas_object_image_alpha_set(cw->obj, 1);
             EINA_LIST_FOREACH(cw->obj_mirror, l, o)
               {
                  evas_object_image_native_surface_set(o, NULL);
                  evas_object_image_alpha_set(o, 1);
               }
             pix = evas_object_image_data_get(cw->obj, 1);
             if (pix)
               {
                  spix = calloc(w * h, sizeof(unsigned char));
                  if (spix)
                    {
                       DBG("SHAPE [0x%x] rects %i\n", cw->win, num);
                       for (i = 0; i < num; i++)
                         {
                            int rx, ry, rw, rh;

                            rx = rects[i].x; ry = rects[i].y;
                            rw = rects[i].width; rh = rects[i].height;
                            E_RECTS_CLIP_TO_RECT(rx, ry, rw, rh, 0, 0, w, h);
                            sp = spix + (w * ry) + rx;
                            for (py = 0; py < rh; py++)
                              {
                                 for (px = 0; px < rw; px++)
                                   {
                                      *sp = 0xff; sp++;
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
                                 unsigned int mask, imask;

                                 mask = ((unsigned int)(*sp)) << 24;
                                 imask = mask >> 8;
                                 imask |= imask >> 8;
                                 imask |= imask >> 8;
                                 *p = mask | (*p & imask);
//                                 if (*sp) *p = 0xff000000 | *p;
//                                 else *p = 0x00000000;
                                 sp++;
                                 p++;
                              }
                         }
                       free(spix);
                    }
                  evas_object_image_data_set(cw->obj, pix);
                  evas_object_image_data_update_add(cw->obj, 0, 0, w, h);
                  EINA_LIST_FOREACH(cw->obj_mirror, l, o)
                    {
                       evas_object_image_data_set(o, pix);
                       evas_object_image_data_update_add(o, 0, 0, w, h);
                    }
               }
          }
     }
   else
     {
        if (cw->shaped)
          {
             unsigned int *pix, *p;
             int w, h, px, py;

             evas_object_image_size_get(cw->obj, &w, &h);
             if ((w > 0) && (h > 0))
               {
                  if (cw->native)
                    {
                       fprintf(stderr, "BUGGER: shape with native surface? cw=%p\n", cw);
                       return;
                    }

                  evas_object_image_alpha_set(cw->obj, 0);
                  EINA_LIST_FOREACH(cw->obj_mirror, l, o)
                    {
                       evas_object_image_alpha_set(o, 1);
                    }
                  pix = evas_object_image_data_get(cw->obj, 1);
                  if (pix)
                    {
                       p = pix;
                       for (py = 0; py < h; py++)
                         {
                            for (px = 0; px < w; px++)
                              *p |= 0xff000000;
                         }
                    }
                  evas_object_image_data_set(cw->obj, pix);
                  evas_object_image_data_update_add(cw->obj, 0, 0, w, h);
                  EINA_LIST_FOREACH(cw->obj_mirror, l, o)
                    {
                       evas_object_image_data_set(o, pix);
                       evas_object_image_data_update_add(o, 0, 0, w, h);
                    }
               }
          }
        // dont need to fix alpha chanel as blending
        // should be totally off here regardless of
        // alpha channel content
     }
}

static Eina_Bool
_e_mod_comp_cb_win_show_ready_timeout(void *data)
{
   E_Comp_Win *cw = data;
   cw->show_ready = 1;
   if (cw->visible)
     {
        if (!cw->update)
          {
             if (cw->update_timeout)
               {
                  ecore_timer_del(cw->update_timeout);
                  cw->update_timeout = NULL;
               }
             cw->update = 1;
             cw->c->updates = eina_list_append(cw->c->updates, cw);
          }
        _e_mod_comp_win_render_queue(cw);
     }
   cw->ready_timeout = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static void
_e_mod_comp_win_ready_timeout_setup(E_Comp_Win *cw)
{
   if (cw->ready_timeout)
     {
        ecore_timer_del(cw->ready_timeout);
        cw->ready_timeout = NULL;
     }
   if (cw->show_ready) return;
   if (cw->counter) return;
// FIXME: make show_ready option
   if (0)
     {
        cw->show_ready = 1;
     }
   else
     {
        cw->ready_timeout = ecore_timer_add
            (_comp_mod->conf->first_draw_delay, _e_mod_comp_cb_win_show_ready_timeout, cw);
     }
}

static void
_e_mod_comp_win_layout_populate(E_Comp_Win *cw)
{
   e_layout_pack(cw->c->layout, cw->shobj);
}

static void
_e_mod_comp_win_restack(E_Comp_Win *cw)
{
   Eina_Inlist *prev, *next;
   Eina_List *l;
   E_Comp_Win *cwp = NULL, *cwn = NULL;
   
   next = EINA_INLIST_GET(cw)->next;
   if (next) cwn = EINA_INLIST_CONTAINER_GET(next, E_Comp_Win);
   prev = EINA_INLIST_GET(cw)->prev;
   if (prev) cwp = EINA_INLIST_CONTAINER_GET(prev, E_Comp_Win);
   
   if (cwp)
     e_layout_child_raise_above(cw->shobj, cwp->shobj);
   else if (cwn)
     e_layout_child_raise_above(cw->shobj, cwn->shobj);
   if (cw->bd)
     {
        E_Border *tmp;

        EINA_LIST_FOREACH(cw->bd->client.e.state.video_child, l, tmp)
          {
             E_Comp_Win *tcw;

             tcw = eina_hash_find(borders,
                                  e_util_winid_str_get(tmp->client.win));
             if (!tcw) continue;
             e_layout_child_lower_below(tcw->shobj, cw->shobj);
          }
     }
}

static void
_e_mod_comp_win_geometry_update(E_Comp_Win *cw)
{
   e_layout_child_move(cw->shobj, cw->x, cw->y);
   e_layout_child_resize(cw->shobj, cw->pw, cw->ph);
}

static void
_e_mod_comp_win_update(E_Comp_Win *cw)
{
   Eina_List *l;
   Evas_Object *o;
   E_Update_Rect *r;
   int i;
   int pshaped = cw->shaped;

   DBG("UPDATE [0x%x] pm = %x\n", cw->win, cw->pixmap);
   if (_comp_mod->conf->grab) ecore_x_grab();
   cw->update = 0;

   if (cw->argb)
     {
        if (cw->rects)
          {
             free(cw->rects);
             cw->rects = NULL;
             cw->rects_num = 0;
          }
     }
   else
     {
        if (cw->shape_changed)
          {
             if (cw->rects)
               {
                  free(cw->rects);
                  cw->rects = NULL;
                  cw->rects_num = 0;
               }
             ecore_x_pixmap_geometry_get(cw->win, NULL, NULL, &(cw->w), &(cw->h));
             cw->rects = ecore_x_window_shape_rectangles_get(cw->win, &(cw->rects_num));
             if (cw->rects)
               {
                  for (i = 0; i < cw->rects_num; i++)
                    {
                       E_RECTS_CLIP_TO_RECT(cw->rects[i].x,
                                            cw->rects[i].y,
                                            cw->rects[i].width,
                                            cw->rects[i].height,
                                            0, 0, (int)cw->w, (int)cw->h);
                    }
               }
             if (!_e_mod_comp_win_shaped_check(cw, cw->rects, cw->rects_num))
               {
                  E_FREE(cw->rects);
                  cw->rects_num = 0;
               }
             if ((cw->rects) && (!cw->shaped))
               {
                  cw->shaped = 1;
               }
             else if ((!cw->rects) && (cw->shaped))
               {
                  cw->shaped = 0;
               }
          }
     }

   if (((!cw->pixmap) || (cw->needpix)) &&
       (!cw->real_hid) && (!cw->nocomp) && (!cw->c->nocomp))
     {
        Ecore_X_Pixmap pm = 0;

/* #ifdef HAVE_WAYLAND_CLIENTS */
/*         if ((cw->bd) && (cw->bd->borderless)) */
/*           pm = e_mod_comp_wl_pixmap_get(cw->win); */
/* #endif */
        if (!pm) pm = ecore_x_composite_name_window_pixmap_get(cw->win);
        if (pm)
          {
             Ecore_X_Pixmap oldpm;

             cw->needpix = 0;
             if (cw->xim) cw->needxim = 1;
             oldpm = cw->pixmap;
             cw->pixmap = pm;
             if (cw->pixmap)
               {
                  ecore_x_pixmap_geometry_get(cw->pixmap, NULL, NULL, &(cw->pw), &(cw->ph));
                  _e_mod_comp_win_ready_timeout_setup(cw);
                  if ((cw->pw > 0) && (cw->ph > 0))
                    _e_mod_comp_win_geometry_update(cw);
               }
             else
               {
                  cw->pw = 0;
                  cw->ph = 0;
               }
             DBG("REND [0x%x] pixmap = [0x%x], %ix%i\n", cw->win, cw->pixmap, cw->pw, cw->ph);
             if ((cw->pw <= 0) || (cw->ph <= 0))
               {
                  if (cw->native)
                    {
                       DBG("  [0x%x] free native\n", cw->win);
                       evas_object_image_native_surface_set(cw->obj, NULL);
                       cw->native = 0;
                       EINA_LIST_FOREACH(cw->obj_mirror, l, o)
                         {
                            evas_object_image_native_surface_set(o, NULL);
                         }
                    }
                  if (cw->pixmap)
                    {
                       DBG("  [0x%x] free pixmap\n", cw->win);
                       ecore_x_pixmap_free(cw->pixmap);
                       cw->pixmap = 0;
//                       cw->show_ready = 0; // hmm maybe not needed?
                    }
                  cw->pw = 0;
                  cw->ph = 0;
               }
             ecore_x_e_comp_pixmap_set(cw->win, cw->pixmap);
             cw->native = 0;
             DBG("  [0x%x] up resize %ix%i\n", cw->win, cw->pw, cw->ph);
             e_mod_comp_update_resize(cw->up, cw->pw, cw->ph);
             e_mod_comp_update_add(cw->up, 0, 0, cw->pw, cw->ph);
             if (oldpm)
               {
                  DBG("  [0x%x] free pm %x\n", cw->win, oldpm);
                  // XXX the below is unreachable code! :)
/*
                  if (cw->native)
                    {
                       cw->native = 0;
                       if (!((cw->pw > 0) && (cw->ph > 0)))
                         {
                            evas_object_image_native_surface_set(cw->obj, NULL);
                            EINA_LIST_FOREACH(cw->obj_mirror, l, o)
                              {
                                 evas_object_image_native_surface_set(o, NULL);
                              }
                         }
                    }
 */
                  ecore_x_pixmap_free(oldpm);
               }
          }
     }
   if (!((cw->pw > 0) && (cw->ph > 0)))
     {
        if (_comp_mod->conf->grab) ecore_x_ungrab();
        return;
     }

//   evas_object_move(cw->shobj, cw->x, cw->y);
   // was cw->w / cw->h
//   evas_object_resize(cw->shobj, cw->pw, cw->ph);
   _e_mod_comp_win_geometry_update(cw);

   if ((cw->c->gl) && (_comp_mod->conf->texture_from_pixmap) &&
       (!cw->shaped) && (!cw->rects) && (cw->pixmap))
     {
/* #ifdef HAVE_WAYLAND_CLIENTS */
/*         DBG("DEBUG - pm now %x\n", e_mod_comp_wl_pixmap_get(cw->win)); */
/* #endif */
        /* DBG("DEBUG - pm now %x\n", ecore_x_composite_name_window_pixmap_get(cw->win)); */
        evas_object_image_size_set(cw->obj, cw->pw, cw->ph);
        EINA_LIST_FOREACH(cw->obj_mirror, l, o)
          {
             evas_object_image_size_set(o, cw->pw, cw->ph);
          }
        if (!cw->native)
          {
             Evas_Native_Surface ns;

             ns.version = EVAS_NATIVE_SURFACE_VERSION;
             ns.type = EVAS_NATIVE_SURFACE_X11;
             ns.data.x11.visual = cw->vis;
             ns.data.x11.pixmap = cw->pixmap;
             evas_object_image_native_surface_set(cw->obj, &ns);
             DBG("NATIVE [0x%x] %x %ix%i\n", cw->win, cw->pixmap, cw->pw, cw->ph);
             cw->native = 1;
             EINA_LIST_FOREACH(cw->obj_mirror, l, o)
               {
                  evas_object_image_native_surface_set(o, &ns);
               }
          }
        r = e_mod_comp_update_rects_get(cw->up);
        if (r)
          {
             e_mod_comp_update_clear(cw->up);
             for (i = 0; r[i].w > 0; i++)
               {
                  int x, y, w, h;

                  x = r[i].x; y = r[i].y;
                  w = r[i].w; h = r[i].h;
                  DBG("UPDATE [0x%x] pm [0x%x] %i %i %ix%i\n", cw->win, cw->pixmap, x, y, w, h);
                  evas_object_image_data_update_add(cw->obj, x, y, w, h);
                  EINA_LIST_FOREACH(cw->obj_mirror, l, o)
                    {
                       evas_object_image_data_update_add(o, x, y, w, h);
                    }
               }
             free(r);
          }
        else
          {
             DBG("UPDATE [0x%x] NO RECTS!!! %i %i - %i %i\n", cw->win, cw->up->w, cw->up->h, cw->up->tw, cw->up->th);
//             cw->update = 1;
          }
     }
   else if (cw->pixmap)
     {
        if (cw->native)
          {
             evas_object_image_native_surface_set(cw->obj, NULL);
             EINA_LIST_FOREACH(cw->obj_mirror, l, o)
               {
                  evas_object_image_native_surface_set(o, NULL);
               }
             cw->native = 0;
          }
        if (cw->needxim)
          {
             cw->needxim = 0;
             if (cw->xim)
               {
                  evas_object_image_size_set(cw->obj, 1, 1);
                  evas_object_image_data_set(cw->obj, NULL);
                  EINA_LIST_FOREACH(cw->obj_mirror, l, o)
                    {
                       evas_object_image_size_set(o, 1, 1);
                       evas_object_image_data_set(o, NULL);
                    }
                  ecore_x_image_free(cw->xim);
                  cw->xim = NULL;
               }
          }
        if (!cw->xim)
          {
             if ((cw->xim = ecore_x_image_new(cw->pw, cw->ph, cw->vis, cw->depth)))
               e_mod_comp_update_add(cw->up, 0, 0, cw->pw, cw->ph);
          }
        r = e_mod_comp_update_rects_get(cw->up);
        if (r)
          {
             if (cw->xim)
               {
                  if (ecore_x_image_is_argb32_get(cw->xim))
                    {
                       unsigned int *pix;
                       
                       pix = ecore_x_image_data_get(cw->xim, NULL, NULL, NULL);
                       evas_object_image_data_set(cw->obj, pix);
                       evas_object_image_size_set(cw->obj, cw->pw, cw->ph);
                       EINA_LIST_FOREACH(cw->obj_mirror, l, o)
                         {
                            evas_object_image_data_set(o, pix);
                            evas_object_image_size_set(o, cw->pw, cw->ph);
                         }
                       
                       e_mod_comp_update_clear(cw->up);
                       for (i = 0; r[i].w > 0; i++)
                         {
                            int x, y, w, h;
                            
                            x = r[i].x; y = r[i].y;
                            w = r[i].w; h = r[i].h;
                            if (!ecore_x_image_get(cw->xim, cw->pixmap, x, y, x, y, w, h))
                              {
                                 DBG("UPDATE [0x%x] %i %i %ix%i FAIL!!!!!!!!!!!!!!!!!\n", cw->win, x, y, w, h);
                                 e_mod_comp_update_add(cw->up, x, y, w, h);
                                 cw->update = 1;
                              }
                            else
                              {
                                 // why do we neeed these 2? this smells wrong
                                 pix = ecore_x_image_data_get(cw->xim, NULL, NULL, NULL);
                                 DBG("UPDATE [0x%x] %i %i %ix%i -- pix = %p\n", cw->win, x, y, w, h, pix);
                                 evas_object_image_data_set(cw->obj, pix);
                                 evas_object_image_data_update_add(cw->obj, x, y, w, h);
                                 EINA_LIST_FOREACH(cw->obj_mirror, l, o)
                                   {
                                      evas_object_image_data_set(o, pix);
                                      evas_object_image_data_update_add(o, x, y, w, h);
                                   }
                              }
                         }
                    }
                  else
                    {
                       unsigned int *pix;
                       int stride;
                       
                       evas_object_image_size_set(cw->obj, cw->pw, cw->ph);
                       pix = evas_object_image_data_get(cw->obj, EINA_TRUE);
                       stride = evas_object_image_stride_get(cw->obj);
                       EINA_LIST_FOREACH(cw->obj_mirror, l, o)
                         {
                            evas_object_image_data_set(o, pix);
                            evas_object_image_size_set(o, cw->pw, cw->ph);
                         }
                       
                       e_mod_comp_update_clear(cw->up);
                       for (i = 0; r[i].w > 0; i++)
                         {
                            int x, y, w, h;
                            
                            x = r[i].x; y = r[i].y;
                            w = r[i].w; h = r[i].h;
                            if (!ecore_x_image_get(cw->xim, cw->pixmap, x, y, x, y, w, h))
                              {
                                 DBG("UPDATE [0x%x] %i %i %ix%i FAIL!!!!!!!!!!!!!!!!!\n", cw->win, x, y, w, h);
                                 e_mod_comp_update_add(cw->up, x, y, w, h);
                                 cw->update = 1;
                              }
                            else
                              {
                                 unsigned int *srcpix;
                                 int srcbpp = 0, srcbpl = 0;
                                 // why do we neeed these 2? this smells wrong
                                 srcpix = ecore_x_image_data_get
                                   (cw->xim, &srcbpl, NULL, &srcbpp);
                                 ecore_x_image_to_argb_convert(srcpix, 
                                                               srcbpp,
                                                               srcbpl, 
                                                               cw->cmap,
                                                               cw->vis,
                                                               x, y, w, h,
                                                               pix,
                                                               stride,
                                                               x, y);
                                 DBG("UPDATE [0x%x] %i %i %ix%i -- pix = %p\n", cw->win, x, y, w, h, pix);
                                 evas_object_image_data_update_add(cw->obj, x, y, w, h);
                                 EINA_LIST_FOREACH(cw->obj_mirror, l, o)
                                   {
                                      evas_object_image_data_update_add(o, x, y, w, h);
                                   }
                              }
                         }
                       evas_object_image_data_set(cw->obj, pix);
                    }
               }
             free(r);
             if (cw->shaped)
               {
                  _e_mod_comp_win_shape_rectangles_apply(cw, cw->rects, cw->rects_num);
               }
             else
               {
                  if (cw->shape_changed)
                    _e_mod_comp_win_shape_rectangles_apply(cw, cw->rects, cw->rects_num);
               }
             cw->shape_changed = 0;
          }
        else
          {
             DBG("UPDATE [0x%x] NO RECTS!!! %i %i - %i %i\n", cw->win, cw->up->w, cw->up->h, cw->up->tw, cw->up->th);
// causes updates to be flagged when not needed - disabled
//             cw->update = 1;
          }
     }
// FIXME: below cw update check screws with show
   if (/*(!cw->update) &&*/ (cw->visible) && (cw->dmg_updates >= 1) &&
                            (cw->show_ready) && (!cw->bd || cw->bd->visible))
     {
        if (!evas_object_visible_get(cw->shobj))
          {
             if (!cw->hidden_override)
               {
                  _e_mod_comp_child_show(cw);
                  _e_mod_comp_win_render_queue(cw);
               }

             if (!cw->show_anim)
               {
                  edje_object_signal_emit(cw->shobj, "e,state,visible,on", "e");
                  if (!cw->animating)
                    {
                       cw->c->animating++;
                    }

                  cw->animating = 1;

                  cw->pending_count++;
                  e_manager_comp_event_src_visibility_send
                    (cw->c->man, (E_Manager_Comp_Source *)cw,
                    _e_mod_comp_cb_pending_after, cw->c);

                  cw->show_anim = EINA_TRUE;
               }
          }
     }
   if ((cw->shobj) && (cw->obj))
     {
        if (pshaped != cw->shaped)
          {
             if (cw->shaped)
               edje_object_signal_emit(cw->shobj, "e,state,shadow,off", "e");
             else
               edje_object_signal_emit(cw->shobj, "e,state,shadow,on", "e");
          }
     }

   if (_comp_mod->conf->grab) ecore_x_ungrab();
}

static void
_e_mod_comp_pre_swap(void *data,
                     Evas *e __UNUSED__)
{
   E_Comp *c = data;

   if (_comp_mod->conf->grab)
     {
        if (c->grabbed)
          {
             ecore_x_ungrab();
             c->grabbed = 0;
          }
     }
}

static Eina_Bool
_e_mod_comp_cb_delayed_update_timer(void *data)
{
   E_Comp *c = data;
   _e_mod_comp_render_queue(c);
   c->new_up_timer = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static void
_e_mod_comp_fps_update(E_Comp *c)
{
   if (_comp_mod->conf->fps_show)
     {
        if (!c->fps_bg)
          {
             c->fps_bg = evas_object_rectangle_add(c->evas);
             evas_object_color_set(c->fps_bg, 0, 0, 0, 128);
             evas_object_layer_set(c->fps_bg, EVAS_LAYER_MAX);
             evas_object_show(c->fps_bg);

             c->fps_fg = evas_object_text_add(c->evas);
             evas_object_text_font_set(c->fps_fg, "Sans", 10);
             evas_object_text_text_set(c->fps_fg, "???");
             evas_object_color_set(c->fps_fg, 255, 255, 255, 255);
             evas_object_layer_set(c->fps_fg, EVAS_LAYER_MAX);
             evas_object_show(c->fps_fg);
          }
     }
   else
     {
        if (c->fps_fg)
          {
             evas_object_del(c->fps_fg);
             c->fps_fg = NULL;
          }
        if (c->fps_bg)
          {
             evas_object_del(c->fps_bg);
             c->fps_bg = NULL;
          }
     }
}

static void
_e_mod_comp_win_release(E_Comp_Win *cw)
{
   Eina_List *l;
   Evas_Object *o;
 
   if (cw->xim)
     {
        evas_object_image_size_set(cw->obj, 1, 1);
        evas_object_image_data_set(cw->obj, NULL);
        ecore_x_image_free(cw->xim);
        cw->xim = NULL;
     }
   evas_object_image_native_surface_set(cw->obj, NULL);
   cw->native = 0;
   EINA_LIST_FOREACH(cw->obj_mirror, l, o)
     {
        if (cw->xim)
          {
             evas_object_image_size_set(o, 1, 1);
             evas_object_image_data_set(o, NULL);
          }
        evas_object_image_native_surface_set(o, NULL);
     }
   if (cw->pixmap)
     {
        ecore_x_pixmap_free(cw->pixmap);
        cw->pixmap = 0;
        cw->pw = 0;
        cw->ph = 0;
        ecore_x_e_comp_pixmap_set(cw->win, cw->pixmap);
        cw->show_ready = 0; // hmm maybe not needed?
     }
   if (cw->redirected)
     {
// we redirect all subwindows anyway
//        ecore_x_composite_unredirect_window(cw->win, ECORE_X_COMPOSITE_UPDATE_MANUAL);
        cw->redirected = 0;
     }
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
}

static void
_e_mod_comp_win_adopt(E_Comp_Win *cw)
{
   if (!cw->damage)
     {
        cw->damage = ecore_x_damage_new
          (cw->win, ECORE_X_DAMAGE_REPORT_DELTA_RECTANGLES);
        eina_hash_add(damages, e_util_winid_str_get(cw->damage), cw);
     }
   if (!cw->update)
     {
        cw->update = 1;
        cw->c->updates = eina_list_append(cw->c->updates, cw);
     }
   cw->redirected = 1;
   e_mod_comp_update_resize(cw->up, cw->pw, cw->ph);
   e_mod_comp_update_add(cw->up, 0, 0, cw->pw, cw->ph);
   _e_mod_comp_win_damage(cw, 0, 0, cw->w, cw->h, 0);
   _e_mod_comp_win_render_queue(cw);
}

static void
_e_mod_comp_cb_nocomp_begin(E_Comp *c)
{
   E_Comp_Win *cw, *cwf;
   
   if (c->nocomp) return;
   
   if (c->nocomp_delay_timer)
     {
        ecore_timer_del(c->nocomp_delay_timer);
        c->nocomp_delay_timer = NULL;
     }

   cwf = _e_mod_comp_fullscreen_check(c);
   if (!cwf) return;
   
   EINA_INLIST_FOREACH(c->wins, cw)
     {
        _e_mod_comp_win_release(cw);
     }
   cw = cwf;

   fprintf(stderr, "NOCOMP win %x shobj %p\n", cw->win, cw->shobj);
   
   _e_mod_comp_win_release(cw);
   
   ecore_x_composite_unredirect_subwindows
     (c->man->root, ECORE_X_COMPOSITE_UPDATE_MANUAL);
   c->nocomp = 1;
   c->render_overflow = OVER_FLOW;
   ecore_x_window_hide(c->win);
   ecore_evas_manual_render_set(c->ee, EINA_TRUE);
   ecore_evas_resize(c->ee, 1, 1);
   edje_file_cache_flush();
   edje_collection_cache_flush();
   evas_image_cache_flush(c->evas);
   evas_font_cache_flush(c->evas);
   evas_render_dump(c->evas);
   cw->nocomp = 1;
   if (cw->redirected)
     {
//        ecore_x_composite_unredirect_window(cw->win, ECORE_X_COMPOSITE_UPDATE_MANUAL);
        cw->redirected = 0;
     }
   if (cw->update_timeout)
     {
        ecore_timer_del(cw->update_timeout);
        cw->update_timeout = NULL;
     }
   if (cw->update)
     {
        cw->update = 0;
        cw->c->updates = eina_list_remove(cw->c->updates, cw);
     }
   if (cw->counter)
     {
        if (cw->bd)
          ecore_x_e_comp_sync_cancel_send(cw->bd->client.win);
        else
          ecore_x_e_comp_sync_cancel_send(cw->win);
        ecore_x_sync_counter_inc(cw->counter, 1);
     }
   DBG("JOB2...\n");
   _e_mod_comp_render_queue(c);
}

static void
_e_mod_comp_cb_nocomp_end(E_Comp *c)
{
   E_Comp_Win *cw;
   
   if (!c->nocomp) return;

   ecore_x_composite_redirect_subwindows
     (c->man->root, ECORE_X_COMPOSITE_UPDATE_MANUAL);
   
   fprintf(stderr, "COMP!\n");
   c->nocomp = 0;
   c->render_overflow = OVER_FLOW;
//   ecore_evas_manual_render_set(c->ee, _comp_mod->conf->lock_fps);
   ecore_evas_manual_render_set(c->ee, EINA_FALSE);
   ecore_evas_resize(c->ee, c->man->w, c->man->h);
   ecore_x_window_show(c->win);
   EINA_INLIST_FOREACH(c->wins, cw)
     {
        if (!cw->nocomp)
          {
             if ((cw->input_only) || (cw->invalid)) continue;
             
             if (cw->nocomp_need_update)
               {
                  cw->nocomp_need_update = EINA_FALSE;
                  
                  e_mod_comp_update_resize(cw->up, cw->pw, cw->ph);
                  e_mod_comp_update_add(cw->up, 0, 0, cw->pw, cw->ph);
               }
             _e_mod_comp_win_adopt(cw);
             continue;
          }
        cw->nocomp = 0;
        
        _e_mod_comp_win_adopt(cw);
        
        fprintf(stderr, "restore comp %x --- %p\n", cw->win, cw->shobj);
        
        if (cw->visible)
          {
             if (!cw->hidden_override) _e_mod_comp_child_show(cw);
             cw->pending_count++;
             e_manager_comp_event_src_visibility_send
               (cw->c->man, (E_Manager_Comp_Source *)cw,
                   _e_mod_comp_cb_pending_after, cw->c);
             // no need for effect
          }
        if (cw->counter)
          {
             if (cw->bd)
               ecore_x_e_comp_sync_begin_send(cw->bd->client.win);
             else
               ecore_x_e_comp_sync_begin_send(cw->win);
          }
     }
}

static Eina_Bool
_e_mod_comp_cb_nocomp_begin_timeout(void *data)
{
   E_Comp *c = data;

   c->nocomp_delay_timer = NULL;
   if (c->nocomp_override == 0)
     {
        if (_e_mod_comp_fullscreen_check(c)) c->nocomp_want = 1;
        _e_mod_comp_cb_nocomp_begin(c);
     }
   return EINA_FALSE;
}

static Eina_Bool
_e_mod_comp_cb_update(E_Comp *c)
{
   E_Comp_Win *cw;
   Eina_List *new_updates = NULL; // for failed pixmap fetches - get them next frame
   Eina_List *update_done = NULL;
//   static int doframeinfo = -1;

   if (!c) return EINA_FALSE;
   c->update_job = NULL;
   DBG("UPDATE ALL\n");
   if (c->nocomp) goto nocomp;
   if (_comp_mod->conf->grab)
     {
        ecore_x_grab();
        ecore_x_sync();
        c->grabbed = 1;
     }
   EINA_LIST_FREE(c->updates, cw)
     {
        if (_comp_mod->conf->efl_sync)
          {
             if (((cw->counter) && (cw->drawme)) || (!cw->counter))
               {
                  _e_mod_comp_win_update(cw);
                  if (cw->drawme)
                    {
                       update_done = eina_list_append(update_done, cw);
                       cw->drawme = 0;
                    }
               }
             else
               cw->update = 0;
          }
        else
          _e_mod_comp_win_update(cw);
        if (cw->update)
          {
             new_updates = eina_list_append(new_updates, cw);
          }
     }
   _e_mod_comp_fps_update(c);
   if (_comp_mod->conf->fps_show)
     {
        char buf[128];
        double fps = 0.0, t, dt;
        int i;
        Evas_Coord x = 0, y = 0, w = 0, h = 0;
        E_Zone *z;

        t = ecore_time_get();
        if (_comp_mod->conf->fps_average_range < 1)
          _comp_mod->conf->fps_average_range = 30;
        else if (_comp_mod->conf->fps_average_range > 120)
          _comp_mod->conf->fps_average_range = 120;
        dt = t - c->frametimes[_comp_mod->conf->fps_average_range - 1];
        if (dt > 0.0) fps = (double)_comp_mod->conf->fps_average_range / dt;
        else fps = 0.0;
        if (fps > 0.0) snprintf(buf, sizeof(buf), "FPS: %1.1f", fps);
        else snprintf(buf, sizeof(buf), "N/A");
        for (i = 121; i >= 1; i--)
          c->frametimes[i] = c->frametimes[i - 1];
        c->frametimes[0] = t;
        c->frameskip++;
        if (c->frameskip >= _comp_mod->conf->fps_average_range)
          {
             c->frameskip = 0;
             evas_object_text_text_set(c->fps_fg, buf);
          }
        evas_object_geometry_get(c->fps_fg, NULL, NULL, &w, &h);
        w += 8;
        h += 8;
        z = e_util_zone_current_get(c->man);
        if (z)
          {
             switch (_comp_mod->conf->fps_corner)
               {
                case 3: // bottom-right
                  x = z->x + z->w - w;
                  y = z->y + z->h - h;
                  break;

                case 2: // bottom-left
                  x = z->x;
                  y = z->y + z->h - h;
                  break;

                case 1: // top-right
                  x = z->x + z->w - w;
                  y = z->y;
                  break;

                default: // 0 // top-left
                  x = z->x;
                  y = z->y;
                  break;
               }
          }
        evas_object_move(c->fps_bg, x, y);
        evas_object_resize(c->fps_bg, w, h);
        evas_object_move(c->fps_fg, x + 4, y + 4);
     }
   if (_comp_mod->conf->lock_fps)
     {
        DBG("MANUAL RENDER...\n");
//        if (!c->nocomp) ecore_evas_manual_render(c->ee);
     }
   if (_comp_mod->conf->efl_sync)
     {
        EINA_LIST_FREE(update_done, cw)
          {
             ecore_x_sync_counter_inc(cw->counter, 1);
          }
     }
   if (_comp_mod->conf->grab)
     {
        if (c->grabbed)
          {
             c->grabbed = 0;
             ecore_x_ungrab();
          }
     }
   if (new_updates)
     {
        DBG("JOB1...\n");
        if (c->new_up_timer) ecore_timer_del(c->new_up_timer);
        c->new_up_timer =
          ecore_timer_add(0.001, _e_mod_comp_cb_delayed_update_timer, c);
//        _e_mod_comp_render_queue(c);
     }
   c->updates = new_updates;
   if (!c->animating) c->render_overflow--;
/*
   if (doframeinfo == -1)
     {
        doframeinfo = 0;
        if (getenv("DFI")) doframeinfo = 1;
     }
   if (doframeinfo)
     {
        static double t0 = 0.0;
        double td, t;

        t = ecore_time_get();
        td = t - t0;
        if (td > 0.0)
          {
             int fps, i;

             fps = 1.0 / td;
             for (i = 0; i < fps; i+= 2) putchar('=');
             printf(" : %3.3f\n", 1.0 / td);
          }
        t0 = t;
     }
 */
nocomp:
   cw = _e_mod_comp_fullscreen_check(c);
   if (cw)
     {
        if (_comp_mod->conf->nocomp_fs)
          {
             if ((!c->nocomp) && (!c->nocomp_override > 0))
               {
                  if (!c->nocomp_delay_timer)
                    c->nocomp_delay_timer = ecore_timer_add
                    (1.0, _e_mod_comp_cb_nocomp_begin_timeout, c);
               }
          }
     }
   else
     {
        c->nocomp_want = 0;
        if (c->nocomp_delay_timer)
          {
             ecore_timer_del(c->nocomp_delay_timer);
             c->nocomp_delay_timer = NULL;
          }
        if (c->nocomp)
          _e_mod_comp_cb_nocomp_end(c);
     }

   DBG("UPDATE ALL DONE: overflow = %i\n", c->render_overflow);
   if (c->render_overflow <= 0)
     {
        c->render_overflow = 0;
        if (c->render_animator) c->render_animator = NULL;
        return ECORE_CALLBACK_CANCEL;
     }
   return ECORE_CALLBACK_RENEW;
}

static void
_e_mod_comp_cb_job(void *data)
{
   DBG("UPDATE ALL JOB...\n");
   _e_mod_comp_cb_update(data);
}

static Eina_Bool
_e_mod_comp_cb_animator(void *data)
{
   return _e_mod_comp_cb_update(data);
}

static void
_e_mod_comp_render_queue(E_Comp *c)
{
   /* FIXME workaround */
   if (!c) return;

   if (_comp_mod->conf->lock_fps)
     {
        if (c->render_animator)
          {
             c->render_overflow = OVER_FLOW;
             return;
          }
        c->render_animator = ecore_animator_add(_e_mod_comp_cb_animator, c);
     }
   else
     {
        if (c->update_job)
          {
             DBG("UPDATE JOB DEL...\n");
             ecore_job_del(c->update_job);
             c->update_job = NULL;
             c->render_overflow = 0;
          }
        DBG("UPDATE JOB ADD...\n");
        c->update_job = ecore_job_add(_e_mod_comp_cb_job, c);
     }
}

static void
_e_mod_comp_win_render_queue(E_Comp_Win *cw)
{
   DBG("JOB3...\n");
   _e_mod_comp_render_queue(cw->c);
}

static E_Comp *
_e_mod_comp_find(Ecore_X_Window root)
{
   Eina_List *l;
   E_Comp *c;

   // fixme: use hash if compositors list > 4
   EINA_LIST_FOREACH(compositors, l, c)
     {
        if (c->man->root == root) return c;
     }
   return NULL;
}

static E_Comp_Win *
_e_mod_comp_win_find(Ecore_X_Window win)
{
   return eina_hash_find(windows, e_util_winid_str_get(win));
}

static E_Comp_Win *
_e_mod_comp_border_client_find(Ecore_X_Window win)
{
   return eina_hash_find(borders, e_util_winid_str_get(win));
}

static E_Comp_Win *
_e_mod_comp_win_damage_find(Ecore_X_Damage damage)
{
   return eina_hash_find(damages, e_util_winid_str_get(damage));
}

static Eina_Bool
_e_mod_comp_win_is_borderless(E_Comp_Win *cw)
{
   if (!cw->bd) return 1;
   if ((cw->bd->client.border.name) &&
       (!strcmp(cw->bd->client.border.name, "borderless")))
     return 1;
   return 0;
}

static Eina_Bool
_e_mod_comp_win_do_shadow(E_Comp_Win *cw)
{
   if (cw->shaped) return 0;
   if (cw->argb)
     {
        if (_e_mod_comp_win_is_borderless(cw)) return 0;
     }
   return 1;
}

static Eina_Bool
_e_mod_comp_win_damage_timeout(void *data)
{
   E_Comp_Win *cw = data;

   if (!cw->update)
     {
        if (cw->update_timeout)
          {
             ecore_timer_del(cw->update_timeout);
             cw->update_timeout = NULL;
          }
        cw->update = 1;
        cw->c->updates = eina_list_append(cw->c->updates, cw);
     }
   cw->drawme = 1;
   _e_mod_comp_win_render_queue(cw);
   cw->update_timeout = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static void
_e_mod_comp_object_del(void *data,
                       void *obj)
{
   E_Comp_Win *cw = data;

   if (!cw) return;

   _e_mod_comp_win_render_queue(cw);
   if (obj == cw->bd)
     {
        if (cw->counter)
          {
             if (cw->bd)
               ecore_x_e_comp_sync_cancel_send(cw->bd->client.win);
             else
               ecore_x_e_comp_sync_cancel_send(cw->win);
             ecore_x_sync_counter_inc(cw->counter, 1);
          }
        if (cw->bd) eina_hash_del(borders, e_util_winid_str_get(cw->bd->client.win), cw);
        cw->bd = NULL;
        evas_object_data_del(cw->shobj, "border");
// hmm - lockup?
//        cw->counter = 0;
     }
   else if (obj == cw->pop)
     {
        cw->pop = NULL;
        evas_object_data_del(cw->shobj, "popup");
     }
   else if (obj == cw->menu)
     {
        cw->menu = NULL;
        evas_object_data_del(cw->shobj, "menu");
     }
   if (cw->dfn)
     {
        e_object_delfn_del(obj, cw->dfn);
        cw->dfn = NULL;
     }
}

static void
_e_mod_comp_done_defer(E_Comp_Win *cw)
{
   if (cw->animating)
     {
        cw->c->animating--;
     }
   cw->animating = 0;
   _e_mod_comp_win_render_queue(cw);
   cw->force = 1;
   if (cw->defer_hide) _e_mod_comp_win_hide(cw);
   cw->force = 1;
   if (cw->delete_me) _e_mod_comp_win_del(cw);
   else cw->force = 0;
}

static void
_e_mod_comp_show_done(void *data,
                      Evas_Object *obj     __UNUSED__,
                      const char *emission __UNUSED__,
                      const char *source   __UNUSED__)
{
   E_Comp_Win *cw = data;
   _e_mod_comp_done_defer(cw);
}

static void
_e_mod_comp_hide_done(void *data,
                      Evas_Object *obj     __UNUSED__,
                      const char *emission __UNUSED__,
                      const char *source   __UNUSED__)
{
   E_Comp_Win *cw = data;
   _e_mod_comp_done_defer(cw);
}

static void
_e_mod_comp_win_sync_setup(E_Comp_Win *cw,
                           Ecore_X_Window win)
{
   if (!_comp_mod->conf->efl_sync) return;

   if (cw->bd)
     {
        if (_e_mod_comp_win_is_borderless(cw) ||
            (_comp_mod->conf->loose_sync))
          cw->counter = ecore_x_e_comp_sync_counter_get(win);
        else
          ecore_x_e_comp_sync_cancel_send(win);
     }
   else
     cw->counter = ecore_x_e_comp_sync_counter_get(win);
   if (cw->counter)
     {
        ecore_x_e_comp_sync_begin_send(win);
        ecore_x_sync_counter_inc(cw->counter, 1);
     }
}

static void
_e_mod_comp_win_shadow_setup(E_Comp_Win *cw)
{
   Evas_Object *o;
   int ok = 0;
   char buf[4096];
   Eina_List *list = NULL, *l;
   Match *m;
   Eina_Bool focus = EINA_FALSE, urgent = EINA_FALSE;
   const char *title = NULL, *name = NULL, *clas = NULL, *role = NULL;
   Ecore_X_Window_Type primary_type = ECORE_X_WINDOW_TYPE_UNKNOWN;

   evas_object_image_smooth_scale_set(cw->obj, _comp_mod->conf->smooth_windows);
   EINA_LIST_FOREACH(cw->obj_mirror, l, o)
     {
        evas_object_image_smooth_scale_set(o, _comp_mod->conf->smooth_windows);
     }
   if (cw->bd)
     {
        list = _comp_mod->conf->match.borders;
        title = cw->bd->client.icccm.title;
        if (cw->bd->client.netwm.name) title = cw->bd->client.netwm.name;
        name = cw->bd->client.icccm.name;
        clas = cw->bd->client.icccm.class;
        role = cw->bd->client.icccm.window_role;
        primary_type = cw->bd->client.netwm.type;
     }
   else if (cw->pop)
     {
        // FIXME: i only added "shelf" as a name for popups that are shelves
        // ... need more nmes like for pager popup, evertything, exebuf
        // etc. etc.
        list = _comp_mod->conf->match.popups;
        name = cw->pop->name;
     }
   else if (cw->menu)
     {
        // FIXME: e has no way to tell e menus apart... need naming
        list = _comp_mod->conf->match.menus;
     }
   else
     {
        list = _comp_mod->conf->match.overrides;
        title = cw->title;
        name = cw->name;
        clas = cw->clas;
        role = cw->role;
        primary_type = cw->primary_type;
     }

   EINA_LIST_FOREACH(list, l, m)
     {
        if (((m->title) && (!title)) ||
            ((title) && (m->title) && (!e_util_glob_match(title, m->title))))
          continue;
        if (((m->name) && (!name)) ||
            ((name) && (m->name) && (!e_util_glob_match(name, m->name))))
          continue;
        if (((m->clas) && (!clas)) ||
            ((clas) && (m->clas) && (!e_util_glob_match(clas, m->clas))))
          continue;
        if (((m->role) && (!role)) ||
            ((role) && (m->role) && (!e_util_glob_match(role, m->role))))
          continue;
        if ((primary_type != ECORE_X_WINDOW_TYPE_UNKNOWN) &&
            (m->primary_type != ECORE_X_WINDOW_TYPE_UNKNOWN) &&
            ((int)primary_type != m->primary_type))
          continue;
        if (cw->bd)
          {
             if (m->borderless != 0)
               {
                  int borderless = 0;

                  if ((cw->bd->client.mwm.borderless) || (cw->bd->borderless))
                    borderless = 1;
                  if (!(((m->borderless == -1) && (!borderless)) ||
                        ((m->borderless == 1) && (borderless))))
                    continue;
               }
             if (m->dialog != 0)
               {
                  int dialog = 0;

                  if (((cw->bd->client.icccm.transient_for != 0) ||
                       (cw->bd->client.netwm.type == ECORE_X_WINDOW_TYPE_DIALOG)))
                    dialog = 1;
                  if (!(((m->dialog == -1) && (!dialog)) ||
                        ((m->dialog == 1) && (dialog))))
                    continue;
               }
             if (m->accepts_focus != 0)
               {
                  int accepts_focus = 0;

                  if (cw->bd->client.icccm.accepts_focus)
                    accepts_focus = 1;
                  if (!(((m->accepts_focus == -1) && (!accepts_focus)) ||
                        ((m->accepts_focus == 1) && (accepts_focus))))
                    continue;
               }
             if (m->vkbd != 0)
               {
                  int vkbd = 0;

                  if (cw->bd->client.vkbd.vkbd)
                    vkbd = 1;
                  if (!(((m->vkbd == -1) && (!vkbd)) ||
                        ((m->vkbd == 1) && (vkbd))))
                    continue;
               }
             if (m->quickpanel != 0)
               {
                  int quickpanel = 0;

                  if (cw->bd->client.illume.quickpanel.quickpanel)
                    quickpanel = 1;
                  if (!(((m->quickpanel == -1) && (!quickpanel)) ||
                        ((m->quickpanel == 1) && (quickpanel))))
                    continue;
               }
             if (m->argb != 0)
               {
                  if (!(((m->argb == -1) && (!cw->argb)) ||
                        ((m->argb == 1) && (cw->argb))))
                    continue;
               }
             if (m->fullscreen != 0)
               {
                  int fullscreen = 0;

                  if (cw->bd->client.netwm.state.fullscreen)
                    fullscreen = 1;
                  if (!(((m->fullscreen == -1) && (!fullscreen)) ||
                        ((m->fullscreen == 1) && (fullscreen))))
                    continue;
               }
             if (m->modal != 0)
               {
                  int modal = 0;

                  if (cw->bd->client.netwm.state.modal)
                    modal = 1;
                  if (!(((m->modal == -1) && (!modal)) ||
                        ((m->modal == 1) && (modal))))
                    continue;
               }
          }
        focus = m->focus;
        urgent = m->urgent;
        if (m->shadow_style)
          {
             snprintf(buf, sizeof(buf), "e/comp/%s",
                      m->shadow_style);
             ok = e_theme_edje_object_set(cw->shobj, "base/theme/borders",
                                          buf);
             if (ok) break;
          }
     }
   if (!ok)
     {
        if (cw->bd && cw->bd->client.e.state.video)
          ok = e_theme_edje_object_set(cw->shobj,
                                       "base/theme/borders",
                                       "e/comp/none");
     }
   if (!ok)
     {
        if (_comp_mod->conf->shadow_style)
          {
             snprintf(buf, sizeof(buf), "e/comp/%s",
                      _comp_mod->conf->shadow_style);
             ok = e_theme_edje_object_set(cw->shobj, "base/theme/borders",
                                          buf);
          }
        if (!ok)
          ok = e_theme_edje_object_set(cw->shobj, "base/theme/borders",
                                       "e/comp/default");
     }
   edje_object_part_swallow(cw->shobj, "e.swallow.content", cw->obj);
   if (cw->bd && cw->bd->client.e.state.video)
     edje_object_signal_emit(cw->shobj, "e,state,shadow,off", "e");
   else
     {
        if (_e_mod_comp_win_do_shadow(cw))
          edje_object_signal_emit(cw->shobj, "e,state,shadow,on", "e");
        else
          edje_object_signal_emit(cw->shobj, "e,state,shadow,off", "e");
     }

   if (cw->bd || focus || urgent)
     {
        if (focus || (cw->bd && cw->bd->focused))
          edje_object_signal_emit(cw->shobj, "e,state,focus,on", "e");
        if (urgent || (cw->bd && cw->bd->client.icccm.urgent))
          edje_object_signal_emit(cw->shobj, "e,state,urgent,on", "e");
     }
   if (cw->visible)
     edje_object_signal_emit(cw->shobj, "e,state,visible,on", "e");
   else
     edje_object_signal_emit(cw->shobj, "e,state,visible,off", "e");
   if (!cw->animating)
     {
        cw->c->animating++;
     }
   cw->animating = 1;
   _e_mod_comp_win_render_queue(cw);
}

static void
_e_mod_comp_cb_win_mirror_del(void *data, Evas *e __UNUSED__, Evas_Object *obj, void *event_info __UNUSED__)
{
   E_Comp_Win *cw;

   if (!(cw = data)) return;
   cw->obj_mirror = eina_list_remove(cw->obj_mirror, obj);
}

static Evas_Object *
_e_mod_comp_win_mirror_add(E_Comp_Win *cw)
{
   Evas_Object *o;

   if (!cw->c) return NULL;

   o = evas_object_image_filled_add(cw->c->evas);
   evas_object_image_colorspace_set(o, EVAS_COLORSPACE_ARGB8888);
   cw->obj_mirror = eina_list_append(cw->obj_mirror, o);
   evas_object_image_smooth_scale_set(o, _comp_mod->conf->smooth_windows);

   evas_object_event_callback_add(o, EVAS_CALLBACK_DEL,
                                  _e_mod_comp_cb_win_mirror_del, cw);

   if ((cw->pixmap) && (cw->pw > 0) && (cw->ph > 0))
     {
        unsigned int *pix;
        Eina_Bool alpha;
        int w, h;

        alpha = evas_object_image_alpha_get(cw->obj);
        evas_object_image_size_get(cw->obj, &w, &h);

        evas_object_image_alpha_set(o, alpha);

        if (cw->shaped)
          {
             pix = evas_object_image_data_get(cw->obj, 0);
             evas_object_image_data_set(o, pix);
             evas_object_image_size_set(o, w, h);
             evas_object_image_data_set(o, pix);
             evas_object_image_data_update_add(o, 0, 0, w, h);
          }
        else
          {
             if (cw->native)
               {
                  Evas_Native_Surface ns;

                  ns.version = EVAS_NATIVE_SURFACE_VERSION;
                  ns.type = EVAS_NATIVE_SURFACE_X11;
                  ns.data.x11.visual = cw->vis;
                  ns.data.x11.pixmap = cw->pixmap;
                  evas_object_image_size_set(o, w, h);
                  evas_object_image_native_surface_set(o, &ns);
                  evas_object_image_data_update_add(o, 0, 0, w, h);
               }
             else if (cw->xim)
               {
                  if (ecore_x_image_is_argb32_get(cw->xim))
                    {
                       pix = ecore_x_image_data_get(cw->xim, NULL, NULL, NULL);
                       evas_object_image_data_set(o, pix);
                       evas_object_image_size_set(o, w, h);
                    }
                  else
                    {
                       pix = evas_object_image_data_get(cw->obj, EINA_TRUE);
                       evas_object_image_data_set(o, pix);
                       evas_object_image_size_set(o, w, h);
                       evas_object_image_data_set(cw->obj, pix);
                    }
                  evas_object_image_data_update_add(o, 0, 0, w, h);
               }
          }
        evas_object_image_size_set(o, w, h);
        evas_object_image_data_update_add(o, 0, 0, w, h);
     }
   return o;
}

static E_Comp_Win *
_e_mod_comp_win_add(E_Comp *c,
                    Ecore_X_Window win)
{
   Ecore_X_Window_Attributes att;
   E_Comp_Win *cw;

   cw = calloc(1, sizeof(E_Comp_Win));
   if (!cw) return NULL;
   cw->win = win;
   cw->c = c;
   cw->real_hid = 1;
   cw->opacity = 255.0;
   cw->bd = e_border_find_by_window(cw->win);
   if (_comp_mod->conf->grab) ecore_x_grab();
   if (cw->bd)
     {
        eina_hash_add(borders, e_util_winid_str_get(cw->bd->client.win), cw);
        cw->dfn = e_object_delfn_add(E_OBJECT(cw->bd), _e_mod_comp_object_del, cw);

        // setup on show
        // _e_mod_comp_win_sync_setup(cw, cw->bd->client.win);
     }
   else if ((cw->pop = e_popup_find_by_window(cw->win)))
     {
        cw->dfn = e_object_delfn_add(E_OBJECT(cw->pop),
                                     _e_mod_comp_object_del, cw);
        cw->show_ready = 1;
     }
   else if ((cw->menu = e_menu_find_by_window(cw->win)))
     {
        cw->dfn = e_object_delfn_add(E_OBJECT(cw->menu),
                                     _e_mod_comp_object_del, cw);
        cw->show_ready = 1;
     }
   else
     {
        char *netwm_title = NULL;

        cw->title = ecore_x_icccm_title_get(cw->win);
        if (ecore_x_netwm_name_get(cw->win, &netwm_title))
          {
             free(cw->title);
             cw->title = netwm_title;
          }
        ecore_x_icccm_name_class_get(cw->win, &cw->name, &cw->clas);
        cw->role = ecore_x_icccm_window_role_get(cw->win);
        if (!ecore_x_netwm_window_type_get(cw->win, &cw->primary_type))
          cw->primary_type = ECORE_X_WINDOW_TYPE_UNKNOWN;
        // setup on show
        // _e_mod_comp_win_sync_setup(cw, cw->win);
     }

   if (!cw->counter)
     {
        // FIXME: config - disable ready timeout for non-counter wins
        //        cw->show_ready = 1;
     }
   // fixme: could use bd/pop/menu for this too
   memset((&att), 0, sizeof(Ecore_X_Window_Attributes));
   if (!ecore_x_window_attributes_get(cw->win, &att))
     {
        free(cw->name);
        free(cw->clas);
        free(cw->role);
        free(cw);
        if (_comp_mod->conf->grab) ecore_x_ungrab();
        return NULL;
     }
   if ((!att.input_only) &&
       ((att.depth != 24) && (att.depth != 32)))
     {
//        printf("WARNING: window 0x%x not 24/32bpp -> %ibpp\n", cw->win, att.depth);
//        cw->invalid = 1;
     }
   cw->input_only = att.input_only;
   cw->override = att.override;
   cw->vis = att.visual;
   cw->cmap = att.colormap;
   cw->depth = att.depth;
   cw->argb = ecore_x_window_argb_get(cw->win);
   eina_hash_add(windows, e_util_winid_str_get(cw->win), cw);
   cw->inhash = 1;
   if ((!cw->input_only) && (!cw->invalid))
     {
        Ecore_X_Rectangle *rects;
        int num;

        cw->damage = ecore_x_damage_new
            (cw->win, ECORE_X_DAMAGE_REPORT_DELTA_RECTANGLES);
        eina_hash_add(damages, e_util_winid_str_get(cw->damage), cw);
        cw->shobj = edje_object_add(c->evas);
        cw->obj = evas_object_image_filled_add(c->evas);
        evas_object_image_colorspace_set(cw->obj, EVAS_COLORSPACE_ARGB8888);
        if (cw->argb) evas_object_image_alpha_set(cw->obj, 1);
        else evas_object_image_alpha_set(cw->obj, 0);

        if (cw->override && !(att.event_mask.mine & ECORE_X_EVENT_MASK_WINDOW_PROPERTY))
          ecore_x_event_mask_set(cw->win, ECORE_X_EVENT_MASK_WINDOW_PROPERTY);

        _e_mod_comp_win_shadow_setup(cw);

        edje_object_signal_callback_add(cw->shobj, "e,action,show,done", "e",
                                        _e_mod_comp_show_done, cw);
        edje_object_signal_callback_add(cw->shobj, "e,action,hide,done", "e",
                                        _e_mod_comp_hide_done, cw);
        
        _e_mod_comp_win_layout_populate(cw);

        evas_object_show(cw->obj);
        ecore_x_window_shape_events_select(cw->win, 1);
        rects = ecore_x_window_shape_rectangles_get(cw->win, &num);
        if (rects)
          {
             int i;

             for (i = 0; i < num; i++)
               E_RECTS_CLIP_TO_RECT(rects[i].x, rects[i].y,
                                    rects[i].width, rects[i].height,
                                    0, 0, (int)att.w, (int)att.h);

             if (_e_mod_comp_shaped_check(att.w, att.h, rects, num))
               cw->shape_changed = 1;

             free(rects);
          }

        if (cw->bd) evas_object_data_set(cw->shobj, "border", cw->bd);
        else if (cw->pop)
          evas_object_data_set(cw->shobj, "popup", cw->pop);
        else if (cw->menu)
          evas_object_data_set(cw->shobj, "menu", cw->menu);

        evas_object_pass_events_set(cw->obj, 1);

        cw->pending_count++;
        e_manager_comp_event_src_add_send
          (cw->c->man, (E_Manager_Comp_Source *)cw,
          _e_mod_comp_cb_pending_after, cw->c);
     }
   else
     {
        cw->shobj = evas_object_rectangle_add(c->evas);
        
        _e_mod_comp_win_layout_populate(cw);
        
        evas_object_color_set(cw->shobj, 0, 0, 0, 0);
     }
   evas_object_pass_events_set(cw->shobj, 1);
   evas_object_data_set(cw->shobj, "win",
                        (void *)((unsigned long)cw->win));
   evas_object_data_set(cw->shobj, "src", cw);

   c->wins_invalid = 1;
   c->wins = eina_inlist_append(c->wins, EINA_INLIST_GET(cw));
   cw->up = e_mod_comp_update_new();
   e_mod_comp_update_tile_size_set(cw->up, 32, 32);
   // for software:
   e_mod_comp_update_policy_set
     (cw->up, E_UPDATE_POLICY_HALF_WIDTH_OR_MORE_ROUND_UP_TO_FULL_WIDTH);
   if (((!cw->input_only) && (!cw->invalid)) && (cw->override))
     {
        cw->redirected = 1;
        // we redirect all subwindows anyway
        // ecore_x_composite_redirect_window(cw->win, ECORE_X_COMPOSITE_UPDATE_MANUAL);
        cw->dmg_updates = 0;
     }
   DBG("  [0x%x] add\n", cw->win);
   if (_comp_mod->conf->grab) ecore_x_ungrab();
   return cw;
}

static void
_e_mod_comp_win_del(E_Comp_Win *cw)
{
   Evas_Object *o;
   int pending_count;

   if (cw->animating)
     {
        cw->c->animating--;
     }
   cw->animating = 0;
   if ((!cw->input_only) && (!cw->invalid))
     {
        cw->pending_count++;
        e_manager_comp_event_src_del_send
          (cw->c->man, (E_Manager_Comp_Source *)cw,
          _e_mod_comp_cb_pending_after, cw->c);
     }

   e_mod_comp_update_free(cw->up);
   DBG("  [0x%x] del\n", cw->win);
   E_FREE(cw->rects);
   if (cw->update_timeout)
     {
        ecore_timer_del(cw->update_timeout);
        cw->update_timeout = NULL;
     }
   if (cw->ready_timeout)
     {
        ecore_timer_del(cw->ready_timeout);
        cw->ready_timeout = NULL;
     }
   if (cw->dfn)
     {
        if (cw->bd)
          {
             eina_hash_del(borders, e_util_winid_str_get(cw->bd->client.win), cw);
             e_object_delfn_del(E_OBJECT(cw->bd), cw->dfn);
             cw->bd = NULL;
          }
        else if (cw->pop)
          {
             e_object_delfn_del(E_OBJECT(cw->pop), cw->dfn);
             cw->pop = NULL;
          }
        else if (cw->menu)
          {
             e_object_delfn_del(E_OBJECT(cw->menu), cw->dfn);
             cw->menu = NULL;
          }
        cw->dfn = NULL;
     }
   if (cw->update)
     {
        cw->update = 0;
        cw->c->updates = eina_list_remove(cw->c->updates, cw);
     }
   
   _e_mod_comp_win_release(cw);
   
   if (cw->obj_mirror)
     {
        EINA_LIST_FREE(cw->obj_mirror, o)
          {
             if (cw->xim) evas_object_image_data_set(o, NULL);
             evas_object_event_callback_del(o, EVAS_CALLBACK_DEL,
                                            _e_mod_comp_cb_win_mirror_del);
             evas_object_del(o);
          }
     }
   if (cw->obj)
     {
        evas_object_del(cw->obj);
        cw->obj = NULL;
     }
   if (cw->shobj)
     {
        evas_object_del(cw->shobj);
        cw->shobj = NULL;
     }
   
   if (cw->inhash)
     eina_hash_del(windows, e_util_winid_str_get(cw->win), cw);
   
   free(cw->title);
   free(cw->name);
   free(cw->clas);
   free(cw->role);
   cw->c->wins_invalid = 1;
   cw->c->wins = eina_inlist_remove(cw->c->wins, EINA_INLIST_GET(cw));
   pending_count = cw->pending_count;
   memset(cw, 0, sizeof(E_Comp_Win));
   cw->pending_count = pending_count;
   cw->delete_pending = 1;
   if (cw->pending_count > 0) return;
   free(cw);
}

static void
_e_mod_comp_win_show(E_Comp_Win *cw)
{
   Eina_List *l;
   Evas_Object *o;

   if (cw->visible) return;
   cw->visible = 1;
   DBG("  [0x%x] sho ++ [redir=%i, pm=%x, dmg_up=%i]\n",
       cw->win, cw->redirected, cw->pixmap, cw->dmg_updates);
   _e_mod_comp_win_configure(cw, cw->hidden.x, cw->hidden.y, cw->w, cw->h, cw->border);
   if ((cw->input_only) || (cw->invalid)) return;

   cw->show_anim = EINA_FALSE;

// setup on show
   if (cw->bd)
     _e_mod_comp_win_sync_setup(cw, cw->bd->client.win);
   else
     _e_mod_comp_win_sync_setup(cw, cw->win);

   if (cw->real_hid)
     {
        DBG("  [0x%x] real hid - fix\n", cw->win);
        cw->real_hid = 0;
        if (cw->native)
          {
             evas_object_image_native_surface_set(cw->obj, NULL);
             cw->native = 0;
             EINA_LIST_FOREACH(cw->obj_mirror, l, o)
               {
                  evas_object_image_native_surface_set(o, NULL);
               }
          }
        if (cw->pixmap)
          {
             ecore_x_pixmap_free(cw->pixmap);
             cw->pixmap = 0;
             cw->pw = 0;
             cw->ph = 0;
             ecore_x_e_comp_pixmap_set(cw->win, cw->pixmap);
          }
        if (cw->xim)
          {
             evas_object_image_size_set(cw->obj, 1, 1);
             evas_object_image_data_set(cw->obj, NULL);
             ecore_x_image_free(cw->xim);
             cw->xim = NULL;
             EINA_LIST_FOREACH(cw->obj_mirror, l, o)
               {
                  evas_object_image_size_set(o, 1, 1);
                  evas_object_image_data_set(o, NULL);
               }
          }
        if (cw->redirected)
          {
             cw->redirected = 0;
             cw->pw = 0;
             cw->ph = 0;
          }
        if (cw->pop)
          cw->dmg_updates = 1;
        else
          cw->dmg_updates = 0;
     }
   else
     cw->dmg_updates = 1;

   if ((!cw->redirected) || (!cw->pixmap))
     {
        // we redirect all subwindows anyway
        //        ecore_x_composite_redirect_window(cw->win, ECORE_X_COMPOSITE_UPDATE_MANUAL);
/* #ifdef HAVE_WAYLAND_CLIENTS */
/*         if ((cw->bd) && (cw->bd->borderless)) */
/*           cw->pixmap = e_mod_comp_wl_pixmap_get(cw->win); */
/* #endif */
        if (!cw->pixmap)
          cw->pixmap = ecore_x_composite_name_window_pixmap_get(cw->win);
        if (cw->pixmap)
          {
             ecore_x_pixmap_geometry_get(cw->pixmap, NULL, NULL, &(cw->pw), &(cw->ph));
             _e_mod_comp_win_ready_timeout_setup(cw);
          }
        else
          {
             cw->pw = 0;
             cw->ph = 0;
          }
        if ((cw->pw <= 0) || (cw->ph <= 0))
          {
             if (cw->pixmap)
               {
                  ecore_x_pixmap_free(cw->pixmap);
                  cw->pixmap = 0;
               }
             //             cw->show_ready = 0; // hmm maybe not needed?
          }
        cw->redirected = 1;
        DBG("  [0x%x] up resize %ix%i\n", cw->win, cw->pw, cw->ph);
        e_mod_comp_update_resize(cw->up, cw->pw, cw->ph);
        e_mod_comp_update_add(cw->up, 0, 0, cw->pw, cw->ph);
        evas_object_image_size_set(cw->obj, cw->pw, cw->ph);
        EINA_LIST_FOREACH(cw->obj_mirror, l, o)
          {
             evas_object_image_size_set(o, cw->pw, cw->ph);
          }
        ecore_x_e_comp_pixmap_set(cw->win, cw->pixmap);
     }
   if ((cw->dmg_updates >= 1) && (cw->show_ready))
     {
        cw->defer_hide = 0;
        if (!cw->hidden_override) _e_mod_comp_child_show(cw);
        edje_object_signal_emit(cw->shobj, "e,state,visible,on", "e");
        if (!cw->animating)
          {
             cw->c->animating++;
          }
        cw->animating = 1;
        _e_mod_comp_win_render_queue(cw);

        cw->pending_count++;
        e_manager_comp_event_src_visibility_send
          (cw->c->man, (E_Manager_Comp_Source *)cw,
          _e_mod_comp_cb_pending_after, cw->c);
     }
   _e_mod_comp_win_render_queue(cw);
}

static void
_e_mod_comp_win_real_hide(E_Comp_Win *cw)
{
   if (cw->bd)
     {
        _e_mod_comp_win_hide(cw);
        return;
     }
   cw->real_hid = 1;
   _e_mod_comp_win_hide(cw);
}

static void
_e_mod_comp_win_hide(E_Comp_Win *cw)
{
   Eina_List *l;
   Evas_Object *o;

   if ((!cw->visible) && (!cw->defer_hide)) return;
   cw->visible = 0;
   if ((cw->input_only) || (cw->invalid)) return;
   DBG("  [0x%x] hid --\n", cw->win);
   if (!cw->force)
     {
        cw->defer_hide = 1;
        edje_object_signal_emit(cw->shobj, "e,state,visible,off", "e");
        if (!cw->animating)
          {
             cw->c->animating++;
          }
        cw->animating = 1;
        _e_mod_comp_win_render_queue(cw);

        cw->pending_count++;
        e_manager_comp_event_src_visibility_send
          (cw->c->man, (E_Manager_Comp_Source *)cw,
          _e_mod_comp_cb_pending_after, cw->c);
        return;
     }
   cw->defer_hide = 0;
   cw->force = 0;
   _e_mod_comp_child_hide(cw);

   if (cw->update_timeout)
     {
        ecore_timer_del(cw->update_timeout);
        cw->update_timeout = NULL;
     }
   if (_comp_mod->conf->keep_unmapped)
     {
        if (_comp_mod->conf->send_flush)
          {
             if (cw->bd) ecore_x_e_comp_flush_send(cw->bd->client.win);
             else ecore_x_e_comp_flush_send(cw->win);
          }
        if (_comp_mod->conf->send_dump)
          {
             if (cw->bd) ecore_x_e_comp_dump_send(cw->bd->client.win);
             else ecore_x_e_comp_dump_send(cw->win);
          }
        return;
     }
   if (cw->ready_timeout)
     {
        ecore_timer_del(cw->ready_timeout);
        cw->ready_timeout = NULL;
     }

   if (cw->native)
     {
        evas_object_image_native_surface_set(cw->obj, NULL);
        cw->native = 0;
        EINA_LIST_FOREACH(cw->obj_mirror, l, o)
          {
             evas_object_image_native_surface_set(o, NULL);
          }
     }
   if (cw->pixmap)
     {
        ecore_x_pixmap_free(cw->pixmap);
        cw->pixmap = 0;
        cw->pw = 0;
        cw->ph = 0;
        ecore_x_e_comp_pixmap_set(cw->win, cw->pixmap);
//        cw->show_ready = 0; // hmm maybe not needed?
     }
   if (cw->xim)
     {
        evas_object_image_size_set(cw->obj, 1, 1);
        evas_object_image_data_set(cw->obj, NULL);
        ecore_x_image_free(cw->xim);
        cw->xim = NULL;
        EINA_LIST_FOREACH(cw->obj_mirror, l, o)
          {
             evas_object_image_size_set(o, 1, 1);
             evas_object_image_data_set(o, NULL);
          }
     }
   if (cw->redirected)
     {
// we redirect all subwindows anyway
//        ecore_x_composite_unredirect_window(cw->win, ECORE_X_COMPOSITE_UPDATE_MANUAL);
        cw->redirected = 0;
        cw->pw = 0;
        cw->ph = 0;
     }
   _e_mod_comp_win_render_queue(cw);
   if (_comp_mod->conf->send_flush)
     {
        if (cw->bd) ecore_x_e_comp_flush_send(cw->bd->client.win);
        else ecore_x_e_comp_flush_send(cw->win);
     }
   if (_comp_mod->conf->send_dump)
     {
        if (cw->bd) ecore_x_e_comp_dump_send(cw->bd->client.win);
        else ecore_x_e_comp_dump_send(cw->win);
     }
}

static void
_e_mod_comp_win_raise_above(E_Comp_Win *cw,
                            E_Comp_Win *cw2)
{
   DBG("  [0x%x] abv [0x%x]\n", cw->win, cw2->win);
   cw->c->wins_invalid = 1;
   cw->c->wins = eina_inlist_remove(cw->c->wins, EINA_INLIST_GET(cw));
   cw->c->wins = eina_inlist_append_relative(cw->c->wins,
                                             EINA_INLIST_GET(cw),
                                             EINA_INLIST_GET(cw2));
   _e_mod_comp_win_restack(cw);
   _e_mod_comp_win_render_queue(cw);
   cw->pending_count++;
   e_manager_comp_event_src_config_send
     (cw->c->man, (E_Manager_Comp_Source *)cw,
     _e_mod_comp_cb_pending_after, cw->c);
}

static void
_e_mod_comp_win_raise(E_Comp_Win *cw)
{
   DBG("  [0x%x] rai\n", cw->win);
   cw->c->wins_invalid = 1;
   cw->c->wins = eina_inlist_remove(cw->c->wins, EINA_INLIST_GET(cw));
   cw->c->wins = eina_inlist_append(cw->c->wins, EINA_INLIST_GET(cw));
   _e_mod_comp_win_restack(cw);
   _e_mod_comp_win_render_queue(cw);
   cw->pending_count++;
   e_manager_comp_event_src_config_send
     (cw->c->man, (E_Manager_Comp_Source *)cw,
     _e_mod_comp_cb_pending_after, cw->c);
}

static void
_e_mod_comp_win_lower(E_Comp_Win *cw)
{
   DBG("  [0x%x] low\n", cw->win);
   cw->c->wins_invalid = 1;
   cw->c->wins = eina_inlist_remove(cw->c->wins, EINA_INLIST_GET(cw));
   cw->c->wins = eina_inlist_prepend(cw->c->wins, EINA_INLIST_GET(cw));
   _e_mod_comp_win_restack(cw);
   _e_mod_comp_win_render_queue(cw);
   cw->pending_count++;
   e_manager_comp_event_src_config_send
     (cw->c->man, (E_Manager_Comp_Source *)cw,
     _e_mod_comp_cb_pending_after, cw->c);
}

static void
_e_mod_comp_win_configure(E_Comp_Win *cw,
                          int x,
                          int y,
                          int w,
                          int h,
                          int border)
{
   Eina_Bool moved = EINA_FALSE, resized = EINA_FALSE;
   
   if (!cw->visible)
     {
        cw->hidden.x = x;
        cw->hidden.y = y;
        cw->border = border;
     }
   else
     {
        if (!((x == cw->x) && (y == cw->y)))
          {
             DBG("  [0x%x] mov %4i %4i\n", cw->win, x, y);
             cw->x = x;
             cw->y = y;
//             evas_object_move(cw->shobj, cw->x, cw->y);
             moved = EINA_TRUE;
          }
        cw->hidden.x = x;
        cw->hidden.y = y;
     }
   cw->hidden.w = w;
   cw->hidden.h = h;
   if (cw->counter)
     {
        if (!((w == cw->w) && (h == cw->h)))
          {
#if 1
             cw->w = w;
             cw->h = h;
             cw->needpix = 1;
             // was cw->w / cw->h
//             evas_object_resize(cw->shobj, cw->pw, cw->ph);
             resized = EINA_TRUE;
             _e_mod_comp_win_damage(cw, 0, 0, cw->w, cw->h, 0);
#else
             if (cw->bd)
               {
                  if ((cw->bd->shading) || (cw->bd->shaded))
                    {
                       cw->needpix = 1;
                       // was cw->w / cw->h
//                       evas_object_resize(cw->shobj, cw->pw, cw->ph);
                       resized = EINA_TRUE;
                       _e_mod_comp_win_damage(cw, 0, 0, cw->w, cw->h, 0);
                    }
                  else
                    cw->update = 0;
               }
             else
               {
                  cw->update = 0;
//                  if (cw->ready_timeout) ecore_timer_del(cw->ready_timeout);
//                  cw->ready_timeout = ecore_timer_add
//                     (_comp_mod->conf->first_draw_delay,
//                         _e_mod_comp_cb_win_show_ready_timeout, cw);
               }
#endif
          }
        if (cw->border != border)
          {
             cw->border = border;
             cw->needpix = 1;
             // was cw->w / cw->h
//             evas_object_resize(cw->shobj, cw->pw, cw->ph);
             resized = EINA_TRUE;
             _e_mod_comp_win_damage(cw, 0, 0, cw->w, cw->h, 0);
          }
        if ((cw->input_only) || (cw->invalid)) return;
     }
   else
     {
        if (!((w == cw->w) && (h == cw->h)))
          {
             DBG("  [0x%x] rsz %4ix%4i\n", cw->win, w, h);
             cw->w = w;
             cw->h = h;
             cw->needpix = 1;
             // was cw->w / cw->h
//             evas_object_resize(cw->shobj, cw->pw, cw->ph);
             resized = EINA_TRUE;
             _e_mod_comp_win_damage(cw, 0, 0, cw->w, cw->h, 0);
          }
        if (cw->border != border)
          {
             cw->border = border;
             cw->needpix = 1;
//             evas_object_resize(cw->shobj, cw->pw, cw->ph);
             resized = EINA_TRUE;
             _e_mod_comp_win_damage(cw, 0, 0, cw->w, cw->h, 0);
          }
        if ((cw->input_only) || (cw->invalid)) return;
        _e_mod_comp_win_render_queue(cw);
     }
   if (moved || resized) _e_mod_comp_win_geometry_update(cw);
   // add pending manager comp event count to match below config send
   cw->pending_count++;
   e_manager_comp_event_src_config_send(cw->c->man,
                                        (E_Manager_Comp_Source *)cw,
                                        _e_mod_comp_cb_pending_after, cw->c);
}

static void
_e_mod_comp_win_damage(E_Comp_Win *cw,
                       int x,
                       int y,
                       int w,
                       int h,
                       Eina_Bool dmg)
{
   if ((cw->input_only) || (cw->invalid)) return;
   DBG("  [0x%x] dmg [%x] %4i %4i %4ix%4i\n", cw->win, cw->damage, x, y, w, h);
   if ((dmg) && (cw->damage))
     {
        Ecore_X_Region parts;

        parts = ecore_x_region_new(NULL, 0);
        ecore_x_damage_subtract(cw->damage, 0, parts);
        ecore_x_region_free(parts);
        cw->dmg_updates++;
     }
   if (cw->nocomp) return;
   if (cw->c->nocomp)
     {
        cw->nocomp_need_update = EINA_TRUE;
        return;
     }
   e_mod_comp_update_add(cw->up, x, y, w, h);
   if (dmg)
     {
        if (cw->counter)
          {
             if (!cw->update_timeout)
               cw->update_timeout = ecore_timer_add
                   (ecore_animator_frametime_get() * 2,
                   _e_mod_comp_win_damage_timeout, cw);
             return;
          }
     }
   if (!cw->update)
     {
        cw->update = 1;
        cw->c->updates = eina_list_append(cw->c->updates, cw);
     }
   _e_mod_comp_win_render_queue(cw);
}

static void
_e_mod_comp_win_reshape(E_Comp_Win *cw)
{
   if (cw->shape_changed) return;
   cw->shape_changed = 1;
   if (cw->c->nocomp)
     {
        cw->nocomp_need_update = EINA_TRUE;
        return;
     }
   if (!cw->update)
     {
        cw->update = 1;
        cw->c->updates = eina_list_append(cw->c->updates, cw);
     }
   e_mod_comp_update_add(cw->up, 0, 0, cw->w, cw->h);
   _e_mod_comp_win_render_queue(cw);
}

//////////////////////////////////////////////////////////////////////////

static Eina_Bool
_e_mod_comp_create(void *data __UNUSED__,
                   int type   __UNUSED__,
                   void *event)
{
   Ecore_X_Event_Window_Create *ev = event;
   E_Comp_Win *cw;
   E_Comp *c = _e_mod_comp_find(ev->parent);
   if (!c) return ECORE_CALLBACK_PASS_ON;
   if (_e_mod_comp_win_find(ev->win)) return ECORE_CALLBACK_PASS_ON;
   if (c->win == ev->win) return ECORE_CALLBACK_PASS_ON;
   if (c->ee_win == ev->win) return ECORE_CALLBACK_PASS_ON;
   cw = _e_mod_comp_win_add(c, ev->win);
   if (cw) _e_mod_comp_win_configure(cw, ev->x, ev->y, ev->w, ev->h, ev->border);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_destroy(void *data __UNUSED__,
                    int type   __UNUSED__,
                    void *event)
{
   Ecore_X_Event_Window_Destroy *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->win);
   if (!cw) return ECORE_CALLBACK_PASS_ON;
   if (cw->animating) cw->delete_me = 1;
   else _e_mod_comp_win_del(cw);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_show(void *data __UNUSED__,
                 int type   __UNUSED__,
                 void *event)
{
   Ecore_X_Event_Window_Show *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->win);
   if (!cw) return ECORE_CALLBACK_PASS_ON;
   cw->defer_hide = 0;
   if (cw->visible) return ECORE_CALLBACK_PASS_ON;
   _e_mod_comp_win_show(cw);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_hide(void *data __UNUSED__,
                 int type   __UNUSED__,
                 void *event)
{
   Ecore_X_Event_Window_Hide *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->win);
   if (!cw) return ECORE_CALLBACK_PASS_ON;
   if (!cw->visible) return ECORE_CALLBACK_PASS_ON;
   _e_mod_comp_win_real_hide(cw);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_reparent(void *data __UNUSED__,
                     int type   __UNUSED__,
                     void *event)
{
   Ecore_X_Event_Window_Reparent *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->win);
   if (!cw) return ECORE_CALLBACK_PASS_ON;
   DBG("== repar [0x%x] to [0x%x]\n", ev->win, ev->parent);
   if (ev->parent != cw->c->man->root)
     _e_mod_comp_win_del(cw);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_configure(void *data __UNUSED__,
                      int type   __UNUSED__,
                      void *event)
{
   Ecore_X_Event_Window_Configure *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->win);
   if (!cw) return ECORE_CALLBACK_PASS_ON;

   if (ev->abovewin == 0)
     {
        if (EINA_INLIST_GET(cw)->prev) _e_mod_comp_win_lower(cw);
     }
   else
     {
        E_Comp_Win *cw2 = _e_mod_comp_win_find(ev->abovewin);

        if (cw2)
          {
             E_Comp_Win *cw3 = (E_Comp_Win *)(EINA_INLIST_GET(cw)->prev);

             if (cw3 != cw2) _e_mod_comp_win_raise_above(cw, cw2);
          }
     }

   if (!((cw->x == ev->x) && (cw->y == ev->y) &&
         (cw->w == ev->w) && (cw->h == ev->h) &&
         (cw->border == ev->border)))
     {
        _e_mod_comp_win_configure(cw, ev->x, ev->y, ev->w, ev->h, ev->border);
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_stack(void *data __UNUSED__,
                  int type   __UNUSED__,
                  void *event)
{
   Ecore_X_Event_Window_Stack *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->win);
   if (!cw) return ECORE_CALLBACK_PASS_ON;
   if (ev->detail == ECORE_X_WINDOW_STACK_ABOVE) _e_mod_comp_win_raise(cw);
   else _e_mod_comp_win_lower(cw);
   return ECORE_CALLBACK_PASS_ON;
}

static void
_e_mod_comp_win_opacity_set(E_Comp_Win *cw)
{
   unsigned int val;

   if (ecore_x_window_prop_card32_get(cw->win, ECORE_X_ATOM_NET_WM_WINDOW_OPACITY, &val, 1) > 0)
     {
        cw->opacity = (val >> 24);
        evas_object_color_set(cw->shobj, cw->opacity, cw->opacity, cw->opacity, cw->opacity);
     }
}

static Eina_Bool
_e_mod_comp_property(void *data  __UNUSED__,
                     int type    __UNUSED__,
                     void *event __UNUSED__)
{
   Ecore_X_Event_Window_Property *ev = event;

   if (ev->atom == ECORE_X_ATOM_NET_WM_WINDOW_OPACITY)
     {
        E_Comp_Win *cw = _e_mod_comp_win_find(ev->win);
        if (!cw) return ECORE_CALLBACK_PASS_ON;
        _e_mod_comp_win_opacity_set(cw);
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_message(void *data __UNUSED__,
                    int type   __UNUSED__,
                    void *event)
{
   Ecore_X_Event_Client_Message *ev = event;
   E_Comp_Win *cw = NULL;
   int version, w = 0, h = 0;
   Eina_Bool force = 0;

   if (ev->message_type == ECORE_X_ATOM_NET_WM_WINDOW_OPACITY)
     {
        cw = _e_mod_comp_win_find(ev->win);
        if (!cw) return ECORE_CALLBACK_PASS_ON;
        _e_mod_comp_win_opacity_set(cw);
        return ECORE_CALLBACK_PASS_ON;
     }

   if ((ev->message_type != ECORE_X_ATOM_E_COMP_SYNC_DRAW_DONE) ||
       (ev->format != 32)) return ECORE_CALLBACK_PASS_ON;
   version = ev->data.l[1];
   cw = _e_mod_comp_border_client_find(ev->data.l[0]);
   if (cw)
     {
        if (!cw->bd) return ECORE_CALLBACK_PASS_ON;
        if (ev->data.l[0] != (int)cw->bd->client.win) return ECORE_CALLBACK_PASS_ON;
     }
   else
     {
        cw = _e_mod_comp_win_find(ev->data.l[0]);
        if (!cw) return ECORE_CALLBACK_PASS_ON;
        if (ev->data.l[0] != (int)cw->win) return ECORE_CALLBACK_PASS_ON;
     }
   if (version == 1) // v 0 was first, v1 added size params
     {
        w = ev->data.l[2];
        h = ev->data.l[3];
        if (cw->bd)
          {
             int clw, clh;

             if ((cw->bd->shading) || (cw->bd->shaded)) force = 1;
             clw = cw->hidden.w -
               cw->bd->client_inset.l -
               cw->bd->client_inset.r;
             clh = cw->hidden.h -
               cw->bd->client_inset.t -
               cw->bd->client_inset.b;
             DBG("  [0x%x] sync draw done @%4ix%4i, bd %4ix%4i\n", cw->win,
                 w, h, cw->bd->client.w, cw->bd->client.h);
             if ((w != clw) || (h != clh))
               {
                  cw->misses++;
                  if (cw->misses > 1)
                    {
                       cw->misses = 0;
                       force = 1;
                    }
                  else return ECORE_CALLBACK_PASS_ON;
               }
             cw->misses = 0;
          }
        else
          {
             DBG("  [0x%x] sync draw done @%4ix%4i, cw %4ix%4i\n", cw->win, w, h, cw->hidden.w, cw->hidden.h);
             if ((w != cw->hidden.w) || (h != cw->hidden.h))
               {
                  if (cw->misses > 1)
                    {
                       cw->misses = 0;
                       force = 1;
                    }
                  else return ECORE_CALLBACK_PASS_ON;
               }
             cw->misses = 0;
          }
     }
   DBG("  [0x%x] sync draw done %4ix%4i\n", cw->win, cw->w, cw->h);
//   if (cw->bd)
   {
      if (cw->counter)
        {
           DBG("  [0x%x] have counter\n", cw->win);
           cw->show_ready = 1;
           if (!cw->update)
             {
                DBG("  [0x%x] set update\n", cw->win);
                if (cw->update_timeout)
                  {
                     DBG("  [0x%x] del timeout\n", cw->win);
                     ecore_timer_del(cw->update_timeout);
                     cw->update_timeout = NULL;
                  }
                cw->update = 1;
                cw->c->updates = eina_list_append(cw->c->updates, cw);
             }
           if ((cw->w != cw->hidden.w) ||
               (cw->h != cw->hidden.h) ||
               (force))
             {
                DBG("  [0x%x] rsz done msg %4ix%4i\n", cw->win, cw->hidden.w, cw->hidden.h);
                cw->w = cw->hidden.w;
                cw->h = cw->hidden.h;
                cw->needpix = 1;
                // was cw->w / cw->h
//                evas_object_resize(cw->shobj, cw->pw, cw->ph);
                _e_mod_comp_win_geometry_update(cw);
                _e_mod_comp_win_damage(cw, 0, 0, cw->w, cw->h, 0);
             }
           cw->drawme = 1;
           _e_mod_comp_win_render_queue(cw);
        }
   }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_shape(void *data __UNUSED__,
                  int type   __UNUSED__,
                  void *event)
{
   Ecore_X_Event_Window_Shape *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->win);
   if (!cw) return ECORE_CALLBACK_PASS_ON;
   if (ev->type != ECORE_X_SHAPE_BOUNDING) return ECORE_CALLBACK_PASS_ON;
   _e_mod_comp_win_reshape(cw);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_damage(void *data __UNUSED__,
                   int type   __UNUSED__,
                   void *event)
{
   Ecore_X_Event_Damage *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_damage_find(ev->damage);
   if (!cw) return ECORE_CALLBACK_PASS_ON;
   _e_mod_comp_win_damage(cw,
                          ev->area.x, ev->area.y,
                          ev->area.width, ev->area.height, 1);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_damage_win(void *data __UNUSED__,
                       int type   __UNUSED__,
                       void *event)
{
   Ecore_X_Event_Window_Damage *ev = event;
   Eina_List *l;
   E_Comp *c;

   // fixme: use hash if compositors list > 4
   EINA_LIST_FOREACH(compositors, l, c)
     {
        if (ev->win == c->ee_win)
          {
             // expose on comp win - init win or some other bypass win did it
             DBG("JOB4...\n");
             _e_mod_comp_render_queue(c);
             break;
          }
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_override_expire(void *data)
{
   E_Comp *c = data;

   c->nocomp_override_timer = NULL;
   c->nocomp_override--;
   
   if (c->nocomp_override <= 0)
     {
        c->nocomp_override = 0;
        if (c->nocomp_want) _e_mod_comp_cb_nocomp_begin(c);
     }
   return EINA_FALSE;
}

static void
_e_mod_comp_override_timed_pop(E_Comp *c)
{
   if (c->nocomp_override <= 0) return;
   if (c->nocomp_override_timer)
       ecore_timer_del(c->nocomp_override_timer);
   c->nocomp_override_timer =
     ecore_timer_add(5.0, _e_mod_comp_override_expire, c);
}

/* here for completeness
static void
_e_mod_comp_override_pop(E_Comp *c)
{
   c->nocomp_override--;
   if (c->nocomp_override <= 0)
     {
        c->nocomp_override = 0;
        if (c->nocomp_want) _e_mod_comp_cb_nocomp_begin(c);
     }
}
*/

static void
_e_mod_comp_override_push(E_Comp *c)
{
   c->nocomp_override++;
   if ((c->nocomp_override > 0) && (c->nocomp)) _e_mod_comp_cb_nocomp_end(c);
}

static void
_e_mod_comp_fade_handle(E_Comp_Zone *cz, int out, double tim)
{
   if (out == 1)
     {
        if (e_backlight_exists())
          {
             e_backlight_update();
             cz->bloff = EINA_TRUE;
             cz->bl = e_backlight_level_get(cz->zone);
             e_backlight_level_set(cz->zone, 0.0, tim);
          }
     }
   else
     {
        if (e_backlight_exists())
          {
             cz->bloff = EINA_FALSE;
             e_backlight_update();
             if (e_backlight_mode_get(cz->zone) != E_BACKLIGHT_MODE_NORMAL)
               e_backlight_mode_set(cz->zone, E_BACKLIGHT_MODE_NORMAL);
             else
               e_backlight_level_set(cz->zone, e_config->backlight.normal, tim);
          }
     }
}

static Eina_Bool
_e_mod_comp_screensaver_on(void *data __UNUSED__,
                           int type   __UNUSED__,
                           void *event __UNUSED__)
{
   Eina_List *l, *ll;
   E_Comp_Zone *cz;
   E_Comp *c;

   // fixme: use hash if compositors list > 4
   EINA_LIST_FOREACH(compositors, l, c)
     {
        if (!c->saver)
          {
             c->saver = EINA_TRUE;
             EINA_LIST_FOREACH(c->zones, ll, cz)
               {
                  _e_mod_comp_override_push(c);
                  _e_mod_comp_fade_handle(cz, 1, 3.0);
                  edje_object_signal_emit(cz->base, 
                                          "e,state,screensaver,on", "e");
                  edje_object_signal_emit(cz->over, 
                                          "e,state,screensaver,on", "e");
               }
          }
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_screensaver_off(void *data __UNUSED__,
                            int type   __UNUSED__,
                            void *event __UNUSED__)
{
   Eina_List *l, *ll;
   E_Comp_Zone *cz;
   E_Comp *c;

   // fixme: use hash if compositors list > 4
   EINA_LIST_FOREACH(compositors, l, c)
     {
        if (c->saver)
          {
             c->saver = EINA_FALSE;
             EINA_LIST_FOREACH(c->zones, ll, cz)
               {
                  edje_object_signal_emit(cz->base, 
                                          "e,state,screensaver,off", "e");
                  edje_object_signal_emit(cz->over, 
                                          "e,state,screensaver,off", "e");
                  _e_mod_comp_fade_handle(cz, 0, 0.5);
                  _e_mod_comp_override_timed_pop(c);
               }
          }
     }
   return ECORE_CALLBACK_PASS_ON;
}

static void
_e_mod_comp_zone_fill(E_Comp *c, E_Comp_Zone *cz)
{
   Evas_Object *o;
   
   cz->base = o = edje_object_add(c->evas);
   e_theme_edje_object_set(o, "base/theme/modules/comp",
                           "e/modules/comp/screen/base/default");
   evas_object_move(o, cz->zone->x, cz->zone->y);
   evas_object_resize(o, cz->zone->w, cz->zone->h);
   evas_object_lower(o);
   evas_object_show(o);

   cz->over = o = edje_object_add(c->evas);
   e_theme_edje_object_set(o, "base/theme/modules/comp",
                           "e/modules/comp/screen/overlay/default");
   evas_object_move(o, cz->zone->x, cz->zone->y);
   evas_object_resize(o, cz->zone->w, cz->zone->h);
   evas_object_raise(o);
   evas_object_show(o);
}

static void
_e_mod_comp_screens_eval(E_Comp *c)
{
   Eina_List *l, *ll;
   E_Container *con;
   E_Zone *zone;
   E_Comp_Zone *cz;
   int zn, cn;

   EINA_LIST_FREE(c->zones, cz)
     {
        evas_object_del(cz->base);
        evas_object_del(cz->over);
        if (cz->bloff)
          {
             if (e_backlight_mode_get(cz->zone) != E_BACKLIGHT_MODE_NORMAL)
               e_backlight_mode_set(cz->zone, E_BACKLIGHT_MODE_NORMAL);
             e_backlight_level_set(cz->zone, e_config->backlight.normal, 0.0);
          }
        free(cz);
     }
   cn = 0;
   EINA_LIST_FOREACH(c->man->containers, l, con)
     {
        zn = 0;
        EINA_LIST_FOREACH(con->zones, ll, zone)
          {
             cz = calloc(1, sizeof(E_Comp_Zone));
             if (cz)
               {
                  cz->zone = zone;
                  cz->container_num = cn;
                  cz->zone_num = zn;
                  cz->x = zone->x;
                  cz->y = zone->y;
                  cz->w = zone->w;
                  cz->h = zone->h;
                  _e_mod_comp_zone_fill(c, cz);
                  c->zones = eina_list_append(c->zones, cz);
               }
             zn++;
          }
        cn++;
     }
   e_layout_freeze(c->layout);
   evas_object_move(c->layout, 0, 0);
   evas_object_resize(c->layout, c->man->w, c->man->h);
   e_layout_virtual_size_set(c->layout, c->man->w, c->man->h);
   e_layout_thaw(c->layout);
}

static void
_e_mod_comp_screen_change(void *data)
{
   E_Comp *c = data;
   
   c->screen_job = NULL;
   if (!c->nocomp) ecore_evas_resize(c->ee, c->man->w, c->man->h);
   _e_mod_comp_screens_eval(c);
}

static Eina_Bool
_e_mod_comp_randr(void *data       __UNUSED__,
                  int type         __UNUSED__,
                  __UNUSED__ void *event)
{
   Eina_List *l;
   E_Comp *c;

   EINA_LIST_FOREACH(compositors, l, c)
     {
        if (c->screen_job) ecore_job_del(c->screen_job);
        c->screen_job = ecore_job_add(_e_mod_comp_screen_change, c);
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_zonech(void *data       __UNUSED__,
                   int type         __UNUSED__,
                   __UNUSED__ void *event)
{
   Eina_List *l;
   E_Comp *c;

   EINA_LIST_FOREACH(compositors, l, c)
     {
        if (c->screen_job) ecore_job_del(c->screen_job);
        c->screen_job = ecore_job_add(_e_mod_comp_screen_change, c);
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_bd_add(void *data __UNUSED__,
                   int type   __UNUSED__,
                   void *event)
{
   E_Event_Border_Add *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->border->win);
   if (!cw) return ECORE_CALLBACK_PASS_ON;
   // fimxe: add/enable compositing here not in show event for borders
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_bd_del(void *data __UNUSED__,
                   int type   __UNUSED__,
                   void *event)
{
   E_Event_Border_Remove *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->border->win);
   if (!cw) return ECORE_CALLBACK_PASS_ON;
   if (cw->bd == ev->border) _e_mod_comp_object_del(cw, ev->border);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_bd_show(void *data __UNUSED__,
                    int type   __UNUSED__,
                    void *event)
{
   E_Event_Border_Show *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->border->win);
   if (!cw) return ECORE_CALLBACK_PASS_ON;
   if (cw->visible) return ECORE_CALLBACK_PASS_ON;
   _e_mod_comp_win_show(cw);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_bd_hide(void *data __UNUSED__,
                    int type   __UNUSED__,
                    void *event)
{
   E_Event_Border_Hide *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->border->win);
   if (!cw) return ECORE_CALLBACK_PASS_ON;
   if (!cw->visible) return ECORE_CALLBACK_PASS_ON;
   _e_mod_comp_win_hide(cw);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_bd_move(void *data __UNUSED__,
                    int type   __UNUSED__,
                    void *event)
{
   E_Event_Border_Move *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->border->win);
   if (!cw) return ECORE_CALLBACK_PASS_ON;
   // fimxe: do move here for composited bd
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_bd_resize(void *data __UNUSED__,
                      int type   __UNUSED__,
                      void *event)
{
   E_Event_Border_Resize *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->border->win);
   if (!cw) return ECORE_CALLBACK_PASS_ON;
   // fimxe: do resize here instead of conf notify
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_bd_iconify(void *data __UNUSED__,
                       int type   __UNUSED__,
                       void *event)
{
   E_Event_Border_Iconify *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->border->win);
   if (!cw) return ECORE_CALLBACK_PASS_ON;
   // fimxe: special iconfiy anim
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_bd_uniconify(void *data __UNUSED__,
                         int type   __UNUSED__,
                         void *event)
{
   E_Event_Border_Uniconify *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->border->win);
   if (!cw) return ECORE_CALLBACK_PASS_ON;
   // fimxe: special uniconfiy anim
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_bd_urgent_change(void *data __UNUSED__,
                             int type   __UNUSED__,
                             void *event)
{
   E_Event_Border_Urgent_Change *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->border->win);
   if (!cw) return ECORE_CALLBACK_PASS_ON;
   if (cw->bd->client.icccm.urgent)
     edje_object_signal_emit(cw->shobj, "e,state,urgent,on", "e");
   else
     edje_object_signal_emit(cw->shobj, "e,state,urgent,off", "e");
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_bd_focus_in(void *data __UNUSED__,
                        int type   __UNUSED__,
                        void *event)
{
   E_Event_Border_Focus_In *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->border->win);
   if (!cw) return ECORE_CALLBACK_PASS_ON;
   edje_object_signal_emit(cw->shobj, "e,state,focus,on", "e");
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_bd_focus_out(void *data __UNUSED__,
                         int type   __UNUSED__,
                         void *event)
{
   E_Event_Border_Focus_Out *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->border->win);
   if (!cw) return ECORE_CALLBACK_PASS_ON;
   edje_object_signal_emit(cw->shobj, "e,state,focus,off", "e");
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_bd_property(void *data __UNUSED__,
                        int type   __UNUSED__,
                        void *event)
{
   E_Event_Border_Property *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->border->win);
   if (!cw) return ECORE_CALLBACK_PASS_ON;
   // fimxe: other properties?
   return ECORE_CALLBACK_PASS_ON;
}

static void
_e_mod_comp_reshadow(E_Comp_Win *cw)
{
   if (cw->visible) evas_object_hide(cw->shobj);
   _e_mod_comp_win_shadow_setup(cw);
//   evas_object_move(cw->shobj, cw->x, cw->y);
//   evas_object_resize(cw->shobj, cw->pw, cw->ph);
   _e_mod_comp_win_geometry_update(cw);
   if (cw->visible)
     {
        evas_object_show(cw->shobj);
        if (cw->show_ready)
          {
             cw->defer_hide = 0;
             if (!cw->hidden_override) _e_mod_comp_child_show(cw);
             edje_object_signal_emit(cw->shobj, "e,state,visible,on", "e");
             if (!cw->animating)
               {
                  cw->c->animating++;
               }
             cw->animating = 1;
             _e_mod_comp_win_render_queue(cw);
          }
     }
}

static Eina_Bool
_e_mod_comp_bd_fullscreen(void *data __UNUSED__,
                          int type   __UNUSED__,
                          void *event)
{
   E_Event_Border_Property *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->border->win);
   if (!cw) return ECORE_CALLBACK_PASS_ON;
   _e_mod_comp_reshadow(cw);
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_bd_unfullscreen(void *data __UNUSED__,
                          int type   __UNUSED__,
                          void *event)
{
   E_Event_Border_Property *ev = event;
   E_Comp_Win *cw = _e_mod_comp_win_find(ev->border->win);
   if (!cw) return ECORE_CALLBACK_PASS_ON;
   _e_mod_comp_reshadow(cw);
   return ECORE_CALLBACK_PASS_ON;
}

//////////////////////////////////////////////////////////////////////////
static void
_e_mod_comp_fps_toggle(void)
{
   if (_comp_mod)
     {
        Eina_List *l;
        E_Comp *c;

        if (_comp_mod->conf->fps_show)
          {
             _comp_mod->conf->fps_show = 0;
             e_config_save_queue();
          }
        else
          {
             _comp_mod->conf->fps_show = 1;
             e_config_save_queue();
          }
        EINA_LIST_FOREACH(compositors, l, c) _e_mod_comp_cb_update(c);
     }
}

static Eina_Bool
_e_mod_comp_key_down(void *data __UNUSED__,
                     int type   __UNUSED__,
                     void *event)
{
   Ecore_Event_Key *ev = event;

   if ((!strcmp(ev->key, "Home")) &&
       (ev->modifiers & ECORE_EVENT_MODIFIER_SHIFT) &&
       (ev->modifiers & ECORE_EVENT_MODIFIER_CTRL) &&
       (ev->modifiers & ECORE_EVENT_MODIFIER_ALT))
     {
        if (_comp_mod)
          {
             _e_mod_config_free(_comp_mod->module);
             _e_mod_config_new(_comp_mod->module);
             e_config_save();
             e_module_disable(_comp_mod->module);
             e_config_save();
             e_sys_action_do(E_SYS_RESTART, NULL);
          }
     }
   else if ((!strcasecmp(ev->key, "f")) &&
            (ev->modifiers & ECORE_EVENT_MODIFIER_SHIFT) &&
            (ev->modifiers & ECORE_EVENT_MODIFIER_CTRL) &&
            (ev->modifiers & ECORE_EVENT_MODIFIER_ALT))
     {
        _e_mod_comp_fps_toggle();
     }
   return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool
_e_mod_comp_signal_user(void *data __UNUSED__,
                        int type   __UNUSED__,
                        void *event)
{
   Ecore_Event_Signal_User *ev = event;

   if (ev->number == 1)
     {
        // e17 uses this to pop up config panel
     }
   else if (ev->number == 2)
     {
        _e_mod_comp_fps_toggle();
     }
   return ECORE_CALLBACK_PASS_ON;
}

//////////////////////////////////////////////////////////////////////////
static Evas *
_e_mod_comp_evas_get_func(void *data,
                          E_Manager *man __UNUSED__)
{
   E_Comp *c = data;
   return c->evas;
}

static void
_e_mod_comp_update_func(void *data,
                        E_Manager *man __UNUSED__)
{
   E_Comp *c = data;
   _e_mod_comp_render_queue(c);
}

static E_Manager_Comp_Source *
_e_mod_comp_border_src_get_func(void *data __UNUSED__,
                         E_Manager *man __UNUSED__,
                         Ecore_X_Window win)
{
   return (E_Manager_Comp_Source *)_e_mod_comp_border_client_find(win);
}

static E_Manager_Comp_Source *
_e_mod_comp_src_get_func(void *data __UNUSED__,
                         E_Manager *man __UNUSED__,
                         Ecore_X_Window win)
{
   return (E_Manager_Comp_Source *)_e_mod_comp_win_find(win);
}

static const Eina_List *
_e_mod_comp_src_list_get_func(void *data,
                              E_Manager *man __UNUSED__)
{
   E_Comp *c = data;
   E_Comp_Win *cw;

   if (!c->wins) return NULL;
   if (c->wins_invalid)
     {
        c->wins_invalid = 0;
        if (c->wins_list) eina_list_free(c->wins_list);
        c->wins_list = NULL;
        EINA_INLIST_FOREACH(c->wins, cw)
          {
             if ((cw->shobj) && (cw->obj))
               c->wins_list = eina_list_append(c->wins_list, cw);
          }
     }
   return c->wins_list;
}

static Evas_Object *
_e_mod_comp_src_image_get_func(void *data             __UNUSED__,
                               E_Manager *man         __UNUSED__,
                               E_Manager_Comp_Source *src)
{
//   E_Comp *c = data;
   E_Comp_Win *cw = (E_Comp_Win *)src;
   if (!cw->c) return NULL;
   return cw->obj;
}

static Evas_Object *
_e_mod_comp_src_shadow_get_func(void *data             __UNUSED__,
                                E_Manager *man         __UNUSED__,
                                E_Manager_Comp_Source *src)
{
//   E_Comp *c = data;
   E_Comp_Win *cw = (E_Comp_Win *)src;
   if (!cw->c) return NULL;
   return cw->shobj;
}

static Evas_Object *
_e_mod_comp_src_image_mirror_add_func(void *data             __UNUSED__,
                                      E_Manager *man         __UNUSED__,
                                      E_Manager_Comp_Source *src)
{
//   E_Comp *c = data;
   E_Comp_Win *cw = (E_Comp_Win *)src;
   if (!cw->c) return NULL;
   return _e_mod_comp_win_mirror_add(cw);
}

static Eina_Bool
_e_mod_comp_src_visible_get_func(void *data             __UNUSED__,
                                 E_Manager *man         __UNUSED__,
                                 E_Manager_Comp_Source *src)
{
//   E_Comp *c = data;
   E_Comp_Win *cw = (E_Comp_Win *)src;
   if (!cw->c) return 0;
   return cw->visible;
}

static void
_e_mod_comp_src_hidden_set_func(void *data             __UNUSED__,
                                E_Manager *man         __UNUSED__,
                                E_Manager_Comp_Source *src,
                                Eina_Bool hidden)
{
//   E_Comp *c = data;
   E_Comp_Win *cw = (E_Comp_Win *)src;
   if (!cw->c) return;
   if (cw->hidden_override == hidden) return;
   cw->hidden_override = hidden;
   if (cw->bd) e_border_comp_hidden_set(cw->bd, cw->hidden_override);
   if (cw->visible)
     {
        if (cw->hidden_override)
          _e_mod_comp_child_hide(cw);
        else if (!cw->bd || cw->bd->visible)
          _e_mod_comp_child_show(cw);
     }
   else
     {
        if (cw->hidden_override) _e_mod_comp_child_hide(cw);
     }
}

static Eina_Bool
_e_mod_comp_src_hidden_get_func(void *data             __UNUSED__,
                                E_Manager *man         __UNUSED__,
                                E_Manager_Comp_Source *src)
{
//   E_Comp *c = data;
   E_Comp_Win *cw = (E_Comp_Win *)src;
   if (!cw->c) return 0;
   return cw->hidden_override;
}

static E_Popup *
_e_mod_comp_src_popup_get_func(void *data             __UNUSED__,
                               E_Manager *man         __UNUSED__,
                               E_Manager_Comp_Source *src)
{
   //   E_Comp *c = data;
   E_Comp_Win *cw = (E_Comp_Win *)src;
   if (!cw->c) return 0;
   return cw->pop;
}

static E_Border *
_e_mod_comp_src_border_get_func(void *data             __UNUSED__,
                                E_Manager *man         __UNUSED__,
                                E_Manager_Comp_Source *src)
{
   //   E_Comp *c = data;
   E_Comp_Win *cw = (E_Comp_Win *)src;
   if (!cw->c) return 0;
   return cw->bd;
}

static Ecore_X_Window
_e_mod_comp_src_window_get_func(void *data             __UNUSED__,
                                E_Manager *man         __UNUSED__,
                                E_Manager_Comp_Source *src)
{
   //   E_Comp *c = data;
   E_Comp_Win *cw = (E_Comp_Win *)src;
   if (!cw->c) return 0;
   return cw->win;
}

static E_Comp *
_e_mod_comp_add(E_Manager *man)
{
   E_Comp *c;
   Ecore_X_Window *wins;
   Ecore_X_Window_Attributes att;
   Eina_Bool res;
   int i, num;

   c = calloc(1, sizeof(E_Comp));
   if (!c) return NULL;

   res = ecore_x_screen_is_composited(man->num);
   if (res)
     {
        e_util_dialog_internal
          (_("Compositor Error"),
          _("Another compositor is already running<br>"
            "on your display server."));
        free(c);
        return NULL;
     }

   c->cm_selection = ecore_x_window_input_new(man->root, 0, 0, 1, 1);
   if (!c->cm_selection)
     {
        free(c);
        return NULL;
     }
   ecore_x_screen_is_composited_set(man->num, c->cm_selection);

   ecore_x_e_comp_sync_supported_set(man->root, _comp_mod->conf->efl_sync);

   c->man = man;
   c->win = ecore_x_composite_render_window_enable(man->root);
   if (!c->win)
     {
        e_util_dialog_internal
          (_("Compositor Error"),
          _("Your display server does not support the<br>"
            "compositor overlay window. This is needed<br>"
            "for it to function."));
        free(c);
        return NULL;
     }

   memset((&att), 0, sizeof(Ecore_X_Window_Attributes));
   ecore_x_window_attributes_get(c->win, &att);

   if ((att.depth != 24) && (att.depth != 32))
     {
/*        
        e_util_dialog_internal
          (_("Compositor Error"),
          _("Your screen is not in 24/32bit display mode.<br>"
            "This is required to be your default depth<br>"
            "setting for the compositor to work properly."));
        ecore_x_composite_render_window_disable(c->win);
        free(c);
        return NULL;
 */
     }

   if (c->man->num == 0) e_alert_composite_win(c->man->root, c->win);

   if (_comp_mod->conf->engine == ENGINE_GL)
     {
        int opt[20];
        int opt_i = 0;

        if (_comp_mod->conf->indirect)
          {
             opt[opt_i] = ECORE_EVAS_GL_X11_OPT_INDIRECT;
             opt_i++;
             opt[opt_i] = 1;
             opt_i++;
          }
        if (_comp_mod->conf->vsync)
          {
             opt[opt_i] = ECORE_EVAS_GL_X11_OPT_VSYNC;
             opt_i++;
             opt[opt_i] = 1;
             opt_i++;
          }
#ifdef ECORE_EVAS_GL_X11_OPT_SWAP_MODE
        if (_comp_mod->conf->swap_mode)
          {
             opt[opt_i] = ECORE_EVAS_GL_X11_OPT_SWAP_MODE;
             opt_i++;
             opt[opt_i] = _comp_mod->conf->swap_mode;
             opt_i++;
          }
#endif        
        if (opt_i > 0)
          {
             opt[opt_i] = ECORE_EVAS_GL_X11_OPT_NONE;
             c->ee = ecore_evas_gl_x11_options_new
                 (NULL, c->win, 0, 0, man->w, man->h, opt);
          }
        if (!c->ee)
          c->ee = ecore_evas_gl_x11_new(NULL, c->win, 0, 0, man->w, man->h);
        if (c->ee)
          {
             c->gl = 1;
             ecore_evas_gl_x11_pre_post_swap_callback_set
               (c->ee, c, _e_mod_comp_pre_swap, NULL);
          }
     }
   if (!c->ee)
     {
        if (_comp_mod->conf->engine == ENGINE_GL)
          {
             e_util_dialog_internal
               (_("Compositor Warning"),
               _("Your display driver does not support OpenGL, or<br>"
                 "no OpenGL engines were compiled or installed for<br>"
                 "Evas or Ecore-Evas. Falling back to software engine."));
          }

        c->ee = ecore_evas_software_x11_new(NULL, c->win, 0, 0, man->w, man->h);
     }

   ecore_evas_comp_sync_set(c->ee, 0);
//   ecore_evas_manual_render_set(c->ee, _comp_mod->conf->lock_fps);
   c->evas = ecore_evas_get(c->ee);
   ecore_evas_show(c->ee);

   c->layout = e_layout_add(c->evas);
   evas_object_show(c->layout);

   _e_mod_comp_screens_eval(c);
   
   c->ee_win = ecore_evas_window_get(c->ee);
   ecore_x_composite_redirect_subwindows
     (c->man->root, ECORE_X_COMPOSITE_UPDATE_MANUAL);

   wins = ecore_x_window_children_get(c->man->root, &num);
   if (wins)
     {
        for (i = 0; i < num; i++)
          {
             E_Comp_Win *cw;
             int x, y, w, h, border;
             char *wname = NULL, *wclass = NULL;

             ecore_x_icccm_name_class_get(wins[i], &wname, &wclass);
             if ((man->initwin == wins[i]) ||
                 ((wname) && (wclass) && (!strcmp(wname, "E")) &&
                  (!strcmp(wclass, "Init_Window"))))
               {
                  free(wname);
                  free(wclass);
                  ecore_x_window_reparent(wins[i], c->win, 0, 0);
                  ecore_x_sync();
                  continue;
               }
             free(wname);
             free(wclass);
             wname = wclass = NULL;
             cw = _e_mod_comp_win_add(c, wins[i]);
             if (!cw) continue;
             ecore_x_window_geometry_get(cw->win, &x, &y, &w, &h);
             border = ecore_x_window_border_width_get(cw->win);
             if (wins[i] == c->win) continue;
             _e_mod_comp_win_configure(cw, x, y, w, h, border);
             if (ecore_x_window_visible_get(wins[i]))
               _e_mod_comp_win_show(cw);
          }
        free(wins);
     }

   ecore_x_window_key_grab(c->man->root,
                           "Home",
                           ECORE_EVENT_MODIFIER_SHIFT |
                           ECORE_EVENT_MODIFIER_CTRL |
                           ECORE_EVENT_MODIFIER_ALT, 0);
   ecore_x_window_key_grab(c->man->root,
                           "F",
                           ECORE_EVENT_MODIFIER_SHIFT |
                           ECORE_EVENT_MODIFIER_CTRL |
                           ECORE_EVENT_MODIFIER_ALT, 0);

   c->comp.data = c;
   c->comp.func.evas_get = _e_mod_comp_evas_get_func;
   c->comp.func.update = _e_mod_comp_update_func;
   c->comp.func.src_get = _e_mod_comp_src_get_func;
   c->comp.func.border_src_get = _e_mod_comp_border_src_get_func;
   c->comp.func.src_list_get = _e_mod_comp_src_list_get_func;
   c->comp.func.src_image_get = _e_mod_comp_src_image_get_func;
   c->comp.func.src_shadow_get = _e_mod_comp_src_shadow_get_func;
   c->comp.func.src_image_mirror_add = _e_mod_comp_src_image_mirror_add_func;
   c->comp.func.src_visible_get = _e_mod_comp_src_visible_get_func;
   c->comp.func.src_hidden_set = _e_mod_comp_src_hidden_set_func;
   c->comp.func.src_hidden_get = _e_mod_comp_src_hidden_get_func;
   c->comp.func.src_window_get = _e_mod_comp_src_window_get_func;
   c->comp.func.src_border_get = _e_mod_comp_src_border_get_func;
   c->comp.func.src_popup_get = _e_mod_comp_src_popup_get_func;

   e_manager_comp_set(c->man, &(c->comp));
   return c;
}

static void
_e_mod_comp_del(E_Comp *c)
{
   E_Comp_Win *cw;
   E_Comp_Zone *cz;
   Eina_List *l, *hide_bd = NULL;
   E_Border *bd;
   
   EINA_LIST_FOREACH(e_border_client_list(), l, bd)
     {
        if (!bd->visible)
          hide_bd = eina_list_append(hide_bd, bd);
     }
   
   if (c->fps_fg)
     {
        evas_object_del(c->fps_fg);
        c->fps_fg = NULL;
     }
   if (c->fps_bg)
     {
        evas_object_del(c->fps_bg);
        c->fps_bg = NULL;
     }
   e_manager_comp_set(c->man, NULL);

   ecore_x_window_key_ungrab(c->man->root,
                             "F",
                             ECORE_EVENT_MODIFIER_SHIFT |
                             ECORE_EVENT_MODIFIER_CTRL |
                             ECORE_EVENT_MODIFIER_ALT, 0);
   ecore_x_window_key_ungrab(c->man->root,
                             "Home",
                             ECORE_EVENT_MODIFIER_SHIFT |
                             ECORE_EVENT_MODIFIER_CTRL |
                             ECORE_EVENT_MODIFIER_ALT, 0);
   if (c->grabbed)
     {
        c->grabbed = 0;
        ecore_x_ungrab();
     }
   while (c->wins)
     {
        cw = (E_Comp_Win *)(c->wins);
        if (cw->counter)
          {
             ecore_x_sync_counter_free(cw->counter);
             cw->counter = 0;
          }
        cw->force = 1;
        _e_mod_comp_win_hide(cw);
        cw->force = 1;
        _e_mod_comp_win_del(cw);
     }
   
   EINA_LIST_FREE(c->zones, cz)
     {
        evas_object_del(cz->base);
        evas_object_del(cz->over);
        if (cz->bloff)
          {
             if (e_backlight_mode_get(cz->zone) != E_BACKLIGHT_MODE_NORMAL)
               e_backlight_mode_set(cz->zone, E_BACKLIGHT_MODE_NORMAL);
             e_backlight_level_set(cz->zone, e_config->backlight.normal, 0.0);
          }
        free(cz);
     }

   if (c->layout) evas_object_del(c->layout);
   
   ecore_evas_free(c->ee);
   ecore_x_composite_unredirect_subwindows
     (c->man->root, ECORE_X_COMPOSITE_UPDATE_MANUAL);
   ecore_x_composite_render_window_disable(c->win);
   if (c->man->num == 0) e_alert_composite_win(c->man->root, 0);
   if (c->render_animator) ecore_animator_del(c->render_animator);
   if (c->new_up_timer) ecore_timer_del(c->new_up_timer);
   if (c->update_job) ecore_job_del(c->update_job);
   if (c->wins_list) eina_list_free(c->wins_list);
   if (c->screen_job) ecore_job_del(c->screen_job);
   if (c->nocomp_delay_timer) ecore_timer_del(c->nocomp_delay_timer);
   if (c->nocomp_override_timer) ecore_timer_del(c->nocomp_override_timer);

   ecore_x_window_free(c->cm_selection);
   ecore_x_e_comp_sync_supported_set(c->man->root, 0);
   ecore_x_screen_is_composited_set(c->man->num, 0);
   
   EINA_LIST_FREE(hide_bd, bd)
     {
        e_border_show(bd);
        e_border_hide(bd, 1);
     }

   free(c);
}

//////////////////////////////////////////////////////////////////////////


static void
_e_mod_comp_sys_done_cb(void *data, Evas_Object *obj, const char *sig, const char *src)
{
   edje_object_signal_callback_del(obj, sig, src, _e_mod_comp_sys_done_cb);
   e_sys_action_raw_do((E_Sys_Action)(long)data, NULL);
}

static void
_e_mod_comp_sys_emit_cb_wait(E_Sys_Action a, const char *sig, const char *rep, Eina_Bool nocomp_push)
{
   Eina_List *l, *ll;
   E_Comp_Zone *cz;
   E_Comp *c;
   Eina_Bool first = EINA_TRUE;
   
   EINA_LIST_FOREACH(compositors, l, c)
     {
        if (nocomp_push) _e_mod_comp_override_push(c);
        else _e_mod_comp_override_timed_pop(c);
        EINA_LIST_FOREACH(c->zones, ll, cz)
          {
             if (nocomp_push) _e_mod_comp_fade_handle(cz, 1, 0.5);
             else _e_mod_comp_fade_handle(cz, 0, 0.5);
             edje_object_signal_emit(cz->base, sig, "e");
             edje_object_signal_emit(cz->over, sig, "e");
             if ((rep) && (first))
               edje_object_signal_callback_add(cz->over, rep, "e",
                                               _e_mod_comp_sys_done_cb,
                                               (void *)(long)a);
             first = EINA_FALSE;
          }
     }
}

static void
_e_mod_comp_sys_suspend(void)
{
   _e_mod_comp_sys_emit_cb_wait(E_SYS_SUSPEND,
                                "e,state,sys,suspend",
                                "e,state,sys,suspend,done",
                                EINA_TRUE);
}

static void
_e_mod_comp_sys_hibernate(void)
{
   _e_mod_comp_sys_emit_cb_wait(E_SYS_HIBERNATE,
                                "e,state,sys,hibernate",
                                "e,state,sys,hibernate,done",
                                EINA_TRUE);
}

static void
_e_mod_comp_sys_reboot(void)
{
   _e_mod_comp_sys_emit_cb_wait(E_SYS_REBOOT,
                                "e,state,sys,reboot",
                                "e,state,sys,reboot,done",
                                EINA_TRUE);
}

static void
_e_mod_comp_sys_shutdown(void)
{
   _e_mod_comp_sys_emit_cb_wait(E_SYS_HALT,
                                "e,state,sys,halt",
                                "e,state,sys,halt,done",
                                EINA_TRUE);
}

static void
_e_mod_comp_sys_logout(void)
{
   _e_mod_comp_sys_emit_cb_wait(E_SYS_LOGOUT,
                                "e,state,sys,logout",
                                "e,state,sys,logout,done",
                                EINA_TRUE);
}

static void
_e_mod_comp_sys_resume(void)
{
   _e_mod_comp_sys_emit_cb_wait(E_SYS_SUSPEND,
                                "e,state,sys,resume",
                                NULL,
                                EINA_FALSE);
}

//////////////////////////////////////////////////////////////////////////

Eina_Bool
e_mod_comp_init(void)
{
   Eina_List *l;
   E_Manager *man;
   
   e_sys_handlers_set(_e_mod_comp_sys_suspend,
                      _e_mod_comp_sys_hibernate,
                      _e_mod_comp_sys_reboot,
                      _e_mod_comp_sys_shutdown,
                      _e_mod_comp_sys_logout,
                      _e_mod_comp_sys_resume);

   windows = eina_hash_string_superfast_new(NULL);
   borders = eina_hash_string_superfast_new(NULL);
   damages = eina_hash_string_superfast_new(NULL);

   E_LIST_HANDLER_APPEND(handlers, ECORE_X_EVENT_WINDOW_CREATE, _e_mod_comp_create, NULL);
   E_LIST_HANDLER_APPEND(handlers, ECORE_X_EVENT_WINDOW_DESTROY, _e_mod_comp_destroy, NULL);
   E_LIST_HANDLER_APPEND(handlers, ECORE_X_EVENT_WINDOW_SHOW, _e_mod_comp_show, NULL);
   E_LIST_HANDLER_APPEND(handlers, ECORE_X_EVENT_WINDOW_HIDE, _e_mod_comp_hide, NULL);
   E_LIST_HANDLER_APPEND(handlers, ECORE_X_EVENT_WINDOW_REPARENT, _e_mod_comp_reparent, NULL);
   E_LIST_HANDLER_APPEND(handlers, ECORE_X_EVENT_WINDOW_CONFIGURE, _e_mod_comp_configure, NULL);
   E_LIST_HANDLER_APPEND(handlers, ECORE_X_EVENT_WINDOW_STACK, _e_mod_comp_stack, NULL);
   E_LIST_HANDLER_APPEND(handlers, ECORE_X_EVENT_WINDOW_PROPERTY, _e_mod_comp_property, NULL);
   E_LIST_HANDLER_APPEND(handlers, ECORE_X_EVENT_CLIENT_MESSAGE, _e_mod_comp_message, NULL);
   E_LIST_HANDLER_APPEND(handlers, ECORE_X_EVENT_WINDOW_SHAPE, _e_mod_comp_shape, NULL);
   E_LIST_HANDLER_APPEND(handlers, ECORE_X_EVENT_DAMAGE_NOTIFY, _e_mod_comp_damage, NULL);
   E_LIST_HANDLER_APPEND(handlers, ECORE_X_EVENT_WINDOW_DAMAGE, _e_mod_comp_damage_win, NULL);
   
   E_LIST_HANDLER_APPEND(handlers, E_EVENT_SCREENSAVER_ON, _e_mod_comp_screensaver_on, NULL);
   E_LIST_HANDLER_APPEND(handlers, E_EVENT_SCREENSAVER_OFF, _e_mod_comp_screensaver_off, NULL);

   E_LIST_HANDLER_APPEND(handlers, ECORE_EVENT_KEY_DOWN, _e_mod_comp_key_down, NULL);
   E_LIST_HANDLER_APPEND(handlers, ECORE_EVENT_SIGNAL_USER, _e_mod_comp_signal_user, NULL);

   E_LIST_HANDLER_APPEND(handlers, E_EVENT_CONTAINER_RESIZE, _e_mod_comp_randr, NULL);

   E_LIST_HANDLER_APPEND(handlers, E_EVENT_ZONE_MOVE_RESIZE, _e_mod_comp_zonech, NULL);
   E_LIST_HANDLER_APPEND(handlers, E_EVENT_ZONE_ADD, _e_mod_comp_zonech, NULL);
   E_LIST_HANDLER_APPEND(handlers, E_EVENT_ZONE_DEL, _e_mod_comp_zonech, NULL);
   
   E_LIST_HANDLER_APPEND(handlers, E_EVENT_BORDER_ADD, _e_mod_comp_bd_add, NULL);
   E_LIST_HANDLER_APPEND(handlers, E_EVENT_BORDER_REMOVE, _e_mod_comp_bd_del, NULL);
   E_LIST_HANDLER_APPEND(handlers, E_EVENT_BORDER_SHOW, _e_mod_comp_bd_show, NULL);
   E_LIST_HANDLER_APPEND(handlers, E_EVENT_BORDER_HIDE, _e_mod_comp_bd_hide, NULL);
   E_LIST_HANDLER_APPEND(handlers, E_EVENT_BORDER_MOVE, _e_mod_comp_bd_move, NULL);
   E_LIST_HANDLER_APPEND(handlers, E_EVENT_BORDER_RESIZE, _e_mod_comp_bd_resize, NULL);
   E_LIST_HANDLER_APPEND(handlers, E_EVENT_BORDER_ICONIFY, _e_mod_comp_bd_iconify, NULL);
   E_LIST_HANDLER_APPEND(handlers, E_EVENT_BORDER_UNICONIFY, _e_mod_comp_bd_uniconify, NULL);
   E_LIST_HANDLER_APPEND(handlers, E_EVENT_BORDER_URGENT_CHANGE, _e_mod_comp_bd_urgent_change, NULL);
   E_LIST_HANDLER_APPEND(handlers, E_EVENT_BORDER_FOCUS_IN, _e_mod_comp_bd_focus_in, NULL);
   E_LIST_HANDLER_APPEND(handlers, E_EVENT_BORDER_FOCUS_OUT, _e_mod_comp_bd_focus_out, NULL);
   E_LIST_HANDLER_APPEND(handlers, E_EVENT_BORDER_PROPERTY, _e_mod_comp_bd_property, NULL);
   E_LIST_HANDLER_APPEND(handlers, E_EVENT_BORDER_FULLSCREEN, _e_mod_comp_bd_fullscreen, NULL);
   E_LIST_HANDLER_APPEND(handlers, E_EVENT_BORDER_UNFULLSCREEN, _e_mod_comp_bd_unfullscreen, NULL);

   if (!ecore_x_composite_query())
     {
        e_util_dialog_internal
          (_("Compositor Error"),
          _("Your display server does not support XComposite,<br>"
            "or Ecore-X was built without XComposite support.<br>"
            "Note that for composite support you will also need<br>"
            "XRender and XFixes support in X11 and Ecore."));
        return 0;
     }
   if (!ecore_x_damage_query())
     {
        e_util_dialog_internal
          (_("Compositor Error"),
          _("Your display server does not support XDamage<br>"
            "or Ecore was built without XDamage support."));
        return 0;
     }

#ifdef HAVE_WAYLAND_CLIENTS
   if (!e_mod_comp_wl_init())
     EINA_LOG_ERR("Failed to initialize Wayland Client Support !!\n");
#endif

   EINA_LIST_FOREACH(e_manager_list(), l, man)
     {
        E_Comp *c;

        c = _e_mod_comp_add(man);
        if (c) compositors = eina_list_append(compositors, c);
     }

   ecore_x_sync();

   return 1;
}

void
e_mod_comp_shutdown(void)
{
   E_FREE_LIST(compositors, _e_mod_comp_del);
   E_FREE_LIST(handlers, ecore_event_handler_del);

#ifdef HAVE_WAYLAND_CLIENTS
   e_mod_comp_wl_shutdown();
#endif
   
   if (damages) eina_hash_free(damages);
   if (windows) eina_hash_free(windows);
   if (borders) eina_hash_free(borders);
   damages = NULL;
   windows = NULL;
   borders = NULL;
   
   e_sys_handlers_set(NULL, NULL, NULL, NULL, NULL, NULL);
}

void
e_mod_comp_shadow_set(void)
{
   Eina_List *l;
   E_Comp *c;

   EINA_LIST_FOREACH(compositors, l, c)
     {
        E_Comp_Win *cw;

//        ecore_evas_manual_render_set(c->ee, _comp_mod->conf->lock_fps);
        _e_mod_comp_fps_update(c);
        EINA_INLIST_FOREACH(c->wins, cw)
          {
             if ((cw->shobj) && (cw->obj))
               {
                  _e_mod_comp_win_shadow_setup(cw);

                  if (cw->visible)
                    {
                       edje_object_signal_emit(cw->shobj, "e,state,visible,on", "e");
                       if (!cw->animating)
                         {
                            cw->c->animating++;
                         }
                       _e_mod_comp_win_render_queue(cw);
                       cw->animating = 1;

                       cw->pending_count++;
                       e_manager_comp_event_src_visibility_send
                         (cw->c->man, (E_Manager_Comp_Source *)cw,
                         _e_mod_comp_cb_pending_after, cw->c);
                    }
               }
          }
     }
}

