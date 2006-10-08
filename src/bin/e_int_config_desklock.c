#include "e.h"

#define LOGINBOX_SHOW_ALL_SCREENS	  -1
#define LOGINBOX_SHOW_CURRENT_SCREENS     -2
#define LOGINBOX_SHOW_SPECIFIC_SCREEN	  0

#define BG_LIST_ICON_SIZE_W 32
#define BG_LIST_ICON_SIZE_H 32

#define BG_PREVIEW_W 280
#define BG_PREVIEW_H ((BG_PREVIEW_W * e_zone_current_get(cfd->dia->win->container)->h) / \
		     e_zone_current_get(cfd->dia->win->container)->w)

#define DEF_DESKLOCK_BACKGROUND	"theme_desklock_background"
#define DEF_THEME_BACKGROUND	"theme_background"


static void *_create_data(E_Config_Dialog *cfd);
static void _free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static int  _basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object  *_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas,
					   E_Config_Dialog_Data *cfdata);

static int  _advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata);
static Evas_Object  *_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas,
					      E_Config_Dialog_Data *cfdata);


/******************************************************************************************/

static int  _e_desklock_zone_num_get(void);
static void _load_bgs(E_Config_Dialog_Data *cfdata);
static void _ibg_list_cb_bg_selected(void *data);

static void _e_desklock_cb_show_passwd(void *data, Evas_Object *obj);
static void _e_desklock_cb_lb_show_change(void *data, Evas_Object *obj);
#ifdef HAVE_PAM
static void _e_desklock_cb_auth_method_change(void *data, Evas_Object *obj);
#endif

/*******************************************************************************************/

struct _E_Config_Dialog_Data
{
   E_Config_Dialog *cfd;
   
   Evas_Object *o_frame;
   Evas_Object *o_fm;
   Evas_Object *o_up_button;
   Evas_Object *o_preview;
   Evas_Object *o_personal;
   Evas_Object *o_system;
   Evas_Object *o_bg_mode_theme;
   Evas_Object *o_bg_mode_bg;
   Evas_Object *o_bg_mode_custom;
   int fmdir;
   
   char *desklock_passwd;
   char *desklock_passwd_cp;
   int show_password; // local
   
   int autolock; // in e_config;
   int use_timeout; // in e_config;
   double timeout; // in e_config;
   
   int login_box_zone; // in e_config;
   int specific_lb_zone; // local variable
   int specific_lb_zone_backup; // used to have smart iface
   
   int zone_count; // local variable;

   int bg_mode; // config
   char *bg; // config
   Evas *evas; // local variable
   Evas_Object *preview_image; // local variable

#ifdef HAVE_PAM
   int auth_method;
#endif
   
   struct {
      Evas_Object *passwd_field;
//      Evas_Object *bg_list;

      Evas_Object *show_passwd_check;
      
      struct {
	 Evas_Object *show_all_screens;
	 Evas_Object *show_current_screen;
	 Evas_Object *show_specific_screen;
	 Evas_Object *screen_slider;
      } loginbox_obj;
   } gui;
};

EAPI E_Config_Dialog *
e_int_config_desklock(E_Container *con)
{
   E_Config_Dialog *cfd;
   E_Config_Dialog_View *v;
   
   if (e_config_dialog_find("E", "_config_desklock_dialog")) return NULL;
   v = E_NEW(E_Config_Dialog_View, 1);
   
   v->create_cfdata = _create_data;
   v->free_cfdata = _free_data;
   v->basic.apply_cfdata = _basic_apply_data;
   v->basic.create_widgets = _basic_create_widgets;
   v->advanced.apply_cfdata = _advanced_apply_data;
   v->advanced.create_widgets = _advanced_create_widgets;
   
   v->override_auto_apply = 1;
   
   cfd = e_config_dialog_new(con,
			     _("Desktop Lock Settings"),
			     "E", "_config_desklock_dialog",
			     "enlightenment/desklock", 0, v, NULL);
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
   E_FREE(cfdata->bg);
   cfdata->bg = strdup(buf);
   if (cfdata->o_preview)
     e_widget_preview_edje_set(cfdata->o_preview, buf, "e/desktop/background");
   if (cfdata->o_frame)
     e_widget_change(cfdata->o_frame);
   e_widget_radio_toggle_set(cfdata->o_bg_mode_custom, 1);
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
   const char *p, *homedir;
   char buf[4096];
   
   cfdata = data;
   if (!cfdata->bg) return;
   if (!cfdata->o_fm) return;
   p = e_fm2_real_path_get(cfdata->o_fm);
   if (p)
     {
	if (strncmp(p, cfdata->bg, strlen(p))) return;
     }
   homedir = e_user_homedir_get();
   snprintf(buf, sizeof(buf), "%s/.e/e/backgrounds", homedir);
   if (!p) return;
   if (!strncmp(cfdata->bg, buf, strlen(buf)))
     p = cfdata->bg + strlen(buf) + 1;
   else
     {
	snprintf(buf, sizeof(buf), "%s/data/backgrounds", e_prefix_data_get());
	if (!strncmp(cfdata->bg, buf, strlen(buf)))
	  p = cfdata->bg + strlen(buf) + 1;
	else
	  p = cfdata->bg;
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
	snprintf(path, sizeof(path), "%s/data/backgrounds", e_prefix_data_get());
     }
   else
     {
	homedir = e_user_homedir_get();
	snprintf(path, sizeof(path), "%s/.e/e/backgrounds", homedir);
     }
   e_fm2_path_set(cfdata->o_fm, path, "/");
}

static void
_bg_mode(void *data, Evas_Object *obj, void *event_info)
{
   E_Config_Dialog_Data *cfdata;
   const char *f;
   
   cfdata = data;
   if (cfdata->bg_mode == 0)
     {
        f = e_theme_edje_file_get("base/theme/desklock", "e/desklock/background");
	if (cfdata->o_preview)
	  e_widget_preview_edje_set(cfdata->o_preview, f, "e/desktop/background");
	E_FREE(cfdata->bg);
     }
   else if (cfdata->bg_mode == 1)
     {
        f = e_theme_edje_file_get("base/theme/backgrounds", "e/desktop/background");
	if (cfdata->o_preview)
	  e_widget_preview_edje_set(cfdata->o_preview, f, "e/desktop/background");
	E_FREE(cfdata->bg);
	cfdata->bg = strdup("theme_background");
     }
   else if (cfdata->bg_mode == 2)
     {
	if (cfdata->bg)
	  {
	     if (cfdata->o_preview)
	       e_widget_preview_edje_set(cfdata->o_preview, cfdata->bg, "e/desktop/background");
	  }
     }
}



static void
_fill_data(E_Config_Dialog_Data *cfdata)
{
   char path[4096];
   const char *homedir;

   // we have to read it from e_config->...
   if (e_config->desklock_personal_passwd)
     {
	cfdata->desklock_passwd = strdup(e_config->desklock_personal_passwd);
	cfdata->desklock_passwd_cp = strdup(e_config->desklock_personal_passwd);
     }
   else
     {
	cfdata->desklock_passwd = strdup("");
	cfdata->desklock_passwd_cp = strdup("");
     }
   
   cfdata->autolock = e_config->desklock_autolock;
   cfdata->use_timeout = e_config->desklock_use_timeout;
   cfdata->timeout = e_config->desklock_timeout;
   
   /* should be taken from e_config */
   //cfdata->login_box_on_zone = -1;
   
   if (e_config->desklock_login_box_zone >= 0)
     {
	cfdata->login_box_zone = LOGINBOX_SHOW_SPECIFIC_SCREEN;
	cfdata->specific_lb_zone_backup = cfdata->specific_lb_zone =
	  e_config->desklock_login_box_zone;
     }
   else
     {
	cfdata->login_box_zone = e_config->desklock_login_box_zone;
	cfdata->specific_lb_zone_backup = cfdata->specific_lb_zone = 0;
     }
   
   cfdata->zone_count = _e_desklock_zone_num_get();
   
   cfdata->show_password = 0;
 
   if ((!e_config->desklock_background) ||
       (!strcmp(e_config->desklock_background, "theme_desklock_background")))
     cfdata->bg_mode = 0;
   else if (!strcmp(e_config->desklock_background, "theme_background"))
     cfdata->bg_mode = 1;
   else
     cfdata->bg_mode = 2;
   if (!e_config->desklock_background)
     cfdata->bg = strdup(DEF_DESKLOCK_BACKGROUND);
   else
     cfdata->bg = strdup(e_config->desklock_background);

   if (cfdata->bg[0] != '/')
     {
	homedir = e_user_homedir_get();
	snprintf(path, sizeof(path), "%s/.e/e/backgrounds/%s", homedir, cfdata->bg);
	if (ecore_file_exists(path))
	  {
	     E_FREE(cfdata->bg);
	     cfdata->bg = strdup(path);
	  }
	else
	  {
	     snprintf(path, sizeof(path), "%s/data/backgrounds/%s", e_prefix_data_get(), cfdata->bg);
	     if (ecore_file_exists(path))
	       {
		  E_FREE(cfdata->bg);
		  cfdata->bg = strdup(path);
	       }
	  }
     }
   
#ifdef HAVE_PAM
   cfdata->auth_method = e_config->desklock_auth_method;
#endif
   
   //vertical_lb_align = e_config->desklock_login
}

static void *
_create_data(E_Config_Dialog *cfd)
{
  E_Config_Dialog_Data *cfdata;

  cfdata = E_NEW(E_Config_Dialog_Data, 1);
  cfdata->desklock_passwd = strdup("");
  cfdata->desklock_passwd_cp = strdup("");
  cfdata->cfd = cfd;

  _fill_data(cfdata);

  return cfdata;
}
static void
_free_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
  if (!cfdata) return;

  E_FREE(cfdata->desklock_passwd);
  E_FREE(cfdata->desklock_passwd_cp);
  E_FREE(cfdata->bg);

  free(cfdata);
}

static int
_basic_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   if (cfdata->desklock_passwd_cp)
     {
	if (e_config->desklock_personal_passwd)
	  evas_stringshare_del(e_config->desklock_personal_passwd);
     }
   e_config->desklock_personal_passwd = evas_stringshare_add(cfdata->desklock_passwd_cp);
   e_config->desklock_autolock = cfdata->autolock;
   e_config->desklock_use_timeout = cfdata->use_timeout;
   e_config->desklock_timeout = cfdata->timeout;
#ifdef HAVE_PAM
   e_config->desklock_auth_method = cfdata->auth_method;
#endif
   if (e_config->desklock_use_timeout)
     {
	ecore_x_screensaver_timeout_set(e_config->desklock_timeout);
     }
   e_config_save_queue();
  return 1;
}

static Evas_Object *
_basic_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *o, *of, *ob;
#ifdef HAVE_PAM
   E_Radio_Group  *rg_auth;
   Evas_Object	  *oc;
#endif
   
   o = e_widget_list_add(evas, 0, 0);

/*   
#ifdef HAVE_PAM
   of = e_widget_framelist_add(evas, _("Password Type"), 0);
   
   rg_auth = e_widget_radio_group_new((int*)(&cfdata->auth_method));
   
   oc = e_widget_radio_add(evas, _("Use my login password"), 0, rg_auth);
   e_widget_on_change_hook_set(oc, _e_desklock_cb_auth_method_change, cfdata);
   e_widget_framelist_object_append(of, oc);
   
   oc = e_widget_radio_add(evas, _("Personalized password"), 1, rg_auth);
   e_widget_on_change_hook_set(oc, _e_desklock_cb_auth_method_change, cfdata);
   e_widget_framelist_object_append(of, oc);
   
   e_widget_list_object_append(o, of, 1, 1, 0.5);
#endif
   
   of = e_widget_framelist_add(evas, _("Personalized Password:"), 0);
   
   cfdata->gui.passwd_field = ob = e_widget_entry_add(evas, &(cfdata->desklock_passwd));
#ifdef HAVE_PAM
   if (cfdata->auth_method == 0) e_widget_disabled_set(ob, 1);
#endif
   
   e_widget_entry_password_set(ob, !cfdata->show_password);
   e_widget_min_size_set(ob, 200, 25);
   e_widget_framelist_object_append(of, ob);
   
   ob = e_widget_check_add(evas, _("Show password"), &(cfdata->show_password));
   e_widget_on_change_hook_set(ob, _e_desklock_cb_show_passwd, cfdata);
   cfdata->gui.show_passwd_check = ob;
#ifdef HAVE_PAM
   if (cfdata->auth_method == 0) e_widget_disabled_set(ob, 1);
#endif
   e_widget_framelist_object_append(of, ob);
   
   e_widget_list_object_append(o, of, 1, 1, 0.5);
*/
   
   of = e_widget_framelist_add(evas, _("Automatic Locking"), 0);
   e_widget_disabled_set(of, !ecore_x_screensaver_event_available_get());
   
   ob = e_widget_check_add(evas, _("Enable screensaver"), &(cfdata->use_timeout));
   e_widget_disabled_set(ob, !ecore_x_screensaver_event_available_get());
   e_widget_framelist_object_append(of, ob);
   
   ob = e_widget_check_add(evas, _("Lock when the screensaver starts"), &(cfdata->autolock));
   e_widget_disabled_set(ob, !ecore_x_screensaver_event_available_get());
   e_widget_framelist_object_append(of, ob);

   ob = e_widget_label_add(evas, _("Time until screensaver starts"));
   e_widget_disabled_set(ob, !ecore_x_screensaver_event_available_get());
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f seconds"),
			    1.0, 600.0,
			    1.0, 0, &(cfdata->timeout), NULL,
			    200);
   e_widget_framelist_object_append(of, ob);
   
   e_widget_list_object_append(o, of, 1, 1, 0.5);
   
   e_dialog_resizable_set(cfd->dia, 0);
   
   return o;
}

/* advanced window */

static int
_advanced_apply_data(E_Config_Dialog *cfd, E_Config_Dialog_Data *cfdata)
{
   if (!cfdata) return 0;
   
   if (cfdata->desklock_passwd_cp)
     {
	if (e_config->desklock_personal_passwd)
	  evas_stringshare_del(e_config->desklock_personal_passwd);
     }
   e_config->desklock_personal_passwd = evas_stringshare_add(cfdata->desklock_passwd_cp);

   if (cfdata->bg)
     {
	if (e_config->desklock_background)
	  evas_stringshare_del(e_config->desklock_background);
	e_config->desklock_background = evas_stringshare_add(cfdata->bg);
     }
   
   if (_e_desklock_zone_num_get() > 1)
     {
	if (cfdata->login_box_zone >= 0)
	  e_config->desklock_login_box_zone = cfdata->specific_lb_zone;
	else
	  e_config->desklock_login_box_zone = cfdata->login_box_zone;
     }
   else
     e_config->desklock_login_box_zone = LOGINBOX_SHOW_ALL_SCREENS;

   e_config->desklock_autolock = cfdata->autolock;
   e_config->desklock_use_timeout = cfdata->use_timeout;
   e_config->desklock_timeout = cfdata->timeout;
#ifdef HAVE_PAM
   e_config->desklock_auth_method = cfdata->auth_method;
#endif

   if (e_config->desklock_use_timeout)
     ecore_x_screensaver_timeout_set(e_config->desklock_timeout);
   
   e_config_save_queue();
   return 1;
}

static Evas_Object *
_advanced_create_widgets(E_Config_Dialog *cfd, Evas *evas, E_Config_Dialog_Data *cfdata)
{
   Evas_Object *ob, *oc;
   Evas_Object *o, *ot, *of, *il, *ol;
   char path[4096];
   const char *homedir;
   const char *f;
   E_Fm2_Config fmc;
   E_Zone *z;
   E_Radio_Group *rg;
#ifdef HAVE_PAM
   E_Radio_Group *rg_auth;
#endif

   homedir = e_user_homedir_get();

   z = e_zone_current_get(cfd->con);
   
   cfdata->evas = evas;
   
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
     snprintf(path, sizeof(path), "%s/data/backgrounds", e_prefix_data_get());
   else
     snprintf(path, sizeof(path), "%s/.e/e/backgrounds", homedir);
   
   o = e_fm2_add(evas);
   cfdata->o_fm = o;
   memset(&fmc, 0, sizeof(E_Fm2_Config));
   fmc.view.mode = E_FM2_VIEW_MODE_LIST;
   fmc.view.open_dirs_in_place = 1;
   fmc.view.selector = 1;
   fmc.view.single_click = 0;
   fmc.view.no_subdir_jump = 0;
   fmc.view.extra_file_source = NULL;
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
   e_widget_min_size_set(of, 100, 100);
   e_widget_table_object_append(ol, of, 0, 2, 1, 1, 1, 1, 1, 1);
   e_widget_table_object_append(ot, ol, 0, 0, 1, 2, 1, 1, 1, 1);   

   of = e_widget_framelist_add(evas, _("Wallpaper Mode"), 0);
   e_widget_framelist_content_align_set(of, 0.0, 0.0);
   
   rg = e_widget_radio_group_new(&(cfdata->bg_mode));
   o = e_widget_radio_add(evas, _("Theme Defined"), 0, rg);
   cfdata->o_bg_mode_theme = o;
   evas_object_smart_callback_add(o, "changed",
				  _bg_mode, cfdata);
   e_widget_framelist_object_append(of, o);
   o = e_widget_radio_add(evas, _("Theme Wallpaper"), 1, rg);
   cfdata->o_bg_mode_bg = o;
   evas_object_smart_callback_add(o, "changed",
				  _bg_mode, cfdata);
   e_widget_framelist_object_append(of, o);
   o = e_widget_radio_add(evas, _("Custom"), 2, rg);
   cfdata->o_bg_mode_custom = o;
   evas_object_smart_callback_add(o, "changed",
				  _bg_mode, cfdata);
   e_widget_framelist_object_append(of, o);
   
   
#ifdef HAVE_PAM
   e_widget_table_object_append(ot, of, 0, 2, 1, 2 ,1 ,1 ,1 ,1);
#else
   e_widget_table_object_append(ot, of, 0, 2, 1, 1 ,1 ,1 ,1 ,1);
#endif

   of = e_widget_list_add(evas, 0, 0);
   
   o = e_widget_preview_add(evas, 200, (200 * z->h) / z->w);
   cfdata->o_preview = o;
   if (cfdata->bg_mode == 0)
     {
        f = e_theme_edje_file_get("base/theme/desklock", "e/desklock/background");
	if (cfdata->o_preview)
	  e_widget_preview_edje_set(cfdata->o_preview, f, "e/desktop/background");
	E_FREE(cfdata->bg);
     }
   else if (cfdata->bg_mode == 1)
     {
        f = e_theme_edje_file_get("base/theme/backgrounds", "e/desktop/background");
	if (cfdata->o_preview)
	  e_widget_preview_edje_set(cfdata->o_preview, f, "e/desktop/background");
	E_FREE(cfdata->bg);
	cfdata->bg = strdup("theme_background");
     }
   else if (cfdata->bg_mode == 2)
     {
	f = cfdata->bg;
	e_widget_preview_edje_set(o, f, "e/desktop/background");
     }
   e_widget_list_object_append(of, o, 1, 0, 0.5);
   
   e_widget_table_object_append(ot, of, 1, 0, 1, 1, 1, 1, 1, 1);
   
   /* start: login box options */
   
   /* Actually I do not know if I have to enable this if. However, if it is enabled
    * this options will be seen only for those peoples who has xinerama.
    * Otherwise, all the other world will not even know about them.
    * Let the world know about them. Maybe in the feature somebody will turn this if
    */
   
   if ((1) || (_e_desklock_zone_num_get() > 1))
     {
	of = e_widget_framelist_add(evas, _("Login Box Settings"), 0);
	rg = e_widget_radio_group_new((int *)(&(cfdata->login_box_zone)));
	
	ob = e_widget_radio_add(evas, _("Show on all screen zones"), LOGINBOX_SHOW_ALL_SCREENS,
				rg);
	e_widget_on_change_hook_set(ob, _e_desklock_cb_lb_show_change, cfdata);
	cfdata->gui.loginbox_obj.show_all_screens = ob;
	if (cfdata->zone_count == 1) e_widget_disabled_set(ob, 1);
	e_widget_framelist_object_append(of, ob);
	
	ob = e_widget_radio_add(evas, _("Show on current screen zone"),
				LOGINBOX_SHOW_CURRENT_SCREENS, rg);
	e_widget_on_change_hook_set(ob, _e_desklock_cb_lb_show_change, cfdata);
	cfdata->gui.loginbox_obj.show_current_screen = ob;
	if (cfdata->zone_count == 1) e_widget_disabled_set(ob, 1);
	e_widget_framelist_object_append(of, ob);
	
	ob = e_widget_radio_add(evas, _("Show on screen zone #:"), LOGINBOX_SHOW_SPECIFIC_SCREEN,
				rg);
	e_widget_on_change_hook_set(ob, _e_desklock_cb_lb_show_change, cfdata);
	cfdata->gui.loginbox_obj.show_specific_screen = ob;
	if (cfdata->zone_count == 1) e_widget_disabled_set(ob, 1);
	e_widget_framelist_object_append(of, ob);
	
	ob = e_widget_slider_add(evas, 1, 0, _("%1.0f"), 0.0, (double)(cfdata->zone_count - 1),
				 1.0, 0, NULL, &(cfdata->specific_lb_zone), 100);
	cfdata->gui.loginbox_obj.screen_slider = ob;
	if (cfdata->zone_count == 1 || cfdata->login_box_zone == LOGINBOX_SHOW_ALL_SCREENS)
	  e_widget_disabled_set(ob, 1);
	e_widget_framelist_object_append(of, ob);
	e_widget_table_object_append(ot, of, 1, 1, 1, 1, 1, 1, 1, 1);
     }
   /* end: login box options */

   /*
#ifdef HAVE_PAM
   of = e_widget_framelist_add(evas, _("Password Type"), 0);
   
   rg_auth = e_widget_radio_group_new((int*)(&cfdata->auth_method));
   
   oc = e_widget_radio_add(evas, _("Use my login password"), 0, rg_auth);
   e_widget_on_change_hook_set(oc, _e_desklock_cb_auth_method_change, cfdata);
   e_widget_framelist_object_append(of, oc);
   
   oc = e_widget_radio_add(evas, _("Personalized password"), 1, rg_auth);
   e_widget_on_change_hook_set(oc, _e_desklock_cb_auth_method_change, cfdata);
   e_widget_framelist_object_append(of,oc);
   
   e_widget_table_object_append(ot, of, 0, 2, 1, 1, 1, 1, 1, 1);
#endif
   
   of = e_widget_framelist_add(evas, _("Personalized Password:"), 0);
   cfdata->gui.passwd_field = ob = e_widget_entry_add(evas, &(cfdata->desklock_passwd));
#ifdef HAVE_PAM
   if (cfdata->auth_method == 0) e_widget_disabled_set(ob, 1);
#endif
   
   e_widget_entry_password_set(ob, !cfdata->show_password);
   e_widget_min_size_set(ob, 200, 25);
   e_widget_framelist_object_append(of, ob);
   
   ob = e_widget_check_add(evas, _("Show password"), &(cfdata->show_password));
   e_widget_on_change_hook_set(ob, _e_desklock_cb_show_passwd, cfdata);
   cfdata->gui.show_passwd_check = ob;
#ifdef HAVE_PAM
   if (cfdata->auth_method == 0) e_widget_disabled_set(ob, 1);
#endif
   e_widget_framelist_object_append(of, ob);
   
#ifdef HAVE_PAM
   e_widget_table_object_append(ot, of, 0, 3, 1, 1, 1, 1, 1, 1);
#else
   e_widget_table_object_append(ot, of, 0, 2, 1, 1, 1, 1, 1, 1);
#endif
   */
   
   of = e_widget_framelist_add(evas, _("Automatic Locking"), 0);
   
   e_widget_disabled_set(of, !ecore_x_screensaver_event_available_get());
   
   ob = e_widget_check_add(evas, _("Enable screensaver"), &(cfdata->use_timeout));
   e_widget_disabled_set(ob, !ecore_x_screensaver_event_available_get());
   e_widget_framelist_object_append(of, ob);
   
   ob = e_widget_check_add(evas, _("Lock when the screensaver starts"), &(cfdata->autolock));
   e_widget_disabled_set(ob, !ecore_x_screensaver_event_available_get());
   e_widget_framelist_object_append(of, ob);
   
   ob = e_widget_label_add(evas, _("Time until screensaver starts"));
   e_widget_disabled_set(ob, !ecore_x_screensaver_event_available_get());
   e_widget_framelist_object_append(of, ob);
   ob = e_widget_slider_add(evas, 1, 0, _("%1.0f seconds"),
			    1.0, 600.0,
			    1.0, 0, &(cfdata->timeout), NULL,
			    100);
   e_widget_framelist_object_append(of, ob);
#ifdef HAVE_PAM
   e_widget_table_object_append(ot, of, 1, 2, 1, 2 ,1 ,1 ,1 ,1);
#else
   e_widget_table_object_append(ot, of, 1, 2, 1, 1 ,1 ,1 ,1 ,1);
#endif
   
   e_dialog_resizable_set(cfd->dia, 0);
   return ot;
}


/* general functionality/callbacks */

static void
_e_desklock_cb_show_passwd(void *data, Evas_Object *obj)
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = data;
   e_widget_entry_password_set(cfdata->gui.passwd_field, !cfdata->show_password);
}

static int
_e_desklock_zone_num_get(void)
{
   int num;
   Evas_List *l, *l2;
   
   num = 0;
   for (l = e_manager_list(); l; l = l->next)
     {
	E_Manager *man = l->data;
	
	for (l2 = man->containers; l2; l2 = l2->next)
	  {
	     E_Container *con = l2->data;
	     
	     num += evas_list_count(con->zones);
	  }
     }
   return num;
}

static void
_e_desklock_cb_lb_show_change(void *data, Evas_Object *obj)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;
   if (cfdata->login_box_zone == LOGINBOX_SHOW_ALL_SCREENS)
     {
	cfdata->specific_lb_zone_backup = cfdata->specific_lb_zone;
	e_widget_disabled_set(cfdata->gui.loginbox_obj.screen_slider, 1);
     }
   else if (cfdata->login_box_zone == LOGINBOX_SHOW_CURRENT_SCREENS)
     {
	cfdata->specific_lb_zone_backup = cfdata->specific_lb_zone;
	e_widget_disabled_set(cfdata->gui.loginbox_obj.screen_slider, 1);
     }
   else if (cfdata->login_box_zone == LOGINBOX_SHOW_SPECIFIC_SCREEN)
     {
	e_widget_slider_value_int_set(cfdata->gui.loginbox_obj.screen_slider, cfdata->specific_lb_zone_backup);
	e_widget_disabled_set(cfdata->gui.loginbox_obj.screen_slider, 0);
     }
}

#ifdef HAVE_PAM
static void
_e_desklock_cb_auth_method_change(void *data, Evas_Object *obj)
{
   E_Config_Dialog_Data *cfdata;

   cfdata = data;
   if (cfdata->auth_method == 0)
     {
	e_widget_disabled_set(cfdata->gui.passwd_field, 1);
	e_widget_disabled_set(cfdata->gui.show_passwd_check, 1);
     }
   else if (cfdata->auth_method == 1)
     {
	e_widget_disabled_set(cfdata->gui.passwd_field, 0);
	e_widget_disabled_set(cfdata->gui.show_passwd_check, 0);
     }
}
#endif
