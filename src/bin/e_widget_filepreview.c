#include "e.h"

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
};

static void  _e_wid_fprev_preview_update(void *data, Evas_Object *obj, void *event_info);
static void  _e_wid_fprev_preview_file(E_Widget_Data *wd, const char *path);
static char *_e_wid_file_size_get(off_t st_size);
static char *_e_wid_file_user_get(uid_t st_uid);
static char *_e_wid_file_perms_get(mode_t st_mode, uid_t st_uid, gid_t gid);
static char *_e_wid_file_time_get(time_t st_modtime);
static void _e_wid_fprev_img_update(E_Widget_Data *wd, const char *path, const char *key);
static void  _e_wid_del_hook(Evas_Object *obj);

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
   e_widget_preview_thumb_set(wd->o_preview_preview, path, key, wd->w, wd->h);
}

static void
_e_wid_fprev_preview_file(E_Widget_Data *wd, const char *path)
{
   char *size, *owner, *perms, *mtime;
   struct stat st;

   if (stat(wd->path, &st) < 0) return;
   wd->mime_icon = EINA_FALSE;
   size = _e_wid_file_size_get(st.st_size);
   owner = _e_wid_file_user_get(st.st_uid);
   perms = _e_wid_file_perms_get(st.st_mode, st.st_uid, st.st_gid);
   mtime = _e_wid_file_time_get(st.st_mtime);
   wd->is_dir = S_ISDIR(st.st_mode);

   _e_wid_fprev_img_update(wd, path, NULL);
   e_widget_table_object_repack(wd->o_preview_preview_table,
                                wd->o_preview_preview,
                                0, 0, 1, 1, 0, 0, 1, 1);

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

   o = e_widget_table_add(evas, 0);
   e_widget_disabled_set(o, 1);
   wd->o_preview_preview_table = o;
   e_widget_sub_object_add(obj, o);

   o = e_widget_preview_add(evas, w, h);
   e_widget_disabled_set(o, 1);
   wd->o_preview_preview = o;
   e_widget_sub_object_add(obj, o);
   evas_object_smart_callback_add(o, "preview_update",
                                  _e_wid_fprev_preview_update, wd);
   e_widget_table_object_append(wd->o_preview_preview_table,
                                wd->o_preview_preview,
                                0, 0, 1, 1, 0, 0, 1, 1);
   e_widget_list_object_append(wd->o_preview_list,
                               wd->o_preview_preview_table,
                               0, 1, 0.5);

   o = e_widget_table_add(evas, 0);
   e_widget_disabled_set(o, 1);
   wd->o_preview_properties_table = o;
   e_widget_sub_object_add(obj, o);

   o = e_widget_label_add(evas, _("Resolution:"));
   e_widget_disabled_set(o, 1);
   wd->o_preview_extra = o;
   e_widget_sub_object_add(obj, o);
   e_widget_table_object_append(wd->o_preview_properties_table,
                                wd->o_preview_extra,
                                0, 0, 1, 1, 1, 1, 1, 1);

   o = e_widget_entry_add(evas, &(wd->preview_extra_text), NULL, NULL, NULL);
   e_widget_entry_readonly_set(o, 1);
   e_widget_disabled_set(o, 1);
   wd->o_preview_extra_entry = o;
   e_widget_sub_object_add(obj, o);
   e_widget_size_min_set(o, 100, -1);
   e_widget_table_object_append(wd->o_preview_properties_table,
                                wd->o_preview_extra_entry,
                                1, 0, 1, 1, 1, 1, 1, 1);

   o = e_widget_label_add(evas, _("Size:"));
   e_widget_disabled_set(o, 1);
   wd->o_preview_size = o;
   e_widget_sub_object_add(obj, o);
   e_widget_table_object_append(wd->o_preview_properties_table,
                                wd->o_preview_size,
                                0, 1, 1, 1, 1, 1, 1, 1);

   o = e_widget_entry_add(evas, &(wd->preview_size_text), NULL, NULL, NULL);
   e_widget_entry_readonly_set(o, 1);
   e_widget_disabled_set(o, 1);
   wd->o_preview_size_entry = o;
   e_widget_sub_object_add(obj, o);
   e_widget_size_min_set(o, 100, -1);
   e_widget_table_object_append(wd->o_preview_properties_table,
                                wd->o_preview_size_entry,
                                1, 1, 1, 1, 1, 1, 1, 1);

   o = e_widget_label_add(evas, _("Owner:"));
   e_widget_disabled_set(o, 1);
   wd->o_preview_owner = o;
   e_widget_sub_object_add(obj, o);
   e_widget_table_object_append(wd->o_preview_properties_table,
                                wd->o_preview_owner,
                                0, 2, 1, 1, 1, 1, 1, 1);

   o = e_widget_entry_add(evas, &(wd->preview_owner_text), NULL, NULL, NULL);
   e_widget_entry_readonly_set(o, 1);
   e_widget_disabled_set(o, 1);
   wd->o_preview_owner_entry = o;
   e_widget_sub_object_add(obj, o);
   e_widget_size_min_set(o, 100, -1);
   e_widget_table_object_append(wd->o_preview_properties_table,
                                wd->o_preview_owner_entry,
                                1, 2, 1, 1, 1, 1, 1, 1);

   o = e_widget_label_add(evas, _("Permissions:"));
   e_widget_disabled_set(o, 1);
   wd->o_preview_perms = o;
   e_widget_sub_object_add(obj, o);
   e_widget_table_object_append(wd->o_preview_properties_table,
                                wd->o_preview_perms,
                                0, 3, 1, 1, 1, 1, 1, 1);

   o = e_widget_entry_add(evas, &(wd->preview_perms_text), NULL, NULL, NULL);
   e_widget_entry_readonly_set(o, 1);
   e_widget_disabled_set(o, 1);
   wd->o_preview_perms_entry = o;
   e_widget_sub_object_add(obj, o);
   e_widget_size_min_set(o, 100, -1);
   e_widget_table_object_append(wd->o_preview_properties_table,
                                wd->o_preview_perms_entry,
                                1, 3, 1, 1, 1, 1, 1, 1);

   o = e_widget_label_add(evas, _("Modified:"));
   e_widget_disabled_set(o, 1);
   wd->o_preview_time = o;
   e_widget_sub_object_add(obj, o);
   e_widget_table_object_append(wd->o_preview_properties_table,
                                wd->o_preview_time,
                                0, 4, 1, 1, 1, 1, 1, 1);

   o = e_widget_entry_add(evas, &(wd->preview_time_text), NULL, NULL, NULL);
   e_widget_entry_readonly_set(o, 1);
   e_widget_disabled_set(o, 1);
   wd->o_preview_time_entry = o;
   e_widget_sub_object_add(obj, o);
   e_widget_size_min_set(o, 100, -1);
   e_widget_table_object_append(wd->o_preview_properties_table,
                                wd->o_preview_time_entry,
                                1, 4, 1, 1, 1, 1, 1, 1);

   e_widget_list_object_append(wd->o_preview_list,
                               wd->o_preview_properties_table,
                               1, 1, 0.5);

   e_widget_size_min_get(wd->o_preview_list, &mw, &mh);
   e_widget_size_min_set(obj, mw, mh);
   evas_object_show(wd->o_preview_preview);
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
   _e_wid_fprev_preview_file(wd, wd->path);
}
