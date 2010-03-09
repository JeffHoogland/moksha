#include "e.h"

/* local structures */
struct _E_Config_Dialog_Data 
{
   int show_favs, show_apps;
   int show_name, show_generic, show_comment;
   double scroll_speed, fast_mouse_move_threshhold;
   double click_drag_timeout;
   int autoscroll_margin, autoscroll_cursor_margin;
};

/* local function prototypes */
static void *_create_data(E_Config_Dialog *cfd);
static void _fill_data(E_Config_Dialog_Data *cfdata);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object *_adv_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static int _adv_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int _adv_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);

E_Config_Dialog *
e_int_config_menus(E_Container *con, const char *params __UNUSED__) 
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "menus/menu_settings")) return NULL;

   v = E_NEW(E_Config_Dialog_View, 1);
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.create_widgets = _basic_create;
   v->basic.apply_cfdata = _basic_apply;
   v->basic.check_changed = _basic_check_changed;
   v->advanced.create_widgets = _adv_create;
   v->advanced.apply_cfdata = _adv_apply;
   v->advanced.check_changed = _adv_check_changed;

   cfd = e_config_dialog_new(con, _("Menu Settings"), "E", "menus/menu_settings", 
                             "preferences-menus", 0, v, NULL);
   return cfd;
}

/* local functions */
static void *
_create_data(E_Config_Dialog *cfd __UNUSED__)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = E_NEW(E_Config_Dialog_Data, 1);
   _fill_data(cfdata);
   return cfdata;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata __UNUSED__)
{
   cfdata->show_favs = e_config->menu_favorites_show;
   cfdata->show_apps = e_config->menu_apps_show;
   cfdata->show_name = e_config->menu_eap_name_show;
   cfdata->show_generic = e_config->menu_eap_generic_show;
   cfdata->show_comment = e_config->menu_eap_comment_show;
   cfdata->scroll_speed = e_config->menus_scroll_speed;
   cfdata->fast_mouse_move_threshhold = 
     e_config->menus_fast_mouse_move_threshhold;
   cfdata->click_drag_timeout = e_config->menus_click_drag_timeout;
   cfdata->autoscroll_margin = e_config->menu_autoscroll_margin;
   cfdata->autoscroll_cursor_margin = e_config->menu_autoscroll_cursor_margin;
}

static void
_free_data(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   E_FREE(cfdata);
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *of, *ow;

   o = e_widget_list_add(evas, 0, 0);

   of = e_widget_framelist_add(evas, _("Main Menu"), 0);
   ow = e_widget_check_add(evas, _("Favorites"), &(cfdata->show_favs));
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_check_add(evas, _("Applications"), &(cfdata->show_apps));
   e_widget_framelist_object_append(of, ow);
   e_widget_list_object_append(o, of, 1, 0, 0.5);

   of = e_widget_framelist_add(evas, _("Display"), 0);
   ow = e_widget_check_add(evas, _("Name"), &(cfdata->show_name));
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_check_add(evas, _("Generic"), &(cfdata->show_generic));
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_check_add(evas, _("Comments"), &(cfdata->show_comment));
   e_widget_framelist_object_append(of, ow);
   e_widget_list_object_append(o, of, 1, 0, 0.5);

   return o;
}

static int
_basic_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   e_config->menu_favorites_show = cfdata->show_favs;
   e_config->menu_apps_show = cfdata->show_apps;
   e_config->menu_eap_name_show = cfdata->show_name;
   e_config->menu_eap_generic_show = cfdata->show_generic;
   e_config->menu_eap_comment_show = cfdata->show_comment;
   e_config_save_queue();
   return 1;
}

static int
_basic_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   return ((e_config->menu_favorites_show != cfdata->show_favs) ||
	   (e_config->menu_apps_show != cfdata->show_apps) ||
	   (e_config->menu_eap_name_show != cfdata->show_name) ||
	   (e_config->menu_eap_generic_show != cfdata->show_generic) ||
	   (e_config->menu_eap_comment_show != cfdata->show_comment));
}

static Evas_Object *
_adv_create(E_Config_Dialog *cfd __UNUSED__, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *otb, *ol, *ow;

   otb = e_widget_toolbook_add(evas, (48 * e_scale), (48 * e_scale));

   ol = e_widget_list_add(evas, 0, 0);
   ow = e_widget_check_add(evas, _("Favorites"), &(cfdata->show_favs));
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_check_add(evas, _("Applications"), &(cfdata->show_apps));
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Main Menu"), ol, 1, 0, 1, 0, 
                                 0.5, 0.0);

   ol = e_widget_list_add(evas, 0, 0);
   ow = e_widget_check_add(evas, _("Name"), &(cfdata->show_name));
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_check_add(evas, _("Generic"), &(cfdata->show_generic));
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_check_add(evas, _("Comments"), &(cfdata->show_comment));
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Display"), ol, 1, 0, 1, 0, 
                                 0.5, 0.0);

   ol = e_widget_list_add(evas, 0, 0);
   ow = e_widget_label_add(evas, _("Margin"));
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_slider_add(evas, 1, 0, _("%2.0f pixels"), 0, 50, 1, 0, NULL, 
                            &(cfdata->autoscroll_margin), 100);
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_label_add(evas, _("Cursor Margin"));
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_slider_add(evas, 1, 0, _("%2.0f pixels"), 0, 50, 1, 0, NULL, 
                            &(cfdata->autoscroll_cursor_margin), 100);
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Autoscroll"), ol, 1, 0, 1, 0, 
                                 0.5, 0.0);

   ol = e_widget_list_add(evas, 0, 0);
   ow = e_widget_label_add(evas, _("Menu Scroll Speed"));
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_slider_add(evas, 1, 0, _("%5.0f pixels/sec"), 0, 20000, 100, 
                            0, &(cfdata->scroll_speed), NULL, 100);
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_label_add(evas, _("Fast Mouse Move Threshhold"));
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_slider_add(evas, 1, 0, _("%4.0f pixels/sec"), 0, 2000, 10, 
                            0, &(cfdata->fast_mouse_move_threshhold), NULL, 100);
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_label_add(evas, _("Click Drag Timeout"));
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   ow = e_widget_slider_add(evas, 1, 0, _("%2.2f sec"), 0, 10, 0.25, 
                            0, &(cfdata->click_drag_timeout), NULL, 100);
   e_widget_list_object_append(ol, ow, 1, 0, 0.5);
   e_widget_toolbook_page_append(otb, NULL, _("Miscellaneous"), ol, 1, 0, 1, 0, 
                                 0.5, 0.0);

   e_widget_toolbook_page_show(otb, 0);
   return otb;
}

static int
_adv_apply(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   e_config->menu_favorites_show = cfdata->show_favs;
   e_config->menu_apps_show = cfdata->show_apps;
   e_config->menu_eap_name_show = cfdata->show_name;
   e_config->menu_eap_generic_show = cfdata->show_generic;
   e_config->menu_eap_comment_show = cfdata->show_comment;

   if (cfdata->scroll_speed == 0) e_config->menus_scroll_speed = 1.0;
   else e_config->menus_scroll_speed = cfdata->scroll_speed;

   if (cfdata->fast_mouse_move_threshhold == 0)
     e_config->menus_fast_mouse_move_threshhold = 1.0;
   else 
     {
        e_config->menus_fast_mouse_move_threshhold = 
          cfdata->fast_mouse_move_threshhold;
     }
   e_config->menus_click_drag_timeout = cfdata->click_drag_timeout;
   e_config->menu_autoscroll_margin = cfdata->autoscroll_margin;
   e_config->menu_autoscroll_cursor_margin = cfdata->autoscroll_cursor_margin;
   e_config_save_queue();
   return 1;
}

static int
_adv_check_changed(E_Config_Dialog *cfd __UNUSED__, E_Config_Dialog_Data *cfdata)
{
   double scroll_speed, move_threshold;

   if (cfdata->scroll_speed == 0)
     scroll_speed = 1.0;
   else
     scroll_speed = cfdata->scroll_speed;

   if (cfdata->fast_mouse_move_threshhold == 0)
     move_threshold = 1.0;
   else
     move_threshold = cfdata->fast_mouse_move_threshhold;

   return ((e_config->menu_favorites_show != cfdata->show_favs) ||
	   (e_config->menu_apps_show != cfdata->show_apps) ||
	   (e_config->menu_eap_name_show != cfdata->show_name) ||
	   (e_config->menu_eap_generic_show != cfdata->show_generic) ||
	   (e_config->menu_eap_comment_show != cfdata->show_comment) ||
	   (e_config->menus_click_drag_timeout != cfdata->click_drag_timeout) ||
	   (e_config->menu_autoscroll_margin != cfdata->autoscroll_margin) ||
	   (e_config->menu_autoscroll_cursor_margin != cfdata->autoscroll_cursor_margin) ||
	   (e_config->menus_scroll_speed != scroll_speed) ||
	   (e_config->menus_fast_mouse_move_threshhold != move_threshold));
}
