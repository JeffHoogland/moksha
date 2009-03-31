/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"

// FIXME:
//   need a proper theme done "ok/accept" button
//   need a "add" button (to add exchange, image file or gradient)
//   need a "where" opopup to select "all desktops, this screen, this desktop"
//   need filename display
//   need "theme wallpaper" image

typedef struct _Info Info;
typedef struct _Smart_Data Smart_Data;
typedef struct _Item Item;

struct _Info
{
   E_Win       *win;
   Evas_Object *bg, *preview, *mini, *button, *box, *sframe, *span;
   char        *bg_file;
   int          iw, ih;
   Eina_List   *dirs;
   char        *curdir;
   DIR         *dir;
   Ecore_Idler *idler;
   int          specific;
   int          use_theme_bg;
   int          con_num, zone_num, desk_x, desk_y;
};

struct _Smart_Data
{
   Evas_Object *child_obj;
   Eina_List   *items;
   Ecore_Idle_Enterer *idle_enter;
   Ecore_Animator *animator;
   Info        *info;
   Evas_Coord   x, y, w, h;
   Evas_Coord   cx, cy, cw, ch;
   Evas_Coord   sx, sy;
   double       seltime;
   double       selmove;
   Evas_Bool    selin : 1;
   Evas_Bool    selout : 1;
};

struct _Item
{
   Evas_Object *obj;
   Evas_Coord x, y, w, h;
   const char *file;
   Evas_Object *frame, *image;
   Evas_Bool selected : 1;
   Evas_Bool have_thumb : 1;
   Evas_Bool do_thumb : 1;
};

static Info *global_info = NULL;

static void _e_smart_reconfigure(Evas_Object *obj);

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

static void _e_smart_reconfigure(Evas_Object *obj);
static void _thumb_gen(void *data, Evas_Object *obj, void *event_info);

static int
_e_smart_reconfigure_do(void *data)
{
   Evas_Object *obj = data;
   Smart_Data *sd = evas_object_smart_data_get(obj);
   Eina_List *l;
   Item *it;
   int iw, redo = 0, changed = 0;
   Evas_Coord x, y, xx, yy, ww, hh, cw, ch, mw, mh, ox, oy, dd;
   
   if (sd->cx > (sd->cw - sd->w)) sd->cx = sd->cw - sd->w;
   if (sd->cy > (sd->ch - sd->h)) sd->cy = sd->ch - sd->h;
   if (sd->cx < 0) sd->cx = 0;
   if (sd->cy < 0) sd->cy = 0;
   
   iw = sd->w / 4;
   if (iw > (120 * e_scale)) iw = 120 * e_scale;
   else
     {
        if (iw < (60 * e_scale)) iw = sd->w / 3;
        if (iw < (60 * e_scale)) iw = sd->w / 2;
        if (iw < (60 * e_scale)) iw = sd->w;
     }
   x = 0;
   y = 0;
   ww = iw;
   hh = (sd->info->ih * iw) / (sd->info->iw);
   
   mw = mh = 0;
   EINA_LIST_FOREACH(sd->items, l, it)
     {
        xx = sd->x - sd->cx + x;
        if (x > (sd->w - ww))
          {
             x = 0;
             y += hh;
             xx = sd->x - sd->cx + x;
          }
        yy = sd->y - sd->cy + y;
        it->x = x;
        it->y = y;
        it->w = ww;
        it->h = hh;
        if (it->selected)
          {
             sd->sx = it->x + (it->w / 2);
             sd->sy = it->y + (it->h / 2);
          }
        if ((x + ww) > mw)mw = x + ww;
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
        if (redo) _e_smart_reconfigure_do(obj);
        changed = 1;
     }
   
   ox = 0;
   if (sd->w > sd->cw) ox = (sd->w - sd->cw) / 2;
   oy = 0;
   if (sd->h > sd->ch) oy = (sd->h - sd->ch) / 2;

   EINA_LIST_FOREACH(sd->items, l, it)
     { 
        Evas_Coord dx, dy, vw, vh;
        
        dx = dy = 0;
        if ((sd->sx >= 0) && 
            (sd->selmove > 0.0)
            
/*            &&
            ((it->x + it->w) > sd->cx) &&
            ((it->x) < (sd->cx + sd->w)) &&
            ((it->y + it->h) > sd->cy) &&
            ((it->y) < (sd->cy + sd->h))
 */
            )
          {
             double a, d, di;
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
                  // |\
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
             dx = dy = 0;
             
             for (p = (char *)it->file; *p; p++) sum += (int)(*p);
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
        evas_object_move(it->frame, 
                         xx + dx,
                         yy + dy);
        evas_object_resize(it->frame, it->w, it->h);
        evas_output_viewport_get(evas_object_evas_get(obj), NULL, NULL, &vw, &vh);
        if (E_INTERSECTS(xx, yy, it->w, it->h, 0, 0, vw, vh))
          {
             if (!it->have_thumb)
               {
                  if (!it->do_thumb)
                    {
                       e_thumb_icon_begin(it->image);
                       it->do_thumb = 1;
                    }
               }
          }
        else
          {
             if (it->have_thumb)
               {
                  if (it->do_thumb)
                    {
                       e_thumb_icon_end(it->image);
                       it->do_thumb = 0;
                    }
                  evas_object_del(it->image);
                  it->have_thumb = 0;
                  it->image = e_thumb_icon_add(evas_object_evas_get(obj));
                  edje_object_part_swallow(it->frame, "e.swallow.content", it->image);
                  evas_object_smart_callback_add(it->image, "e_thumb_gen", _thumb_gen, it);
                  e_thumb_icon_file_set(it->image, it->file, "e/desktop/background");
                  e_thumb_icon_size_set(it->image, sd->info->iw, sd->info->ih);
                  evas_object_show(it->image);
                  edje_object_signal_emit(it->frame, "e,action,thumb,ungen", "e");
               }
             else
               {
                  if (it->do_thumb)
                    {
                       e_thumb_icon_end(it->image);
                       it->do_thumb = 0;
                    }
               }
          }
     }
   
   if (changed)
     evas_object_smart_callback_call(obj, "changed", NULL);
   sd->idle_enter = NULL;
   return 0;
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
   sd->x = sd->y = sd->w = sd->h = 0;
   sd->sx = sd->sy = -1;
   evas_object_smart_data_set(obj, sd);
}

static void
_e_smart_del(Evas_Object *obj)
{
   Smart_Data *sd = evas_object_smart_data_get(obj);
   if (sd->idle_enter)
     ecore_idle_enterer_del(sd->idle_enter);
   if (sd->animator)
     ecore_animator_del(sd->animator);
   free(sd);
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
_e_smart_show(Evas_Object *obj)
{
   Smart_Data *sd = evas_object_smart_data_get(obj);
   evas_object_show(sd->child_obj);
}

static void
_e_smart_hide(Evas_Object *obj)
{
   Smart_Data *sd = evas_object_smart_data_get(obj);
   evas_object_hide(sd->child_obj);
}

static void
_e_smart_color_set(Evas_Object *obj, int r, int g, int b, int a)
{
   Smart_Data *sd = evas_object_smart_data_get(obj);
   evas_object_color_set(sd->child_obj, r, g, b, a);
}

static void
_e_smart_clip_set(Evas_Object *obj, Evas_Object * clip)
{
   Smart_Data *sd = evas_object_smart_data_get(obj);
   evas_object_clip_set(sd->child_obj, clip);
}

static void
_e_smart_clip_unset(Evas_Object *obj)
{
   Smart_Data *sd = evas_object_smart_data_get(obj);
   evas_object_clip_unset(sd->child_obj);
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


static int
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
             sd->selin = 1;
             sd->selout = 0;
             sd->seltime = ecore_loop_time_get();
             return 1;
          }
        sd->selout = 0;
        sd->selin = 0;
        sd->animator = NULL;
        return 0;
     }
   return 1;
}

static void
_pan_sel(Evas_Object *obj, Item *it)
{
   Smart_Data *sd = evas_object_smart_data_get(obj);
   if (sd->selmove > 0.0) return;
   edje_object_signal_emit(it->frame, "e,state,selected", "e");
   if (!it->selected)
     {
        Eina_List *l;
        Item *it2;

        EINA_LIST_FOREACH(sd->items, l, it2)
          {
             if (it2->selected) it2->selected = 0;
          }
        it->selected = 1;
        if (sd->info->bg_file) free(sd->info->bg_file);
        sd->info->bg_file = strdup(it->file);
        evas_object_hide(sd->info->mini);
        edje_object_file_set(sd->info->mini, sd->info->bg_file,
                             "e/desktop/background");
        evas_object_show(sd->info->mini);
     }
   if (!sd->animator)
     {
        sd->seltime = ecore_loop_time_get();
        sd->animator = ecore_animator_add(_sel_anim, obj);
        sd->selin = 0;
     }
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
        sd->selin = 1;
     }
   else
     {
        if (sd->selin) return;
        sd->selout = 1;
     }
}

static void
_thumb_gen(void *data, Evas_Object *obj, void *event_info)
{
   Item *it = data;
   edje_object_signal_emit(it->frame, "e,action,thumb,gen", "e");
   it->have_thumb = 1;
}

static void         
_item_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *ev = event_info;
   Item *it = data;
//   _pan_sel(it->obj, it);
}
    
static void         
_item_up(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Up *ev = event_info;
   Item *it = data;
   if (ev->button == 1)
     {
        if (!(ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD))
          {
             _pan_sel(it->obj, it);
             // FIXME: select image!!!
          }
     }
}
    
static void
_pan_file_add(Evas_Object *obj, const char *file)
{
   Smart_Data *sd = evas_object_smart_data_get(obj);
   Item *it = calloc(1, sizeof(Item));
   if(!it) return;
   printf("+%s\n", file);
   sd->items = eina_list_append(sd->items, it);
   it->obj = obj;
   it->file = eina_stringshare_add(file);
   it->frame = edje_object_add(evas_object_evas_get(obj));
   e_theme_edje_object_set(it->frame, "base/theme/widgets",
                           "e/conf/wallpaper/main/mini");
   evas_object_event_callback_add(it->frame, EVAS_CALLBACK_MOUSE_DOWN,
                                  _item_down, it);
   evas_object_event_callback_add(it->frame, EVAS_CALLBACK_MOUSE_UP,
                                  _item_up, it);
   
   evas_object_smart_member_add(it->frame, obj);
   evas_object_clip_set(it->frame, evas_object_clip_get(obj));
   evas_object_show(it->frame);
   it->image = e_thumb_icon_add(evas_object_evas_get(obj));
   edje_object_part_swallow(it->frame, "e.swallow.content", it->image);
   evas_object_smart_callback_add(it->image, "e_thumb_gen", _thumb_gen, it);
   e_thumb_icon_file_set(it->image, it->file, "e/desktop/background");
   e_thumb_icon_size_set(it->image, sd->info->iw, sd->info->ih);
   evas_object_show(it->image);
//   e_thumb_icon_begin(it->image);
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
_delete(E_Win *wn)
{
   wp_conf_hide();
}

static void
_bg_clicked(void *data, Evas_Object *obj, const char *emission, const char *source)
{
   Info *info = data;
   _pan_sel_up(info->span);
}

static void
_ok(void *data, void *data2)
{
   Info *info = data;
   if (info->specific)
     {
        /* update a specific config */
        e_bg_del(info->con_num, info->zone_num, info->desk_x, info->desk_y);
        e_bg_add(info->con_num, info->zone_num, info->desk_x, info->desk_y, info->bg_file);
     }
   else
     {
        /* set the default and nuke individual configs */
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
        
//        info->all_this_desk_screen = 0;
     }
   e_bg_update();
   e_config_save_queue();
   wp_conf_hide();
}

static void _scan(Info *info);

static int
_idler(void *data)
{
   struct dirent *dp;
   char buf[PATH_MAX], *p;
   Info *info = data;
   
   if (!info->dir)
     {
        info->idler = NULL;
        return 0;
     }
   dp = readdir(info->dir);
   if (!dp)
     {
        free(info->curdir);
        info->curdir = NULL;
        closedir(info->dir);
        info->dir = NULL;
        info->idler = NULL;
        _scan(info);
        return 0;
     }
   if ((!strcmp(dp->d_name, ".")) || (!strcmp(dp->d_name, "..")))
     return 1;
   if (dp->d_name[0] == '.')
     return 1;
   snprintf(buf, sizeof(buf), "%s/%s", info->curdir, dp->d_name);
   if (ecore_file_is_dir(buf))
     {
        info->dirs = eina_list_append(info->dirs, strdup(buf));
        return 1;
     }
   _pan_file_add(info->span, buf);
   
   e_util_wakeup();
   return 1;
}

static void
_scan(Info *info)
{
   if (info->dirs)
     {
        if (info->curdir) free(info->curdir);
        info->curdir = info->dirs->data;
        info->dirs = eina_list_remove_list(info->dirs, info->dirs);
        if (!info->dir) info->dir = opendir(info->curdir);
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
   char buf[PATH_MAX];   
   
   info = calloc(1, sizeof(Info));
   if (!info) return NULL;
   
   zone = e_util_zone_current_get(con->manager);
   desk = e_desk_current_get(zone);
   info->con_num = con->num;
   info->zone_num = zone->id;
   info->desk_x = desk->x;
   info->desk_y = desk->y;
   
   cfbg = e_bg_config_get(con->num, zone->id, desk->x, desk->y);
   if (cfbg)
     {
        if ((cfbg->container >= 0) && (cfbg->zone >= 0))
          {
             // info->specific = 1;
//             if (cfbg->desk_x >= 0 && cfbg->desk_y >= 0)
//               cfdata->all_this_desk_screen = E_CONFIG_WALLPAPER_DESK;
//             else
//               cfdata->all_this_desk_screen = E_CONFIG_WALLPAPER_SCREEN;
          }
        info->bg_file = strdup(cfbg->file);
     }
   if ((!info->bg_file) && (e_config->desktop_default_background))
     {
        // default bg
        info->bg_file = strdup(e_config->desktop_default_background);
     }
   else
     {
        // use theme bg
//        info->use_theme_bg = 1;
     }
   
   info->iw = 256;
   info->ih = (zone->h * 256) / (zone->w);
   
   win = e_win_new(con);
   if (!win)
     {
        free(info);
        info = NULL;
        return NULL;
     }
   info->win = win;
   win->data = info;
   
   snprintf(buf, sizeof(buf), "%s/.e/e/backgrounds", e_user_homedir_get());
   info->dirs = eina_list_append(info->dirs, strdup(buf));
   snprintf(buf, sizeof(buf), "%s/data/backgrounds", e_prefix_data_get());
   info->dirs = eina_list_append(info->dirs, strdup(buf));
   
   e_win_title_set(win, _("Wallpaer Settings"));
   e_win_name_class_set(win, "E", "_config_wallpaper_dialog");
   e_win_border_icon_set(win, "preferences-desktop-wallpaper");
   e_win_resize_callback_set(win, _resize);
   e_win_delete_callback_set(win, _delete);

   // bg + container
   info->bg = edje_object_add(info->win->evas);
   e_theme_edje_object_set(info->bg, "base/theme/widgets",
                           "e/conf/wallpaper/main/window");
   edje_object_signal_callback_add(info->bg, "e,action,click", "e",
                                   _bg_clicked, info);

   // ok button
   info->box = e_widget_list_add(info->win->evas, 1, 1);
   
   info->button = e_widget_button_add(info->win->evas, _("OK"), NULL, 
                                      _ok, info, NULL);
   evas_object_show(info->button);
   e_widget_list_object_append(info->box, info->button, 1, 0, 0.5);
   
   e_widget_min_size_get(info->box, &mw, &mh);
   edje_extern_object_min_size_set(info->box, mw, mh);
   edje_object_part_swallow(info->bg, "e.swallow.buttons", info->box);
   evas_object_show(info->box);

   // preview
   info->preview = e_livethumb_add(info->win->evas);
   e_livethumb_vsize_set(info->preview, zone->w, zone->h);
   edje_extern_object_aspect_set(info->preview, EDJE_ASPECT_CONTROL_NEITHER, zone->w, zone->h);
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

   // min size calc
   edje_object_size_min_calc(info->bg, &mw, &mh);
   e_win_size_min_set(win, mw, mh);
   if ((zone->w / 4) > mw) mw = (zone->w / 4);
   if ((zone->h / 4) > mh) mh = (zone->h / 4);
   e_win_resize(win, mw, mh);
   e_win_centered_set(win, 1);
   e_win_show(win);

   evas_object_resize(info->bg, info->win->w, info->win->h);
   evas_object_show(info->bg);
   
   _scan(info);
   return info;
}

void
wp_broser_free(Info *info)
{
   if (!info) return;
   e_object_del(E_OBJECT(info->win));
   // del other stuff
   free(info);
   info = NULL;
}

void
wp_conf_show(E_Container *con, const char *params __UNUSED__)
{
   if (global_info)
     {
        e_win_show(global_info->win);
        e_win_raise(global_info->win);
     }
   global_info = wp_browser_new(con);
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
