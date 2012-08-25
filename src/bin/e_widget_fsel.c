#include "e.h"
#include "e_widget_filepreview.h"

typedef struct _E_Widget_Data E_Widget_Data;
struct _E_Widget_Data
{
   Evas_Object *obj;
   Evas_Object *fm_overlay_clip;
   Evas_Object *o_table;
   Evas_Object *o_table2;
   Evas_Object *o_preview_frame;
   Evas_Object *o_preview;
   Evas_Object *o_up_button;
   Evas_Object *o_favorites_frame;
   Evas_Object *o_favorites_fm;
   Evas_Object *o_favorites_add;
   Evas_Object *o_files_frame;
   Evas_Object *o_files_fm;
   Evas_Object *o_entry;
   Evas_Coord   preview_w, preview_h;
   char        *entry_text;
   const char *path;
   void         (*sel_func)(void *data, Evas_Object *obj);
   void        *sel_data;
   void         (*chg_func)(void *data, Evas_Object *obj);
   void        *chg_data;
   int          preview;
};

static void  _e_wid_del_hook(Evas_Object *obj);

/* local subsystem functions */
static void
_e_wid_fsel_button_up(void *data1, void *data2 __UNUSED__)
{
   E_Widget_Data *wd;

   wd = data1;
   if (wd->o_files_fm)
     e_fm2_parent_go(wd->o_files_fm);
   if (wd->o_files_frame)
     e_widget_scrollframe_child_pos_set(wd->o_files_frame, 0, 0);
//   e_widget_entry_text_set(wd->o_entry,
//			   e_fm2_real_path_get(wd->o_files_fm));
}

static void
_e_wid_fsel_favorites_add(void *data1, void *data2 __UNUSED__)
{
   E_Widget_Data *wd;
   const char *current_path, *fn;
   char buf[PATH_MAX], *fname;
   struct stat st;
   FILE *f;
   size_t len;

   wd = data1;
   current_path = e_fm2_real_path_get(wd->o_files_fm);
   if (!ecore_file_is_dir(current_path)) return;

   len = e_user_dir_snprintf(buf, sizeof(buf), "fileman/favorites/%s",
                             ecore_file_file_get(current_path));
   if (len >= sizeof(buf)) return;
   if (stat(buf, &st) < 0)
     symlink(current_path, buf);
   else
     {
        int i = 1, maxlen;

        buf[len] = '-';
        len++;
        if (len == sizeof(buf)) return;
        maxlen = sizeof(buf) - len;
        do
          {
             if (snprintf(buf + len, maxlen, "%d", i) >= maxlen)
               return;
             i++;
          }
        while (stat(buf, &st) == 0);
        symlink(current_path, buf);
     }
   fn = ecore_file_file_get(buf);
   len = strlen(fn) + 1;
   fname = alloca(len);
   memcpy(fname, fn, len);
   e_user_dir_concat_static(buf, "fileman/favorites/.order");
   if (ecore_file_exists(buf))
     {
        f = fopen(buf, "a");
        if (f)
          {
             fprintf(f, "%s\n", fname);
             fclose(f);
          }
     }
   e_fm2_refresh(wd->o_favorites_fm);
}

static void
_e_wid_fsel_favorites_files_changed(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   E_Widget_Data *wd;
   Eina_List *icons, *l;
   E_Fm2_Icon_Info *ici;
   const char *rp;
   char *p1, *p2;

   wd = data;
   if (!wd->o_favorites_fm) return;
   if (!wd->o_files_fm) return;
   icons = e_fm2_all_list_get(wd->o_favorites_fm);
   if (!icons) return;
   rp = e_fm2_real_path_get(wd->o_files_fm);
   p1 = ecore_file_realpath(rp);
   if (!p1) goto done;
   EINA_LIST_FOREACH(icons, l, ici)
     {
        if (ici->link)
          {
             p2 = ecore_file_realpath(ici->link);
             if (p2)
               {
                  if (!strcmp(p1, p2))
                    {
                       e_fm2_select_set(wd->o_favorites_fm, ici->file, 1);
                       E_FREE(p2);
                       goto done;
                    }
                  E_FREE(p2);
               }
          }
     }
done:
//   e_widget_entry_text_set(wd->o_entry, rp);
   E_FREE(p1);
   eina_list_free(icons);
}

static void
_e_wid_fsel_favorites_selected(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   E_Widget_Data *wd;
   Eina_List *selected;
   E_Fm2_Icon_Info *ici;

   wd = data;
   if (!wd->o_favorites_fm) return;
   if (!wd->o_files_frame) return;
   selected = e_fm2_selected_list_get(wd->o_favorites_fm);
   if (!selected) return;
   ici = eina_list_data_get(selected);
   if ((ici->link) && (ici->mount))
     e_fm2_path_set(wd->o_files_fm, ici->link, "/");
   else if (ici->real_link)
     e_fm2_path_set(wd->o_files_fm, NULL, ici->real_link);
   eina_list_free(selected);
   e_widget_scrollframe_child_pos_set(wd->o_files_frame, 0, 0);
//   e_widget_entry_text_set(wd->o_entry,
//			   e_fm2_real_path_get(wd->o_files_fm));
}

static void
_e_wid_fsel_files_changed(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   E_Widget_Data *wd;

   wd = data;
   if (!wd->o_files_fm) return;
   if (e_fm2_selected_count(wd->o_files_fm)) return;
   e_fm2_first_sel(wd->o_files_fm);
}

static void
_e_wid_fsel_files_dir_changed(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
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
//   if ((wd->path) && (stat(wd->path, &st) == 0))
//     e_widget_entry_text_set(wd->o_entry, ecore_file_file_get(wd->path));
   eina_stringshare_replace(&wd->path, NULL);
   if (wd->chg_func) wd->chg_func(wd->chg_data, wd->obj);
}

static void
_e_wid_fsel_typebuf_change(E_Widget_Data *wd, Evas_Object *obj __UNUSED__, const char *str)
{
   e_widget_entry_text_set(wd->o_entry, str);
}

static void
_e_wid_fsel_files_selection_change(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   E_Widget_Data *wd;
   Eina_List *selected;
   E_Fm2_Icon_Info *ici;
   const char *rp;
   char buf[PATH_MAX];
   struct stat st;

   wd = data;
   if (!wd->o_files_fm) return;
   selected = e_fm2_selected_list_get(wd->o_files_fm);
   if (!selected) return;
   ici = eina_list_data_get(selected);
   rp = e_fm2_real_path_get(wd->o_files_fm);
   if (!strcmp(rp, "/"))
     {
        snprintf(buf, sizeof(buf), "/%s", ici->file);
     }
   else
     {
        snprintf(buf, sizeof(buf), "%s/%s",
                 rp, ici->file);
     }
   eina_stringshare_replace(&wd->path, buf);
   if (stat(wd->path, &st) == 0)
     {
        if (wd->preview) e_widget_filepreview_path_set(wd->o_preview, wd->path, ici->mime);
        if (!S_ISDIR(st.st_mode))
          e_widget_entry_text_set(wd->o_entry, ici->file);
//	else
//	  e_widget_entry_text_set(wd->o_entry, wd->path);
     }
   eina_list_free(selected);
   if (wd->chg_func) wd->chg_func(wd->chg_data, wd->obj);
}

static void
_e_wid_fsel_preview_file_selected(void *data, Evas_Object *obj __UNUSED__, void *event_info)
{
   E_Widget_Data *wd;
   Eina_List *l;
   E_Fm2_Icon_Info *ici;
   const char *path, *dev, *newpath;

   wd = data;
   l = e_fm2_selected_list_get(event_info);
   if (!l) return;
   ici = eina_list_data_get(l);
   if (S_ISDIR(ici->statinfo.st_mode))
     {
        e_fm2_path_get(event_info, &dev, &path);
        newpath = eina_stringshare_printf("%s/%s", path, ici->file);
        e_fm2_path_set(wd->o_files_fm, dev, newpath);
        eina_stringshare_del(newpath);
        return;
     }
   if (wd->sel_func) wd->sel_func(wd->sel_data, wd->obj);
}

static void
_e_wid_fsel_files_selected(void *data, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   E_Widget_Data *wd;

   wd = data;
   if (wd->path)
     {
        if (wd->sel_func) wd->sel_func(wd->sel_data, wd->obj);
     }
}

static void
_e_wid_fsel_moveresize(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   E_Widget_Data *wd = data;
   int x, y, w, h;
   evas_object_geometry_get(wd->o_files_frame, &x, &y, &w, &h);
   evas_object_resize(wd->fm_overlay_clip, x + w, y + h);
   evas_object_geometry_get(wd->o_favorites_frame, &x, &y, NULL, NULL);
   evas_object_move(wd->fm_overlay_clip, x, y);
}

/* externally accessible functions */
EAPI Evas_Object *
e_widget_fsel_add(Evas *evas, const char *dev, const char *path, char *selected, char *filter __UNUSED__,
                  void (*sel_func)(void *data, Evas_Object *obj), void *sel_data,
                  void (*chg_func)(void *data, Evas_Object *obj), void *chg_data,
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

   o = e_widget_button_add(evas, _("Add to Favorites"), "bookmark-new",
                           _e_wid_fsel_favorites_add, wd, NULL);
   wd->o_favorites_add = o;
   e_widget_table_object_append(wd->o_table2, o, 0, 0, 1, 1, 0, 0, 0, 0);

   o = e_widget_button_add(evas, _("Go up a Directory"), "go-up",
                           _e_wid_fsel_button_up, wd, NULL);
   wd->o_up_button = o;
   e_widget_table_object_append(wd->o_table2, o, 1, 0, 1, 1, 0, 0, 1, 0);

   if (preview)
     {
        Evas_Coord mw2, mh2;
        
        wd->o_preview_frame = e_widget_framelist_add(evas, _("Preview"), 0);
        wd->o_preview = e_widget_filepreview_add(evas, 128, 128, 0);
        e_widget_filepreview_filemode_force(wd->o_preview);
        e_widget_framelist_object_append(wd->o_preview_frame, wd->o_preview);
        evas_object_smart_callback_add(wd->o_preview, "selected",
                                       _e_wid_fsel_preview_file_selected, wd);
        
        e_widget_size_min_get(wd->o_preview, &mw, &mh);
        e_widget_size_min_get(wd->o_preview_frame, &mw2, &mh2);
        /* need size of preview here or min size will be off */
        e_widget_size_min_set(wd->o_preview_frame, mw2, mh + 128);
     }

   o = e_fm2_add(evas);
   wd->o_favorites_fm = o;
   memset(&fmc, 0, sizeof(E_Fm2_Config));
   fmc.view.mode = E_FM2_VIEW_MODE_LIST;
   fmc.view.open_dirs_in_place = 1;
   fmc.view.selector = 1;
   fmc.view.single_click = 1;
   fmc.view.no_subdir_jump = 1;
   fmc.view.no_subdir_drop = 1;
   fmc.view.link_drop = 1;
   fmc.icon.list.w = 24;
   fmc.icon.list.h = 24;
   fmc.icon.fixed.w = 1;
   fmc.icon.fixed.h = 1;
   fmc.icon.extension.show = 0;
   fmc.icon.key_hint = NULL;
   fmc.list.sort.no_case = 1;
   fmc.list.sort.dirs.first = 0;
   fmc.list.sort.dirs.last = 0;
   fmc.selection.single = 1;
   fmc.selection.windows_modifiers = 0;
   fmc.view.no_click_rename = 1;
   e_fm2_config_set(o, &fmc);
   e_fm2_icon_menu_flags_set(o, E_FM2_MENU_NO_VIEW_MENU | E_FM2_MENU_NO_ACTIVATE_CHANGE);
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
   evas_object_propagate_events_set(wd->o_favorites_fm, 0);
   e_widget_scrollframe_focus_object_set(o, wd->o_favorites_fm);

   wd->o_favorites_frame = o;
   e_widget_size_min_set(o, 128, 128);
   e_widget_table_object_append(wd->o_table2, o, 0, 1, 1, 1, 1, 1, 0, 1);

   o = e_fm2_add(evas);
   wd->o_files_fm = o;
   memset(&fmc, 0, sizeof(E_Fm2_Config));
   fmc.view.mode = E_FM2_VIEW_MODE_LIST;
   fmc.view.open_dirs_in_place = 1;
   fmc.view.selector = 1;
   fmc.view.single_click = e_config->filemanager_single_click;
   fmc.view.no_subdir_jump = 0;
   fmc.icon.list.w = 24;
   fmc.icon.list.h = 24;
   fmc.icon.fixed.w = 1;
   fmc.icon.fixed.h = 1;
   fmc.icon.extension.show = 1;
   fmc.icon.key_hint = NULL;
   fmc.list.sort.no_case = 1;
   fmc.list.sort.dirs.first = 1;
   fmc.list.sort.dirs.last = 0;
   fmc.selection.single = 1;
   fmc.selection.windows_modifiers = 0;
   fmc.view.no_click_rename = 1;
   e_fm2_config_set(o, &fmc);
   e_fm2_icon_menu_flags_set(o, E_FM2_MENU_NO_VIEW_MENU);
   evas_object_smart_callback_add(o, "changed",
                                  _e_wid_fsel_files_changed, wd);
   evas_object_smart_callback_add(o, "dir_changed",
                                  _e_wid_fsel_files_dir_changed, wd);
   evas_object_smart_callback_add(o, "selection_change",
                                  _e_wid_fsel_files_selection_change, wd);
   evas_object_smart_callback_add(o, "selected",
                                  _e_wid_fsel_files_selected, wd);
   evas_object_smart_callback_add(o, "typebuf_changed",
                                  (Evas_Smart_Cb)_e_wid_fsel_typebuf_change, wd);
   e_fm2_path_set(o, dev, path);

   o = e_widget_scrollframe_pan_add(evas, wd->o_files_fm,
                                    e_fm2_pan_set,
                                    e_fm2_pan_get,
                                    e_fm2_pan_max_get,
                                    e_fm2_pan_child_size_get);
   evas_object_propagate_events_set(wd->o_files_fm, 0);
   e_widget_scrollframe_focus_object_set(o, wd->o_files_fm);

   wd->o_files_frame = o;
   e_widget_size_min_set(o, 128, 128);
   e_widget_table_object_append(wd->o_table2, o, 1, 1, 1, 1, 1, 1, 0, 1);

   o = e_widget_entry_add(evas, &(wd->entry_text), NULL, NULL, NULL);
   wd->o_entry = o;
   if (selected) e_widget_entry_text_set(o, selected);

   wd->fm_overlay_clip = evas_object_rectangle_add(evas);
   evas_object_static_clip_set(wd->fm_overlay_clip, EINA_TRUE);
   e_widget_sub_object_add(obj, wd->fm_overlay_clip);
   e_fm2_overlay_clip_to(wd->o_favorites_fm, wd->fm_overlay_clip);
   e_fm2_overlay_clip_to(wd->o_files_fm, wd->fm_overlay_clip);
   evas_object_event_callback_add(wd->o_favorites_frame, EVAS_CALLBACK_RESIZE, _e_wid_fsel_moveresize, wd);
   evas_object_event_callback_add(wd->o_favorites_frame, EVAS_CALLBACK_MOVE, _e_wid_fsel_moveresize, wd);
   evas_object_event_callback_add(wd->o_files_frame, EVAS_CALLBACK_RESIZE, _e_wid_fsel_moveresize, wd);
   evas_object_event_callback_add(wd->o_files_frame, EVAS_CALLBACK_MOVE, _e_wid_fsel_moveresize, wd);
   evas_object_show(wd->fm_overlay_clip);

   if (preview)
     {
        e_widget_table_object_append(wd->o_table2,
                                     wd->o_preview_frame,
                                     2, 1, 1, 1, 1, 1, 1, 1);
     }

   e_widget_table_object_append(wd->o_table, wd->o_table2,
                                0, 0, 1, 1, 1, 1, 1, 1);
   e_widget_table_object_append(wd->o_table, wd->o_entry,
                                0, 1, 1, 1, 1, 0, 1, 0);

   e_widget_size_min_get(wd->o_table, &mw, &mh);
   e_widget_size_min_set(obj, mw, mh);

   evas_object_show(wd->o_favorites_add);
   evas_object_show(wd->o_up_button);
   evas_object_show(wd->o_favorites_frame);
   evas_object_show(wd->o_favorites_fm);
   evas_object_show(wd->o_files_frame);
   evas_object_show(wd->o_files_fm);
   evas_object_show(wd->o_entry);
   evas_object_show(wd->o_table2);
   evas_object_show(wd->o_table);
   e_fm2_first_sel(wd->o_files_fm);
   evas_object_focus_set(wd->o_files_fm, EINA_TRUE);
   return obj;
}

EAPI void
e_widget_fsel_path_get(Evas_Object *obj, const char **dev, const char **path)
{
   E_Widget_Data *wd;

   if (!obj) return;
   wd = e_widget_data_get(obj);
   e_fm2_path_get(wd->o_files_fm, dev, path);
}

EAPI const char *
e_widget_fsel_selection_path_get(Evas_Object *obj)
{
   E_Widget_Data *wd;
   const char *s, *dir;
   char buf[PATH_MAX];

   if (!obj) return NULL;
   wd = e_widget_data_get(obj);
   s = e_widget_entry_text_get(wd->o_entry);
   dir = e_fm2_real_path_get(wd->o_files_fm);
   if (s)
     {
        snprintf(buf, sizeof(buf), "%s/%s", dir, s);
        eina_stringshare_replace(&wd->path, buf);
     }
   else
     {
        eina_stringshare_replace(&wd->path, dir);
     }
   return wd->path;
}

EAPI void
e_widget_fsel_window_object_set(Evas_Object *obj, E_Object *eobj)
{
   E_Widget_Data *wd;

   if (!obj) return;
   wd = e_widget_data_get(obj);
   e_fm2_window_object_set(wd->o_favorites_fm, eobj);
   e_fm2_window_object_set(wd->o_files_fm, eobj);
}

EAPI Eina_Bool
e_widget_fsel_typebuf_visible_get(Evas_Object *obj)
{
   E_Widget_Data *wd;

   if (!obj) return EINA_FALSE;
   wd = e_widget_data_get(obj);
   if (!wd) return EINA_FALSE;
   return e_fm2_typebuf_visible_get(wd->o_files_fm) || e_fm2_typebuf_visible_get(wd->o_favorites_fm);
}

static void
_e_wid_del_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   E_FREE(wd->entry_text);
   eina_stringshare_del(wd->path);

   free(wd);
}
