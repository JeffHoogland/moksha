#include "e.h"

typedef struct _E_Widget_Data E_Widget_Data;
struct _E_Widget_Data
{
   Evas_Object *obj;
   Evas_Object *o_preview_table;
   Evas_Object *o_preview_preview_table;
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
   char        *preview_extra_text;
   char        *preview_size_text;
   char        *preview_owner_text;
   char        *preview_perms_text;
   char        *preview_time_text;
   const char        *path;
};

static void  _e_wid_fsel_preview_update(void *data, Evas_Object *obj, void *event_info);
static void  _e_wid_fsel_preview_file(E_Widget_Data *wd);
static char *_e_wid_file_size_get(off_t st_size);
static char *_e_wid_file_user_get(uid_t st_uid);
static char *_e_wid_file_perms_get(mode_t st_mode, uid_t st_uid, gid_t gid);
static char *_e_wid_file_time_get(time_t st_modtime);
static void  _e_wid_del_hook(Evas_Object *obj);

static void
_e_wid_fsel_preview_update(void *data, Evas_Object *obj, void *event_info __UNUSED__)
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
        snprintf(buf, sizeof(buf), "%ix%i", iw, ih);
        e_widget_entry_text_set(wd->o_preview_extra_entry, buf);
     }
   e_widget_table_object_repack(wd->o_preview_preview_table,
                                wd->o_preview_preview,
                                0, 0, 1, 1, 0, 0, 1, 1);
}

static void
_e_wid_fsel_preview_file(E_Widget_Data *wd)
{
   char *size, *owner, *perms, *mtime;
   struct stat st;

   if (stat(wd->path, &st) < 0) return;

   size = _e_wid_file_size_get(st.st_size);
   owner = _e_wid_file_user_get(st.st_uid);
   perms = _e_wid_file_perms_get(st.st_mode, st.st_uid, st.st_gid);
   mtime = _e_wid_file_time_get(st.st_mtime);

   e_widget_preview_thumb_set(wd->o_preview_preview, wd->path,
                              "e/desktop/background", 128, 128);

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

   free(wd);
}

EAPI Evas_Object *
e_widget_filepreview_add(Evas *evas)
{
   Evas_Object *obj, *o;
   int mw, mh;
   E_Widget_Data *wd;

   obj = e_widget_add(evas);

   e_widget_del_hook_set(obj, _e_wid_del_hook);
   wd = E_NEW(E_Widget_Data, 1);
   e_widget_data_set(obj, wd);
   wd->obj = obj;


   o = e_widget_table_add(evas, 0);
   wd->o_preview_table = o;
   e_widget_resize_object_set(obj, o);
   e_widget_sub_object_add(obj, o);
   e_widget_table_freeze(o);

   o = e_widget_table_add(evas, 0);
   wd->o_preview_preview_table = o;
   e_widget_sub_object_add(obj, o);

   o = e_widget_preview_add(evas, 128, 128);
   wd->o_preview_preview = o;
   e_widget_sub_object_add(obj, o);
   evas_object_smart_callback_add(o, "preview_update",
                                  _e_wid_fsel_preview_update, wd);
   e_widget_table_object_append(wd->o_preview_preview_table,
                                wd->o_preview_preview,
                                0, 0, 1, 1, 0, 0, 1, 1);
   e_widget_table_object_append(wd->o_preview_table,
                                wd->o_preview_preview_table,
                                0, 0, 2, 1, 0, 0, 1, 1);

   o = e_widget_label_add(evas, _("Resolution:"));
   wd->o_preview_extra = o;
   e_widget_sub_object_add(obj, o);
   e_widget_table_object_append(wd->o_preview_table,
                                wd->o_preview_extra,
                                0, 1, 1, 1, 1, 1, 1, 1);

   o = e_widget_entry_add(evas, &(wd->preview_extra_text), NULL, NULL, NULL);
   e_widget_entry_readonly_set(o, 1);
   e_widget_can_focus_set(obj, 0);
   wd->o_preview_extra_entry = o;
   e_widget_sub_object_add(obj, o);
   e_widget_size_min_set(o, 100, -1);
   e_widget_table_object_append(wd->o_preview_table,
                                wd->o_preview_extra_entry,
                                1, 1, 1, 1, 1, 1, 1, 1);

   o = e_widget_label_add(evas, _("Size:"));
   wd->o_preview_size = o;
   e_widget_sub_object_add(obj, o);
   e_widget_table_object_append(wd->o_preview_table,
                                wd->o_preview_size,
                                0, 2, 1, 1, 1, 1, 1, 1);

   o = e_widget_entry_add(evas, &(wd->preview_size_text), NULL, NULL, NULL);
   e_widget_entry_readonly_set(o, 1);
   e_widget_can_focus_set(obj, 0);
   wd->o_preview_size_entry = o;
   e_widget_sub_object_add(obj, o);
   e_widget_size_min_set(o, 100, -1);
   e_widget_table_object_append(wd->o_preview_table,
                                wd->o_preview_size_entry,
                                1, 2, 1, 1, 1, 1, 1, 1);

   o = e_widget_label_add(evas, _("Owner:"));
   wd->o_preview_owner = o;
   e_widget_sub_object_add(obj, o);
   e_widget_table_object_append(wd->o_preview_table,
                                wd->o_preview_owner,
                                0, 3, 1, 1, 1, 1, 1, 1);

   o = e_widget_entry_add(evas, &(wd->preview_owner_text), NULL, NULL, NULL);
   e_widget_entry_readonly_set(o, 1);
   e_widget_can_focus_set(obj, 0);
   wd->o_preview_owner_entry = o;
   e_widget_sub_object_add(obj, o);
   e_widget_size_min_set(o, 100, -1);
   e_widget_table_object_append(wd->o_preview_table,
                                wd->o_preview_owner_entry,
                                1, 3, 1, 1, 1, 1, 1, 1);

   o = e_widget_label_add(evas, _("Permissions:"));
   wd->o_preview_perms = o;
   e_widget_sub_object_add(obj, o);
   e_widget_table_object_append(wd->o_preview_table,
                                wd->o_preview_perms,
                                0, 4, 1, 1, 1, 1, 1, 1);

   o = e_widget_entry_add(evas, &(wd->preview_perms_text), NULL, NULL, NULL);
   e_widget_entry_readonly_set(o, 1);
   e_widget_can_focus_set(obj, 0);
   wd->o_preview_perms_entry = o;
   e_widget_sub_object_add(obj, o);
   e_widget_size_min_set(o, 100, -1);
   e_widget_table_object_append(wd->o_preview_table,
                                wd->o_preview_perms_entry,
                                1, 4, 1, 1, 1, 1, 1, 1);

   o = e_widget_label_add(evas, _("Modified:"));
   wd->o_preview_time = o;
   e_widget_sub_object_add(obj, o);
   e_widget_table_object_append(wd->o_preview_table,
                                wd->o_preview_time,
                                0, 5, 1, 1, 1, 1, 1, 1);

   o = e_widget_entry_add(evas, &(wd->preview_time_text), NULL, NULL, NULL);
   e_widget_entry_readonly_set(o, 1);
   e_widget_can_focus_set(obj, 0);
   wd->o_preview_time_entry = o;
   e_widget_sub_object_add(obj, o);
   e_widget_size_min_set(o, 100, -1);
   e_widget_table_object_append(wd->o_preview_table,
                                wd->o_preview_time_entry,
                                1, 5, 1, 1, 1, 1, 1, 1);

   e_widget_table_thaw(wd->o_preview_table);
   e_widget_size_min_get(wd->o_preview_table, &mw, &mh);
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
   evas_object_show(wd->o_preview_table);
   return obj;
}

EAPI void
e_widget_filepreview_path_set(Evas_Object *obj, const char *path)
{
   E_Widget_Data *wd;

   if (!obj) return;
   wd = e_widget_data_get(obj);
   if (!wd) return;
   eina_stringshare_replace(&wd->path, path);
   _e_wid_fsel_preview_file(wd);
}
