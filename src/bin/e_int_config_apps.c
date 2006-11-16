/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

/* TODO:
 * More filtering/sorting of left side to make managing thousands of apps easy.
 *
 * These things require support from e_fm -
 * Stop user from deleting standard directories on right side.
 * Stop user from creating new directories on left side.
 */


static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static void _move_file_up_in_order(const char *order, const char *file);
static void _move_file_down_in_order(const char *order, const char *file);
static void _append_to_order(const char *order, const char *file);

struct _E_Config_Dialog_Data
{
   E_Config_Dialog *cfd;
   E_Fm2_Icon_Info *info;
   char *selected;
   char path_all[4096], path_everything[4096], path[4096], *homedir;
   int sorted;
   struct {
      Evas_Object *o_fm_all;
      Evas_Object *o_fm;
      Evas_Object *o_frame;
      Evas_Object *o_up_button;
      Evas_Object *o_up_all_button;
      Evas_Object *o_add_button;
      Evas_Object *o_create_button;
      Evas_Object *o_delete_left_button;
      Evas_Object *o_delete_right_button;
      Evas_Object *o_regen_button;
      Evas_Object *o_move_up_button;
      Evas_Object *o_move_down_button;
   } gui;
   E_App_Edit *editor;
};

struct _E_Config_Once
{
   const char *title;
   const char *label;
   const char *path;
   int (*func) (void *data, const char *path);
   void *data;
};


EAPI E_Config_Dialog *
e_int_config_apps_once(E_Container *con, const char *title, const char *label, const char *path, int (*func) (void *data, const char *path), void *data)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   struct _E_Config_Once *once = NULL;

   v = E_NEW(E_Config_Dialog_View, 1);

   v->create_cfdata           = _create_data;
   v->free_cfdata             = _free_data;
   v->basic.create_widgets    = _basic_create_widgets;

   if ((path) || (func))
      {
         once = E_NEW(struct _E_Config_Once, 1);
	 if (once)
	    {
	       once->title = title;
	       once->label = label;
	       once->path = path;
	       once->func = func;
	       once->data = data;
	    }
      }

   cfd = e_config_dialog_new(con,
			     _("Applications"),
			     "E", "_config_applications_dialog",
			     "enlightenment/applications", 0, v, once);
   return cfd;
}

EAPI E_Config_Dialog *
e_int_config_apps(E_Container *con)
{
   if (e_config_dialog_find("E", "_config_applications_dialog")) return NULL;
   return e_int_config_apps_once(con, NULL, NULL, NULL, NULL, NULL);
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   cfdata->homedir = strdup(e_user_homedir_get());
   if (!cfdata->homedir) return;

   snprintf(cfdata->path_everything, sizeof(cfdata->path_everything), "%s/.e/e/applications/all", cfdata->homedir);
   snprintf(cfdata->path, sizeof(cfdata->path), "%s/.e/e/applications", cfdata->homedir);
   snprintf(cfdata->path_all, sizeof(cfdata->path_all), "%s/.e/e/applications/menu/all", cfdata->homedir);
   return;
}

static void *
_create_data(E_Config_Dialog *cfd)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfdata->cfd = cfd;
   _fill_data(cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   if (cfdata->selected)   free(cfdata->selected);
   if (cfdata->cfd->data)  free(cfdata->cfd->data);
   free(cfdata);
}

static void
_cb_button_up(void *data1, void *data2)
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = data1;
   if (cfdata->gui.o_fm)
     e_fm2_parent_go(cfdata->gui.o_fm);
   if (cfdata->gui.o_frame)
     e_widget_scrollframe_child_pos_set(cfdata->gui.o_frame, 0, 0);
}

static void
_cb_button_up_all(void *data1, void *data2)
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = data1;
   if (cfdata->gui.o_fm_all)
     e_fm2_parent_go(cfdata->gui.o_fm_all);
   if (cfdata->gui.o_frame)
     e_widget_scrollframe_child_pos_set(cfdata->gui.o_frame, 0, 0);
}

static void
_cb_files_dir_changed(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = data;
   if (!cfdata->gui.o_fm) return;
   if (!e_fm2_has_parent_get(cfdata->gui.o_fm))
     {
	if (cfdata->gui.o_up_button)
	  e_widget_disabled_set(cfdata->gui.o_up_button, 1);
     }
   else
     {
	if (cfdata->gui.o_up_button)
	  e_widget_disabled_set(cfdata->gui.o_up_button, 0);
     }
   if (cfdata->gui.o_move_up_button)
     e_widget_disabled_set(cfdata->gui.o_move_up_button, 1);
   if (cfdata->gui.o_move_down_button)
     e_widget_disabled_set(cfdata->gui.o_move_down_button, 1);
   if (cfdata->gui.o_frame)
     e_widget_scrollframe_child_pos_set(cfdata->gui.o_frame, 0, 0);
   if (cfdata->selected)   free(cfdata->selected);
   cfdata->selected = NULL;
}

static void
_cb_files_dir_changed_all(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = data;
   if (!cfdata->gui.o_fm_all) return;
   if (!e_fm2_has_parent_get(cfdata->gui.o_fm_all))
     {
	if (cfdata->gui.o_up_all_button)
	  e_widget_disabled_set(cfdata->gui.o_up_all_button, 1);
     }
   else
     {
	if (cfdata->gui.o_up_all_button)
	  e_widget_disabled_set(cfdata->gui.o_up_all_button, 0);
     }
   if (cfdata->gui.o_frame)
     e_widget_scrollframe_child_pos_set(cfdata->gui.o_frame, 0, 0);
}

static void
_cb_files_selection_change(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;
   Evas_List *selected;
   E_Fm2_Icon_Info *ici = NULL;
   const char *realpath;
   char buf[4096];
   
   cfdata = data;
   if (!cfdata->gui.o_fm) return;
   selected = e_fm2_selected_list_get(cfdata->gui.o_fm);
   if (selected)   ici = selected->data;
   realpath = e_fm2_real_path_get(cfdata->gui.o_fm);
   snprintf(buf, sizeof(buf), "%s/.order", realpath);
   if (cfdata->selected)   free(cfdata->selected);
   cfdata->selected = NULL;
   if (ecore_file_exists(buf))
     {
	if (cfdata->gui.o_move_up_button)
	   e_widget_disabled_set(cfdata->gui.o_move_up_button, 0);
	if (cfdata->gui.o_move_down_button)
	   e_widget_disabled_set(cfdata->gui.o_move_down_button, 0);
	if ((ici) && (ici->file))
           cfdata->selected = strdup(ici->file);
     }
   else
     {
	if (cfdata->gui.o_move_up_button)
	  e_widget_disabled_set(cfdata->gui.o_move_up_button, 1);
	if (cfdata->gui.o_move_down_button)
	  e_widget_disabled_set(cfdata->gui.o_move_down_button, 1);
     }
}

static void
_cb_files_changed(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = data;
   if (!cfdata->gui.o_fm) return;
   if (cfdata->selected)
      {
         if (cfdata->gui.o_move_up_button)
            e_widget_disabled_set(cfdata->gui.o_move_up_button, 0);
         if (cfdata->gui.o_move_down_button)
            e_widget_disabled_set(cfdata->gui.o_move_down_button, 0);
         e_fm2_select_set(cfdata->gui.o_fm, cfdata->selected, 1);
      }
   else
      {
         if (cfdata->gui.o_move_up_button)
            e_widget_disabled_set(cfdata->gui.o_move_up_button, 1);
         if (cfdata->gui.o_move_down_button)
            e_widget_disabled_set(cfdata->gui.o_move_down_button, 1);
      }
}

static void
_cb_editor_del(void *obj)
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = e_object_data_get(obj);
   cfdata->editor = NULL;
   e_object_del_attach_func_set(obj, NULL);
   e_object_data_set(obj, NULL);
//   e_fm2_refresh(cfdata->gui.o_fm);
//   e_fm2_refresh(cfdata->gui.o_fm_all);
}

static void
_cb_button_create(void *data1, void *data2)
{
   E_App *a;
   E_Config_Dialog_Data *cfdata;
   
   cfdata = data1;
   a = e_app_empty_new(NULL);
   if (cfdata->editor) e_object_del(E_OBJECT(cfdata->editor));
   cfdata->editor = e_eap_edit_show(cfdata->cfd->con, a);
   e_object_data_set(E_OBJECT(cfdata->editor), cfdata);
   e_object_del_attach_func_set(E_OBJECT(cfdata->editor), _cb_editor_del);
}

static void
_cb_files_selected_all(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;
   Evas_List *selected;
   E_Fm2_Icon_Info *ici;
   const char *realpath;
   char buf[4096];
   E_App *a;
   
   cfdata = data;
   if (!cfdata->gui.o_fm_all) return;
   selected = e_fm2_selected_list_get(cfdata->gui.o_fm_all);
   if (!selected) return;
   ici = selected->data;
   realpath = e_fm2_real_path_get(cfdata->gui.o_fm_all);
   if (!strcmp(realpath, "/"))
     snprintf(buf, sizeof(buf), "/%s", ici->file);
   else
     snprintf(buf, sizeof(buf), "%s/%s", realpath, ici->file);
   evas_list_free(selected);
   if (ecore_file_is_dir(buf)) return;
   a = e_app_new(buf, 0);
   if (a)
     {
	if (cfdata->editor) e_object_del(E_OBJECT(cfdata->editor));
	cfdata->editor = e_eap_edit_show(cfdata->cfd->con, a);
	e_object_data_set(E_OBJECT(cfdata->editor), cfdata);
	e_object_del_attach_func_set(E_OBJECT(cfdata->editor), _cb_editor_del);
     }
}

static void
_cb_files_selected_all2(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;
   Evas_List *selected;
   E_Fm2_Icon_Info *ici;
   const char *realpath;
   char buf[4096];
   E_App *a;
   
   cfdata = data;
   if (!cfdata->gui.o_fm) return;
   selected = e_fm2_selected_list_get(cfdata->gui.o_fm);
   if (!selected) return;
   ici = selected->data;
   realpath = e_fm2_real_path_get(cfdata->gui.o_fm);
   if (!strcmp(realpath, "/"))
     snprintf(buf, sizeof(buf), "/%s", ici->file);
   else
     snprintf(buf, sizeof(buf), "%s/%s", realpath, ici->file);
   evas_list_free(selected);
   if (ecore_file_is_dir(buf)) return;
   a = e_app_new(buf, 0);
   if (a)
     {
	if (cfdata->editor) e_object_del(E_OBJECT(cfdata->editor));
	cfdata->editor = e_eap_edit_show(cfdata->cfd->con, a);
	e_object_data_set(E_OBJECT(cfdata->editor), cfdata);
	e_object_del_attach_func_set(E_OBJECT(cfdata->editor), _cb_editor_del);
     }
}

static void
_cb_files_selection_change_all(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = data;
   e_widget_disabled_set(cfdata->gui.o_add_button, 0);
}

static void
_cb_files_changed_all(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = data;
   e_widget_disabled_set(cfdata->gui.o_add_button, 1);
}

static void
_cb_files_edited(void *data, E_Menu *m, E_Menu_Item *mi)
{
   E_Config_Dialog_Data *cfdata;
   E_Fm2_Icon_Info *info;
   const char *realpath;
   char buf[4096];
   E_App *a;
   
   cfdata = data;
   if (!cfdata->gui.o_fm_all) return;
   if (!cfdata->info) return;
   info = cfdata->info;

   realpath = e_fm2_real_path_get(cfdata->gui.o_fm_all);

   if (info->pseudo_link)
     snprintf(buf, sizeof(buf), "%s/%s", info->link, info->file);
   else
     snprintf(buf, sizeof(buf), "%s/%s", realpath, info->file);

   if (ecore_file_is_dir(buf)) return;
   a = e_app_new(buf, 0);
   if (a)
     {
	if (cfdata->editor) e_object_del(E_OBJECT(cfdata->editor));
	cfdata->editor = e_eap_edit_show(cfdata->cfd->con, a);
	e_object_data_set(E_OBJECT(cfdata->editor), cfdata);
	e_object_del_attach_func_set(E_OBJECT(cfdata->editor), _cb_editor_del);
     }
}

static void
_cb_files_add_edited(void *data, Evas_Object *obj, E_Menu *m, E_Fm2_Icon_Info *info)
{
   E_Config_Dialog_Data *cfdata;
   E_Menu_Item *mi;

   cfdata = data;
   if (!info) return;
   /* We need to get this info data to the menu callback, all this is created on the fly when user right clicks. */
   cfdata->info = info;
   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Edit Application"));
   e_menu_item_callback_set(mi, _cb_files_edited, cfdata);
}

static void
_cb_files_sorted_changed(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = data;
   if (!cfdata->gui.o_fm_all) return;
   if (cfdata->sorted)
      e_fm2_path_set(cfdata->gui.o_fm_all, cfdata->path_everything, "/");
   else
      e_fm2_path_set(cfdata->gui.o_fm_all, cfdata->path_all, "/");
}

static void
_cb_button_add(void *data1, void *data2)
{
   E_Config_Dialog_Data *cfdata;
   Evas_List *selected;
   E_Fm2_Icon_Info *ici;
   const char *realpath;
   char buf[4096];
   int do_it = 1;

   cfdata = data1;
   if (!cfdata->gui.o_fm_all) return;

   selected = e_fm2_selected_list_get(cfdata->gui.o_fm_all);
   if (!selected) return;
   ici = selected->data;
   realpath = e_fm2_real_path_get(cfdata->gui.o_fm_all);
   if (cfdata->sorted)
      {
         if (!strcmp(realpath, "/"))
            snprintf(buf, sizeof(buf), "/%s", ici->file);
         else
            snprintf(buf, sizeof(buf), "%s/%s", realpath, ici->file);
      }
   else
      snprintf(buf, sizeof(buf), "%s/%s", cfdata->path_everything, ici->file);
   evas_list_free(selected);
   if (ecore_file_is_dir(buf)) return;

   if (cfdata->cfd->data)
      {
         struct _E_Config_Once *once = NULL;

         once = cfdata->cfd->data;
	 if (once->func)
	   {
	      once->func(once->data, buf);
	      do_it = 0;
	   }
      }
   if (do_it)
      {
         if (!cfdata->gui.o_fm) return;

//         a = e_app_new(buf, 0);
         realpath = e_fm2_real_path_get(cfdata->gui.o_fm);
//         parent = e_app_new(realpath, 0);
//         if (parent) e_app_subdir_scan(parent, 0);
//         e_app_append(a, parent);
         _append_to_order(realpath, ecore_file_get_file(buf));
      }

// FIXME: When fm is fixed to create the .order files for us, then we can remove this.
   e_fm2_refresh(cfdata->gui.o_fm);
}

static void
_cb_button_move_up(void *data1, void *data2)
{
   E_Config_Dialog_Data *cfdata;
   Evas_List *selected;
   E_Fm2_Icon_Info *ici;
   const char *realpath;

   cfdata = data1;
   if (!cfdata->gui.o_fm) return;

   selected = e_fm2_selected_list_get(cfdata->gui.o_fm);
   if (!selected) return;
   ici = selected->data;
   realpath = e_fm2_real_path_get(cfdata->gui.o_fm);
   evas_list_free(selected);
   _move_file_up_in_order(realpath, ici->file);
   e_fm2_refresh(cfdata->gui.o_fm);
}

static void
_cb_button_move_down(void *data1, void *data2)
{
   E_Config_Dialog_Data *cfdata;
   Evas_List *selected;
   E_Fm2_Icon_Info *ici;
   const char *realpath;

   cfdata = data1;
   if (!cfdata->gui.o_fm) return;

   selected = e_fm2_selected_list_get(cfdata->gui.o_fm);
   if (!selected) return;
   ici = selected->data;
   realpath = e_fm2_real_path_get(cfdata->gui.o_fm);
   evas_list_free(selected);
   _move_file_down_in_order(realpath, ici->file);
   e_fm2_refresh(cfdata->gui.o_fm);
}

static E_Dialog *_e_int_config_apps_regen_dialog = NULL;

static void
_e_int_config_apps_cb_regen_dialog_regen(void *data, E_Dialog *dia)
{
   e_object_del(E_OBJECT(_e_int_config_apps_regen_dialog));
   _e_int_config_apps_regen_dialog = NULL;
   e_fdo_menu_to_order(1);
//   if (cfdata->gui.o_fm_all)   e_fm2_refresh(cfdata->gui.o_fm_all);
//   if (cfdata->gui.o_fm)       e_fm2_refresh(cfdata->gui.o_fm);
}

static void
_e_int_config_apps_cb_regen_dialog_update(void *data, E_Dialog *dia)
{
   e_object_del(E_OBJECT(_e_int_config_apps_regen_dialog));
   _e_int_config_apps_regen_dialog = NULL;
   e_fdo_menu_to_order(0);
}

static void
_e_int_config_apps_cb_regen_dialog_delete(E_Win *win)
{
   E_Dialog *dia;

   dia = win->data;
   e_object_del(E_OBJECT(_e_int_config_apps_regen_dialog));
   _e_int_config_apps_regen_dialog = NULL;
}

static void
_cb_button_regen(void *data1, void *data2)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data1;
   if (_e_int_config_apps_regen_dialog) e_object_del(E_OBJECT(_e_int_config_apps_regen_dialog));
   _e_int_config_apps_regen_dialog = e_dialog_new(e_container_current_get(e_manager_current_get()), "E", "_menu_regen_dialog");
   if (!_e_int_config_apps_regen_dialog) return;
   e_win_delete_callback_set(_e_int_config_apps_regen_dialog->win, _e_int_config_apps_cb_regen_dialog_delete);
   e_dialog_title_set(_e_int_config_apps_regen_dialog, _("Regenerate or update your Applications menu?"));
   e_dialog_text_set(_e_int_config_apps_regen_dialog,
		     _("You can regenerate your Applications menu.<br>"
		       "This will delete any customizations you have made.<br>"
		       "This will replace the Applications menu with the system menu.<br>"
		       "<br>"
		       "Or you could just update your Applications menu.<br>"
		       "This will add any new Applications, and remove any old ones from<br>"
		       "your Applications menu.  Customizations you have made will remain."
		       ));
   e_dialog_icon_set(_e_int_config_apps_regen_dialog, "enlightenment/regenerate_menus", 64);
   e_dialog_button_add(_e_int_config_apps_regen_dialog, _("Regenerate"), NULL,
		       _e_int_config_apps_cb_regen_dialog_regen, NULL);
   e_dialog_button_add(_e_int_config_apps_regen_dialog, _("Update"), NULL,
		       _e_int_config_apps_cb_regen_dialog_update, NULL);
   e_dialog_button_focus_num(_e_int_config_apps_regen_dialog, 1);
   e_win_centered_set(_e_int_config_apps_regen_dialog->win, 1);
   e_dialog_show(_e_int_config_apps_regen_dialog);
   e_dialog_border_icon_set(_e_int_config_apps_regen_dialog,"enlightenment/regenerate_menus");
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   struct _E_Config_Once *once = NULL;
   Evas_Object *o, *of, *ob, *ot, *mt;
   E_Fm2_Config fmc_all, fmc;

   if (cfdata->cfd->data)
      once = cfdata->cfd->data;

   o = e_widget_list_add(evas, 1, 0);
   ot = e_widget_table_add(evas, 1);

   of = e_widget_framelist_add(evas, _("Available Applications"), 0);

   mt = e_widget_button_add(evas, _("Go up a Directory"), "widget/up_dir",
		           _cb_button_up_all, cfdata, NULL);
   cfdata->gui.o_up_all_button = mt;
   e_widget_framelist_object_append(of, mt);

   mt = e_fm2_add(evas);
   cfdata->gui.o_fm_all = mt;
   memset(&fmc_all, 0, sizeof(E_Fm2_Config));
   fmc_all.view.mode = E_FM2_VIEW_MODE_LIST;
   fmc_all.view.open_dirs_in_place = 1;
   fmc_all.view.selector = 1;
   fmc_all.view.single_click = 0;
   fmc_all.view.no_subdir_jump = 0;
   fmc_all.view.extra_file_source = cfdata->path_everything;
   fmc_all.view.always_order = 1;
   fmc_all.icon.list.w = 24;
   fmc_all.icon.list.h = 24;
   fmc_all.icon.fixed.w = 1;
   fmc_all.icon.fixed.h = 1;
   fmc_all.icon.extension.show = 1;
   fmc_all.icon.key_hint = NULL;
   fmc_all.list.sort.no_case = 1;
   fmc_all.list.sort.dirs.first = 1;
   fmc_all.list.sort.dirs.last = 0;
   fmc_all.selection.single = 1;
   fmc_all.selection.windows_modifiers = 0;
   e_fm2_config_set(mt, &fmc_all);
   e_fm2_icon_menu_flags_set(mt, E_FM2_MENU_NO_SHOW_HIDDEN);
   evas_object_smart_callback_add(mt, "dir_changed",
				  _cb_files_dir_changed_all, cfdata);
   evas_object_smart_callback_add(mt, "selected",
				  _cb_files_selected_all, cfdata);
   evas_object_smart_callback_add(mt, "selection_change",
				  _cb_files_selection_change_all, cfdata);
   evas_object_smart_callback_add(mt, "changed",
				  _cb_files_changed_all, cfdata);
   e_fm2_icon_menu_start_extend_callback_set(mt, _cb_files_add_edited, cfdata);
   e_fm2_path_set(cfdata->gui.o_fm_all, cfdata->path_all, "/");
   e_fm2_window_object_set(mt, E_OBJECT(cfd->dia->win));
   ob = e_widget_scrollframe_pan_add(evas, mt,
				     e_fm2_pan_set,
				     e_fm2_pan_get,
				     e_fm2_pan_max_get,
				     e_fm2_pan_child_size_get);
   cfdata->gui.o_frame = ob;
   e_widget_min_size_set(ob, 150, 220);
   e_widget_framelist_object_append(of, ob);
   e_box_pack_options_set(ob,
			  1, 1, /* fill */
			  1, 1, /* expand */
			  0.5, 0.5, /* align */
			  150, 220, /* min */
			  99999, 99999 /* max */
			  );
   mt = e_widget_check_add(evas, _("Sort applications"), &(cfdata->sorted));
   evas_object_smart_callback_add(mt, "changed",
				  _cb_files_sorted_changed, cfdata);
   e_widget_framelist_object_append(of, mt);

   mt = NULL;
   if ((once) && (once->label))
      mt = e_widget_button_add(evas, _(once->label), "enlightenment/e",
			   _cb_button_add, cfdata, NULL);
/*
   else
      mt = e_widget_button_add(evas, _("Add application ->"), "enlightenment/e",
			   _cb_button_add, cfdata, NULL);
*/
   if (mt)
     {
        cfdata->gui.o_add_button = mt;

        e_widget_framelist_object_append(of, mt);
        e_widget_disabled_set(mt, 1);
     }

   mt = e_widget_button_add(evas, _("Create a new application"), "enlightenment/e",
			   _cb_button_create, cfdata, NULL);
   cfdata->gui.o_create_button = mt;
   e_widget_framelist_object_append(of, mt);

   e_widget_table_object_append(ot, of, 0, 0, 2, 4, 1, 1, 1, 1);

   if ((!once) || (once->path))
      {
	 if ((once) && (once->title))
            of = e_widget_framelist_add(evas, once->title, 0);
	 else
            of = e_widget_framelist_add(evas, _("Bars, Menus, etc."), 0);

         mt = e_widget_button_add(evas, _("Go up a Directory"), "widget/up_dir",
			   _cb_button_up, cfdata, NULL);
         cfdata->gui.o_up_button = mt;
         e_widget_framelist_object_append(of, mt);

         mt = e_fm2_add(evas);
         cfdata->gui.o_fm = mt;
         memset(&fmc, 0, sizeof(E_Fm2_Config));
         fmc.view.mode = E_FM2_VIEW_MODE_LIST;
         fmc.view.open_dirs_in_place = 1;
         fmc.view.selector = 1;
         fmc.view.single_click = 0;
         fmc.view.no_subdir_jump = 0;
	 fmc.view.extra_file_source = cfdata->path_everything;
	 fmc.view.always_order = 1;
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
         e_fm2_config_set(mt, &fmc);
	 e_fm2_icon_menu_flags_set(mt, E_FM2_MENU_NO_SHOW_HIDDEN);
         evas_object_smart_callback_add(mt, "dir_changed",
					_cb_files_dir_changed, cfdata);
	 evas_object_smart_callback_add(mt, "selected",
					_cb_files_selected_all2, cfdata);
         evas_object_smart_callback_add(mt, "selection_change",
					_cb_files_selection_change, cfdata);
         evas_object_smart_callback_add(mt, "changed",
					_cb_files_changed, cfdata);
	 e_fm2_icon_menu_start_extend_callback_set(mt, _cb_files_add_edited, cfdata);
	 if ((once) && (once->path))
            e_fm2_path_set(cfdata->gui.o_fm, once->path, "/");
	 else
            e_fm2_path_set(cfdata->gui.o_fm, cfdata->path, "/");
	 e_fm2_window_object_set(cfdata->gui.o_fm, E_OBJECT(cfd->dia->win));

         ob = e_widget_scrollframe_pan_add(evas, mt,
				     e_fm2_pan_set,
				     e_fm2_pan_get,
				     e_fm2_pan_max_get,
				     e_fm2_pan_child_size_get);
         cfdata->gui.o_frame = ob;
         e_widget_min_size_set(ob, 150, 220);
         e_widget_framelist_object_append(of, ob);
         e_box_pack_options_set(ob,
			  1, 1, /* fill */
			  1, 1, /* expand */
			  0.5, 0.5, /* align */
			  150, 220, /* min */
			  99999, 99999 /* max */
			  );
/*
         mt = e_widget_button_add(evas, _("Move application up"), "widget/up_arrow",
			   _cb_button_move_up, cfdata, NULL);
         cfdata->gui.o_move_up_button = mt;
         e_widget_framelist_object_append(of, mt);
	 e_widget_disabled_set(cfdata->gui.o_move_up_button, 1);

         mt = e_widget_button_add(evas, _("Move application down"), "widget/down_arrow",
			   _cb_button_move_down, cfdata, NULL);
         cfdata->gui.o_move_down_button = mt;
         e_widget_framelist_object_append(of, mt);
	 e_widget_disabled_set(cfdata->gui.o_move_down_button, 1);
*/
         mt = e_widget_button_add(evas, _("Regenerate/update \"Applications\" Menu"), "enlightenment/regenerate_menus",
			   _cb_button_regen, cfdata, NULL);
         cfdata->gui.o_regen_button = mt;
         e_widget_framelist_object_append(of, mt);

         e_widget_table_object_append(ot, of, 2, 0, 2, 4, 1, 1, 1, 1);
      }

   e_widget_list_object_append(o, ot, 1, 1, 0.5);
   e_dialog_resizable_set(cfd->dia, 1);

   return o;
}

static void
_move_file_up_in_order(const char *order, const char *file)
{
   char buf[4096];
   Evas_List *list = NULL, *l;
   FILE *f;

   snprintf(buf, sizeof(buf), "%s/.order", order);
   if (!ecore_file_exists(buf)) return;
   f = fopen(buf, "rb");
   if (!f) return;

   while (fgets(buf, sizeof(buf), f))
     {
	int len;

	len = strlen(buf);
	if (len > 0)
	  {
	     if (buf[len - 1] == '\n')
	       {
		  buf[len - 1] = 0;
		  len--;
	       }
	     if (strcmp(buf, file) != 0)
                list = evas_list_append(list, strdup(buf));
	     else
	        {
		   l = evas_list_last(list);
		   if (l)
                      list = evas_list_prepend_relative_list(list, strdup(buf), l);
		   else
                      list = evas_list_append(list, strdup(buf));
		}
	  }
     }
   fclose(f);

   snprintf(buf, sizeof(buf), "%s/.order", order);
   ecore_file_unlink(buf);
   f = fopen(buf, "wb");
   if (!f) return;
   for (l = list; l; l = l->next)
     {
        char *text;

        text = l->data;
	fprintf(f, "%s\n", text);
	free(text);
     }
   fclose(f);
   evas_list_free(list);

   snprintf(buf, sizeof(buf), "%s/.eap.cache.cfg", order);
   ecore_file_unlink(buf);

   return;
}

static void
_move_file_down_in_order(const char *order, const char *file)
{
   char buf[4096], *last;
   Evas_List *list = NULL, *l;
   FILE *f;

   snprintf(buf, sizeof(buf), "%s/.order", order);
   if (!ecore_file_exists(buf)) return;
   f = fopen(buf, "rb");
   if (!f) return;

   last = NULL;
   while (fgets(buf, sizeof(buf), f))
     {
	int len;

	len = strlen(buf);
	if (len > 0)
	  {
	     if (buf[len - 1] == '\n')
	       {
		  buf[len - 1] = 0;
		  len--;
	       }
	     if (strcmp(buf, file) != 0)
	        {
                   list = evas_list_append(list, strdup(buf));
		   if (last)
		      {
                         list = evas_list_append(list, last);
			 last = NULL;
		      }
		}
	     else
	        {
		   last = strdup(buf);
		}
	  }
     }
   if (last)
      {
          list = evas_list_append(list, last);
	  last = NULL;
      }
   fclose(f);

   snprintf(buf, sizeof(buf), "%s/.order", order);
   ecore_file_unlink(buf);
   f = fopen(buf, "wb");
   if (!f) return;
   for (l = list; l; l = l->next)
     {
        char *text;

        text = l->data;
	fprintf(f, "%s\n", text);
	free(text);
     }
   fclose(f);
   evas_list_free(list);

   snprintf(buf, sizeof(buf), "%s/.eap.cache.cfg", order);
   ecore_file_unlink(buf);

   return;
}



static void
_append_to_order(const char *order, const char *file)
{
   char buf[4096];
   Evas_List *list = NULL, *l;
   FILE *f;

   snprintf(buf, sizeof(buf), "%s/.order", order);
   if (ecore_file_exists(buf))
     {
        f = fopen(buf, "r");
	if (f)
	  {
             while (fgets(buf, sizeof(buf), f))
               {
	          int len;

	          len = strlen(buf);
	          if (len > 0)
	            {
	               if (buf[len - 1] == '\n')
	                 {
		            buf[len - 1] = 0;
		            len--;
	                 }
                       list = evas_list_append(list, strdup(buf));
	            }
               }
             fclose(f);
	  }
        snprintf(buf, sizeof(buf), "%s/.order", order);
        ecore_file_unlink(buf);
     }

   list = evas_list_append(list, strdup(file));

   f = fopen(buf, "w");
   if (!f) return;
   for (l = list; l; l = l->next)
     {
        char *text;

        text = l->data;
	fprintf(f, "%s\n", text);
	free(text);
     }
   fclose(f);
   evas_list_free(list);

   return;
}
