#include "e.h"

static void        *_create_data(E_Config_Dialog *cfd);
static void         _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int          _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
void                _ilist_cb_init_selected(void *data);

struct _E_Config_Dialog_Data 
{
   E_Config_Dialog *cfd;
   Evas_Object *o_frame;
   Evas_Object *o_fm;
   Evas_Object *o_up_button;
   Evas_Object *o_preview;
   Evas_Object *o_personal;
   Evas_Object *o_system;
   int fmdir;

   int show_splash;
   char *splash;
};

EAPI E_Config_Dialog *
e_int_config_startup(E_Container *con, const char *params __UNUSED__) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   
   if (e_config_dialog_find("E", "_config_startup_dialog")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);
   
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;
   
   cfd = e_config_dialog_new(con,
			     _("Startup Settings"),
			     "E", "_config_startup_dialog",
			     "enlightenment/startup", 0, v, NULL);
   return cfd;
}

static void
_cb_button_up(void *data1, void *data2)
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = data1;
   if (cfdata->o_fm)
     e_fm2_parent_go(cfdata->o_fm);
   if (cfdata->o_frame)
     e_widget_scrollframe_child_pos_set(cfdata->o_frame, 0, 0);
}

static void
_cb_files_changed(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = data;
   if (!cfdata->o_fm) return;
   if (!e_fm2_has_parent_get(cfdata->o_fm))
     {
	if (cfdata->o_up_button)
	  e_widget_disabled_set(cfdata->o_up_button, 1);
     }
   else
     {
	if (cfdata->o_up_button)
	  e_widget_disabled_set(cfdata->o_up_button, 0);
     }
   if (cfdata->o_frame)
     e_widget_scrollframe_child_pos_set(cfdata->o_frame, 0, 0);
}

static void
_cb_files_selection_change(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;
   Eina_List *selected;
   E_Fm2_Icon_Info *ici;
   const char *realpath;
   char buf[4096];
   
   cfdata = data;
   if (!cfdata->o_fm) return;
   selected = e_fm2_selected_list_get(cfdata->o_fm);
   if (!selected) return;
   ici = selected->data;
   realpath = e_fm2_real_path_get(cfdata->o_fm);
   if (!strcmp(realpath, "/"))
     snprintf(buf, sizeof(buf), "/%s", ici->file);
   else
     snprintf(buf, sizeof(buf), "%s/%s", realpath, ici->file);
   eina_list_free(selected);
   if (ecore_file_is_dir(buf)) return;
   E_FREE(cfdata->splash);
   cfdata->splash = strdup(buf);
   if (cfdata->o_preview)
     e_widget_preview_edje_set(cfdata->o_preview, buf, "e/init/splash");
   if (cfdata->o_frame)
     e_widget_change(cfdata->o_frame);
}

static void
_cb_files_selected(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = data;
}

static void
_cb_files_files_changed(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;
   const char *p, *homedir;
   char buf[4096];
   
   cfdata = data;
   if (!cfdata->splash) return;
   if (!cfdata->o_fm) return;
   p = e_fm2_real_path_get(cfdata->o_fm);
   if (p)
     {
	if (strncmp(p, cfdata->splash, strlen(p))) return;
     }
   homedir = e_user_homedir_get();
   snprintf(buf, sizeof(buf), "%s/.e/e/themes", homedir);
   if (!p) return;
   if (!strncmp(cfdata->splash, buf, strlen(buf)))
     p = cfdata->splash + strlen(buf) + 1;
   else
     {
	snprintf(buf, sizeof(buf), "%s/data/themes", e_prefix_data_get());
	if (!strncmp(cfdata->splash, buf, strlen(buf)))
	  p = cfdata->splash + strlen(buf) + 1;
	else
	  p = cfdata->splash;
     }
   e_fm2_select_set(cfdata->o_fm, p, 1);
   e_fm2_file_show(cfdata->o_fm, p);
}

static void
_cb_dir(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;
   char path[4096];
   const char *homedir;
   
   cfdata = data;
   if (cfdata->fmdir == 1)
     {
	snprintf(path, sizeof(path), "%s/data/themes", e_prefix_data_get());
     }
   else
     {
	homedir = e_user_homedir_get();
	snprintf(path, sizeof(path), "%s/.e/e/themes", homedir);
     }
   e_fm2_path_set(cfdata->o_fm, path, "/");
}

static void
_fill_data(E_Config_Dialog_Data *cfdata) 
{
   char path[4096];
   const char *homedir;

   cfdata->show_splash = e_config->show_splash;
   cfdata->splash = NULL;
   if (e_config->init_default_theme)
     cfdata->splash = strdup(e_config->init_default_theme);
   else
     {
        snprintf(path, sizeof(path), "%s/data/themes/default.edj", e_prefix_data_get());
	cfdata->splash = strdup(path);
     }
   if (cfdata->splash[0] != '/')
     {
	homedir = e_user_homedir_get();
	snprintf(path, sizeof(path), "%s/.e/e/themes/%s", homedir, cfdata->splash);
	if (ecore_file_exists(path))
	  {
	     E_FREE(cfdata->splash);
	     cfdata->splash = strdup(path);
	  }
	else
	  {
	     snprintf(path, sizeof(path), "%s/data/themes/%s", e_prefix_data_get(), cfdata->splash);
	     if (ecore_file_exists(path))
	       {
		  E_FREE(cfdata->splash);
		  cfdata->splash = strdup(path);
	       }
	  }
     }
   
   snprintf(path, sizeof(path), "%s/data/themes", e_prefix_data_get());
   if (!strncmp(cfdata->splash, path, strlen(path)))
     cfdata->fmdir = 1;
}

static void *
_create_data(E_Config_Dialog *cfd) 
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _fill_data(cfdata);
   cfd->cfdata = cfdata;
   cfdata->cfd = cfd;
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   E_FREE(cfdata->splash);
   E_FREE(cfdata);
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   e_config->show_splash = cfdata->show_splash;
   if (e_config->init_default_theme)
     eina_stringshare_del(e_config->init_default_theme);
   e_config->init_default_theme = NULL;
   
   if (cfdata->splash)
     {
	if (cfdata->splash[0])
	  {
	     const char *f;
	     
	     f = ecore_file_file_get(cfdata->splash);
	     e_config->init_default_theme = eina_stringshare_add(f);
	  }
     }
   
   e_config_save_queue();
   return 1;
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o, *ot, *of, *il, *ol;
   char path[4096];
   const char *homedir;
   E_Fm2_Config fmc;
   E_Zone *z;
   E_Radio_Group *rg;

   homedir = e_user_homedir_get();

   z = e_zone_current_get(cfd->con);
   
   ot = e_widget_table_add(evas, 0);
   ol = e_widget_table_add(evas, 0);
   il = e_widget_table_add(evas, 1);
   
   rg = e_widget_radio_group_new(&(cfdata->fmdir));
   o = e_widget_radio_add(evas, _("Personal"), 0, rg);
   cfdata->o_personal = o;
   evas_object_smart_callback_add(o, "changed",
				  _cb_dir, cfdata);
   e_widget_table_object_append(il, o, 0, 0, 1, 1, 1, 1, 0, 0);
   o = e_widget_radio_add(evas, _("System"), 1, rg);
   cfdata->o_system = o;
   evas_object_smart_callback_add(o, "changed",
				  _cb_dir, cfdata);
   e_widget_table_object_append(il, o, 1, 0, 1, 1, 1, 1, 0, 0);
   
   e_widget_table_object_append(ol, il, 0, 0, 1, 1, 0, 0, 0, 0);
   
   o = e_widget_button_add(evas, _("Go up a Directory"), "widget/up_dir",
			   _cb_button_up, cfdata, NULL);
   cfdata->o_up_button = o;
   e_widget_table_object_append(ol, o, 0, 1, 1, 1, 0, 0, 0, 0);
   
   if (cfdata->fmdir == 1)
     snprintf(path, sizeof(path), "%s/data/themes", e_prefix_data_get());
   else
     snprintf(path, sizeof(path), "%s/.e/e/themes", homedir);
   
   o = e_fm2_add(evas);
   cfdata->o_fm = o;
   memset(&fmc, 0, sizeof(E_Fm2_Config));
   fmc.view.mode = E_FM2_VIEW_MODE_LIST;
   fmc.view.open_dirs_in_place = 1;
   fmc.view.selector = 1;
   fmc.view.single_click = 0;
   fmc.view.no_subdir_jump = 0;
   fmc.icon.list.w = 48;
   fmc.icon.list.h = 48;
   fmc.icon.fixed.w = 1;
   fmc.icon.fixed.h = 1;
   fmc.icon.extension.show = 0;
   fmc.icon.key_hint = "e/init/splash";
   fmc.list.sort.no_case = 1;
   fmc.list.sort.dirs.first = 0;
   fmc.list.sort.dirs.last = 1;
   fmc.selection.single = 1;
   fmc.selection.windows_modifiers = 0;
   e_fm2_config_set(o, &fmc);
   e_fm2_icon_menu_flags_set(o, E_FM2_MENU_NO_SHOW_HIDDEN);
   evas_object_smart_callback_add(o, "dir_changed",
				  _cb_files_changed, cfdata);
   evas_object_smart_callback_add(o, "selection_change",
				  _cb_files_selection_change, cfdata);
   evas_object_smart_callback_add(o, "selected",
				  _cb_files_selected, cfdata);
   evas_object_smart_callback_add(o, "changed",
				  _cb_files_files_changed, cfdata);
   e_fm2_path_set(o, path, "/");
   
   of = e_widget_scrollframe_pan_add(evas, o,
				     e_fm2_pan_set,
				     e_fm2_pan_get,
				     e_fm2_pan_max_get,
				     e_fm2_pan_child_size_get);
   cfdata->o_frame = of;
   e_widget_min_size_set(of, 160, 160);
   e_widget_table_object_append(ol, of, 0, 2, 1, 1, 1, 1, 1, 1);
   e_widget_table_object_append(ot, ol, 0, 0, 1, 1, 1, 1, 1, 1);
   
   of = e_widget_list_add(evas, 0, 0);

   o = e_widget_check_add(evas, _("Show Splash Screen on Login"), 
			  &(cfdata->show_splash));
   e_widget_list_object_append(of, o, 1, 0, 0.0);
   
   o = e_widget_preview_add(evas, 320, (320 * z->h) / z->w);
   cfdata->o_preview = o;
   if (cfdata->splash)
     e_widget_preview_edje_set(o, cfdata->splash, "e/init/splash");
   e_widget_list_object_append(of, o, 0, 0, 0.5);
   
   e_widget_table_object_append(ot, of, 1, 0, 1, 1, 0, 0, 0, 0);
   e_dialog_resizable_set(cfd->dia, 1);
   return ot;
}
