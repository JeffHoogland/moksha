/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

#define MOD_UNLOADED 0
#define MOD_ENABLED 1


static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

struct _E_Config_Dialog_Data
{
   E_Config_Dialog *cfd;
   int state;
   struct {
      Evas_Object *o_fm_all;
      Evas_Object *o_fm;
      Evas_Object *o_frame;
      Evas_Object *o_up_button;
      Evas_Object *o_create_button;
      Evas_Object *o_regen_button;
   } gui;
};

EAPI E_Config_Dialog *
e_int_config_apps(E_Container *con)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   v = E_NEW(E_Config_Dialog_View, 1);

   v->create_cfdata           = _create_data;
   v->free_cfdata             = _free_data;
   v->basic.create_widgets    = _basic_create_widgets;

   cfd = e_config_dialog_new(con,
			     _("All Applications"),
			     "E", "_config_applications_dialog",
			     "enlightenment/applications", 0, v, NULL);
   return cfd;
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
_cb_button_delete(void *data1, void *data2)
{
}

static void
_cb_button_regen(void *data1, void *data2)
{
   e_fdo_menu_to_order();
}


static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *of, *ob, *ot, *ilist, *mt;
   Evas_List *l;
   E_Fm2_Config fmc_all, fmc;
   char path_all[4096], path[4096], *homedir;
   int i;

   homedir = e_user_homedir_get();
   if (!homedir) return NULL;

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

   mt = e_widget_button_add(evas, _("Delete application"), "enlightenment/e",
			   _cb_button_delete, cfdata, NULL);
   cfdata->gui.o_create_button = mt;
   e_widget_framelist_object_append(of, mt);

   e_widget_table_object_append(ot, of, 0, 0, 2, 4, 1, 1, 1, 1);


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


   mt = e_widget_button_add(evas, _("Regenerate \"All Applications\" Menu"), "enlightenment/e",
			   _cb_button_regen, cfdata, NULL);
   cfdata->gui.o_regen_button = mt;
   e_widget_framelist_object_append(of, mt);

   e_widget_table_object_append(ot, of, 2, 0, 2, 4, 1, 1, 1, 1);

   e_widget_list_object_append(o, ot, 1, 1, 0.5);
   e_dialog_resizable_set(cfd->dia, 1);

   return o;
}
