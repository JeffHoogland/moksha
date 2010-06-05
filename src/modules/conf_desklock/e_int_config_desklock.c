/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */
#include "e.h"

static void *_create_data (E_Config_Dialog *cfd);
static void  _free_data   (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static void  _fill_data   (E_Config_Dialog_Data *cfdata);
static int   _basic_apply (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int   _basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int   _adv_apply   (E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int   _adv_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);

static Evas_Object *_basic_create (E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);
static Evas_Object *_adv_create   (E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata);

static void _cb_method_change (void *data, Evas_Object *obj, void *event_info);
static void _cb_radio_change  (void *data, Evas_Object *obj);
static void _cb_login_change  (void *data, Evas_Object *obj);
static void _cb_button_up     (void *data1, void *data2);
static void _cb_fm_dir_change (void *data, Evas_Object *obj, void *event_info);
static void _cb_fm_sel_change (void *data, Evas_Object *obj, void *event_info);
static void _cb_fm_change     (void *data, Evas_Object *obj, void *event_info);

static int _zone_count_get(void);

static void _cb_disable_check(void *data, Evas_Object *obj);

static void _cb_ask_presentation_changed(void *data, Evas_Object *obj);

struct _E_Config_Dialog_Data
{
   E_Config_Dialog *cfd;

   /* Common vars */
   int use_xscreensaver;
   int fmdir;
   int zone_count;

   /* Locking */
   int start_locked;
   int lock_on_suspend;
   int auto_lock;
   int locking_method;
   int login_zone;
   int zone;
   char *custom_lock_cmd;
   
   /* Timers */
   int screensaver_lock;
   double idle_time;
   double post_screensaver_time;

   /* Adv props */
   int bg_method;
   const char *bg;
   int custom_lock;
   int ask_presentation;
   double ask_presentation_timeout;

   struct
   {
      Evas_Object *loginbox_slider;
      Evas_Object *post_screensaver_slider;
      Evas_Object *auto_lock_slider;
      Evas_Object *ask_presentation_slider;
      Evas_Object *o_fm, *o_sf, *o_btn, *o_custom;
   } gui;
};


E_Config_Dialog *
e_int_config_desklock(E_Container *con, const char *params __UNUSED__)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;

   if (e_config_dialog_find("E", "screen/screen_lock")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);
   if (!v) return NULL;
   v->create_cfdata = _create_data;
   v->free_cfdata =   _free_data;

   v->basic.create_widgets = _basic_create;
   v->basic.apply_cfdata = _basic_apply;
   v->basic.check_changed = _basic_check_changed;
   v->override_auto_apply = 1;

   cfd = e_config_dialog_new(con, _("Screen Lock Settings"), "E",
			     "screen/screen_lock", "preferences-desklock",
			     0, v, NULL);
   return cfd;
}

static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   cfdata->fmdir = 0;
   if (e_config->desklock_background)
     {
	cfdata->bg = eina_stringshare_ref(e_config->desklock_background);
	if (!strstr(cfdata->bg, e_user_homedir_get()))
	  cfdata->fmdir = 1;
     }
   else
     cfdata->bg = eina_stringshare_add("theme_desklock_background");

   if (!strcmp(cfdata->bg, "theme_desklock_background"))
     cfdata->bg_method = 0;
   else if (!strcmp(cfdata->bg, "theme_background"))
     cfdata->bg_method = 1;
   else if (!strcmp(cfdata->bg, "user_background"))
     cfdata->bg_method = 2;
   else
     cfdata->bg_method = 3;

   cfdata->use_xscreensaver = ecore_x_screensaver_event_available_get();
   cfdata->zone_count = _zone_count_get();

   cfdata->custom_lock = e_config->desklock_use_custom_desklock;
   if (e_config->desklock_custom_desklock_cmd)
     cfdata->custom_lock_cmd = strdup(e_config->desklock_custom_desklock_cmd);

   cfdata->start_locked = e_config->desklock_start_locked;
   cfdata->lock_on_suspend = e_config->desklock_on_suspend;
   cfdata->auto_lock = e_config->desklock_autolock_idle;
   cfdata->screensaver_lock = e_config->desklock_autolock_screensaver;
   cfdata->idle_time = e_config->desklock_autolock_idle_timeout / 60;
   cfdata->post_screensaver_time = e_config->desklock_post_screensaver_time;
   if (e_config->desklock_login_box_zone >= 0)
     {
	cfdata->login_zone = 0;
	cfdata->zone = e_config->desklock_login_box_zone;
     }
   else
     {
	cfdata->login_zone = e_config->desklock_login_box_zone;
	cfdata->zone = 0;
     }

   cfdata->ask_presentation = e_config->desklock_ask_presentation;
   cfdata->ask_presentation_timeout = e_config->desklock_ask_presentation_timeout;
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
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   E_FREE(cfdata->custom_lock_cmd);
   eina_stringshare_del(cfdata->bg);
   E_FREE(cfdata);
}

static void
_basic_auto_lock_cb_changed(void *data, Evas_Object *o)
{
   E_Config_Dialog_Data *cfdata = data;
   int disable;

   disable = ((!cfdata->use_xscreensaver) ||
	      (!cfdata->auto_lock));
   e_widget_disabled_set(cfdata->gui.auto_lock_slider, disable);
}

static void
_basic_screensaver_lock_cb_changed(void *data, Evas_Object *o)
{
   E_Config_Dialog_Data *cfdata = data;
   int disable;

   disable = ((!cfdata->use_xscreensaver) ||
	      (!cfdata->screensaver_lock));
   e_widget_disabled_set(cfdata->gui.post_screensaver_slider, disable);
}

static Evas_Object *
_basic_create(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *otb, *ol, *of, *ow, *rt;
   E_Radio_Group *rg;
   E_Fm2_Config fmc;
   E_Zone *zone;
   int screen_count;
   char path[PATH_MAX];
   
   zone = e_zone_current_get(cfd->con);
   screen_count = ecore_x_xinerama_screen_count_get();
   
   o = e_widget_list_add(evas, 0, 0);
   otb = e_widget_toolbook_add(evas, (48 * e_scale), (48 * e_scale));
   
   ol = e_widget_list_add(evas, 0, 0);
   ow = e_widget_check_add(evas, _("Lock when Enlightenment starts"),
			   &cfdata->start_locked);
   e_widget_disabled_set(ow, !cfdata->use_xscreensaver);
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);
   
   of = e_widget_framelist_add(evas, _("Custom Screenlock Command"), 0);
   ow = e_widget_entry_add(evas, &(cfdata->custom_lock_cmd), NULL, NULL, NULL);
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_check_add(evas, _("Lock on Suspend"),
			   &cfdata->lock_on_suspend);
   e_widget_disabled_set(ow, !cfdata->use_xscreensaver);
   e_widget_list_object_append(ol, ow, 1, 1, 0.5);
   
   of = e_widget_framelist_add(evas, _("Custom Screenlock Command"), 0);
   ow = e_widget_entry_add(evas, &(cfdata->custom_lock_cmd), NULL, NULL, NULL);
   e_widget_framelist_object_append(of, ow);
   e_widget_list_object_append(ol, of, 1, 1, 0.5);
   
   of = e_widget_framelist_add(evas, _("Login Box Settings"), 0);
   e_widget_disabled_set(of, (screen_count <= 0));
   rg = e_widget_radio_group_new(&(cfdata->login_zone));
   ow = e_widget_radio_add(evas, _("Show on all screen zones"), -1, rg);
   e_widget_on_change_hook_set(ow, _cb_login_change, cfdata);
   e_widget_disabled_set(ow, (screen_count <= 0));
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_radio_add(evas, _("Show on current screen zone"), -2, rg);
   e_widget_on_change_hook_set(ow, _cb_login_change, cfdata);
   e_widget_disabled_set(ow, (screen_count <= 0));
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_radio_add(evas, _("Show on screen zone #:"), 0, rg);
   e_widget_on_change_hook_set(ow, _cb_login_change, cfdata);
   e_widget_disabled_set(ow, (screen_count <= 0));
   e_widget_framelist_object_append(of, ow);
   cfdata->gui.loginbox_slider =
     e_widget_slider_add(evas, 1, 0, _("%1.0f"), 0.0, (cfdata->zone_count - 1),
                         1.0, 0, NULL, &(cfdata->zone), 100);
   e_widget_disabled_set(cfdata->gui.loginbox_slider, (screen_count <= 0));
   e_widget_framelist_object_append(of, cfdata->gui.loginbox_slider);
   e_widget_list_object_append(ol, of, 1, 1, 0.5);
   
   e_widget_toolbook_page_append(otb, NULL, _("Locking"), ol,
                                 1, 0, 1, 0, 0.5, 0.0);
   
   ol = e_widget_list_add(evas, 0, 0);
   
   of = e_widget_framelist_add(evas, _("Screen Lock Timers"), 0);
   ow = e_widget_check_add(evas, _("Lock after X screensaver activates"),
			   &cfdata->screensaver_lock);
   e_widget_on_change_hook_set(ow, _basic_screensaver_lock_cb_changed, cfdata);
   e_widget_disabled_set(ow, !cfdata->use_xscreensaver);
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_slider_add(evas, 1, 0, _("%1.0f seconds"), 0.0, 300.0, 10.0, 0,
			    &(cfdata->post_screensaver_time), NULL, 100);
   cfdata->gui.post_screensaver_slider = ow;
   e_widget_disabled_set(ow, !cfdata->use_xscreensaver);
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_check_add(evas, _("Lock when idle time exceeded"),
			   &cfdata->auto_lock);
   e_widget_on_change_hook_set(ow, _basic_auto_lock_cb_changed, cfdata);
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_slider_add(evas, 1, 0, _("%1.0f minutes"), 1.0, 90.0, 1.0, 0,
			    &(cfdata->idle_time), NULL, 100);
   cfdata->gui.auto_lock_slider = ow;
   e_widget_disabled_set(ow, !cfdata->use_xscreensaver);
   e_widget_framelist_object_append(of, ow);

   e_widget_list_object_append(ol, of, 1, 1, 0.5);
   
   of = e_widget_framelist_add(evas, _("Presentation Mode"), 0);
   ow =
     e_widget_check_add(evas, _("Suggest if deactivated before"),
                                                &(cfdata->ask_presentation));
   e_widget_on_change_hook_set(ow, _cb_ask_presentation_changed, cfdata);
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_slider_add(evas, 1, 0, _("%1.0f seconds"),
                            1.0, 300.0, 10.0, 0,
                            &(cfdata->ask_presentation_timeout), NULL, 100);
   cfdata->gui.ask_presentation_slider = ow;
   e_widget_framelist_object_append(of, ow);
   
   e_widget_list_object_append(ol, of, 1, 1, 0.5);

   e_widget_toolbook_page_append(otb, NULL, _("Timers"), ol,
                                 1, 0, 1, 0, 0.5, 0.0);
   
   ol = e_widget_list_add(evas, 0, 0);
   
   of = e_widget_framelist_add(evas, _("Wallpaper Mode"), 0);
   rg = e_widget_radio_group_new(&(cfdata->bg_method));
   ow = e_widget_radio_add(evas, _("Theme Defined"), 0, rg);
   evas_object_smart_callback_add(ow, "changed", _cb_method_change, cfdata);
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_radio_add(evas, _("Theme Wallpaper"), 1, rg);
   evas_object_smart_callback_add(ow, "changed", _cb_method_change, cfdata);
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_radio_add(evas, _("User Wallpaper"), 2, rg);
   evas_object_smart_callback_add(ow, "changed", _cb_method_change, cfdata);
   e_widget_framelist_object_append(of, ow);
   ow = e_widget_radio_add(evas, _("Custom"), 3, rg);
   evas_object_smart_callback_add(ow, "changed", _cb_method_change, cfdata);
   cfdata->gui.o_custom = ow;
   e_widget_framelist_object_append(of, ow);
   
   e_widget_list_object_append(ol, of, 1, 1, 0.5);
   
   of = e_widget_framelist_add(evas, _("Custom Wallpaper"), 0);
   
   rt = e_widget_table_add(evas, 0);
   rg = e_widget_radio_group_new(&(cfdata->fmdir));
   ow = e_widget_radio_add(evas, _("Personal"), 0, rg);
   e_widget_on_change_hook_set(ow, _cb_radio_change, cfdata);
   e_widget_table_object_append(rt, ow, 0, 0, 1, 1, 1, 1, 0, 0);
   ow = e_widget_radio_add(evas, _("System"), 1, rg);
   e_widget_on_change_hook_set(ow, _cb_radio_change, cfdata);
   e_widget_table_object_append(rt, ow, 1, 0, 1, 1, 1, 1, 0, 0);
   cfdata->gui.o_btn = e_widget_button_add(evas, _("Directory up"),
				       "go-up", _cb_button_up,
				       cfdata, NULL);
   e_widget_table_object_append(rt, cfdata->gui.o_btn, 2, 0, 1, 1, 1, 1, 1, 0);
   e_widget_framelist_object_append(of, rt);

   if (cfdata->fmdir == 1)
     e_prefix_data_concat_static(path, "data/backgrounds");
   else
     e_user_dir_concat_static(path, "backgrounds");

   ow = e_fm2_add(evas);
   cfdata->gui.o_fm = ow;
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
   fmc.icon.key_hint = NULL;
   fmc.list.sort.no_case = 1;
   fmc.list.sort.dirs.first = 0;
   fmc.list.sort.dirs.last = 1;
   fmc.selection.single = 1;
   fmc.selection.windows_modifiers = 0;
   e_fm2_config_set(ow, &fmc);
   e_fm2_icon_menu_flags_set(ow, E_FM2_MENU_NO_SHOW_HIDDEN);

   e_fm2_path_set(ow, path, "/");
   evas_object_smart_callback_add(ow, "dir_changed",
				  _cb_fm_dir_change, cfdata);
   evas_object_smart_callback_add(ow, "selection_change",
				  _cb_fm_sel_change, cfdata);
   evas_object_smart_callback_add(ow, "changed", _cb_fm_change, cfdata);

   cfdata->gui.o_sf = e_widget_scrollframe_pan_add(evas, ow, e_fm2_pan_set,
                                                   e_fm2_pan_get,
                                                   e_fm2_pan_max_get,
                                                   e_fm2_pan_child_size_get);
   e_widget_size_min_set(cfdata->gui.o_sf, 100, 100);
   e_widget_framelist_object_append(of, cfdata->gui.o_sf);
   e_widget_list_object_append(ol, of, 1, 1, 0.5);
   
   e_widget_toolbook_page_append(otb, NULL, _("Wallpaper"), ol,
                                 1, 0, 1, 0, 0.5, 0.0);
   
   e_widget_list_object_append(o, otb, 1, 1, 0.5);
   e_widget_toolbook_page_show(otb, 0);

   _basic_auto_lock_cb_changed(cfdata, NULL);
   _basic_screensaver_lock_cb_changed(cfdata, NULL);

   of = e_widget_framelist_add(evas, _("Enter Presentation Mode"), 0);
   
   return o;
}

static int
_basic_apply(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   e_config->desklock_start_locked = cfdata->start_locked;
   e_config->desklock_on_suspend = cfdata->lock_on_suspend;
   e_config->desklock_autolock_idle = cfdata->auto_lock;
   e_config->desklock_post_screensaver_time = cfdata->post_screensaver_time;
   e_config->desklock_autolock_screensaver = cfdata->screensaver_lock;
   e_config->desklock_autolock_idle_timeout = cfdata->idle_time * 60;
   e_config->desklock_ask_presentation = cfdata->ask_presentation;
   e_config->desklock_ask_presentation_timeout = cfdata->ask_presentation_timeout;

   if (cfdata->bg)
     {
	if (e_config->desklock_background)
	  e_filereg_deregister(e_config->desklock_background);
	eina_stringshare_replace(&e_config->desklock_background, cfdata->bg);
	e_filereg_register(e_config->desklock_background);
     }

   if (cfdata->login_zone < 0)
     {
	e_config->desklock_login_box_zone = cfdata->login_zone;
     }
   else
     e_config->desklock_login_box_zone = cfdata->zone;

   e_config->desklock_use_custom_desklock = cfdata->custom_lock;
   if (cfdata->custom_lock_cmd)
     {
	if (e_config->desklock_custom_desklock_cmd)
	  eina_stringshare_del(e_config->desklock_custom_desklock_cmd);
	e_config->desklock_custom_desklock_cmd =
	  eina_stringshare_add(cfdata->custom_lock_cmd);
     }

   e_config_save_queue();
   return 1;
}

static int
_basic_check_changed(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   if (e_config->desklock_start_locked != cfdata->start_locked)
     return 1;

   if (e_config->desklock_autolock_idle != cfdata->auto_lock)
     return 1;

   if (e_config->desklock_autolock_screensaver != cfdata->screensaver_lock)
     return 1;

   if (e_config->desklock_post_screensaver_time !=
       cfdata->post_screensaver_time)
     return 1;

   if (e_config->desklock_autolock_idle_timeout != cfdata->idle_time * 60)
     return 1;

   if (e_config->desklock_background != cfdata->bg)
     return 1;

   if (cfdata->login_zone < 0)
     {
	if (e_config->desklock_login_box_zone != cfdata->login_zone)
	  return 1;
     }
   else
     {
	if (e_config->desklock_login_box_zone != cfdata->zone)
	  return 1;
     }

   if (e_config->desklock_use_custom_desklock != cfdata->custom_lock)
     return 1;

   if (e_config->desklock_custom_desklock_cmd && cfdata->custom_lock_cmd)
     {
	if (strcmp(e_config->desklock_custom_desklock_cmd, cfdata->custom_lock_cmd) != 0)
	  return 1;
     }
   else if (e_config->desklock_custom_desklock_cmd != cfdata->custom_lock_cmd)
     return 1;

   return ((e_config->desklock_ask_presentation != cfdata->ask_presentation) ||
	   (e_config->desklock_ask_presentation_timeout != cfdata->ask_presentation_timeout));
}

static void
_cb_method_change(void *data, Evas_Object * obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;
   Eina_List *sel;
   E_Fm2_Icon_Info *ic;
   char path[PATH_MAX];
   const char *f;

   cfdata = data;
   switch(cfdata->bg_method)
     {
     case 0:
	eina_stringshare_replace(&cfdata->bg, "theme_desklock_background");
	break;
     case 1:
	eina_stringshare_replace(&cfdata->bg, "theme_background");
	break;
     case 2:
	eina_stringshare_replace(&cfdata->bg, "user_background");
	break;
     default:
	sel = e_fm2_selected_list_get(cfdata->gui.o_fm);
	if (!sel) sel = e_fm2_all_list_get(cfdata->gui.o_fm);
	if (!sel) return;
	ic = eina_list_nth(sel, 0);
	eina_list_free(sel);
	if (!ic)
	   return;
	e_fm2_select_set(cfdata->gui.o_fm, ic->file, 1);
	if (cfdata->fmdir == 0)
	   e_user_dir_snprintf(path, sizeof(path), "backgrounds/%s", ic->file);
	else
	   e_prefix_data_snprintf(path, sizeof(path), "data/backgrounds/%s",
				  ic->file);
	if (ecore_file_is_dir(path))
	   return;
	eina_stringshare_replace(&cfdata->bg, path);
	break;
     }
}

static void
_cb_radio_change(void *data, Evas_Object * obj)
{
   E_Config_Dialog_Data *cfdata;
   char path[4096];

   cfdata = data;
   if (!cfdata->gui.o_fm) return;
   if (cfdata->fmdir == 0)
     e_user_dir_concat_static(path, "backgrounds");
   else
     e_prefix_data_concat_static(path, "data/backgrounds");
   e_fm2_path_set(cfdata->gui.o_fm, path, "/");
}

static void
_cb_login_change(void *data, Evas_Object *obj)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;
   if (cfdata->login_zone < 0)
     e_widget_disabled_set(cfdata->gui.loginbox_slider, 1);
   else
     e_widget_disabled_set(cfdata->gui.loginbox_slider, 0);
}

static void
_cb_button_up(void *data1, void *data2)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data1;
   if (!cfdata->gui.o_fm) return;
   e_fm2_parent_go(cfdata->gui.o_fm);
   e_widget_scrollframe_child_pos_set(cfdata->gui.o_sf, 0, 0);
}

static void
_cb_fm_dir_change(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;
   if (!cfdata->gui.o_fm) return;
   if (!e_fm2_has_parent_get(cfdata->gui.o_fm))
      e_widget_disabled_set(cfdata->gui.o_btn, 1);
   else
      e_widget_disabled_set(cfdata->gui.o_btn, 0);
   e_widget_scrollframe_child_pos_set(cfdata->gui.o_sf, 0, 0);
}

static void
_cb_fm_sel_change(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;
   Eina_List *sel;
   E_Fm2_Icon_Info *ic;
   char path[PATH_MAX];

   cfdata = data;
   if (!cfdata->gui.o_fm) return;
   sel = e_fm2_selected_list_get(cfdata->gui.o_fm);
   if (!sel) return;
   ic = sel->data;
   eina_list_free(sel);

   if (cfdata->fmdir == 0)
     {
	e_user_dir_snprintf(path, sizeof(path), "backgrounds/%s",
			    ic->file);
     }
   else
     {
	e_prefix_data_snprintf(path, sizeof(path), "data/backgrounds/%s",
			       ic->file);
     }
   if (ecore_file_is_dir(path)) return;
   eina_stringshare_replace(&cfdata->bg, path);
   e_widget_change(cfdata->gui.o_sf);
   e_widget_radio_toggle_set(cfdata->gui.o_custom, 1);
}

static void
_cb_fm_change(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;
   const char *p;
   char path[PATH_MAX];
   size_t len;

   cfdata = data;
   if (!cfdata->bg) return;
   if (!cfdata->gui.o_fm) return;
   p = e_fm2_real_path_get(cfdata->gui.o_fm);
   if (p)
     {
	if (strncmp(p, cfdata->bg, strlen(p))) return;
     }
   else
     return;

   len = e_user_dir_concat_static(path, "backgrounds");
   if (!strncmp(cfdata->bg, path, len))
     p = cfdata->bg + len + 1;
   else
     {
	len = e_prefix_data_concat_static(path, "data/backgrounds");
	if (!strncmp(cfdata->bg, path, len))
	  p = cfdata->bg + len + 1;
	else
	  p = cfdata->bg;
     }
   e_fm2_select_set(cfdata->gui.o_fm, p, 1);
   e_fm2_file_show(cfdata->gui.o_fm, p);
}

static int
_zone_count_get(void)
{
   int num = 0;
   Eina_List *m, *c;

   for (m = e_manager_list(); m; m = m->next)
     {
	E_Manager *man;

	man = m->data;
	if (!man) continue;
	for (c = man->containers; c; c = c->next)
	  {
	     E_Container *con;

	     con = c->data;
	     if (!con) continue;
	     num += eina_list_count(con->zones);
	  }
     }
   return num;
}

static void
_cb_ask_presentation_changed(void *data, Evas_Object *obj __UNUSED__)
{
   E_Config_Dialog_Data *cfdata = data;
   Eina_Bool disable;

   disable = (!cfdata->ask_presentation);

   e_widget_disabled_set(cfdata->gui.ask_presentation_slider, disable);
}
