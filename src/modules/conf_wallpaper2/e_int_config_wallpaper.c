#include "e.h"
#include "e_mod_main.h"

// FIXME:
//   need choice after add (file, gradient, online source)
//   need delete select mode
//   need after select on delete an ok/cancel if file or "ok to remove whole online source" if online
//   need to make "exchange" wallpapers have a different look
//   bug: animated wp doesn't workon first show
//   need to be able to "type name to search/filter"

typedef struct _Info Info;
typedef struct _Smart_Data Smart_Data;
typedef struct _Item Item;

struct _Info
{
   E_Win *win;
   Evas_Object *bg, *preview, *mini, *button, *box, *sframe, *span;
   char *bg_file;
   int iw, ih;
   Eina_List *dirs;
   char *curdir;
   Eina_Iterator *dir;
   Ecore_Idler *idler;
   int scans;
   int con_num, zone_num, desk_x, desk_y;
   int use_theme_bg;
   int mode;
};

struct _Smart_Data
{
   Eina_List *items;
   Ecore_Idle_Enterer *idle_enter;
   Ecore_Animator *animator;
   Ecore_Timer *seltimer;
   Info *info;
   Evas_Coord x, y, w, h;
   Evas_Coord cx, cy, cw, ch;
   Evas_Coord sx, sy;
   int id_num;
   int sort_num;
   double seltime;
   double selmove;
   Eina_Bool selin : 1;
   Eina_Bool selout : 1;
   Eina_Bool jump2hi : 1;
};

struct _Item
{
   Evas_Object *obj;
   Evas_Coord x, y, w, h;
   const char *file;
   char *sort_id;
   Evas_Object *frame, *image;
   Eina_Bool selected : 1;
   Eina_Bool have_thumb : 1;
   Eina_Bool do_thumb : 1;
   Eina_Bool remote : 1;
   Eina_Bool theme : 1;
   Eina_Bool visible : 1;
   Eina_Bool hilighted : 1;
};

static Info *global_info = NULL;

static void _e_smart_reconfigure(Evas_Object *obj);
static Eina_Bool _e_smart_reconfigure_do(void *data);
static void _thumb_gen(void *data, Evas_Object *obj, void *event_info);
static void _item_down(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _item_up(void *data, Evas *e, Evas_Object *obj, void *event_info);
static int _sort_cb(const void *d1, const void *d2);
static void _scan(Info *info);

static void
_pan_set(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   Smart_Data *sd = evas_object_smart_data_get(obj);

   if (x > (sd->cw - sd->w)) x = sd->cw - sd->w;
   if (y > (sd->ch - sd->h)) y = sd->ch - sd->h;
   if (x < 0) x = 0;
   if (y < 0) y = 0;
   if ((sd->cx == x) && (sd->cy == y)) return;
   sd->cx = x;
   sd->cy = y;
   _e_smart_reconfigure(obj);
}

static void
_pan_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y)
{
   Smart_Data *sd = evas_object_smart_data_get(obj);

   if (x) *x = sd->cx;
   if (y) *y = sd->cy;
}

static void
_pan_max_get(Evas_Object *obj, Evas_Coord *x, Evas_Coord *y)
{
   Smart_Data *sd = evas_object_smart_data_get(obj);

   if (x)
     {
        if (sd->w < sd->cw) *x = sd->cw - sd->w;
        else *x = 0;
     }
   if (y)
     {
        if (sd->h < sd->ch) *y = sd->ch - sd->h;
        else *y = 0;
     }
}

static void
_pan_child_size_get(Evas_Object *obj, Evas_Coord *w, Evas_Coord *h)
{
   Smart_Data *sd = evas_object_smart_data_get(obj);

   if (w) *w = sd->cw;
   if (h) *h = sd->ch;
}

static Eina_Bool
_e_smart_reconfigure_do(void *data)
{
   Evas_Object *obj = data;
   Smart_Data *sd = evas_object_smart_data_get(obj);
   Eina_List *l;
   Item *it;
   int iw, redo = 0, changed = 0;
   static int recursion = 0;
   Evas_Coord x, y, xx, yy, ww, hh, mw, mh, ox, oy, dd;
   Evas_Coord vw, vh;
   Evas *evas;

   if (!sd) return ECORE_CALLBACK_CANCEL;
   if (sd->cx > (sd->cw - sd->w)) sd->cx = sd->cw - sd->w;
   if (sd->cy > (sd->ch - sd->h)) sd->cy = sd->ch - sd->h;
   if (sd->cx < 0) sd->cx = 0;
   if (sd->cy < 0) sd->cy = 0;

   iw = sd->w / 4;
   if (iw > (120 * e_scale)) iw = (120 * e_scale);
   else
     {
        if (iw < (60 * e_scale)) iw = (sd->w / 3);
        if (iw < (60 * e_scale)) iw = (sd->w / 2);
        if (iw < (60 * e_scale)) iw = sd->w;
     }
   x = 0;
   y = 0;
   ww = iw;
   hh = (sd->info->ih * iw) / (sd->info->iw);
   mw = mh = 0;

   evas = evas_object_evas_get(obj);
   evas_output_viewport_get(evas, NULL, NULL, &vw, &vh);

   EINA_LIST_FOREACH(sd->items, l, it)
     {
        if (x > (sd->w - ww))
          {
             x = 0;
             y += hh;
          }
        it->x = x;
        it->y = y;
        it->w = ww;
        it->h = hh;
        if (it->selected)
          {
             sd->sx = it->x + (it->w / 2);
             sd->sy = it->y + (it->h / 2);
          }
        if ((x + ww) > mw) mw = x + ww;
        if ((y + hh) > mh) mh = y + hh;
        x += ww;
     }
   if ((mw != sd->cw) || (mh != sd->ch))
     {
        sd->cw = mw;
        sd->ch = mh;
        if (sd->cx > (sd->cw - sd->w))
          {
             sd->cx = sd->cw - sd->w;
             redo = 1;
          }
        if (sd->cy > (sd->ch - sd->h))
          {
             sd->cy = sd->ch - sd->h;
             redo = 1;
          }
        if (sd->cx < 0)
          {
             sd->cx = 0;
             redo = 1;
          }
        if (sd->cy < 0)
          {
             sd->cy = 0;
             redo = 1;
          }
        if (redo)
	  {
	     recursion = 1;
	     _e_smart_reconfigure_do(obj);
	     recursion = 0;
	  }
        changed = 1;
     }

   ox = 0;
   if (sd->w > sd->cw) ox = (sd->w - sd->cw) / 2;
   oy = 0;
   if (sd->h > sd->ch) oy = (sd->h - sd->ch) / 2;

   EINA_LIST_FOREACH(sd->items, l, it)
     {
        Evas_Coord dx, dy;

        dx = dy = 0;
        if ((sd->sx >= 0) && (sd->selmove > 0.0)

/*            &&
            ((it->x + it->w) > sd->cx) &&
            ((it->x) < (sd->cx + sd->w)) &&
            ((it->y + it->h) > sd->cy) &&
            ((it->y) < (sd->cy + sd->h))
 */
            )
          {
             double a, d;
             int sum = 0;
             char *p;

             // -----0X0+++++
             dx = (it->x + (it->w / 2)) - sd->sx;
             dy = (it->y + (it->h / 2)) - sd->sy;
             if (dx > 0)
               {
                  // |/
                  // +--
                  if (dy < 0)
                    a = -atan(-(double)dy / (double)dx);
                  // +--
                  /* |\ */
                  else
                    a = atan((double)dy / (double)dx);
               }
             else if (dx == 0)
               {
                  //   |
                  //   +
                  if (dy < 0) a = -M_PI / 2;
                  //   +
                  //   |
                  else a = M_PI / 2;
               }
             else
               {
                  //  \|
                  // --+
                  if (dy < 0)
                    a = -M_PI + atan((double)dy / (double)dx);
                  // --+
                  //  /|
                  else
                    a = M_PI - atan(-(double)dy / (double)dx);
               }
             d = sqrt((double)(dx * dx) + (double)(dy * dy));

             sum = 0;
             if (it->file)
               {
                  for (p = (char *)it->file; *p; p++) sum += (int)(*p);
               }
             sum = (sum & 0xff) - 128;
             a = a + ((double)sum / 1024.0);
             xx = sd->sx - sd->cx + ox;
             yy = sd->sy - sd->cy + oy;
             if (xx < (sd->w / 2)) dx = sd->w - xx;
             else dx = xx;
             if (yy < (sd->h / 2)) dy = sd->h - yy;
             else dy = yy;
             dd = dx - d;
             if (dy > dx) dd = dy - d;
             if (dd < 0) dd = 0;
             dy = sin(a) * sd->selmove * (dd * 0.9);
             dx = cos(a) * sd->selmove * (dd * 0.9);
          }
        xx = sd->x - sd->cx + it->x + ox;
        yy = sd->y - sd->cy + it->y + oy;
        if (E_INTERSECTS(xx, yy, it->w, it->h, 0, 0, vw, vh))
          {
             if (!it->have_thumb)
               {
                  if (!it->do_thumb)
                    {
                       e_thumb_icon_begin(it->image);
                       it->do_thumb = EINA_TRUE;
                    }
               }
             else
               {
                  if (!it->frame)
                    {
                       it->frame = edje_object_add(evas);
                       if (it->theme)
                         e_theme_edje_object_set(it->frame, "base/theme/widgets",
                                                 "e/conf/wallpaper/main/mini-theme");
                       else if (it->remote)
                         e_theme_edje_object_set(it->frame, "base/theme/widgets",
                                                 "e/conf/wallpaper/main/mini-remote");
                       else
                         e_theme_edje_object_set(it->frame, "base/theme/widgets",
                                                 "e/conf/wallpaper/main/mini");
                       if (it->hilighted)
                         {
                            edje_object_signal_emit(it->frame, "e,state,selected", "e");
                            evas_object_raise(it->frame);
                         }
                       evas_object_event_callback_add(it->frame, EVAS_CALLBACK_MOUSE_DOWN,
                                                      _item_down, it);
                       evas_object_event_callback_add(it->frame, EVAS_CALLBACK_MOUSE_UP,
                                                      _item_up, it);

                       evas_object_smart_member_add(it->frame, obj);
                       evas_object_clip_set(it->frame, evas_object_clip_get(obj));
                       it->image = e_thumb_icon_add(evas);
                       edje_object_part_swallow(it->frame, "e.swallow.content", it->image);
                       evas_object_smart_callback_add(it->image, "e_thumb_gen", _thumb_gen, it);
                       if (it->theme)
                         {
                            const char *f;

                            f = e_theme_edje_file_get("base/theme/backgrounds",
                                                      "e/desktop/background");
                            e_thumb_icon_file_set(it->image, f,
                                                  "e/desktop/background");
                         }
                       else
                         e_thumb_icon_file_set(it->image, it->file,
                                               "e/desktop/background");
                       e_thumb_icon_size_set(it->image, sd->info->iw,
                                             sd->info->ih);
                       evas_object_show(it->image);

                       e_thumb_icon_begin(it->image);
                    }
               }
             evas_object_move(it->frame, xx + dx, yy + dy);
             evas_object_resize(it->frame, it->w, it->h);
             evas_object_show(it->frame);
             it->visible = EINA_TRUE;
          }
        else
          {
             if (it->have_thumb)
               {
                  if (it->do_thumb)
                    {
                       e_thumb_icon_end(it->image);
                       it->do_thumb = EINA_FALSE;
                    }
                  evas_object_del(it->image);
                  it->image = NULL;
                  evas_object_del(it->frame);
                  it->frame = NULL;
               }
             it->visible = EINA_FALSE;
/*
             if (it->have_thumb)
               {
                  if (it->do_thumb)
                    {
                       e_thumb_icon_end(it->image);
                       it->do_thumb = EINA_FALSE;
                    }
                  evas_object_del(it->image);
                  it->have_thumb = EINA_FALSE;
                  it->image = e_thumb_icon_add(evas);
                  edje_object_part_swallow(it->frame, "e.swallow.content", it->image);
                  evas_object_smart_callback_add(it->image, "e_thumb_gen", _thumb_gen, it);
                  if (it->theme)
                    {
                       const char *f = e_theme_edje_file_get("base/theme/backgrounds",
                                                             "e/desktop/background");
                       e_thumb_icon_file_set(it->image, f, "e/desktop/background");
                    }
                  else
                    e_thumb_icon_file_set(it->image, it->file, "e/desktop/background");
                  e_thumb_icon_size_set(it->image, sd->info->iw, sd->info->ih);
                  evas_object_show(it->image);
                  edje_object_signal_emit(it->frame, "e,action,thumb,ungen", "e");
               }
             else
               {
                  if (it->sort_id)
                    {
                       if (it->do_thumb)
                         {
                            e_thumb_icon_end(it->image);
                            it->do_thumb = 0;
                         }
                    }
               }
 */
          }
     }

   if (changed)
     evas_object_smart_callback_call(obj, "changed", NULL);
   if (recursion == 0) sd->idle_enter = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static void
_e_smart_reconfigure(Evas_Object *obj)
{
   Smart_Data *sd = evas_object_smart_data_get(obj);

   if (sd->idle_enter) return;
   sd->idle_enter = ecore_idle_enterer_before_add(_e_smart_reconfigure_do, obj);
}

static void
_e_smart_add(Evas_Object *obj)
{
   Smart_Data *sd = calloc(1, sizeof(Smart_Data));

   if (!sd) return;

   sd->sx = sd->sy = -1;
   evas_object_smart_data_set(obj, sd);
}

static void
_e_smart_del(Evas_Object *obj)
{
   Smart_Data *sd = evas_object_smart_data_get(obj);
   Item *it;

   if (sd->seltimer)
     ecore_timer_del(sd->seltimer);
   if (sd->idle_enter)
     ecore_idle_enterer_del(sd->idle_enter);
   if (sd->animator)
     ecore_animator_del(sd->animator);
   // sd->info is just referenced
   // sd->child_obj is unused
   EINA_LIST_FREE(sd->items, it)
     {
        if (it->frame) evas_object_del(it->frame);
        if (it->image) evas_object_del(it->image);
        if (it->file) eina_stringshare_del(it->file);
        free(it->sort_id);
        free(it);
     }
   free(sd);
   evas_object_smart_data_set(obj, NULL);
}

static void
_e_smart_move(Evas_Object *obj, Evas_Coord x, Evas_Coord y)
{
   Smart_Data *sd = evas_object_smart_data_get(obj);

   sd->x = x;
   sd->y = y;
   _e_smart_reconfigure(obj);
}

static void
_e_smart_resize(Evas_Object *obj, Evas_Coord w, Evas_Coord h)
{
   Smart_Data *sd = evas_object_smart_data_get(obj);

   sd->w = w;
   sd->h = h;
   _e_smart_reconfigure(obj);
   evas_object_smart_callback_call(obj, "changed", NULL);
}

static void
_e_smart_show(Evas_Object *obj __UNUSED__)
{
//   Smart_Data *sd = evas_object_smart_data_get(obj);
//   evas_object_show(sd->child_obj);
}

static void
_e_smart_hide(Evas_Object *obj __UNUSED__)
{
//   Smart_Data *sd = evas_object_smart_data_get(obj);
//   evas_object_hide(sd->child_obj);
}

static void
_e_smart_color_set(Evas_Object *obj __UNUSED__, int r __UNUSED__, int g __UNUSED__, int b __UNUSED__, int a __UNUSED__)
{
//   Smart_Data *sd = evas_object_smart_data_get(obj);
//   evas_object_color_set(sd->child_obj, r, g, b, a);
}

static void
_e_smart_clip_set(Evas_Object *obj __UNUSED__, Evas_Object * clip __UNUSED__)
{
//   Smart_Data *sd = evas_object_smart_data_get(obj);
//   evas_object_clip_set(sd->child_obj, clip);
}

static void
_e_smart_clip_unset(Evas_Object *obj __UNUSED__)
{
//   Smart_Data *sd = evas_object_smart_data_get(obj);
//   evas_object_clip_unset(sd->child_obj);
}

static Evas_Object *
_pan_add(Evas *evas)
{
   static Evas_Smart *smart = NULL;
   static const Evas_Smart_Class sc =
     {
        "wp_pan",
          EVAS_SMART_CLASS_VERSION,
          _e_smart_add,
          _e_smart_del,
          _e_smart_move,
          _e_smart_resize,
          _e_smart_show,
          _e_smart_hide,
          _e_smart_color_set,
          _e_smart_clip_set,
          _e_smart_clip_unset,
          NULL,
          NULL,
          NULL,
          NULL,
          NULL,
          NULL,
          NULL
     };
   smart = evas_smart_class_new(&sc);
   return evas_object_smart_add(evas, smart);
}

static void
_pan_info_set(Evas_Object *obj, Info *info)
{
   Smart_Data *sd = evas_object_smart_data_get(obj);

   sd->info = info;
}

static Eina_Bool
_sel_anim(void *data)
{
   Evas_Object *obj = data;
   Smart_Data *sd = evas_object_smart_data_get(obj);
   double t = ecore_loop_time_get() - sd->seltime;
   double len = 1.0;
   double p = t / len;
   double d;

   if (p > 1.0) p = 1.0;
   if (!sd->selin)
     {
        d = (p * 2) - 1.0;
        if (d > 0.0)
          {
             d = 1.0 - d;
             d = d * d * d;
             d = 1.0 - d;
          }
        else
          {
             d = -1.0 - d;
             d = d * d * d;
             d = -1.0 - d;
          }
        d = (1.0 + d) / 2.0;
        sd->selmove = d;
     }
   else
     {
        d = ((1.0 - p) * 2) - 1.0;
        if (d > 0.0)
          {
             d = 1.0 - d;
             d = d * d * d;
             d = 1.0 - d;
          }
        else
          {
             d = -1.0 - d;
             d = d * d * d;
             d = -1.0 - d;
          }
        d = (1.0 + d) / 2.0;
        sd->selmove = d;
     }
   _e_smart_reconfigure(obj);
   if (p == 1.0)
     {
        if (sd->selout)
          {
             sd->selin = EINA_TRUE;
             sd->selout = EINA_FALSE;
             sd->seltime = ecore_loop_time_get();
             return ECORE_CALLBACK_RENEW;
          }
        sd->selout = EINA_FALSE;
        sd->selin = EINA_FALSE;
        sd->animator = NULL;
        return ECORE_CALLBACK_CANCEL;
     }
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_sel_timer(void *data)
{
   Evas_Object *obj = data;
   Smart_Data *sd = evas_object_smart_data_get(obj);

   if (!sd->animator)
     {
        sd->seltime = ecore_time_get();
        sd->animator = ecore_animator_add(_sel_anim, obj);
        sd->selin = EINA_FALSE;
     }
   sd->seltimer = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static void
_pan_unhilight(Evas_Object *obj __UNUSED__, Item *it)
{
//   Smart_Data *sd = evas_object_smart_data_get(obj);
   if (!it->hilighted) return;
   it->hilighted = 0;
   if (it->frame)
     edje_object_signal_emit(it->frame, "e,state,unselected", "e");
}

static void
_pan_hilight(Evas_Object *obj __UNUSED__, Item *it)
{
   Eina_List *l;
   Item *it2;
   Smart_Data *sd = evas_object_smart_data_get(obj);

   if (it->hilighted) return;
   EINA_LIST_FOREACH(sd->items, l, it2)
     {
        if (it2->hilighted)
          {
             _pan_unhilight(obj, it2);
             break;
          }
     }
   it->hilighted = 1;
   if (it->frame)
     {
        edje_object_signal_emit(it->frame, "e,state,selected", "e");
        evas_object_raise(it->frame);
     }
}


static void
_pan_sel(Evas_Object *obj, Item *it)
{
   Smart_Data *sd = evas_object_smart_data_get(obj);

   if (sd->selmove > 0.0) return;
   edje_object_signal_emit(it->frame, "e,state,selected", "e");
   evas_object_raise(it->frame);
   if (!it->selected)
     {
        Eina_List *l;
        Item *it2;

        EINA_LIST_FOREACH(sd->items, l, it2)
          {
             if (it2->selected) it2->selected = 0;
          }
        it->selected = EINA_TRUE;
        free(sd->info->bg_file);
        evas_object_hide(sd->info->mini);
        if (it->file)
          {
             char *name = NULL, *p;

             sd->info->use_theme_bg = 0;
             sd->info->bg_file = strdup(it->file);
             edje_object_file_set(sd->info->mini, sd->info->bg_file,
                                  "e/desktop/background");
             p = strrchr(sd->info->bg_file, '/');
             if (p)
               {
                  p++;
                  name = strdup(p);
                  p = strrchr(name, '.');
                  if (p) *p = 0;
               }
             edje_object_part_text_set(sd->info->bg, "e.text.filename", name);
             free(name);
          }
        else
          {
             const char *f = e_theme_edje_file_get("base/theme/backgrounds",
                                                   "e/desktop/background");

             edje_object_file_set(sd->info->mini, f, "e/desktop/background");
             sd->info->use_theme_bg = 1;
             sd->info->bg_file = NULL;
             edje_object_part_text_set(sd->info->bg, "e.text.filename",
                                       _("Theme Wallpaper"));
          }
        evas_object_show(sd->info->mini);
     }
   if (sd->seltimer) ecore_timer_del(sd->seltimer);
   sd->seltimer = ecore_timer_add(0.2, _sel_timer, obj);
}

static void
_pan_sel_up(Evas_Object *obj)
{
   Smart_Data *sd = evas_object_smart_data_get(obj);

   if (sd->selmove == 0.0) return;
   if (!sd->animator)
     {
        sd->seltime = ecore_loop_time_get();
        sd->animator = ecore_animator_add(_sel_anim, obj);
        sd->selin = EINA_TRUE;
     }
   else
     {
        if (sd->selin) return;
        sd->selout = EINA_TRUE;
     }
}

static int
_sort_cb(const void *d1, const void *d2)
{
   const Item *it1 = d1, *it2 = d2;

   if ((!it1->sort_id) || (!it2->sort_id)) return 0;
   return strcmp(it1->sort_id, it2->sort_id);
}

static void
_item_sort(Item *it)
{
   Evas_Object *obj = it->obj;
   Smart_Data *sd = evas_object_smart_data_get(obj);
   int num, dosort = 0;

   sd->id_num++;
   sd->info->scans--;
   num = eina_list_count(sd->items);
//   if (sd->sort_num < sd->id_num)
//     {
//        sd->sort_num = sd->id_num + 10;
//        dosort = 1;
//     }
   if ((sd->id_num == num) || (dosort))
     {
        sd->items = eina_list_sort(sd->items, num, _sort_cb);
        _e_smart_reconfigure_do(obj);
         if (sd->jump2hi)
          {
             Eina_List *l;
             Item *it2 = NULL;

             EINA_LIST_FOREACH(sd->items, l, it2)
               {
                  if (it2->hilighted) break;
                  it2 = NULL;
               }
             if (it2)
               e_scrollframe_child_region_show(sd->info->sframe,
                                               it2->x, it2->y, it2->w, it2->h);
             sd->jump2hi = 1;
          }
     }
   if (sd->info->scans == 0)
     edje_object_signal_emit(sd->info->bg, "e,state,busy,off", "e");
}

static void
_thumb_gen(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Item *it = data;

   edje_object_signal_emit(it->frame, "e,action,thumb,gen", "e");
   if (!it->sort_id)
     {
        const char *id = e_thumb_sort_id_get(it->image);

        if (id)
          {
             it->sort_id = strdup(id);
             _item_sort(it);
          }
     }
   it->have_thumb = EINA_TRUE;
   if (!it->visible)
     {
        if (it->do_thumb)
          {
             e_thumb_icon_end(it->image);
             it->do_thumb = EINA_FALSE;
          }
        evas_object_del(it->image);
        it->image = NULL;
        evas_object_del(it->frame);
        it->frame = NULL;
     }
}

static void
_item_down(void *data __UNUSED__, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
//   Evas_Event_Mouse_Down *ev = event_info;
//   Item *it = data;
//   _pan_sel(it->obj, it);
}

static void
_item_up(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Evas_Event_Mouse_Up *ev = event_info;
   Item *it = data;

   if (ev->button == 1)
     {
        if (!(ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD))
          {
             _pan_hilight(it->obj, it);
             _pan_sel(it->obj, it);
             // FIXME: select image!!!
          }
     }
}

static void
_pan_file_add(Evas_Object *obj, const char *file, Eina_Bool remote, Eina_Bool theme)
{
   Smart_Data *sd = evas_object_smart_data_get(obj);
   Item *it = calloc(1, sizeof(Item));
   Evas *evas;

   if (!it) return;
   evas = evas_object_evas_get(obj);
   sd->items = eina_list_append(sd->items, it);
   it->obj = obj;
   it->remote = remote;
   it->theme = theme;
   it->file = eina_stringshare_add(file);
   it->frame = edje_object_add(evas);
   if (it->theme)
     e_theme_edje_object_set(it->frame, "base/theme/widgets",
                             "e/conf/wallpaper/main/mini-theme");
   else if (it->remote)
     e_theme_edje_object_set(it->frame, "base/theme/widgets",
                             "e/conf/wallpaper/main/mini-remote");
   else
     e_theme_edje_object_set(it->frame, "base/theme/widgets",
                             "e/conf/wallpaper/main/mini");
   if (it->hilighted)
     {
        edje_object_signal_emit(it->frame, "e,state,selected", "e");
        evas_object_raise(it->frame);
     }
   evas_object_event_callback_add(it->frame, EVAS_CALLBACK_MOUSE_DOWN,
                                  _item_down, it);
   evas_object_event_callback_add(it->frame, EVAS_CALLBACK_MOUSE_UP,
                                  _item_up, it);

   evas_object_smart_member_add(it->frame, obj);
   evas_object_clip_set(it->frame, evas_object_clip_get(obj));
   evas_object_show(it->frame);
   it->image = e_thumb_icon_add(evas);
   edje_object_part_swallow(it->frame, "e.swallow.content", it->image);
   evas_object_smart_callback_add(it->image, "e_thumb_gen", _thumb_gen, it);
   if (it->theme)
     {
        const char *f = e_theme_edje_file_get("base/theme/backgrounds",
                                              "e/desktop/background");

        e_thumb_icon_file_set(it->image, f, "e/desktop/background");
     }
   else
     e_thumb_icon_file_set(it->image, it->file, "e/desktop/background");
   e_thumb_icon_size_set(it->image, sd->info->iw, sd->info->ih);
   evas_object_show(it->image);

   e_thumb_icon_begin(it->image);
   it->do_thumb = 1;
//   e_thumb_icon_begin(it->image);

   if (it->theme)
     {
        if (sd->info->use_theme_bg)
          {
             _pan_hilight(it->obj, it);
             edje_object_part_text_set(sd->info->bg, "e.text.filename",
                                       _("Theme Wallpaper"));
          }
     }
   else
     {
        if (sd->info->bg_file)
          {
             int match = 0;

             if (!strcmp(sd->info->bg_file, it->file)) match = 1;
             if (!match)
               {
                  const char *p1, *p2;

                  p1 = ecore_file_file_get(sd->info->bg_file);
                  p2 = ecore_file_file_get(it->file);
                  if (!strcmp(p1, p2)) match = 1;
               }
             if (match)
               {
                  char *name = NULL, *p;

                  sd->jump2hi = 1;
                  _pan_hilight(it->obj, it);
                  p = strrchr(sd->info->bg_file, '/');
                  if (p)
                    {
                       p++;
                       name = strdup(p);
                       p = strrchr(name, '.');
                       if (p) *p = 0;
                    }
                  edje_object_part_text_set(sd->info->bg, "e.text.filename", name);
                  free(name);
               }
          }
     }
   _e_smart_reconfigure(obj);
}

////////

static void
_resize(E_Win *wn)
{
   Info *info = wn->data;

   evas_object_resize(info->bg, wn->w, wn->h);
}

static void
_delete(E_Win *wn __UNUSED__)
{
   wp_conf_hide();
}

static void
_bg_clicked(void *data, Evas_Object *obj __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Info *info = data;

   _pan_sel_up(info->span);
}

static void
_apply(void *data, void *data2 __UNUSED__)
{
   Info *info = data;

   if (info->mode == 0)
     {
        /* all desktops */
        while (e_config->desktop_backgrounds)
          {
             E_Config_Desktop_Background *cfbg;

             cfbg = e_config->desktop_backgrounds->data;
             e_bg_del(cfbg->container, cfbg->zone, cfbg->desk_x, cfbg->desk_y);
          }
        if ((info->use_theme_bg) || (!info->bg_file))
          e_bg_default_set(NULL);
        else
          e_bg_default_set(info->bg_file);
     }
   else if (info->mode == 1)
     {
        /* specific desk */
        e_bg_del(info->con_num, info->zone_num, info->desk_x, info->desk_y);
        e_bg_add(info->con_num, info->zone_num, info->desk_x, info->desk_y,
                 info->bg_file);
     }
   else
     {
        Eina_List *dlist = NULL, *l;
        E_Config_Desktop_Background *cfbg;

        /* this screen */
        EINA_LIST_FOREACH(e_config->desktop_backgrounds, l, cfbg)
          {
             if (cfbg->zone == info->zone_num)
               dlist = eina_list_append(dlist, cfbg);
          }
        EINA_LIST_FREE(dlist, cfbg)
          e_bg_del(cfbg->container, cfbg->zone, cfbg->desk_x, cfbg->desk_y);
        e_bg_add(info->con_num, info->zone_num, -1, -1, info->bg_file);
     }
   e_bg_update();
   e_config_save_queue();
}

static void
_close(void *data __UNUSED__, void *data2 __UNUSED__)
{
   wp_conf_hide();
}

static void
_ok(void *data, void *data2 __UNUSED__)
{
  _apply(data, data2);
  wp_conf_hide();
}

static void
_wp_add(void *data, void *data2 __UNUSED__)
{
   Info *info = data;

   edje_object_signal_emit(info->bg, "e,action,panel,hide", "e");
}

static void
_wp_delete(void *data, void *data2 __UNUSED__)
{
   Info *info = data;

   edje_object_signal_emit(info->bg, "e,action,panel,hide", "e");
}

static void
_wp_changed(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Info *info = data;

   edje_object_signal_emit(info->bg, "e,action,panel,hide", "e");
}

static Eina_Bool
_idler(void *data)
{
   Eina_File_Direct_Info *st;
   Info *info = data;

   if (!info->dir)
     {
        info->idler = NULL;
        return ECORE_CALLBACK_CANCEL;
     }
   if (!eina_iterator_next(info->dir, (void**) &st))
     {
        E_FREE(info->curdir);
        eina_iterator_free(info->dir);
        info->dir = NULL;
        info->idler = NULL;
        _scan(info);
        return ECORE_CALLBACK_CANCEL;
     }
   if ((st->path[st->name_start]) == '.')
     return ECORE_CALLBACK_RENEW;
   if (st->type == EINA_FILE_DIR)
     {
        info->dirs = eina_list_append(info->dirs, strdup(st->path));
        return ECORE_CALLBACK_RENEW;
     }
   info->scans++;
   _pan_file_add(info->span, st->path, 0, 0);

   e_util_wakeup();
   return ECORE_CALLBACK_RENEW;
}

static void
_scan(Info *info)
{
   if (info->dirs)
     {
        if (info->scans <= 0)
          {
             info->scans = 0;
             edje_object_signal_emit(info->bg, "e,state,busy,on", "e");
             edje_object_part_text_set(info->bg, "e.text.busy_label",
                                       _("Loading files..."));
          }
        free(info->curdir);
        info->curdir = info->dirs->data;
        info->dirs = eina_list_remove_list(info->dirs, info->dirs);
        if (!info->dir) info->dir = eina_file_stat_ls(info->curdir);
        info->idler = ecore_idler_add(_idler, info);
     }
}

Info *
wp_browser_new(E_Container *con)
{
   Info *info;
   E_Win *win;
   E_Zone *zone;
   E_Desk *desk;
   const E_Config_Desktop_Background *cfbg;
   Evas_Coord mw, mh;
   Evas_Object *o, *o2, *ob;
   E_Radio_Group *rg;
   char buf[PATH_MAX];

   info = calloc(1, sizeof(Info));
   if (!info) return NULL;

   zone = e_util_zone_current_get(con->manager);
   desk = e_desk_current_get(zone);
   info->con_num = con->num;
   info->zone_num = zone->num;
   info->desk_x = desk->x;
   info->desk_y = desk->y;
   info->mode = 0;
   cfbg = e_bg_config_get(con->num, zone->num, desk->x, desk->y);
   if (cfbg)
     {
        if ((cfbg->container >= 0) && (cfbg->zone >= 0))
          {
             if ((cfbg->desk_x >= 0) && (cfbg->desk_y >= 0))
               info->mode = 1;
             else
               info->mode = 2;
          }
        info->bg_file = strdup(cfbg->file);
     }
   if ((!info->bg_file) && (e_config->desktop_default_background))
     info->bg_file = strdup(e_config->desktop_default_background);
   else
     info->use_theme_bg = 1;

   info->iw = (120 * e_scale);
   info->ih = (zone->h * info->iw) / (zone->w);

   win = e_win_new(con);
   if (!win)
     {
        E_FREE(info);
        return NULL;
     }
   info->win = win;
   win->data = info;

   e_user_dir_concat_static(buf, "backgrounds");
   info->dirs = eina_list_append(info->dirs, strdup(buf));
   e_prefix_data_concat_static(buf, "data/backgrounds");
   info->dirs = eina_list_append(info->dirs, strdup(buf));

   e_win_title_set(win, _("Wallpaper Settings"));
   e_win_name_class_set(win, "E", "_config::appearance/wallpaper2");
   e_win_resize_callback_set(win, _resize);
   e_win_delete_callback_set(win, _delete);

   // bg + container
   info->bg = edje_object_add(info->win->evas);
   e_theme_edje_object_set(info->bg, "base/theme/widgets",
                           "e/conf/wallpaper/main/window");
   edje_object_signal_callback_add(info->bg, "e,action,click", "e",
                                   _bg_clicked, info);

   info->box = e_widget_list_add(info->win->evas, 1, 1);

   // ok button
   info->button = e_widget_button_add(info->win->evas, _("OK"), NULL,
                                      _ok, info, NULL);
   evas_object_show(info->button);
   e_widget_list_object_append(info->box, info->button, 1, 0, 0.5);

   // apply button
   info->button = e_widget_button_add(info->win->evas, _("Apply"), NULL,
                                      _apply, info, NULL);
   evas_object_show(info->button);
   e_widget_list_object_append(info->box, info->button, 1, 0, 0.5);

   // close button
   info->button = e_widget_button_add(info->win->evas, _("Close"), NULL,
                                      _close, info, NULL);
   evas_object_show(info->button);
   e_widget_list_object_append(info->box, info->button, 1, 0, 0.5);

   e_widget_size_min_get(info->box, &mw, &mh);
   evas_object_size_hint_min_set(info->box, mw, mh);
   edje_object_part_swallow(info->bg, "e.swallow.buttons", info->box);
   evas_object_show(info->box);

   // preview
   info->preview = e_livethumb_add(info->win->evas);
   e_livethumb_vsize_set(info->preview, zone->w, zone->h);
   edje_extern_object_aspect_set(info->preview, EDJE_ASPECT_CONTROL_NEITHER,
                                 zone->w, zone->h);
   edje_object_part_swallow(info->bg, "e.swallow.preview", info->preview);
   evas_object_show(info->preview);

   info->mini = edje_object_add(e_livethumb_evas_get(info->preview));
   e_livethumb_thumb_set(info->preview, info->mini);
   evas_object_show(info->mini);
   if (info->bg_file)
     edje_object_file_set(info->mini, info->bg_file, "e/desktop/background");
   else
     {
        const char *f = e_theme_edje_file_get("base/theme/backgrounds",
                                              "e/desktop/background");

        edje_object_file_set(info->mini, f, "e/desktop/background");
     }

   // scrolled thumbs
   info->span = _pan_add(info->win->evas);
   _pan_info_set(info->span, info);

   // the scrollframe holding the scrolled thumbs
   info->sframe = e_scrollframe_add(info->win->evas);
   e_scrollframe_custom_theme_set(info->sframe, "base/theme/widgets",
                                  "e/conf/wallpaper/main/scrollframe");
   e_scrollframe_extern_pan_set(info->sframe, info->span,
                                _pan_set, _pan_get, _pan_max_get,
                                _pan_child_size_get);
   edje_object_part_swallow(info->bg, "e.swallow.list", info->sframe);
   evas_object_show(info->sframe);
   evas_object_show(info->span);

   ob = e_widget_list_add(info->win->evas, 0, 1);

   o = e_widget_list_add(info->win->evas, 1, 0);

   rg = e_widget_radio_group_new(&(info->mode));
   o2 = e_widget_radio_add(info->win->evas, _("All Desktops"), 0, rg);
   evas_object_smart_callback_add(o2, "changed", _wp_changed, info);
   e_widget_list_object_append(o, o2, 1, 0, 0.5);
   e_widget_disabled_set(o2, (e_util_container_desk_count_get(con) < 2));
   evas_object_show(o2);

   o2 = e_widget_radio_add(info->win->evas, _("This Desktop"), 1, rg);
   evas_object_smart_callback_add(o2, "changed", _wp_changed, info);
   e_widget_list_object_append(o, o2, 1, 0, 0.5);
   evas_object_show(o2);

   o2 = e_widget_radio_add(info->win->evas, _("This Screen"), 2, rg);
   evas_object_smart_callback_add(o2, "changed", _wp_changed, info);
   e_widget_list_object_append(o, o2, 1, 0, 0.5);
   if (!(e_util_container_zone_number_get(0, 1) ||
         (e_util_container_zone_number_get(1, 0))))
     e_widget_disabled_set(o2, EINA_TRUE);
   evas_object_show(o2);

   e_widget_list_object_append(ob, o, 1, 0, 0.5);
   evas_object_show(o);

   o = e_widget_list_add(info->win->evas, 1, 0);

   o2 =  e_widget_button_add(info->win->evas, _("Add"), NULL,
                             _wp_add, info, NULL);
   e_widget_list_object_append(o, o2, 1, 0, 0.5);
   evas_object_show(o2);

   o2 =  e_widget_button_add(info->win->evas, _("Delete"), NULL,
                             _wp_delete, info, NULL);
   e_widget_list_object_append(o, o2, 1, 0, 0.5);
   evas_object_show(o2);

   e_widget_list_object_append(ob, o, 1, 0, 0.5);
   evas_object_show(o);

   e_widget_size_min_get(ob, &mw, &mh);
   evas_object_size_hint_min_set(ob, mw, mh);
   edje_object_part_swallow(info->bg, "e.swallow.extras", ob);
   evas_object_show(ob);

   // min size calc
   edje_object_size_min_calc(info->bg, &mw, &mh);
   e_win_size_min_set(win, mw, mh);
   if ((zone->w / 4) > mw) mw = (zone->w / 4);
   if ((zone->h / 4) > mh) mh = (zone->h / 4);
   e_win_resize(win, mw, mh);
   e_win_centered_set(win, 1);
   e_win_show(win);
   e_win_border_icon_set(win, "preferences-desktop-wallpaper");

   evas_object_resize(info->bg, info->win->w, info->win->h);
   evas_object_show(info->bg);

   // add theme bg
   _pan_file_add(info->span, NULL, 0, 1);

   _scan(info);
   return info;
}

void
wp_broser_free(Info *info)
{
   char *s;

   if (!info) return;
   e_object_del(E_OBJECT(info->win));
   if (info->dir) eina_iterator_free(info->dir);
   free(info->bg_file);
   free(info->curdir);
   EINA_LIST_FREE(info->dirs, s)
     free(s);
   if (info->idler) ecore_idler_del(info->idler);
   // del other stuff
   E_FREE(info);
}

E_Config_Dialog *
wp_conf_show(E_Container *con, const char *params __UNUSED__)
{
   if (global_info)
     {
        e_win_show(global_info->win);
        e_win_raise(global_info->win);
     }
   global_info = wp_browser_new(con);

   return NULL;
}

void
wp_conf_hide(void)
{
   if (global_info)
     {
        wp_broser_free(global_info);
        global_info = NULL;
     }
}
