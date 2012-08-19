#include "e.h"
#include "e_fm_device.h"
#include <sys/statvfs.h>

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
   char        *preview_extra_text;
   char        *preview_size_text;
   char        *preview_owner_text;
   char        *preview_perms_text;
   char        *preview_time_text;
   const char        *path;
   const char        *mime;
   Eina_Bool mime_icon : 1;
   Eina_Bool is_dir : 1;
   Eina_Bool prev_is_fm : 1;
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
static void _e_wid_fprev_preview_fm(E_Widget_Data *wd);

static void
_e_wid_fprev_preview_update(void *data, Evas_Object *obj, void *event_info __UNUSED__)
{
   E_Widget_Data *wd;
   Evas_Object *o;
   char buf[256];
   int iw = 0, ih = 0;

   wd = data;
   o = evas_object_image_add(evas_object_evas_get(obj));
   evas_object_image_file_set(o, wd->path, NULL);
   evas_object_image_size_get(o, &iw, &ih);
   evas_object_del(o);
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
   if (wd->is_dir) return;
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
}

static void
_e_wid_fprev_preview_fs_widgets(E_Widget_Data *wd)
{
   Evas *evas = evas_object_evas_get(wd->obj);
   Evas_Object *o;
   int mw, mh, y = 0;
   
   _e_wid_fprev_clear_widgets(wd);

   o = e_widget_table_add(evas, 0);
   e_widget_disabled_set(o, 1);
   wd->o_preview_properties_table = o;

#define WIDROW(lab, labob, entob, entw) \
   do { \
      o = e_widget_label_add(evas, lab); \
      e_widget_disabled_set(o, 1); \
      wd->labob = o; \
      e_widget_table_object_append(wd->o_preview_properties_table, \
                                   wd->labob, \
                                   0, y, 1, 1, 1, 1, 1, 1); \
      o = e_widget_entry_add(evas, &(wd->preview_extra_text), NULL, NULL, NULL); \
      e_widget_entry_readonly_set(o, 1); \
      e_widget_disabled_set(o, 1); \
      wd->entob = o; \
      e_widget_size_min_set(o, entw, -1); \
      e_widget_table_object_append(wd->o_preview_properties_table, \
                                   wd->entob, \
                                   1, y, 1, 1, 1, 1, 1, 1); \
      y++; \
   } while (0)
   
   WIDROW(_("Used:"), o_preview_extra, o_preview_extra_entry, 100);
   WIDROW(_("Size:"), o_preview_size, o_preview_size_entry, 100);
   WIDROW(_("Reserved:"), o_preview_owner, o_preview_owner_entry, 100);
   WIDROW(_("Mount status:"), o_preview_perms, o_preview_perms_entry, 100);
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
_e_wid_fprev_preview_file_widgets(E_Widget_Data *wd)
{
   Evas *evas = evas_object_evas_get(wd->obj);
   Evas_Object *o;
   int mw, mh, y = 0;
   
   _e_wid_fprev_clear_widgets(wd);

   o = e_widget_table_add(evas, 0);
   e_widget_disabled_set(o, 1);
   wd->o_preview_preview_table = o;
   e_widget_size_min_set(o, 32, 32);

   e_widget_list_object_append(wd->o_preview_list,
                               wd->o_preview_preview_table,
                               0, 1, 0.5);

   o = e_widget_table_add(evas, 0);
   e_widget_disabled_set(o, 1);
   wd->o_preview_properties_table = o;

   WIDROW(_("Resolution:"), o_preview_extra, o_preview_extra_entry, 100);
   WIDROW(_("Size:"), o_preview_size, o_preview_size_entry, 100);
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
                       if (v)
                         {
                            if (statvfs(v->mount_point, &stfs) == 0)
                              {
                                 ok = EINA_TRUE;
                                 eina_stringshare_del(file);
                                 file = eina_stringshare_add(v->mount_point);
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
                            _e_wid_fprev_preview_fs_widgets(wd);
                            
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
                                   e_widget_entry_text_set(wd->o_preview_perms_entry, _("Read / Write"));
                              }
                            else
                              e_widget_entry_text_set(wd->o_preview_perms_entry, _("Unmounted"));
                            //-------------------
                            e_widget_entry_text_set(wd->o_preview_time_entry, fstype);
                         }
                    }
                  if (!is_fs)
                    {
                       _e_wid_fprev_preview_fs_widgets(wd);
                       e_widget_entry_text_set(wd->o_preview_extra_entry, _("Unknown"));
                       e_widget_entry_text_set(wd->o_preview_size_entry, _("Unknown"));
                       e_widget_entry_text_set(wd->o_preview_owner_entry, _("Unknown"));
                       e_widget_entry_text_set(wd->o_preview_perms_entry, _("Unmounted"));
                       e_widget_entry_text_set(wd->o_preview_time_entry, _("Unknown"));
                       is_fs = EINA_TRUE;
                    }
                  eina_stringshare_del(file);
               }
          }
        if (desktop) efreet_desktop_free(desktop);
     }
   if (is_fs) return;
   _e_wid_fprev_preview_file_widgets(wd);
   
   wd->mime_icon = EINA_FALSE;
   size = _e_wid_file_size_get(st.st_size);
   owner = _e_wid_file_user_get(st.st_uid);
   perms = _e_wid_file_perms_get(st.st_mode, st.st_uid, st.st_gid);
   mtime = _e_wid_file_time_get(st.st_mtime);
   wd->is_dir = S_ISDIR(st.st_mode);

   _e_wid_fprev_preview_reset(wd);
   _e_wid_fprev_preview_fm(wd);
   _e_wid_fprev_img_update(wd, wd->path, NULL);

   e_widget_size_min_get(wd->o_preview_list, &mw, &mh);
   e_widget_size_min_set(wd->obj, mw, mh);
   e_widget_entry_text_set(wd->o_preview_extra_entry, "");
   e_widget_entry_text_set(wd->o_preview_size_entry, size);
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

   free(wd);
}

static void
_e_wid_fprev_preview_reset(E_Widget_Data *wd)
{
   Evas_Object *o;
   if (wd->is_dir) return;
   if (wd->o_preview_scrollframe && wd->prev_is_fm)
     {
        evas_object_del(wd->o_preview_scrollframe);
        wd->o_preview_scrollframe = wd->o_preview_preview = NULL;
     }
   if (wd->o_preview_preview) return;
   o = e_widget_preview_add(evas_object_evas_get(wd->obj), wd->w, wd->h);
   wd->prev_is_fm = EINA_FALSE;
   e_widget_disabled_set(o, 1);
   wd->o_preview_preview = o;
   evas_object_smart_callback_add(o, "preview_update",
                                  _e_wid_fprev_preview_update, wd);
   e_widget_table_object_append(wd->o_preview_preview_table,
                                wd->o_preview_preview,
                                0, 0, 1, 1, 0, 0, 1, 1);
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
        e_widget_size_min_set(o, MAX(wd->w, mw), wd->h);
        evas_object_propagate_events_set(wd->o_preview_preview, 0);
        e_widget_scrollframe_focus_object_set(o, wd->o_preview_preview);
        e_widget_table_object_append(wd->o_preview_preview_table,
                                     o, 0, 0, 1, 1, 0, 0, 1, 1);
        e_widget_list_object_repack(wd->o_preview_list,
                                    wd->o_preview_preview_table,
                                    0, 1, 0.5);
        e_widget_list_object_repack(wd->o_preview_list,
                                    wd->o_preview_properties_table,
                                    1, 1, 0.5);
     }
   e_fm2_path_set(wd->o_preview_preview, "/", wd->path);
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
   e_widget_disabled_set(o, 1);
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
   e_widget_disabled_set(obj, 1);
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
   _e_wid_fprev_preview_file(wd);
}
