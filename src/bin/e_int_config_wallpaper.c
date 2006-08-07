/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */

#include "e.h"

static void        *_create_data                (E_Config_Dialog *cfd);
static void         _free_data                  (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static void         _fill_data                  (E_Config_Dialog_Data *cfdata);
static int          _basic_apply_data           (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets       (E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int          _advanced_apply_data        (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_advanced_create_widgets    (E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

struct _E_Config_Dialog_Data
{
   E_Config_Dialog *cfd;
   Evas_Object *o_usrbg_frame;
   Evas_Object *o_usrbg_fm;
   Evas_Object *o_usrbg_up_button;
   Evas_Object *o_sysbg_frame;
   Evas_Object *o_sysbg_fm;
   Evas_Object *o_sysbg_up_button;
   Evas_Object *o_preview;
   Evas_Object *o_theme_bg;
   int use_theme_bg;
   char *bg;
   /* advanced */
   int all_this_desk_screen;
   /* dialogs */
   E_Win *win_import;
};

EAPI E_Config_Dialog *
e_int_config_wallpaper(E_Container *con)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   v = E_NEW(E_Config_Dialog_View, 1);
   
   v->create_cfdata           = _create_data;
   v->free_cfdata             = _free_data;
   v->basic.apply_cfdata      = _basic_apply_data;
   v->basic.create_widgets    = _basic_create_widgets;
   v->advanced.apply_cfdata   = _advanced_apply_data;
   v->advanced.create_widgets = _advanced_create_widgets;
   v->override_auto_apply = 1;
   cfd = e_config_dialog_new(con, _("Wallpaper Settings"), "enlightenment/background", 0, v, NULL);
   return cfd;
}

EAPI void
e_int_config_wallpaper_update(E_Config_Dialog *dia, char *file)
{
   E_Config_Dialog_Data *cfdata;
   char path[4096], *homedir;
   
   cfdata = dia->cfdata;
   homedir = e_user_homedir_get();
   if (!homedir) return;
   snprintf(path, sizeof(path), "%s/.e/e/backgrounds", homedir);
   E_FREE(cfdata->bg);
   cfdata->bg = strdup(file);
   cfdata->use_theme_bg = 0;
   if (cfdata->o_theme_bg)
     e_widget_check_checked_set(cfdata->o_theme_bg, cfdata->use_theme_bg);
   if (cfdata->o_usrbg_fm)
     e_fm2_path_set(cfdata->o_usrbg_fm, path, "/");
   if (cfdata->o_preview)
     e_widget_preview_edje_set(cfdata->o_preview, cfdata->bg, "desktop/background");
   if (cfdata->o_theme_bg)
     e_widget_check_checked_set(cfdata->o_theme_bg, 0);
   cfdata->use_theme_bg = 0;
   if (cfdata->o_usrbg_frame)
     e_widget_change(cfdata->o_usrbg_frame);
   if (cfdata->o_sysbg_fm)
     e_fm2_select_set(cfdata->o_sysbg_fm, "", 0);
}

EAPI void
e_int_config_wallpaper_import_done(E_Config_Dialog *dia)
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = dia->cfdata;
   cfdata->win_import = NULL;
   printf("DONE!\n");
}


static void
_cb_usrbg_button_up(void *data1, void *data2)
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = data1;
   if (cfdata->o_usrbg_fm)
     e_fm2_parent_go(cfdata->o_usrbg_fm);
   if (cfdata->o_usrbg_frame)
     e_widget_scrollframe_child_pos_set(cfdata->o_usrbg_frame, 0, 0);
}

static void
_cb_usrbg_files_changed(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = data;
   if (!cfdata->o_usrbg_fm) return;
   if (!e_fm2_has_parent_get(cfdata->o_usrbg_fm))
     {
	if (cfdata->o_usrbg_up_button)
	  e_widget_disabled_set(cfdata->o_usrbg_up_button, 1);
     }
   else
     {
	if (cfdata->o_usrbg_up_button)
	  e_widget_disabled_set(cfdata->o_usrbg_up_button, 0);
     }
   if (cfdata->o_usrbg_frame)
     e_widget_scrollframe_child_pos_set(cfdata->o_usrbg_frame, 0, 0);
}

static void
_cb_usrbg_files_selection_change(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;
   Evas_List *selected;
   E_Fm2_Icon_Info *ici;
   const char *realpath;
   char buf[4096];
   
   cfdata = data;
   if (!cfdata->o_usrbg_fm) return;
   selected = e_fm2_selected_list_get(cfdata->o_usrbg_fm);
   if (!selected) return;
   ici = selected->data;
   realpath = e_fm2_real_path_get(cfdata->o_usrbg_fm);
   if (!strcmp(realpath, "/"))
     snprintf(buf, sizeof(buf), "/%s", ici->file);
   else
     snprintf(buf, sizeof(buf), "%s/%s", realpath, ici->file);
   evas_list_free(selected);
   if (ecore_file_is_dir(buf)) return;
   E_FREE(cfdata->bg);
   cfdata->bg = strdup(buf);
   if (cfdata->o_preview)
     e_widget_preview_edje_set(cfdata->o_preview, buf, "desktop/background");
   if (cfdata->o_theme_bg)
     e_widget_check_checked_set(cfdata->o_theme_bg, 0);
   cfdata->use_theme_bg = 0;
   if (cfdata->o_usrbg_frame)
     e_widget_change(cfdata->o_usrbg_frame);
   e_fm2_select_set(cfdata->o_sysbg_fm, "", 0);
}

static void
_cb_usrbg_files_selected(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = data;
}

static void
_cb_usrbg_files_files_changed(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;
   char *p, *homedir, buf[4096];
   
   cfdata = data;
   if (!cfdata->bg) return;
   if (!cfdata->o_usrbg_fm) return;
   p = (char *)e_fm2_real_path_get(cfdata->o_usrbg_fm);
   if (p)
     {
	if (strncmp(p, cfdata->bg, strlen(p))) return;
     }
   homedir = e_user_homedir_get();
   if (!homedir) return;
   snprintf(buf, sizeof(buf), "%s/.e/e/backgrounds", homedir);
   free(homedir);
   if (!p) return;
   if (!strncmp(cfdata->bg, buf, strlen(buf)))
     p = cfdata->bg + strlen(buf) + 1;
   else
     p = cfdata->bg;
   e_fm2_select_set(cfdata->o_usrbg_fm, p, 1);
   e_fm2_file_show(cfdata->o_usrbg_fm, p);
}


static void
_cb_sysbg_button_up(void *data1, void *data2)
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = data1;
   if (cfdata->o_sysbg_fm)
     e_fm2_parent_go(cfdata->o_sysbg_fm);
   if (cfdata->o_sysbg_frame)
     e_widget_scrollframe_child_pos_set(cfdata->o_sysbg_frame, 0, 0);
}

static void
_cb_sysbg_files_changed(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = data;
   if (!cfdata->o_sysbg_fm) return;
   if (!e_fm2_has_parent_get(cfdata->o_sysbg_fm))
     {
	if (cfdata->o_sysbg_up_button)
	  e_widget_disabled_set(cfdata->o_sysbg_up_button, 1);
     }
   else
     {
	if (cfdata->o_sysbg_up_button)
	  e_widget_disabled_set(cfdata->o_sysbg_up_button, 0);
     }
   if (cfdata->o_sysbg_frame)
     e_widget_scrollframe_child_pos_set(cfdata->o_sysbg_frame, 0, 0);
}

static void
_cb_sysbg_files_selection_change(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;
   Evas_List *selected;
   E_Fm2_Icon_Info *ici;
   const char *realpath;
   char buf[4096];
   
   cfdata = data;
   if (!cfdata->o_sysbg_fm) return;
   selected = e_fm2_selected_list_get(cfdata->o_sysbg_fm);
   if (!selected) return;
   ici = selected->data;
   realpath = e_fm2_real_path_get(cfdata->o_sysbg_fm);
   if (!strcmp(realpath, "/"))
     snprintf(buf, sizeof(buf), "/%s", ici->file);
   else
     snprintf(buf, sizeof(buf), "%s/%s", realpath, ici->file);
   evas_list_free(selected);
   if (ecore_file_is_dir(buf)) return;
   E_FREE(cfdata->bg);
   cfdata->bg = strdup(buf);
   if (cfdata->o_preview)
     e_widget_preview_edje_set(cfdata->o_preview, buf, "desktop/background");
   if (cfdata->o_theme_bg)
     e_widget_check_checked_set(cfdata->o_theme_bg, 0);
   cfdata->use_theme_bg = 0;
   if (cfdata->o_sysbg_frame)
     e_widget_change(cfdata->o_sysbg_frame);
   e_fm2_select_set(cfdata->o_usrbg_fm, "", 0);
}

static void
_cb_sysbg_files_selected(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = data;
}

static void
_cb_sysbg_files_files_changed(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;
   char *p, buf[4096];
   
   cfdata = data;
   if (!cfdata->bg) return;
   if (!cfdata->o_sysbg_fm) return;
   p = (char *)e_fm2_real_path_get(cfdata->o_sysbg_fm);
   if (p)
     {
	if (strncmp(p, cfdata->bg, strlen(p))) return;
     }
   snprintf(buf, sizeof(buf), "%s/data/backgrounds", e_prefix_data_get());
   if (!strncmp(cfdata->bg, buf, strlen(buf)))
     p = cfdata->bg + strlen(buf) + 1;
   else
     p = cfdata->bg;
   if (!p) return;
   e_fm2_select_set(cfdata->o_sysbg_fm, p, 1);
   e_fm2_file_show(cfdata->o_sysbg_fm, p);
}


static void
_cb_theme_wallpaper(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;
   const char *f;
   
   cfdata = data;
   if (cfdata->use_theme_bg)
     {
	f = e_theme_edje_file_get("base/theme/backgrounds", "desktop/background");
	if (cfdata->o_preview)
	  e_widget_preview_edje_set(cfdata->o_preview, f, "desktop/background");
     }
   else
     {
	if (cfdata->bg)
	  {
	     if (cfdata->o_preview)
	       e_widget_preview_edje_set(cfdata->o_preview, cfdata->bg, "desktop/background");
	  }
     }
}

static void
_cb_import(void *data1, void *data2)
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = data1;
   if (cfdata->win_import)
     {
	e_win_raise(cfdata->win_import);
     }
   else cfdata->win_import = e_int_config_wallpaper_import(cfdata->cfd);
}




static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   E_Zone *z;
   E_Desk *d;
   Evas_List *l;
   
   if (e_config->desktop_default_background)
     cfdata->bg = strdup(e_config->desktop_default_background);
   
   z = e_zone_current_get(cfdata->cfd->con);
   if (!z) return;
   d = e_desk_current_get(z);
   if (!d) return;
   for (l = e_config->desktop_backgrounds; l; l = l->next)
     {
	E_Config_Desktop_Background *cfbg;
	
	cfbg = l->data;
	if (((cfbg->container == z->container->num) ||
	     (cfbg->container < 0)) && 
	    ((cfbg->zone == z->num) ||
	     (cfbg->zone < 0)) &&
	    ((cfbg->desk_x == d->x) ||
	     (cfbg->desk_x < 0)) && 
	    ((cfbg->desk_y == d->y) ||
	     (cfbg->desk_y < 0)))
	  {
	     E_FREE(cfdata->bg);
	     cfdata->bg = strdup(cfbg->file);
	     if ((cfbg->container >= 0) ||
		 (cfbg->zone >= 0))
	       cfdata->all_this_desk_screen = 2;
	     if ((cfbg->desk_x >= 0) ||
		 (cfbg->desk_y >= 0))
	       cfdata->all_this_desk_screen = 1;
	     break;
	  }
     }

   if (!cfdata->bg) cfdata->use_theme_bg = 1;
}

static void *
_create_data(E_Config_Dialog *cfd)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   cfd->cfdata = cfdata;
   cfdata->cfd = cfd;
   _fill_data(cfdata);
   return cfdata;
}

static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   if (cfdata->win_import) e_int_config_wallpaper_del(cfdata->win_import);
   E_FREE(cfdata->bg);
   free(cfdata);
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *ot, *of, *il, *ol;
   char path[4096], *homedir;
   const char *f;
   E_Fm2_Config fmc;
   E_Zone *z;
   
   homedir = e_user_homedir_get();
   if (!homedir) return NULL;

   z = e_zone_current_get(cfd->con);
   
   ot = e_widget_table_add(evas, 0);
   
  
   ol = e_widget_frametable_add(evas, _("Personal Wallpapers"), 0);
			  
   o = e_widget_button_add(evas, _("Go up a Directory"), "widget/up_dir",
			   _cb_usrbg_button_up, cfdata, NULL);
   cfdata->o_usrbg_up_button = o;
   e_widget_frametable_object_append(ol, o, 0, 0, 1, 1, 0, 0, 0, 0);
   
   snprintf(path, sizeof(path), "%s/.e/e/backgrounds", homedir);
   
   o = e_fm2_add(evas);
   cfdata->o_usrbg_fm = o;
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
   fmc.list.sort.no_case = 1;
   fmc.list.sort.dirs.first = 0;
   fmc.list.sort.dirs.last = 1;
   fmc.selection.single = 1;
   fmc.selection.windows_modifiers = 0;
   e_fm2_config_set(o, &fmc);
   evas_object_smart_callback_add(o, "dir_changed",
				  _cb_usrbg_files_changed, cfdata);
   evas_object_smart_callback_add(o, "selection_change",
				  _cb_usrbg_files_selection_change, cfdata);
   evas_object_smart_callback_add(o, "selected",
				  _cb_usrbg_files_selected, cfdata);
   evas_object_smart_callback_add(o, "changed",
				  _cb_usrbg_files_files_changed, cfdata);
   e_fm2_path_set(o, path, "/");

   of = e_widget_scrollframe_pan_add(evas, o,
				     e_fm2_pan_set,
				     e_fm2_pan_get,
				     e_fm2_pan_max_get,
				     e_fm2_pan_child_size_get);
   cfdata->o_usrbg_frame = of;
   e_widget_min_size_set(of, 160, 256);
   e_widget_frametable_object_append(ol, of, 0, 1, 1, 1, 1, 1, 1, 1);
   e_widget_table_object_append(ot, ol, 0, 0, 1, 1, 1, 1, 1, 1);
   
   
   ol = e_widget_frametable_add(evas, _("System Wallpapers"), 0);
			  
   o = e_widget_button_add(evas, _("Go up a Directory"), "widget/up_dir",
			   _cb_sysbg_button_up, cfdata, NULL);
   cfdata->o_sysbg_up_button = o;
   e_widget_frametable_object_append(ol, o, 0, 0, 1, 1, 0, 0, 0, 0);
   
   snprintf(path, sizeof(path), "%s/data/backgrounds", e_prefix_data_get());
   
   o = e_fm2_add(evas);
   cfdata->o_sysbg_fm = o;
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
   fmc.list.sort.no_case = 1;
   fmc.list.sort.dirs.first = 0;
   fmc.list.sort.dirs.last = 1;
   fmc.selection.single = 1;
   fmc.selection.windows_modifiers = 0;
   e_fm2_config_set(o, &fmc);
   evas_object_smart_callback_add(o, "dir_changed",
				  _cb_sysbg_files_changed, cfdata);
   evas_object_smart_callback_add(o, "selection_change",
				  _cb_sysbg_files_selection_change, cfdata);
   evas_object_smart_callback_add(o, "selected",
				  _cb_sysbg_files_selected, cfdata);
   evas_object_smart_callback_add(o, "changed",
				  _cb_sysbg_files_files_changed, cfdata);
   e_fm2_path_set(o, path, "/");

   of = e_widget_scrollframe_pan_add(evas, o,
				     e_fm2_pan_set,
				     e_fm2_pan_get,
				     e_fm2_pan_max_get,
				     e_fm2_pan_child_size_get);
   cfdata->o_sysbg_frame = of;
   e_widget_min_size_set(of, 160, 256);
   e_widget_frametable_object_append(ol, of, 0, 1, 1, 1, 1, 1, 1, 1);
   e_widget_table_object_append(ot, ol, 2, 0, 1, 1, 1, 1, 1, 1);
   
   il = e_widget_list_add(evas, 0, 0);
   o = e_widget_check_add(evas, _("Theme Wallpaper"), &cfdata->use_theme_bg);
   cfdata->o_theme_bg = o;
   evas_object_smart_callback_add(o, "changed",
				  _cb_theme_wallpaper, cfdata);
   e_widget_list_object_append(il, o, 1, 0, 0.5);

   ol = e_widget_list_add(evas, 1, 1);
   o = e_widget_button_add(evas, _("Picture..."), "enlightenment/picture",
			   _cb_import, cfdata, NULL);
   e_widget_list_object_append(ol, o, 1, 0, 0.5);
   o = e_widget_button_add(evas, _("Gradient..."), "enlightenment/gradient",
			   NULL, cfdata, NULL);
   e_widget_list_object_append(ol, o, 1, 0, 0.5);
   e_widget_list_object_append(il, ol, 1, 0, 0.5);
   
   o = e_widget_preview_add(evas, 240, (240 * z->h) / z->w);
   cfdata->o_preview = o;
   if (cfdata->bg)
     f = cfdata->bg;
   else
     f = e_theme_edje_file_get("base/theme/backgrounds", "desktop/background");
   e_widget_preview_edje_set(o, f, "desktop/background");
   e_widget_list_object_append(il, o, 0, 0, 0.5);
   
   e_widget_table_object_append(ot, il, 1, 0, 1, 1, 0, 1, 0, 1);
   
   free(homedir);
   e_dialog_resizable_set(cfd->dia, 1);
   return ot;
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   while (e_config->desktop_backgrounds)
     {
	E_Config_Desktop_Background *cfbg;
	cfbg = e_config->desktop_backgrounds->data;
	e_bg_del(cfbg->container, cfbg->zone, cfbg->desk_x, cfbg->desk_y);
     }
   if (e_config->desktop_default_background)
     evas_stringshare_del(e_config->desktop_default_background);
   if ((cfdata->use_theme_bg) || (!cfdata->bg))
     e_config->desktop_default_background = NULL;
   else
     e_config->desktop_default_background = evas_stringshare_add(cfdata->bg);
   
   cfdata->all_this_desk_screen = 0;
   
   e_bg_update();
   e_config_save_queue();
   return 1;
}

static Evas_Object *
_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *ot, *of, *il, *ol;
   char path[4096], *homedir;
   const char *f;
   E_Fm2_Config fmc;
   E_Zone *z;
   E_Radio_Group *rg;
   
   homedir = e_user_homedir_get();
   if (!homedir) return NULL;

   z = e_zone_current_get(cfd->con);
   
   ot = e_widget_table_add(evas, 0);
   
  
   ol = e_widget_frametable_add(evas, _("Personal Wallpapers"), 0);
			  
   o = e_widget_button_add(evas, _("Go up a Directory"), "widget/up_dir",
			   _cb_usrbg_button_up, cfdata, NULL);
   cfdata->o_usrbg_up_button = o;
   e_widget_frametable_object_append(ol, o, 0, 0, 1, 1, 0, 0, 0, 0);
   
   snprintf(path, sizeof(path), "%s/.e/e/backgrounds", homedir);
   
   o = e_fm2_add(evas);
   cfdata->o_usrbg_fm = o;
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
   fmc.list.sort.no_case = 1;
   fmc.list.sort.dirs.first = 0;
   fmc.list.sort.dirs.last = 1;
   fmc.selection.single = 1;
   fmc.selection.windows_modifiers = 0;
   e_fm2_config_set(o, &fmc);
   evas_object_smart_callback_add(o, "dir_changed",
				  _cb_usrbg_files_changed, cfdata);
   evas_object_smart_callback_add(o, "selection_change",
				  _cb_usrbg_files_selection_change, cfdata);
   evas_object_smart_callback_add(o, "selected",
				  _cb_usrbg_files_selected, cfdata);
   evas_object_smart_callback_add(o, "changed",
				  _cb_usrbg_files_files_changed, cfdata);
   e_fm2_path_set(o, path, "/");

   of = e_widget_scrollframe_pan_add(evas, o,
				     e_fm2_pan_set,
				     e_fm2_pan_get,
				     e_fm2_pan_max_get,
				     e_fm2_pan_child_size_get);
   cfdata->o_usrbg_frame = of;
   e_widget_min_size_set(of, 160, 256);
   e_widget_frametable_object_append(ol, of, 0, 1, 1, 1, 1, 1, 1, 1);
   e_widget_table_object_append(ot, ol, 0, 0, 1, 1, 1, 1, 1, 1);
   
   
   ol = e_widget_frametable_add(evas, _("System Wallpapers"), 0);
			  
   o = e_widget_button_add(evas, _("Go up a Directory"), "widget/up_dir",
			   _cb_sysbg_button_up, cfdata, NULL);
   cfdata->o_sysbg_up_button = o;
   e_widget_frametable_object_append(ol, o, 0, 0, 1, 1, 0, 0, 0, 0);
   
   snprintf(path, sizeof(path), "%s/data/backgrounds", e_prefix_data_get());
   
   o = e_fm2_add(evas);
   cfdata->o_sysbg_fm = o;
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
   fmc.list.sort.no_case = 1;
   fmc.list.sort.dirs.first = 0;
   fmc.list.sort.dirs.last = 1;
   fmc.selection.single = 1;
   fmc.selection.windows_modifiers = 0;
   e_fm2_config_set(o, &fmc);
   evas_object_smart_callback_add(o, "dir_changed",
				  _cb_sysbg_files_changed, cfdata);
   evas_object_smart_callback_add(o, "selection_change",
				  _cb_sysbg_files_selection_change, cfdata);
   evas_object_smart_callback_add(o, "selected",
				  _cb_sysbg_files_selected, cfdata);
   evas_object_smart_callback_add(o, "changed",
				  _cb_sysbg_files_files_changed, cfdata);
   e_fm2_path_set(o, path, "/");

   of = e_widget_scrollframe_pan_add(evas, o,
				     e_fm2_pan_set,
				     e_fm2_pan_get,
				     e_fm2_pan_max_get,
				     e_fm2_pan_child_size_get);
   cfdata->o_sysbg_frame = of;
   e_widget_min_size_set(of, 160, 256);
   e_widget_frametable_object_append(ol, of, 0, 1, 1, 1, 1, 1, 1, 1);
   e_widget_table_object_append(ot, ol, 2, 0, 1, 1, 1, 1, 1, 1);
   
   il = e_widget_list_add(evas, 0, 0);
   o = e_widget_check_add(evas, _("Theme Wallpaper"), &cfdata->use_theme_bg);
   cfdata->o_theme_bg = o;
   evas_object_smart_callback_add(o, "changed",
				  _cb_theme_wallpaper, cfdata);
   e_widget_list_object_append(il, o, 1, 0, 0.5);

   ol = e_widget_list_add(evas, 1, 1);
   o = e_widget_button_add(evas, _("Picture..."), "enlightenment/picture",
			   _cb_import, cfdata, NULL);
   e_widget_list_object_append(ol, o, 1, 0, 0.5);
   o = e_widget_button_add(evas, _("Gradient..."), "enlightenment/gradient",
			   NULL, cfdata, NULL);
   e_widget_list_object_append(ol, o, 1, 0, 0.5);
   e_widget_list_object_append(il, ol, 1, 0, 0.5);
   
   o = e_widget_preview_add(evas, 240, (240 * z->h) / z->w);
   cfdata->o_preview = o;
   if (cfdata->bg)
     f = cfdata->bg;
   else
     f = e_theme_edje_file_get("base/theme/backgrounds", "desktop/background");
   e_widget_preview_edje_set(o, f, "desktop/background");
   e_widget_list_object_append(il, o, 0, 0, 0.5);
   
   ol = e_widget_framelist_add(evas, _("Where to place the Wallpaper"), 0);
   e_widget_framelist_content_align_set(ol, 0.0, 0.0);
   rg = e_widget_radio_group_new(&(cfdata->all_this_desk_screen));

   o = e_widget_radio_add(evas, _("All Desktops"), 0, rg);
   e_widget_framelist_object_append(ol, o);
   o = e_widget_radio_add(evas, _("This Desktop"), 1, rg);
   e_widget_framelist_object_append(ol, o);
   o = e_widget_radio_add(evas, _("This Screen"), 2, rg);
   if (!((e_util_container_zone_number_get(0, 1)) ||
	 (e_util_container_zone_number_get(1, 0))))
     e_widget_disabled_set(o, 1);
   e_widget_framelist_object_append(ol, o);
   
   e_widget_list_object_append(il, ol, 0, 1, 0.5);
   
   e_widget_table_object_append(ot, il, 1, 0, 1, 1, 0, 1, 0, 1);
   
   free(homedir);
   e_dialog_resizable_set(cfd->dia, 1);
   return ot;
}

static int
_advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   Evas_List *fl = NULL, *l;
   E_Zone *z;
   E_Desk *d;
   
   z = e_zone_current_get(cfdata->cfd->con);
   if (!z) return 0;
   d = e_desk_current_get(z);
   if (!d) return 0;
   if (cfdata->use_theme_bg)
     {
	while (e_config->desktop_backgrounds)
	  {
	     E_Config_Desktop_Background *cfbg;
	     cfbg = e_config->desktop_backgrounds->data;
	     e_bg_del(cfbg->container, cfbg->zone, cfbg->desk_x, cfbg->desk_y);
	  }
	if (e_config->desktop_default_background)
	  evas_stringshare_del(e_config->desktop_default_background);
	e_config->desktop_default_background = NULL;
     }
   else
     {
	if (cfdata->all_this_desk_screen == 0)
	  {
	     while (e_config->desktop_backgrounds)
	       {
		  E_Config_Desktop_Background *cfbg;
		  cfbg = e_config->desktop_backgrounds->data;
		  e_bg_del(cfbg->container, cfbg->zone, cfbg->desk_x, cfbg->desk_y);
	       }
	     if (e_config->desktop_default_background)
	       evas_stringshare_del(e_config->desktop_default_background);
	     e_config->desktop_default_background = evas_stringshare_add(cfdata->bg);
	  }
	else if (cfdata->all_this_desk_screen == 1)
	  {
	     e_bg_del(z->container->num, z->num, d->x, d->y);
	     e_bg_del(z->container->num, -1, d->x, d->y);
	     e_bg_del(-1, z->num, d->x, d->y);
	     e_bg_del(-1, -1, d->x, d->y);
	     e_bg_add(z->container->num, z->num, d->x, d->y, cfdata->bg);
	     
	  }
	else if (cfdata->all_this_desk_screen == 2)
	  {
	     for (l = e_config->desktop_backgrounds; l; l = l->next)
	       {
		  E_Config_Desktop_Background *cfbg;
		  
		  cfbg = l->data;
		  if (
		      (((cfbg->container == z->container->num) ||
			(cfbg->container < 0)) && 
		       ((cfbg->zone == 0) ||
			(cfbg->zone < 0)))
		      ||
		      (((cfbg->zone == z->num) ||
			(cfbg->zone < 0)) && 
		       ((cfbg->container == 0) ||
			(cfbg->container < 0)))
		      )
		    fl = evas_list_append(fl, cfbg);
	       }
	     while (fl)
	       {
		  E_Config_Desktop_Background *cfbg;
		  cfbg = fl->data;
		  e_bg_del(cfbg->container, cfbg->zone, cfbg->desk_x, cfbg->desk_y);
		  fl = evas_list_remove_list(fl, fl);
	       }
	     e_bg_add(z->container->num, z->num, -1, -1, cfdata->bg);
	  }
     }
   e_bg_update();
   e_config_save_queue();
   return 1;
}
