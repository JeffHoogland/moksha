/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */

#include "e.h"

static void        *_create_data                (E_Config_Dialog *cfd);
static void         _free_data                  (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static void         _fill_data                  (E_Config_Dialog_Data *cfdata);
static int          _basic_apply_data           (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create_widgets       (E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

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
   
   char *theme;
};

EAPI E_Config_Dialog *
e_int_config_theme(E_Container *con)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   v = E_NEW(E_Config_Dialog_View, 1);
   
   v->create_cfdata           = _create_data;
   v->free_cfdata             = _free_data;
   v->basic.apply_cfdata      = _basic_apply_data;
   v->basic.create_widgets    = _basic_create_widgets;
   v->override_auto_apply = 1;
   cfd = e_config_dialog_new(con,
			     _("Theme Selector"),
			     "E", "_config_theme_dialog",
			     "enlightenment/themes", 0, v, NULL);
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
   Evas_List *selected;
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
   evas_list_free(selected);
   if (ecore_file_is_dir(buf)) return;
   E_FREE(cfdata->theme);
   cfdata->theme = strdup(buf);
   if (cfdata->o_preview)
     e_widget_preview_edje_set(cfdata->o_preview, buf, "desktop/background");
   if (cfdata->o_frame)
     e_widget_change(cfdata->o_frame);
}

static void
_cb_files_selected(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = data;
   printf("SEL\n");
}

static void
_cb_files_files_changed(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;
   char *p, *homedir, buf[4096];
   
   cfdata = data;
   if (!cfdata->theme) return;
   if (!cfdata->o_fm) return;
   p = (char *)e_fm2_real_path_get(cfdata->o_fm);
   if (p)
     {
	if (strncmp(p, cfdata->theme, strlen(p))) return;
     }
   homedir = e_user_homedir_get();
   if (!homedir) return;
   snprintf(buf, sizeof(buf), "%s/.e/e/themes", homedir);
   free(homedir);
   if (!p) return;
   if (!strncmp(cfdata->theme, buf, strlen(buf)))
     p = cfdata->theme + strlen(buf) + 1;
   else
     {
	snprintf(buf, sizeof(buf), "%s/data/themes", e_prefix_data_get());
	if (!strncmp(cfdata->theme, buf, strlen(buf)))
	  p = cfdata->theme + strlen(buf) + 1;
	else
	  p = cfdata->theme;
     }
   e_fm2_select_set(cfdata->o_fm, p, 1);
   e_fm2_file_show(cfdata->o_fm, p);
}

static void
_cb_dir(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;
   char path[4096], *homedir;
   
   cfdata = data;
   if (cfdata->fmdir == 1)
     {
	snprintf(path, sizeof(path), "%s/data/themes", e_prefix_data_get());
     }
   else
     {
	homedir = e_user_homedir_get();
	snprintf(path, sizeof(path), "%s/.e/e/themes", homedir);
	free(homedir);
     }
   e_fm2_path_set(cfdata->o_fm, path, "/");
}




static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   E_Config_Theme * c;
   char path[4096], *homedir;
   
   c = e_theme_config_get("theme");
   if (c)
     cfdata->theme = strdup(c->file);
   else
     {
	snprintf(path, sizeof(path), "%s/data/themes/default.edj", e_prefix_data_get());
	cfdata->theme = strdup(path);
     }
   if (cfdata->theme[0] != '/')
     {
        homedir = e_user_homedir_get();
        snprintf(path, sizeof(path), "%s/.e/e/themes/%s", homedir, cfdata->theme);
	if (ecore_file_exists(path))
	  {
	     E_FREE(cfdata->theme);
	     cfdata->theme = strdup(path);
	  }
	else
	  {
	     snprintf(path, sizeof(path), "%s/data/themes/%s", e_prefix_data_get(), cfdata->theme);
	     if (ecore_file_exists(path))
	       {
		  E_FREE(cfdata->theme);
		  cfdata->theme = strdup(path);
	       }
	  }
        free(homedir);
     }
   
   snprintf(path, sizeof(path), "%s/data/themes", e_prefix_data_get());
   if (!strncmp(cfdata->theme, path, strlen(path)))
     cfdata->fmdir = 1;
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
   E_FREE(cfdata->theme);
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
   E_Radio_Group *rg;
   
   homedir = e_user_homedir_get();
   if (!homedir) return NULL;

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
   fmc.list.sort.no_case = 1;
   fmc.list.sort.dirs.first = 0;
   fmc.list.sort.dirs.last = 1;
   fmc.selection.single = 1;
   fmc.selection.windows_modifiers = 0;
   e_fm2_config_set(o, &fmc);
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
   
   o = e_widget_preview_add(evas, 320, (320 * z->h) / z->w);
   cfdata->o_preview = o;
   if (cfdata->theme)
     f = cfdata->theme;
   e_widget_preview_edje_set(o, f, "desktop/background");
   e_widget_list_object_append(of, o, 1, 0, 0.5);
   
   e_widget_table_object_append(ot, of, 1, 0, 1, 1, 0, 1, 0, 1);
   
   free(homedir);
   e_dialog_resizable_set(cfd->dia, 1);
   return ot;
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   E_Action *a;
   
   /* Actually take our cfdata settings and apply them in real life */
   e_theme_config_set("theme", cfdata->theme);
   e_config_save_queue();
   
   a = e_action_find("restart");
   if ((a) && (a->func.go)) a->func.go(NULL, NULL);
   return 1; /* Apply was OK */
}
