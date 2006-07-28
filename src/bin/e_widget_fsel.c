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
   Evas_Object *o_up_button;
   Evas_Object *o_favorites_frame;
   Evas_Object *o_favorites_fm;
   Evas_Object *o_files_frame;
   Evas_Object *o_files_fm;
   Evas_Object *o_entry;
   char *entry_text;
   char *path;
   void (*sel_func) (void *data, Evas_Object *obj);
   void *sel_data;
   void (*chg_func) (void *data, Evas_Object *obj);
   void *chg_data;
};

static void _e_wid_del_hook(Evas_Object *obj);

/* local subsystem functions */
static void
_e_wid_fsel_button_up(void *data1, void *data2)
{
   E_Widget_Data *wd;
   
   wd = data1;
   e_fm2_parent_go(wd->o_files_fm);
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
   if (!e_fm2_has_parent_get(wd->o_files_fm))
     e_widget_disabled_set(wd->o_up_button, 1);
   else
     e_widget_disabled_set(wd->o_up_button, 0);
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
/* FIXME: callback only exists for double-clicking and selecting. need to add
 * one for when the selection changes as well
 */
EAPI Evas_Object *
e_widget_fsel_add(Evas *evas, char *dev, char *path, char *selected, char *filter, 
		  void (*sel_func) (void *data, Evas_Object *obj), void *sel_data,
		  void (*chg_func) (void *data, Evas_Object *obj), void *chg_data)
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
   
   o = e_widget_table_add(evas, 0);
   wd->o_table = o;
   e_widget_sub_object_add(obj, o);
   e_widget_resize_object_set(obj, o);
   
   o = e_widget_table_add(evas, 0);
   wd->o_table2 = o;
   e_widget_sub_object_add(obj, o);
   
   o = e_widget_button_add(evas, _("Go up a Directory"), NULL,
			   _e_wid_fsel_button_up, wd, NULL);
   wd->o_up_button = o;
   e_widget_sub_object_add(obj, o);
   e_widget_table_object_append(wd->o_table2, o, 1, 0, 1, 1, 0, 0, 1, 0);

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
   e_fm2_path_set(o, "favorites", "/");
   evas_object_smart_callback_add(o, "files_changed", 
				  _e_wid_fsel_favorites_files_changed, wd);
   evas_object_smart_callback_add(o, "selected", 
				  _e_wid_fsel_favorites_selected, wd);
   
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
   fmc.icon.extension.show = 0;
   fmc.list.sort.no_case = 1;
   fmc.list.sort.dirs.first = 1;
   fmc.list.sort.dirs.last = 0;
   fmc.selection.single = 1;
   fmc.selection.windows_modifiers = 0;
   e_fm2_config_set(o, &fmc);
   e_fm2_path_set(o, dev, path);
   evas_object_smart_callback_add(o, "changed", 
				  _e_wid_fsel_files_changed, wd);
   evas_object_smart_callback_add(o, "selection_change", 
				  _e_wid_fsel_files_selection_change, wd);
   evas_object_smart_callback_add(o, "selected", 
				  _e_wid_fsel_files_selected, wd);
   
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
   evas_object_show(wd->o_table2);
   evas_object_show(wd->o_table);
   return obj;
}

EAPI const char *
e_widget_fsel_selection_path_get(Evas_Object *obj)
{
   E_Widget_Data *wd;

   wd = e_widget_data_get(obj);
   return wd->path;
}

static void
_e_wid_del_hook(Evas_Object *obj)
{
   E_Widget_Data *wd;
   
   wd = e_widget_data_get(obj);
   E_FREE(wd->entry_text);
   free(wd);
}
