/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

#define MOD_UNLOADED 0
#define MOD_ENABLED 1

/* TODO:
 * FDO menu generation puts symlinks to .desktops in /all/,
 *   if we edit one of those, replace symlink with resulting .desktop, 
 *   include original path in new .desktop.
 * Filtering/sorting of left side to make managing thousands of apps easy.
 *
 * These things require support from e_fm -
 * DND from left side to righ side, and to ibar etc.
 * Stop user from deleting standard directories on right side.
 * Stop user from creating new directories on left side.
 */


static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

struct _E_Config_Dialog_Data
{
   E_Config_Dialog *cfd;
   E_Fm2_Icon_Info *info;
   int state;
   struct {
      Evas_Object *o_fm_all;
      Evas_Object *o_fm;
      Evas_Object *o_frame;
      Evas_Object *o_up_button;
      Evas_Object *o_add_button;
      Evas_Object *o_create_button;
      Evas_Object *o_delete_left_button;
      Evas_Object *o_delete_right_button;
      Evas_Object *o_regen_button;
   } gui;
};

struct _E_Config_Once
{
   const char *label;
   int (*func) (void *data, const char *path);
   void *data;
};


EAPI E_Config_Dialog *
e_int_config_apps_once(E_Container *con, const char *label, int (*func) (void *data, const char *path), void *data)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   struct _E_Config_Once *once = NULL;

   v = E_NEW(E_Config_Dialog_View, 1);

   v->create_cfdata           = _create_data;
   v->free_cfdata             = _free_data;
   v->basic.create_widgets    = _basic_create_widgets;

   if (func)
      {
         /* FIXME: figure out some way of freeing this once we have finished with it. */
         once = E_NEW(struct _E_Config_Once, 1);
	 if (once)
	    {
	       once->label = label;
	       once->func = func;
	       once->data = data;
	    }
      }

   cfd = e_config_dialog_new(con,
			     _("All Applications"),
			     "E", "_config_applications_dialog",
			     "enlightenment/applications", 0, v, once);
   return cfd;
}

EAPI E_Config_Dialog *
e_int_config_apps(E_Container *con)
{
   return e_int_config_apps_once(con, NULL, NULL, NULL);
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
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
_cb_files_changed(void *data, Evas_Object *obj, void *event_info)
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
   if (cfdata->gui.o_frame)
     e_widget_scrollframe_child_pos_set(cfdata->gui.o_frame, 0, 0);
}

static void
_cb_button_create(void *data1, void *data2)
{
   E_App *a;
   E_Config_Dialog_Data *cfdata;

   cfdata = data1;
   a = e_app_empty_new(NULL);
   e_eap_edit_show(cfdata->cfd->con, a);
}

static void
_cb_files_selected(void *data, Evas_Object *obj, void *event_info)
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
      e_eap_edit_show(cfdata->cfd->con, a);
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
      e_eap_edit_show(cfdata->cfd->con, a);
}

static void
_cb_files_add_edited(void *data, Evas_Object *obj, E_Menu *m, E_Fm2_Icon_Info *info)
{
   E_Config_Dialog_Data *cfdata;
   E_Menu_Item *mi;

   cfdata = data;
   /* We need to get this info data to the menu callback, all this is created on the fly when user right clicks. */
   cfdata->info = info;
   mi = e_menu_item_new(m);
   e_menu_item_label_set(mi, _("Edit Application"));
   e_menu_item_callback_set(mi, _cb_files_edited, cfdata);
}

static void
_cb_button_delete_left(void *data1, void *data2)
{
   E_Config_Dialog_Data *cfdata;
   Evas_List *selected;
   E_Fm2_Icon_Info *ici;
   const char *realpath;
   char buf[4096];
   E_App *a;

   cfdata = data1;
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
printf("DELETING LEFT APPLICATION %s\n", buf);
// FIXME: find parent, so that e_app_remove can do the right thing.
//   a = e_app_new(buf, 0);
//   if (a)
//      e_app_remove(a);
   e_fm2_refresh(cfdata->gui.o_fm_all);
}

static void
_cb_button_delete_right(void *data1, void *data2)
{
   E_Config_Dialog_Data *cfdata;
   Evas_List *selected;
   E_Fm2_Icon_Info *ici;
   const char *realpath;
   char buf[4096];
   E_App *a;

   cfdata = data1;
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
//   if (ecore_file_is_dir(buf)) return;
printf("DELETING RIGHT APPLICATION %s\n", buf);
// FIXME: find parent .order, so that e_app_remove can do the right thing.
   e_fm2_refresh(cfdata->gui.o_fm);
}

static void
_cb_button_add(void *data1, void *data2)
{
   E_Config_Dialog_Data *cfdata;
   Evas_List *selected;
   E_Fm2_Icon_Info *ici;
   const char *realpath;
   char buf[4096];
   E_App *a, *parent;

   cfdata = data1;
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

   if (cfdata->cfd->data)
      {
         struct _E_Config_Once *once = NULL;

         once = cfdata->cfd->data;
	 once->func(once->data, buf);
      }
   else
      {
         if (!cfdata->gui.o_fm) return;

         a = e_app_new(buf, 0);
         realpath = e_fm2_real_path_get(cfdata->gui.o_fm);
         parent = e_app_new(realpath, 0);
         if ((a) && (parent))
            e_app_append(a, parent);
      }
   e_fm2_refresh(cfdata->gui.o_fm);
}

static void
_cb_button_regen(void *data1, void *data2)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data1;
   e_fdo_menu_to_order();
   if (cfdata->gui.o_fm_all)   e_fm2_refresh(cfdata->gui.o_fm_all);
   if (cfdata->gui.o_fm)       e_fm2_refresh(cfdata->gui.o_fm);
}


static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   struct _E_Config_Once *once = NULL;
   Evas_Object *o, *of, *ob, *ot, *ilist, *mt;
   Evas_List *l;
   E_Fm2_Config fmc_all, fmc;
   char path_all[4096], path[4096], *homedir;
   int i;

   homedir = e_user_homedir_get();
   if (!homedir) return NULL;
   
   if (cfdata->cfd->data)
      once = cfdata->cfd->data;

   o = e_widget_list_add(evas, 1, 0);
   ot = e_widget_table_add(evas, 1);

   of = e_widget_framelist_add(evas, _("All Applications"), 0);

   mt = e_widget_button_add(evas, _("Create a new application"), "enlightenment/e",
			   _cb_button_create, cfdata, NULL);
   cfdata->gui.o_create_button = mt;
   e_widget_framelist_object_append(of, mt);

   mt = e_fm2_add(evas);
   cfdata->gui.o_fm_all = mt;
   memset(&fmc_all, 0, sizeof(E_Fm2_Config));
   fmc_all.view.mode = E_FM2_VIEW_MODE_LIST;
   fmc_all.view.open_dirs_in_place = 1;
   fmc_all.view.selector = 1;
   fmc_all.view.single_click = 0;
   fmc_all.view.no_subdir_jump = 0;
   fmc_all.view.extra_file_source = NULL;
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
   evas_object_smart_callback_add(mt, "selected",
				  _cb_files_selected, cfdata);
   e_fm2_icon_menu_start_extend_callback_set(mt, _cb_files_add_edited, cfdata);
   snprintf(path_all, sizeof(path_all), "%s/.e/e/applications/all", homedir);
   e_fm2_path_set(cfdata->gui.o_fm_all, path_all, "/");

   ob = e_widget_scrollframe_pan_add(evas, mt,
				     e_fm2_pan_set,
				     e_fm2_pan_get,
				     e_fm2_pan_max_get,
				     e_fm2_pan_child_size_get);
   cfdata->gui.o_frame = ob;
   e_widget_min_size_set(ob, 150, 220);
   e_widget_framelist_object_append(of, ob);

   if (once)
      mt = e_widget_button_add(evas, _(once->label), "enlightenment/e",
			   _cb_button_add, cfdata, NULL);
   else
      mt = e_widget_button_add(evas, _("Add application ->"), "enlightenment/e",
			   _cb_button_add, cfdata, NULL);
   cfdata->gui.o_add_button = mt;
   e_widget_framelist_object_append(of, mt);

   mt = e_widget_button_add(evas, _("Delete application"), "enlightenment/e",
			   _cb_button_delete_left, cfdata, NULL);
   cfdata->gui.o_delete_left_button = mt;
   e_widget_framelist_object_append(of, mt);

   e_widget_table_object_append(ot, of, 0, 0, 2, 4, 1, 1, 1, 1);

   if (!once)
      {
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
	 fmc.view.extra_file_source = path_all;
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
         evas_object_smart_callback_add(mt, "dir_changed",
				  _cb_files_changed, cfdata);
         snprintf(path, sizeof(path), "%s/.e/e/applications", homedir);
         e_fm2_path_set(cfdata->gui.o_fm, path, "/");

         ob = e_widget_scrollframe_pan_add(evas, mt,
				     e_fm2_pan_set,
				     e_fm2_pan_get,
				     e_fm2_pan_max_get,
				     e_fm2_pan_child_size_get);
         cfdata->gui.o_frame = ob;
         e_widget_min_size_set(ob, 150, 220);
         e_widget_framelist_object_append(of, ob);

         mt = e_widget_button_add(evas, _("Remove application"), "enlightenment/e",
			   _cb_button_delete_right, cfdata, NULL);
         cfdata->gui.o_delete_right_button = mt;
         e_widget_framelist_object_append(of, mt);

         mt = e_widget_button_add(evas, _("Regenerate \"All Applications\" Menu"), "enlightenment/e",
			   _cb_button_regen, cfdata, NULL);
         cfdata->gui.o_regen_button = mt;
         e_widget_framelist_object_append(of, mt);

         e_widget_table_object_append(ot, of, 2, 0, 2, 4, 1, 1, 1, 1);
      }

   e_widget_list_object_append(o, ot, 1, 1, 0.5);
   e_dialog_resizable_set(cfd->dia, 1);

   return o;
}
