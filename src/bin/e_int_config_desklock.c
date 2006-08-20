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
   
   char *bg; // local variable;
   Evas *evas; // local variable
   Evas_Object *preview_image; // local variable

#ifdef HAVE_PAM
   int auth_method;
#endif
   
   struct {
      Evas_Object *passwd_field;
      Evas_Object *bg_list;

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
_fill_desklock_data(E_Config_Dialog_Data *cfdata)
{
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
   
   if (!e_config->desklock_background)
     cfdata->bg = strdup(DEF_DESKLOCK_BACKGROUND);
   else
     cfdata->bg = strdup(e_config->desklock_background);

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

  _fill_desklock_data(cfdata);

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
   e_config->desklock_personal_passwd = (char *)evas_stringshare_add(cfdata->desklock_passwd_cp);
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
   
   //_fill_desklock_data(cfdata);
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
   e_config->desklock_personal_passwd = (char *)evas_stringshare_add(cfdata->desklock_passwd_cp);

   if (cfdata->bg)
     {
	if (e_config->desklock_background)
	  evas_stringshare_del(e_config->desklock_background);
	e_config->desklock_background = (char *)evas_stringshare_add(cfdata->bg);
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
   Evas_Object *of, *ob, *oc;
   E_Radio_Group *rg;
   Evas_Object *ot;
#ifdef HAVE_PAM
   E_Radio_Group *rg_auth;
#endif
   
   cfdata->evas = evas;
   
   ot = e_widget_table_add(evas, 0);
   
   /* start: bkg list */
   cfdata->gui.bg_list = e_widget_ilist_add(evas, BG_LIST_ICON_SIZE_W,
					    BG_LIST_ICON_SIZE_H, &(cfdata->bg));
   
   e_widget_ilist_selector_set(cfdata->gui.bg_list, 1);
   e_widget_min_size_set(cfdata->gui.bg_list, 180, 200);
   
   _load_bgs(cfdata);
   
   e_widget_focus_set(cfdata->gui.bg_list, 1);
   e_widget_ilist_go(cfdata->gui.bg_list);
   
   e_widget_table_object_append(ot, cfdata->gui.bg_list, 0, 0, 1, 2, 1, 1, 1, 1);
   /* end: bkg list */
   
   /* start: Desk Lock Window Preview */
   
   e_widget_table_object_append(ot, cfdata->preview_image, 1, 0, 1, 1, 1, 1, 1, 1);
   /* end: Desk Lock Window Preview */
   
   /* start: login box options */
   
   /* Actually I do not know if I have to enable this if. However, if it is enabled
    * this options will be seen only for those peoples who has xinerama.
    * Otherwise, all the other world will not even know about them.
    * Let the world know about them. Maybe in the feature somebody will turn this if
    */
   
   if (1 || _e_desklock_zone_num_get() > 1)
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
			    200);
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
_load_bgs(E_Config_Dialog_Data *cfdata)
{
   E_Config_Dialog *cfd;
   Evas_Object *o, *ic, *im;
   Ecore_Evas *eebuf;
   Evas *evasbuf;
   Evas_List *bg_dirs, *bg;
   const char *f, *f1;
   char *c;
   
   if (!cfdata || !cfdata->gui.bg_list)
     return;
   
   cfd = cfdata->cfd;
   eebuf = ecore_evas_buffer_new(1, 1);
   evasbuf = ecore_evas_get(eebuf);
   
   ic = NULL;
   /* Desklock background */
   o = edje_object_add(evasbuf);
   f1 = e_theme_edje_file_get("base/theme/desklock", "desklock/background");
   c = strdup(f1);
   
   e_widget_ilist_header_append(cfdata->gui.bg_list, NULL, _("Theme Backgrounds"));
   if (edje_object_file_set(o, f1, "desklock/background"))
     {
        ic = e_thumb_icon_add(cfd->dia->win->evas);
	e_thumb_icon_file_set(ic, (char *)f1, "desktop/background");
	e_thumb_icon_size_set(ic, 64,
			       (64 * e_zone_current_get(cfd->dia->win->container)->h) /
			       e_zone_current_get(cfd->dia->win->container)->w);
	e_thumb_icon_begin(ic);
	e_widget_ilist_append(cfdata->gui.bg_list, ic, _("Theme Desklock Background"),
			      _ibg_list_cb_bg_selected, cfdata, DEF_DESKLOCK_BACKGROUND);
     }
   
   if ((!e_config->desklock_background) ||
       (!strcmp(e_config->desklock_background, DEF_DESKLOCK_BACKGROUND)))
     e_widget_ilist_selected_set(cfdata->gui.bg_list, 1);
   
   im = e_widget_preview_add(cfdata->evas, BG_PREVIEW_W, BG_PREVIEW_H);
   e_widget_preview_edje_set(im, c, "desklock/background");
   
   evas_object_del(o);
   ecore_evas_free(eebuf);
   free(c);
   /* end: Desklock background */
   
   /* Theme Background */
   eebuf = ecore_evas_buffer_new(1, 1);
   evasbuf = ecore_evas_get(eebuf);
   
   o = edje_object_add(evasbuf);
   f = e_theme_edje_file_get("base/theme/backgrounds", "desktop/background");
   c = strdup(f);
   ic = NULL;
   if (edje_object_file_set(o, f, "desktop/background"))
     {
        ic = e_thumb_icon_add(cfd->dia->win->evas);
	e_thumb_icon_file_set(ic, (char *)f, "desktop/background");
	e_thumb_icon_size_set(ic, 64,
			       (64 * e_zone_current_get(cfd->dia->win->container)->h) /
			       e_zone_current_get(cfd->dia->win->container)->w);
	e_thumb_icon_begin(ic);
	e_widget_ilist_append(cfdata->gui.bg_list, ic, _("Theme Background"),
			      _ibg_list_cb_bg_selected, cfdata, DEF_THEME_BACKGROUND);
     }
   
   if ((e_config->desklock_background) &&
       (!strcmp(e_config->desklock_background, DEF_THEME_BACKGROUND)))
     {
	e_widget_ilist_selected_set(cfdata->gui.bg_list, 2);
	evas_object_del(im);
	im = e_widget_preview_add(cfdata->evas, BG_PREVIEW_W, BG_PREVIEW_H);
	e_widget_preview_edje_set(im, c, "desktop/background");
     }
   
   evas_object_del(o);
   ecore_evas_free(eebuf);
   free(c);
   
   e_widget_ilist_header_append(cfdata->gui.bg_list, NULL, _("Personal"));
   bg_dirs = e_path_dir_list_get(path_backgrounds);
   for (bg = bg_dirs; bg; bg = bg->next)
     {
	E_Path_Dir *d;
	
	d = bg->data;
	if (ecore_file_is_dir(d->dir))
	  {
	     char *bg_file;
	     Ecore_List *bgs;
	     int i = e_widget_ilist_count(cfdata->gui.bg_list);
	     
	     bgs = ecore_file_ls(d->dir);
	     if (!bgs) continue;
	     while ((bg_file = ecore_list_next(bgs)))
	       {
		  char full_path[4096];
		  
		  snprintf(full_path, sizeof(full_path), "%s/%s", d->dir, bg_file);
		  if (ecore_file_is_dir(full_path)) continue;
		  if (!e_util_edje_collection_exists(full_path, "desktop/background")) continue;
		  
		  ic = e_thumb_icon_add(cfd->dia->win->evas);
		  e_thumb_icon_file_set(ic, full_path, "desktop/background");
		  e_thumb_icon_size_set(ic, 64,
					 (64 * e_zone_current_get(cfd->dia->win->container)->h) /
					 e_zone_current_get(cfd->dia->win->container)->w);
		  e_thumb_icon_begin(ic);
		  e_widget_ilist_append(cfdata->gui.bg_list, ic, ecore_file_strip_ext(bg_file),
					_ibg_list_cb_bg_selected, cfdata, full_path);
		  
		  if ((e_config->desklock_background) &&
		      (!strcmp(e_config->desklock_background, full_path)))
		    {
		       e_widget_ilist_selected_set(cfdata->gui.bg_list, i);
		       evas_object_del(im);
		       im = e_widget_preview_add(cfdata->evas, BG_PREVIEW_W, BG_PREVIEW_H);
		       e_widget_preview_edje_set(im, full_path, "desktop/background");
		    }
		  i++;
	       }
	     free(bg_file);
	     ecore_list_destroy(bgs);
	  }
	free(d);
     }
   evas_list_free(bg);
   evas_list_free(bg_dirs);
   
   cfdata->preview_image = im;
}

static void
_ibg_list_cb_bg_selected(void *data)
{
   E_Config_Dialog_Data *cfdata;
   
   cfdata = data;
   if (cfdata->bg[0])
     {
	if (!strcmp(cfdata->bg, DEF_DESKLOCK_BACKGROUND))
	  {
	     const char *theme;
	     
	     theme = e_theme_edje_file_get("base/theme/desklock", "desklock/background");
	     e_widget_preview_edje_set(cfdata->preview_image, theme, "desklock/background");
	  }
	else if (!strcmp(cfdata->bg, DEF_THEME_BACKGROUND))
	  {
	     const char *theme;
	     
	     theme = e_theme_edje_file_get("base/theme/backgrounds", "desktop/background");
	     e_widget_preview_edje_set(cfdata->preview_image, theme, "desktop/background");
	  }
	else
	  e_widget_preview_edje_set(cfdata->preview_image, cfdata->bg, "desktop/background");
     }
   else
     {
	const char *theme;
	
	theme = e_theme_edje_file_get("base/theme/desklock", "desklock/background");
	e_widget_preview_edje_set(cfdata->preview_image, theme, "desklock/background");
     }
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
