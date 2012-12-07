#include "e.h"
#include "e_fm_device.h"
#include <sys/statvfs.h>
#ifdef HAVE_EMOTION
# include <Emotion.h>
#endif

#define FILEPREVIEW_TEXT_PREVIEW_SIZE 2048

typedef struct _E_Widget_Data E_Widget_Data;
struct _E_Widget_Data
{
   Evas_Object *obj;
   Evas_Object *o_preview_list;
   Evas_Object *o_preview_preview_table;
   Evas_Object *o_preview_properties_table;
   Evas_Object *o_preview_scroll;
   Evas_Object *o_preview_extra;
   Evas_Object *o_preview_extra_entry;
   Evas_Object *o_preview_size;
   Evas_Object *o_preview_size_entry;
   Evas_Object *o_preview_owner;
   Evas_Object *o_preview_owner_entry;
   Evas_Object *o_preview_perms;
   Evas_Object *o_preview_perms_entry;
   Evas_Object *o_preview_time;
   Evas_Object *o_preview_time_entry;
   Evas_Object *o_preview_preview;
   Evas_Object *o_preview_scrollframe;
   Evas_Coord   preview_w, preview_h;
   int w, h;
   Ecore_Thread *preview_text_file_thread;
   Eio_Monitor *monitor;
   Eina_List *handlers;
   char        *preview_extra_text;
   char        *preview_size_text;
   char        *preview_owner_text;
   char        *preview_perms_text;
   char        *preview_time_text;
   const char        *path;
   const char        *mime;
   double vid_pct;
   Eina_Bool mime_icon : 1;
   Eina_Bool is_dir : 1;
   Eina_Bool is_txt : 1;
   Eina_Bool prev_is_fm : 1;
   Eina_Bool prev_is_txt : 1;
   Eina_Bool prev_is_video : 1;
};

static void  _e_wid_fprev_preview_update(void *data, Evas_Object *obj, void *event_info);
static void  _e_wid_fprev_preview_file(E_Widget_Data *wd);
static char *_e_wid_file_size_get(off_t st_size);
static char *_e_wid_file_user_get(uid_t st_uid);
static char *_e_wid_file_perms_get(mode_t st_mode, uid_t st_uid, gid_t gid);
static char *_e_wid_file_time_get(time_t st_modtime);
static void _e_wid_fprev_img_update(E_Widget_Data *wd, const char *path, const char *key);
static void _e_wid_del_hook(Evas_Object *obj);
static void _e_wid_fprev_preview_reset(E_Widget_Data *wd);
static void _e_wid_fprev_preview_txt(E_Widget_Data *wd);
static void _e_wid_fprev_preview_fm(E_Widget_Data *wd);

static void
_e_wid_fprev_preview_update(void *data, Evas_Object *obj, void *event_info __UNUSED__)
{
   E_Widget_Data *wd;
   Evas_Object *o;
   char buf[256];
   int iw = 0, ih = 0;

   wd = data;
   if (evas_object_image_extension_can_load_get(wd->path))
     {
        o = evas_object_image_add(evas_object_evas_get(obj));
        evas_object_image_file_set(o, wd->path, NULL);
        evas_object_image_size_get(o, &iw, &ih);
        evas_object_del(o);
     }
   if ((iw > 0) && (ih > 0))
     {
        e_widget_label_text_set(wd->o_preview_extra, _("Resolution:"));
        snprintf(buf, sizeof(buf), _("%i×%i"), iw, ih);
        e_widget_entry_text_set(wd->o_preview_extra_entry, buf);
     }
   else if (wd->mime)
     {
        if (wd->mime_icon)
          {
             e_widget_label_text_set(wd->o_preview_extra, _("Mime-type:"));
             e_widget_entry_text_set(wd->o_preview_extra_entry, wd->mime);
          }
        else
          {
             const char *mime;
             Efreet_Desktop *ed = NULL;
             unsigned int size;
                  char group[1024];
             Eina_Bool edj;
             
             wd->mime_icon = EINA_TRUE;
             size = wd->w;
             mime = e_util_mime_icon_get(wd->mime, size);
             if (mime)
               {
                  edj = eina_str_has_extension(mime, "edj");
                  if (edj)
                    snprintf(group, sizeof(group), "e/icons/fileman/mime/%s", wd->mime);
                  _e_wid_fprev_img_update(wd, mime, edj ? group : NULL);
                  return;
               }
             if (eina_str_has_extension(wd->path, "desktop"))
               {
                  ed = efreet_desktop_new(wd->path);
                  if (ed)
                    mime = efreet_icon_path_find(e_config->icon_theme, ed->icon, size);
               }
             if (!mime)
               mime = efreet_icon_path_find(e_config->icon_theme, "unknown", size);
             if (!mime)
               mime = efreet_icon_path_find(e_config->icon_theme, "text/plain", size);
             _e_wid_fprev_img_update(wd, mime, NULL);
             if (ed) efreet_desktop_free(ed);
          }
     }
   e_widget_table_object_repack(wd->o_preview_preview_table,
                                wd->o_preview_preview,
                                0, 0, 1, 1, 0, 0, 1, 1);
}

static void
_e_wid_fprev_img_update(E_Widget_Data *wd, const char *path, const char *key)
{
   if (!path) return;
   if (wd->is_dir || wd->is_txt) return;
   if (eina_str_has_extension(path, ".gif"))
     {
        e_widget_preview_file_set(wd->o_preview_preview, path, key);
        _e_wid_fprev_preview_update(wd, wd->o_preview_preview, NULL);
     }
   else
     e_widget_preview_thumb_set(wd->o_preview_preview, path, key, wd->w, wd->h);
}

static void
_e_wid_fprev_clear_widgets(E_Widget_Data *wd)
{
#define CLRWID(xx) \
   do { if (wd->xx) { evas_object_del(wd->xx); wd->xx = NULL; } } while (0)

   CLRWID(o_preview_preview_table);
   CLRWID(o_preview_properties_table);
   CLRWID(o_preview_scroll);
   CLRWID(o_preview_extra);
   CLRWID(o_preview_extra_entry);
   CLRWID(o_preview_size);
   CLRWID(o_preview_size_entry);
   CLRWID(o_preview_owner);
   CLRWID(o_preview_owner_entry);
   CLRWID(o_preview_perms);
   CLRWID(o_preview_perms_entry);
   CLRWID(o_preview_time);
   CLRWID(o_preview_time_entry);
   CLRWID(o_preview_preview);
   CLRWID(o_preview_scrollframe);
   wd->is_dir = wd->is_txt = wd->prev_is_fm = wd->prev_is_video = EINA_FALSE;
   wd->vid_pct = 0;
   if (wd->preview_text_file_thread) ecore_thread_cancel(wd->preview_text_file_thread);
   wd->preview_text_file_thread = NULL;
}

#ifdef HAVE_EMOTION

static void
_e_wid_fprev_preview_video_position(E_Widget_Data *wd, Evas_Object *obj, void *event_info __UNUSED__)
{
   double t, tot, ratio;
   int iw, ih;
   Evas_Coord w, h;
   
   evas_object_geometry_get(wd->o_preview_properties_table, NULL, NULL, &w, &h);

   tot = emotion_object_play_length_get(obj);
   if (!tot) return;
   wd->vid_pct = t = (emotion_object_position_get(obj) * 100.0) / emotion_object_play_length_get(obj);
   e_widget_slider_value_double_set(wd->o_preview_time, t);

   if (w < 10) return;
   w -= 4;
   emotion_object_size_get(obj, &iw, &ih);
   ratio = emotion_object_ratio_get(obj);
   if (ratio > 0.0) iw = (ih * ratio) + 0.5;
   if (iw < 1) iw = 1;
   if (ih < 1) ih = 1;
   e_widget_preview_vsize_set(wd->o_preview_preview, w, (w * ih) / iw);
   e_widget_size_min_set(wd->o_preview_preview, w, (w * ih) / iw);
   e_widget_table_object_repack(wd->o_preview_properties_table,
                                wd->o_preview_preview, 0, 0, 2, 2, 0, 0, 1, 1);
}

static void
_e_wid_fprev_preview_video_opened(E_Widget_Data *wd, Evas_Object *obj, void *event_info __UNUSED__)
{
    e_widget_entry_text_set(wd->o_preview_extra_entry, e_util_time_str_get(emotion_object_play_length_get(obj)));
}

static void
_e_wid_fprev_preview_video_change(void *data, Evas_Object *obj)
{
   double pos, tot, t;

   tot = emotion_object_play_length_get(data);
   t = emotion_object_position_get(data);
   e_widget_slider_value_double_get(obj, &pos);
   pos = (pos * tot) / 100.0;
   t = pos - t;
   if (t < 0.0) t = -t;
   if (t > 0.25) emotion_object_position_set(data, pos);
}

static void
_e_wid_fprev_preview_video_widgets(E_Widget_Data *wd)
{
   Evas *evas = evas_object_evas_get(wd->obj);
   Evas_Object *o, *em;
   int mw, mh, y = 3;
   
   _e_wid_fprev_clear_widgets(wd);

   o = e_widget_table_add(evas, 0);
   wd->o_preview_properties_table = o;

#define WIDROW(lab, labob, entob, entw) \
   do { \
      o = e_widget_label_add(evas, lab); \
      wd->labob = o; \
      e_widget_table_object_align_append(wd->o_preview_properties_table, \
                                         wd->labob,                     \
                                         0, y, 1, 1, 0, 1, 0, 0, 1.0, 0.0); \
      o = e_widget_entry_add(evas, &(wd->preview_extra_text), NULL, NULL, NULL); \
      e_widget_entry_readonly_set(o, 1); \
      wd->entob = o; \
      e_widget_size_min_set(o, entw, -1); \
      e_widget_table_object_align_append(wd->o_preview_properties_table, \
                                         wd->entob,                     \
                                         1, y, 1, 1, 1, 1, 1, 0, 0.0, 0.0); \
      y++; \
   } while (0)

   o = e_widget_table_add(evas, 0);
   e_widget_size_min_set(o, wd->w, wd->h);
   e_widget_table_object_append(wd->o_preview_properties_table,
                                o, 0, 0, 2, 2, 1, 1, 1, 1);
   
   wd->o_preview_preview = e_widget_preview_add(evas, 4, 4);
   em = o = emotion_object_add(e_widget_preview_evas_get(wd->o_preview_preview));
   emotion_object_init(o, NULL);
   emotion_object_file_set(o, wd->path);
   emotion_object_play_set(o, EINA_TRUE);
   evas_object_size_hint_aspect_set(o, EVAS_ASPECT_CONTROL_BOTH, wd->w, wd->h);
   e_widget_preview_extern_object_set(wd->o_preview_preview, o);
   e_widget_table_object_append(wd->o_preview_properties_table,
                                wd->o_preview_preview, 0, 0, 2, 2, 1, 1, 1, 1);
   
   evas_object_smart_callback_add(o, "length_change", (Evas_Smart_Cb)_e_wid_fprev_preview_video_opened, wd);
   evas_object_smart_callback_add(o, "frame_decode", (Evas_Smart_Cb)_e_wid_fprev_preview_video_position, wd);

   o = e_widget_slider_add(evas, 1, 0, _("%3.1f%%"), 0, 100, 0.5, 0, &wd->vid_pct, NULL, 40);
   wd->o_preview_time = o;
   e_widget_table_object_align_append(wd->o_preview_properties_table,
                                      wd->o_preview_time,                    
                                      0, 2, 2, 1, 1, 0, 1, 0, 0.5, 0.5);
   e_widget_on_change_hook_set(o, _e_wid_fprev_preview_video_change, em);
   WIDROW(_("Length:"), o_preview_extra, o_preview_extra_entry, 40);
   WIDROW(_("Size:"), o_preview_size, o_preview_size_entry, 40);
   /* FIXME: other infos? */

   e_widget_list_object_append(wd->o_preview_list,
                               wd->o_preview_properties_table,
                               1, 1, 0.5);
   
   e_widget_size_min_get(wd->o_preview_list, &mw, &mh);
   e_widget_size_min_set(wd->obj, mw, mh);
   evas_object_show(wd->o_preview_preview_table);
   evas_object_show(wd->o_preview_extra);
   evas_object_show(wd->o_preview_extra_entry);
   evas_object_show(wd->o_preview_size);
   evas_object_show(wd->o_preview_size_entry);
   evas_object_show(wd->o_preview_owner);
   evas_object_show(wd->o_preview_owner_entry);
   evas_object_show(wd->o_preview_perms);
   evas_object_show(wd->o_preview_perms_entry);
   evas_object_show(wd->o_preview_time);
   evas_object_show(wd->o_preview_time_entry);
   evas_object_show(wd->o_preview_properties_table);
   wd->prev_is_video = EINA_TRUE;
#undef WIDROW
}

#endif

static void
_e_wid_fprev_preview_fs_widgets(E_Widget_Data *wd, Eina_Bool mount_point)
{
   Evas *evas = evas_object_evas_get(wd->obj);
   Evas_Object *o;
   int mw, mh, y = 0;
   
   _e_wid_fprev_clear_widgets(wd);

   o = e_widget_table_add(evas, 0);
   wd->o_preview_properties_table = o;

#define WIDROW(lab, labob, entob, entw) \
   do { \
      o = e_widget_label_add(evas, lab); \
      wd->labob = o; \
      e_widget_table_object_align_append(wd->o_preview_properties_table, \
                                         wd->labob,                     \
                                         0, y, 1, 1, 0, 1, 0, 0, 1.0, 0.0); \
      o = e_widget_entry_add(evas, &(wd->preview_extra_text), NULL, NULL, NULL); \
      e_widget_entry_readonly_set(o, 1); \
      wd->entob = o; \
      e_widget_size_min_set(o, entw, -1); \
      e_widget_table_object_align_append(wd->o_preview_properties_table, \
                                         wd->entob,                     \
                                         1, y, 1, 1, 1, 1, 1, 0, 0.0, 0.0); \
      y++; \
   } while (0)
   
   WIDROW(_("Used:"), o_preview_extra, o_preview_extra_entry, 100);
   WIDROW(_("Size:"), o_preview_size, o_preview_size_entry, 100);
   WIDROW(_("Reserved:"), o_preview_owner, o_preview_owner_entry, 100);
   WIDROW(_("Mount status:"), o_preview_perms, o_preview_perms_entry, 100);
   if (mount_point)
     WIDROW(_("Type:"), o_preview_time, o_preview_time_entry, 100);

   e_widget_list_object_append(wd->o_preview_list,
                               wd->o_preview_properties_table,
                               1, 1, 0.5);
   
   e_widget_size_min_get(wd->o_preview_list, &mw, &mh);
   e_widget_size_min_set(wd->obj, mw, mh);
   evas_object_show(wd->o_preview_preview_table);
   evas_object_show(wd->o_preview_extra);
   evas_object_show(wd->o_preview_extra_entry);
   evas_object_show(wd->o_preview_size);
   evas_object_show(wd->o_preview_size_entry);
   evas_object_show(wd->o_preview_owner);
   evas_object_show(wd->o_preview_owner_entry);
   evas_object_show(wd->o_preview_perms);
   evas_object_show(wd->o_preview_perms_entry);
   evas_object_show(wd->o_preview_time);
   evas_object_show(wd->o_preview_time_entry);
   evas_object_show(wd->o_preview_properties_table);
}

static void
_e_wid_fprev_preview_file_widgets(E_Widget_Data *wd, Eina_Bool dir, Eina_Bool txt)
{
   Evas *evas = evas_object_evas_get(wd->obj);
   Evas_Object *o;
   int mw, mh, y = 0;
   
   _e_wid_fprev_clear_widgets(wd);

   o = e_widget_table_add(evas, 0);
   wd->o_preview_preview_table = o;
   e_widget_size_min_set(o, 32, 32);

   e_widget_list_object_append(wd->o_preview_list,
                               wd->o_preview_preview_table,
                               0, 1, 0.5);

   o = e_widget_table_add(evas, 0);
   wd->o_preview_properties_table = o;
   wd->is_dir = dir;
   wd->is_txt = txt;

   if (!dir)
     {
        if (!txt)
          WIDROW(_("Resolution:"), o_preview_extra, o_preview_extra_entry, 100);
        WIDROW(_("Size:"), o_preview_size, o_preview_size_entry, 100);
     }
   WIDROW(_("Owner:"), o_preview_owner, o_preview_owner_entry, 100);
   WIDROW(_("Permissions:"), o_preview_perms, o_preview_perms_entry, 100);
   WIDROW(_("Modified:"), o_preview_time, o_preview_time_entry, 100);

   e_widget_list_object_append(wd->o_preview_list,
                               wd->o_preview_properties_table,
                               1, 1, 0.5);

   e_widget_size_min_get(wd->o_preview_list, &mw, &mh);
   e_widget_size_min_set(wd->obj, mw, mh);
   evas_object_show(wd->o_preview_preview_table);
   evas_object_show(wd->o_preview_extra);
   evas_object_show(wd->o_preview_extra_entry);
   evas_object_show(wd->o_preview_size);
   evas_object_show(wd->o_preview_size_entry);
   evas_object_show(wd->o_preview_owner);
   evas_object_show(wd->o_preview_owner_entry);
   evas_object_show(wd->o_preview_perms);
   evas_object_show(wd->o_preview_perms_entry);
   evas_object_show(wd->o_preview_time);
   evas_object_show(wd->o_preview_time_entry);
   evas_object_show(wd->o_preview_properties_table);
}

static void
_e_wid_fprev_preview_file(E_Widget_Data *wd)
{
   char *size, *owner, *perms, *mtime;
   struct stat st;
   int mw, mh;
   Eina_Bool is_fs = EINA_FALSE;

   if (stat(wd->path, &st) < 0) return;
   // if its a desktop file treat is spcially
   if (((wd->mime) &&(!strcasecmp(wd->mime, "application/x-desktop"))) ||
       (eina_str_has_extension(wd->path, "desktop")))
     {
        Efreet_Desktop *desktop;
        const char *type, *file;
        
        // load it and if its a specual removable or mount point
        // desktop file for e, then we want to do something special
        desktop = efreet_desktop_new(wd->path);
        if ((desktop) && (desktop->url) &&
            (desktop->type == EFREET_DESKTOP_TYPE_LINK) &&
            (desktop->x) &&
            ((type = eina_hash_find(desktop->x, "X-Enlightenment-Type"))) &&
            ((!strcmp(type, "Mount")) || (!strcmp(type, "Removable"))))
          {
             // find the "mountpoint" or "link" - it's stringshared
             if ((file = e_fm2_desktop_url_eval(desktop->url)))
               {
                  struct statvfs stfs;
                  Eina_Bool ok = EINA_FALSE;
                  
                  memset(&stfs, 0, sizeof(stfs));
                  if (statvfs(file, &stfs) == 0) ok = EINA_TRUE;
                  else
                    {
                       E_Volume *v;
                       
                       v = e_fm2_device_volume_find(file);
                       if (v && v->mount_point)
                         {
                            if (statvfs(v->mount_point, &stfs) == 0)
                              {
                                 ok = EINA_TRUE;
                                 eina_stringshare_replace(&file, v->mount_point);
                              }
                         }
                    }
                  
                  if (ok)
                    {
                       unsigned long fragsz;
                       unsigned long long blknum, blkused, blkres;
                       double mbsize, mbused, mbres;
                       Eina_Bool rdonly = EINA_FALSE;
                       char buf[PATH_MAX], mpoint[4096], fstype[4096];
                       FILE *f;
                       
                       fragsz = stfs.f_frsize;
                       blknum  = stfs.f_blocks;
                       blkused = stfs.f_blocks - stfs.f_bfree;
                       blkres = stfs.f_bfree - stfs.f_bavail;
                       
                       fstype[0] = 0;
                       mpoint[0] = 0;
                       f = fopen("/etc/mtab", "r");
                       if (f)
                         {
                            while (fgets(buf, sizeof(buf), f))
                              {
                                 if (sscanf(buf, "%*s %4000s %4000s %*s",
                                            mpoint, fstype) == 2)
                                   {
                                      if (!strcmp(mpoint, file)) break;
                                   }
                                 fstype[0] = 0;
                                 mpoint[0] = 0;
                              }
                            fclose(f);
                         }
                       if (blknum > blkres)
                         {
                            is_fs = EINA_TRUE;
                            
                            mbres = ((double)blkres * (double)fragsz) / 
                              (double)(1024*1024);
                            mbsize = ((double)blknum * (double)fragsz) /
                              (double)(1024*1024);
                            mbused = (mbsize * (double)blkused) / 
                              (double)blknum;
#ifdef ST_RDONLY                                 
                            if (stfs.f_flag & ST_RDONLY) rdonly = EINA_TRUE;
#endif                                 
                            _e_wid_fprev_preview_fs_widgets(wd, EINA_TRUE);
                            
                            //-------------------
                            if (mbused > 1024.0)
                              snprintf(buf, sizeof(buf), "%1.2f Gb", mbused / 1024.0);
                            else
                              snprintf(buf, sizeof(buf), "%1.2f Mb", mbused);
                            e_widget_entry_text_set(wd->o_preview_extra_entry, buf);
                            //-------------------
                            if (mbsize > 1024.0)
                              snprintf(buf, sizeof(buf), "%1.2f Gb", mbsize / 1024.0);
                            else
                              snprintf(buf, sizeof(buf), "%1.2f Mb", mbsize);
                            e_widget_entry_text_set(wd->o_preview_size_entry, buf);
                            //-------------------
                            if (mbres > 1024.0)
                              snprintf(buf, sizeof(buf), "%1.2f Gb", mbres / 1024.0);
                            else
                              snprintf(buf, sizeof(buf), "%1.2f Mb", mbres);
                            e_widget_entry_text_set(wd->o_preview_owner_entry, buf);
                            //-------------------
                            if (mpoint[0])
                              {
                                 if (rdonly) 
                                   e_widget_entry_text_set(wd->o_preview_perms_entry, _("Read Only"));
                                 else
                                   e_widget_entry_text_set(wd->o_preview_perms_entry, _("Read-Write"));
                              }
                            else
                              e_widget_entry_text_set(wd->o_preview_perms_entry, _("Unmounted"));
                            //-------------------
                            e_widget_entry_text_set(wd->o_preview_time_entry, fstype);
                         }
                    }
                  if (!is_fs)
                    {
                       _e_wid_fprev_preview_fs_widgets(wd, EINA_FALSE);
                       e_widget_entry_text_set(wd->o_preview_extra_entry, _("Unknown"));
                       e_widget_entry_text_set(wd->o_preview_size_entry, _("Unknown"));
                       e_widget_entry_text_set(wd->o_preview_owner_entry, _("Unknown"));
                       e_widget_entry_text_set(wd->o_preview_perms_entry, _("Unmounted"));
                       is_fs = EINA_TRUE;
                    }
                  eina_stringshare_del(file);
               }
          }
        else if ((desktop) && (desktop->url) &&
                 (desktop->type == EFREET_DESKTOP_TYPE_LINK))
          {
             const char *url;
             /* we don't know what the desktop points to here,
              * so we'll just run recursively until we get a real file
              */
             url = e_fm2_desktop_url_eval(desktop->url);
             if (url)
               {
                  eina_stringshare_del(wd->path);
                  wd->path = url;
                  _e_wid_fprev_preview_file(wd);
                  return;
               }
          }
        if (desktop) efreet_desktop_free(desktop);
     }
#ifdef HAVE_EMOTION
   else if (wd->mime && (emotion_object_extension_may_play_get(wd->path)))
     {
        size_t sz;

        _e_wid_fprev_preview_video_widgets(wd);
        e_widget_entry_text_set(wd->o_preview_extra_entry, _("Unknown"));
        sz = ecore_file_size(wd->path);
        if (sz)
          {
             char *s;

             s = e_util_size_string_get(sz);
             e_widget_entry_text_set(wd->o_preview_size_entry, s);
             free(s);
          }
        else
          e_widget_entry_text_set(wd->o_preview_size_entry, _("Unknown"));
        is_fs = EINA_TRUE;
     }
#endif
   if (is_fs) return;

   wd->mime_icon = EINA_FALSE;
   size = _e_wid_file_size_get(st.st_size);
   owner = _e_wid_file_user_get(st.st_uid);
   perms = _e_wid_file_perms_get(st.st_mode, st.st_uid, st.st_gid);
   mtime = _e_wid_file_time_get(st.st_mtime);
   wd->is_dir = S_ISDIR(st.st_mode);
   if (wd->mime && st.st_size && (!wd->is_dir))
     {
        
        wd->is_txt = !strncmp(wd->mime, "text/", 5);
        if (!wd->is_txt)
          wd->is_txt = !strcmp(wd->mime, "application/x-shellscript");
     }
   _e_wid_fprev_preview_file_widgets(wd, wd->is_dir, wd->is_txt);

   _e_wid_fprev_preview_reset(wd);
   _e_wid_fprev_preview_fm(wd);
   _e_wid_fprev_preview_txt(wd);
   _e_wid_fprev_img_update(wd, wd->path, NULL);

   e_widget_size_min_get(wd->o_preview_list, &mw, &mh);
   e_widget_size_min_set(wd->obj, mw, mh);
   if (!wd->is_dir)
     {
        if (!wd->is_txt)
          e_widget_entry_text_set(wd->o_preview_extra_entry, "");
        e_widget_entry_text_set(wd->o_preview_size_entry, size);
     }
   e_widget_entry_text_set(wd->o_preview_owner_entry, owner);
   e_widget_entry_text_set(wd->o_preview_perms_entry, perms);
   e_widget_entry_text_set(wd->o_preview_time_entry, mtime);

   free(size);
   free(owner);
   free(perms);
   free(mtime);
}


static char *
_e_wid_file_size_get(off_t st_size)
{
   return e_util_size_string_get(st_size);
}

static char *
_e_wid_file_user_get(uid_t st_uid)
{
   char name[4096];
   struct passwd *pwd;

   if (getuid() == st_uid)
     snprintf(name, sizeof(name), _("You"));
   else
     {
        pwd = getpwuid(st_uid);
        if (pwd)
          snprintf(name, sizeof(name), "%s", pwd->pw_name);
        else
          snprintf(name, sizeof(name), "%-8d", (int)st_uid);
     }
   return strdup(name);
}

static char *
_e_wid_file_perms_get(mode_t st_mode, uid_t st_uid, gid_t st_gid)
{
   char perms[256];
   int acc = 0;
   int owner = 0;
   int group = 0;
   int user_read = 0;
   int user_write = 0;
   int group_read = 0;
   int group_write = 0;
   int other_read = 0;
   int other_write = 0;

   if (getuid() == st_uid)
     owner = 1;
   if (getgid() == st_gid)
     group = 1;

   if ((S_IRUSR & st_mode) == S_IRUSR)
     user_read = 1;
   if ((S_IWUSR & st_mode) == S_IWUSR)
     user_write = 1;

   if ((S_IRGRP & st_mode) == S_IRGRP)
     group_read = 1;
   if ((S_IWGRP & st_mode) == S_IWGRP)
     group_write = 1;

   if ((S_IROTH & st_mode) == S_IROTH)
     other_read = 1;
   if ((S_IWOTH & st_mode) == S_IWOTH)
     other_write = 1;

   if (owner)
     {
        if ((!user_read) && (!user_write))
          snprintf(perms, sizeof(perms), _("Protected"));
        else if ((user_read) && (!user_write))
          snprintf(perms, sizeof(perms), _("Read Only"));
        else if ((user_read) && (user_write))
          acc = 1;
     }
   else if (group)
     {
        if ((!group_read) && (!group_write))
          snprintf(perms, sizeof(perms), _("Forbidden"));
        else if ((group_read) && (!group_write))
          snprintf(perms, sizeof(perms), _("Read Only"));
        else if ((group_read) && (group_write))
          acc = 1;
     }
   else
     {
        if ((!other_read) && (!other_write))
          snprintf(perms, sizeof(perms), _("Forbidden"));
        else if ((other_read) && (!other_write))
          snprintf(perms, sizeof(perms), _("Read Only"));
        else if ((other_read) && (other_write))
          acc = 1;
     }
   if (!acc)
     return strdup(perms);
   else
     return strdup(_("Read-Write"));
}

static char *
_e_wid_file_time_get(time_t st_modtime)
{
   return e_util_file_time_get(st_modtime);
}

static void
_e_wid_del_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   E_FREE(wd->preview_extra_text);
   E_FREE(wd->preview_size_text);
   E_FREE(wd->preview_owner_text);
   E_FREE(wd->preview_perms_text);
   E_FREE(wd->preview_time_text);
   eina_stringshare_del(wd->path);
   eina_stringshare_del(wd->mime);
   if (wd->preview_text_file_thread) ecore_thread_cancel(wd->preview_text_file_thread);
   wd->preview_text_file_thread = NULL;
   if (wd->monitor) eio_monitor_del(wd->monitor);
   E_FREE_LIST(wd->handlers, ecore_event_handler_del);
   free(wd);
}

static void
_e_wid_fprev_preview_reset(E_Widget_Data *wd)
{
   Evas_Object *o;
   
   evas_object_del(wd->o_preview_scrollframe);
   wd->o_preview_scrollframe = wd->o_preview_preview = NULL;
   if (wd->preview_text_file_thread) ecore_thread_cancel(wd->preview_text_file_thread);
   wd->preview_text_file_thread = NULL;
   if (wd->is_dir || wd->is_txt) return;
   o = e_widget_preview_add(evas_object_evas_get(wd->obj), wd->w, wd->h);
   wd->prev_is_txt = wd->prev_is_fm = EINA_FALSE;
   wd->o_preview_preview = o;
   evas_object_smart_callback_add(o, "preview_update",
                                  _e_wid_fprev_preview_update, wd);
   e_widget_table_object_append(wd->o_preview_preview_table,
                                wd->o_preview_preview,
                                0, 0, 2, 1, 0, 0, 1, 1);
   e_widget_list_object_repack(wd->o_preview_list,
                               wd->o_preview_preview_table,
                               0, 1, 0.5);
   e_widget_list_object_repack(wd->o_preview_list,
                               wd->o_preview_properties_table,
                               1, 1, 0.5);
}


static void
_e_wid_cb_selected(void *data, Evas_Object *obj, void *event __UNUSED__)
{
   evas_object_smart_callback_call(data, "selected", obj);
}

static void
_e_wid_cb_selection_change(void *data, Evas_Object *obj, void *event __UNUSED__)
{
   evas_object_smart_callback_call(data, "selection_change", obj);
}

static void
_e_wid_cb_dir_changed(void *data, Evas_Object *obj, void *event __UNUSED__)
{
   evas_object_smart_callback_call(data, "dir_changed", obj);
}

static void
_e_wid_cb_changed(void *data, Evas_Object *obj, void *event __UNUSED__)
{
   evas_object_smart_callback_call(data, "changed", obj);
}

static void
_e_wid_cb_file_deleted(void *data, Evas_Object *obj, void *event __UNUSED__)
{
   evas_object_smart_callback_call(data, "files_deleted", obj);
}

static void
_e_wid_fprev_preview_txt_read_cancel(void *data __UNUSED__, Ecore_Thread *eth __UNUSED__)
{
}

static void
_e_wid_fprev_preview_txt_read_end(void *data __UNUSED__, Ecore_Thread *eth __UNUSED__)
{
}

static void
_e_wid_fprev_preview_txt_read_notify(void *data, Ecore_Thread *eth __UNUSED__, void *msg)
{
   E_Widget_Data *wd = data;

   //INF("text='%s'", (char*)msg);
   edje_object_part_text_set(wd->o_preview_preview, "e.textblock.message", msg);
   free(msg);
}

static void
_e_wid_fprev_preview_txt_read(void *data __UNUSED__, Ecore_Thread *eth)
{
   char *text;
   char buf[FILEPREVIEW_TEXT_PREVIEW_SIZE + 1];
   FILE *f;
   size_t n;

   text = ecore_thread_global_data_find("fprev_file");
   if (!text) return;
   f = fopen(text, "r");
   if (!f) return;
   n = fread(buf, sizeof(char), FILEPREVIEW_TEXT_PREVIEW_SIZE, f);
   buf[n] = 0;
   ecore_thread_feedback(eth, evas_textblock_text_utf8_to_markup(NULL, buf));
   fclose(f);
}

static void
_e_wid_fprev_preview_txt(E_Widget_Data *wd)
{
   Evas_Object *o;
   int mw;

   if (!wd->is_txt) return;
   if (!wd->path) return;
   if (wd->o_preview_preview && (!wd->prev_is_txt))
     {
        evas_object_del(wd->o_preview_preview);
        wd->o_preview_preview = NULL;
     }
   if (!wd->o_preview_preview)
     {
        Evas *evas;

        evas = evas_object_evas_get(wd->obj);
        o = edje_object_add(evas);
        /* using dialog theme for now because it's simple, common, and doesn't require all
         * themes to be updated
         */
        e_theme_edje_object_set(o, "base/theme/dialog", "e/widgets/dialog/text");
        edje_object_signal_emit(o, "e,state,left", "e");
        edje_object_message_signal_process(o);
        edje_object_part_text_set(wd->o_preview_preview, "e.textblock.message", "");
        wd->o_preview_preview = o;
        wd->prev_is_txt = EINA_TRUE;
        evas_object_resize(o, wd->w, wd->h);
        o = e_widget_scrollframe_simple_add(evas, o);
        wd->o_preview_scrollframe = o;
        e_widget_size_min_get(wd->o_preview_list, &mw, NULL);
        e_widget_size_min_set(o, wd->w, wd->h);
        evas_object_propagate_events_set(wd->o_preview_preview, 0);
        e_widget_table_object_append(wd->o_preview_preview_table,
                                     o, 0, 0, 2, 1, 1, 1, 1, 1);
        e_widget_list_object_repack(wd->o_preview_list,
                                    wd->o_preview_preview_table,
                                    1, 1, 0.5);
        e_widget_list_object_repack(wd->o_preview_list,
                                    wd->o_preview_properties_table,
                                    1, 1, 0.5);
     }
   ecore_thread_global_data_del("fprev_file");
   if (wd->preview_text_file_thread) ecore_thread_cancel(wd->preview_text_file_thread);
   ecore_thread_global_data_add("fprev_file", strdup(wd->path), free, 0);
   wd->preview_text_file_thread = ecore_thread_feedback_run(_e_wid_fprev_preview_txt_read, _e_wid_fprev_preview_txt_read_notify,
                                                            _e_wid_fprev_preview_txt_read_end, _e_wid_fprev_preview_txt_read_cancel, wd, EINA_FALSE);
}

static void
_e_wid_fprev_preview_fm(E_Widget_Data *wd)
{
   E_Fm2_Config fmc;
   Evas_Object *o;
   int mw;

   if (!wd->is_dir) return;
   if (!wd->path) return;
   if (wd->o_preview_preview && (!wd->prev_is_fm))
     {
        evas_object_del(wd->o_preview_preview);
        wd->o_preview_preview = NULL;
     }
   if (!wd->o_preview_preview)
     {
        Evas *evas;

        evas = evas_object_evas_get(wd->obj);
        o = e_fm2_add(evas);
        wd->o_preview_preview = o;
        wd->prev_is_fm = EINA_TRUE;
        memset(&fmc, 0, sizeof(E_Fm2_Config));
        fmc.view.mode = E_FM2_VIEW_MODE_LIST;
        fmc.view.open_dirs_in_place = 1;
        fmc.view.selector = 1;
        fmc.view.single_click = 0;
        fmc.view.no_subdir_jump = 1;
        fmc.view.no_subdir_drop = 1;
        fmc.view.link_drop = 1;
        fmc.icon.list.w = 24;
        fmc.icon.list.h = 24;
        fmc.icon.fixed.w = 1;
        fmc.icon.fixed.h = 1;
        fmc.list.sort.no_case = 1;
        fmc.view.no_click_rename = 1;
        fmc.selection.single = 1;
        e_fm2_config_set(o, &fmc);
        e_fm2_icon_menu_flags_set(o, E_FM2_MENU_NO_VIEW_MENU);
        evas_object_smart_callback_add(o, "dir_changed",
                                       _e_wid_cb_dir_changed, wd->obj);
        evas_object_smart_callback_add(o, "selection_change",
                                       _e_wid_cb_selection_change, wd->obj);
        evas_object_smart_callback_add(o, "changed",
                                       _e_wid_cb_changed, wd->obj);
        evas_object_smart_callback_add(o, "files_deleted",
                                       _e_wid_cb_file_deleted, wd->obj);
        evas_object_smart_callback_add(o, "selected",
                                       _e_wid_cb_selected, wd->obj);
        o = e_widget_scrollframe_pan_add(evas, wd->o_preview_preview,
                                         e_fm2_pan_set,
                                         e_fm2_pan_get,
                                         e_fm2_pan_max_get,
                                         e_fm2_pan_child_size_get);
        wd->o_preview_scrollframe = o;
        e_widget_size_min_get(wd->o_preview_list, &mw, NULL);
//        e_widget_size_min_set(o, MAX(wd->w, mw), wd->h);
        e_widget_size_min_set(o, 0, wd->h);
        evas_object_propagate_events_set(wd->o_preview_preview, 0);
        e_widget_scrollframe_focus_object_set(o, wd->o_preview_preview);
        e_widget_table_object_append(wd->o_preview_preview_table,
                                     o, 0, 0, 2, 1, 1, 1, 1, 1);
        e_widget_list_object_repack(wd->o_preview_list,
                                    wd->o_preview_preview_table,
                                    1, 1, 0.5);
        e_widget_list_object_repack(wd->o_preview_list,
                                    wd->o_preview_properties_table,
                                    1, 1, 0.5);
     }
   e_fm2_path_set(wd->o_preview_preview, "/", wd->path);
}

static Eina_Bool
_e_wid_fprev_cb_del(E_Widget_Data *wd, int type __UNUSED__, Eio_Monitor_Event *ev)
{
   if (wd->monitor != ev->monitor) return ECORE_CALLBACK_RENEW;
   _e_wid_fprev_clear_widgets(wd);
   eio_monitor_del(wd->monitor);
   wd->monitor = NULL;
   E_FREE_LIST(wd->handlers, ecore_event_handler_del);
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_e_wid_fprev_cb_mod(E_Widget_Data *wd, int type __UNUSED__, Eio_Monitor_Event *ev)
{
   if (wd->monitor != ev->monitor) return ECORE_CALLBACK_RENEW;
   _e_wid_fprev_preview_file(wd);
   return ECORE_CALLBACK_RENEW;
}

EAPI Evas_Object *
e_widget_filepreview_add(Evas *evas, int w, int h, int horiz)
{
   Evas_Object *obj, *o;
   int mw, mh;
   E_Widget_Data *wd;

   obj = e_widget_add(evas);

   e_widget_del_hook_set(obj, _e_wid_del_hook);
   wd = E_NEW(E_Widget_Data, 1);
   e_widget_data_set(obj, wd);
   wd->obj = obj;

   wd->w = w, wd->h = h;

   o = e_widget_list_add(evas, 0, horiz);
   wd->o_preview_list = o;
   e_widget_resize_object_set(obj, o);
   e_widget_sub_object_add(obj, o);

   e_widget_size_min_get(wd->o_preview_list, &mw, &mh);
   e_widget_size_min_set(obj, mw, mh);
   evas_object_show(wd->o_preview_preview_table);
   evas_object_show(wd->o_preview_extra);
   evas_object_show(wd->o_preview_extra_entry);
   evas_object_show(wd->o_preview_size);
   evas_object_show(wd->o_preview_size_entry);
   evas_object_show(wd->o_preview_owner);
   evas_object_show(wd->o_preview_owner_entry);
   evas_object_show(wd->o_preview_perms);
   evas_object_show(wd->o_preview_perms_entry);
   evas_object_show(wd->o_preview_time);
   evas_object_show(wd->o_preview_time_entry);
   evas_object_show(wd->o_preview_properties_table);
   evas_object_show(wd->o_preview_list);
   return obj;
}

EAPI void
e_widget_filepreview_path_set(Evas_Object *obj, const char *path, const char *mime)
{
   E_Widget_Data *wd;

   if (!obj) return;
   wd = e_widget_data_get(obj);
   if (!wd) return;
   eina_stringshare_replace(&wd->path, path);
   eina_stringshare_replace(&wd->mime, mime);
   if (wd->monitor) eio_monitor_del(wd->monitor);
   wd->monitor = eio_monitor_stringshared_add(wd->path);
   if (!wd->handlers)
     {
        E_LIST_HANDLER_APPEND(wd->handlers, EIO_MONITOR_FILE_MODIFIED, _e_wid_fprev_cb_mod, wd);
        E_LIST_HANDLER_APPEND(wd->handlers, EIO_MONITOR_FILE_DELETED, _e_wid_fprev_cb_del, wd);
        E_LIST_HANDLER_APPEND(wd->handlers, EIO_MONITOR_ERROR, _e_wid_fprev_cb_del, wd);
        E_LIST_HANDLER_APPEND(wd->handlers, EIO_MONITOR_SELF_DELETED, _e_wid_fprev_cb_del, wd);
     }
   _e_wid_fprev_preview_file(wd);
}

EAPI void
e_widget_filepreview_filemode_force(Evas_Object *obj)
{
   E_Widget_Data *wd;

   if (!obj) return;
   wd = e_widget_data_get(obj);
   if (!wd) return;
   _e_wid_fprev_preview_file_widgets(wd, 0, 0);
}
