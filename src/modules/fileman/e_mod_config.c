#include "e.h"
#include "e_mod_main.h"
#include "e_mod_config.h"

struct _E_Config_Dialog_Data 
{
   /* general view mode */
   struct {
      E_Fm2_View_Mode mode;
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
   } view;
   /* display of icons */
   struct {
      struct {
	 int           w, h;
      } icon;
      struct {
	 int           w, h;
      } list;
      struct {
	 int w;
	 int h;
      } fixed;
      struct {
	 int show;
      } extension;
      const char      *key_hint;
   } icon;
   /* how to sort files */
   struct {
      struct {
	 int    no_case;
	 struct {
	    int first;
	    int last;
	 } dirs;
      } sort;
   } list;
   /* control how you can select files */
   struct {
      int    single;
      int    windows_modifiers;
   } selection;
   /* the background - if any, and how to handle it */
   /* FIXME: not implemented yet */
   struct {
      const char      *background;
      const char      *frame;
      const char      *icons;
      int    fixed;
   } theme;
   
   E_Config_Dialog *cfd;
};

static void *_create_data(E_Config_Dialog *cfd);
static void  _fill_data(E_Config_Dialog_Data *cfdata);
static void  _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int   _basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

void
_config_fileman_module(E_Container *con) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   
   if (e_config_dialog_find("E", "_config_fileman_dialog")) return;
   v = E_NEW(E_Config_Dialog_View, 1);
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply;
   v->basic.create_widgets = _basic_create;
   cfd = e_config_dialog_new(con, _("Fileman Settings"), "E", 
			     "_config_fileman_dialog",
			     "enlightenment/fileman", 0, v, NULL);
}

static void *
_create_data(E_Config_Dialog *cfd) 
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _fill_data(cfdata);
   return cfdata;
}

static void 
_fill_data(E_Config_Dialog_Data *cfdata) 
{
   /* Disable changing view mode for now until they are ready */
   //cfdata->view.mode = fileman_config->view.mode;
   
   cfdata->view.open_dirs_in_place = fileman_config->view.open_dirs_in_place;
   cfdata->view.single_click = fileman_config->view.single_click;
   cfdata->view.show_full_path = fileman_config->view.show_full_path;
   cfdata->view.show_desktop_icons = fileman_config->view.show_desktop_icons;
   cfdata->icon.icon.w = fileman_config->icon.icon.w;
   cfdata->icon.icon.h = fileman_config->icon.icon.h;
   cfdata->icon.extension.show = fileman_config->icon.extension.show;
   cfdata->list.sort.dirs.first = fileman_config->list.sort.dirs.first;
}

static void 
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   E_FREE(cfdata);
}

static int 
_basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata) 
{
   /* Disable changing view mode for now until they are ready */
   //fileman_config->view.mode = cfdata->view.mode;
   
   fileman_config->view.open_dirs_in_place = cfdata->view.open_dirs_in_place;
   fileman_config->view.single_click = cfdata->view.single_click;
   fileman_config->view.show_full_path = cfdata->view.show_full_path;
   fileman_config->view.show_desktop_icons = cfdata->view.show_desktop_icons;
   fileman_config->icon.extension.show = cfdata->icon.extension.show;

   /* Make these two equal so that icons are proportioned correctly */
   fileman_config->icon.icon.w = cfdata->icon.icon.w;
   fileman_config->icon.icon.h = cfdata->icon.icon.w;
   
   fileman_config->list.sort.dirs.first = cfdata->list.sort.dirs.first;
   fileman_config->list.sort.dirs.last = !(cfdata->list.sort.dirs.first);
     
   e_config_save_queue();
   
   /* FIXME: reload/refresh existing fm's */
   e_fwin_reload_all();
   return 1;
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata) 
{
   Evas_Object *o, *ob;
   
   o = e_widget_list_add(evas, 1, 0);

   ob = e_widget_label_add(evas, _("Icon Size"));
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f"), 16.0, 256.0, 1.0, 0, 
			    NULL, &(cfdata->icon.icon.w), 150);
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   
   ob = e_widget_check_add(evas, _("Open Dirs In Place"), 
			   &(cfdata->view.open_dirs_in_place));
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   ob = e_widget_check_add(evas, _("Sort Dirs First"), 
			   &(cfdata->list.sort.dirs.first));
   e_widget_list_object_append(o, ob, 1, 1, 0.5);
   ob = e_widget_check_add(evas, _("Use Single Click"), 
			   &(cfdata->view.single_click));
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
   
   return o;
}
