/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

typedef struct _E_Widget_Data E_Widget_Data;
struct _E_Widget_Data
{
   Evas_Object *obj;
   Evas_Object *o_table;
   Evas_Object *o_table2;
   Evas_Object *o_preview_table;
   Evas_Object *o_preview_preview_table;
   Evas_Object *o_preview_frame;
   Evas_Object *o_preview_scroll;
   Evas_Object *o_preview_size;
   Evas_Object *o_preview_size_entry;
   Evas_Object *o_preview_owner;
   Evas_Object *o_preview_owner_entry;
   Evas_Object *o_preview_perms;
   Evas_Object *o_preview_perms_entry;
   Evas_Object *o_preview_time;
   Evas_Object *o_preview_time_entry;
   Evas_Object *o_preview_preview;
   Evas_Object *o_up_button;
   Evas_Object *o_favorites_frame;
   Evas_Object *o_favorites_fm;
   Evas_Object *o_files_frame;
   Evas_Object *o_files_fm;
   Evas_Object *o_entry;
   char *entry_text;
   char *preview_size_text;
   char *preview_owner_text;
   char *preview_perms_text;
   char *preview_time_text;
   char *path;
   void (*sel_func) (void *data, Evas_Object *obj);
   void *sel_data;
   void (*chg_func) (void *data, Evas_Object *obj);
   void *chg_data;
   int preview;
};

static void _e_wid_fsel_preview_file(E_Widget_Data *wd);
static char *_e_wid_file_size_get(off_t st_size);
static char *_e_wid_file_user_get(uid_t st_uid);
static char *_e_wid_file_perms_get(mode_t st_mode, uid_t st_uid, gid_t gid);
static char *_e_wid_file_time_get(time_t st_modtime);
static void _e_wid_del_hook(Evas_Object *obj);

/* local subsystem functions */
static void
_e_wid_fsel_button_up(void *data1, void *data2)
{
   E_Widget_Data *wd;
   
   wd = data1;
   if (wd->o_files_fm)
     e_fm2_parent_go(wd->o_files_fm);
   if (wd->o_files_frame)
     e_widget_scrollframe_child_pos_set(wd->o_files_frame, 0, 0);
}

static void
_e_wid_fsel_favorites_files_changed(void *data, Evas_Object *obj, void *event_info)
{
   E_Widget_Data *wd;
   Evas_List *icons, *l;
   E_Fm2_Icon_Info *ici;
   const char *realpath;
   char *p1, *p2;
   
   wd = data;
   if (!wd->o_favorites_fm) return;
   if (!wd->o_files_fm) return;
   icons = e_fm2_all_list_get(wd->o_favorites_fm);
   if (!icons) return;
   realpath = e_fm2_real_path_get(wd->o_files_fm);
   p1 = ecore_file_realpath(realpath);
   if (!p1) goto done;
   for (l = icons; l; l = l->next)
     {
	ici = l->data;
	if (ici->link)
	  {
	     p2 = ecore_file_realpath(ici->link);
	     if (p2)
	       {
		  if (!strcmp(p1, p2))
		    {
		       e_fm2_select_set(wd->o_favorites_fm, (char *)ici->file, 1);
		       E_FREE(p2);
		       goto done;
		    }
		  E_FREE(p2);
	       }
	  }
     }
   done:
   E_FREE(p1);
   evas_list_free(icons);
}

static void
_e_wid_fsel_favorites_selected(void *data, Evas_Object *obj, void *event_info)
{
   E_Widget_Data *wd;
   Evas_List *selected;
   E_Fm2_Icon_Info *ici;
   
   wd = data;
   if (!wd->o_favorites_fm) return;
   if (!wd->o_files_frame) return;
   selected = e_fm2_selected_list_get(wd->o_favorites_fm);
   if (!selected) return;
   ici = selected->data;
   if ((ici->link) && (ici->mount))
     e_fm2_path_set(wd->o_files_fm, (char *)ici->link, "/");
   else if (ici->link)
     e_fm2_path_set(wd->o_files_fm, NULL, (char *)ici->link);
   evas_list_free(selected);
   e_widget_scrollframe_child_pos_set(wd->o_files_frame, 0, 0);
}

static void
_e_wid_fsel_files_changed(void *data, Evas_Object *obj, void *event_info)
{
   E_Widget_Data *wd;
   
   wd = data;
   if (!wd->o_files_fm) return;
   if (!e_fm2_has_parent_get(wd->o_files_fm))
     {
	if (wd->o_up_button)
	  e_widget_disabled_set(wd->o_up_button, 1);
     }
   else
     {
	if (wd->o_up_button)
	  e_widget_disabled_set(wd->o_up_button, 0);
     }
   if (wd->o_files_frame)
     e_widget_scrollframe_child_pos_set(wd->o_files_frame, 0, 0);
   E_FREE(wd->path);
   if (wd->chg_func) wd->chg_func(wd->chg_data, wd->obj);
}

static void
_e_wid_fsel_files_selection_change(void *data, Evas_Object *obj, void *event_info)
{
   E_Widget_Data *wd;
   Evas_List *selected;
   E_Fm2_Icon_Info *ici;
   const char *realpath;
   char buf[4096];
   
   wd = data;
   if (!wd->o_files_fm) return;
   selected = e_fm2_selected_list_get(wd->o_files_fm);
   if (!selected) return;
   ici = selected->data;
   E_FREE(wd->path);
   realpath = e_fm2_real_path_get(wd->o_files_fm);
   if (!strcmp(realpath, "/"))
     {
	snprintf(buf, sizeof(buf), "/%s", ici->file);
     }
   else
     {
	snprintf(buf, sizeof(buf), "%s/%s",
		 realpath, ici->file);
     }
   wd->path = strdup(buf);
   if (wd->preview)
     _e_wid_fsel_preview_file(wd);
   e_widget_entry_text_set(wd->o_entry, ici->file);
   evas_list_free(selected);
   if (wd->chg_func) wd->chg_func(wd->chg_data, wd->obj);
}

static void
_e_wid_fsel_files_selected(void *data, Evas_Object *obj, void *event_info)
{
   E_Widget_Data *wd;
   
   wd = data;
   if (wd->path)
     {
	if (wd->sel_func) wd->sel_func(wd->sel_data, wd->obj);
     }
}

/* externally accessible functions */
EAPI Evas_Object *
e_widget_fsel_add(Evas *evas, char *dev, char *path, char *selected, char *filter, 
		  void (*sel_func) (void *data, Evas_Object *obj), void *sel_data,
		  void (*chg_func) (void *data, Evas_Object *obj), void *chg_data,
		  int preview)
{
   Evas_Object *obj, *o;
   E_Widget_Data *wd;
   Evas_Coord mw = 0, mh = 0;
   E_Fm2_Config fmc;
   
   obj = e_widget_add(evas);
   
   e_widget_del_hook_set(obj, _e_wid_del_hook);
   wd = calloc(1, sizeof(E_Widget_Data));
   e_widget_data_set(obj, wd);

   wd->obj = obj;
   wd->sel_func = sel_func;
   wd->sel_data = sel_data;
   wd->chg_func = chg_func;
   wd->chg_data = chg_data;
   wd->preview = preview;
   
   o = e_widget_table_add(evas, 0);
   wd->o_table = o;
   e_widget_sub_object_add(obj, o);
   e_widget_resize_object_set(obj, o);
   
   o = e_widget_table_add(evas, 0);
   wd->o_table2 = o;
   e_widget_sub_object_add(obj, o);
   
   o = e_widget_button_add(evas, _("Go up a Directory"), "widget/up_dir",
			   _e_wid_fsel_button_up, wd, NULL);
   wd->o_up_button = o;
   e_widget_sub_object_add(obj, o);
   e_widget_table_object_append(wd->o_table2, o, 1, 0, 1, 1, 0, 0, 1, 0);

   if (preview)
     {
        o = e_widget_frametable_add(evas, "Preview", 0);
        wd->o_preview_frame = o;
        e_widget_sub_object_add(obj, o);

        o = e_widget_table_add(evas, 0);
        wd->o_preview_preview_table = o;
        e_widget_sub_object_add(obj, o);

        o = e_widget_table_add(evas, 0);
        wd->o_preview_table = o;
        e_widget_sub_object_add(obj, o);

        o = e_widget_preview_add(evas, 64, 64);
        wd->o_preview_preview = o;
        e_widget_sub_object_add(obj, o);
        e_widget_table_object_append(wd->o_preview_preview_table,
                                     wd->o_preview_preview,
                                     0, 0, 1, 1, 1, 1, 1, 1);

        o = e_widget_label_add(evas, "Size:");
        wd->o_preview_size = o;
        e_widget_sub_object_add(obj, o);
        e_widget_table_object_append(wd->o_preview_table, wd->o_preview_size,
                                     0, 0, 1, 1, 1, 1, 1, 1);

        o = e_widget_entry_add(evas, &(wd->preview_size_text));
        wd->o_preview_size_entry = o;
        e_widget_sub_object_add(obj, o);
        e_widget_min_size_set(o, 100, 15);
        e_widget_table_object_append(wd->o_preview_table,
                        wd->o_preview_size_entry, 1, 0, 1, 1, 1, 1, 1, 1);

        o = e_widget_label_add(evas, "Owner:");
        wd->o_preview_owner = o;
        e_widget_sub_object_add(obj, o);
        e_widget_table_object_append(wd->o_preview_table, wd->o_preview_owner,
                                     0, 1, 1, 1, 1, 1, 1, 1);

        o = e_widget_entry_add(evas, &(wd->preview_owner_text));
        wd->o_preview_owner_entry = o;
        e_widget_sub_object_add(obj, o);
        e_widget_min_size_set(o, 100, 15);
        e_widget_table_object_append(wd->o_preview_table,
                        wd->o_preview_owner_entry, 1, 1, 1, 1, 1, 1, 1, 1);

        o = e_widget_label_add(evas, "Permissions:");
        wd->o_preview_perms = o;
        e_widget_sub_object_add(obj, o);
        e_widget_table_object_append(wd->o_preview_table, wd->o_preview_perms,
                                     0, 2, 1, 1, 1, 1, 1, 1);

        o = e_widget_entry_add(evas, &(wd->preview_perms_text));
        wd->o_preview_perms_entry = o;
        e_widget_sub_object_add(obj, o);
        e_widget_min_size_set(o, 100, 15);
        e_widget_table_object_append(wd->o_preview_table,
                        wd->o_preview_perms_entry, 1, 2, 1, 1, 1, 1, 1, 1);

        o = e_widget_label_add(evas, "Modified:");
        wd->o_preview_time = o;
        e_widget_sub_object_add(obj, o);
        e_widget_table_object_append(wd->o_preview_table, wd->o_preview_time,
                                     0, 3, 1, 1, 1, 1, 1, 1);

        o = e_widget_entry_add(evas, &(wd->preview_time_text));
        wd->o_preview_time_entry = o;
        e_widget_sub_object_add(obj, o); 
        e_widget_min_size_set(o, 100, 15);
        e_widget_table_object_append(wd->o_preview_table,
                        wd->o_preview_time_entry, 1, 3, 1, 1, 1, 1, 1, 1);

     }


   o = e_fm2_add(evas);
   wd->o_favorites_fm = o;
   e_widget_sub_object_add(obj, o);
   memset(&fmc, 0, sizeof(E_Fm2_Config));
   fmc.view.mode = E_FM2_VIEW_MODE_LIST;
   fmc.view.open_dirs_in_place = 1;
   fmc.view.selector = 1;
   fmc.view.single_click = 1;
   fmc.view.no_subdir_jump = 1;
   fmc.icon.list.w = 24;
   fmc.icon.list.h = 24;
   fmc.icon.fixed.w = 1;
   fmc.icon.fixed.h = 1;
   fmc.icon.extension.show = 0;
   fmc.list.sort.no_case = 1;
   fmc.list.sort.dirs.first = 0;
   fmc.list.sort.dirs.last = 0;
   fmc.selection.single = 1;
   fmc.selection.windows_modifiers = 0;
   e_fm2_config_set(o, &fmc);
   evas_object_smart_callback_add(o, "changed", 
				  _e_wid_fsel_favorites_files_changed, wd);
   evas_object_smart_callback_add(o, "selected", 
				  _e_wid_fsel_favorites_selected, wd);
   e_fm2_path_set(o, "favorites", "/");
   
   o = e_widget_scrollframe_pan_add(evas, wd->o_favorites_fm,
				    e_fm2_pan_set, 
				    e_fm2_pan_get,
				    e_fm2_pan_max_get, 
				    e_fm2_pan_child_size_get);
   wd->o_favorites_frame = o;
   e_widget_sub_object_add(obj, o);
   e_widget_min_size_set(o, 128, 128);
   e_widget_table_object_append(wd->o_table2, o, 0, 1, 1, 1, 0, 1, 0, 1);

   o = e_fm2_add(evas);
   wd->o_files_fm = o;
   e_widget_sub_object_add(obj, o);
   memset(&fmc, 0, sizeof(E_Fm2_Config));
   fmc.view.mode = E_FM2_VIEW_MODE_LIST;
   fmc.view.open_dirs_in_place = 1;
   fmc.view.selector = 1;
   fmc.view.single_click = 0;
   fmc.view.no_subdir_jump = 0;
   fmc.icon.list.w = 24;
   fmc.icon.list.h = 24;
   fmc.icon.fixed.w = 1;
   fmc.icon.fixed.h = 1;
   fmc.icon.extension.show = 1;
   fmc.list.sort.no_case = 1;
   fmc.list.sort.dirs.first = 1;
   fmc.list.sort.dirs.last = 0;
   fmc.selection.single = 1;
   fmc.selection.windows_modifiers = 0;
   e_fm2_config_set(o, &fmc);
   evas_object_smart_callback_add(o, "dir_changed", 
				  _e_wid_fsel_files_changed, wd);
   evas_object_smart_callback_add(o, "selection_change", 
				  _e_wid_fsel_files_selection_change, wd);
   evas_object_smart_callback_add(o, "selected", 
				  _e_wid_fsel_files_selected, wd);
   e_fm2_path_set(o, dev, path);
   
   o = e_widget_scrollframe_pan_add(evas, wd->o_files_fm,
				    e_fm2_pan_set,
				    e_fm2_pan_get,
				    e_fm2_pan_max_get,
				    e_fm2_pan_child_size_get);
   wd->o_files_frame = o;
   e_widget_sub_object_add(obj, o);
   e_widget_min_size_set(o, 128, 128);
   e_widget_table_object_append(wd->o_table2, o, 1, 1, 1, 1, 1, 1, 1, 1);

   o = e_widget_entry_add(evas, &(wd->entry_text));
   wd->o_entry = o;
   e_widget_sub_object_add(obj, o);
   
  if (preview)
    {
	e_widget_frametable_object_append(wd->o_preview_frame, 
					  wd->o_preview_preview_table,
					  0, 0, 1, 1, 1, 1, 1, 1);
        e_widget_frametable_object_append(wd->o_preview_frame, 
					  wd->o_preview_table,
					  0, 1, 1, 1, 1, 1, 1, 1);
        e_widget_table_object_append(wd->o_table2, 
				     wd->o_preview_frame,
                                     2, 1, 1, 1, 1, 1, 1, 1);
    }

   e_widget_table_object_append(wd->o_table, wd->o_table2,
                                0, 0, 1, 1, 1, 1, 1, 1);
   e_widget_table_object_append(wd->o_table, wd->o_entry,
                                0, 1, 1, 1, 1, 0, 1, 0);

   e_widget_min_size_get(wd->o_table, &mw, &mh);
   e_widget_min_size_set(obj, mw, mh);
   
   evas_object_show(wd->o_up_button);
   evas_object_show(wd->o_favorites_frame);
   evas_object_show(wd->o_favorites_fm);
   evas_object_show(wd->o_files_frame);
   evas_object_show(wd->o_files_fm);
   evas_object_show(wd->o_entry);
   if (preview)
     {
        evas_object_show(wd->o_preview_preview);
        evas_object_show(wd->o_preview_preview_table);
        evas_object_show(wd->o_preview_size);
        evas_object_show(wd->o_preview_size_entry);
        evas_object_show(wd->o_preview_owner);
        evas_object_show(wd->o_preview_owner_entry);
        evas_object_show(wd->o_preview_perms);
        evas_object_show(wd->o_preview_perms_entry);
        evas_object_show(wd->o_preview_time);
        evas_object_show(wd->o_preview_time_entry);
        evas_object_show(wd->o_preview_table);
        evas_object_show(wd->o_preview_frame);
      }
   evas_object_show(wd->o_table2);
   evas_object_show(wd->o_table);
   return obj;
}

EAPI void
e_widget_fsel_path_get(Evas_Object *obj, const char **dev, const char **path)
{
   E_Widget_Data *wd;

   if (!obj) return;
   wd = e_widget_data_get(obj);
   return e_fm2_path_get(wd->o_files_fm, dev, path);
}

EAPI const char *
e_widget_fsel_selection_path_get(Evas_Object *obj)
{
   E_Widget_Data *wd;

   if (!obj) return NULL;
   wd = e_widget_data_get(obj);
   return wd->path;
}

static void
_e_wid_fsel_preview_file(E_Widget_Data *wd)
{
   Evas_Coord mw = 0, mh = 0;
   char *size, *owner, *perms, *time;
   struct stat st;
 
   stat(wd->path, &st);

   size =  _e_wid_file_size_get(st.st_size);
   owner = _e_wid_file_user_get(st.st_uid);
   perms = _e_wid_file_perms_get(st.st_mode, st.st_uid, st.st_gid);
   time = _e_wid_file_time_get(st.st_mtime); 
   
   e_widget_preview_thumb_set(wd->o_preview_preview, wd->path, "background", 120, 120);
   
   /*e_table_unpack(wd->o_preview_preview);
   e_table_pack(wd->o_preview_preview_table, wd->o_preview_preview, 0, 0, 1, 1);
   e_widget_min_size_get(wd->o_preview_preview, &mw, &mh);
   e_table_pack_options_set(wd->o_preview_preview, 1, 1, 1, 1, 0.5, 0.5, mw, mh, 99999, 99999);
   e_table_min_size_get(wd->o_preview_preview_table, &mw, &mh);
   e_widget_min_size_set(wd->o_preview_preview_table, mw, mh);*/
   
   e_widget_entry_text_set(wd->o_preview_size_entry, size);
   e_widget_entry_text_set(wd->o_preview_owner_entry, owner);
   e_widget_entry_text_set(wd->o_preview_perms_entry, perms);
   e_widget_entry_text_set(wd->o_preview_time_entry, time);

   free(size);
   free(owner);
   free(perms);
   free(time);
}

static char *
_e_wid_file_size_get(off_t st_size)
{
   double dsize;
   char size[1024];

   dsize = (double)st_size;
   if (dsize < 1024) 
      snprintf(size, 1024, "%'.0f B", dsize);
   else
     {
       dsize /= 1024.0;
       if (dsize < 1024) 
          snprintf(size, 1024, "%'.0f KB", dsize);
       else
         {
           dsize /= 1024.0;
           if (dsize < 1024) 
              snprintf(size, 1024, "%'.0f MB", dsize);
           else
             {
               dsize /= 1024.0;
               snprintf(size, 1024, "%'.1f gb", dsize);
             }
         }
     }
   return strdup(size); 
}

static char * 
_e_wid_file_user_get(uid_t st_uid)
{
   char name[PATH_MAX];
   struct passwd *pwd;

   if (getuid() == st_uid) 
      snprintf(name, PATH_MAX, "You");
   else
     {
       pwd = getpwuid(st_uid);
       if (pwd) 
          snprintf(name, PATH_MAX, "%s", pwd->pw_name);
       else 
          snprintf(name, PATH_MAX, "%-8d", (int)st_uid);
     }
   return strdup(name);
}

static char *
_e_wid_file_perms_get(mode_t st_mode, uid_t st_uid, gid_t st_gid)
{
   char *perm;
   char perms[PATH_MAX];
   int access = 0;
   int owner = 0;
   int group = 0;
   int user_read = 0;
   int user_write = 0;
   int group_read = 0;
   int group_write = 0;
   int other_read = 0;
   int other_write = 0;
   int i;

   if (getuid() == st_uid) 
      owner = 1;
   if (getgid() == st_gid) 
      group = 1;   

   perm = (char *)malloc(sizeof(char) * 10);

   for (i = 0; i < 9; i++) 
      perm[i] = '-';
   perm[9] = '\0';

   if ((S_IRUSR & st_mode) == S_IRUSR) 
     {
        perm[0] = 'r';
        user_read = 1;
     }
   if ((S_IWUSR & st_mode) == S_IWUSR) 
     {
        perm[1] = 'w';
        user_write = 1;
     }
   if ((S_IXUSR & st_mode) == S_IXUSR) 
      perm[2] = 'x';

   if ((S_IRGRP & st_mode) == S_IRGRP) 
     {
	perm[3] = 'r';
        group_read = 1;
     }
   if ((S_IWGRP & st_mode) == S_IWGRP) 
     {	
        perm[4] = 'w';
        group_write = 1;
     }
   if ((S_IXGRP & st_mode) == S_IXGRP) 
      perm[5] = 'x';
   
   if ((S_IROTH & st_mode) == S_IROTH)
     {
        perm[5] = 'r';
        other_read = 1;
     }
   if ((S_IWOTH & st_mode) == S_IWOTH)
     {
        perm[6] = 'w';
        other_write = 1;
     }
   if ((S_IXOTH & st_mode) == S_IXOTH) 
      perm[8] = 'x';

   if (owner)
     {
        if ((!user_read) && (!user_write)) 
           snprintf(perms, PATH_MAX, "Protected");
        else if ((user_read) && (!user_write)) 
           snprintf(perms, PATH_MAX, "Read Only");
        else if ((user_read) && (user_write)) 
           access = 1;
     }
   else if (group)
     {
        if ((!group_read) && (!group_write)) 
           snprintf(perms, PATH_MAX, "Forbidden");
        else if ((group_read) && (!group_write)) 
           snprintf(perms, PATH_MAX, "Read Only");
        else if ((group_read) && (group_write)) 
           access = 1;
     }
   else if ((!owner) && (!group))
     {
        if ((!other_read) && (!other_write)) 
           snprintf(perms, PATH_MAX, "Forbidden");
        else if ((other_read) && (!other_write)) 
           snprintf(perms, PATH_MAX, "Read Only");
        else if ((other_read) && (other_write)) 
           access = 1;
     }
   if (!access) 
      return strdup(perms);
   else 
      return strdup("Read-Write");
}

static char * 
_e_wid_file_time_get(time_t st_modtime)
{
   float diff;
   time_t ltime;
   char modtime[PATH_MAX];
   char *motime;   

   ltime = time(NULL);
   diff = difftime(ltime, st_modtime);
   if (diff <= 60) 
      snprintf(modtime, PATH_MAX, "Under a minute ago");
   if (diff > 60) 
      snprintf(modtime, PATH_MAX, "%d minutes ago", ((int)diff/60));
   if (diff >= 3600) 
      snprintf(modtime, PATH_MAX, "%d hours ago", ((int)diff/3600));
   if (diff >= 86400) 
      snprintf(modtime, PATH_MAX, "%d days ago", ((int)diff/86400));
   if (diff >= 604800) 
      snprintf(modtime, PATH_MAX, "%d weeks ago", ((int)diff/604800));
   if (diff >= 2592000) 
      snprintf(modtime, PATH_MAX, "%d months ago", ((int)diff/2592000));
   if (diff >= 31526000) 
      snprintf(modtime, PATH_MAX, "%d years ago", ((int)diff/31526000));
 
   if (modtime) 
      motime = strdup(modtime);
   else 
      motime = strdup("unknown");
   return motime;
}

static void
_e_wid_del_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(obj);
   E_FREE(wd->entry_text);
   free(wd);
}
