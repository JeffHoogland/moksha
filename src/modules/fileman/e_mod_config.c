/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"
#include "e_fm_hal.h"

struct _E_Config_Dialog_Data 
{
   /* general view mode */
   struct {
      int   mode;
      int   open_dirs_in_place;
      int   selector;
      int   single_click;
      int   no_subdir_jump;
      int   no_subdir_drop;
      int   always_order;
      int   link_drop;
      int   fit_custom_pos;
      int   show_full_path;
      int   show_desktop_icons;
      int   show_toolbar;
   } view;
   /* display of icons */
   struct 
     {
	struct 
	  {
	     int w, h;
	  } icon, list, fixed;
	struct 
	  {
	     int show;
	  } extension;
	const char *key_hint;
     } icon;
   /* how to sort files */
   struct 
     {
	struct 
	  {
	     int case_sen;
	     struct {
		int first, last;
	     } dirs;
	  } sort;
     } list;
   /* control how you can select files */
   struct 
     {
	int single;
	int windows_modifiers;
     } selection;
   /* the background - if any, and how to handle it */
   /* FIXME: not implemented yet */
   struct 
     {
	const char *background;
	const char *frame, *icons;
	int fixed;
     } theme;

   struct
     {
        int desktop;
     } hal;

   E_Config_Dialog *cfd;
};

static void *_create_data(E_Config_Dialog *cfd);
static void  _fill_data(E_Config_Dialog_Data *cfdata);
static void  _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int   _basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

EAPI E_Config_Dialog *
e_int_config_fileman(E_Container *con, const char *params __UNUSED__) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   
   if (e_config_dialog_find("E", "_config_fileman_dialog")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply;
   v->basic.create_widgets = _basic_create;
   cfd = e_config_dialog_new(con, _("Fileman Settings"), "E", 
			     "_config_fileman_dialog",
			     "system-file-manager", 0, v, NULL);
   return cfd;
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
_fill_data(E_Config_Dialog_Data *cfdata) 
{
   cfdata->view.mode = fileman_config->view.mode;
   cfdata->view.open_dirs_in_place = fileman_config->view.open_dirs_in_place;
   cfdata->view.single_click = fileman_config->view.single_click;
   cfdata->view.show_full_path = fileman_config->view.show_full_path;
   cfdata->view.show_desktop_icons = fileman_config->view.show_desktop_icons;
   cfdata->view.show_toolbar = fileman_config->view.show_toolbar;
   cfdata->icon.icon.w = fileman_config->icon.icon.w;
   cfdata->icon.icon.h = fileman_config->icon.icon.h;
   cfdata->icon.extension.show = fileman_config->icon.extension.show;
   cfdata->selection.windows_modifiers = fileman_config->selection.windows_modifiers;
   cfdata->list.sort.dirs.first = fileman_config->list.sort.dirs.first;
   cfdata->list.sort.case_sen = !(fileman_config->list.sort.no_case);
   cfdata->hal.desktop = e_config->hal_desktop;
}

static void 
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   E_FREE(cfd->data);
   E_FREE(cfdata);
}

static int 
_basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   fileman_config->view.mode = cfdata->view.mode;
   fileman_config->view.open_dirs_in_place = cfdata->view.open_dirs_in_place;
   fileman_config->view.single_click = cfdata->view.single_click;
   fileman_config->view.show_full_path = cfdata->view.show_full_path;
   fileman_config->view.show_desktop_icons = cfdata->view.show_desktop_icons;
   fileman_config->view.show_toolbar = cfdata->view.show_toolbar;
   fileman_config->icon.extension.show = cfdata->icon.extension.show;

   fileman_config->selection.windows_modifiers = cfdata->selection.windows_modifiers;

   /* Make these two equal so that icons are proportioned correctly */
   fileman_config->icon.icon.w = cfdata->icon.icon.w;
   fileman_config->icon.icon.h = cfdata->icon.icon.w;
   
   fileman_config->list.sort.dirs.first = cfdata->list.sort.dirs.first;
   fileman_config->list.sort.dirs.last = !(cfdata->list.sort.dirs.first);
   fileman_config->list.sort.no_case = !(cfdata->list.sort.case_sen);

   e_config->hal_desktop = cfdata->hal.desktop;
   if(e_config->hal_desktop) 
     e_fm2_hal_show_desktop_icons();
   else
     e_fm2_hal_hide_desktop_icons();
     
   e_config_save_queue();
   
   /* FIXME: reload/refresh existing fm's */
   e_fwin_reload_all();
   return 1;
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o, *ob, *of, *otb;
   E_Radio_Group *rg;
   
   otb = e_widget_toolbook_add(evas, 48 * e_scale, 48 * e_scale);

   o = e_widget_list_add(evas, 0, 0);
   of = e_widget_framelist_add(evas, _("View Mode"), 0);
   rg = e_widget_radio_group_new(&(cfdata->view.mode));
   ob = e_widget_radio_add(evas, _("Icons"), 0, rg);
   e_widget_disabled_set(ob, 1);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Grid Icons"), 1, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Custom Icons"), 2, rg);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Custom Grid Icons"), 3, rg);
   e_widget_disabled_set(ob, 1);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("Custom Smart Grid Icons"), 4, rg);
   e_widget_disabled_set(ob, 1);
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_radio_add(evas, _("List"), 5, rg);
   e_widget_framelist_object_append(of, ob);
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   
   ob = e_widget_label_add(evas, _("Icon Size"));
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f"), 16.0, 256.0, 1.0, 0, 
			    NULL, &(cfdata->icon.icon.w), 150);
   e_widget_list_object_append(o, ob, 1, 1, 0.5);

   e_widget_toolbook_page_append(otb, NULL, _("View"), o, 0, 0, 0, 0, 0.5, 0.0);

   o = e_widget_list_add(evas, 1, 0);
   ob = e_widget_check_add(evas, _("Open Dirs In Place"), 
			   &(cfdata->view.open_dirs_in_place));
   e_widget_list_object_append(o, ob, 1, 1, 0.5);

   ob = e_widget_check_add(evas, _("Sort Dirs First"), 
			   &(cfdata->list.sort.dirs.first));
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   ob = e_widget_check_add(evas, _("Case Sensitive"), 
			   &(cfdata->list.sort.case_sen));
   e_widget_list_object_append(o, ob, 1, 1, 0.5);

   ob = e_widget_check_add(evas, _("Use Single Click"), 
			   &(cfdata->view.single_click));
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   ob = e_widget_check_add(evas, _("Use Alternate Selection Modifiers"), 
			   &(cfdata->selection.windows_modifiers));
   e_widget_list_object_append(o, ob, 1, 1, 0.5);

   ob = e_widget_check_add(evas, _("Show Icon Extension"), 
			   &(cfdata->icon.extension.show));
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   ob = e_widget_check_add(evas, _("Show Full Path"), 
			   &(cfdata->view.show_full_path));
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   ob = e_widget_check_add(evas, _("Show Desktop Icons"), 
			   &(cfdata->view.show_desktop_icons));
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   ob = e_widget_check_add(evas, _("Show Toolbar"), 
			   &(cfdata->view.show_toolbar));
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   ob = e_widget_check_add(evas, _("Show HAL icons on desktop"), 
			   &(cfdata->hal.desktop));
   e_widget_list_object_append(o, ob, 1, 1, 0.5);

   e_widget_toolbook_page_append(otb, NULL, _("Behavior"), o, 0, 0, 0, 0, 0.5, 0.0);

   e_widget_toolbook_page_show(otb, 0);
   return otb;
}
